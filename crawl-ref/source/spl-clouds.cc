/**
 * @file
 * @brief Cloud creating spells.
**/

#include "AppHdr.h"

#include "spl-clouds.h"

#include <algorithm>

#include "butcher.h"
#include "cloud.h"
#include "coord.h"
#include "coordit.h"
#include "directn.h"
#include "english.h"
#include "env.h"
#include "fprop.h"
#include "fight.h"
#include "god-abil.h"
#include "items.h"
#include "level-state-type.h"
#include "message.h"
#include "mon-behv.h" // ME_WHACK
#include "ouch.h"
#include "prompt.h"
#include "random-pick.h"
#include "religion.h"
#include "rot.h"
#include "shout.h"
#include "spl-cast.h"
#include "spl-util.h"
#include "target.h"
#include "terrain.h"

// Random Cloud type to be created by a chaos magic tick.
cloud_type chaos_cloud(bool player)
{
    if (one_chance_in(3))
        return CLOUD_CHAOS;

    cloud_type retval;

    retval = random_choose_weighted(30, CLOUD_FIRE,
                                     8, CLOUD_MEPHITIC,
                                    30, CLOUD_COLD,
                                    10, CLOUD_POISON,
                                     6, CLOUD_PETRIFY,
                                    15, CLOUD_HOLY,
                                     4, CLOUD_RAIN,
                                    18, CLOUD_MUTAGENIC,
                                    20, CLOUD_ACID,
                                    20, CLOUD_STORM);

    if (player && !is_good_god(you.religion) && retval == CLOUD_HOLY)
        retval = random_choose_weighted(20, CLOUD_NEGATIVE_ENERGY,
                                        10, CLOUD_SPECTRAL,
                                         5, CLOUD_HOLY,
                                         5, CLOUD_MIASMA);
    return retval;
}

spret conjure_flame(const actor *agent, int pow, const coord_def& where,
                         bool fail)
{
    const int range = (!agent || agent->is_player()) ? spell_range(SPELL_CONJURE_FLAME, pow)
                                                     : mon_spell_range(SPELL_CONJURE_FLAME, agent->as_monster());

    // FIXME: This would be better handled by a flag to enforce max range.
    if (grid_distance(where, agent->pos()) > range
        || !in_bounds(where))
    {
        if (agent->is_player())
            mpr("That's too far away.");
        return spret::abort;
    }

    if (cell_is_solid(where))
    {
        if (agent->is_player())
        {
            const char *feat = feat_type_name(env.grid(where));
            mprf("You can't place the cloud on %s.", article_a(feat).c_str());
        }
        return spret::abort;
    }

    cloud_struct* cloud = cloud_at(where);

    if (cloud && cloud->type != CLOUD_FIRE)
    {
        if (agent->is_player())
            mpr("There's already a cloud there!");
        return spret::abort;
    }

    actor* victim = actor_at(where);
    if (victim)
    {
        if (agent->can_see(*victim))
        {
            if (agent->is_player())
                mpr("You can't place the cloud on a creature.");
            return spret::abort;
        }

        fail_check();

        // FIXME: maybe should do _paranoid_option_disable() here?
        if (agent->is_player())
            canned_msg(MSG_GHOSTLY_OUTLINE);
        return spret::success;      // Don't give free detection!
    }

    fail_check();

    if (cloud)
    {
        // Reinforce the cloud - but not too much.
        // It must be a fire cloud from a previous test.
        if (you.see_cell(where))
            mpr("The fire blazes with new energy!");
        const int extra_dur = 2 + min(random2(pow) / 2, 20);
        cloud->decay += extra_dur * 5;
        cloud->source = agent->mid;
        if (agent->is_player())
            cloud->set_whose(KC_YOU);
        else
            cloud->set_killer(KILL_MON_MISSILE);
    }
    else
    {
        bool chaos = determine_chaos(agent, SPELL_CONJURE_FLAME);
        const int durat = min(5 + (random2(pow)/2) + (random2(pow)/2), 23);
        place_cloud(chaos ? chaos_cloud(agent->is_player()) : CLOUD_FIRE, where, durat, agent);
        if (you.see_cell(where))
        {
            if (agent->is_player())
                mpr("The fire ignites!");
            else
                mpr("A cloud of flames bursts into life!");
        }
    }
    noisy(spell_effect_noise(SPELL_CONJURE_FLAME), where);

    return spret::success;
}

spret cast_poisonous_vapours(int pow, const dist &beam, bool fail)
{
    if (cell_is_solid(beam.target))
    {
        canned_msg(MSG_UNTHINKING_ACT);
        return spret::abort;
    }

    monster* mons = monster_at(beam.target);
    if (!mons || mons->submerged())
    {
        fail_check();
        canned_msg(MSG_SPELL_FIZZLES);
        return spret::success; // still losing a turn
    }

    const bool possible_chaos = determine_chaos(&you, SPELL_POISONOUS_VAPOURS, false);
    const bool chaos = determine_chaos(&you, SPELL_POISONOUS_VAPOURS);

    if ((!possible_chaos && actor_cloud_immune(*mons, CLOUD_POISON)) && mons->observable())
    {
        mprf("But poisonous vapours would do no harm to %s!",
             mons->name(DESC_THE).c_str());
        return spret::abort;
    }

    if (possible_chaos && mons->cloud_immune() && mons->observable())
    {
        mprf("Clouds can do no harm to %s!",
            mons->name(DESC_THE).c_str());
        return spret::abort;
    }

    if (stop_attack_prompt(mons, false, you.pos()))
        return spret::abort;

    cloud_struct* cloud = cloud_at(beam.target);
    if (cloud && cloud->type != CLOUD_POISON && cloud->type != CLOUD_MEPHITIC)
    {
        mpr("There's already a cloud there!");
        return spret::abort;
    }

    fail_check();

    const int cloud_duration = max(random2(pow + 1) / 10, 1); // in dekaauts
    if (cloud)
    {
        // Reinforce the cloud.
        mprf("The poisonous vapours %s!", chaos ? "aberrate" : "increase");
        cloud->decay += cloud_duration * 10; // in this case, we're using auts
        cloud->set_whose(KC_YOU);
        cloud->type = CLOUD_POISON;
        if (chaos)
        {
            cloud_type cl = CLOUD_POISON;
            while (cl == CLOUD_POISON || cl == CLOUD_MEPHITIC)
                cl = chaos_cloud(true);
            cloud->type = cl;
        }
    }
    else
    {
        place_cloud(chaos ? chaos_cloud(true) : CLOUD_POISON, beam.target, cloud_duration, &you);
        mprf("%s vapours surround %s!", chaos ? "Random" : "Poisonous", mons->name(DESC_THE).c_str());
    }

    behaviour_event(mons, ME_WHACK, &you);

    return spret::success;
}

spret cast_big_c(int pow, spell_type spl, const actor *caster, bolt &beam,
                      bool fail)
{
    if (grid_distance(beam.target, you.pos()) > beam.range
        || !in_bounds(beam.target))
    {
        mpr("That is beyond the maximum range.");
        return spret::abort;
    }

    if (cell_is_solid(beam.target))
    {
        const char *feat = feat_type_name(env.grid(beam.target));
        mprf("You can't place clouds on %s.", article_a(feat).c_str());
        return spret::abort;
    }

    cloud_type cty = CLOUD_NONE;
    //XXX: there should be a better way to specify beam cloud types
    switch (spl)
    {
        case SPELL_POISONOUS_CLOUD:
            beam.flavour = BEAM_POISON;
            beam.name = "blast of poison";
            cty = CLOUD_POISON;
            break;
        case SPELL_HOLY_BREATH:
            beam.flavour = BEAM_HOLY;
            beam.origin_spell = SPELL_HOLY_BREATH;
            cty = CLOUD_HOLY;
            break;
        case SPELL_FREEZING_CLOUD:
            beam.flavour = BEAM_COLD;
            beam.name = "freezing blast";
            cty = CLOUD_COLD;
            break;
        default:
            mpr("That kind of cloud doesn't exist!");
            return spret::abort;
    }

    if (determine_chaos(caster, spl))
    {
        beam.flavour = BEAM_CHAOTIC;
        beam.name = "chaotic burst";
        cty = CLOUD_CHAOS;
    }

    beam.thrower           = KILL_YOU;
    beam.hit               = AUTOMATIC_HIT;
    beam.damage            = CONVENIENT_NONZERO_DAMAGE;
    beam.is_tracer         = true;
    beam.use_target_as_pos = true;
    beam.origin_spell      = spl;
    beam.affect_endpoint();
    if (beam.beam_cancelled)
        return spret::abort;

    fail_check();

    big_cloud(cty, caster, beam.target, pow, 8 + random2(3), -1);
    noisy(spell_effect_noise(spl), beam.target);
    return spret::success;
}

/*
 * A cloud_func that places an individual cloud as part of a cloud area. This
 * function is called by apply_area_cloud();
 *
 * @param where       The location of the cloud.
 * @param pow         The spellpower of the spell placing the clouds, which
 *                    determines how long the cloud will last.
 * @param spread_rate How quickly the cloud spreads.
 * @param ctype       The type of cloud to place.
 * @param agent       Any agent that may have caused the cloud. If this is the
 *                    player, god conducts are applied.
 * @param excl_rad    How large of an exclusion radius to make around the
 *                    cloud.
 * @returns           The number of clouds made, which is always 1.
 *
*/
static int _make_a_normal_cloud(coord_def where, int pow, int spread_rate,
                                cloud_type ctype, const actor *agent,
                                int excl_rad)
{
    place_cloud(ctype, where,
                (3 + random2(pow / 4) + random2(pow / 4) + random2(pow / 4)),
                agent, spread_rate, excl_rad);

    return 1;
}

/*
 * Make a large area of clouds centered on a given place. This never creates
 * player exclusions.
 *
 * @param ctype       The type of cloud to place.
 * @param agent       Any agent that may have caused the cloud. If this is the
 *                    player, god conducts are applied.
 * @param where       The location of the cloud.
 * @param pow         The spellpower of the spell placing the clouds, which
 *                    determines how long the cloud will last.
 * @param size        How large a radius of clouds to place from the `where'
 *                    argument.
 * @param spread_rate How quickly the cloud spreads.
*/
void big_cloud(cloud_type cl_type, const actor *agent,
               const coord_def& where, int pow, int size, int spread_rate)
{
    // The starting point _may_ be a place no cloud can be placed on.
    apply_area_cloud(_make_a_normal_cloud, where, pow, size,
                     cl_type, agent, spread_rate, -1);
}

spret cast_ring_of_flames(int power, bool fail)
{
    targeter_radius hitfunc(&you, LOS_NO_TRANS, 1);
    if (stop_attack_prompt(hitfunc, "make a ring of flames",
                [](const actor *act) -> bool {
                    return act->res_fire() < 3;
                }))
    {
        return spret::abort;
    }

    fail_check();
    you.increase_duration(DUR_FIRE_SHIELD,
                          6 + (power / 10) + (random2(power) / 5), 50,
                          "The air around you leaps into flame!");
    you.redraw_resists = true;
    manage_fire_shield();
    return spret::success;
}

void manage_fire_shield()
{
    ASSERT(you.duration[DUR_FIRE_SHIELD]);

    // Melt ice armour entirely.
    maybe_melt_player_enchantments(BEAM_FIRE, 100);

    const bool chaos = determine_chaos(&you, SPELL_RING_OF_FLAMES);

    surround_actor_with_cloud(&you, chaos ? CLOUD_RANDOM : CLOUD_FIRE);
}

spret cast_corpse_rot(bool fail)
{
    if (!you.res_rotting() && !you.cloud_immune())
    {
        for (stack_iterator si(you.pos()); si; ++si)
        {
            if (si->is_type(OBJ_CORPSES, CORPSE_BODY))
            {
                if (!yesno(("Really cast Corpse Rot while standing on " + si->name(DESC_A) + "?").c_str(), false, 'n'))
                {
                    canned_msg(MSG_OK);
                    return spret::abort;
                }
                break;
            }
        }
    }

    fail_check();
    if (you.attribute[ATTR_KIKU_CORPSE])
    {
        // Drop a fair amount of low quality corpses for rotting.
        const int pow = apply_invo_enhancer(you.skill(SK_NECROMANCY), false);
        int count = div_rand_round(pow, 3);
        count += div_rand_round(count, 3);
        count += 1 + random2avg(count, 3);

        vector<coord_def> targets;

        for (radius_iterator ri(you.pos(), LOS_NO_TRANS, true); ri; ++ri)
        {
            monster * mons = monster_at(*ri);
            if (mons && !mons->wont_attack() && !mons->res_rotting()
                && !cell_is_solid(*ri) && !feat_destroys_items(env.grid(*ri))
                && !mons->cloud_immune() && !cloud_at(*ri))
            {
                bool include = true;
                for (stack_iterator si(*ri); si; ++si)
                {
                    if (si->is_type(OBJ_CORPSES, CORPSE_BODY))
                    {
                        include = false;
                        break;
                    }
                }

                if (include)
                    targets.emplace_back(mons->pos());
            }
        }
        
        if (targets.size())
        {
            apply_invo_enhancer(10, true); // For the message.
            mprf(MSGCH_GOD, "Kikubaaqudgha casts corpses upon the feet of your enemies!");
            lose_piety(3);

            // More possible targets than alloted corpses; clear extras at random.
            while ((int)targets.size() > count)
                targets.erase(targets.begin() + random2((int)targets.size()));
            // Corpse slap valid targets
            for (int i = 0; i < (int)targets.size(); i++)
                place_kiku_corpse(random_kiku_monster(random2(pow)), targets[i]);

            count -= (int)targets.size();

            // Infinite loop protection for small areas, etc.
            int tries = 100;
            // Place additional corpses adjacent to targets.
            while (tries--)
            {
                coord_def t = coord_def (0, 0);
                for (adjacent_iterator ai(targets[random2(targets.size())]); ai; ++ai)
                {
                    if (cell_is_solid(*ai) || feat_destroys_items(env.grid(*ai))
                        || cloud_at(*ai))
                    {
                        continue;
                    }

                    actor * act = actor_at(*ai);
                    if (act && (act->wont_attack() || act->is_player()))
                        continue;

                    bool inc = true;
                    for (stack_iterator si(*ai); si; ++si)
                    {
                        if (si->is_type(OBJ_CORPSES, CORPSE_BODY))
                        {
                            inc = false;
                            break;
                        }
                    }

                    if (inc)
                    {
                        if (t.origin() || grid_distance(you.pos(), t) > grid_distance(you.pos(), *ai))
                            t = *ai;
                    }
                }

                if (!t.origin())
                {
                    count--;
                    place_kiku_corpse(random_kiku_monster(random2(pow)), t);
                    if (count <= 0)
                        tries = 0;
                }
            }
        }
    }

    return corpse_rot(&you);
}

spret corpse_rot(actor* caster)
{
    // If there is no caster (god wrath), centre the effect on the player.
    const coord_def center = caster ? caster->pos() : you.pos();
    bool saw_rot = false;
    int flies_count = 0;

    for (radius_iterator ri(center, LOS_NO_TRANS); ri; ++ri)
    {
        if (!is_sanctuary(*ri) && !cloud_at(*ri))
            for (stack_iterator si(*ri); si; ++si)
                if (si->is_type(OBJ_CORPSES, CORPSE_BODY))
                {
                    if (coinflip())
                    {
                        spawn_flies(*si, false);
                        if (you.see_cell(*ri))
                            flies_count++;
                    }

                    // Found a corpse. Skeletonise it if possible.
                    if (!mons_skeleton(si->mon_type))
                    {
                        item_was_destroyed(*si);
                        destroy_item(si->index());
                    }
                    else
                        turn_corpse_into_skeleton(*si);

                    place_cloud(CLOUD_MIASMA, *ri, 4+random2avg(16, 3),caster);

                    if (!saw_rot && you.see_cell(*ri))
                        saw_rot = true;

                    // Don't look for more corpses here.
                    break;
                }
    }

    if (saw_rot)
    {
        mprf("You %s decay.", you.can_smell() ? "smell" : "sense");
        if (flies_count)
            mprf("Flies burst forth from the corpse%s.", flies_count > 1 ? "s" : "");
    }
    else if (caster && caster->is_player())
    {
        // Abort the spell for players; monsters and wrath fail silently
        mpr("There is nothing fresh enough to decay nearby.");
        return spret::abort;
    }

    return spret::success;
}

void holy_flames(monster* caster, actor* defender)
{
    const coord_def pos = defender->pos();
    int cloud_count = 0;
    const int dur = 8 + random2avg(caster->get_hit_dice() * 3, 2);

    for (adjacent_iterator ai(pos); ai; ++ai)
    {
        if (!in_bounds(*ai)
            || cloud_at(*ai)
            || cell_is_solid(*ai)
            || is_sanctuary(*ai)
            || monster_at(*ai))
        {
            continue;
        }

        place_cloud(CLOUD_HOLY, *ai, dur, caster);

        cloud_count++;
    }

    if (cloud_count)
    {
        if (defender->is_player())
            mpr("Blessed fire suddenly surrounds you!");
        else
            simple_monster_message(*defender->as_monster(),
                                   " is surrounded by blessed fire!");
    }
}

static cloud_type _god_blesses_cloud(cloud_type cloud, god_type god)
{
    switch (god)
    {
    case GOD_ZIN:
        if (cloud == CLOUD_CHAOS || cloud == CLOUD_MUTAGENIC)
        {
            simple_god_message(" cleanses the chaos from the conjured clouds!", god);
            return CLOUD_HOLY;
        }
    // Fallthrough
    case GOD_ELYVILON:
    case GOD_SHINING_ONE:
        if (cloud == CLOUD_NEGATIVE_ENERGY)
        {
            simple_god_message(" cleanses the evil from the conjured clouds!", god);
            return CLOUD_HOLY;
        }
        break;
    case GOD_KIKUBAAQUDGHA:
        if (cloud == CLOUD_POISON || (one_chance_in(4) && cloud != CLOUD_NEGATIVE_ENERGY))
        {
            simple_god_message(" blesses the clouds with foul necrotic miasma!", god);
            return CLOUD_MIASMA;
        }
        break;
    case GOD_XOM:
        if (cloud != CLOUD_CHAOS && one_chance_in(10))
        {
            simple_god_message(" doesn't think the clouds are chaotic enough!", god);
            return CLOUD_CHAOS;
        }
        break;
    case GOD_YREDELEMNUL:
        if (cloud == CLOUD_NEGATIVE_ENERGY || cloud == CLOUD_MIASMA || (cloud == CLOUD_POISON && one_chance_in(3)))
        {
            if (!(you.religion == GOD_YREDELEMNUL))
                simple_god_message(" enhances the clouds to heal the undead!", god);
            else if (you.undead_state() == US_ALIVE)
                simple_god_message(" enhances your necrotic clouds to heal your undead servants!", god);
            else
                simple_god_message(" enhances your necrotic clouds to heal you and your undead slaves!", god);
            return CLOUD_SPECTRAL;
        }
        break;
    default:
        break;
    }
    return cloud;
}

random_pick_entry<cloud_type> cloud_cone_clouds[] =
{
  // Poison Group: Total 225
  {  0, 12, 200, FALL, CLOUD_MEPHITIC },
  { 10, 30, 125, PEAK, CLOUD_POISON },
  { 22, 35, 100, RISE, CLOUD_MIASMA },

  // Useless Group: Total: 350
  {  0, 36, 125, FALL, CLOUD_RAIN },
  {  0, 12,  75, FALL, CLOUD_MIST },
  {  0,  8,  75, FALL, CLOUD_MAGIC_TRAIL },
  {  0, 14,  75, FALL, CLOUD_DUST },

  // Core Group: Total 760
  {  0, 30, 125, PEAK, CLOUD_FIRE },
  {  0, 40,  75, RISE, CLOUD_STEAM },
  {  0, 30, 125, PEAK, CLOUD_COLD },
  { 10, 30, 125, RISE, CLOUD_NEGATIVE_ENERGY },
  { 12, 30, 135, RISE, CLOUD_STORM },
  { 15, 30, 175, RISE, CLOUD_ACID },

  // Chaos Group: Total 50.
  {  0, 30,  25,  FLAT, CLOUD_CHAOS },
  {  0, 30,  25,  FLAT, CLOUD_MUTAGENIC },

  // Null
  {  0,  0,   0, FLAT, CLOUD_NONE }
};

spret cast_cloud_cone(const actor *caster, int pow, const coord_def &pos,
                           bool fail)
{
    if (env.level_state & LSTATE_STILL_WINDS)
    {
        if (caster->is_player())
            mpr("The air is too still to form clouds.");
        return spret::abort;
    }

    const int range = (!caster || caster->is_player()) ? spell_range(SPELL_CLOUD_CONE, pow)
                                                       : mon_spell_range(SPELL_CLOUD_CONE, caster->as_monster());

    targeter_shotgun hitfunc(caster, CLOUD_CONE_BEAM_COUNT, range);

    hitfunc.set_aim(pos);

    if (caster->is_player())
    {
        if (stop_attack_prompt(hitfunc, "cloud"))
            return spret::abort;
    }

    fail_check();

    random_picker<cloud_type, NUM_CLOUD_TYPES> cloud_picker;
    cloud_type cloud = cloud_picker.pick(cloud_cone_clouds, min(pow, 30), CLOUD_NONE);

    mprf("%s %s a blast of %s!",
        caster->name(DESC_THE).c_str(),
        caster->conj_verb("create").c_str(),
        cloud_type_name(cloud).c_str());

    if (caster->is_player())
        cloud = _god_blesses_cloud(cloud, you.religion);
    else
        cloud = _god_blesses_cloud(cloud, caster->as_monster()->god);

    for (const auto &entry : hitfunc.zapped)
    {
        if (entry.second <= 0)
            continue;
        place_cloud(cloud, entry.first,
            max(5, random2avg(pow * 2, 3)),
            caster, div_round_up(pow, 10) - 1);
    }

    return spret::success;
}
