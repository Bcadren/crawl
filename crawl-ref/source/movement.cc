/**
 * @file
 * @brief Movement, open-close door commands, movement effects.
**/

#include <algorithm>
#include <cstring>
#include <string>
#include <sstream>

#include "AppHdr.h"

#include "movement.h"

#include "abyss.h"
#include "attack.h"
#include "bloodspatter.h"
#include "cloud.h"
#include "coord.h"
#include "coordit.h"
#include "delay.h"
#include "directn.h"
#include "dungeon.h"
#include "env.h"
#include "fight.h"
#include "food.h"
#include "fprop.h"
#include "god-abil.h"
#include "god-conduct.h"
#include "god-passive.h"
#include "items.h"
#include "message.h"
#include "mon-act.h"
#include "mon-death.h"
#include "mon-place.h"
#include "mon-util.h"
#include "player.h"
#include "player-reacts.h"
#include "prompt.h"
#include "random.h"
#include "religion.h"
#include "shout.h"
#include "state.h"
#include "stringutil.h"
#include "spl-damage.h"
#include "spl-selfench.h" // noxious_bog_cell
#include "spl-summoning.h" // new animate skeleton
#include "target-compass.h"
#include "terrain.h"
#include "traps.h"
#include "travel.h"
#include "transform.h"
#include "xom.h" // XOM_CLOUD_TRAIL_TYPE_KEY

static void _apply_move_time_taken(int additional_time_taken = 0);

// Swap monster to this location. Player is swapped elsewhere.
// Moves the monster into position, but does not move the player
// or apply location effects: the latter should happen after the
// player is moved.
static void _swap_places(monster* mons, const coord_def &loc)
{
    ASSERT(map_bounds(loc));
    ASSERT(monster_habitable_grid(mons, env.grid(loc)));

    if (monster_at(loc))
    {
        if (mons->type == MONS_WANDERING_MUSHROOM
            && monster_at(loc)->type == MONS_TOADSTOOL)
        {
            // We'll fire location effects for 'mons' back in move_player_action,
            // so don't do so here. The toadstool won't get location effects,
            // but the player will trigger those soon enough. This wouldn't
            // work so well if toadstools were aquatic, had clinging, or were
            // otherwise handled specially in monster_swap_places or in
            // apply_location_effects.
            monster_swaps_places(mons, loc - mons->pos(), true, false);
            return;
        }
        else
        {
            mpr("Something prevents you from swapping places.");
            return;
        }
    }

    mpr("You swap places.");

    mons->move_to_pos(loc, true, true);

    // Foxfire attacks and then dissapates at the new location after the swap.
    if (mons->type == MONS_FOXFIRE || mons->type == MONS_EPHEMERAL_SPIRIT)
    {
        foxfire_attack(mons, &you);
        monster_die(*mons, KILL_DISMISSED, NON_MONSTER, true);
    }

    return;
}

// Check squares adjacent to player for given feature and return how
// many there are. If there's only one, return the dx and dy.
static int _check_adjacent(dungeon_feature_type feat, coord_def& delta)
{
    int num = 0;

    set<coord_def> doors;
    for (adjacent_iterator ai(you.pos(), true); ai; ++ai)
    {
        if (env.grid(*ai) == feat)
        {
            // Specialcase doors to take into account gates.
            if (feat_is_door(feat))
            {
                // Already included in a gate, skip this door.
                if (doors.count(*ai))
                    continue;

                // Check if it's part of a gate. If so, remember all its doors.
                set<coord_def> all_door;
                find_connected_identical(*ai, all_door);
                doors.insert(begin(all_door), end(all_door));
            }

            num++;
            delta = *ai - you.pos();
        }
    }

    return num;
}

static void _entered_malign_portal(actor* act)
{
    ASSERT(act); // XXX: change to actor &act
    if (you.can_see(*act))
    {
        mprf("%s %s twisted violently and ejected from the portal!",
             act->name(DESC_THE).c_str(), act->conj_verb("be").c_str());
    }

    act->blink();
    act->hurt(nullptr, roll_dice(2, 4), BEAM_MISSILE, KILLED_BY_WILD_MAGIC,
              "", "entering a malign gateway");
}

bool cancel_barbed_move(bool rampaging)
{
    if ((!you.mounted() && you.duration[DUR_BARBS]) || you.duration[DUR_MOUNT_BARBS] && !you.props.exists(BARBS_MOVE_KEY))
    {
        string prompt = make_stringf("The spiked barbs in your %s will hurt %s if you move.%s Continue?", 
            you.mounted() ? you.mount_name().c_str() : "flesh",
            you.mounted() ? "it" : "you", // BCADDO: Mount pronouns?
            rampaging ? " Rampaging like this could really hurt!" : "");
        if (!yesno(prompt.c_str(), false, 'n'))
        {
            canned_msg(MSG_OK);
            return true;
        }

        you.props[BARBS_MOVE_KEY] = true;
    }

    return false;
}

void apply_barbs_damage(bool rampaging)
{
    bool harm_you = you.duration[DUR_BARBS] && !you.mounted();
    bool harm_mount = you.duration[DUR_MOUNT_BARBS];

    if (harm_you || harm_mount)
    {
        int dam = roll_dice(2, you.attribute[ATTR_BARBS_POW]);
        mprf(MSGCH_WARN, "The barbed spikes %s%s as you move%s",
            harm_you ? "dig painfully into your body" : "cut into your ",
            harm_you ? "" : you.mount_name(true).c_str(),
            attack_strength_punctuation(dam).c_str());
        if (harm_you)
        {
            ouch(dam, KILLED_BY_BARBS);
            bleed_onto_floor(you.pos(), MONS_PLAYER, dam, false);
        }
        else
        {
            damage_mount(dam);
            bleed_onto_floor(you.pos(), mount_mons(), dam, false);
        }

        // Sometimes decrease duration even when we move.
        if (one_chance_in(3))
            extract_manticore_spikes("The barbed spikes snap loose.", harm_mount);
        // But if that failed to end the effect, duration stays the same.
        if (!rampaging)
        {
            if (harm_you && you.duration[DUR_BARBS])
                you.duration[DUR_BARBS] += you.time_taken;
            if (harm_mount && you.duration[DUR_MOUNT_BARBS])
                you.duration[DUR_MOUNT_BARBS] += you.time_taken;
        }
    }
}

void remove_water_hold()
{
    if (you.duration[DUR_WATER_HOLD])
    {
        mpr("You slip free of the water engulfing you.");
        you.props.erase("water_holder");
        you.clear_far_engulf();
    }

    if (you.duration[DUR_AIR_HOLD])
    {
        mpr("You slip free of the deadly gas.");
        you.props.erase("air_holder");
        you.clear_far_engulf();
    }
}

static void _clear_constriction_data()
{
    you.stop_directly_constricting_all(true);
    if (you.is_directly_constricted())
        you.stop_being_constricted();
}

void apply_auto_skeleton(coord_def initial_position)
{
    if (you.attribute[ATTR_SKELETON])
    {
        bool fail = x_chance_in_y(failure_rate_to_int(raw_spell_fail(SPELL_SKELETAL_UPRISING)), 100);
        if (cast_animate_skeleton(you.religion, fail, initial_position))
        {
            int spellpower = calc_spell_power(SPELL_SKELETAL_UPRISING, true);
            dec_mp(div_rand_round(spellpower, 100));
        }
    }
}

void apply_noxious_bog(const coord_def old_pos, const coord_def targ)
{
    if (you.duration[DUR_NOXIOUS_BOG])
    {
        if (!cell_is_solid(old_pos))
        {
            int x = targ.x - old_pos.x;
            int y = targ.y - old_pos.y;
            coord_def p0 = coord_def(old_pos.x - y, old_pos.y + x);
            coord_def p1 = coord_def(old_pos.x + y, old_pos.y - x);
            coord_def p2 = coord_def(old_pos.x - y, old_pos.y);
            coord_def p3 = coord_def(old_pos.x, old_pos.y - x);
            noxious_bog_cell(old_pos);
            if (x && y)
            {
                if (!cell_is_solid(p2) && !actor_at(p2))
                    noxious_bog_cell(p2);
                if (!cell_is_solid(p3) && !actor_at(p2))
                    noxious_bog_cell(p3);
            }
            else
            {
                if (!cell_is_solid(p0) && !actor_at(p2))
                    noxious_bog_cell(p0);
                if (!cell_is_solid(p1) && !actor_at(p2))
                    noxious_bog_cell(p1);
            }
        }
    }
}

bool apply_cloud_trail(const coord_def old_pos)
{
    if (you.duration[DUR_CLOUD_TRAIL])
    {
        if (!cell_is_solid(old_pos))
        {
            auto cloud = static_cast<cloud_type>(
                you.props[XOM_CLOUD_TRAIL_TYPE_KEY].get_int());
            ASSERT(cloud != CLOUD_NONE);
            check_place_cloud(cloud, old_pos, random_range(3, 10), &you,
                0, -1);

            return true;
        }
    }

    return false;
}

bool apply_slime_trail(const coord_def old_pos)
{
    if (you.get_mutation_level(MUT_MELT) >= 2)
    {
        const dungeon_feature_type feat = env.grid(old_pos);
        if (!feat_is_critical(feat) && !cell_is_solid(old_pos) && !feat_is_watery(feat) && one_chance_in(5))
        {
            const int dur = random_range(5, 12);
            temp_change_terrain(old_pos, DNGN_SLIMY_WATER, dur * BASELINE_DELAY, TERRAIN_CHANGE_SLIME);
            check_place_cloud(CLOUD_FIRE, old_pos, dur - 1, &you);
            return true;
        }
    }
    return false;
}

bool cancel_confused_move(bool stationary)
{
    dungeon_feature_type dangerous = DNGN_FLOOR;
    monster *bad_mons = 0;
    string bad_suff, bad_adj;
    bool penance = false;
    bool flight = false;
    for (adjacent_iterator ai(you.pos(), false); ai; ++ai)
    {
        if (!stationary
            && is_feat_dangerous(env.grid(*ai), true)
            && need_expiration_warning(env.grid(*ai))
            && (dangerous == DNGN_FLOOR || env.grid(*ai) == DNGN_LAVA))
        {
            dangerous = env.grid(*ai);
            if (need_expiration_warning(DUR_FLIGHT, env.grid(*ai)))
                flight = true;
            break;
        }
        else
        {
            string suffix, adj;
            monster *mons = monster_at(*ai);
            if (mons
                && (stationary
                    || !(is_sanctuary(you.pos()) && is_sanctuary(mons->pos()))
                       && !fedhas_passthrough(mons))
                && bad_attack(mons, adj, suffix, penance)
                && mons->angered_by_attacks())
            {
                bad_mons = mons;
                bad_suff = suffix;
                bad_adj = adj;
                if (penance)
                    break;
            }
        }
    }

    if (dangerous != DNGN_FLOOR || bad_mons)
    {
        string prompt = "";
        prompt += "Are you sure you want to ";
        prompt += !stationary ? "stumble around" : "swing wildly";
        prompt += " while confused and next to ";

        if (dangerous != DNGN_FLOOR)
        {
            prompt += (            dangerous == DNGN_LAVA ? "lava" :
                            dangerous == DNGN_SLIMY_WATER ? "caustic ooze" :
                       dangerous == DNGN_DEEP_SLIMY_WATER ? "deep caustic ooze"
                                                          : "deep water");
            prompt += flight ? " while you are losing your buoyancy"
                             : " while your transformation is expiring";
        }
        else
        {
            string name = bad_mons->name(DESC_PLAIN);
            if (starts_with(name, "the "))
               name.erase(0, 4);
            if (!starts_with(bad_adj, "your"))
               bad_adj = "the " + bad_adj;
            prompt += bad_adj + name + bad_suff;
        }
        prompt += "?";

        if (penance)
            prompt += " This could place you under penance!";

        if (!crawl_state.disables[DIS_CONFIRMATIONS]
            && !yesno(prompt.c_str(), false, 'n'))
        {
            canned_msg(MSG_OK);
            return true;
        }
    }

    return false;
}

// Opens doors.
// If move is !::origin, it carries a specific direction for the
// door to be opened (eg if you type ctrl + dir).
void open_door_action(coord_def move)
{
    ASSERT(!crawl_state.game_is_arena());
    ASSERT(!crawl_state.arena_suspended);

    if (you.attribute[ATTR_HELD])
    {
        free_self_from_net();
        you.turn_is_over = true;
        return;
    }

    if (you.confused())
    {
        canned_msg(MSG_TOO_CONFUSED);
        return;
    }

    coord_def delta;

    // The player hasn't picked a direction yet.
    if (move.origin())
    {
        const int num = _check_adjacent(DNGN_CLOSED_DOOR, move)
                        + _check_adjacent(DNGN_CLOSED_CLEAR_DOOR, move)
                        + _check_adjacent(DNGN_RUNED_DOOR, move)
                        + _check_adjacent(DNGN_RUNED_CLEAR_DOOR, move);

        if (num == 0)
        {
            mpr("There's nothing to open nearby.");
            return;
        }

        // If there's only one door to open, don't ask.
        if (num == 1 && Options.easy_door)
            delta = move;
        else
        {
            delta = prompt_compass_direction();
            if (delta.origin())
                return;
        }
    }
    else
        delta = move;

    // We got a valid direction.
    const coord_def doorpos = you.pos() + delta;

    if (door_vetoed(doorpos))
    {
        // Allow doors to be locked.
        const string door_veto_message = env.markers.property_at(doorpos,
                                                                 MAT_ANY,
                                                                 "veto_reason");
        if (door_veto_message.empty())
            mpr("The door is shut tight!");
        else
            mpr(door_veto_message);
        if (you.confused())
            you.turn_is_over = true;

        return;
    }

    const dungeon_feature_type feat = (in_bounds(doorpos) ? env.grid(doorpos)
                                                          : DNGN_UNSEEN);
    switch (feat)
    {
    case DNGN_CLOSED_DOOR:
    case DNGN_CLOSED_CLEAR_DOOR:
    case DNGN_RUNED_DOOR:
    case DNGN_RUNED_CLEAR_DOOR:
        player_open_door(doorpos);
        break;
    case DNGN_OPEN_DOOR:
    case DNGN_OPEN_CLEAR_DOOR:
    {
        string door_already_open = "";
        if (in_bounds(doorpos))
        {
            door_already_open = env.markers.property_at(doorpos, MAT_ANY,
                                                    "door_verb_already_open");
        }

        if (!door_already_open.empty())
            mpr(door_already_open);
        else
            mpr("It's already open!");
        break;
    }
    case DNGN_SEALED_DOOR:
    case DNGN_SEALED_CLEAR_DOOR:
        mpr("That door is sealed shut!");
        break;
    default:
        mpr("There isn't anything that you can open there!");
        break;
    }
}

void close_door_action(coord_def move)
{
    if (you.attribute[ATTR_HELD])
    {
        mprf("You can't close doors while %s.", held_status());
        return;
    }

    if (you.confused())
    {
        canned_msg(MSG_TOO_CONFUSED);
        return;
    }

    coord_def delta;

    if (move.origin())
    {
        // If there's only one door to close, don't ask.
        int num = _check_adjacent(DNGN_OPEN_DOOR, move)
                  + _check_adjacent(DNGN_OPEN_CLEAR_DOOR, move);
        if (num == 0)
        {
            mpr("There's nothing to close nearby.");
            return;
        }
        // move got set in _check_adjacent
        else if (num == 1 && Options.easy_door)
            delta = move;
        else
        {
            delta = prompt_compass_direction();
            if (delta.origin())
                return;
        }
    }
    else
        delta = move;

    const coord_def doorpos = you.pos() + delta;
    const dungeon_feature_type feat = (in_bounds(doorpos) ? env.grid(doorpos)
                                                          : DNGN_UNSEEN);

    switch (feat)
    {
    case DNGN_OPEN_DOOR:
    case DNGN_OPEN_CLEAR_DOOR:
        player_close_door(doorpos);
        break;
    case DNGN_CLOSED_DOOR:
    case DNGN_CLOSED_CLEAR_DOOR:
    case DNGN_RUNED_DOOR:
    case DNGN_RUNED_CLEAR_DOOR:
    case DNGN_SEALED_DOOR:
    case DNGN_SEALED_CLEAR_DOOR:
        mpr("It's already closed!");
        break;
    default:
        mpr("There isn't anything that you can close there!");
        break;
    }
}

// Maybe prompt to enter a portal, return true if we should enter the
// portal, false if the user said no at the prompt.
bool prompt_dangerous_portal(dungeon_feature_type ftype)
{
    switch (ftype)
    {
    case DNGN_ENTER_PANDEMONIUM:
    case DNGN_ENTER_ABYSS:
        return yesno("If you enter this portal you might not be able to return "
                     "immediately. Continue?", false, 'n');

    case DNGN_MALIGN_GATEWAY:
        return yesno("Are you sure you wish to approach this portal? There's no "
                     "telling what its forces would wreak upon your fragile "
                     "self.", false, 'n');

    default:
        return true;
    }
}

/**
 * Rampages the player toward a hostile monster, if one exists in the direction
 * of the move input. Invalid things along the rampage path cancel the rampage.
 *
 * @param move  A relative coord_def of the player's CMD_MOVE input,
 *              as called by move_player_action().
 * @return      spret::fail if something invalid prevented the rampage,
 *              spret::abort if a player prompt response should cancel the move
 *              entirely,
 *              spret::success if the rampage occurred.
 */
static spret _rampage_forward(coord_def move)
{
    ASSERT(!crawl_state.game_is_arena());

    // Assert if the requested move is not a move delta
    // this would throw off our tracer_target.
    ASSERT(abs(move.x) <= 1 && abs(move.y) <= 1);

    if (crawl_state.is_repeating_cmd())
    {
        crawl_state.cant_cmd_repeat("You can't repeat rampage.");
        crawl_state.cancel_cmd_again();
        crawl_state.cancel_cmd_repeat();
        return spret::fail;
    }

    // Don't rampage if the player has status effects that should prevent it:
    // fungusform + terrified, confusion, immobile (tree)form, or constricted.
    if (you.is_nervous()
        || you.confused()
        || you.is_stationary()
        || you.is_constricted())
    {
        return spret::fail;
    }

    const int tracer_range = you.current_vision;
    const int rampage_distance = 1;

    // This logic assumes that the relative coord_def move is from [-1,1].
    // If the move_player_action() calls are ever rewritten in a way that
    // breaks this assumption, these targeters will need to be updated.
    const coord_def tracer_target = you.pos() + (move * tracer_range);
    const coord_def rampage_destination = you.pos() + (move * rampage_distance);
    const coord_def rampage_target = you.pos() + (move * (rampage_distance + 1));

    // Setup the rampage tracer beam.
    bolt beam;
    beam.range           = LOS_RADIUS;
    beam.aimed_at_spot   = true;
    beam.target          = tracer_target;
    beam.name            = "rampaging";
    beam.source_name     = "you";
    beam.source          = you.pos();
    beam.source_id       = MID_PLAYER;
    beam.thrower         = KILL_YOU;
    // The rampage reposition is explicitly noiseless for stab synergy.
    // Its ensuing move or attack action will generate a normal amount of noise.
    beam.loudness        = 0;
    beam.pierce          = true;
    beam.affects_nothing = true;
    beam.is_tracer       = true;
    // is_targeting prevents bolt::do_fire() from interrupting with a prompt,
    // if our tracer crosses something that blocks line of fire.
    beam.is_targeting    = true;
    beam.fire();

    const monster* valid_target = nullptr;

    // Iterate the tracer to see if the first visible target is a hostile mons.
    for (coord_def p : beam.path_taken)
    {
        // Don't rampage without direct visibility to the target tile.
        if (!you.see_cell_no_trans(p))
            return spret::fail;

        // Don't rampage if our tracer path is broken by something we can't
        // safely pass through before it reaches a monster.
        if (!you.can_pass_through(p) || is_feat_dangerous(env.grid(p)))
            return spret::fail;

        const monster* mon = monster_at(p);
        if (!mon)
            continue;
        // Allow our tracer to passthrough Fedhas allies.
        else if (mon && fedhas_passthrough(mon))
            continue;
        // Don't rampage at invis mons, but allow the tracer to keep going.
        else if (mon && !you.can_see(*mon))
            continue;
        // Don't rampage if the closest mons is non-hostile or a (non-Fedhas) plant.
        else if (mon && (mon->friendly()
                         || mon->neutral()
                         || mons_is_firewood(*mon)))
        {
            return spret::fail;
        }
        // Okay, the first mons along the tracer is a valid target.
        else if (mon)
        {
            valid_target = mon;
            break;
        }
    }
    if (!valid_target)
        return spret::fail;

    // Reset the beam target to the actual rampage_destination distance.
    beam.target = rampage_destination;

    // Will the second move be an attack?
    const bool attacking = valid_target->pos() == rampage_target;

    // Don't rampage if the player's tile is being targeted, somehow.
    if (beam.target == you.pos())
        return spret::fail;

    // Beholder/fearmonger messaging is handled by the movement code,
    // so we return fail even though the move will ultimately be aborted.
    // Rampaging within (up to the edge) of the allowed range is permitted.
    // Don't rampage if it would take us away from a beholder.
    const monster* beholder = you.get_beholder(beam.target);
    if (beholder)
        return spret::fail;

    // Don't rampage if it would take us toward a fearmonger.
    const monster* fearmonger = you.get_fearmonger(beam.target);
    if (fearmonger)
        return spret::fail;

    // Do allow rampaging on top of Fedhas plants,
    const monster* mons = monster_at(beam.target);
    bool fedhas_move = false;
    if (mons && fedhas_passthrough(mons))
        fedhas_move = true;
    // but otherwise, don't rampage if it would land us on top of a monster,
    else if (mons)
    {
        if (!you.can_see(*mons))
        {
            // .. and if a mons was in the way and invisible, notify the player.
            clear_messages();
            mpr("Something unexpectedly blocked you, preventing you from rampaging!");
        }
        return spret::fail;
    }

    // Abort if the player answers no to a dangerous terrain/trap/cloud/
    // exclusion prompt and weapon check prompts;
    // messaging for this is handled by check_moveto().
    if (!check_moveto(beam.target, "rampage")
        || attacking && !wielded_weapons_check()
        || !attacking && !check_moveto(rampage_target, "rampage"))
    {
        stop_running();
        you.turn_is_over = false;
        return spret::abort;
    }

    // Abort if the player answers no to a DUR_BARBS damaging move prompt.
    if (cancel_barbed_move(true))
        return spret::abort;

    // We've passed the validity checks, go ahead and rampage.

    // First, apply any necessary pre-move effects:
    remove_water_hold();
    _clear_constriction_data();
    const coord_def old_pos = you.pos();

    clear_messages();
    const monster* current = monster_at(you.pos());
    if (fedhas_move && (!current || !fedhas_passthrough(current)))
    {
        mprf("You rampage quickly through the %s towards %s!",
             mons_genus(mons->type) == MONS_FUNGUS ? "fungus" : "plants",
             valid_target->name(DESC_THE, true).c_str());
    }
    else
        mprf("You rampage towards %s!", valid_target->name(DESC_THE, true).c_str());
    // stepped = true, we're flavouring this as movement, not a blink.
    move_player_to_grid(beam.target, true);

    // Lastly, apply post-move effects unhandled by move_player_to_grid().
    apply_barbs_damage(true);
    apply_noxious_bog(old_pos, you.pos());
    apply_cloud_trail(old_pos);
    apply_auto_skeleton(old_pos);

    // If there is somehow an active run delay here, update the travel trail.
    if (you_are_delayed() && current_delay()->is_run())
        env.travel_trail.push_back(you.pos());

    return spret::success;
}

static void _apply_move_time_taken(int additional_time_taken)
{
    if (you.mounted())
        you.time_taken = 10;

    you.time_taken *= player_movement_speed();
    you.time_taken = div_rand_round(you.time_taken, 10);
    you.time_taken += additional_time_taken;

    if (you.running && you.running.travel_speed)
    {
        you.time_taken = max(you.time_taken,
                             div_round_up(100, you.running.travel_speed));
    }

    if (you.duration[DUR_NO_HOP])
        you.duration[DUR_NO_HOP] += you.time_taken;
}

// The "first square" of rampaging ordinarily has no time cost, and the "second
// square" is where its move delay or attack delay would be applied. If the
// player begins a rampage, and then cancels the second move, as through a
// prompt, we have to ensure they don't get zero-cost movement out of it. Here
// we apply movedelay, end the turn, and call relevant post-move effects.
static void _finalize_cancelled_rampage_move()
{
    _apply_move_time_taken();
    you.turn_is_over = true;

    if (player_in_branch(BRANCH_ABYSS))
        maybe_shift_abyss_around_player();

    you.apply_berserk_penalty = true;

    // rampaging is pretty dang hasty
    if (you_worship(GOD_CHEIBRIADOS) && one_chance_in(2))
        did_god_conduct(DID_HASTY, 1, true);

    // Rampaging prevents Wu Jian attacks, so we do not process them
    // here
    update_acrobat_status();
}

// Called when the player moves by walking/running. Also calls attack
// function etc when necessary.
void move_player_action(coord_def move)
{
    ASSERT(!crawl_state.game_is_arena() && !crawl_state.arena_suspended);

    bool attacking = false;
    bool moving = true;         // used to prevent eventual movement (swap)
    bool swap = false;

    int additional_time_taken = 0; // Extra time independent of movement speed

    ASSERT(!in_bounds(you.pos()) || !cell_is_solid(you.pos())
           || you.wizmode_teleported_into_rock);

    if (you.attribute[ATTR_HELD])
    {
        free_self_from_net();
        you.turn_is_over = true;
        return;
    }

    coord_def initial_position = you.pos();

    // When confused, sometimes make a random move.
    if (you.confused())
    {
        if (you.is_stationary())
        {
            // Don't choose a random location to try to attack into - allows
            // abuse, since trying to move (not attack) takes no time, and
            // shouldn't. Just force confused trees to use ctrl.
            mpr("You cannot move. (Use ctrl+direction or * direction to "
                "attack without moving.)");
            return;
        }

        if (cancel_confused_move(false))
            return;

        if (cancel_barbed_move())
            return;

        if (!one_chance_in(3))
        {
            move.x = random2(3) - 1;
            move.y = random2(3) - 1;
            if (move.origin())
            {
                mpr("You're too confused to move!");
                you.apply_berserk_penalty = true;
                you.turn_is_over = true;
                return;
            }
        }

        const coord_def new_targ = you.pos() + move;
        if (!in_bounds(new_targ) || !you.can_pass_through(new_targ))
        {
            you.turn_is_over = true;
            if (you.digging) // no actual damage
            {
                mprf("Your mandibles retract as you bump into %s.",
                     feature_description_at(new_targ, false,
                                            DESC_THE).c_str());
                you.digging = false;
            }
            else
            {
                mprf("You bump into %s.",
                     feature_description_at(new_targ, false,
                                            DESC_THE).c_str());
            }
            you.apply_berserk_penalty = true;
            crawl_state.cancel_cmd_repeat();

            return;
        }
    }

    bool rampaged = false;

    if (you.rampaging())
    {
        switch (_rampage_forward(move))
        {
            // Check the player's position again; rampage may have moved us.

            // Cancel the move entirely if rampage was aborted from a prompt.
            case spret::abort:
                ASSERT(!in_bounds(you.pos()) || !cell_is_solid(you.pos())
                       || you.wizmode_teleported_into_rock);
                return;

            case spret::success:
                rampaged = true;
                // If we've rampaged, reset initial_position for the second
                // move.
                initial_position = you.pos();
                // intentional fallthrough
            default:
            case spret::fail:
                ASSERT(!in_bounds(you.pos()) || !cell_is_solid(you.pos())
                       || you.wizmode_teleported_into_rock);
                break;
        }
    }

    const coord_def targ = you.pos() + move;
    string wall_jump_err;
    // Don't allow wall jump against close doors via movement -- need to use
    // the ability. Also, if moving into a closed door, don't call
    // wu_jian_can_wall_jump, to avoid printing a spurious message (see 11940).
    bool can_wall_jump = Options.wall_jump_move
                         && (!in_bounds(targ)
                             || !feat_is_closed_door(env.grid(targ)))
                         && wu_jian_can_wall_jump(targ, wall_jump_err);
    bool did_wall_jump = false;
    // You can't walk out of bounds!
    if (!in_bounds(targ) && !can_wall_jump)
    {
        // Why isn't the border permarock?
        if (you.digging)
            mpr("This wall is too hard to dig through.");
        return;
    }

    const string walkverb = you.airborne()                       ? "fly"
                          : you.swimming()                       ? "swim"
                          : you.form == transformation::scorpion ? "crawl"
                          : ((you.species == SP_NAGA || you.char_class == JOB_NAGA)
                             && form_keeps_mutations())          ? "slither"
                                                                 : "walk";

    monster* targ_monst = monster_at(targ);
    if (fedhas_passthrough(targ_monst) && !you.is_stationary())
    {
        // Moving on a plant takes 1.5 x normal move delay. We
        // will print a message about it but only when moving
        // from open space->plant (hopefully this will cut down
        // on the message spam).
        you.time_taken = div_rand_round(you.time_taken * 3, 2);

        monster* current = monster_at(you.pos());
        if (!current || !fedhas_passthrough(current))
        {
            // Probably need a better message. -cao
            mprf("You %s carefully through the %s.", walkverb.c_str(),
                 mons_genus(targ_monst->type) == MONS_FUNGUS ? "fungus"
                                                             : "plants");
        }
        targ_monst = nullptr;
    }

    bool targ_pass = you.can_pass_through(targ) && !you.is_stationary();

    if (you.digging)
    {
        if (apply_starvation_penalties())
        {
            you.digging = false;
            canned_msg(MSG_TOO_HUNGRY);
        }
        else if (feat_is_diggable(env.grid(targ), true))
            targ_pass = true;
        else // moving or attacking ends dig
        {
            you.digging = false;
            if (feat_is_solid(env.grid(targ)))
                mpr("You can't dig through that.");
            else
                mpr("You retract your mandibles.");
        }
    }

    if (you.mounted() && (you.mount == mount_type::slime) 
        && (feat_is_diggable(env.grid(targ), true) || feat_is_tree(env.grid(targ))))
    {
        targ_pass = true;
        you.digging = true;
    }

    // You can swap places with a friendly or good neutral monster if
    // you're not confused, or even with hostiles if both of you are inside
    // a sanctuary.
    const bool try_to_swap = targ_monst
                             && (targ_monst->wont_attack()
                                    && !you.confused()
                                 || is_sanctuary(you.pos())
                                    && is_sanctuary(targ));

    // You cannot move away from a siren but you CAN fight monsters on
    // neighbouring squares.
    monster* beholder = nullptr;
    if (!you.confused())
        beholder = you.get_beholder(targ);

    // You cannot move closer to a fear monger.
    monster *fmonger = nullptr;
    if (!you.confused())
        fmonger = you.get_fearmonger(targ);

    if (!rampaged && you.running.check_stop_running())
    {
        // [ds] Do we need this? Shouldn't it be false to start with?
        you.turn_is_over = false;
        return;
    }

    coord_def mon_swap_dest;

    if (targ_monst)
    {
        if (try_to_swap && !beholder && !fmonger)
        {
            if (swap_check(targ_monst, mon_swap_dest))
                swap = true;
            else
            {
                stop_running();
                moving = false;
            }
        }
        else if ((targ_monst->temp_attitude() == ATT_NEUTRAL) 
            && !you.confused() && targ_monst->visible_to(&you))
        {
            simple_monster_message(*targ_monst, " refuses to make way for you. "
                              "(Use ctrl+direction or * direction to attack.)");
            you.turn_is_over = false;
            return;
        }
        else if (!try_to_swap) // attack!
        {
            // Don't allow the player to freely locate invisible monsters
            // with confirmation prompts.
            // Rampaging forcibly initiates the attack, but the attack
            // can still be cancelled.
            if (!rampaged && !you.can_see(*targ_monst)
                && !you.confused()
                && !check_moveto(targ, walkverb)
                // Attack cancelled by fight_melee
                || !fight_melee(&you, targ_monst))
            {
                stop_running();
                // If we cancel this move after rampaging, we end the turn.
                if (rampaged)
                {
                    move.reset();
                    _finalize_cancelled_rampage_move();
                    return;
                }
                you.turn_is_over = false;
                return;
            }

            you.turn_is_over = true;
            you.berserk_penalty = 0;
            attacking = true;
        }
    }
    else if (you.form == transformation::fungus && moving && !you.confused())
    {
        if (you.is_nervous())
        {
            mpr("You're too terrified to move while being watched!");
            stop_running();
            you.turn_is_over = false;
            return;
        }
    }

    const bool running = you_are_delayed() && current_delay()->is_run();
    bool dug = false;

    if (!attacking && (targ_pass || can_wall_jump)
        && moving && !beholder && !fmonger)
    {
        if (you.confused() && is_feat_dangerous(env.grid(targ)))
        {
            mprf("You nearly stumble into %s!",
                 feature_description_at(targ, false, DESC_THE).c_str());
            you.apply_berserk_penalty = true;
            you.turn_is_over = true;
            return;
        }

        // can_wall_jump means `targ` is solid and can be walljumped off of,
        // so the player will never enter `targ`. Therefore, we don't want to
        // check exclusions at `targ`.

        // Prompt already handled by rampage
        if (!you.confused() && !can_wall_jump 
               && !rampaged && !check_moveto(targ, walkverb))
        {
            stop_running();
            you.turn_is_over = false;
            return;
        }

        // If confused, we've already been prompted (in case of stumbling into
        // a monster and attacking instead).
        // If rampaging we've already been prompted.
        if (!you.confused() && !rampaged && cancel_barbed_move())
            return;

        if (!you.attempt_escape()) // false means constricted and did not escape
            return;

        if (you.digging)
        {
            if (you.mount == mount_type::slime)
            {
                mprf("Your slimy mount dissolves %s.", feature_description_at(targ, false,
                    DESC_THE).c_str());
                you.digging = false; // Toggled back off to prevent mandibles messages.
            }
            else
            {
                mprf("You dig through %s.", feature_description_at(targ, false,
                    DESC_THE).c_str());
                make_hungry(50, true);
            }
            additional_time_taken += BASELINE_DELAY / 5;
            dug = true;
            destroy_wall(targ);
            noisy(6, you.pos());
        }

        if (swap)
            _swap_places(targ_monst, mon_swap_dest);

        if (running && env.travel_trail.empty())
            env.travel_trail.push_back(you.pos());
        else if (!running)
            clear_travel_trail();

        coord_def old_pos = you.pos();
        // Don't trigger things that require movement
        // when confusion causes no move.
        if (you.pos() != targ && targ_pass)
        {
            remove_water_hold();
            _clear_constriction_data();
            move_player_to_grid(targ, true);
            apply_barbs_damage();
            apply_noxious_bog(old_pos, targ);
            apply_cloud_trail(old_pos);
            apply_slime_trail(old_pos);
        }
        else if (can_wall_jump && !running)
        {
            if (!wu_jian_do_wall_jump(targ, false))
                return; // wall jump only in the ready state, or cancelled
            else
                did_wall_jump = true;
        }

        // Now it is safe to apply the swappee's location effects and add
        // trailing effects. Doing so earlier would allow e.g. shadow traps to
        // put a monster at the player's location.
        if (swap)
            targ_monst->apply_location_effects(targ);

        if (you_are_delayed() && current_delay()->is_run())
            env.travel_trail.push_back(you.pos());

        _apply_move_time_taken(additional_time_taken);

        move.reset();
        you.turn_is_over = true;
        request_autopickup();
    }

    if (!attacking && !targ_pass && !can_wall_jump && !running
        && moving && !beholder && !fmonger
        && Options.wall_jump_move
        && wu_jian_can_wall_jump_in_principle(targ))
    {
        // do messaging for a failed wall jump
        mpr(wall_jump_err);
    }

    // BCR - Easy doors single move
    if ((Options.travel_open_doors || !you.running)
        && !attacking
        && feat_is_closed_door(env.grid(targ)))
    {
        open_door_action(move);
        move.reset();
        return;
    }
    else if (!targ_pass && env.grid(targ) == DNGN_MALIGN_GATEWAY
             && !attacking && !you.is_stationary())
    {
        if (!crawl_state.disables[DIS_CONFIRMATIONS]
            && !prompt_dangerous_portal(env.grid(targ)))
        {
            // No rampage check because the portal blocks the
            // rampage tracer
            return;
        }

        move.reset();
        you.turn_is_over = true;

        _entered_malign_portal(&you);
        return;
    }
    else if (!targ_pass && !attacking && !can_wall_jump)
    {
        // No rampage check here, since you can't rampage at walls
        if (you.is_stationary())
            canned_msg(MSG_CANNOT_MOVE);
        else if (env.grid(targ) == DNGN_OPEN_SEA)
            mpr("The ferocious winds and tides of the open sea thwart your progress.");
        else if (env.grid(targ) == DNGN_ENDLESS_SLUDGE)
        {
            if (you.can_smell())
                mpr("The horrid stench of the seemingly endless sewage repels you from going further.");
            else
                mpr("The rot and filth of the seemingly endless sewage repels you from going further.");
        }
        else if (env.grid(targ) == DNGN_LAVA_SEA)
            mpr("The endless sea of lava is not a nice place.");
        else if (feat_is_tree(env.grid(targ)) && you_worship(GOD_FEDHAS))
            mpr("You cannot walk through the dense trees.");

        stop_running();
        move.reset();
        you.turn_is_over = false;
        crawl_state.cancel_cmd_repeat();
        return;
    }
    else if (beholder && !attacking && !can_wall_jump)
    {
        mprf("You cannot move away from %s!",
            beholder->name(DESC_THE).c_str());
        stop_running();
        // If this would have been the move after rampaging, we end the turn.
        if (rampaged)
        {
            move.reset();
            _finalize_cancelled_rampage_move();
            return;
        }
        you.turn_is_over = false;
        return;
    }
    else if (fmonger && !attacking && !can_wall_jump)
    {
        mprf("You cannot move closer to %s!",
            fmonger->name(DESC_THE).c_str());
        stop_running();
        // If this would have been the move after rampaging, we end the turn.
        if (rampaged)
        {
            move.reset();
            _finalize_cancelled_rampage_move();
            return;
        }
        you.turn_is_over = false;
        return;
    }

    if (you.running == RMODE_START)
        you.running = RMODE_CONTINUE;

    if (player_in_branch(BRANCH_ABYSS))
        maybe_shift_abyss_around_player();

    you.apply_berserk_penalty = !attacking;

    if (!attacking
        && you_worship(GOD_CHEIBRIADOS)
        && ((one_chance_in(10) && you.run())
             || (one_chance_in(2) && rampaged)))
    {
        did_god_conduct(DID_HASTY, 1, true);
    }

    bool did_wu_jian_attack = false;

    if (you_worship(GOD_WU_JIAN) && !attacking && !dug && !rampaged)
        did_wu_jian_attack = wu_jian_post_move_effects(did_wall_jump, initial_position);

   apply_auto_skeleton(initial_position);

    // If you actually moved you are eligible for amulet of the acrobat.
    if (!attacking && moving && !did_wu_jian_attack && !did_wall_jump)
        update_acrobat_status();
}
