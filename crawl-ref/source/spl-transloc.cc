/**
 * @file
 * @brief Translocation spells.
**/

#include "AppHdr.h"

#include "spl-transloc.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "abyss.h"
#include "act-iter.h"
#include "areas.h"
#include "cloud.h"
#include "colour.h"
#include "coordit.h"
#include "delay.h"
#include "directn.h"
#include "dungeon.h"
#include "english.h"
#include "item-prop.h"
#include "items.h"
#include "level-state-type.h"
#include "libutil.h"
#include "los.h"
#include "losglobal.h"
#include "losparam.h"
#include "message.h"
#include "mon-behv.h"
#include "mon-tentacle.h"
#include "mon-util.h"
#include "nearby-danger.h"
#include "orb.h"
#include "output.h"
#include "prompt.h"
#include "religion.h"
#include "shout.h"
#include "spl-util.h"
#include "stash.h"
#include "state.h"
#include "stringutil.h"
#include "target.h"
#include "teleport.h"
#include "terrain.h"
#include "tiledoll.h"
#include "traps.h"
#include "view.h"
#include "viewmap.h"
#include "xom.h"

/**
 * Place a cloud of translocational energy at a player's previous location,
 * to make it easier for players to tell what just happened.
 *
 * @param origin    The player's previous location.
 */
static void _place_tloc_cloud(const coord_def &origin)
{
    if (!cell_is_solid(origin))
        place_cloud(CLOUD_TLOC_ENERGY, origin, 1 + random2(3), &you);
}

spret cast_disjunction(int pow, bool fail)
{
    fail_check();
    int rand = random_range(35, 45) + random2(pow / 12);
    you.duration[DUR_DISJUNCTION] = min(90 + pow / 12,
        max(you.duration[DUR_DISJUNCTION] + rand,
        30 + rand));
    contaminate_player(750 + random2(500), true);
    disjunction_spell();
    return spret::success;
}

void disjunction_spell()
{
    int steps = you.time_taken;
    invalidate_agrid(true);
    for (int step = 0; step < steps; ++step)
    {
        vector<monster*> mvec;
        for (radius_iterator ri(you.pos(), LOS_RADIUS, C_SQUARE); ri; ++ri)
        {
            monster* mons = monster_at(*ri);
            if (!mons || !you.see_cell(*ri))
                continue;
            mvec.push_back(mons);
        }
        if (mvec.empty())
            return;
        // blink should be isotropic
        shuffle_array(mvec);
        for (auto mons : mvec)
        {
            if (!mons->alive() || mons->no_tele())
                continue;
            coord_def p = mons->pos();
            if (!disjunction_haloed(p))
                continue;

            int dist = grid_distance(you.pos(), p);
            int decay = max(1, (dist - 1) * (dist + 1));
            int chance = pow(0.8, 1.0 / decay) * 1000;
            if (!x_chance_in_y(chance, 1000))
                blink_away(mons, &you, false);
        }
    }
}

/**
 * Attempt to blink the player to a random nearby tile.
 *
 * @param override_stasis       Whether to blink even if the player is under
 *                              stasis (& thus normally unable to).
 */
void uncontrolled_blink(bool override_stasis, coord_def disp_center)
{
    if (you.no_tele(true, true, true) && !override_stasis)
    {
        canned_msg(MSG_STRANGE_STASIS);
        return;
    }

    bool needs_msg = (!disp_center.origin() && !you.see_cell(disp_center));

    bool adjacent = true;
    int positions = 0;
    coord_def target = coord_def (0, 0);

    for (rectangle_iterator ri(you.pos(), you.current_vision); ri; ++ri)
    {
        if (!in_bounds(*ri))
            continue;

        if (!you.see_cell_no_trans(*ri))
            continue;

        if (!you.can_pass_through(*ri))
            continue;

        if (*ri == disp_center)
            continue;

        if (actor_at(*ri))
            continue;

        if (!disp_center.origin())
        {
            if ((grid_distance(*ri, disp_center) > you.current_vision) || !(cell_see_cell(*ri, disp_center, LOS_SOLID))
                || grd(*ri) == DNGN_TRAP_CONGREGATION) // no chain dispersal
            {
                continue;
            }
        }

        if (!adjacent && (grid_distance(you.pos(), *ri) < 2))
            continue;

        if (one_chance_in(++positions))
        {
            target = *ri;
            if (grid_distance(you.pos(), *ri) > 1)
                adjacent = false;
        }
    }

    if (target.origin())
    {
        if (disp_center.origin())
            mpr("You feel jittery for a moment.");
        return;
    }

    if (you.is_constricted() && needs_msg)
        mpr("You feel yourself being warped!");

    if (!you.attempt_escape(2)) // prints its own messages
        return;

    if (needs_msg)
        mpr("Yoink! You're pulled towards a congregation trap!");
    else
        canned_msg(MSG_YOU_BLINK);
    const coord_def origin = you.pos();
    move_player_to_grid(target, false);
    _place_tloc_cloud(origin);
}

/**
 * Let the player choose a destination for their controlled blink or similar
 * effect.
 *
 * @param target[out]   The target found, if any.
 * @param safe_cancel   Whether it's OK to let the player cancel the control
 *                      of the blink (or whether there should be a prompt -
 *                      for e.g. ?blink with blurryvis)
 * @param verb          What kind of movement is this, exactly?
 *                      (E.g. 'blink', 'hop'.)
 * @param hitfunc       A hitfunc passed to the direction_chooser.
 * @return              True if a target was found; false if the player aborted.
 */
static bool _find_cblink_target(coord_def &target, bool safe_cancel,
                                string verb, targeter *hitfunc = nullptr)
{
    while (true)
    {
        // query for location {dlb}:
        direction_chooser_args args;
        args.restricts = DIR_TARGET;
        args.needs_path = false;
        args.top_prompt = uppercase_first(verb) + " to where?";
        args.hitfunc = hitfunc;
        dist beam;
        direction(beam, args);

        if (crawl_state.seen_hups)
        {
            mprf("Cancelling %s due to HUP.", verb.c_str());
            return false;
        }

        if (!beam.isValid || beam.target == you.pos())
        {
            const string prompt =
                "Are you sure you want to cancel this " + verb + "?";
            if (!safe_cancel && !yesno(prompt.c_str(), false, 'n'))
            {
                clear_messages();
                continue;
            }

            canned_msg(MSG_OK);
            return false;
        }

        const monster* beholder = you.get_beholder(beam.target);
        if (beholder)
        {
            mprf("You cannot %s away from %s!",
                 verb.c_str(),
                 beholder->name(DESC_THE, true).c_str());
            continue;
        }

        const monster* fearmonger = you.get_fearmonger(beam.target);
        if (fearmonger)
        {
            mprf("You cannot %s closer to %s!",
                 verb.c_str(),
                 fearmonger->name(DESC_THE, true).c_str());
            continue;
        }

        if (cell_is_solid(beam.target))
        {
            clear_messages();
            mprf("You can't %s into that!", verb.c_str());
            continue;
        }

        monster* target_mons = monster_at(beam.target);
        if (target_mons && you.can_see(*target_mons))
        {
            mprf("You can't %s onto %s!", verb.c_str(),
                 target_mons->name(DESC_THE).c_str());
            continue;
        }

        if (!check_moveto(beam.target, verb))
        {
            continue;
            // try again (messages handled by check_moveto)
        }

        if (!you.see_cell_no_trans(beam.target))
        {
            clear_messages();
            if (you.trans_wall_blocking(beam.target))
                canned_msg(MSG_SOMETHING_IN_WAY);
            else
                canned_msg(MSG_CANNOT_SEE);
            continue;
        }

        target = beam.target; // Grid in los, no problem.
        return true;
    }
}

void wizard_blink()
{
    // query for location {dlb}:
    direction_chooser_args args;
    args.restricts = DIR_TARGET;
    args.needs_path = false;
    targeter_smite tgt(&you, LOS_RADIUS);
    tgt.obeys_mesmerise = false;
    args.hitfunc = &tgt;

    args.top_prompt = "Blink to where?";
    dist beam;
    direction(beam, args);

    if (!beam.isValid || beam.target == you.pos())
    {
        canned_msg(MSG_OK);
        return;
    }

    if (!in_bounds(beam.target))
    {
        clear_messages();
        mpr("Please don't blink into the map border.");
        return wizard_blink();
    }

    if (monster_at(beam.target))
    {
        clear_messages();
        mpr("Please don't try to blink into monsters.");
        return wizard_blink();
    }

    if (!check_moveto(beam.target, "blink"))
    {
        return wizard_blink();
        // try again (messages handled by check_moveto)
    }

    // Allow wizard blink to send player into walls, in case the
    // user wants to alter that grid to something else.
    if (cell_is_solid(beam.target))
        grd(beam.target) = DNGN_FLOOR;

    move_player_to_grid(beam.target, false);
}

static const int HOP_FUZZ_RADIUS = 2;
static const int SPIDER_FUZZ_RADIUS = 1;

/**
 * Randomly choose one of the spaces near the given target for the player's hop
 * to land on.
 *
 * @param target    The tile the player wants to land on.
 * @return          A nearby, unoccupied, inhabitable tile.
 */
static coord_def _fuzz_hop_destination(coord_def target, int radius)
{
    coord_def chosen;
    int seen = 0;
    for (radius_iterator ri(target, radius, C_SQUARE, LOS_NO_TRANS);
         ri; ++ri)
    {
        if (valid_blink_destination(&you, *ri) && one_chance_in(++seen))
            chosen = *ri;
    }
    return chosen;
}

/**
 * Attempt to hop the player to a space near a tile of their choosing.
 *
 * @param fail          Whether this came from a mis-invoked ability (& should
 *                      therefore fail after selecting a target)
 * @return              Whether the hop succeeded, aborted, or was miscast.
 */
spret frog_hop(bool fail, bool spider)
{
    const int splpow = calc_spell_power(SPELL_SUMMON_SPIDER_MOUNT, true);

    const int hop_range = spider ? (2 + div_round_up(splpow, 50)) // 3-6.
                                 : (2 + you.get_mutation_level(MUT_FROG_LEGS) * 2); // 4-6
    coord_def target;
    targeter_smite tgt(&you, hop_range, 0, spider ? SPIDER_FUZZ_RADIUS : HOP_FUZZ_RADIUS);
    tgt.obeys_mesmerise = true;
    while (true)
    {
        if (!_find_cblink_target(target, true, spider ? "jump" : "hop", &tgt))
            return spret::abort;
        if (grid_distance(you.pos(), target) > hop_range)
        {
            mpr("That's out of range!"); // ! targeting
            continue;
        }
        break;
    }
    target = _fuzz_hop_destination(target, spider ? SPIDER_FUZZ_RADIUS : HOP_FUZZ_RADIUS);

    fail_check();

    if (!you.attempt_escape(2)) // XXX: 1?
        return spret::success; // of a sort

    // invisible monster that the targeter didn't know to avoid, or similar
    if (target.origin())
    {
        mprf("You%s tried to %s, but there was no room to land!", 
                spider ? "r spider" : "", spider ? "jump" : "hop");
        // TODO: what to do here?
        return spret::success; // of a sort
    }

    if (!cell_is_solid(you.pos())) // should be safe.....
        place_cloud(CLOUD_DUST, you.pos(), 2 + random2(3), &you);
    move_player_to_grid(target, false);
    crawl_state.cancel_cmd_again();
    crawl_state.cancel_cmd_repeat();
    if (spider)
    {
        mpr("Woah!");
        you.increase_duration(DUR_MOUNT_BREATH, 1 + random2(26 - div_rand_round(splpow, 10)));
    }
    else
    {
        mpr("Boing!");
        you.increase_duration(DUR_NO_HOP, 12 + random2(13));
    }

    return spret::success; // TODO
}

spret blink_bolt(bool fail, int power)
{
    coord_def target;
    targeter_beam tgt(&you, 6, ZAP_SLIMEBOLT, power, 0, 0);
    tgt.obeys_mesmerise = true;
    if (!_find_cblink_target(target, true, "cascade", &tgt))
        return spret::abort;

    bolt beam;
    beam.source = you.pos();
    beam.range = INFINITE_DISTANCE;
    beam.target = target;
    beam.aimed_at_spot = true;

    if (!player_tracer(ZAP_SLIMEBOLT, power, beam, INFINITE_DISTANCE))
        return spret::abort;

    fail_check();

    const int d = 6 + random2(3 + you.skill(SK_INVOCATIONS));
    temp_change_terrain(you.pos(), DNGN_SLIMY_WATER, d * BASELINE_DELAY, TERRAIN_CHANGE_SLIME);
    place_cloud(CLOUD_FIRE, you.pos(), d - 1, &you);

    beam.fire();

    move_player_to_grid(target, false);
    return spret::success;
}

/**
 * Attempt to blink the player to a nearby tile of their choosing.
 *
 * @param fail          Whether this came from a miscast spell (& should
 *                      therefore fail after selecting a target)
 * @param safe_cancel   Whether it's OK to let the player cancel the control
 *                      of the blink (or whether there should be a prompt -
 *                      for e.g. ?blink with blurryvis)
 * @return              Whether the blink succeeded, aborted, or was miscast.
 */
spret controlled_blink(bool fail, bool safe_cancel, int power)
{
    int range = min(LOS_RADIUS, (LOS_RADIUS * (power - 37)) / 63);
    range = max(1, range);
    coord_def target;
    targeter_smite tgt(&you, range);
    tgt.obeys_mesmerise = true;
    if (!_find_cblink_target(target, safe_cancel, "blink", &tgt))
        return spret::abort;

    fail_check();

    if (you.no_tele(true, true, true))
    {
        canned_msg(MSG_STRANGE_STASIS);
        return spret::success; // of a sort
    }

    if (!you.attempt_escape(2))
        return spret::success; // of a sort

    // invisible monster that the targeter didn't know to avoid
    if (monster_at(target))
    {
        mpr("Oops! There was something there already!");
        uncontrolled_blink();
        return spret::success; // of a sort
    }

    _place_tloc_cloud(you.pos());
    move_player_to_grid(target, false);
    // Controlling teleport contaminates the player. -- bwr
    contaminate_player(750 + random2(500), true);

    crawl_state.cancel_cmd_again();
    crawl_state.cancel_cmd_repeat();

    return spret::success;
}

// Is there dangerous terrain in LoS.
bool dangerous_terrain_seen()
{
    for (rectangle_iterator ri(you.pos(), 7, true); ri; ++ri)
    {
        if (you.see_cell_no_trans(*ri))
        {
            if (is_feat_dangerous(grd(*ri), false, true))
                return true;
        }
    }
    return false;
}

/**
 * Cast the player spell Blink.
 *
 * @param fail              Whether the player miscast the spell.
 * @return                  Whether the spell was successfully cast, aborted,
 *                          or miscast.
 */
spret cast_blink(bool fail, bool force)
{
    // effects that cast the spell through the player, I guess (e.g. xom)
    if (you.no_tele(false, false, true))
        return fail ? spret::fail : spret::success; // probably always SUCCESS

    if (!you.airborne() && dangerous_terrain_seen() && !force && !yesno("Really blink while near dangerous terrain?", false, 'n'))
    {
        canned_msg(MSG_OK);
        return spret::abort;
    }

    fail_check();
    uncontrolled_blink();
    return spret::success;
}

/**
 * Cast the player spell Controlled Blink.
 *
 * @param fail    Whether the player miscast the spell.
 * @param safe    Whether it's safe to abort (not e.g. unknown ?blink)
 * @return        Whether the spell was successfully cast, aborted, or miscast.
 */
spret cast_controlled_blink(bool fail, bool safe, int power)
{
    // don't prompt if it's useless
    if (you.no_tele(true, true, true))
    {
        canned_msg(MSG_STRANGE_STASIS);
        return spret::abort;
    }

    if (crawl_state.is_repeating_cmd())
    {
        crawl_state.cant_cmd_repeat("You can't repeat controlled blinks.");
        crawl_state.cancel_cmd_again();
        crawl_state.cancel_cmd_repeat();
        return spret::abort;
    }

    if (orb_limits_translocation())
    {
        if (!yesno("Your blink will be uncontrolled - continue anyway?",
                   false, 'n'))
        {
            return spret::abort;
        }

        if (!you.airborne() && dangerous_terrain_seen() && !yesno("Really make an uncontrolled blink while near dangerous terrain?", false, 'n'))
        {
            canned_msg(MSG_OK);
            return spret::abort;
        }

        mprf(MSGCH_ORB, "The Orb prevents control of your translocation!");
        return cast_blink(fail);
    }

    return controlled_blink(fail, safe, power);
}

void you_teleport()
{
    // [Cha] here we block teleportation, which will save the player from
    // death from read-id'ing scrolls (in sprint)
    if (you.no_tele(true, true))
        canned_msg(MSG_STRANGE_STASIS);
    else if (you.duration[DUR_TELEPORT])
    {
        mpr("You feel strangely stable.");
        you.duration[DUR_TELEPORT] = 0;
    }
    else
    {
        mpr("You feel strangely unstable.");

        int teleport_delay = 3 + random2(3);

        if (player_in_branch(BRANCH_ABYSS))
        {
            mpr("You feel the power of the Abyss delaying your translocation!");
            teleport_delay += 5 + random2(10);
        }
        else if (orb_limits_translocation())
        {
            mprf(MSGCH_ORB, "You feel the Orb delaying your translocation!");
            teleport_delay += 5 + random2(5);
        }

        you.set_duration(DUR_TELEPORT, teleport_delay);
    }
}

// Should return true if we don't want anyone to teleport here.
static bool _cell_vetoes_teleport(const coord_def cell, bool check_monsters = true,
                                  bool wizard_tele = false)
{
    // Monsters always veto teleport.
    if (monster_at(cell) && check_monsters)
        return true;

    // As do all clouds; this may change.
    if (cloud_at(cell) && !wizard_tele)
        return true;

    if (cell_is_solid(cell))
        return true;

    return is_feat_dangerous(grd(cell), true) && !wizard_tele;
}

static void _handle_teleport_update(bool large_change, const coord_def old_pos)
{
    if (large_change)
    {
        viewwindow();
        for (monster_iterator mi; mi; ++mi)
        {
            const bool see_cell = you.see_cell(mi->pos());

            if (mi->foe == MHITYOU && !see_cell && !you.penance[GOD_ASHENZARI])
            {
                mi->foe_memory = 0;
                behaviour_event(*mi, ME_EVAL);
            }
            else if (see_cell)
                behaviour_event(*mi, ME_EVAL);
        }
    }

#ifdef USE_TILE
    if (you.species == SP_MERFOLK || you.char_class == JOB_MERFOLK)
    {
        const dungeon_feature_type new_grid = grd(you.pos());
        const dungeon_feature_type old_grid = grd(old_pos);
        if (feat_is_water(old_grid) && !feat_is_water(new_grid)
            || !feat_is_water(old_grid) && feat_is_water(new_grid))
        {
            init_player_doll();
        }
    }
#else
    UNUSED(old_pos);
#endif
}

static bool _teleport_player(bool wizard_tele, bool teleportitis,
                             string reason="")
{
    if (!wizard_tele && !teleportitis
        && (crawl_state.game_is_sprint() || you.no_tele())
            && !player_in_branch(BRANCH_ABYSS))
    {
        if (!reason.empty())
            mpr(reason);
        canned_msg(MSG_STRANGE_STASIS);
        return false;
    }

    // After this point, we're guaranteed to teleport. Kill the appropriate
    // delays. Teleportitis needs to check the target square first, though.
    if (!teleportitis)
        interrupt_activity(activity_interrupt::teleport);

    // Update what we can see at the current location as well as its stash,
    // in case something happened in the exact turn that we teleported
    // (like picking up/dropping an item).
    viewwindow();
    StashTrack.update_stash(you.pos());

    if (player_in_branch(BRANCH_ABYSS) && !wizard_tele)
    {
        if (teleportitis)
            return false;

        if (!reason.empty())
            mpr(reason);
        abyss_teleport();
        if (you.pet_target != MHITYOU)
            you.pet_target = MHITNOT;

        return true;
    }

    coord_def pos(1, 0);
    const coord_def old_pos = you.pos();
    bool      large_change  = false;

    if (wizard_tele)
    {
        while (true)
        {
            level_pos lpos;
            bool chose = show_map(lpos, false, true, false);
            pos = lpos.pos;
            redraw_screen();

            // If we've received a HUP signal then the user can't choose a
            // location, so cancel the teleport.
            if (crawl_state.seen_hups)
            {
                mprf(MSGCH_ERROR, "Controlled teleport interrupted by HUP signal, "
                                  "cancelling teleport.");
                return false;
            }

            dprf("Target square (%d,%d)", pos.x, pos.y);

            if (!chose || pos == you.pos())
                return false;

            break;
        }

        if (!you.see_cell(pos))
            large_change = true;

        if (_cell_vetoes_teleport(pos, true, wizard_tele))
        {
            mprf(MSGCH_WARN, "Even you can't go there right now. Sorry!");
            return false;
        }
        else
            move_player_to_grid(pos, false);
    }
    else
    {
        coord_def newpos;
        int tries = 500;
        do
        {
            newpos = random_in_bounds();
        }
        while (--tries > 0
               && (_cell_vetoes_teleport(newpos)
                   || testbits(env.pgrid(newpos), FPROP_NO_TELE_INTO)));

        // Running out of tries shouldn't happen; no message. Return false so
        // it doesn't count as a random teleport for Xom purposes.
        if (tries == 0)
            return false;
        // Teleportitis requires a monster in LOS of the new location, else
        // it silently fails.
        else if (teleportitis)
        {
            int mons_near_target = 0;
            for (monster_near_iterator mi(newpos, LOS_NO_TRANS); mi; ++mi)
                if (mons_is_threatening(**mi) && mons_attitude(**mi) == ATT_HOSTILE)
                    mons_near_target++;
            if (!mons_near_target)
            {
                dprf("teleportitis: no monster near target");
                return false;
            }
            else if (you.no_tele())
            {
                if (!reason.empty())
                    mpr(reason);
                canned_msg(MSG_STRANGE_STASIS);
                return false;
            }
            else
            {
                interrupt_activity(activity_interrupt::teleport);
                if (!reason.empty())
                    mpr(reason);
                mprf("You are suddenly yanked towards %s nearby monster%s!",
                     mons_near_target > 1 ? "some" : "a",
                     mons_near_target > 1 ? "s" : "");
            }
        }

        if (newpos == old_pos)
            mpr("Your surroundings flicker for a moment.");
        else if (you.see_cell(newpos))
            mpr("Your surroundings seem slightly different.");
        else
        {
            mpr("Your surroundings suddenly seem different.");
            large_change = true;
        }

        // Leave a purple cloud.
        _place_tloc_cloud(old_pos);

        move_player_to_grid(newpos, false);
        stop_delay(true);
    }

    _handle_teleport_update(large_change, old_pos);
    return !wizard_tele;
}

bool you_teleport_to(const coord_def where_to, bool move_monsters)
{
    // Attempts to teleport the player from their current location to 'where'.
    // Follows this line of reasoning:
    //   1. Check the location (against _cell_vetoes_teleport), if valid,
    //      teleport the player there.
    //   2. If not because of a monster, and move_monster, teleport that
    //      monster out of the way, then teleport the player there.
    //   3. Otherwise, iterate over adjacent squares. If a sutiable position is
    //      found (or a monster can be moved out of the way, with move_monster)
    //      then teleport the player there.
    //   4. If not, give up and return false.

    const coord_def old_pos = you.pos();
    coord_def where = where_to;
    coord_def old_where = where_to;

    // Don't bother to calculate a possible new position if it's out of bounds.
    if (!in_bounds(where))
        return false;

    if (_cell_vetoes_teleport(where))
    {
        if (monster_at(where) && move_monsters && !_cell_vetoes_teleport(where, false))
        {
            // dlua only, don't heed no_tele
            monster* mons = monster_at(where);
            mons->teleport(true);
        }
        else
        {
            for (adjacent_iterator ai(where); ai; ++ai)
            {
                if (!_cell_vetoes_teleport(*ai))
                {
                    where = *ai;
                    break;
                }
                else
                {
                    if (monster_at(*ai) && move_monsters
                            && !_cell_vetoes_teleport(*ai, false))
                    {
                        monster* mons = monster_at(*ai);
                        mons->teleport(true);
                        where = *ai;
                        break;
                    }
                }
            }
            // Give up, we can't find a suitable spot.
            if (where == old_where)
                return false;
        }
    }

    // If we got this far, we're teleporting the player.
    _place_tloc_cloud(old_pos);

    bool large_change = you.see_cell(where);

    move_player_to_grid(where, false);

    _handle_teleport_update(large_change, old_pos);
    return true;
}

void you_teleport_now(bool wizard_tele, bool teleportitis, string reason)
{
    const bool randtele = _teleport_player(wizard_tele, teleportitis, reason);

    // Xom is amused by teleports that land you in a dangerous place, unless
    // the player is in the Abyss and teleported to escape from all the
    // monsters chasing him/her, since in that case the new dangerous area is
    // almost certainly *less* dangerous than the old dangerous area.
    if (randtele && !player_in_branch(BRANCH_ABYSS)
        && player_in_a_dangerous_place())
    {
        xom_is_stimulated(200);
    }
}

spret cast_portal_projectile(int pow, bool fail)
{
    fail_check();
    if (!you.duration[DUR_PORTAL_PROJECTILE])
        mpr("You begin teleporting projectiles to their destination.");
    else
        mpr("You renew your portal.");
    // Calculate the accuracy bonus based on current spellpower.
    you.attribute[ATTR_PORTAL_PROJECTILE] = pow;
    you.increase_duration(DUR_PORTAL_PROJECTILE, 3 + random2(pow / 2) + random2(pow / 5), 50);
    return spret::success;
}

spret cast_apportation(int pow, bool fail)
{
    vector<int> items_to_apport;
    bool want_destroy = false;
    bool seen = false;

    for (rectangle_iterator ri(you.pos(), LOS_RADIUS); ri; ++ri)
    {
        // It doesn't make sense to apport from your own feet.
        if (*ri == you.pos())
            continue;

        // Can't apport from places we can't see.
        if (!you.see_cell_no_trans(*ri))
            continue;

        // Let's look at the top item in that square...
        // And don't allow apporting from shop inventories.
        // Using visible_igrd takes care of deep water/lava where appropriate.
        const int item_idx = igrd(*ri);
        if (item_idx == NON_ITEM || !in_bounds(*ri))
            continue;

        item_def& item = mitm[item_idx];

        const int seen_int = you.visible_igrd(*ri);
        if (seen_int && seen_int == item_idx)
            seen = true;

        // Nets can be apported when they have a victim trapped.
        if (item_is_stationary(item) && !item_is_stationary_net(item))
            continue;

        items_to_apport.push_back(item_idx);
    }

    if (!seen) 
    {
        if (!yesno("There is nothing you can see to apport. Continue anyway?", true, 0))
            return spret::abort;
    }

    if (grd(you.pos()) == DNGN_TRAP_SHAFT)
    {
        if (!yesno("Apporting items while standing on a shaft will cause them to fall to a lower floor. Continue anyways?", true, 0))
            return spret::abort;
    }
    else if (feat_eliminates_items(grd(you.pos())))
    {
        string msg = make_stringf("Items apported to you now will be %s. Continue anyways?", feat_destroys_items(grd(you.pos())) ? "destroyed" : "unreachable without apporting elsewhere");
        if (!yesno(msg.c_str(), true, 0))
            return spret::abort;
        want_destroy = true;
    }

    // BCADDO: Perhaps intentionally allow items to fall into lava when NOT standing over it?

    fail_check();
    int items_prevented = 0;

    for (int x : items_to_apport)
    {
        item_def& item = mitm[x];
        coord_def where = item.pos;

        // We need to modify the item *before* we move it, because
        // move_top_item() might change the location, or merge
        // with something at our position.
        if (item_is_orb(item))
        {
            fake_noisy(30, where);

            // There's also a 1-in-3 flat chance of apport failing.
            if (one_chance_in(3))
            {
                orb_pickup_noise(where, 30,
                    "The Orb shrieks and becomes a dead weight against your magic!",
                    "The Orb lets out a furious burst of light and becomes "
                    "a dead weight against your magic!");
                return spret::success;
            }
            else // Otherwise it's just a noisy little shiny thing
            {
                orb_pickup_noise(where, 30,
                    "The Orb shrieks as your magic touches it!",
                    "The Orb lets out a furious burst of light as your magic touches it!");
                start_orb_run(CHAPTER_ANGERED_PANDEMONIUM, "Now pick up the Orb and get out of here!");
            }
        }

        // If we apport a net, free the monster under it.
        if (item_is_stationary_net(item))
        {
            free_stationary_net(x);
            mpr("The net rips apart by the force of your magic!");
            if (monster* mons = monster_at(where))
                mons->del_ench(ENCH_HELD, true);
        }

        bolt beam;
        beam.source = you.pos();
        beam.target = where;
        beam.colour = ETC_WARP;
        beam.range = LOS_RADIUS;

        beam.is_tracer = true;
        beam.aimed_at_spot = true;
        beam.affects_nothing = true;
        beam.fire();

        // Pop the item's location off the end
        beam.path_taken.pop_back();

        // The actual number of squares it needs to traverse to get to you.
        int dist = beam.path_taken.size();

        // The maximum number of squares the item will actually move, always
        // at least one square. Always has a chance to move the entirety of default
        // LOS (7), but only becomes certain at max power (50).
        int max_dist = max(1, min(LOS_RADIUS, random2(8) + div_rand_round(pow, 7)));

        dprf("Apport dist=%d, max_dist=%d", dist, max_dist);

        int location_on_path = max(-1, dist - max_dist);
        coord_def new_spot;
        if (location_on_path == -1)
            new_spot = you.pos();
        else
            new_spot = beam.path_taken[location_on_path];
        // Try to find safe terrain for the item.
        while (location_on_path < dist)
        {
            if (!feat_eliminates_items(grd(new_spot)) || want_destroy)
                break;
            location_on_path++;
            new_spot = beam.path_taken[location_on_path];
        }
        if (location_on_path == dist)
            items_prevented++;
        dprf("Apport: new spot is %d/%d", new_spot.x, new_spot.y);

        // Actually move the item.
        if (where != new_spot)
        {
            move_top_item(where, new_spot);

            // Effects!
            beam.is_tracer = false;
            beam.source = where;
            beam.target = new_spot;
            beam.seen = true;
            beam.fire();
        }

        // Mark the item as found now.
        origin_set(new_spot);
    }

    // Messaging.
    int items_apported = (items_to_apport.size() - items_prevented);
    if (!items_apported)
        mpr("Unfortunately, you failed to move any items for fear they'd be lost to the hazardous terrain in the way.");
    else
    { 
        mprf("Yoink! You pull %s item%s towards yourself.",
             number_in_words(items_apported).c_str(),
             (items_apported > 1) ? "s" : "");
        if (items_prevented)
            mprf("Unfortunately, %s item%s didn't move because of the hazardous terrain in the way.",
                number_in_words(items_prevented).c_str(),
                (items_prevented > 1) ? "s" : "");
    }

    return spret::success;
}

spret cast_golubrias_passage(const coord_def& where, bool fail)
{
    if (orb_limits_translocation())
    {
        mprf(MSGCH_ORB, "The Orb prevents you from opening a passage!");
        return spret::abort;
    }

    if (player_in_branch(BRANCH_GAUNTLET))
    {
        mprf(MSGCH_ORB, "A magic seal in the Gauntlet prevents you from "
                "opening a passage!");
        return spret::abort;
    }

    // randomize position a bit to make it not as useful to use on monsters
    // chasing you, as well as to not give away hidden trap positions
    int tries = 0;
    int tries2 = 0;
    coord_def randomized_where = where;
    coord_def randomized_here = you.pos();
    do
    {
        tries++;
        randomized_where = where;
        randomized_where.x += random_range(-2, 2);
        randomized_where.y += random_range(-2, 2);
    }
    while ((!in_bounds(randomized_where)
            || grd(randomized_where) != DNGN_FLOOR
            || monster_at(randomized_where)
            || !you.see_cell(randomized_where)
            || you.trans_wall_blocking(randomized_where)
            || randomized_where == you.pos())
           && tries < 100);

    do
    {
        tries2++;
        randomized_here = you.pos();
        randomized_here.x += random_range(-2, 2);
        randomized_here.y += random_range(-2, 2);
    }
    while ((!in_bounds(randomized_here)
            || grd(randomized_here) != DNGN_FLOOR
            || monster_at(randomized_here)
            || !you.see_cell(randomized_here)
            || you.trans_wall_blocking(randomized_here)
            || randomized_here == randomized_where)
           && tries2 < 100);

    if (tries >= 100 || tries2 >= 100)
    {
        if (you.trans_wall_blocking(randomized_where))
            mpr("You cannot create a passage on the other side of the transparent wall.");
        else
            // XXX: bleh, dumb message
            mpr("Creating a passage of Golubria requires sufficient empty space.");
        return spret::abort;
    }

    fail_check();
    place_specific_trap(randomized_where, TRAP_GOLUBRIA);
    place_specific_trap(randomized_here, TRAP_GOLUBRIA);
    env.level_state |= LSTATE_GOLUBRIA;

    trap_def *trap = trap_at(randomized_where);
    trap_def *trap2 = trap_at(randomized_here);
    if (!trap || !trap2)
    {
        mpr("Something buggy happened.");
        return spret::abort;
    }

    trap->reveal();
    trap2->reveal();

    return spret::success;
}

static int _disperse_monster(monster& mon, int pow)
{
    if (mon.no_tele())
        return false;

    if (mon.check_res_magic(pow) > 0)
        monster_blink(&mon, false);
    else
        monster_teleport(&mon, true);

    // Moving the monster may have killed it in apply_location_effects.
    if (mon.alive() && mon.check_res_magic(pow) <= 0)
        mon.confuse(&you, 1 + random2avg(pow / 10, 2));

    return true;
}

spret cast_dispersal(int pow, bool fail)
{
    fail_check();
    const int radius = spell_range(SPELL_DISPERSAL, pow);
    if (!apply_monsters_around_square([pow] (monster& mon) {
            return _disperse_monster(mon, pow);
        }, you.pos(), radius))
    {
        mpr("The air shimmers briefly around you.");
    }
    return spret::success;
}

int gravitas_range(int pow)
{
    return pow >= 80 ? 3 : 2;
}


#define GRAVITY "by gravitational forces"

static void _attract_actor(const actor* agent, actor* victim,
                           const coord_def pos, int pow, int strength)
{
    ASSERT(victim); // XXX: change to actor &victim
    const bool fedhas_prot = agent->deity() == GOD_FEDHAS
                             && victim->is_monster()
                             && fedhas_protects(victim->as_monster());

    ray_def ray;
    if (!find_ray(victim->pos(), pos, ray, opc_solid))
    {
        // This probably shouldn't ever happen, but just in case:
        if (you.can_see(*victim))
        {
            mprf("%s violently %s moving!",
                 victim->name(DESC_THE).c_str(),
                 victim->conj_verb("stop").c_str());
        }
        if (fedhas_prot)
        {
            simple_god_message(
                make_stringf(" protects %s from harm.",
                    agent->is_player() ? "your" : "a").c_str(), GOD_FEDHAS);
        }
        else
        {
            victim->hurt(agent, roll_dice(strength / 2, pow / 20),
                         BEAM_MMISSILE, KILLED_BY_BEAM, "", GRAVITY);
        }
        return;
    }

    for (int i = 0; i < strength; i++)
    {
        ray.advance();
        const coord_def newpos = ray.pos();

        if (!victim->can_pass_through_feat(grd(newpos)))
        {
            victim->collide(newpos, agent, pow);
            break;
        }
        else if (actor* act_at_space = actor_at(newpos))
        {
            if (victim != act_at_space)
                victim->collide(newpos, agent, pow);
            break;
        }
        else if (!victim->is_habitable(newpos))
            break;
        else
            victim->move_to_pos(newpos, false);

        if (victim->is_monster() && !fedhas_prot)
        {
            behaviour_event(victim->as_monster(),
                            ME_ANNOY, agent, agent ? agent->pos()
                                                   : coord_def(0, 0));
        }

        if (victim->pos() == pos)
            break;
    }
}

bool fatal_attraction(const coord_def& pos, const actor *agent, int pow)
{
    bool affected = false;
    for (actor_near_iterator ai(pos, LOS_SOLID); ai; ++ai)
    {
        if (*ai == agent || ai->is_stationary() || ai->pos() == pos)
            continue;

        const int range = (pos - ai->pos()).rdist();
        if (range > gravitas_range(pow))
            continue;

        const int strength = ((pow + 100) / 20) / (range*range);

        affected = true;
        _attract_actor(agent, *ai, pos, pow, strength);
    }

    return affected;
}

spret cast_gravitas(int pow, const coord_def& where, bool fail)
{
    if (cell_is_solid(where))
    {
        canned_msg(MSG_UNTHINKING_ACT);
        return spret::abort;
    }

    fail_check();

    monster* mons = monster_at(where);

    mprf("Gravity reorients around %s.",
         mons                      ? mons->name(DESC_THE).c_str() :
         feat_is_solid(grd(where)) ? feature_description(grd(where),
                                                         NUM_TRAPS, "",
                                                         DESC_THE, false)
                                                         .c_str()
                                   : "empty space");
    fatal_attraction(where, &you, pow);
    return spret::success;
}

/**
 * Where is the closest point along the given path to its source that the given
 * actor can be moved to?
 *
 * @param beckoned      The actor to be moved.
 * @param path          The path for the actor to be moved along
 * @return              The closest point for the actor to be moved to;
 *                      guaranteed to be on the path or its original location.
 */
static coord_def _beckon_destination(const coord_def &origin, const actor &beckoned, const bolt &path, int pow, const actor &agent)
{
    if (beckoned.is_stationary()  // don't move statues, etc
        || mons_is_tentacle_or_tentacle_segment(beckoned.type)) // a mess...
    {
        return beckoned.pos();
    }

    pow -= 6;
    int distance = grid_distance(origin, beckoned.pos());
    distance -= div_rand_round(pow, 4);
    coord_def retval = beckoned.pos();
    ray_def ray;

    if (path.source == path.target)
    {
        if (!find_ray(beckoned.pos(), agent.pos(), ray, opc_fully_no_trans, 8))
            return beckoned.pos();

        distance = grid_distance(beckoned.pos(), agent.pos());
        distance -= div_rand_round(pow, 4);

        int loops = 0;

        while (loops < 15)
        {
            ray.advance();

            coord_def pos = ray.pos();

            if (pos == agent.pos())
            {
                loops += 30;
                continue;
            }

            if (actor_at(pos) || !beckoned.can_pass_through(pos))
                continue; // actor could be caster, or a bush

            int x = grid_distance(pos, agent.pos());

            if (x >= distance && x < grid_distance(retval, agent.pos()))
                retval = pos;

            loops++;
        }
        return retval;
    }

    for (coord_def pos : path.path_taken)
    {
        if (actor_at(pos) || !beckoned.can_pass_through(pos))
            continue; // actor could be caster, or a bush

        int x = grid_distance(pos, origin);

        if (x >= distance && x < grid_distance(retval, origin))
            retval = pos;
    }

    return retval;
}

/**
 * Attempt to move the beckoned creature to the spot on the path closest to its
 * beginning (that is, to the caster of the effect). Also handles some
 * messaging.
 *
 * @param beckoned  The creature being moved.
 * @param path      The path to move the creature along.
 * @return          Whether the beckoned creature actually moved.
 */
bool beckon(coord_def &origin, actor &beckoned, const bolt &path, int pow, actor &agent)
{
    const coord_def dest = _beckon_destination(origin, beckoned, path, pow, agent);
    if (dest == beckoned.pos())
        return false;

    const coord_def old_pos = beckoned.pos();
    if (!beckoned.move_to_pos(dest))
        return false;

    mprf("%s %s wrenched violently forward by a lasso of force!",
        beckoned.name(DESC_THE).c_str(),
        beckoned.is_player() ? "are" : "is");

    beckoned.props[PULLED_KEY] = (int)agent.mid;

    // If pulled adjacent ministun.
    if (adjacent(origin, dest) && beckoned.is_monster())
        beckoned.as_monster()->lose_energy(EUT_MOVE, 2, 1);

    beckoned.apply_location_effects(old_pos); // traps, etc.
    if (beckoned.is_monster())
        mons_relocated(beckoned.as_monster()); // cleanup tentacle segments

    return true;
}
