/**
 * @file
 * @brief Damage-dealing spells not already handled elsewhere.
 *           Other targeted spells are covered in spl-zap.cc.
**/

#include "AppHdr.h"


#include "spl-damage.h"

#include <functional>

#include "act-iter.h"
#include "areas.h"
#include "art-enum.h"
#include "attack.h"
#include "beam.h"
#include "chaos.h"
#include "cloud.h"
#include "colour.h"
#include "coordit.h"
#include "directn.h"
#include "dungeon.h"
#include "english.h"
#include "env.h"
#include "fight.h"
#include "food.h"
#include "fprop.h"
#include "god-abil.h"      // Silver Draconian EMPOWERED breath.
#include "god-conduct.h"
#include "invent.h"
#include "item-prop.h"
#include "items.h"
#include "los.h"
#include "losglobal.h"
#include "macro.h"
#include "mapmark.h"
#include "message.h"
#include "misc.h"
#include "mon-behv.h"
#include "mon-death.h"
#include "mon-tentacle.h"
#include "mon-place.h"
#include "mutation.h"
#include "ouch.h"
#include "prompt.h"
#include "random.h"
#include "religion.h"
#include "shout.h"
#include "spl-clouds.h"
#include "spl-summoning.h"
#include "spl-util.h"
#include "spl-zap.h"
#include "stepdown.h"
#include "stringutil.h"
#include "target.h"
#include "terrain.h"
#include "transform.h"
#include "traps.h"
#include "unicode.h"
#include "viewchar.h"
#include "view.h"

void setup_fire_storm(const actor *source, int pow, bolt &beam)
{
    zappy(ZAP_FIRE_STORM, pow, source->is_monster(), beam);
    beam.ex_size      = 2 + (random2(1000) < pow);
    beam.source_id    = source->mid;
    // XXX: Should this be KILL_MON_MISSILE?
    beam.thrower      =
        source->is_player() ? KILL_YOU_MISSILE : KILL_MON;
    beam.aux_source.clear();
    beam.is_tracer    = false;
    beam.origin_spell = SPELL_FIRE_STORM;
}

spret cast_fire_storm(int pow, bolt &beam, bool fail)
{
    if (grid_distance(beam.target, beam.source) > beam.range)
    {
        mpr("That is beyond the maximum range.");
        return spret::abort;
    }

    if (cell_is_solid(beam.target))
    {
        const char *feat = feat_type_name(grd(beam.target));
        mprf("You can't place the storm on %s.", article_a(feat).c_str());
        if (feat_is_tree(grd(beam.target)) && feat_is_door(grd(beam.target)))
            mpr("Place your storm next to the wood you want to burn.");
        return spret::abort;
    }

    setup_fire_storm(&you, pow, beam);

    bolt tempbeam = beam;
    tempbeam.ex_size = (pow > 76) ? 3 : 2;
    tempbeam.is_tracer = true;

    tempbeam.explode(false);
    if (tempbeam.beam_cancelled)
        return spret::abort;

    fail_check();

    beam.apply_beam_conducts();
    beam.refine_for_explosion();
    beam.explode(false);

    viewwindow();
    return spret::success;
}

// No setup/cast split here as monster damnation is completely different.
// XXX make this not true
bool cast_smitey_damnation(int pow, bolt &beam)
{
    beam.name              = "pillar of hellfire";
    beam.aux_source        = "hellfire";
    beam.ex_size           = 1;
    beam.flavour           = BEAM_DAMNATION;
    beam.real_flavour      = beam.flavour;
    beam.glyph             = dchar_glyph(DCHAR_FIRED_BURST);
    beam.colour            = LIGHTRED;
    beam.source_id         = MID_PLAYER;
    beam.thrower           = KILL_YOU;
    beam.obvious_effect    = false;
    beam.pierce            = false;
    beam.is_explosion      = true;
    beam.ench_power        = pow;      // used for radius
    beam.hit               = 20 + pow / 10;
    beam.damage            = calc_dice(6, 30 + pow);
    beam.attitude          = ATT_FRIENDLY;
    beam.friend_info.count = 0;
    beam.is_tracer         = true;

    beam.explode(false);

    if (beam.beam_cancelled)
    {
        canned_msg(MSG_OK);
        return false;
    }

    mpr("You call forth a mighty pillar of hellfire!");

    beam.is_tracer = false;
    beam.in_explosion_phase = false;
    beam.explode(true);

    return true;
}

static bool _is_menacing(const actor * caster, spell_type spell)
{
    item_def * staff = caster->staff();
    if (staff && get_staff_facet(*staff) == SPSTF_MENACE 
              && staff_enhances_spell(staff, spell))
        return true;
    return false;
}

// XXX no friendly check
spret cast_chain_spell(spell_type spell_cast, int pow,
                            const actor *caster, bool fail)
{
    fail_check();
    bolt beam;
    beam.real_flavour = BEAM_CHAOTIC;
    beam.flavour = BEAM_CHAOTIC;

    bool chaos = determine_chaos(caster, spell_cast);
    if (chaos && spell_cast == SPELL_CHAIN_LIGHTNING)
        spell_cast = SPELL_CHAIN_OF_CHAOS;

    // initialise beam structure
    switch (spell_cast)
    {
        case SPELL_CHAIN_LIGHTNING:
            beam.name           = "lightning arc";
            beam.aux_source     = "chain lightning";
            beam.glyph          = dchar_glyph(DCHAR_FIRED_ZAP);
            beam.real_flavour   = BEAM_ELECTRICITY;
            beam.flavour        = BEAM_ELECTRICITY;
            break;
        case SPELL_LESSER_CHAOS_CHAIN:
            beam.real_flavour   = BEAM_CHAOS;
            beam.flavour        = BEAM_CHAOS;
        case SPELL_CHAIN_OF_CHAOS:
            beam.name           = "arc of chaos";
            beam.aux_source     = "chain of chaos";
            beam.glyph          = dchar_glyph(DCHAR_FIRED_ZAP);
            break;
        default:
            die("buggy chain spell %d cast", spell_cast);
            break;
    }
    beam.source_id      = caster->mid;
    beam.thrower        = caster->is_player() ? KILL_YOU_MISSILE : KILL_MON_MISSILE;
    beam.range          = 8;
    beam.hit            = AUTOMATIC_HIT;
    beam.obvious_effect = true;
    beam.pierce         = false;       // since we want to stop at our target
    beam.is_explosion   = false;
    beam.is_tracer      = false;
    beam.origin_spell   = spell_cast;

    if (const monster* mons = caster->as_monster())
        beam.source_name = mons->name(DESC_PLAIN, true);

    bool first = true;
    coord_def source, target;

    for (source = caster->pos(); pow > 0;
         pow -= 8 + random2(13), source = target)
    {
        // infinity as far as this spell is concerned
        // (Range - 1) is used because the distance is randomised and
        // may be shifted by one.
        int min_dist = LOS_DEFAULT_RANGE - 1;

        int dist;
        int count = 0;

        target.x = -1;
        target.y = -1;

        for (monster_iterator mi; mi; ++mi)
        {
            if (invalid_monster(*mi))
                continue;

            // Don't arc to things we cannot hit.
            if (beam.ignores_monster(*mi))
                continue;

            dist = grid_distance(source, mi->pos());

            // check for the source of this arc
            if (!dist)
                continue;

            // randomise distance (arcs don't care about a couple of feet)
            dist += (random2(3) - 1);

            // always ignore targets further than current one
            if (dist > min_dist)
                continue;

            if (!cell_see_cell(source, mi->pos(), LOS_SOLID)
                || !cell_see_cell(caster->pos(), mi->pos(), LOS_SOLID_SEE))
            {
                continue;
            }

            // check for actors along the arc path
            ray_def ray;
            if (!find_ray(source, mi->pos(), ray, opc_solid))
                continue;

            while (ray.advance())
                if (actor_at(ray.pos()))
                    break;

            if (ray.pos() != mi->pos())
                continue;

            count++;

            if (dist < min_dist)
            {
                // switch to looking for closer targets (but not always)
                if (!one_chance_in(10))
                {
                    min_dist = dist;
                    target = mi->pos();
                    count = 0;
                }
            }
            else if (target.x == -1 || one_chance_in(count))
            {
                // either first target, or new selected target at
                // min_dist == dist.
                target = mi->pos();
            }
        }

        // now check if the player is a target
        dist = grid_distance(source, you.pos());

        if (dist)       // i.e., player was not the source
        {
            // distance randomised (as above)
            dist += (random2(3) - 1);

            // select player if only, closest, or randomly selected
            if ((target.x == -1
                    || dist < min_dist
                    || (dist == min_dist && one_chance_in(count + 1)))
                && cell_see_cell(source, you.pos(), LOS_SOLID))
            {
                target = you.pos();
            }
        }

        const bool see_source = you.see_cell(source);
        const bool see_targ   = you.see_cell(target);

        if (target.x == -1)
        {
            if (see_source)
                mprf("The %s grounds out.", beam.name.c_str());

            break;
        }

        // Trying to limit message spamming here so we'll only mention
        // the thunder at the start or when it's out of LoS.
        switch (spell_cast)
        {
            case SPELL_CHAIN_LIGHTNING:
            {
                const char* msg = "You hear a mighty clap of thunder!";
                noisy(spell_effect_noise(SPELL_CHAIN_LIGHTNING), source,
                      (first || !see_source) ? msg : nullptr);
                break;
            }
            case SPELL_CHAIN_OF_CHAOS:
                if (first && see_source)
                    mpr("A swirling arc of seething chaos appears!");
                break;
            default:
                break;
        }
        first = false;

        if (see_source && !see_targ)
            mprf("The %s arcs out of your line of sight!", beam.name.c_str());
        else if (!see_source && see_targ)
            mprf("The %s suddenly appears!", beam.name.c_str());

        beam.source = source;
        beam.target = target;
        beam.ench_power = pow;
        switch (spell_cast)
        {
            case SPELL_CHAIN_LIGHTNING:
                beam.colour = LIGHTBLUE;
                beam.damage = caster->is_player()
                    ? calc_dice(5, 10 + pow * 2 / 3)
                    : calc_dice(5, 46 + pow / 6);
                break;
            case SPELL_LESSER_CHAOS_CHAIN:
                beam.colour = ETC_RANDOM;
                beam.real_flavour = BEAM_CHAOS;
                beam.flavour = BEAM_CHAOS;
                beam.damage = calc_dice(3, 5 + pow / 6);
                break;
            case SPELL_CHAIN_OF_CHAOS:
                beam.colour = ETC_JEWEL;
                beam.real_flavour = BEAM_CHAOTIC;
                beam.flavour = BEAM_CHAOTIC;
                beam.damage = caster->is_player()
                    ? calc_dice(5, 12 + pow)
                    : calc_dice(5, 51 + pow / 3);
                break;
            default:
                break;
        }

        if (_is_menacing(caster, spell_cast))
            beam.damage.num += 2;

        // Be kinder to the caster.
        if (target == caster->pos())
        {
            // This should not hit the caster, too scary as a player effect and
            // too kind to the player as a monster effect.
            if (spell_cast == SPELL_LESSER_CHAOS_CHAIN)
            {
                beam.real_flavour = BEAM_VISUAL;
                beam.flavour      = BEAM_VISUAL;
            }

            // Reduce damage when the spell arcs to the caster.
            beam.damage.num = max(1, beam.damage.num / 2);
            beam.damage.size = max(3, beam.damage.size / 2);

            // Fairies aren't hurt by their own arcs 2/3 times or at all if rElec.
            if (caster->is_fairy() && (caster->res_elec() || !one_chance_in(3)))
            {
                beam.real_flavour = BEAM_VISUAL;
                beam.flavour      = BEAM_VISUAL;
            }
        }
        beam.fire();
    }

    return spret::success;
}

/*
 * Handle the application of damage from a player spell that doesn't apply these
 * through struct bolt. This can apply god conducts and handles any necessary
 * death cleanup.
 * @param mon          The monster.
 * @param damage       The damage to apply, if any. Regardless of damage done,
 *                     the monster will have death cleanup applied via
 *                     monster_die() if it's now dead.
 * @param flavour      The beam flavour of damage.
 * @param god_conducts If true, apply any god conducts, in which case the
 *                     monster must be alive. Some callers need to apply
 *                     effects prior to damage that might kill the monster,
 *                     hence handle conducts on their own.
*/
static void _player_hurt_monster(monster &mon, int damage, beam_type flavour,
                                 bool god_conducts = true)
{
    ASSERT(mon.alive() || !god_conducts);

    if (god_conducts && you.deity() == GOD_FEDHAS && fedhas_neutralises(mon))
    {
        simple_god_message(" protects your plant from harm.", GOD_FEDHAS);
        return;
    }

    god_conduct_trigger conducts[3];
    if (god_conducts)
        set_attack_conducts(conducts, mon, you.can_see(mon));

    if (damage)
        mon.hurt(&you, damage, flavour, KILLED_BY_BEAM);

    if (mon.alive())
    {
        behaviour_event(&mon, ME_WHACK, &you);

        if (damage && you.can_see(mon))
            print_wounds(mon);
    }
    // monster::hurt() wasn't called, so we do death cleanup.
    else if (!damage)
        monster_die(mon, KILL_YOU, NON_MONSTER);
}

static counted_monster_list _counted_monster_list_from_vector(
    vector<monster *> affected_monsters)
{
    counted_monster_list mons;
    for (auto mon : affected_monsters)
        mons.add(mon);
    return mons;
}

static bool _drain_lifeable(const actor* agent, const actor* act)
{
    if (act->res_negative_energy() >= 3)
        return false;

    if (!agent)
        return true;

    const monster* mons = agent->as_monster();
    const monster* m = act->as_monster();

    return !(agent->is_player() && act->wont_attack()
             || mons && act->is_player() && mons->wont_attack()
             || mons && m && mons_atts_aligned(mons->attitude, m->attitude));
}

static bool _damageable(const actor *caster, const actor *act)
{
    return act != caster
            && !(caster->deity() == GOD_FEDHAS
                && fedhas_protects(act->as_monster()));
}           

static void _los_spell_pre_damage_monsters(const actor* agent,
                                           vector<monster *> affected_monsters,
                                           const char *verb)
{
    // Filter out affected monsters that we don't know for sure are there
    vector<monster*> seen_monsters;
    for (monster *mon : affected_monsters)
        if (you.can_see(*mon))
            seen_monsters.push_back(mon);

    if (!seen_monsters.empty())
    {
        counted_monster_list mons_list =
            _counted_monster_list_from_vector(seen_monsters);
        const string message = make_stringf("%s %s %s.",
                mons_list.describe(DESC_THE).c_str(),
                conjugate_verb("be", mons_list.count() > 1).c_str(), verb);
        if (strwidth(message) < get_number_of_cols() - 2)
            mpr(message);
        else
        {
            // Exclamation mark to suggest that a lot of creatures were
            // affected.
            mprf("The monsters around %s are %s!",
                agent && agent->is_monster() && you.can_see(*agent)
                ? agent->as_monster()->name(DESC_THE).c_str()
                : "you", verb);
        }
    }
}

static int _los_spell_damage_player(actor* agent, bolt &beam,
                                    bool actual)
{
    int hurted = actual ? beam.damage.roll()
                        // Monsters use the average for foe calculations.
                        : (1 + beam.damage.max()) / 2;
    hurted = check_your_resists(hurted, beam.flavour, beam.name, 0,
            // Drain life doesn't apply drain effects.
            actual && beam.origin_spell != SPELL_DRAIN_LIFE);
    if (actual && hurted > 0)
    {
        bool chaos = beam.real_flavour == BEAM_CHAOTIC;
        if (beam.origin_spell == SPELL_OZOCUBUS_REFRIGERATION)
            mprf("You feel very %s.", chaos ? "spastic" : "cold");

        if (agent && !agent->is_player())
        {
            ouch(hurted, KILLED_BY_BEAM, agent->mid,
                 make_stringf("by %s", beam.name.c_str()).c_str(), true,
                 agent->as_monster()->name(DESC_A).c_str(), beam.flavour == BEAM_FIRE);
            you.expose_to_element(beam.flavour, 5);
            if (beam.origin_spell == SPELL_OZOCUBUS_REFRIGERATION && (player_res_cold() < 1))
                slow_player(hurted);
        }
        // -harm from player casting Ozo's Refridge.
        // we don't actually take damage, but can get slowed
        else if (beam.origin_spell == SPELL_OZOCUBUS_REFRIGERATION)
        {
            you.expose_to_element(beam.flavour, 5);
            if (chaos)
                temp_mutate(RANDOM_CORRUPT_MUTATION, "chaos magic");
            else
                you.increase_duration(DUR_NO_POTIONS, 7 + random2(9), 15);
        }
    }

    return hurted;
}

static int _los_spell_damage_monster(actor* agent, monster &target,
                                     bolt &beam, bool actual)
{

    beam.thrower = (agent && agent->is_player()) ? KILL_YOU :
                    agent                        ? KILL_MON
                                                 : KILL_MISC;

    // Set conducts here. The monster needs to be alive when this is done, and
    // mons_adjust_flavoured() could kill it.
    god_conduct_trigger conducts[3];
    if (YOU_KILL(beam.thrower))
        set_attack_conducts(conducts, target, you.can_see(target));

    beam.fake_flavour();
    if (beam.flavour == BEAM_WAND_HEALING)
        beam.flavour = BEAM_FIRE;

    int hurted = actual ? beam.damage.roll()
                        // Monsters use the average for foe calculations.
                        : (1 + beam.damage.max()) / 2;
    hurted = mons_adjust_flavoured(&target, beam, hurted,
                 // Drain life doesn't apply drain effects.
                 actual && beam.origin_spell != SPELL_DRAIN_LIFE);
    dprf("damage done: %d", hurted);

    if (actual)
    {
        if (YOU_KILL(beam.thrower))
            _player_hurt_monster(target, hurted, beam.flavour, false);
        else if (hurted)
            target.hurt(agent, hurted, beam.flavour);

        // Cold-blooded creatures can be slowed.
        if (target.alive())
        {
            if (beam.real_flavour == BEAM_CHAOTIC)
                chaotic_status(&target, 3 + hurted + random2(hurted), agent);
            else if (beam.origin_spell == SPELL_OZOCUBUS_REFRIGERATION && !(target.res_cold() > 0))
                target.slow_down(agent, hurted);
            target.expose_to_element(beam.flavour, 5);
        }
    }

    // So that summons don't restore HP.
    if (beam.origin_spell == SPELL_DRAIN_LIFE && target.is_summoned())
        return 0;

    return hurted;
}


static spret _cast_los_attack_spell(spell_type spell, int pow,
                                         actor* agent, actor* /*defender*/,
                                         bool actual, bool fail,
                                         int* damage_done)
{
    const monster* mons = agent ? agent->as_monster() : nullptr;

    const zap_type zap = spell_to_zap(spell);
    if (zap == NUM_ZAPS)
        return spret::abort;

    bolt beam;
    zappy(zap, pow, mons, beam);
    beam.source_id = agent ? agent->mid : MID_NOBODY;
    beam.foe_ratio = 80;
    bool chaos = beam.real_flavour == BEAM_CHAOTIC;

    const char *player_msg = nullptr, *global_msg = nullptr,
               *mons_vis_msg = nullptr, *mons_invis_msg = nullptr,
               *verb = nullptr, *prompt_verb = nullptr;
    bool (*vulnerable)(const actor *, const actor *) = nullptr;

    switch (spell)
    {
        case SPELL_OZOCUBUS_REFRIGERATION:
            if (chaos)
            {
                player_msg = "The order of the universe unravels around you.";
                global_msg = "Something unravels the area into chaos.";
                mons_vis_msg = " unravels the surrounding area into chaos!";
                mons_invis_msg = "Chaos unravels!";
                verb = "chaotically stricken";
            }
            else
            {
                player_msg = "The heat is drained from your surroundings.";
                global_msg = "Something drains the heat from around you.";
                mons_vis_msg = " drains the heat from the surrounding"
                    " environment!";
                mons_invis_msg = "The ambient heat is drained!";
                verb = "frozen";
            }
            if (you.staff() && get_staff_facet(*you.staff()) && staff_enhances_spell(you.staff(), SPELL_OZOCUBUS_REFRIGERATION) || you.wearing(EQ_AMULET, AMU_CHAOS))
            {
                prompt_verb = "refrigerate or chaotically strike";
                vulnerable = [](const actor *caster, const actor *act) {
                    return !(caster->deity() == GOD_FEDHAS
                            && fedhas_protects(act->as_monster()));
                };
            }
            else
            {
                prompt_verb = "refrigerate";
                vulnerable = [](const actor *caster, const actor *act) {
                    return act->is_player() || act->res_cold() < 3
                        && !(caster->deity() == GOD_FEDHAS
                            && fedhas_protects(act->as_monster()));
                };
            }
            break;

        case SPELL_DRAIN_LIFE:
            player_msg = "You draw life from your surroundings.";
            global_msg = "Something draws the life force from your"
                         " surroundings.";
            mons_vis_msg = " draws from the surrounding life force!";
            mons_invis_msg = "The surrounding life force dissipates!";
            verb = "drained of life";
            prompt_verb = "drain life";
            vulnerable = &_drain_lifeable;
            break;

        case SPELL_EMPOWERED_BREATH:
            player_msg = "You exhale a mighty gale!";
            global_msg = ""; // Enemies can't get it anyways.
            mons_vis_msg = "";
            mons_invis_msg = "";
            verb = "buffeted";
            prompt_verb = "breathe mighty wind";
            vulnerable = &_damageable;
            break;


        case SPELL_SONIC_WAVE:
            player_msg = "You send a blast of sound all around you.";
            global_msg = "Something sends a blast of sound all around you.";
            mons_vis_msg = " sends a blast of sound all around you!";
            mons_invis_msg = "Sound blasts the surrounding area!";
            verb = "blasted";
            // prompt_verb = "sing" The singing sword prompts in melee-attack
            vulnerable = &_damageable;
            break;

        default:
            return spret::abort;
    }

    auto vul_hitfunc = [vulnerable](const actor *act) -> bool
    {
        return (*vulnerable)(&you, act);
    };

    if (agent && agent->is_player())
    {
        ASSERT(actual);

        targeter_radius hitfunc(&you, LOS_NO_TRANS);
        // Singing Sword's spell shouldn't give a prompt at this time.
        if (spell != SPELL_SONIC_WAVE)
        {
            if (stop_attack_prompt(hitfunc, prompt_verb, vul_hitfunc))
                return spret::abort;

            fail_check();
        }

        mpr(player_msg);
        flash_view_delay(UA_PLAYER, beam.colour, 300, &hitfunc);
    }
    else if (actual)
    {
        if (!agent)
            mpr(global_msg);
        else if (you.can_see(*agent))
            simple_monster_message(*mons, mons_vis_msg);
        else if (you.see_cell(agent->pos()))
            mpr(mons_invis_msg);

        if (!agent || you.see_cell(agent->pos()))
            flash_view_delay(UA_MONSTER, beam.colour, 300);
    }

    bool affects_you = false;
    vector<monster *> affected_monsters;

    for (actor_near_iterator ai((agent ? agent : &you)->pos(), LOS_NO_TRANS);
         ai; ++ai)
    {
        if ((*vulnerable)(agent, *ai))
        {
            if (ai->is_player())
                affects_you = true;
            else
                affected_monsters.push_back(ai->as_monster());
        }
    }

    const int avg_damage = (1 + beam.damage.max()) / 2;
    int total_damage = 0;
    // XXX: This ordering is kind of broken; it's to preserve the message
    // order from the original behaviour in the case of refrigerate.
    if (affects_you)
    {
        total_damage = _los_spell_damage_player(agent, beam, actual);
        if (!actual && mons)
        {
            if (mons->wont_attack())
            {
                beam.friend_info.count++;
                beam.friend_info.power +=
                    (you.get_experience_level() * total_damage / avg_damage);
            }
            else
            {
                beam.foe_info.count++;
                beam.foe_info.power +=
                    (you.get_experience_level() * total_damage / avg_damage);
            }
        }
    }

    if (actual && !affected_monsters.empty())
        _los_spell_pre_damage_monsters(agent, affected_monsters, verb);

    for (auto m : affected_monsters)
    {
        // Watch out for invalidation. Example: Ozocubu's refrigeration on
        // a bunch of ballistomycete spores that blow each other up.
        if (!m->alive())
            continue;

        int this_damage = _los_spell_damage_monster(agent, *m, beam, actual);
        total_damage += this_damage;

        if (!actual && mons)
        {
            if (mons_atts_aligned(m->attitude, mons->attitude))
            {
                beam.friend_info.count++;
                beam.friend_info.power +=
                    (m->get_hit_dice() * this_damage / avg_damage);
            }
            else
            {
                beam.foe_info.count++;
                beam.foe_info.power +=
                    (m->get_hit_dice() * this_damage / avg_damage);
            }
        }
    }

    if (damage_done)
        *damage_done = total_damage;

    if (actual)
        return spret::success;
    return mons_should_fire(beam) ? spret::success : spret::abort;
}

spret trace_los_attack_spell(spell_type spell, int pow, const actor* agent)
{
    // This is a bad practice; but it works so sue me. 
    // (The proper practice would take redoing the entirety of mon-cast logic
    //  or duplicating a lot of the code.) ~Bcadren.

    actor * bad_agent = actor_by_mid(agent->mid);

    return _cast_los_attack_spell(spell, pow, bad_agent, nullptr, false, false,
                                  nullptr);
}

spret fire_los_attack_spell(spell_type spell, int pow, actor* agent,
                                 actor *defender, bool fail, int* damage_done)
{
    return _cast_los_attack_spell(spell, pow, agent, defender, true, fail,
                                  damage_done);
}

spret vampiric_drain(int pow, monster* mons, bool fail)
{
    if (you.hp == you.hp_max)
    {
        canned_msg(MSG_FULL_HEALTH);
        return spret::abort;
    }

    const bool observable = mons && mons->observable();
    if (!mons
        || !observable && !actor_is_susceptible_to_vampirism(*mons))
    {
        fail_check();

        canned_msg(MSG_NOTHING_CLOSE_ENOUGH);
        // Cost to disallow freely locating invisible
        // monsters.
        return spret::success;
    }

    // TODO: check known rN instead of holiness
    if (observable && !actor_is_susceptible_to_vampirism(*mons))
    {
        mpr("You can't drain life from that!");
        return spret::abort;
    }

    if (stop_attack_prompt(mons, false, you.pos()))
    {
        canned_msg(MSG_OK);
        return spret::abort;
    }

    fail_check();

    if (!mons->alive())
    {
        canned_msg(MSG_NOTHING_HAPPENS);
        return spret::success;
    }

    // The practical maximum of this is about 25 (pow @ 100). - bwr
    int hp_gain = 3 + random2avg(9, 2) + random2(pow) / 7;
    if (_is_menacing(&you, SPELL_VAMPIRIC_DRAINING))
        hp_gain *= div_rand_round(3 * hp_gain, 2);

    hp_gain = min(mons->hit_points, hp_gain);
    hp_gain = min(you.hp_max - you.hp, hp_gain);
    hp_gain = resist_adjust_damage(mons, BEAM_NEG, hp_gain);

    if (!hp_gain)
    {
        canned_msg(MSG_NOTHING_HAPPENS);
        return spret::success;
    }

    _player_hurt_monster(*mons, hp_gain, BEAM_NEG);

    hp_gain = div_rand_round(hp_gain, 2);

    if (hp_gain && !you.duration[DUR_DEATHS_DOOR])
    {
        mpr("You feel life coursing into your body.");
        inc_hp(hp_gain);
    }

    return spret::success;
}

spret cast_freeze(int pow, monster* mons, bool fail)
{
    pow = min(25, pow);

    if (!mons)
    {
        fail_check();
        canned_msg(MSG_NOTHING_CLOSE_ENOUGH);
        // If there's no monster there, you still pay the costs in
        // order to prevent locating invisible/submerged monsters.
        return spret::success;
    }

    if (stop_attack_prompt(mons, false, you.pos()))
    {
        canned_msg(MSG_OK);
        return spret::abort;
    }

    fail_check();

    beam_type damtype = BEAM_COLD;

    bool chaos = determine_chaos(&you, SPELL_FREEZE);

    if (chaos)
        damtype = chaos_damage_type(true);

    if (you.staff() && is_unrandom_artefact(*you.staff(), UNRAND_MAJIN))
        damtype = eldritch_damage_type();

    // Set conducts here. The monster needs to be alive when this is done, and
    // mons_adjust_flavoured() could kill it.
    god_conduct_trigger conducts[3];
    set_attack_conducts(conducts, *mons);

    bolt beam;
    beam.flavour = damtype;
    beam.thrower = KILL_YOU;

    string dam_verb = "freeze";
    if (chaos)
    {
        switch (damtype)
        {
        case BEAM_FIRE: dam_verb = "burn"; break;
        case BEAM_ELECTRICITY: dam_verb = "shock"; break;
        case BEAM_NEG: dam_verb = "drain"; break;
        case BEAM_ACID: dam_verb = "dissolve"; break;
        case BEAM_DAMNATION: dam_verb = "sear"; break;
        case BEAM_HOLY: dam_verb = "smite"; break;
        default:
        case BEAM_DEVASTATION: dam_verb = "discombobulate"; break;
        }
    }

    int orig_hurted = 0;

    if (you.staff() && staff_enhances_spell(you.staff(), SPELL_FREEZE)
                    && get_staff_facet(*you.staff()) == SPSTF_MENACE)
    {
        orig_hurted = roll_dice(2, 3 + pow / 3);
    }
    else
        orig_hurted = roll_dice(1, 3 + pow / 3);
    if (chaos)
        orig_hurted = div_rand_round(orig_hurted * 5, 4);
    int hurted = mons_adjust_flavoured(mons, beam, orig_hurted);
    mprf("You %s %s%s%s",
         dam_verb.c_str(),
         mons->name(DESC_THE).c_str(),
         hurted ? "" : " but do no damage",
         attack_strength_punctuation(hurted).c_str());

    _player_hurt_monster(*mons, hurted, beam.flavour, false);

    if (mons->alive())
        mons->expose_to_element(damtype, orig_hurted);

    return spret::success;
}

void cloud_strike(actor * caster, actor * foe, int damage)
{
    coord_def pos = foe->pos();
    if (!cloud_at(pos))
        return;
    if (foe->is_fairy() || foe->cloud_immune())
        return;
    cloud_type cloud = cloud_at(pos)->type;

    if (cloud == CLOUD_CHAOS)
    {
        int x = random2(9);
        switch (x)
        {
        case 0:
            cloud = CLOUD_HOLY;
        case 1:
            cloud = CLOUD_ACID;
        case 2:
            cloud = CLOUD_FIRE;
        case 3:
            cloud = CLOUD_COLD;
        case 4:
            cloud = CLOUD_NEGATIVE_ENERGY;
        case 5:
            cloud = CLOUD_MIASMA;
        case 6:
            cloud = CLOUD_PETRIFY;
        case 7:
            cloud = CLOUD_BLACK_SMOKE;
        case 8:
            cloud = CLOUD_STORM;
        default:
            cloud = CLOUD_HOLY;
        }
    }

    if (cloud == CLOUD_ROT)
    {
        if (foe->holiness() & (MH_UNDEAD | MH_NONLIVING))
            cloud = CLOUD_ACID;
        else
            cloud = CLOUD_MIASMA;
    }

    switch (cloud)
    {
    case CLOUD_NONE:
        return;
    case CLOUD_FIRE:
    case CLOUD_FOREST_FIRE:
        damage = resist_adjust_damage(foe, BEAM_FIRE, damage);
        if (damage > 0)
        {
            foe->hurt(caster, damage, BEAM_FIRE, KILLED_BY_BEAM,
                "", "by the air");
            if (foe->is_player())
                mprf("You are engulfed in scorching flames%s", 
                      attack_strength_punctuation(damage).c_str());
            else
            {
                string msg = make_stringf(" is engulfed in scorching flames%s", 
                                            attack_strength_punctuation(damage).c_str());
                simple_monster_message(*foe->as_monster(), msg.c_str());
            }
        }
        break;
    case CLOUD_STEAM:
        damage = resist_adjust_damage(foe, BEAM_FIRE, damage);
        if (damage > 0)
        {
            foe->hurt(caster, damage, BEAM_FIRE, KILLED_BY_BEAM,
                "", "by the air");
            if (foe->is_player())
                mprf("You are burned by the wild steam%s",
                      attack_strength_punctuation(damage).c_str());
            else
            {
                string msg = make_stringf(" is burned by wild steam%s",
                    attack_strength_punctuation(damage).c_str());
                simple_monster_message(*foe->as_monster(), msg.c_str());
            }
        }
        break;
    case CLOUD_MEPHITIC:
    case CLOUD_POISON:
    case CLOUD_MIASMA:
        damage = resist_adjust_damage(foe, BEAM_POISON, damage);
        if (damage > 0)
        {
            foe->hurt(caster, damage, BEAM_POISON, KILLED_BY_BEAM,
                "", "by the air");
            if (foe->is_player())
            {
                poison_player(1, "the air");
                mprf("You are engulfed by the poisonous vapours%s",
                    attack_strength_punctuation(damage).c_str());
            }
            else
            {
                poison_monster(foe->as_monster(), caster);
                string msg = make_stringf(" is engulfed in poisonous vapours%s",
                    attack_strength_punctuation(damage).c_str());
                simple_monster_message(*foe->as_monster(), msg.c_str());
            }
        }
        break;
    case CLOUD_COLD:
        damage = resist_adjust_damage(foe, BEAM_COLD, damage);
        if (damage > 0)
        {
            foe->hurt(caster, damage, BEAM_COLD, KILLED_BY_BEAM,
                "", "by the air");
            if (foe->is_player())
                mprf("The freezing vapours engulf you%s",
                    attack_strength_punctuation(damage).c_str());
            else
            {
                string msg = make_stringf(" is engulfed in freezing vapours%s",
                    attack_strength_punctuation(damage).c_str());
                simple_monster_message(*foe->as_monster(), msg.c_str());
            }
        }
        break;
    case CLOUD_HOLY:
        damage = resist_adjust_damage(foe, BEAM_HOLY, damage);
        if (damage > 0)
        {
            foe->hurt(caster, damage, BEAM_HOLY, KILLED_BY_BEAM,
                "", "by the air");
            if (foe->is_player())
                mprf("You are rebuked by the blessed clouds%s",
                    attack_strength_punctuation(damage).c_str());
            else
            {
                string msg = make_stringf(" is rebuked by the holy cloud%s",
                    attack_strength_punctuation(damage).c_str());
                simple_monster_message(*foe->as_monster(), msg.c_str());
            }
        }
        break;
    case CLOUD_BLACK_SMOKE:
    case CLOUD_GREY_SMOKE:
    case CLOUD_BLUE_SMOKE:
    case CLOUD_PURPLE_SMOKE:
    case CLOUD_TLOC_ENERGY:
    case CLOUD_MIST:
    case CLOUD_RAIN:
    case CLOUD_MAGIC_TRAIL:
    case CLOUD_DUST:
    case CLOUD_XOM_TRAIL:
    case CLOUD_SALT:
    case CLOUD_FLUFFY:
    case CLOUD_GOLD_DUST:
        damage = foe->apply_ac(damage, damage);
        if (damage > 0)
        {
            foe->hurt(caster, damage, BEAM_COLD, KILLED_BY_BEAM,
                "", "by the air");
            if (foe->is_player())
                mprf("The cloud makes the air strike you more forcefully%s",
                    attack_strength_punctuation(damage).c_str());
            else
            {
                string msg = make_stringf(" is struck by the cloud%s",
                    attack_strength_punctuation(damage).c_str());
                simple_monster_message(*foe->as_monster(), msg.c_str());
            }
        }
        break;
    case CLOUD_MUTAGENIC: // BCADDO: Change the mutagenic cloudstrike effect.
    case CLOUD_PETRIFY:
    case CLOUD_TORNADO:
        damage = foe->apply_ac(damage * 2, damage * 2);
        if (damage > 0)
        {
            foe->hurt(caster, damage, BEAM_COLD, KILLED_BY_BEAM,
                "", "by the air");
            if (foe->is_player())
                mprf("The dense cloud makes the air strike wildly%s",
                    attack_strength_punctuation(damage).c_str());
            else
            {
                string msg = make_stringf(" is struck by thick clouds%s",
                    attack_strength_punctuation(damage).c_str());
                simple_monster_message(*foe->as_monster(), msg.c_str());
            }
        }
        break;

    case CLOUD_BLOOD:
        damage = resist_adjust_damage(foe, BEAM_NEG, damage);
        if (damage > 0)
        {
            foe->hurt(caster, damage, BEAM_NEG, KILLED_BY_BEAM,
                "", "by the air");
            actor * src = cloud_at(pos)->agent();
            if (foe->is_player())
            {
                string msg = make_stringf("You are stricken by the vampiric fog%s",
                    attack_strength_punctuation(damage).c_str());
                if (src)
                {
                    int heals = random2avg(damage, 3);
                    src->heal(heals);
                     msg += make_stringf(" %s draws strength from your wounds%s",
                        src->name(DESC_THE).c_str(),
                        attack_strength_punctuation(heals).c_str());
                }
                mpr(msg);
            }
            else
            {
                string msg = make_stringf(" is stricken by the vampiric fog%s",
                    attack_strength_punctuation(damage).c_str());
                if (src && src != foe)
                {
                    int heals = random2avg(damage, 3);
                    src->heal(heals);
                    msg += make_stringf(" %s draws strength from %s wounds%s",
                        src->name(DESC_THE).c_str(),
                        foe->pronoun(PRONOUN_POSSESSIVE).c_str(),
                        attack_strength_punctuation(heals).c_str());
                }
                simple_monster_message(*foe->as_monster(), msg.c_str());
            }
        }
        break;
    case CLOUD_NEGATIVE_ENERGY:
    case CLOUD_SPECTRAL:
        damage = resist_adjust_damage(foe, BEAM_NEG, damage);
        if (damage > 0)
        {
            foe->hurt(caster, damage, BEAM_NEG, KILLED_BY_BEAM,
                "", "by the air");
            if (foe->is_player())
            {
                mprf("You are drained as the cloud strikes you%s",
                    attack_strength_punctuation(damage).c_str());
                drain_player();
            }
            else
            {
                string msg = make_stringf(" is drained by the clouds%s",
                    attack_strength_punctuation(damage).c_str());
                simple_monster_message(*foe->as_monster(), msg.c_str());
            }
        }
        break;
    case CLOUD_ACID:
        damage = resist_adjust_damage(foe, BEAM_ACID, damage);
        if (damage > 0)
        {
            foe->hurt(caster, damage, BEAM_ACID, KILLED_BY_BEAM,
                "", "by the air");
            if (foe->is_player())
                mprf("You are engulfed in acidic fog%s",
                    attack_strength_punctuation(damage).c_str());
            else
            {
                string msg = make_stringf(" is engulfed in acidic fog%s",
                    attack_strength_punctuation(damage).c_str());
                simple_monster_message(*foe->as_monster(), msg.c_str());
            }
        }
        break;
    case CLOUD_STORM:
        damage = resist_adjust_damage(foe, BEAM_ELECTRICITY, damage);
        if (damage > 0)
        {
            foe->hurt(caster, damage, BEAM_ELECTRICITY, KILLED_BY_BEAM,
                "", "by the air");
            if (foe->is_player())
                mprf("The airstrike triggers a lightning strike through you%s",
                    attack_strength_punctuation(damage).c_str());
            else
            {
                string msg = make_stringf(" is struck by lightning%s",
                    attack_strength_punctuation(damage).c_str());
                simple_monster_message(*foe->as_monster(), msg.c_str());
            }
            noisy(15, pos);
        }
        break;
    default:
        break;
    }
    if (foe->is_monster())
        print_wounds(*foe->as_monster());
    return;
}

spret warped_cast(zap_type zap, int pow, bolt target, actor * caster)
{
    if (cell_is_solid(target.target))
    {
        canned_msg(MSG_UNTHINKING_ACT);
        return spret::abort;
    }

    bolt beam;
    zappy(zap, pow, !caster->is_player(), beam);

    beam.ex_size = 0;

    if (zap == ZAP_FIREBALL)
        beam.ex_size = 1;

    beam.hit *= 3;
    beam.hit /= 2;
    beam.ench_power *= 5;
    beam.ench_power /= 4;
    beam.aimed_at_spot = true;
    beam.source = beam.target = target.target;
    beam.set_agent(caster);
    beam.fire();

    return spret::success;
}

spret cast_airstrike(int pow, const dist &beam, bool fail)
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

    if (you.can_see(*mons) && mons->res_wind())
    {
        mprf("But the air would do no harm to %s", mons->name(DESC_THE).c_str());
        return spret::abort;
    }

    if (stop_attack_prompt(mons, false, you.pos()))
        return spret::abort;
    fail_check();

    noisy(spell_effect_noise(SPELL_AIRSTRIKE), beam.target);

    bool chaos = determine_chaos(&you, SPELL_AIRSTRIKE);
    beam_type damtype = BEAM_AIR;

    if (chaos)
        damtype = chaos_damage_type(true);

    if (you.staff() && is_unrandom_artefact(*you.staff(), UNRAND_MAJIN))
        damtype = eldritch_damage_type();

    bolt pbeam;
    pbeam.flavour = damtype;

    int dam = 8 + random2avg(2 + div_rand_round(pow, 7), 3);
    if (chaos)
        dam = div_rand_round(5 * dam, 4);
    if (_is_menacing(&you, SPELL_AIRSTRIKE))
        dam = div_rand_round(3 * dam, 2);
    int hurted = mons->apply_ac(mons->beam_resists(pbeam, dam, false), 10 + div_round_up(pow, 7));
    dprf("preac: %d, postac: %d", dam, hurted);

    mprf("The air %stwists around and %sstrikes %s%s%s",
        chaos ? "chaotically " : "",
        mons->airborne() ? "violently " : "",
        mons->name(DESC_THE).c_str(),
        hurted ? "" : " but does no damage",
        attack_strength_punctuation(hurted).c_str());
    _player_hurt_monster(*mons, hurted, pbeam.flavour);

    if (mons->alive())
    {
        mons->expose_to_element(damtype, 2);
        cloud_strike(&you, mons, dam);
    }

    return spret::success;
}

static int _dice_for_mon_type(const monster_type mc)
{
    // Removed a lot of silly monsters down here... people, just because
    // it says ice, rock, or iron in the name doesn't mean it's actually
    // made out of the substance. - bwr
    switch (mc)
    { 
        // Double damage to stone, metal and crystal.
        case MONS_EARTH_ELEMENTAL:
        case MONS_USHABTI:
        case MONS_STATUE:
        case MONS_GARGOYLE:
        case MONS_IRON_ELEMENTAL:
        case MONS_IRON_GOLEM:
        case MONS_PEACEKEEPER:
        case MONS_WAR_GARGOYLE:
        case MONS_SALTLING:
        case MONS_CRYSTAL_GUARDIAN:
        case MONS_OBSIDIAN_STATUE:
        case MONS_ORANGE_STATUE:
        case MONS_ROXANNE:
            return 6;
        default:
            break;
    }

    if (mons_class_flag(mc, M_INSUBSTANTIAL))
        return 0;
    if (is_skeletal_type(mc) || is_icy_type(mc))
        return 6;
    if (mons_class_is_slime(mc))
        return 1;
    return 3;
}

static int _shatter_mount_dice()
{
    if (!you.mounted())
        return 0;

    int retval = 0;

    if (you.petrified(true))
        retval = 6;
    else
        retval = _dice_for_mon_type(mount_mons());

    if (retval == 0)
        return 0;

    if (you.petrifying(true))
        retval += 1;
    if (you.airborne())
        retval--;

    retval--;

    return max(retval, 1);
}

static int _shatter_mon_dice(const monster *mon)
{
    if (!mon)
        return 0;

    int retval = 0;

    if (mon->petrified())
        retval = 6;
    else
        retval = _dice_for_mon_type(mon->type);

    if (retval == 0)
        return 0;

    if (mon->petrifying())
        retval += 1;
    if (mon->airborne())
        retval -= 2;

    return max(retval, 1);
}

static int _shatter_monsters(coord_def where, int pow, actor *agent, bool chaos)
{
    dice_def dam_dice(0, 5 + pow / 3); // Number of dice set below.
    monster* mon = monster_at(where);

    if (!mon || !mon->alive() || mon == agent)
        return 0;

    dam_dice.num = _shatter_mon_dice(mon);
    if (_is_menacing(agent, SPELL_SHATTER) && dam_dice.num != 0)
        dam_dice.num++;
    int damage = mon->apply_ac(dam_dice.roll(), dam_dice.max(), ac_type::half);

    if (agent->is_player())
        _player_hurt_monster(*mon, damage, BEAM_MMISSILE);
    else if (damage)
        mon->hurt(agent, damage);

    if (chaos && mon->alive() && one_chance_in(5))
        chaotic_status(mon, div_rand_round(damage, 3), agent);

    return damage;
}

static int _shatter_walls(coord_def where, int /*pow*/, actor *agent)
{
    int chance = 0;

    // if not in-bounds then we can't really shatter it -- bwr
    if (!in_bounds(where))
        return 0;

    if (env.markers.property_at(where, MAT_ANY, "veto_shatter") == "veto")
        return 0;

    const dungeon_feature_type grid = grd(where);

    switch (grid)
    {
    case DNGN_CLOSED_DOOR:
    case DNGN_CLOSED_CLEAR_DOOR:
    case DNGN_RUNED_DOOR:
    case DNGN_RUNED_CLEAR_DOOR:
    case DNGN_OPEN_DOOR:
    case DNGN_OPEN_CLEAR_DOOR:
    case DNGN_SEALED_DOOR:
    case DNGN_SEALED_CLEAR_DOOR:
        if (you.see_cell(where))
            mpr("A door shatters!");
        chance = 100;
        break;

    case DNGN_GRATE:
        if (you.see_cell(where))
            mpr("An iron grate is ripped into pieces!");
        chance = 100;
        break;

    case DNGN_ORCISH_IDOL:
    case DNGN_SARCOPHAGUS:
    case DNGN_GRANITE_STATUE:
        chance = 100;
        break;

    case DNGN_SILVER_WALL:
    case DNGN_METAL_WALL:
        chance = 15;
        break;

    case DNGN_RUNED_CLEAR_STONE_WALL:
    case DNGN_CLEAR_STONE_WALL:
    case DNGN_STONE_WALL:
        chance = 25;
        break;

    case DNGN_CLEAR_ROCK_WALL:
    case DNGN_ROCK_WALL:
    case DNGN_SLIMY_WALL:
    case DNGN_CRYSTAL_WALL:
    case DNGN_TREE:
    case DNGN_SLIMESHROOM:
    case DNGN_MANGROVE:
        chance = 33;
        break;

    default:
        break;
    }

    if (agent->deity() == GOD_FEDHAS && feat_is_tree(grid))
        return 0;

    if (x_chance_in_y(chance, 100))
    {
        noisy(spell_effect_noise(SPELL_SHATTER), where);

        destroy_wall(where);

        if (agent->is_player() && grid == DNGN_ORCISH_IDOL)
            did_god_conduct(DID_DESTROY_ORCISH_IDOL, 8);

        return 1;
    }

    return 0;
}

static int _shatter_player_dice()
{
    if (you.is_insubstantial())
        return 0;

    int retval = 3;

    if (you.form == transformation::statue
        || you.form == transformation::ice_beast
        || you.species == SP_GARGOYLE
        || (you.get_mutation_level(MUT_DRACONIAN_DEFENSE, true) && you.drac_colour == DR_BONE)
        || you.petrified())
    {
        retval = 6;
    }

    if (you.petrifying())
        retval++; 
    if (you.airborne())
        retval -= 2;
    if (you.mounted())
        retval--;
    if (you.get_mutation_level(MUT_SLIME) >= 3)
        retval -= 2;

    return max(1, retval);
}

/**
 * Is this a valid target for shatter?
 *
 * @param act     The actor being considered
 * @return        Whether the actor will take damage from shatter.
 */
static bool _shatterable(const actor *act)
{
    return !act->is_insubstantial();
}

enum shatter_event_type
{
    SE_ERUPTION,
    SE_ICEFALL,
    SE_STALACTITE,
    SE_ANNOYED_DEMON,
    SE_STATUE,
    SE_CAVEIN
};

static void _shatter_chaos(actor * agent, int pow)
{
    int eruptions = 0;
    int icefalls = 0;
    int stalactites = 0;
    int annoyed_demons = 0;
    int statues = 0;
    bool cavein = false;

    for (rectangle_iterator ri(agent->pos(), LOS_RADIUS, true); ri; ++ri)
    {
        if (!feat_is_solid(grd(*ri)) && agent->see_cell_no_trans(*ri) && one_chance_in(40))
        {
            bolt beam;
            beam.set_agent(agent);
            beam.apply_beam_conducts();
            beam.target = *ri;
            beam.ex_size = 1;
            shatter_event_type happening = random_choose_weighted(10, SE_ERUPTION,
                4, SE_ICEFALL,
                4, SE_STALACTITE,
                3, SE_STATUE,
                10, SE_CAVEIN,
                6, SE_ANNOYED_DEMON);
            switch (happening)
            {
            case SE_ERUPTION:
                zappy(ZAP_CHAOS_ERUPTION, pow, false, beam);
                beam.explode();
                eruptions++;
                if (!feat_is_critical(grd(*ri)))
                    dungeon_terrain_changed(*ri, feat_is_water(grd(*ri)) ? DNGN_OBSIDIAN : DNGN_LAVA, false, true);
                for (rectangle_iterator sri(*ri, 1); sri; ++sri)
                    if (!feat_is_critical(grd(*sri)) && x_chance_in_y(2, 3) && (!feat_is_solid(grd(*sri)) || feat_is_tree(grd(*sri))))
                        dungeon_terrain_changed(*sri, feat_is_water(grd(*sri)) ? DNGN_OBSIDIAN : DNGN_LAVA, false, true);
                break;
            case SE_ICEFALL:
                zappy(ZAP_CHAOS_ICEFALL, pow, false, beam);
                beam.explode();
                icefalls++;
                break;
            case SE_STALACTITE:
                zappy(ZAP_CHAOS_STALACTITE, pow, false, beam);
                beam.explode();
                stalactites++;
                if (!feat_is_critical(grd(*ri)) && !(grd(*ri) == DNGN_FLOOR))
                    dungeon_terrain_changed(*ri, DNGN_FLOOR, false, true, false, false);
                break;
            case SE_ANNOYED_DEMON:
            {
                monster * m = create_monster(mgen_data(RANDOM_DEMON_GREATER, BEH_HOSTILE, *ri, MHITYOU, MG_FORBID_BANDS, GOD_MAKHLEB));
                if (m)
                {
                    beam.flavour = BEAM_VISUAL;
                    beam.colour = LIGHTMAGENTA;
                    beam.ex_size = 0;
                    beam.explode();

                    m->go_frenzy(agent);
                    m->flags |= MF_NO_REWARD;
                    m->add_ench(mon_enchant(ENCH_FAKE_ABJURATION, 2));

                    if (is_valid_shaft_level())
                        place_specific_trap(*ri, TRAP_SHAFT);

                    annoyed_demons++;
                }
                break;
            }
            case SE_STATUE:
                beam.flavour = BEAM_VISUAL;
                /* BCADDO: Golden statue chance?
                if (one_chance_in(4))
                beam.colour = YELLOW;
                */
                beam.colour = DARKGREY;
                beam.explode();
                dungeon_terrain_changed(*ri, DNGN_GRANITE_STATUE, false, true, false, false);
                statues++;
                break;
            case SE_CAVEIN:
                if (!actor_at(*ri) && !feat_is_critical(grd(*ri)))
                {
                    beam.flavour = BEAM_VISUAL;
                    beam.colour = BROWN;
                    beam.explode();
                    dungeon_terrain_changed(*ri, (you.where_are_you == BRANCH_SLIME && !jiyva_is_dead()) ? DNGN_SLIMY_WALL : DNGN_ROCK_WALL, true, true, false, false);
                    cavein = true;
                    for (rectangle_iterator tri(*ri, 1); tri; ++tri)
                        if (!feat_is_critical(grd(*tri)) && x_chance_in_y(3, 4))
                            dungeon_terrain_changed(*tri, (you.where_are_you == BRANCH_SLIME && !jiyva_is_dead()) ? DNGN_SLIMY_WALL : DNGN_ROCK_WALL, true, true, false, false);
                }
                break;
            }
        }
    }

    if (eruptions)
        mprf("The earthquake triggers %s.", eruptions > 1 ? "some eruptions" : "an eruption");

    if (statues)
        mprf("The chaotic vibrations somehow carve %s.", statues > 1 ? "some statues" : "a perfect statue");

    if (cavein)
        mpr("Part of the ceiling caves in.");

    if (icefalls || stalactites)
    {
        string local;
        if (icefalls)
        {
            if (icefalls > 1)
                local = "Some ice chunks";
            else
                local = "A chunk of ice";
            if (stalactites)
            {
                local += " and ";
                if (stalactites > 1)
                    local += "stone stalactites";
                else
                    local += "a stalactite";
            }
        }
        else
        {
            if (stalactites > 1)
                local += "stone stalactites";
            else
                local += "a stalactite";
        }
        bool plural = false;
        if (icefalls && stalactites || icefalls > 1 || stalactites > 1)
            plural = true;

        mprf("%s crash%s down from the ceiling.", local.c_str(), plural ? "es" : "");
    }

    if (annoyed_demons)
        mprf("%s appear%s, cursing about the noise.", annoyed_demons < 2 ? "An annoyed demon" : "Some annoyed demons"
                                                    , annoyed_demons > 1 ? "" : "s");
}

spret cast_shatter(int pow, bool fail)
{
    targeter_radius hitfunc(&you, LOS_ARENA);
    if (stop_attack_prompt(hitfunc, "attack", _shatterable))
        return spret::abort;

    fail_check();
    const bool silence = silenced(you.pos());

    if (silence)
        mpr("The dungeon shakes!");
    else
    {
        noisy(spell_effect_noise(SPELL_SHATTER), you.pos());
        mprf(MSGCH_SOUND, "The dungeon rumbles!");
    }

    bool chaos = determine_chaos(&you, SPELL_SHATTER);

    run_animation(ANIMATION_SHAKE_VIEWPORT, UA_PLAYER);

    int dest = 0;
    for (distance_iterator di(you.pos(), true, true, LOS_RADIUS); di; ++di)
    {
        // goes from the center out, so newly dug walls recurse
        if (!cell_see_cell(you.pos(), *di, LOS_SOLID))
            continue;

        _shatter_monsters(*di, pow, &you, chaos);
        dest += _shatter_walls(*di, pow, &you);
    }

    if (!silence && dest)
        mprf(MSGCH_SOUND, "Ka-crash!");

    if (chaos)
        _shatter_chaos(&you, pow);

    return spret::success;
}

static int _shatter_player(int pow, actor *wielder, bool devastator = false)
{
    if (wielder->is_player())
        return 0;

    dice_def dam_dice(_shatter_player_dice(), 5 + pow / 3);
    dice_def mount_dice(_shatter_mount_dice(), 5 + pow / 3);

    if (!devastator && dam_dice.num > 0 && _is_menacing(wielder, SPELL_SHATTER))
    {
        dam_dice.num++;
        mount_dice.num++;
    }

    int damage = you.apply_ac(dam_dice.roll(), dam_dice.max(), ac_type::half);
    int mntdmg = 0;
    
    if (you.mounted())
        mntdmg = apply_mount_ac(mount_dice.roll(), mount_dice.max(), ac_type::half);

    if (damage > 0)
    {
        mprf(damage > 15 ? "You shudder from the earth-shattering force%s"
                        : "You shudder%s",
             attack_strength_punctuation(damage).c_str());
        if (devastator)
            ouch(damage, KILLED_BY_MONSTER, wielder->mid);
        else
            ouch(damage, KILLED_BY_BEAM, wielder->mid, "by Shatter");
    }

    if (mntdmg)
    {
        mprf("Your %s trembles%s",
            you.mount_name(true).c_str(),
            attack_strength_punctuation(damage).c_str());

        damage_mount(mntdmg);
    }

    return damage;
}

bool mons_shatter(monster* caster, bool actual)
{
    const bool silence = silenced(caster->pos());
    int foes = 0;

    bool chaos = determine_chaos(caster, SPELL_SHATTER);
    chaos &= actual;

    if (actual)
    {
        if (silence)
        {
            mprf("The dungeon shakes around %s!",
                 caster->name(DESC_THE).c_str());
        }
        else
        {
            noisy(spell_effect_noise(SPELL_SHATTER), caster->pos(), caster->mid);
            mprf(MSGCH_SOUND, "The dungeon rumbles around %s!",
                 caster->name(DESC_THE).c_str());
        }
    }

    int pow = 5 + div_rand_round(caster->get_hit_dice() * 9, 2);

    int dest = 0;
    for (distance_iterator di(caster->pos(), true, true, LOS_RADIUS); di; ++di)
    {
        // goes from the center out, so newly dug walls recurse
        if (!cell_see_cell(caster->pos(), *di, LOS_SOLID))
            continue;

        if (actual)
        {
            _shatter_monsters(*di, pow, caster, chaos);
            if (*di == you.pos())
                _shatter_player(pow, caster);
            dest += _shatter_walls(*di, pow, caster);
        }
        else
        {
            if (you.pos() == *di)
                foes -= _shatter_player_dice();
            if (const monster *victim = monster_at(*di))
            {
                dprf("[%s]", victim->name(DESC_PLAIN, true).c_str());
                foes += _shatter_mon_dice(victim)
                     * (victim->wont_attack() ? -1 : 1);
            }
        }
    }

    if (dest && !silence)
        mprf(MSGCH_SOUND, "Ka-crash!");

    if (chaos)
        _shatter_chaos(caster, pow);

    if (actual)
        run_animation(ANIMATION_SHAKE_VIEWPORT, UA_MONSTER);

    if (!caster->wont_attack())
        foes *= -1;

    if (!actual)
        dprf("Shatter foe HD: %d", foes);

    return foes > 0; // doesn't matter if actual
}

void shillelagh(actor *wielder, coord_def where, int pow)
{
    bolt beam;
    beam.name = "shillelagh";
    beam.flavour = BEAM_VISUAL;
    beam.set_agent(wielder);
    beam.colour = BROWN;
    beam.glyph = dchar_glyph(DCHAR_EXPLOSION);
    beam.range = 1;
    beam.ex_size = 1;
    beam.is_explosion = true;
    beam.source = wielder->pos();
    beam.target = where;
    beam.hit = AUTOMATIC_HIT;
    beam.loudness = 7;
    beam.explode();

    counted_monster_list affected_monsters;
    for (adjacent_iterator ai(where, false); ai; ++ai)
    {
        monster *mon = monster_at(*ai);
        if (!mon || !mon->alive()
            || mon->is_insubstantial() || !you.can_see(*mon)
            || mon == wielder)
        {
            continue;
        }
        affected_monsters.add(mon);
    }
    if (!affected_monsters.empty())
    {
        const string message =
            make_stringf("%s shudder%s.",
                         affected_monsters.describe().c_str(),
                         affected_monsters.count() == 1? "s" : "");
        if (strwidth(message) < get_number_of_cols() - 2)
            mpr(message);
        else
            mpr("There is a shattering impact!");
    }

    // need to do this again to do the actual damage
    for (adjacent_iterator ai(where, false); ai; ++ai)
        _shatter_monsters(*ai, pow * 3 / 2, wielder, false);

    if ((you.pos() - wielder->pos()).rdist() <= 1 && in_bounds(you.pos()))
        _shatter_player(pow, wielder, true);
}

/**
 * Irradiate the given cell. (Per the spell.)
 *
 * @param where     The cell in question.
 * @param pow       The power with which the spell is being cast.
 * @param agent     The agent (player or monster) doing the irradiating.
 */
static int _irradiate_cell(coord_def where, int pow, actor *agent)
{
    actor *act = actor_at(where);
    if (!act || !act->alive())
        return 0; // XXX: handle damaging the player for mons casts...?

    bool chaos = determine_chaos(agent, SPELL_IRRADIATE);
    bool menace = _is_menacing(agent, SPELL_IRRADIATE);
    const int dice = menace ? 8 : 6;
    const int max_dam = chaos ? 40 + div_rand_round (5 * pow, 8) 
                              : 30 + div_rand_round(pow, 2);
    const dice_def dam_dice = calc_dice(dice, max_dam);
    const int dam = dam_dice.roll();
    const int dam2 = dam_dice.roll();
    if (act->is_player())
    {
        mprf("You are blasted with magical radiation%s",
            attack_strength_punctuation(dam).c_str());
        if (you.mounted())
        {
            mprf("Your %s is also blasted by the radiation%s",
                you.mount_name(true).c_str(),
                attack_strength_punctuation(dam2).c_str());

            damage_mount(dam2);

            if (you.mounted())
            {
                if (!you.duration[DUR_MOUNT_WRETCHED])
                    mprf("Your %s twists and deforms!", you.mount_name().c_str());
                you.increase_duration(DUR_MOUNT_WRETCHED, 5 + roll_dice(3, 8), 50);
            }
        }
    }
    else
        mprf("%s is blasted with magical radiation%s",
             act->name(DESC_THE).c_str(),
             attack_strength_punctuation(dam).c_str());
    dprf("irr for %d (%d pow, max %d)", dam, pow, max_dam);

    monster * mons = act->as_monster();

    if (agent->deity() == GOD_FEDHAS && fedhas_protects(mons))
    {
        simple_god_message(
                    make_stringf(" protects %s plant from harm.",
                        agent->is_player() ? "your" : "a").c_str(),
                    GOD_FEDHAS);
        return 0;
    }

    if (agent->is_player())
        _player_hurt_monster(*mons, dam, chaos ? BEAM_DEVASTATION : BEAM_MMISSILE);
    else if (dam)
        act->hurt(agent, dam, chaos ? BEAM_DEVASTATION : BEAM_MMISSILE);

    if (act->alive() && chaos)
        chaotic_debuff(act, dam, agent);

    if (act->alive())
        act->malmutate("magical radiation");

    return dam;
}

/**
 * Attempt to cast the spell "Irradiate", damaging & deforming enemies around
 * the player.
 *
 * @param pow   The power at which the spell is being cast.
 * @param who   The actor doing the irradiating.
 * @param fail  Whether the player has failed to cast the spell.
 * @return      spret::abort if the player changed their mind about casting after
 *              realizing they would hit an ally; spret::fail if they failed the
 *              cast chance; spret::success otherwise.
 */
spret cast_irradiate(int powc, actor* who, bool fail)
{
    targeter_radius hitfunc(who, LOS_NO_TRANS, 1, 0, 1);
    auto vulnerable = [who](const actor *act) -> bool
    {
        return !act->is_player()
               && !(who->deity() == GOD_FEDHAS
                    && fedhas_protects(act->as_monster()));
    };

    if (who->is_player() && stop_attack_prompt(hitfunc, "irradiate", vulnerable))
        return spret::abort;

    fail_check();

    ASSERT(who);
    if (who->is_player())
        mpr("You erupt in a fountain of uncontrolled magic!");
    else
    {
        simple_monster_message(*who->as_monster(),
                               " erupts in a fountain of uncontrolled magic!");
    }

    bolt beam;
    beam.name = "irradiate";
    beam.flavour = BEAM_VISUAL;
    beam.set_agent(who);
    beam.colour = ETC_MUTAGENIC;
    beam.glyph = dchar_glyph(DCHAR_EXPLOSION);
    beam.range = 1;
    beam.ex_size = 1;
    beam.is_explosion = true;
    beam.explode_delay = beam.explode_delay * 3 / 2;
    beam.source = who->pos();
    beam.target = who->pos();
    beam.hit = AUTOMATIC_HIT;
    beam.loudness = 0;
    beam.explode(true, true);

    apply_random_around_square([powc, who] (coord_def where) {
        return _irradiate_cell(where, powc, who);
    }, who->pos(), true, 8);

    if (who->is_player())
        contaminate_player(1000 + random2(500));
    return spret::success;
}

spret cast_heal_blast(int powc, actor* who, bool fail)
{
    fail_check();

    ASSERT(who);
    if (who->is_player())
        mpr("A fountain of healing energy spills forth from you!");
    else
    {
        simple_monster_message(*who->as_monster(),
            " erupts in a fountain of healing energy!");
    }

    bolt beam;
    beam.name = "healing mist";
    beam.flavour = BEAM_WAND_HEALING;
    beam.set_agent(who);
    beam.colour = WHITE;
    beam.glyph = dchar_glyph(DCHAR_EXPLOSION);
    beam.range = 1;
    beam.ex_size = 1;
    beam.is_explosion = true;
    beam.explode_delay = beam.explode_delay * 3 / 2;
    beam.source = who->pos();
    beam.target = who->pos();
    beam.hit = AUTOMATIC_HIT;
    beam.loudness = 20;
    beam.damage = dice_def(4, 3 + powc/3);
    beam.explode(true, false);

    return spret::success;
}

// How much work can we consider we'll have done by igniting a cloud here?
// Considers a cloud under a susceptible ally bad, a cloud under a a susceptible
// enemy good, and other clouds relatively unimportant.
static int _ignite_tracer_cloud_value(coord_def where, actor *agent)
{
    actor* act = actor_at(where);
    if (act)
    {
        const int dam = actor_cloud_immune(*act, CLOUD_FIRE)
                        ? 0
                        : resist_adjust_damage(act, BEAM_FIRE, 40);

        if (agent->deity() == GOD_FEDHAS && agent->is_player()
            && fedhas_protects(act->as_monster()))
        {
            return 0;
        }

        return mons_aligned(act, agent) ? -dam : dam;
    }
    // We've done something, but its value is indeterminate
    else
        return 1;
}

/**
 * Place flame clouds over toxic bogs, by the power of Ignite Poison.
 *
 * @param where     The tile in question.
 * @param pow       The power with which Ignite Poison is being cast.
 *                  If -1, this indicates the spell is a test-run 'tracer'.
 * @param agent     The caster of Ignite Poison.
 * @return          If we're just running a tracer, return the expected 'value'
 *                  of creating fire clouds in the given location (could be
 *                  negative if there are allies there).
 *                  If it's not a tracer, return 1 if a flame cloud is created
 *                  and 0 otherwise.
 */
static int _ignite_poison_bog(coord_def where, beam_type damtype, int pow, actor *agent)
{
    const bool tracer = (pow == -1);  // Only testing damage, not dealing it

    if (grd(where) != DNGN_TOXIC_BOG && grd(where) != DNGN_QUAGMIRE)
        return false;

    if (tracer)
    {
        const int value = _ignite_tracer_cloud_value(where, agent);
        // Player doesn't care about magnitude.
        return agent && agent->is_player() ? sgn(value) : value;
    }

    cloud_type cloud = CLOUD_FIRE;

    switch (damtype)
    {
    case BEAM_HOLY: cloud = CLOUD_HOLY;   break;
    case BEAM_COLD: cloud = CLOUD_COLD;   break;
    case BEAM_ACID: cloud = CLOUD_ACID;   break;
    case BEAM_NEG:  cloud = CLOUD_MIASMA; break;
    default:      /*cloud = CLOUD_FIRE;*/ break;
    }

    int dur = 30 + random2(20 + pow);

    if (damtype == BEAM_DEVASTATION)
        dur = random2(5);

    place_cloud(cloud, where, dur, agent);
    return true;
}

/**
 * Turn poisonous clouds in the given tile into flame clouds, by the power of
 * Ignite Poison.
 *
 * @param where     The tile in question.
 * @param pow       The power with which Ignite Poison is being cast.
 *                  If -1, this indicates the spell is a test-run 'tracer'.
 * @param agent     The caster of Ignite Poison.
 * @return          If we're just running a tracer, return the expected 'value'
 *                  of creating fire clouds in the given location (could be
 *                  negative if there are allies there).
 *                  If it's not a tracer, return 1 if a flame cloud is created
 *                  and 0 otherwise.
 */
static int _ignite_poison_clouds(coord_def where, beam_type damtype, int pow, actor *agent)
{
    const bool tracer = (pow == -1);  // Only testing damage, not dealing it

    cloud_struct* cloud = cloud_at(where);
    if (!cloud)
        return false;

    if (cloud->type != CLOUD_MEPHITIC && cloud->type != CLOUD_POISON)
        return false;

    if (tracer)
    {
        const int value = _ignite_tracer_cloud_value(where, agent);
        // Player doesn't care about magnitude.
        return agent && agent->is_player() ? sgn(value) : value;
    }

    switch (damtype)
    {
    case BEAM_HOLY: cloud->type = CLOUD_HOLY;   break;
    case BEAM_COLD: cloud->type = CLOUD_COLD;   break;
    case BEAM_ACID: cloud->type = CLOUD_ACID;   break;
    case BEAM_NEG:  cloud->type = CLOUD_MIASMA; break;
    default:        cloud->type = CLOUD_FIRE;   break;
    }

    cloud->decay = 30 + random2(20 + pow); // from 3-5 turns to 3-15 turns
    cloud->whose = agent->kill_alignment();
    cloud->killer = agent->is_player() ? KILL_YOU_MISSILE : KILL_MON_MISSILE;
    cloud->source = agent->mid;

    if (damtype == BEAM_DEVASTATION)
        cloud->decay = 1;

    return true;
}

/**
 * Burn poisoned monsters in the given tile, removing their poison state &
 * damaging them.
 *
 * @param where     The tile in question.
 * @param pow       The power with which Ignite Poison is being cast.
 *                  If -1, this indicates the spell is a test-run 'tracer'.
 * @param agent     The caster of Ignite Poison.
 * @return          If we're just running a tracer, return the expected damage
 *                  of burning the monster in the given location (could be
 *                  negative if there are allies there).
 *                  If it's not a tracer, return 1 if damage is caused & 0
 *                  otherwise.
 */
static int _ignite_poison_monsters(coord_def where, beam_type damtype, int pow, actor *agent)
{
    bolt beam;
    beam.flavour = damtype;   // This is dumb, only used for adjust!

    const bool tracer = (pow == -1);  // Only testing damage, not dealing it
    if (tracer)                       // Give some fake damage to test resists
        pow = 100;

    string verb;

    switch (damtype)
    {
    default:               verb = "burn";     break;
    case BEAM_COLD:        verb = "freeze";   break;
    case BEAM_NEG:         verb = "rot";      break;
    case BEAM_HOLY:        verb = "collapse"; break;
    case BEAM_ACID:        verb = "dissolve"; break;
    case BEAM_DEVASTATION: verb = "explode";  break;
    }

    // If a monster casts Ignite Poison, it can't hit itself.
    // This doesn't apply to the other functions: it can ignite
    // clouds where it's standing!

    monster* mon = monster_at(where);
    if (invalid_monster(mon) || mon == agent)
        return 0;

    // how poisoned is the victim?
    const mon_enchant ench = mon->get_ench(ENCH_POISON);
    const int pois_str = ench.ench == ENCH_NONE ? 0 : ench.degree;
    bool menacing = _is_menacing(agent, SPELL_IGNITE_POISON);
    bool chaos = (damtype != BEAM_COLD);

    // poison currently does roughly 6 damage per degree (over its duration)
    // do roughly 2x to 3x that much, scaling with spellpower
    const dice_def dam_dice(menacing ? div_rand_round(pois_str * 5, 2)
                                     : pois_str * 2, chaos ? 15 + div_rand_round(pow * 3,  40)
                                                           : 12 + div_rand_round(pow * 6, 100));

    const int base_dam = dam_dice.roll();
    int damage = mons_adjust_flavoured(mon, beam, base_dam, false);
    if (damtype == BEAM_ACID || damtype == BEAM_NEG)
        damage = div_rand_round(2 * damage, 3);
    if (damage <= 0)
        return 0;

    if (agent && agent->deity() == GOD_FEDHAS && fedhas_protects(mon))
    {
        if (!tracer)
        {
            simple_god_message(
                        make_stringf(" protects %s plant from harm.",
                            agent->is_player() ? "your" : "a").c_str(),
                        GOD_FEDHAS);
        }
        return 0;
    }

    mon->expose_to_element(damtype, damage);

    if (tracer)
    {
        // players don't care about magnitude, just care if enemies exist
        if (agent && agent->is_player())
            return mons_aligned(mon, agent) ? -1 : 1;
        return mons_aligned(mon, agent) ? -1 * damage : damage;
    }

    if (you.see_cell(mon->pos()))
    {
        mprf("%s seems to %s from within%s",
             mon->name(DESC_THE).c_str(),
             verb.c_str(),
             attack_strength_punctuation(damage).c_str());
    }

    dprf("Dice: %dd%d; Damage: %d", dam_dice.num, dam_dice.size, damage);

    mon->hurt(agent, damage, damtype);

    if (mon->alive())
    {
        behaviour_event(mon, ME_WHACK, agent);

        // Monster survived, remove any poison.
        mon->del_ench(ENCH_POISON, true); // suppress spam
        if (damtype == BEAM_ACID)
            mon->add_ench(mon_enchant(ENCH_CORROSION, 2, agent));
        if (damtype == BEAM_NEG)
            mon->drain_exp(agent, true);
        print_wounds(*mon);
    }

    return 1;
}

/**
 * Burn poisoned players in the given tile, removing their poison state &
 * damaging them.
 *
 * @param where     The tile in question.
 * @param pow       The power with which Ignite Poison is being cast.
 *                  If -1, this indicates the spell is a test-run 'tracer'.
 * @param agent     The caster of Ignite Poison.
 * @return          If we're just running a tracer, return the expected damage
 *                  of burning the player in the given location (could be
 *                  negative if the player is an ally).
 *                  If it's not a tracer, return 1 if damage is caused & 0
 *                  otherwise.
 */

static int _ignite_poison_player(coord_def where, beam_type damtype, int pow, actor *agent)
{
    if (agent->is_player() || where != you.pos())
        return 0;

    const bool tracer = (pow == -1);  // Only testing damage, not dealing it
    if (tracer)                       // Give some fake damage to test resists
        pow = 100;

    // Step down heavily beyond light poisoning (or we could easily one-shot a heavily poisoned character)
    const int pois_str = stepdown((double)you.duration[DUR_POISONING] / 5000,
                                  2.25);
    if (!pois_str)
        return 0;

    const int base_dam = roll_dice(pois_str, 5 + pow/7);
    const int damage = resist_adjust_damage(&you, damtype, base_dam);

    if (tracer)
        return mons_aligned(&you, agent) ? -1 * damage : damage;

    if (damtype == BEAM_FIRE)
    {
        const int resist = player_res_fire();
        if (resist > 0)
            mpr("You feel like your blood is boiling!");
        else if (resist < 0)
            mpr("The poison in your system burns terribly!");
        else
            mpr("The poison in your system burns!");
    }
    else
        mpr("The poison in your blood reacts to the chaos!");

    ouch(damage, KILLED_BY_BEAM, agent->mid,
         (damtype == BEAM_FIRE) ? "by burning poison" : "by chaotically catalyzed poison", 
         you.can_see(*agent), agent->as_monster()->name(DESC_A, true).c_str(), damtype == BEAM_FIRE);
    if (damage > 0)
    {
        you.expose_to_element(damtype, 2);
        if (damtype == BEAM_ACID)
            you.corrode_equipment("acidified poison", div_round_up(damage, 10));
        if (damtype == BEAM_NEG)
            drain_player();
    }

    mprf(MSGCH_RECOVERY, "You are no longer poisoned.");
    you.duration[DUR_POISONING] = 0;

    return damage ? 1 : 0;
}

/**
 * Would casting Ignite Poison possibly harm one of the player's allies in the
 * given cell?
 *
 * @param  where    The cell in question.
 * @return          1 if there's potential harm, 0 otherwise.
 */
static int _ignite_ally_harm(const coord_def &where)
{
    if (where == you.pos())
        return 0; // you're not your own ally!
    // (prevents issues with duplicate prompts when standing in an igniteable
    // cloud)

    return (_ignite_poison_clouds(where, BEAM_FIRE, -1, &you) < 0)   ? 1 :
           (_ignite_poison_monsters(where, BEAM_FIRE, -1, &you) < 0) ? 1 :
           (_ignite_poison_bog(where, BEAM_FIRE, -1, &you) < 0)      ? 1 :
            0;
}

/**
 * Let the player choose to abort a casting of ignite poison, if it seems
 * like a bad idea. (If they'd ignite themself.)
 *
 * @return      Whether the player chose to abort the casting.
 */
static bool maybe_abort_ignite()
{
    string prompt = "You are standing ";

    // XXX XXX XXX major code duplication (ChrisOelmueller)
    if (const cloud_struct* cloud = cloud_at(you.pos()))
    {
        if ((cloud->type == CLOUD_MEPHITIC || cloud->type == CLOUD_POISON)
            && !actor_cloud_immune(you, CLOUD_FIRE))
        {
            prompt += "in a cloud of ";
            prompt += cloud->cloud_name(true);
            prompt += "! Ignite poison anyway?";
            if (!yesno(prompt.c_str(), false, 'n'))
                return true;
        }
    }

    if (apply_area_visible(_ignite_ally_harm, you.pos()) > 0)
    {
        return !yesno("You might harm nearby allies! Ignite poison anyway?",
                      false, 'n');
    }

    return false;
}

/**
 * Does Ignite Poison affect the given creature?
 *
 * @param act       The creature in question.
 * @return          Whether Ignite Poison can directly damage the given
 *                  creature (not counting clouds).
 */
bool ignite_poison_affects(const actor* act)
{
    if (act->is_player())
        return you.duration[DUR_POISONING];
    return act->as_monster()->has_ench(ENCH_POISON);
}

// Is there ANY poison to transmute in the environment?
static bool _olgreb_check(actor * agent)
{
    coord_def center = agent->pos();
    for (radius_iterator ri(center, LOS_NO_TRANS, true); ri; ++ri)
    {
        if (agent->see_cell_no_trans(*ri))
        {
            if (actor * act = actor_at(*ri))
            {
                if (act->is_player() && you.duration[DUR_POISONING])
                    return true;
                else if (act->is_monster() && act->as_monster()->has_ench(ENCH_POISON))
                    return true;
            }
            if (cloud_struct * c = cloud_at(*ri))
            {
                if (c->type == CLOUD_POISON || c->type == CLOUD_MEPHITIC)
                    return true;
            }
            if (grd(*ri) == DNGN_TOXIC_BOG || grd(*ri) == DNGN_QUAGMIRE)
                return true;
        }
    }
    return false;
}

/**
 * Cast the spell Ignite Poison, burning poisoned creatures and poisonous
 * clouds in LOS.
 *
 * @param agent         The spell's caster.
 * @param pow           The power with which the spell is being cast.
 * @param fail          If it's a player spell, whether the spell fail chance
 *                      was hit (whether the spell will fail as soon as the
 *                      player chooses not to abort the casting)
 * @param mon_tracer    Whether the 'casting' is just a tracer (a check to see
 *                      if it's worth actually casting)
 * @return              If it's a tracer, spret::success if the spell should
 *                      be cast & spret::abort otherwise.
 *                      If it's a real spell, spret::abort if the player chose
 *                      to abort the spell, spret::fail if they failed the cast
 *                      chance, and spret::success otherwise.
 */
spret cast_ignite_poison(actor* agent, int pow, bool fail, bool tracer, bool olgreb)
{
    if (tracer)
    {
        // Estimate how much useful effect we'd get if we cast the spell now
        const int work = apply_area_visible([agent] (coord_def where) {
            return _ignite_poison_clouds(where, BEAM_FIRE, -1, agent)
                 + _ignite_poison_monsters(where, BEAM_FIRE, -1, agent)
                 + _ignite_poison_player(where, BEAM_FIRE, -1, agent)
                 + _ignite_poison_bog(where, BEAM_FIRE, -1, agent);
        }, agent->pos());

        return work > 0 ? spret::success : spret::abort;
    }

    if (!olgreb && agent->is_player())
    {
        if (maybe_abort_ignite())
        {
            canned_msg(MSG_OK);
            return spret::abort;
        }
        fail_check();
    }

    if (olgreb && !_olgreb_check(agent))
        return spret::abort;

    bool chaos = determine_chaos(agent, SPELL_IGNITE_POISON);
    bool nice = is_good_god(you.religion) && !(you.staff() && is_unrandom_artefact(*you.staff(), UNRAND_MAJIN));

    if (olgreb && !one_chance_in(4))
        chaos = true;

    beam_type dam_type = BEAM_FIRE;
    targeter_radius hitfunc(agent, LOS_NO_TRANS);
    colour_t tyr;
    string verb = "";

    if (chaos)
        dam_type = random_choose_weighted(3, BEAM_COLD, 
                                          5, BEAM_ACID, 
                                          4, nice ? BEAM_HOLY : BEAM_NEG,
                                          1, BEAM_HOLY, 
                                          6, BEAM_DEVASTATION);

    if (olgreb && dam_type == BEAM_HOLY)
        dam_type = BEAM_ACID;

    switch (dam_type)
    {
    default:
    case BEAM_FIRE:        verb = "ignite";  tyr = RED;       break;
    case BEAM_COLD:        verb = "freeze";  tyr = LIGHTCYAN; break;
    case BEAM_NEG:         verb = "rot";     tyr = BROWN;     break;
    case BEAM_HOLY:        verb = "bless";   tyr = ETC_HOLY;  break;
    case BEAM_ACID:        verb = "acidify"; tyr = YELLOW;    break;
    case BEAM_DEVASTATION: verb = "explode"; tyr = MAGENTA;   break;
    }

    flash_view_delay(agent->is_player() ? UA_PLAYER
                                        : UA_MONSTER,
                     tyr, 100, &hitfunc);

    if (olgreb)
        mpr("The staff of Olgreb transmutes the poison in its environment!");
    mprf("%s %s the poison in %s surroundings!", agent->name(DESC_THE).c_str(),
        agent->conj_verb(verb).c_str(),
        agent->pronoun(PRONOUN_POSSESSIVE).c_str());

    // this could conceivably cause crashes if the player dies midway through
    // maybe split it up...?
    apply_area_visible([dam_type, pow, agent] (coord_def where) {
        _ignite_poison_clouds(where, dam_type, pow, agent);
        _ignite_poison_monsters(where, dam_type, pow, agent);
        _ignite_poison_bog(where, dam_type, pow, agent);
        // Only relevant if a monster is casting this spell
        // (never hurts the caster)
        _ignite_poison_player(where, dam_type, pow, agent);
        return 0; // ignored
    }, agent->pos());

    return spret::success;
}

static void _ignition_square(const actor */*agent*/, bolt beam, coord_def square, bool center)
{
    // HACK: bypass visual effect
    beam.target = square;
    beam.in_explosion_phase = true;
    beam.explosion_affect_cell(square);
    if (center)
        noisy(spell_effect_noise(SPELL_IGNITION),square);
}

spret cast_ignition(const actor *agent, int pow, bool fail)
{
    ASSERT(agent->is_player());

    fail_check();

    bool chaos = determine_chaos(agent, SPELL_IGNITION);
    bool evil = (you.staff() && is_unrandom_artefact(*you.staff(), UNRAND_MAJIN));
    bool menacing = _is_menacing(agent, SPELL_IGNITION);
    //targeter_radius hitfunc(agent, LOS_NO_TRANS);

    // Ignition affects squares that had hostile monsters on them at the time
    // of casting. This way nothing bad happens when monsters die halfway
    // through the spell.
    vector<coord_def> blast_sources;

    for (actor_near_iterator ai(agent->pos(), LOS_NO_TRANS);
         ai; ++ai)
    {
        if (ai->is_monster()
            && !ai->as_monster()->wont_attack()
            && !mons_is_firewood(*ai->as_monster())
            && !mons_is_tentacle_segment(ai->as_monster()->type))
        {
            blast_sources.push_back(ai->position);
        }
    }

    if (blast_sources.empty())
        canned_msg(MSG_NOTHING_HAPPENS);
    else
    {
        if (!chaos)
            mpr("The air bursts into flame!");
        else
            mpr("Entropy unravels around your enemies!");

        vector<coord_def> blast_adjacents;

        // Used to draw explosion cells
        bolt beam_visual;
        beam_visual.set_agent(agent);
        beam_visual.flavour       = BEAM_VISUAL;
        beam_visual.glyph         = dchar_glyph(DCHAR_FIRED_BURST);
        beam_visual.ex_size       = 1;
        beam_visual.is_explosion  = true;

        // Used to deal damage; invisible
        bolt beam_actual;
        beam_actual.set_agent(agent);
        beam_actual.glyph         = 0;
        beam_actual.damage        = calc_dice(menacing ? 4 
                                                       : 3, chaos ? 25 / 2 + pow * 5 / 12 
                                                                  : 10 + pow/3); // less than fireball
        beam_actual.ex_size       = 0;
        beam_actual.is_explosion  = true;
        beam_actual.loudness      = 0;

        beam_type flavour = BEAM_FIRE;

        if (chaos)
        {
            if (evil)
            {
                beam_visual.colour = ETC_UNHOLY;
                flavour = BEAM_ELDRITCH;
                beam_actual.colour = ETC_UNHOLY;
                beam_actual.name = "eldritch blast";
            }
            else
            {
                beam_visual.colour = ETC_CHAOS;
                flavour = BEAM_CHAOTIC;
                beam_actual.colour = ETC_CHAOS;
                beam_actual.name = "entropic burst";
            }
        }
        else
        {
            beam_visual.colour = RED;
            beam_actual.colour = RED;
            beam_actual.name = "fireball";
        }

        beam_actual.flavour = flavour;
        beam_actual.real_flavour = flavour;

        beam_actual.apply_beam_conducts();

#ifdef DEBUG_DIAGNOSTICS
        dprf(DIAG_BEAM, "ignition dam=%dd%d",
             beam_actual.damage.num, beam_actual.damage.size);
#endif

        // Fake "shaped" radius 1 explosions (skipping squares with friends).
        for (coord_def pos : blast_sources)
        {
            for (adjacent_iterator ai(pos); ai; ++ai)
            {
                if (cell_is_solid(*ai))
                {
                    if (flavour == BEAM_FIRE && (feat_is_tree(grd(*ai)) || feat_is_door(grd(*ai))))
                    {
                        if (feat_is_tree(grd(*ai)) && agent->is_player())
                            did_god_conduct(DID_KILL_PLANT, 1, true);
                        // Destroy the wall.
                        destroy_wall(*ai);
                        if (you.see_cell(*ai))
                        {
                            if (feat_is_door(grd(*ai)))
                                mpr("The door bursts into flame!");
                            else if (player_in_branch(BRANCH_SWAMP))
                                mpr("The tree smoulders and burns.");
                            else
                                mpr("The tree burns like a torch!");
                        }
                        else if (you.can_smell())
                            mpr("You smell burning wood.");

                        // Trees do not burn so readily in a wet environment.
                        if (player_in_branch(BRANCH_SWAMP))
                            place_cloud(CLOUD_FIRE, *ai, random2(12) + 5, agent);
                        else
                            place_cloud(CLOUD_FOREST_FIRE, *ai, random2(30) + 25, agent);
                    }
                    continue;
                }
                actor *act = actor_at(*ai);

                // Friendly creature, don't blast this square.
                if (act && (act == agent
                            || (act->is_monster()
                                && act->as_monster()->wont_attack())))
                {
                    continue;
                }

                blast_adjacents.push_back(*ai);
                beam_visual.explosion_draw_cell(*ai);
            }
            beam_visual.explosion_draw_cell(pos);
        }
        update_screen();
        scaled_delay(50);

        // Real explosions on each individual square.
        for (coord_def pos : blast_sources)
            _ignition_square(agent, beam_actual, pos, true);
        for (coord_def pos : blast_adjacents)
            _ignition_square(agent, beam_actual, pos, false);
    }

    return spret::success;
}

static int _discharge_monsters(const coord_def &where, int pow,
                               const actor &agent, const bool chaos,
                               const bool evil)
{
    actor* victim = actor_at(where);

    if (!victim || !victim->alive())
        return 0;

    int damage = (&agent == victim) ? 1 + random2(3 + pow / 15)
                                    : 3 + random2(5 + pow / 10 
                                        + (random2(pow) / 10));

    if (_is_menacing(&agent, SPELL_DISCHARGE))
        damage += random2(damage);

    bolt beam;
    beam_type flavour = BEAM_ELECTRICITY;
    beam.glyph = dchar_glyph(DCHAR_FIRED_ZAP);
    beam.colour = LIGHTBLUE;

    if (chaos)
    {
        flavour = BEAM_CHAOTIC;
        beam.colour = ETC_CHAOS;
    }
    if (evil)
    {
        flavour = BEAM_ELDRITCH;
        beam.colour = ETC_UNHOLY;
    }
    beam.flavour = flavour;
    beam.real_flavour = flavour;

#ifdef USE_TILE
    beam.tile_beam  = -1;
#endif
    beam.draw_delay = 0;

    dprf("Static discharge on (%d,%d) pow: %d", where.x, where.y, pow);
    if (victim->is_player() || victim->res_elec() <= 0)
        beam.draw(where);

    if (victim->is_fairy())
    {
        // Elec resist is full resist for fairy.
        if (victim->res_elec())
            return 0;
        // Fairies are protected against most their own arcs.
        else if ((agent.mid == victim->mid) && !one_chance_in(4))
            return 0;
    }

    if (victim->is_player())
    {
        damage = 1 + random2(3 + pow / 15);
        dprf("You: static discharge damage: %d", damage);
        damage = check_your_resists(damage, BEAM_ELECTRICITY,
                                    "static discharge");
        mprf("You are struck by an arc of %s%s",
             chaos ? "chaos" : "electricity",
             attack_strength_punctuation(damage).c_str());
        ouch(damage, KILLED_BY_BEAM, agent.mid, chaos ? "by chaotic static" : "by static electricity", true,
             agent.is_player() ? "you" : agent.name(DESC_A).c_str());
        if (damage > 0)
            victim->expose_to_element(BEAM_ELECTRICITY, 2);
    }
    // rEelec monsters don't allow arcs to continue.
    else if (victim->as_monster()->immune_to_flavour(flavour))
        return 0;
    else if (agent.deity() == GOD_FEDHAS
             && fedhas_protects(victim->as_monster()))
    {
        simple_god_message(
                    make_stringf(" protects %s plant from harm.",
                        agent.is_player() ? "your" : "a").c_str(),
                    GOD_FEDHAS);
        return 0;
    }
    else
    {
        monster* mons = victim->as_monster();

        // We need to initialize these before the monster has died.
        god_conduct_trigger conducts[3];
        if (agent.is_player())
            set_attack_conducts(conducts, *mons, you.can_see(*mons));

        dprf("%s: static discharge damage: %d",
             mons->name(DESC_PLAIN, true).c_str(), damage);
        damage = mons_adjust_flavoured(mons, beam, damage);
        mprf("%s is struck by an arc of %s%s",
                mons->name(DESC_THE).c_str(),
                chaos ? "chaos" : "lightning",
                attack_strength_punctuation(damage).c_str());

        if (agent.is_player())
            _player_hurt_monster(*mons, damage, beam.flavour, false);
        else if (damage)
            mons->hurt(agent.as_monster(), damage);
    }

    // Recursion to give us chain-lightning -- bwr
    // Low power slight chance added for low power characters -- bwr
    if ((pow >= 10 && !one_chance_in(4)) || (pow >= 3 && one_chance_in(10)))
    {
        pow /= random_range(2, 3);
        damage += apply_random_around_square([pow, &agent, chaos, evil] (coord_def where2) {
            return _discharge_monsters(where2, pow, agent, chaos, evil);
        }, where, true, 1);
    }
    else if (damage > 0)
    {
        // Only printed if we did damage, so that the messages in
        // cast_discharge() are clean. -- bwr
        if (evil)
            mpr("The dark synaptic static calms down.");
        else if (chaos)
            mpr("The chaos dissipates.");
        else
            mpr("The lightning grounds out.");
    }

    return damage;
}

bool safe_discharge(coord_def where, vector<const actor *> &exclude)
{
    for (adjacent_iterator ai(where); ai; ++ai)
    {
        const actor *act = actor_at(*ai);
        if (!act)
            continue;

        if (find(exclude.begin(), exclude.end(), act) == exclude.end())
        {
            if (act->is_monster())
            {
                // Harmless to these monsters, so don't prompt about them.
                if (act->res_elec() > 0
                    || you.deity() == GOD_FEDHAS
                       && fedhas_protects(act->as_monster()))
                {
                    continue;
                }

                if (stop_attack_prompt(act->as_monster(), false, where))
                    return false;
            }
            // Don't prompt for the player, but always continue arcing.

            exclude.push_back(act);
            if (!safe_discharge(act->pos(), exclude))
                return false;
        }
    }

    return true;
}

spret cast_discharge(int pow, const actor &agent, bool fail, bool prompt)
{
    vector<const actor *> exclude;
    if (agent.is_player() && prompt && !safe_discharge(you.pos(), exclude))
        return spret::abort;

    fail_check();

    bool chaos = determine_chaos(&agent, SPELL_DISCHARGE);
    bool evil = (agent.staff() && is_unrandom_artefact(*agent.staff(), UNRAND_MAJIN));
    const int num_targs = 1 + random2(random_range(1, 3) + pow / 20);
    const int dam =
        apply_random_around_square([pow, &agent, chaos, evil] (coord_def target) {
            return _discharge_monsters(target, pow, agent, chaos, evil);
        }, agent.pos(), true, num_targs);

    dprf("Arcs: %d Damage: %d", num_targs, dam);

    if (dam > 0)
        scaled_delay(100);
    else
    {
        if (evil)
            mpr("Pure evil pours through cracks in reality.");
        else if (chaos)
            mpr("Entropic static scintillates and crepitates around.");
        else if (coinflip())
            mpr("The air crackles with electrical energy.");
        else
        {
            const bool plural = coinflip();
            mprf("%s blue arc%s ground%s harmlessly.",
                 plural ? "Some" : "A",
                 plural ? "s" : "",
                 plural ? " themselves" : "s itself");
        }
    }
    return spret::success;
}

static bool _finish_LRD_setup(bolt &beam, const actor *caster)
{
    if (determine_chaos(caster, SPELL_LRD))
    {
        if (caster->staff() && is_unrandom_artefact(*caster->staff(), UNRAND_MAJIN))
        {
            beam.name = "an eldritch " + beam.name;
            beam.colour = ETC_UNHOLY;
            beam.real_flavour = beam.flavour = BEAM_ELDRITCH;
        }
        else
        {
            beam.name = "an entropically infused " + beam.name;
            beam.colour = ETC_JEWEL;
            beam.real_flavour = beam.flavour = BEAM_CHAOTIC;
        }
        beam.damage.size = div_rand_round(5 * beam.damage.size, 4);
    }
    if (_is_menacing(caster, SPELL_LRD))
        beam.damage.num++;

    beam.aux_source = beam.name;

    return true;
}

bool setup_fragmentation_beam(bolt &beam, int pow, const actor *caster,
                              const coord_def target, bool quiet,
                              const char **what, bool &hole, bool &destroy)
{
    beam.flavour      = BEAM_FRAG;
    beam.glyph        = dchar_glyph(DCHAR_FIRED_BURST);
    beam.source_id    = caster->mid;
    beam.thrower      = caster->is_player() ? KILL_YOU : KILL_MON;
    beam.ex_size      = 1;
    beam.source       = you.pos();
    beam.hit          = AUTOMATIC_HIT;
    beam.origin_spell = SPELL_LRD;

    beam.source_name = caster->name(DESC_PLAIN, true);
    beam.aux_source = "by Lee's Rapid Deconstruction"; // for direct attack

    beam.target = target;

    // Number of dice vary from 2-4.
    beam.damage = dice_def(0, 5 + pow / 5);

    monster* mon = monster_at(target);
    const dungeon_feature_type grid = grd(target);

    destroy = false;

    if (target == you.pos())
    {
        const bool petrified = (you.petrified() || you.petrifying());

        if (you.form == transformation::statue || you.species == SP_GARGOYLE)
        {
            beam.name       = "blast of rock fragments";
            beam.colour     = BROWN;
            beam.damage.num = you.form == transformation::statue ? 3 : 2;
            return _finish_LRD_setup(beam, caster);
        }
        else if (petrified)
        {
            beam.name       = "blast of petrified fragments";
            beam.colour     = mons_class_colour(player_mons(true));
            beam.damage.num = 3;
            return _finish_LRD_setup(beam, caster);
        }
        else if (you.form == transformation::ice_beast) // blast of ice
        {
            beam.name       = "icy blast";
            beam.colour     = WHITE;
            beam.damage.num = 3;
            beam.flavour    = BEAM_ICE;
            return _finish_LRD_setup(beam, caster);
        }
        else if (you.get_mutation_level(MUT_DRACONIAN_DEFENSE, true) && you.drac_colour == DR_BONE)
        {
            beam.name = "blast of bone shards";
            beam.colour = WHITE;
            beam.damage.num = (you.form == transformation::dragon) ? 5 : 3;
            beam.flavour = BEAM_FRAG;
            return _finish_LRD_setup(beam, caster);
        }
    }
    else if (mon
             && mon->alive()
             && (caster->is_monster() || (you.can_see(*mon))))
    {
        switch (mon->type)
        {
        case MONS_TOENAIL_GOLEM:
            beam.name       = "blast of toenail fragments";
            beam.colour     = RED;
            beam.damage.num = 3;
            break;

        case MONS_IRON_ELEMENTAL:
        case MONS_IRON_GOLEM:
        case MONS_PEACEKEEPER:
        case MONS_WAR_GARGOYLE:
            beam.name       = "blast of metal fragments";
            beam.colour     = CYAN;
            beam.damage.num = 4;
            break;

        case MONS_EARTH_ELEMENTAL:
        case MONS_USHABTI:
        case MONS_STATUE:
        case MONS_GARGOYLE:
            beam.name       = "blast of rock fragments";
            beam.colour     = BROWN;
            beam.damage.num = 3;
            break;

        case MONS_SALTLING:
            beam.name       = "blast of salt crystal fragments";
            beam.colour     = WHITE;
            beam.damage.num = 3;
            break;

        case MONS_OBSIDIAN_STATUE:
        case MONS_ORANGE_STATUE:
        case MONS_CRYSTAL_GUARDIAN:
        case MONS_ROXANNE:
            beam.ex_size    = 2;
            beam.damage.num = 4;
            if (mon->type == MONS_OBSIDIAN_STATUE)
            {
                beam.name       = "blast of obsidian shards";
                beam.colour     = DARKGREY;
            }
            else if (mon->type == MONS_ORANGE_STATUE)
            {
                beam.name       = "blast of orange crystal shards";
                beam.colour     = LIGHTRED;
            }
            else if (mon->type == MONS_CRYSTAL_GUARDIAN)
            {
                beam.name       = "blast of crystal shards";
                beam.colour     = GREEN;
            }
            else
            {
                beam.name       = "blast of sapphire shards";
                beam.colour     = BLUE;
            }
            break;

        default:
            const bool petrified = (mon->petrified() || mon->petrifying());

            // Petrifying or petrified monsters can be exploded.
            if (petrified)
            {
                monster_info minfo(mon);
                beam.name       = "blast of petrified fragments";
                beam.colour     = minfo.colour();
                beam.damage.num = 3;
                break;
            }
            else if (mon->is_icy()) // blast of ice
            {
                beam.name       = "icy blast";
                beam.colour     = WHITE;
                beam.damage.num = 3;
                beam.flavour    = BEAM_ICE;
                break;
            }
            else if (mon->is_skeletal()) // blast of bone
            {
                beam.name   = "blast of bone shards";
                beam.colour = LIGHTGREY;
                beam.damage.num = 3;
                break;
            }
            // Targeted monster not shatterable, try the terrain instead.
            goto do_terrain;
        }
        
        // Got a target, let's blow it up.
        return _finish_LRD_setup(beam, caster);
    }

  do_terrain:
    switch (grid)
    {
    // Stone and rock terrain
    case DNGN_ORCISH_IDOL:
        if (!caster->is_player())
            return false; // don't let monsters blow up orcish idols

        if (what && *what == nullptr)
            *what = "stone idol";
        destroy = true;
        // fall-through
    case DNGN_ROCK_WALL:
    case DNGN_CLEAR_ROCK_WALL:
    case DNGN_SLIMY_WALL:
        if (what && *what == nullptr)
            *what = "wall";

        beam.name = "blast of rock fragments";
        beam.damage.num = 2;

        if (x_chance_in_y(pow, 500))
        {
            destroy = true;
            beam.ex_size++;
            beam.damage.num++;
        }

        break;
    case DNGN_STONE_WALL:
    case DNGN_CLEAR_STONE_WALL:
    case DNGN_RUNED_CLEAR_STONE_WALL:
        if (what && *what == nullptr)
            *what = "wall";

        beam.name = "blast of stone fragments";
        beam.damage.num = 3;

        if (x_chance_in_y(pow, 1600))
        {
            beam.ex_size++;
            beam.damage.num++;
            destroy = true;
        }
        break;
    case DNGN_GRANITE_STATUE:
        if (what && *what == nullptr)
            *what = "statue";

        beam.name       = "blast of granite fragments";
        beam.damage.num = 4;
        destroy = true;
        break;

    // Metal -- small but nasty explosion
    case DNGN_METAL_WALL:
        if (what)
            *what = "metal wall";
        beam.name = "blast of metal fragments";
        beam.damage.num = 4;
        break;

    case DNGN_GRATE:
        if (what && *what == nullptr)
            *what = "iron grate";
        beam.name = "blast of metal fragments";
        beam.damage.num = 4;
        destroy = true;
        break;

    // Silver
    case DNGN_SILVER_WALL: // Zin's blessed silver walls do bonus damage to chaotics.
        if (what)
            *what = "silver wall";
        beam.name = "blast of silver fragments";
        beam.colour = WHITE;
        beam.damage.num = 4;
        beam.flavour = BEAM_SILVER_FRAG;
        break;

    // Crystal
    case DNGN_CRYSTAL_WALL:       // crystal -- large & nasty explosion
        if (what)
            *what = "crystal wall";
        beam.ex_size    = 2;
        beam.name       = "blast of crystal shards";
        beam.damage.num = 4;
        if (coinflip())
        {
            beam.ex_size++;
            beam.damage.num++;
            destroy = true;
        }
        break;

    // Stone arches
    case DNGN_STONE_ARCH:
        if (what && *what == nullptr)
            *what = "stone arch";
        hole            = false;  // to hit monsters standing on doors
        beam.name       = "blast of rock fragments";
        beam.damage.num = 3;
        if (x_chance_in_y(pow, 1600))
            destroy = true;
        break;

    default:
        // Couldn't find a monster or wall to shatter - abort casting!
        if (caster->is_player() && !quiet)
            mpr("You can't deconstruct that!");
        return false;
    }

    // If it was recoloured, use that colour instead.
    if (env.grid_colours(target))
        beam.colour = env.grid_colours(target);
    else
        beam.colour = element_colour(get_feature_def(grid).colour(),
                                     false, target);

    return _finish_LRD_setup(beam, caster);
}

spret cast_fragmentation(int pow, const actor *caster,
                              const coord_def target, bool fail)
{
    bool hole                = true;
    bool destroy             = false;
    const char *what         = nullptr;
    const dungeon_feature_type grid = grd(target);

    bolt beam;

    if (!in_bounds(target))
    {
        mpr("The outer wall of the dungeon cannot be deconstructed...");
        return spret::abort;
    }

    if (!setup_fragmentation_beam(beam, pow, caster, target, false, &what, hole, destroy))
    {
        return spret::abort;
    }

    if (caster->is_player())
    {
        if (grid == DNGN_ORCISH_IDOL)
        {
            if (!yesno("Really insult Beogh by defacing this idol?",
                       false, 'n'))
            {
                canned_msg(MSG_OK);
                return spret::abort;
            }
        }

        bolt tempbeam;
        bool temp;
        setup_fragmentation_beam(tempbeam, pow, caster, target, true, nullptr,
                                 temp, temp);
        tempbeam.is_tracer = true;
        tempbeam.explode(false);
        if (tempbeam.beam_cancelled)
        {
            canned_msg(MSG_OK);
            return spret::abort;
        }
    }

    fail_check();

    if (what != nullptr) // Terrain explodes.
    {
        if (you.see_cell(target))
            mprf("The %s shatters!", what);

        if (destroy)
            destroy_wall(target);
    }
    else if (target == you.pos()) // You explode.
    {
        const int dam = beam.damage.roll();
        mprf("You shatter%s", attack_strength_punctuation(dam).c_str());

        ouch(dam, KILLED_BY_BEAM, caster->mid,
             "by Lee's Rapid Deconstruction", true,
             caster->is_player() ? "you"
                                 : caster->name(DESC_A).c_str());
    }
    else // Monster explodes.
    {
        // Checks by setup_fragmentation_beam() must guarantee that we have an
        // alive monster.
        monster* mon = monster_at(target);
        ASSERT(mon);

        const int dam = beam.damage.roll();
        if (you.see_cell(target))
        {
            mprf("%s shatters%s", mon->name(DESC_THE).c_str(),
                 attack_strength_punctuation(dam).c_str());
        }

        if (caster->is_player())
            _player_hurt_monster(*mon, dam, BEAM_DISINTEGRATION);
        else if (dam)
            mon->hurt(caster, dam, BEAM_DISINTEGRATION);
    }

    beam.explode(true, hole);

    // Monsters shouldn't be able to blow up idols,
    // but this check is here just in case...
    if (caster->is_player() && grid == DNGN_ORCISH_IDOL)
        did_god_conduct(DID_DESTROY_ORCISH_IDOL, 8);

    return spret::success;
}

static bool _elec_not_immune(const actor *act)
{
    return act->res_elec() < 3 && !(you_worship(GOD_FEDHAS)
                                    && fedhas_protects(act->as_monster()));
}

spret cast_thunderbolt(actor *caster, int pow, coord_def aim, bool fail)
{
    coord_def prev;

    int &charges = caster->props[THUNDERBOLT_CHARGES_KEY].get_int();
    ASSERT(charges <= LIGHTNING_MAX_CHARGE);

    int &last_turn = caster->props[THUNDERBOLT_LAST_KEY].get_int();
    coord_def &last_aim = caster->props[THUNDERBOLT_AIM_KEY].get_coord();


    if (last_turn && last_turn + 1 == you.num_turns)
        prev = last_aim;
    else
        charges = 0;

    targeter_thunderbolt hitfunc(caster, spell_range(SPELL_THUNDERBOLT, pow),
                                 prev);
    hitfunc.set_aim(aim);

    if (caster->is_player()
        && stop_attack_prompt(hitfunc, "zap", _elec_not_immune))
    {
        return spret::abort;
    }

    fail_check();

    const int juice
        = (spell_mana(SPELL_THUNDERBOLT) + charges) * LIGHTNING_CHARGE_MULT;

    dprf("juice: %d", juice);

    bolt beam;
    beam.name              = "thunderbolt";
    beam.aux_source        = "lightning rod";
    beam.flavour           = BEAM_ELECTRICITY;
    beam.glyph             = dchar_glyph(DCHAR_FIRED_BURST);
    beam.colour            = LIGHTCYAN;
    beam.range             = 1;
    beam.hit               = AUTOMATIC_HIT;
    beam.ac_rule           = ac_type::proportional;
    beam.set_agent(caster);
#ifdef USE_TILE
    beam.tile_beam = -1;
#endif
    beam.draw_delay = 0;

    for (const auto &entry : hitfunc.zapped)
    {
        if (entry.second <= 0)
            continue;

        beam.draw(entry.first);
    }

    scaled_delay(200);

    beam.glyph = 0; // FIXME: a hack to avoid "appears out of thin air"

    for (const auto &entry : hitfunc.zapped)
    {
        if (entry.second <= 0)
            continue;

        // beams are incredibly spammy in debug mode
        if (!actor_at(entry.first))
            continue;

        int arc = hitfunc.arc_length[entry.first.distance_from(hitfunc.origin)];
        ASSERT(arc > 0);
        dprf("at distance %d, arc length is %d",
             entry.first.distance_from(hitfunc.origin), arc);
        beam.source = beam.target = entry.first;
        beam.source.x -= sgn(beam.source.x - hitfunc.origin.x);
        beam.source.y -= sgn(beam.source.y - hitfunc.origin.y);
        beam.damage = dice_def(div_rand_round(juice, LIGHTNING_CHARGE_MULT),
                               div_rand_round(30 + pow / 6, arc + 2));
        beam.fire();
    }

    last_turn = you.num_turns;
    last_aim = aim;
    if (charges < LIGHTNING_MAX_CHARGE)
        charges++;

    return spret::success;
}

// Find an enemy who would suffer from Awaken Forest.
actor* forest_near_enemy(const actor *mon)
{
    const coord_def pos = mon->pos();

    for (radius_iterator ri(pos, LOS_NO_TRANS); ri; ++ri)
    {
        actor* foe = actor_at(*ri);
        if (!foe || mons_aligned(foe, mon))
            continue;

        for (adjacent_iterator ai(*ri); ai; ++ai)
            if (feat_is_tree(grd(*ai)) && cell_see_cell(pos, *ai, LOS_DEFAULT))
                return foe;
    }

    return nullptr;
}

// Print a message only if you can see any affected trees.
void forest_message(const coord_def pos, const string &msg, msg_channel_type ch)
{
    for (radius_iterator ri(pos, LOS_DEFAULT); ri; ++ri)
        if (feat_is_tree(grd(*ri))
            && cell_see_cell(you.pos(), *ri, LOS_DEFAULT))
        {
            mprf(ch, "%s", msg.c_str());
            return;
        }
}

void forest_damage(actor *mon)
{
    const coord_def pos = mon->pos();
    const int hd = mon->get_hit_dice();

    if (one_chance_in(4))
    {
        forest_message(pos, random_choose(
            "The trees move their gnarly branches around.",
            "You feel roots moving beneath the ground.",
            "Branches wave dangerously above you.",
            "Trunks creak and shift.",
            "Tree limbs sway around you."), MSGCH_TALK_VISUAL);
    }

    for (radius_iterator ri(pos, LOS_NO_TRANS); ri; ++ri)
    {
        actor* foe = actor_at(*ri);
        if (!foe || mons_aligned(foe, mon))
            continue;

        if (is_sanctuary(foe->pos()))
            continue;

        for (adjacent_iterator ai(*ri); ai; ++ai)
            if (feat_is_tree(grd(*ai)) && cell_see_cell(pos, *ai, LOS_NO_TRANS))
            {
                int dmg = 0;
                string msg;

                if (!apply_chunked_AC(1, foe->evasion(ev_ignore::none, mon)))
                {
                    msg = random_choose(
                            "@foe@ @is@ waved at by @branch@",
                            "A @tree@ reaches out but misses @foe@",
                            "A root lunges up near @foe@");
                }
                else if (!(dmg = foe->apply_ac(hd + random2(hd), hd * 2 - 1,
                                               ac_type::proportional)))
                {
                    msg = random_choose(
                            "@foe@ @is@ scraped by @branch@",
                            "A @tree@ reaches out and scrapes @foe@",
                            "A root barely touches @foe@ from below");
                    if (foe->is_monster())
                        behaviour_event(foe->as_monster(), ME_WHACK);
                }
                else
                {
                    msg = random_choose(
                        "@foe@ @is@ hit by @branch@",
                        "A @tree@ reaches out and hits @foe@",
                        "A root smacks @foe@ from below");
                    if (foe->is_monster())
                        behaviour_event(foe->as_monster(), ME_WHACK);
                }

                msg = replace_all(replace_all(replace_all(replace_all(msg,
                    "@foe@", foe->name(DESC_THE)),
                    "@is@", foe->conj_verb("be")),
                    "@tree@", player_in_branch(BRANCH_SLIME) ? "mushroom" : "tree"),
                    "@branch@", player_in_branch(BRANCH_SLIME) ? "some gills" : "a branch")
                    + attack_strength_punctuation(dmg);
                if (you.see_cell(foe->pos()))
                    mpr(msg);

                if (dmg <= 0)
                    break;

                if (determine_chaos(mon, SPELL_AWAKEN_FOREST) && one_chance_in(3))
                {
                    mprf("Chaos arcs from the awakened %s.", player_in_branch(BRANCH_SLIME) ? "mushrooms" : "trees");
                    chaotic_status(foe, 5 + random2(15), mon);
                }

                foe->hurt(mon, dmg, BEAM_MISSILE, KILLED_BY_BEAM, "", player_in_branch(BRANCH_SLIME) ? "by angry mushrooms" :
                          "by angry trees");

                break;
            }
    }
}

vector<bolt> get_spray_rays(const actor *caster, coord_def aim, int range,
                            int max_rays, int max_spacing)
{
    coord_def aim_dir = (caster->pos() - aim).sgn();

    int num_targets = 0;
    vector<bolt> beams;

    bolt base_beam;

    base_beam.set_agent(caster);
    base_beam.attitude = caster->is_player() ? ATT_FRIENDLY
                                             : caster->as_monster()->attitude;
    base_beam.is_tracer = true;
    base_beam.is_targeting = true;
    base_beam.dont_stop_player = true;
    base_beam.friend_info.dont_stop = true;
    base_beam.foe_info.dont_stop = true;
    base_beam.range = range;
    base_beam.source = caster->pos();
    base_beam.target = aim;

    bolt center_beam = base_beam;
    center_beam.hit = AUTOMATIC_HIT;
    center_beam.fire();
    center_beam.target = center_beam.path_taken.back();
    center_beam.hit = 1;
    center_beam.fire();
    center_beam.is_tracer = false;
    center_beam.dont_stop_player = false;
    center_beam.foe_info.dont_stop = false;
    center_beam.friend_info.dont_stop = false;
    // Prevent self-hits, specifically when you aim at an adjacent wall.
    if (center_beam.path_taken.back() != caster->pos())
        beams.push_back(center_beam);

    for (distance_iterator di(aim, false, false, max_spacing); di; ++di)
    {
        if (monster_at(*di))
        {
            coord_def delta = caster->pos() - *di;

            //Don't aim secondary rays at friendlies
            if (mons_aligned(caster, monster_at(*di)))
                continue;

            if (!caster->can_see(*monster_at(*di)))
                continue;

            //Don't try to aim at a target if it's out of range
            if (delta.rdist() > range)
                continue;

            //Don't try to aim at targets in the opposite direction of main aim
            if (abs(aim_dir.x - delta.sgn().x) + abs(aim_dir.y - delta.sgn().y) >= 2)
                continue;

            //Test if this beam stops at a location used by any prior beam
            bolt testbeam = base_beam;
            testbeam.target = *di;
            testbeam.hit = AUTOMATIC_HIT;
            testbeam.fire();
            bool duplicate = false;

            for (const bolt &beam : beams)
            {
                if (testbeam.path_taken.back() == beam.target)
                {
                    duplicate = true;
                    continue;
                }
            }
            if (!duplicate)
            {
                bolt tempbeam = base_beam;
                tempbeam.target = testbeam.path_taken.back();
                tempbeam.fire();
                tempbeam.is_tracer = false;
                tempbeam.is_targeting = false;
                tempbeam.dont_stop_player = false;
                tempbeam.foe_info.dont_stop = false;
                tempbeam.friend_info.dont_stop = false;
                beams.push_back(tempbeam);
                num_targets++;
            }

            if (num_targets == max_rays - 1)
              break;
        }
    }

    return beams;
}

static bool _dazzle_can_hit(const actor *act)
{
    if (act->is_monster())
    {
        const monster* mons = act->as_monster();
        bolt testbeam;
        testbeam.thrower = KILL_YOU;
        zappy(ZAP_BLINDING_SPRAY, 100, false, testbeam);

        return !testbeam.ignores_monster(mons);
    }
    else
        return false;
}

spret cast_dazzling_spray(int pow, coord_def aim, bool fail)
{
    int range = spell_range(SPELL_BLINDING_SPRAY, pow);

    targeter_spray hitfunc(&you, range, ZAP_BLINDING_SPRAY);
    hitfunc.set_aim(aim);
    if (stop_attack_prompt(hitfunc, "fire towards", _dazzle_can_hit))
        return spret::abort;

    fail_check();

    if (hitfunc.beams.size() == 0)
    {
        mpr("You can't see any targets in that direction!");
        return spret::abort;
    }

    for (bolt &beam : hitfunc.beams)
    {
        zappy(ZAP_BLINDING_SPRAY, pow, false, beam);
        beam.fire();
    }

    return spret::success;
}

static bool _toxic_can_affect(const actor *act)
{
    // currently monsters are still immune at rPois 1
    return act->res_poison() < (act->is_player() ? 3 : 1);
}

spret cast_toxic_radiance(actor *agent, int pow, bool fail, bool mon_tracer)
{
    bool chaos = determine_chaos(agent, SPELL_OLGREBS_TOXIC_RADIANCE);
    if (agent->is_player())
    {
        targeter_radius hitfunc(&you, LOS_NO_TRANS);
        if (stop_attack_prompt(hitfunc, "poison", _toxic_can_affect))
            return spret::abort;

        fail_check();

        if (!you.duration[DUR_TOXIC_RADIANCE])
            mpr("You begin to radiate toxic energy.");
        else
            mpr("Your toxic radiance grows in intensity.");

        you.increase_duration(DUR_TOXIC_RADIANCE, 2 + random2(pow/20), 15);
        toxic_radiance_effect(&you, 10, true);

        flash_view_delay(UA_PLAYER, chaos ? colour_t(ETC_SLIME) : colour_t(GREEN), 300, &hitfunc);

        return spret::success;
    }
    else if (mon_tracer)
    {
        for (actor_near_iterator ai(agent->pos(), LOS_NO_TRANS); ai; ++ai)
        {
            if (!_toxic_can_affect(*ai) || mons_aligned(agent, *ai))
                continue;
            else
                return spret::success;
        }

        // Didn't find any susceptible targets
        return spret::abort;
    }
    else
    {
        monster* mon_agent = agent->as_monster();
        simple_monster_message(*mon_agent,
                               " begins to radiate toxic energy.");

        mon_agent->add_ench(mon_enchant(ENCH_TOXIC_RADIANCE, 1, mon_agent,
                                        (4 + random2avg(pow/15, 2)) * BASELINE_DELAY));
        toxic_radiance_effect(agent, 10);

        targeter_radius hitfunc(mon_agent, LOS_NO_TRANS);
        flash_view_delay(UA_MONSTER, chaos ? colour_t(ETC_SLIME) : colour_t(GREEN), 300, &hitfunc);

        return spret::success;
    }
}

/*
 * Attempt to poison all monsters in line of sight of the caster.
 *
 * @param agent   The caster.
 * @param mult    A number to multiply the damage by.
 *                This is the time taken for the player's action in auts,
 *                or 10 if the spell was cast this turn.
 * @param on_cast Whether the spell was cast this turn. This only matters
 *                if the player cast the spell. If true, we trigger conducts
 *                if the player hurts allies; if false, we don't, to avoid
 *                the player being accidentally put under penance.
 *                Defaults to false.
 */
void toxic_radiance_effect(actor* agent, int mult, bool on_cast, bool chaos)
{
    if (!chaos)
        chaos = determine_chaos(agent, SPELL_OLGREBS_TOXIC_RADIANCE);
    int pow;
    if (agent->is_player())
        pow = calc_spell_power(SPELL_OLGREBS_TOXIC_RADIANCE, true);
    else
        pow = agent->as_monster()->get_hit_dice() * 8;

    for (actor_near_iterator ai(agent->pos(), LOS_NO_TRANS); ai; ++ai)
    {
        if (!chaos && !_toxic_can_affect(*ai))
            continue;

        // Monsters can skip hurting friendlies
        if (agent->is_monster() && mons_aligned(agent, *ai))
            continue;

        beam_type damtype = BEAM_POISON;

        if (chaos)
            damtype = chaos_damage_type(agent->is_player());

        bool menace = _is_menacing(agent, SPELL_OLGREBS_TOXIC_RADIANCE);

        int dam = roll_dice(menace ? 2 : 1, chaos ? 1 + pow/ 16 : 1 + pow / 20)
                                            * div_rand_round(mult, BASELINE_DELAY);
        dam = resist_adjust_damage(*ai, damtype, dam);

        if (ai->is_player())
        {
            // We're affected only if we're not the agent.
            if (!agent->is_player())
            {
                ouch(dam, KILLED_BY_BEAM, agent->mid,
                    "by Olgreb's Toxic Radiance", true,
                    agent->as_monster()->name(DESC_A).c_str());

                poison_player(roll_dice(2, 3), agent->name(DESC_A),
                              "toxic radiance", false);

                if (you.mounted())
                {
                    int dam2 = roll_dice(menace ? 2 : 1, chaos ? 1 + pow / 16 : 1 + pow / 20)
                        * div_rand_round(mult, BASELINE_DELAY);
                    dam2 = resist_adjust_damage(*ai, damtype, dam2, true);

                    damage_mount(dam2);

                    if (you.mounted())
                        poison_mount(roll_dice(2, 3));
                }
            }
        }
        else
        {
            god_conduct_trigger conducts[3];

            // Only trigger conducts on the turn the player casts the spell
            // (see PR #999).
            if (on_cast && agent->is_player())
                set_attack_conducts(conducts, *ai->as_monster());

            ai->hurt(agent, dam, damtype);

            if (ai->alive())
            {
                behaviour_event(ai->as_monster(), ME_ANNOY, agent,
                                agent->pos());
                ai->expose_to_element(damtype, 1);
                int q = mult / BASELINE_DELAY;
                int levels = roll_dice(q, 2) - q + (roll_dice(1, 20) <= (mult % BASELINE_DELAY));
                if (!ai->as_monster()->has_ench(ENCH_POISON)) // Always apply poison to an unpoisoned enemy
                    levels = max(levels, 1);
                poison_monster(ai->as_monster(), agent, levels);
                if (chaos && (on_cast || one_chance_in(5)))
                    chaotic_debuff(*ai, 5 + random2(15), agent);
            }
        }
    }
}

spret cast_searing_ray(int pow, bolt &beam, bool fail)
{
    const spret ret = zapping(ZAP_SEARING_RAY_I, pow, beam, true, nullptr,
                                   fail);

    if (ret == spret::success)
    {
        // Special value, used to avoid terminating ray immediately, since we
        // took a non-wait action on this turn (ie: casting it)
        you.attribute[ATTR_SEARING_RAY] = -1;
        you.props["searing_ray_target"].get_coord() = beam.target;
        you.props["searing_ray_aimed_at_spot"].get_bool() = beam.aimed_at_spot;
        string msg = "(Press <w>%</w> to maintain the ray.)";
        insert_commands(msg, { CMD_WAIT });
        mpr(msg);
    }

    return ret;
}

void handle_searing_ray()
{
    ASSERT_RANGE(you.attribute[ATTR_SEARING_RAY], 1, 4);

    // All of these effects interrupt a channeled ray
    if (you.confused() || you.berserk())
    {
        end_searing_ray();
        return;
    }

    if (!enough_mp(1, true) && (you.species != SP_FAIRY))
    {
        mpr("Without enough magic to sustain it, your searing ray dissipates.");
        end_searing_ray();
        return;
    }

    const zap_type zap = zap_type(ZAP_SEARING_RAY_I + (you.attribute[ATTR_SEARING_RAY]-1));
    const int pow = calc_spell_power(SPELL_SEARING_RAY, true);

    bolt beam;
    beam.thrower = KILL_YOU_MISSILE;
    beam.range   = calc_spell_range(SPELL_SEARING_RAY, pow);
    beam.flavour = BEAM_LAVA;
    beam.source  = you.pos();
    beam.target  = you.props["searing_ray_target"].get_coord();
    beam.aimed_at_spot = you.props["searing_ray_aimed_at_spot"].get_bool();

    // If friendlies have moved into the beam path, give a chance to abort
    if (!player_tracer(zap, pow, beam))
    {
        mpr("You stop channeling your searing ray.");
        end_searing_ray();
        return;
    }

    zappy(zap, pow, false, beam);

    if (determine_chaos(&you, SPELL_SEARING_RAY))
        beam.flavour = beam.real_flavour = BEAM_CHAOTIC;

    if (you.staff() && is_unrandom_artefact(*you.staff(), UNRAND_MAJIN))
        beam.flavour = beam.real_flavour = BEAM_ELDRITCH;

    aim_battlesphere(&you, SPELL_SEARING_RAY, pow, beam);
    beam.fire();
    trigger_battlesphere(&you, beam);

    if (you.species != SP_FAIRY)
        dec_mp(1);

    if (++you.attribute[ATTR_SEARING_RAY] > 3)
    {
        mpr("You finish channeling your searing ray.");
        end_searing_ray();
    }
}

void end_searing_ray()
{
    you.attribute[ATTR_SEARING_RAY] = 0;
    you.props.erase("searing_ray_target");
    you.props.erase("searing_ray_aimed_at_spot");
}

/**
 * Can a casting of Glaciate by the player injure the given creature?
 *
 * @param victim        The potential victim.
 * @return              Whether Glaciate can harm that victim.
 *                      (False for IOODs or friendly battlespheres.)
 */
static bool _player_glaciate_affects(const actor *victim)
{
    // TODO: deduplicate this with beam::ignores
    if (!victim)
        return false;

    const monster* mon = victim->as_monster();
    if (!mon) // player
        return true;

    return !mons_is_projectile(*mon)
            && (!mons_is_avatar(mon->type) || !mons_aligned(&you, mon));
}

spret cast_glaciate(actor *caster, int pow, coord_def aim, bool fail)
{
    const int range = spell_range(SPELL_GLACIATE, pow);
    targeter_cone hitfunc(caster, range);
    hitfunc.set_aim(aim);

    if (caster->is_player()
        && stop_attack_prompt(hitfunc, "glaciate", _player_glaciate_affects))
    {
        return spret::abort;
    }

    bool chaos = determine_chaos(caster, SPELL_GLACIATE);
    bool evil = caster->staff() && is_unrandom_artefact(*caster->staff(), UNRAND_MAJIN);

    fail_check();

    bolt beam;
    if (chaos)
    {
        if (evil)
        {
            beam.name = "eldritch fissure";
            beam.aux_source = "great undoing";
            beam.real_flavour = BEAM_ELDRITCH;
            beam.flavour = BEAM_ELDRITCH;
            beam.colour = ETC_UNHOLY;
        }
        else
        {
            beam.name = "cone of craziness";
            beam.aux_source = "great chaos burst";
            beam.real_flavour = BEAM_CHAOTIC;
            beam.flavour = BEAM_CHAOTIC;
            beam.colour = LIGHTCYAN;
        }
    }
    else
    {
        beam.name = "great icy blast";
        beam.aux_source = "great icy blast";
        beam.real_flavour = BEAM_FREEZE;
        beam.flavour = BEAM_FREEZE;
        beam.colour = WHITE;
    }
    beam.glyph             = dchar_glyph(DCHAR_EXPLOSION);
    beam.range             = 1;
    beam.hit               = AUTOMATIC_HIT;
    beam.source_id         = caster->mid;
    beam.hit_verb          = "engulfs";
    beam.origin_spell      = SPELL_GLACIATE;
    beam.set_agent(caster);
#ifdef USE_TILE
    beam.tile_beam = -1;
#endif
    beam.draw_delay = 0;

    for (int i = 1; i <= range; i++)
    {
        for (const auto &entry : hitfunc.sweep[i])
        {
            if (entry.second <= 0)
                continue;

            beam.draw(entry.first);
        }
        scaled_delay(25);
    }

    scaled_delay(100);

    if (you.can_see(*caster) || caster->is_player())
    {
        if (!chaos)
            mprf("%s %s a mighty blast of ice!",
                 caster->name(DESC_THE).c_str(),
                 caster->conj_verb("conjure").c_str());
        else
            mprf("%s %s a grandiose cone of craziness!",
                caster->name(DESC_THE).c_str(),
                caster->conj_verb("exude").c_str());
    }

    beam.glyph = 0;

    for (int i = 1; i <= range; i++)
    {
        for (const auto &entry : hitfunc.sweep[i])
        {
            if (entry.second <= 0)
                continue;

            const int eff_range = max(3, (6 * i / LOS_DEFAULT_RANGE));

            // At or within range 3, this is equivalent to the old Ice Storm
            // damage.
            beam.damage =
                caster->is_player()
                    ? calc_dice(7, (66 + 3 * pow) / eff_range)
                    : calc_dice(10, (54 + 3 * pow / 2) / eff_range);

            if (chaos)
                beam.damage.size = div_rand_round(5 * beam.damage.size, 4);
            if (_is_menacing(caster, SPELL_GLACIATE))
                beam.damage.num += 2;

            beam.fake_flavour();

            beam.source = beam.target = entry.first;
            beam.source.x -= sgn(beam.source.x - hitfunc.origin.x);
            beam.source.y -= sgn(beam.source.y - hitfunc.origin.y);
            beam.fire();
            
            place_cloud(chaos ? chaos_cloud() : CLOUD_COLD, entry.first,
                        (18 + random2avg(45,2)) / eff_range, caster);
        }
    }

    noisy(spell_effect_noise(SPELL_GLACIATE), hitfunc.origin);

    return spret::success;
}

spret cast_random_bolt(int pow, bolt& beam, bool fail)
{
    // Need to use a 'generic' tracer regardless of the actual beam type,
    // to account for the possibility of both bouncing and irresistible damage
    // (even though only one of these two ever occurs on the same bolt type).
    bolt tracer = beam;
    if (!player_tracer(ZAP_RANDOM_BOLT_TRACER, 200, tracer))
        return spret::abort;

    fail_check();

    zap_type zap = random_choose(ZAP_BOLT_OF_FIRE,
                                 ZAP_BOLT_OF_COLD,
                                 ZAP_VENOM_BOLT,
                                 ZAP_BOLT_OF_DRAINING,
                                 ZAP_QUICKSILVER_BOLT,
                                 ZAP_CRYSTAL_BOLT,
                                 ZAP_LIGHTNING_BOLT,
                                 ZAP_CORROSIVE_BOLT);
    beam.origin_spell = SPELL_NO_SPELL; // let zapping reset this
    zapping(zap, pow * 7 / 6 + 15, beam, false);

    return spret::success;
}

size_t shotgun_beam_count(int pow)
{
    return 2 + div_round_up(pow, 7);
}

spret cast_scattershot(const actor *caster, int pow, const coord_def &pos,
                            bool fail, zap_type zap, bool empowered)
{
    const size_t range = spell_range(SPELL_SCATTERSHOT, pow);
    size_t beam_count = shotgun_beam_count(pow);

    if (zap != ZAP_SCATTERSHOT)
        beam_count *= 2;

    targeter_shotgun hitfunc(caster, beam_count, range);

    hitfunc.set_aim(pos);

    if (caster->is_player())
    {
        if (stop_attack_prompt(hitfunc, zap == ZAP_SCATTERSHOT  ? "scattershot" : "breathe at"))
            return spret::abort;
    }

    fail_check();

    bolt beam;
    beam.thrower = (caster && caster->is_player()) ? KILL_YOU :
                   (caster)                        ? KILL_MON
                                                   : KILL_MISC;
    beam.range       = range;
    beam.source      = caster->pos();
    beam.source_id   = caster->mid;
    beam.source_name = caster->name(DESC_PLAIN, true);
    zappy(zap, pow, false, beam);
    beam.aux_source  = beam.name;

    map<mid_t, int> hit_count;

    // for each beam...
    for (size_t i = 0; i < beam_count; i++)
    {
        // find the beam's path.
        ray_def ray = hitfunc.rays[i];
        for (size_t j = 0; j < range; j++)
            ray.advance();

        // fire the beam...
        bolt tempbeam = beam;
        tempbeam.draw_delay = 0;
        tempbeam.target = ray.pos();
        tempbeam.fire();
        scaled_delay(5);
        for (auto it : tempbeam.hit_count)
            hit_count[it.first] += it.second;
    }

    for (auto it : hit_count)
    {
        if (it.first == MID_PLAYER)
            continue;

        monster* mons = monster_by_mid(it.first);
        if (!mons || !mons->alive() || !you.can_see(*mons))
            continue;

        if (empowered)
        {
            switch (zap)
            {
            case ZAP_BREATHE_BONE:
                impale_monster_with_barbs(mons, &you, "bone shards");
                break;
            case ZAP_BREATHE_METAL:
                impale_monster_with_barbs(mons, &you, "metal splinters");
                break;
            case ZAP_BREATHE_SILVER:
            {
                int degree = max(max(1, mons->how_chaotic(true)), mons->how_unclean(false));
                int check = div_rand_round(pow, 3) - mons->get_hit_dice();
                check -= random2(5);
                if (check > 1)
                {
                    zin_eff effect = effect_for_prayer_type(RECITE_BREATH, check, 0, mons);
                    zin_affect(mons, effect, degree, RECITE_BREATH, pow);
                }
                break;
            }
            default:
                break;
            }
        }

        print_wounds(*mons);
    }

    return spret::success;
}

static void _setup_borgnjors_vile_clutch(bolt &beam, int pow)
{
    beam.name         = "vile clutch";
    beam.aux_source   = "vile_clutch";
    beam.flavour      = BEAM_VILE_CLUTCH;
    beam.glyph        = dchar_glyph(DCHAR_FIRED_BURST);
    beam.colour       = GREEN;
    beam.source_id    = MID_PLAYER;
    beam.thrower      = KILL_YOU;
    beam.is_explosion = true;
    beam.ex_size      = 1;
    beam.ench_power   = pow;
    beam.origin_spell = SPELL_BORGNJORS_VILE_CLUTCH;
}

spret cast_borgnjors_vile_clutch(int pow, bolt &beam, bool fail)
{
    if (cell_is_solid(beam.target))
    {
        canned_msg(MSG_SOMETHING_IN_WAY);
        return spret::abort;
    }

    fail_check();

    _setup_borgnjors_vile_clutch(beam, pow);
    mpr("Decaying hands burst forth from the earth!");
    beam.explode();

    return spret::success;
}

static void _chaos_bolt_flavour(bolt beam, int x)
{
    bool good = is_good_god(you.religion) && !(you.staff() && is_unrandom_artefact(*you.staff(), UNRAND_MAJIN));

    x %= 8;
    switch (x)
    {
    default:
    case 0:
    case 4:
        beam.real_flavour = BEAM_CHAOTIC;
        beam.colour = ETC_JEWEL;
        break;
    case 1:
        beam.flavour = BEAM_ACID;
        beam.colour = YELLOW;
        break;
    case 2:
        if (good)
        {
            beam.flavour = BEAM_HOLY;
            beam.colour = ETC_HOLY;
        }
        else
        {
            beam.flavour = BEAM_DAMNATION;
            beam.colour = LIGHTRED;
        }
        break;
    case 3:
        beam.flavour = BEAM_COLD;
        beam.colour = LIGHTCYAN;
        break;
    case 5:
        if (good)
        {
            beam.flavour = BEAM_WAND_HEALING;
            beam.colour = ETC_HEAL;
        }
        else
        {
            beam.flavour = BEAM_MIASMA;
            beam.colour = ETC_DARK;
        }
        break;
    case 6:
        beam.flavour = BEAM_LAVA;
        beam.colour = RED;
        break;
    case 7:
        beam.flavour = BEAM_ELECTRICITY;
        beam.colour = LIGHTMAGENTA;
        break;
    }
}

spret cast_starburst(int pow, bool fail, bool tracer)
{
    int range = spell_range(SPELL_STARBURST, pow);

    bool chaos = determine_chaos(&you, SPELL_STARBURST);
    int x = random2(7);

    vector<coord_def> offsets = { coord_def(range, 0),
                                coord_def(range, range),
                                coord_def(0, range),
                                coord_def(-range, range),
                                coord_def(-range, 0),
                                coord_def(-range, -range),
                                coord_def(0, -range),
                                coord_def(range, -range) };

    bolt beam;
    beam.range        = range;
    beam.source       = you.pos();
    beam.source_id    = MID_PLAYER;
    beam.is_tracer    = tracer;
    beam.is_targeting = tracer;
    beam.dont_stop_player = true;
    beam.friend_info.dont_stop = true;
    beam.foe_info.dont_stop = true;
    beam.attitude = ATT_FRIENDLY;
    beam.thrower      = KILL_YOU;
    beam.origin_spell = SPELL_STARBURST;
    beam.draw_delay   = 5;
    zappy(ZAP_BOLT_OF_FIRE, pow, false, beam);
    if (_is_menacing(&you, SPELL_STARBURST))
        beam.damage.num++;

    for (const coord_def & offset : offsets)
    {
        beam.target = you.pos() + offset;
        if (!tracer && !player_tracer(ZAP_BOLT_OF_FIRE, pow, beam))
            return spret::abort;

        if (tracer)
        {
            if (chaos)
            {
                _chaos_bolt_flavour(beam, x);
                x++;
            }
            beam.fire();
            // something to hit
            if (beam.foe_info.count > 0)
                return spret::success;
        }
    }

    if (tracer)
        return spret::abort;

    fail_check();

    // Randomize for nice animations
    shuffle_array(offsets);
    for (auto & offset : offsets)
    {
        beam.target = you.pos() + offset;
        beam.fire();
    }

    return spret::success;
}

void foxfire_attack(const monster *foxfire, const actor *target)
{
    actor * summoner = actor_by_mid(foxfire->summoner);
    const bool chaos = foxfire->type == MONS_EPHEMERAL_SPIRIT;

    // Don't allow foxfires that have wandered off to attack before dissapating
    if (summoner && !(summoner->can_see(*foxfire)
                      && summoner->see_cell(target->pos())))
    {
        return;
    }

    bolt beam;
    beam.thrower = (foxfire && foxfire->friendly()) ? KILL_YOU :
                   (foxfire)                       ? KILL_MON
                                                  : KILL_MISC;
    beam.range       = 1;
    beam.source      = foxfire->pos();
    beam.source_id   = foxfire->summoner;
    beam.source_name = summoner->name(DESC_PLAIN, true);
    zappy(chaos ? ZAP_CHAOSFIRE : ZAP_FOXFIRE, foxfire->get_hit_dice(), !foxfire->friendly(), beam);
    if (_is_menacing(&you, SPELL_FOXFIRE))
        beam.damage.num++;
    beam.aux_source  = beam.name;
    beam.target      = target->pos();
    beam.fire();
}

/**
 * Hailstorm the given cell. (Per the spell.)
 *
 * @param where     The cell in question.
 * @param pow       The power with which the spell is being cast.
 * @param agent     The agent (player or monster) doing the hailstorming.
 */
static void _hailstorm_cell(coord_def where, int pow, actor *agent, bool chaos)
{
    bolt beam;
    beam.thrower    = agent->is_player() ? KILL_YOU : KILL_MON;
    beam.source_id  = agent->mid;
    beam.attitude   = agent->temp_attitude();
    beam.glyph      = dchar_glyph(DCHAR_FIRED_BURST);
#ifdef USE_TILE
    beam.tile_beam  = -1;
#endif
    beam.draw_delay = 10;
    beam.source     = where;
    beam.target     = where;
    beam.hit        = 18 + pow / 6;
    if (chaos)
    {
        beam.real_flavour = beam.flavour = BEAM_CHAOTIC;
        beam.colour = ETC_JEWEL;
        beam.name = "chaos shards";
        beam.hit_verb = "pelt";
        beam.damage = calc_dice(3, 13 + (pow * 5) / 8);
    }
    else
    {
        beam.flavour = BEAM_ICE;
        beam.colour = ETC_ICE;
        beam.name = "hail";
        beam.hit_verb = "pelts";
        beam.damage = calc_dice(3, 10 + pow / 2);
    }

    if (_is_menacing(&you, SPELL_HAILSTORM))
        beam.damage.num++;

    monster *mons = monster_at(where);
    if (!chaos && mons && mons->is_icy())
    {
        string msg;
        msg = "%s is unaffected.";
        if (!mons->is_stationary() && one_chance_in(8))
            msg = "%s dances in the hail.";
        if (you.can_see(*mons))
            mprf(msg.c_str(), mons->name(DESC_THE).c_str());
        else
            mprf(msg.c_str(), "Something");

        beam.draw(where);
        return;
    }
    if (chaos && mons && (mons->type == MONS_CHAOS_ELEMENTAL || mons->has_ench(ENCH_CHAOTIC_INFUSION)
        || mons->is_shapeshifter()))
    {
        string msg;
        one_chance_in(8)  ? msg = "%s gyres and gimbles in the cannonade." :
                            msg = "%s is unaffected.";
        if (you.can_see(*mons))
            mprf(msg.c_str(), mons->name(DESC_THE).c_str());
        else
            mprf(msg.c_str(), "Something");

        beam.draw(where);
        return;
    }

    beam.fire();
}

spret cast_hailstorm(int pow, bool fail, bool tracer)
{
    targeter_radius hitfunc(&you, LOS_NO_TRANS, 3, 0, 2);
    bool (*vulnerable) (const actor *) = [](const actor * act) -> bool
    {
        return !act->is_icy()
            && !(you_worship(GOD_FEDHAS)
                 && fedhas_protects(act->as_monster()));
    };

    if (tracer)
    {
        for (radius_iterator ri(you.pos(), 3, C_SQUARE, LOS_NO_TRANS, true); ri; ++ri)
        {
            if (grid_distance(you.pos(), *ri) == 1 || !in_bounds(*ri))
                continue;

            const monster* mon = monster_at(*ri);

            if (!mon || !you.can_see(*mon))
                continue;

            if (!mon->friendly() && (*vulnerable)(mon))
                return spret::success;
        }

        return spret::abort;
    }

    if (stop_attack_prompt(hitfunc, "hailstorm", vulnerable))
        return spret::abort;

    fail_check();

    bool chaos = determine_chaos(&you, SPELL_HAILSTORM);

    if (chaos)
        mpr("A hail of chaotic shards descend around you!");
    else
        mpr("A cannonade of hail descends around you!");

    for (radius_iterator ri(you.pos(), 3, C_SQUARE, LOS_NO_TRANS, true); ri; ++ri)
    {
        if (grid_distance(you.pos(), *ri) == 1 || !in_bounds(*ri))
            continue;

        _hailstorm_cell(*ri, pow, &you, chaos);
    }

    return spret::success;
}

static void _imb_actor(actor * act, int pow, bool chaos)
{
    bolt beam;
    beam.source          = you.pos();
    beam.thrower         = KILL_YOU;
    beam.source_id       = MID_PLAYER;
    beam.range           = LOS_RADIUS;
    if (chaos)
        beam.colour      = ETC_JEWEL;
    else
        beam.colour      = ETC_AIR;
    beam.glyph           = dchar_glyph(DCHAR_FIRED_ZAP);
    beam.name            = "atmospheric blast";
    beam.origin_spell    = SPELL_MUSE_OAMS_AIR_BLAST;
    beam.ench_power      = pow;
    beam.aimed_at_spot   = true;
    beam.loudness        = 10;

    beam.target          = act->pos();

    beam.flavour         = BEAM_VISUAL;
    beam.affects_nothing = true;
    beam.pierce          = true;
    beam.fire();

    if (chaos)
        beam.real_flavour = beam.flavour = BEAM_CHAOTIC;
    else
        beam.flavour     = BEAM_AIR;
    beam.affects_nothing = false;
    beam.hit             = 10 + pow / 7;
    if (chaos)
        beam.damage      = calc_dice(2, 7 + (pow * 5 / 12));
    else
        beam.damage      = calc_dice(2, 6 + pow / 3);

    if (_is_menacing(&you, SPELL_MUSE_OAMS_AIR_BLAST))
        beam.damage.num++;

    beam.affect_actor(act);
}

struct dist_sorter
{
    coord_def pos;
    bool operator()(const actor* a, const actor* b)
    {
        return a->pos().distance_from(pos) > b->pos().distance_from(pos);
    }
};

spret cast_imb(int pow, bool fail)
{
    int range = spell_range(SPELL_MUSE_OAMS_AIR_BLAST, pow);
    targeter_radius hitfunc(&you, LOS_SOLID_SEE, range);
    bool (*vulnerable) (const actor *) = [](const actor * act) -> bool
    {
        return !(act->is_monster() && mons_is_conjured(act->as_monster()->type)) 
                                   && !cell_is_solid((act->pos()));
    }; 

    if (stop_attack_prompt(hitfunc, "blast", vulnerable))
        return spret::abort;

    fail_check();

    bool chaos = determine_chaos(&you, SPELL_MUSE_OAMS_AIR_BLAST);

    mprf("A blast of %sair wooshes around you!", chaos ? "chaotic ": "");

    vector<actor *> act_list;

    for (actor_near_iterator ai(you.pos(), LOS_SOLID_SEE); ai; ++ai)
    {
        if (ai->pos().distance_from(you.pos()) > range
            || cell_is_solid(ai->pos())
            || ai->pos() == you.pos() // so it's never aimed_at_feet
            || mons_is_conjured(ai->as_monster()->type)) // skip prisms &c.
        {
            continue;
        }

        act_list.push_back(*ai);
    }

    dist_sorter sorter = {you.pos()};
    sort(act_list.begin(), act_list.end(), sorter);

    for (actor *act : act_list)
        _imb_actor(act, pow, chaos);

    return spret::success;
}

void actor_apply_toxic_bog(actor * act)
{
    if (grd(act->pos()) != DNGN_TOXIC_BOG && grd(act->pos()) != DNGN_QUAGMIRE)
        return;

    if (!act->ground_level())
        return;

    const bool chaos = grd(act->pos()) == DNGN_QUAGMIRE;
    const bool player = act->is_player();
    monster *mons = !player ? act->as_monster() : nullptr;

    actor *oppressor = nullptr;

    for (map_marker *marker : env.markers.get_markers_at(act->pos()))
    {
        if (marker->get_type() == MAT_TERRAIN_CHANGE)
        {
            map_terrain_change_marker* tmarker =
                    dynamic_cast<map_terrain_change_marker*>(marker);
            if (tmarker->change_type == TERRAIN_CHANGE_BOG)
                oppressor = actor_by_mid(tmarker->mon_num);
        }
    }

    beam_type dam_type = BEAM_POISON_ARROW;
    if (chaos)
    {
        switch (random2(2))
        {
        default: //just in case.
        case 0: dam_type = chaos_damage_type();
        case 1: dam_type = BEAM_ACID;
        case 2: dam_type = BEAM_LAVA;
        }
    }

    const int base_damage = dice_def(4, 6).roll();
    const int damage = resist_adjust_damage(act, dam_type, base_damage);
    const int resist = base_damage - damage;

    const int final_damage = timescale_damage(act, damage);

    if (chaos)
    {
        if (player && final_damage > 0)
        {
            mprf("You marinate in the quagmire%s",
                attack_strength_punctuation(final_damage).c_str());
        }
        else if (final_damage > 0)
        {
            behaviour_event(mons, ME_DISTURB, 0, act->pos());
            mprf("%s marinates in the quagmire%s",
                mons->name(DESC_THE).c_str(),
                attack_strength_punctuation(final_damage).c_str());
        }
    }

    else
    {
        if (player && final_damage > 0)
        {
            mprf("You fester in the toxic bog%s",
                attack_strength_punctuation(final_damage).c_str());
        }
        else if (final_damage > 0)
        {
            behaviour_event(mons, ME_DISTURB, 0, act->pos());
            mprf("%s festers in the toxic bog%s",
                mons->name(DESC_THE).c_str(),
                attack_strength_punctuation(final_damage).c_str());
        }
    }

    if (!chaos)
    {
        if (final_damage > 0 && resist > 0)
        {
            if (player)
                canned_msg(MSG_YOU_PARTIALLY_RESIST);

            act->poison(oppressor, 7, true);
        }
        else if (final_damage > 0)
            act->poison(oppressor, 21, true);
    }
    else if (final_damage)
    {
        if (one_chance_in(3))
            chaotic_status(act, final_damage, oppressor);
        chaotic_debuff(act, final_damage, oppressor);
    }

    if (final_damage)
    {

        const string oppr_name =
            oppressor ? " "+apostrophise(oppressor->name(DESC_THE))
                      : "";
        dprf("%s %s %d damage from%s toxic bog.",
             act->name(DESC_THE).c_str(),
             act->conj_verb("take").c_str(),
             final_damage,
             oppr_name.c_str());

        act->hurt(oppressor, final_damage, BEAM_MISSILE,
                  KILLED_BY_POISON, "", "toxic bog");
    }
}
