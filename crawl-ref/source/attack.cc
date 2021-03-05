/*
 *  File:       attack.cc
 *  Summary:    Methods of the attack class, generalized functions which may
 *              be overloaded by inheriting classes.
 *  Written by: Robert Burnham
 */

#include "AppHdr.h"

#include "attack.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>

#include "art-enum.h"
#include "beam.h"
#include "chardump.h"
#include "delay.h"
#include "english.h"
#include "env.h"
#include "exercise.h"
#include "fight.h"
#include "fineff.h"
#include "food.h"
#include "god-conduct.h"
#include "god-passive.h" // passive_t::no_haste
#include "item-name.h"
#include "item-prop.h"
#include "message.h"
#include "mon-behv.h"
#include "mon-clone.h"
#include "mon-death.h"
#include "mon-util.h"
#include "nearby-danger.h"
#include "pronoun-type.h"
#include "religion.h"
#include "spl-miscast.h"
#include "spl-util.h"
#include "state.h"
#include "stepdown.h"
#include "stringutil.h"
#include "transform.h"
#include "traps.h" // ensnare
#include "xom.h"

/*
 **************************************************
 *             BEGIN PUBLIC FUNCTIONS             *
 **************************************************
*/
attack::attack(actor *attk, actor *defn, actor *blame)
    : attacker(attk), defender(defn), responsible(blame ? blame : attk),
      attack_occurred(false), cancel_attack(false), did_hit(false),
      needs_message(false), attacker_visible(false), defender_visible(false),
      perceived_attack(false), obvious_effect(false), to_hit(0),
      damage_done(0), special_damage(0), aux_damage(0), min_delay(0),
      final_attack_delay(0), special_damage_flavour(BEAM_NONE),
      stab_attempt(false), stab_bonus(0), ev_margin(0), weapon(nullptr),
      damage_brand(SPWPN_NORMAL), wpn_skill(SK_UNARMED_COMBAT),
      shield(nullptr), art_props(0), unrand_entry(nullptr),
      attacker_to_hit_penalty(0), attack_verb("bug"), verb_degree(),
      no_damage_message(), special_damage_message(), aux_attack(), aux_verb(),
      attacker_armour_tohit_penalty(0), attacker_shield_tohit_penalty(0),
      defender_shield(nullptr), miscast_level(-1), miscast_type(spschool::none),
      miscast_target(nullptr), fake_chaos_attack(false), simu(false),
      aux_source(""), kill_type(KILLED_BY_MONSTER)
{
    // No effective code should execute, we'll call init_attack again from
    // the child class, since initializing an attack will vary based the within
    // type of attack actually being made (melee, ranged, etc.)
}

bool attack::handle_phase_attempted()
{
    return true;
}

static void _handle_staff_shield(beam_type flavour, int str, bool player, monster * mon)
{
    if (player)
        lose_staff_shield(flavour, str);
    else
        mon_lose_staff_shield(*mon, flavour, str);
}

bool attack::handle_phase_blocked()
{
    damage_done = 0;

    if (weapon)
    {
        if (weapon->base_type == OBJ_STAVES)
        {
            switch (weapon->sub_type)
            {
            case STAFF_FIRE:
                _handle_staff_shield(BEAM_FIRE, 1, defender->is_player(), defender->as_monster());
                break;
            case STAFF_COLD:
                _handle_staff_shield(BEAM_COLD, 1, defender->is_player(), defender->as_monster());
                break;
            case STAFF_AIR:
                _handle_staff_shield(BEAM_ELECTRICITY, 1, defender->is_player(), defender->as_monster());
                break;
            case STAFF_POISON:
                _handle_staff_shield(BEAM_POISON, 1, defender->is_player(), defender->as_monster());
                break;
            default:
                break;
            }
        }
        else
        {
            switch (get_weapon_brand(*weapon))
            {
            case SPWPN_ACID:
                _handle_staff_shield(BEAM_ACID, 1, defender->is_player(), defender->as_monster());
                break;
            case SPWPN_CHAOS:
                switch (random2(3))
                {
                case 0: _handle_staff_shield(BEAM_FIRE, 1, defender->is_player(), defender->as_monster()); break;
                case 1: _handle_staff_shield(BEAM_COLD, 1, defender->is_player(), defender->as_monster()); break;
                case 2: _handle_staff_shield(BEAM_ELECTRICITY, 1, defender->is_player(), defender->as_monster()); break;
                case 3: _handle_staff_shield(BEAM_POISON, 1, defender->is_player(), defender->as_monster()); break;
                }
                break;
            case SPWPN_ELECTROCUTION:
                _handle_staff_shield(BEAM_ELECTRICITY, 1, defender->is_player(), defender->as_monster());
                break;
            case SPWPN_FREEZING:
                _handle_staff_shield(BEAM_COLD, 1, defender->is_player(), defender->as_monster());
                break;
            case SPWPN_MOLTEN:
                _handle_staff_shield(BEAM_FIRE, 1, defender->is_player(), defender->as_monster());
                break;
            default:
                break;
            }
        }
    }

    return true;
}

bool attack::handle_phase_damaged()
{
    // We have to check in_bounds() because removed kraken tentacles are
    // temporarily returned to existence (without a position) when they
    // react to damage.
    if (!mount_defend
        && defender->can_bleed()
        && !defender->is_summoned()
        && in_bounds(defender->pos())
        && !simu)
    {
        int blood = damage_done;
        if (blood > defender->stat_hp())
            blood = defender->stat_hp();
        if (blood)
            blood_fineff::schedule(defender, defender->pos(), blood);
    }

    announce_hit();
    // Inflict stored damage
    damage_done = inflict_damage(damage_done);

    // TODO: Unify these, added here so we can get rid of player_attack
    if (attacker->is_player())
    {
        if (damage_done)
            player_exercise_combat_skills();
    }
    else
    {
        if (!mons_attack_effects())
            return false;
    }

    // It's okay if a monster took lethal damage, but we should stop
    // the combat if it was already reset (e.g. a spectral weapon that
    // took damage and then noticed that its caster is gone).
    return defender->is_player() || !invalid_monster(defender->as_monster());
}

bool attack::handle_phase_killed()
{
    monster* mon = defender->as_monster();
    if (!invalid_monster(mon))
        monster_die(*mon, attacker);

    return true;
}

bool attack::handle_phase_end()
{
    // This may invalidate both the attacker and defender.
    fire_final_effects();

    return true;
}

/**
 * Calculate the to-hit for an attacker
 *
 * @param random If false, calculate average to-hit deterministically.
 * @param player_aux If true, ignore the weapon the player is wielding.
 *     and calc without any weapon skill (auxillary attack calc).
 */
int attack::calc_to_hit(bool random, bool player_aux)
{
    float mhit = attacker->is_player() ?
                4 + you.dex() * 0.85
              : calc_mon_to_hit_base();

#ifdef DEBUG_DIAGNOSTICS
    const int base_hit = mhit;
#endif

    // This if statement is temporary, it should be removed when the
    // implementation of a more universal (and elegant) to-hit calculation
    // is designed. The actual code is copied from the old mons_to_hit and
    // player_to_hit methods.
    if (attacker->is_player())
    {
        // fighting contribution
        mhit *= (2700 + you.skill(SK_FIGHTING, 100));
        mhit /= 2700;

        // weapon skill contribution
        if (player_aux) {}
        else if (using_weapon())
        {
            if (wpn_skill != SK_FIGHTING)
            {
                if (you.skill(wpn_skill) < 1 && player_in_a_dangerous_place())
                    xom_is_stimulated(10); // Xom thinks that is mildly amusing.

                mhit *= (2700 + you.skill(wpn_skill, 100));
                mhit /= 2700;
            }
        }
        else if (you.form_uses_xl())
        {
            mhit *= (27 + you.experience_level);
            mhit /= 27;
        }
        else
        {
            // Claws give a slight bonus to accuracy when active
            mhit *= (you.get_mutation_level(MUT_CLAWS) > 0
                     && wpn_skill == SK_UNARMED_COMBAT) ? 1.1 : 1;

            mhit *= (2700 + you.skill(wpn_skill, 100));
            mhit /= 2700;
        }

        // slaying bonus
        mhit *= (50 + slaying_bonus(weapon && is_range_weapon(*weapon)
                                           && using_weapon(),
                                              using_weapon()));
        mhit /= 50;

        // hunger penalty
        if (apply_starvation_penalties())
            mhit *= 0.85;

        // armour penalty
        mhit *= max((20 - attacker_armour_tohit_penalty - attacker_shield_tohit_penalty), 5);
        mhit /= 20;

        // vertigo penalty
        if (you.duration[DUR_VERTIGO])
            mhit *= 0.85;

        // mutation
        if (you.get_mutation_level(MUT_EYEBALLS))
        {
            mhit *= (20 + you.get_mutation_level(MUT_EYEBALLS));
            mhit /= 20;
        }

        // +0 for normal vision, +5 for Supernaturally Acute Vision, -5 For Impaired Vision
        mhit *= 10 + you.vision();
        mhit /= 10;
    }
    else    // Monster to-hit.
    {
        mhit *= 10 - attacker->inaccuracy();
        mhit /= 10;

        const int jewellery = attacker->as_monster()->inv[MSLOT_JEWELLERY];
        if (jewellery != NON_ITEM
            && mitm[jewellery].is_type(OBJ_JEWELLERY, RING_SLAYING))
        {
            mhit *= 1.25;
        }

        if (attacker->scan_artefacts(ARTP_SLAYING))
            mhit *= 1.25;
    }

    // weapon bonus contribution
    if (using_weapon())
    {
        if (weapon->base_type == OBJ_WEAPONS || weapon->base_type == OBJ_SHIELDS)
        {
            mhit *= (20 + weapon->plus);
            mhit *= (20 + property(*weapon, PWPN_HIT));
            mhit /= 400;
        }
        else if (weapon->base_type == OBJ_STAVES)
        {
            mhit *= (20 + property(*weapon, PWPN_HIT));
            mhit /= 20;
        }
    }

    // Penalties and Buffs for both players and monsters:

    if (attacker->confused())
        mhit *= 0.75;

    if (using_weapon()
        && (is_unrandom_artefact(*weapon, UNRAND_WOE)
            || is_unrandom_artefact(*weapon, UNRAND_SNIPER)))
    {
        return AUTOMATIC_HIT;
    }

    // If no defender, we're calculating to-hit for debug-display
    // purposes, so don't drop down to defender code below
    if (defender == nullptr)
        return mhit;

    if (!defender->visible_to(attacker))
            mhit *= 0.75;
    else
    {
        // This can only help if you're visible!
        const int how_transparent = you.get_mutation_level(MUT_TRANSLUCENT_SKIN);
        if (defender->is_player() && how_transparent)
        {
            mhit *= (20 - 1 * how_transparent); 
            mhit /= 20;
        }

        if (defender->backlit(false))
        {
            mhit *= (20 + 1 + random2(9));
            mhit /= 20;
        }
        else if (!attacker->nightvision()
            && defender->umbra())
        {
            mhit *= (20 - 1 - random2(4));
            mhit /= 20;
        }
    }

    // Don't delay doing this roll until test_hit().
    mhit = maybe_random2(mhit, random) + maybe_random2(mhit, random);

    dprf(DIAG_COMBAT, "%s: Base to-hit: %d, Final to-hit: %d",
         attacker->name(DESC_PLAIN).c_str(),
         base_hit, mhit);

    return rand_round(mhit);
}

/* Returns an actor's name
 *
 * Takes into account actor visibility/invisibility and the type of description
 * to be used (capitalization, possessiveness, etc.)
 */
string attack::actor_name(const actor *a, description_level_type desc,
                          bool actor_visible)
{
    return actor_visible ? a->name(desc) : anon_name(desc);
}

/* Returns an actor's pronoun
 *
 * Takes into account actor visibility
 */
string attack::actor_pronoun(const actor *a, pronoun_type pron,
                             bool actor_visible)
{
    return actor_visible ? a->pronoun(pron) : anon_pronoun(pron);
}

/* Returns an anonymous actor's name
 *
 * Given the actor visible or invisible, returns the
 * appropriate possessive pronoun.
 */
string attack::anon_name(description_level_type desc)
{
    switch (desc)
    {
    case DESC_NONE:
        return "";
    case DESC_YOUR:
    case DESC_ITS:
        return "something's";
    case DESC_THE:
    case DESC_A:
    case DESC_PLAIN:
    default:
        return "something";
    }
}

/* Returns an anonymous actor's pronoun
 *
 * Given invisibility (whether out of LOS or just invisible), returns the
 * appropriate possessive, inflexive, capitalised pronoun.
 */
string attack::anon_pronoun(pronoun_type pron)
{
    return decline_pronoun(GENDER_NEUTER, pron);
}

/* Initializes an attack, setting up base variables and values
 *
 * Does not make any changes to any actors, items, or the environment,
 * in case the attack is cancelled or otherwise fails. Only initializations
 * that are universal to all types of attacks should go into this method,
 * any initialization properties that are specific to one attack or another
 * should go into their respective init_attack.
 *
 * Although this method will get overloaded by the derived class, we are
 * calling it from attack::attack(), before the overloading has taken place.
 */
void attack::init_attack(skill_type unarmed_skill, int attk_num)
{
    ASSERT(attacker);
    attack_number   = attk_num;
    weapon          = attacker->weapon(attack_number);

    mount_attack    = (you.mounted() && attacker->is_player() && attack_number >= 2);
    mount_defend    = defender->is_player() && mount_hit();

    wpn_skill       = weapon ? item_attack_skill(*weapon) : unarmed_skill;
    if (attacker->is_player() && you.form_uses_xl())
        wpn_skill = SK_FIGHTING; // for stabbing, mostly

    attacker_armour_tohit_penalty =
        div_rand_round(attacker->armour_tohit_penalty(true, 20), 20);
    attacker_shield_tohit_penalty =
        div_rand_round(attacker->shield_tohit_penalty(true, 20), 20);
    to_hit          = calc_to_hit(true);

    shield = attacker->shield();
    defender_shield = defender ? defender->shield() : defender_shield;

    if (weapon && weapon->base_type == OBJ_WEAPONS && is_artefact(*weapon))
    {
        artefact_properties(*weapon, art_props);
        if (is_unrandom_artefact(*weapon))
            unrand_entry = get_unrand_entry(weapon->unrand_idx);
    }

    attacker_visible   = attacker->observable();
    defender_visible   = defender && defender->observable();
    needs_message      = (attacker_visible || defender_visible);

    if (attacker->is_monster())
    {
        mon_attack_def mon_attk = mons_attack_spec(*attacker->as_monster(),
                                                   attack_number,
                                                   false);

        attk_type       = mon_attk.type;
        attk_flavour    = mon_attk.flavour;

        // Don't scale damage for YOU_FAULTLESS etc.
        if (attacker->get_experience_level() == 0)
            attk_damage = mon_attk.damage;
        else
        {
            attk_damage = div_rand_round(mon_attk.damage
                                             * attacker->get_hit_dice(),
                                         attacker->get_experience_level());
        }

        if (attk_type == AT_WEAP_ONLY)
        {
            int weap = attacker->as_monster()->inv[MSLOT_WEAPON];
            if (weap == NON_ITEM || is_range_weapon(mitm[weap]))
                attk_type = AT_NONE;
            else
                attk_type = AT_HIT;
        }
        else if (attk_type == AT_TRUNK_SLAP && attacker->type == MONS_SKELETON)
        {
            // Elephant trunks have no bones inside.
            attk_type = AT_NONE;
        }
    }
    else
    {
        attk_type    = AT_HIT;
        attk_flavour = AF_PLAIN;
    }
}

void attack::alert_defender()
{
    // Allow monster attacks to draw the ire of the defender. Player
    // attacks are handled elsewhere.
    if (perceived_attack
        && defender->is_monster()
        && attacker->is_monster()
        && attacker->alive() && defender->alive()
        && (defender->as_monster()->foe == MHITNOT || one_chance_in(3)))
    {
        behaviour_event(defender->as_monster(), ME_WHACK, attacker);
    }

    // If an enemy attacked a friend, set the pet target if it isn't set
    // already, but not if sanctuary is in effect (pet target must be
    // set explicitly by the player during sanctuary).
    if (perceived_attack && attacker->alive()
        && (defender->is_player() || defender->as_monster()->friendly())
        && !attacker->is_player()
        && !crawl_state.game_is_arena()
        && !attacker->as_monster()->wont_attack())
    {
        if (defender->is_player())
        {
            interrupt_activity(activity_interrupt::monster_attacks,
                               attacker->as_monster());
        }
        if (you.pet_target == MHITNOT && env.sanctuary_time <= 0)
            you.pet_target = attacker->mindex();
    }
}

bool attack::distortion_affects_defender()
{
    enum disto_effect
    {
        SMALL_DMG,
        BIG_DMG,
        BANISH,
        BLINK,
        TELE_INSTANT,
        TELE_DELAYED,
        NONE
    };

    const disto_effect choice = random_choose_weighted(33, SMALL_DMG,
                                                       22, BIG_DMG,
                                                       5,  BANISH,
                                                       15, BLINK,
                                                       10, TELE_INSTANT,
                                                       10, TELE_DELAYED,
                                                       5,  NONE);

    if (simu && !(choice == SMALL_DMG || choice == BIG_DMG))
        return false;

    switch (choice)
    {
    case SMALL_DMG:
        if (defender->is_fairy())
            return false;
        special_damage += 1 + random2avg(7, 2);
        special_damage_message = make_stringf("Space bends around %s%s",
                                              defender_name(false).c_str(),
                                              attack_strength_punctuation(special_damage).c_str());
        break;
    case BIG_DMG:
        special_damage += 3 + random2avg(24, 2);
        special_damage_message =
            make_stringf("Space warps horribly around %s%s",
                         defender_name(false).c_str(),
                         attack_strength_punctuation(special_damage).c_str());
        break;
    case BLINK:
        if (defender_visible)
            obvious_effect = true;
        if (!defender->no_tele(true, false))
            blink_fineff::schedule(defender);
        break;
    case BANISH:
        if (mount_defend)
        {
            mprf(MSGCH_WARN, "Your %s was banished out from underneath you!", you.mount_name().c_str());
            dismount();
        }
        else
        {
            if (defender_visible)
                obvious_effect = true;
            defender->banish(attacker, attacker->name(DESC_PLAIN, true),
                attacker->get_experience_level());
        }
        return true;
    case TELE_INSTANT:
    case TELE_DELAYED:
        if (defender_visible)
            obvious_effect = true;
        if (crawl_state.game_is_sprint() && defender->is_player()
            || defender->no_tele())
        {
            if (defender->is_player())
                canned_msg(MSG_STRANGE_STASIS);
            return false;
        }

        if (choice == TELE_INSTANT)
            teleport_fineff::schedule(defender);
        else
            defender->teleport();
        break;
    case NONE:
        // Do nothing
        break;
    }

    return false;
}

void attack::antimagic_affects_defender(int pow)
{
    // Caster mount not only doesn't exist but is probably a completely nonsense thought...
    // Yes, I'm riding an Orc Wizard. Hah. Anyways not coding antimagic affects mount for now.
    // Maybe a cat could ride on a ogre mage hahahaha.
    if (!mount_defend) 
    {
        obvious_effect =
            enchant_actor_with_flavour(defender, nullptr, BEAM_DRAIN_MAGIC, pow);
    }
}

/// Whose skill should be used for a pain-weapon effect?
static actor* _pain_weapon_user(actor* attacker)
{
    if (attacker->type != MONS_SPECTRAL_WEAPON)
        return attacker;

    const mid_t summoner_mid = attacker->as_monster()->summoner;
    if (summoner_mid == MID_NOBODY)
        return attacker;

    actor* summoner = actor_by_mid(attacker->as_monster()->summoner);
    if (!summoner || !summoner->alive())
        return attacker;
    return summoner;
}

void attack::pain_affects_defender()
{
    actor* user = _pain_weapon_user(attacker);
    if (!one_chance_in(user->skill_rdiv(SK_NECROMANCY) + 1))
    {
        special_damage += resist_adjust_damage(defender, BEAM_NEG,
                              random2(1 + user->skill_rdiv(SK_NECROMANCY)), mount_defend);

        if (special_damage && defender->is_fairy() && x_chance_in_y(30 - special_damage, 30))
            special_damage = 0;

        if (special_damage && defender_visible)
        {
            special_damage_message =
                make_stringf("%s%s %s in agony%s",
                             mount_defend ? "Your " : "",
                             mount_defend ? you.mount_name(true).c_str()
                                          : defender->name(DESC_THE).c_str(),
                             mount_defend ? "writhes"
                                          : defender->conj_verb("writhe").c_str(),
                           attack_strength_punctuation(special_damage).c_str());
        }
    }
}

static bool _is_chaos_polyable(const actor &defender, const bool md)
{
    if (md)
        return false; // no polymorphing mounts (they don't have enough forms).

    if (!defender.can_safely_mutate())
        return false;  // no polymorphing undead

    const monster* mon = defender.as_monster();
    if (!mon)
        return true;

    return !mons_is_firewood(*mon) && !mons_immune_magic(*mon);
}

static bool _is_chaos_slowable(const actor &defender, const bool md)
{
    if (md)
        return true; 

    const monster* mon = defender.as_monster();
    if (!mon)
        return true;

    return !mons_is_firewood(*mon) && !mon->is_stationary()
        && !mon->stasis();
}

struct chaos_effect
{
    string name;
    int chance;
    function<bool(const actor& def, const bool md)> valid;
    beam_type flavour;
    function<bool(attack &attack, const bool md)> misc_effect;
};

// Total Weight: 69 (Arbitrary)
static const vector<chaos_effect> chaos_effects = {
    {
        "clone", 1, [](const actor &d, const bool md) {
            return md || (d.is_monster() && mons_clonable(d.as_monster(), true));
        },
        BEAM_NONE, [](attack &attack, const bool /*md*/) {
            actor &defender = *attack.defender;
            bool obvious_effect;
            monster *clone = clone_mons(&defender, true, &obvious_effect);
            if (!clone)
                return false;

            if (one_chance_in(3))
                clone->attitude = coinflip() ? ATT_FRIENDLY : ATT_NEUTRAL;

            // The player shouldn't get new permanent followers from cloning.
            if (clone->attitude == ATT_FRIENDLY && !clone->is_summoned())
                clone->mark_summoned(6, true, MON_SUMM_CLONE);

            // Monsters being cloned is interesting.
            xom_is_stimulated(clone->friendly() ? 12 : 25);
            return obvious_effect;
        },
    },
    {
        "polymorph", 2, _is_chaos_polyable, BEAM_POLYMORPH,
    },
    {
        "shifter", 1, [](const actor &defender, const bool md)
        {
            const monster *mon = defender.as_monster();
            return _is_chaos_polyable(defender, md)
                   && mon && !mon->is_shapeshifter()
                   && defender.holiness() & MH_NATURAL;
        },
        BEAM_NONE, [](attack &attack, const bool /*md*/) {
            monster* mon = attack.defender->as_monster();
            ASSERT(_is_chaos_polyable(*attack.defender, false));
            ASSERT(mon);
            ASSERT(!mon->is_shapeshifter());

            const bool obvious_effect = you.can_see(*attack.defender);
            mon->add_ench(one_chance_in(3) ? ENCH_GLOWING_SHAPESHIFTER
                                           : ENCH_SHAPESHIFTER);
            // Immediately polymorph monster, just to make the effect obvious.
            mon->polymorph();

            // Xom loves it if this happens!
            const int friend_factor = mon->friendly() ? 1 : 2;
            const int glow_factor   = mon->has_ench(ENCH_SHAPESHIFTER) ? 1 : 2;
            xom_is_stimulated(64 * friend_factor * glow_factor);

            return obvious_effect;
        },
    },
    {
        "miscast", 20, nullptr, BEAM_NONE, [](attack &attack, const bool /*md*/) {

            // Mount Defend unused; just let miscasts go to the player.

            const int HD = attack.defender->get_hit_dice();

            // At level == 27 there's a 13.9% chance of a level 3 miscast.
            const int level0_chance = HD;
            const int level1_chance = max(0, HD - 7);
            const int level2_chance = max(0, HD - 12);
            const int level3_chance = max(0, HD - 17);

            attack.miscast_level  = random_choose_weighted(level0_chance, 0,
                                                           level1_chance, 1,
                                                           level2_chance, 2,
                                                           level3_chance, 3);
            attack.miscast_type   = spschool::random;
            attack.miscast_target = attack.defender;

            return false;
        },
    },
    {
        "rage", 5, [](const actor &defender, const bool md) {
            return !md && defender.can_go_berserk();
        }, BEAM_NONE, [](attack &attack, const bool /*md*/) {
            attack.defender->go_berserk(false);
            return you.can_see(*attack.defender);
        },
    },
    { "hasting", 10, [](const actor &defender, const bool md) {
            return !md && _is_chaos_slowable(defender, md);
        }, BEAM_HASTE },
    { "invisible", 10, nullptr, BEAM_INVISIBILITY, },
    { "mighting", 10, [](const actor &/*defender*/, const bool md) { return !md; }, BEAM_MIGHT, },
    { "agility", 10, [](const actor &/*defender*/, const bool md) { return !md; }, BEAM_AGILITY, },
    { "entropic burst", 30, [](const actor &defender, const bool /*md*/) {
            return defender.is_monster();
        }, BEAM_ENTROPIC_BURST, },
    { "chaotic infusion", 30, [](const actor &defender, const bool /*md*/) {
            return defender.is_monster();
        }, BEAM_CHAOTIC_INFUSION, },
    { "slowing", 10, _is_chaos_slowable, BEAM_SLOW },
    {
        "petrify", 10, [](const actor &defender, const bool md) {
            return _is_chaos_slowable(defender, md) && !defender.res_petrify(true, md);
        }, BEAM_PETRIFY,
    },
};

void attack::chaos_affects_defender()
{
    ASSERT(defender);

    vector<pair<const chaos_effect&, int>> weights;
    for (const chaos_effect &effect : chaos_effects)
        if (!effect.valid || effect.valid(*defender, mount_defend))
            weights.push_back({effect, effect.chance});

    const chaos_effect &effect = *random_choose_weighted(weights);

#ifdef NOTE_DEBUG_CHAOS_EFFECTS
    take_note(Note(NOTE_MESSAGE, 0, 0, "CHAOS effect: " + effect.name), true);
#endif

    if (effect.misc_effect && effect.misc_effect(*this, mount_defend))
        obvious_effect = true;

    bolt beam;
    beam.flavour = effect.flavour;
    if (beam.flavour != BEAM_NONE)
    {
        if (defender->is_player() && have_passive(passive_t::no_haste)
            && beam.flavour == BEAM_HASTE)
        {
            simple_god_message(" protects you from inadvertent hurry.");
            obvious_effect = true;
            return;
        }

        beam.glyph        = 0;
        beam.range        = 0;
        beam.colour       = BLACK;
        beam.effect_known = false;
        // Wielded brand is always known, but maybe this was from a call
        // to chaos_affect_actor.
        beam.effect_wanton = !fake_chaos_attack;

        if (using_weapon() && you.can_see(*attacker))
        {
            beam.name = wep_name(DESC_YOUR);
            beam.item = weapon;
        }
        else
            beam.name = atk_name(DESC_THE);

        beam.thrower =
            (attacker->is_player())           ? KILL_YOU
            : attacker->as_monster()->confused_by_you() ? KILL_YOU_CONF
                                                        : KILL_MON;

        if (beam.thrower == KILL_YOU || attacker->as_monster()->friendly())
            beam.attitude = ATT_FRIENDLY;

        beam.source_id = attacker->mid;

        beam.source = defender->pos();
        beam.target = defender->pos();

        beam.damage = dice_def(damage_done + special_damage + aux_damage, 1);

        beam.ench_power = beam.damage.num;

        const bool you_could_see = you.can_see(*defender);
        beam.fire();

        if (you_could_see)
            obvious_effect = beam.obvious_effect;
    }

    if (!you.can_see(*attacker))
        obvious_effect = false; // XXX: VERY dubious!
}

struct chaos_attack_type
{
    attack_flavour flavour;
    brand_type brand;
    int chance;
    function<bool(const actor& def)> valid;
};

// Chaos melee attacks randomly choose a brand from here, with brands that
// definitely won't affect the target being invalid. Chaos itself should
// always be a valid option, triggering a more unpredictable chaos_effect
// instead of a normal attack brand when selected.
static const vector<chaos_attack_type> chaos_types = {
    { AF_FIRE,      SPWPN_MOLTEN,        10,
      [](const actor &d) { return !d.is_fiery(); } },
    { AF_COLD,      SPWPN_FREEZING,      10,
      [](const actor &d) { return !d.is_icy(); } },
    { AF_ELEC,      SPWPN_ELECTROCUTION, 10,
      nullptr },
    { AF_POISON,    SPWPN_VENOM,         10,
      [](const actor &d) {
          return !(d.holiness() & (MH_UNDEAD | MH_NONLIVING)); } },
    { AF_CHAOTIC,   SPWPN_CHAOS,         10,
      nullptr },
    { AF_DRAIN_XP,  SPWPN_DRAINING,      5,
      [](const actor &d) { return bool(d.holiness() & MH_NATURAL); } },
    { AF_VAMPIRIC,  SPWPN_VAMPIRISM,     5,
      [](const actor &d) {
          return !d.is_summoned() && bool(d.holiness() & MH_NATURAL); } },
    { AF_HOLY,      SPWPN_HOLY_WRATH,    5,
      [](const actor &d) { return d.holy_wrath_susceptible(); } },
    { AF_ANTIMAGIC, SPWPN_ANTIMAGIC,     5,
      [](const actor &d) { return d.antimagic_susceptible(); } },
    { AF_CONFUSE,   SPWPN_CONFUSE,       2,
      [](const actor &d) {
          return !(d.holiness() & (MH_NONLIVING | MH_PLANT)); } },
    { AF_DISTORT,   SPWPN_DISTORTION,    2,
      nullptr },
};

brand_type attack::random_chaos_brand()
{
    vector<pair<brand_type, int>> weights;
    for (const chaos_attack_type &choice : chaos_types)
        if (!choice.valid || choice.valid(*defender))
            weights.push_back({choice.brand, choice.chance});

    ASSERT(!weights.empty());

    brand_type brand = *random_choose_weighted(weights);

#ifdef NOTE_DEBUG_CHAOS_BRAND
    string brand_name = "CHAOS brand: ";
    switch (brand)
    {
    case SPWPN_MOLTEN:          brand_name += "molten"; break;
    case SPWPN_FREEZING:        brand_name += "freezing"; break;
    case SPWPN_HOLY_WRATH:      brand_name += "holy wrath"; break;
    case SPWPN_ELECTROCUTION:   brand_name += "electrocution"; break;
    case SPWPN_VENOM:           brand_name += "venom"; break;
    case SPWPN_DRAINING:        brand_name += "draining"; break;
    case SPWPN_DISTORTION:      brand_name += "distortion"; break;
    case SPWPN_VAMPIRISM:       brand_name += "vampirism"; break;
    case SPWPN_ANTIMAGIC:       brand_name += "antimagic"; break;
    case SPWPN_CHAOS:           brand_name += "chaos"; break;
    case SPWPN_CONFUSE:         brand_name += "confusion"; break;
    default:                    brand_name += "BUGGY"; break;
    }

    // Pretty much duplicated by the chaos effect note,
    // which will be much more informative.
    if (brand != SPWPN_CHAOS)
        take_note(Note(NOTE_MESSAGE, 0, 0, brand_name), true);
#endif
    return brand;
}

attack_flavour attack::random_chaos_attack_flavour()
{
    vector<pair<attack_flavour, int>> weights;
    for (const chaos_attack_type &choice : chaos_types)
        if (!choice.valid || choice.valid(*defender))
            weights.push_back({choice.flavour, choice.chance});

    ASSERT(!weights.empty());

    return *random_choose_weighted(weights);
}

void attack::do_miscast()
{
    if (miscast_level == -1)
        return;

    ASSERT(miscast_target != nullptr);
    ASSERT_RANGE(miscast_level, 0, 4);
    ASSERT(count_bits(static_cast<uint64_t>(miscast_type)) == 1);

    if (!miscast_target->alive())
        return;

    if (miscast_target->is_player() && you.banished)
        return;

    const bool chaos_brand =
        using_weapon() && get_weapon_brand(*weapon) == SPWPN_CHAOS;

    // If the miscast is happening on the attacker's side and is due to
    // a chaos weapon then make smoke/sand/etc pour out of the weapon
    // instead of the attacker's hands.
    string hand_str;

    string cause = atk_name(DESC_THE);

    const int ignore_mask = ISFLAG_KNOW_CURSE | ISFLAG_KNOW_PLUSES;

    if (attacker->is_player())
    {
        if (chaos_brand)
        {
            cause = "a chaos effect from ";
            // Ignore a lot of item flags to make cause as short as possible,
            // so it will (hopefully) fit onto a single line in the death
            // cause screen.
            cause += wep_name(DESC_YOUR, ignore_mask | ISFLAG_COSMETIC_MASK);

            if (miscast_target == attacker)
                hand_str = wep_name(DESC_PLAIN, ignore_mask);
        }
    }
    else
    {
        if (chaos_brand && miscast_target == attacker
            && you.can_see(*attacker))
        {
            hand_str = wep_name(DESC_PLAIN, ignore_mask);
        }
    }

    MiscastEffect(miscast_target, attacker, {miscast_source::melee},
                  (spschool) miscast_type, miscast_level, cause,
                  nothing_happens::NEVER, 0, hand_str, false);

    // Don't do miscast twice for one attack.
    miscast_level = -1;
}

void attack::drain_defender()
{
    if ((defender->is_monster() || mount_defend) && coinflip())
        return;

    special_damage = resist_adjust_damage(defender, BEAM_NEG,
                                          (1 + random2(damage_done)) / 2, mount_defend);

    if (mount_defend)
        obvious_effect = drain_mount(10 + min(special_damage, 35));

    else if (defender->drain_exp(attacker, true, 20 + min(35, damage_done)))
    {
        if (defender->is_player())
            obvious_effect = true;
        else if (defender_visible)
        {
            special_damage_message =
                make_stringf(
                    "%s %s %s%s",
                    atk_name(DESC_THE).c_str(),
                    attacker->conj_verb("drain").c_str(),
                    defender_name(true).c_str(),
                    attack_strength_punctuation(special_damage).c_str());
        }
    }
}

void attack::drain_defender_speed()
{
    if (needs_message)
    {
        mprf("%s %s %s vigour!",
             atk_name(DESC_THE).c_str(),
             attacker->conj_verb("drain").c_str(),
             def_name(DESC_ITS).c_str());
    }
    if (mount_defend)
        slow_mount(5 + random2(7));
    else
        defender->slow_down(attacker, 5 + random2(7));
}

int attack::inflict_damage(int dam, beam_type flavour, bool clean)
{
    if (flavour == NUM_BEAMS)
        flavour = special_damage_flavour;
    // Auxes temporarily clear damage_brand so we don't need to check
    if (damage_brand == SPWPN_REAPING
        || damage_brand == SPWPN_CHAOS && one_chance_in(100))
    {
        defender->props["reaping_damage"].get_int() += dam;
        // With two reapers of different friendliness, the most recent one
        // gets the zombie. Too rare a case to care any more.
        defender->props["reaper"].get_int() = attacker->mid;
    }
    if (mount_defend)
        damage_mount(dam);
    else
    {
        return defender->hurt(responsible, dam, flavour, kill_type,
            "", aux_source.c_str(), clean);
    }

    return dam;
}

/* Returns standard attack punctuation
 *
 * Used in player / monster (both primary and aux) attacks
 */
string attack_strength_punctuation(int dmg)
{
    string exclams = "";
    if (dmg < HIT_WEAK)
        exclams = ".";
    else if (dmg < HIT_MED)
        exclams = "!";
    else if (dmg < HIT_STRONG)
        exclams = "!!";
    else
        exclams = string(3 + (int) log2(dmg / HIT_STRONG), '!');
    return make_stringf(" (%d)%s", dmg, exclams.c_str());
}

/* Returns evasion adverb
 *
 */
string attack::evasion_margin_adverb()
{
    return (ev_margin <= -20) ? " completely" :
           (ev_margin <= -12) ? "" :
           (ev_margin <= -6)  ? " closely"
                              : " barely";
}

void attack::stab_message()
{
    defender->props["helpless"] = true;

    if (weapon && weapon->base_type == OBJ_STAVES 
               && weapon->sub_type == STAFF_SUMMONING)
        return;

    switch (stab_bonus)
    {
    case 6:     // big melee, monster surrounded/not paying attention
        if (coinflip())
        {
            mprf("You %s %s from a blind spot!",
                  (you.species == SP_FELID) ? "pounce on" : "strike",
                  defender->name(DESC_THE).c_str());
        }
        else
        {
            mprf("You catch %s momentarily off-guard.",
                  defender->name(DESC_THE).c_str());
        }
        break;
    case 4:     // confused/fleeing
        if (!one_chance_in(3))
        {
            mprf("You catch %s completely off-guard!",
                  defender->name(DESC_THE).c_str());
        }
        else
        {
            mprf("You %s %s from behind!",
                  (you.species == SP_FELID) ? "pounce on" : "strike",
                  defender->name(DESC_THE).c_str());
        }
        break;
    case 2:
    case 1:
        if (you.species == SP_FELID && coinflip())
        {
            mprf("You pounce on the unaware %s!",
                 defender->name(DESC_PLAIN).c_str());
            break;
        }
        mprf("%s fails to defend %s.",
              defender->name(DESC_THE).c_str(),
              defender->pronoun(PRONOUN_REFLEXIVE).c_str());
        break;
    }

    defender->props.erase("helpless");
}

/* Returns the attacker's name
 *
 * Helper method to easily access the attacker's name
 */
string attack::atk_name(description_level_type desc)
{
    if (mount_attack)
        return make_stringf("Your %s", you.mount_name().c_str());

    return actor_name(attacker, desc, attacker_visible);
}

/* Returns the defender's name
 *
 * Helper method to easily access the defender's name
 */
string attack::def_name(description_level_type desc)
{
    if (mount_defend && you.mounted())
        return make_stringf("your %s", you.mount_name().c_str());

    return actor_name(defender, desc, defender_visible);
}

/* Returns the attacking weapon's name
 *
 * Sets upthe appropriate descriptive level and obtains the name of a weapon
 * based on if the attacker is a player or non-player (non-players use a
 * plain name and a manually entered pronoun)
 */
string attack::wep_name(description_level_type desc, iflags_t ignre_flags)
{
    ASSERT(weapon != nullptr);

    if (attacker->is_player())
        return weapon->name(desc, false, false, false, false, ignre_flags);

    string name;
    bool possessive = false;
    if (desc == DESC_YOUR)
    {
        desc       = DESC_THE;
        possessive = true;
    }

    if (possessive)
        name = apostrophise(atk_name(desc)) + " ";

    name += weapon->name(DESC_PLAIN, false, false, false, false, ignre_flags);

    return name;
}

/* TODO: Remove this!
 * Removing it may not really be practical, in retrospect. Its only used
 * below, in calc_elemental_brand_damage, which is called for both frost and
 * flame brands for both players and monsters.
 */
string attack::defender_name(bool allow_reflexive)
{
    if (allow_reflexive && attacker == defender)
        return actor_pronoun(attacker, PRONOUN_REFLEXIVE, attacker_visible);
    else
        return def_name(DESC_THE);
}

int attack::player_stat_modify_damage(int damage)
{
    int dammod = 39;

    if (you.strength() > 10)
        dammod += (random2(you.strength() - 9) * 2);
    else if (you.strength() < 10)
        dammod -= (random2(11 - you.strength()) * 3);

    damage *= dammod;
    damage /= 39;

    return damage;
}

int attack::player_apply_weapon_skill(int damage)
{
    if (using_weapon())
    {
        damage *= 2500 + (random2(you.skill(wpn_skill, 100) + 1));
        damage /= 2500;
    }

    return damage;
}

int attack::player_apply_fighting_skill(int damage, bool aux)
{
    const int base = aux? 40 : 30;

    damage *= base * 100 + (random2(you.skill(SK_FIGHTING, 100) + 1));
    damage /= base * 100;

    return damage;
}

int attack::player_apply_misc_modifiers(int damage)
{
    if (you.submerged() && !you.can_swim())
        damage = div_rand_round(2 * damage, 3);

    return damage;
}

/**
 * Get the damage bonus from a weapon's enchantment.
 */
int attack::get_weapon_plus()
{
    if (weapon->is_type(OBJ_WEAPONS, WPN_TRIPLE_CROSSBOW))
        return div_rand_round(weapon->plus, 3);
    if (weapon->base_type == OBJ_RODS)
        return 0;
    return weapon->plus;
}

// Slaying and weapon enchantment. Apply this for slaying even if not
// using a weapon to attack.
int attack::player_apply_slaying_bonuses(int damage, bool aux)
{
    int damage_plus = 0;
    if (!aux && using_weapon())
        damage_plus = get_weapon_plus();
    if (you.duration[DUR_CORROSION])
    {
        if (weapon && weapon->is_type(OBJ_WEAPONS, WPN_TRIPLE_CROSSBOW))
            return div_rand_round(weapon->plus, 3);
        damage_plus -= 4 * you.props["corrosion_amount"].get_int();
    }
    damage_plus += slaying_bonus(weapon && is_range_weapon(*weapon)
                                            && using_weapon(),
                                               using_weapon());

    damage += (damage_plus > -1) ? (random2(1 + damage_plus))
                                 : (-random2(1 - damage_plus));
    return damage;
}

int attack::player_apply_final_multipliers(int damage)
{
    // Can't affect much of anything as a shadow.
    if (you.form == transformation::shadow)
        damage = div_rand_round(damage, 2);

    return damage;
}

void attack::player_exercise_combat_skills()
{
}

/* Returns attacker base unarmed damage
 *
 * Scales for current mutations and unarmed effects
 * TODO: Complete symmetry for base_unarmed damage
 * between monsters and players.
 */
int attack::calc_base_unarmed_damage()
{
    // Should only get here if we're not wielding something that's a weapon.
    // If there's a non-weapon in hand, it has no base damage.
    if (weapon)
        return 0;

    if (!attacker->is_player())
        return 0;

    if (mount_attack)
    {
        switch (you.mount)
        {
        default:
        case mount_type::slime:
            return 30;
        case mount_type::hydra:
            return 18;
        case mount_type::drake:
            return 12;
        case mount_type::spider:
            return 15;
        }
    }

    // BCADDO: It's a little wack that it's just a base damage additive then skill for most forms
    // Consider revising.
    int damage = get_form()->get_base_unarmed_damage();

    // Claw damage only applies for bare hands.
    if (you.has_usable_claws())
        damage += you.has_claws();

    if (you.form_uses_xl())
        damage += div_rand_round(you.experience_level, 3);
    else if (you.form == transformation::scorpion)
        damage += div_rand_round(you.skill_rdiv(wpn_skill), 3);
    else
        damage += div_rand_round(2 * you.skill_rdiv(wpn_skill), 3);

    if (damage < 0)
        damage = 0;

    return damage;
}

int attack::calc_damage()
{
    int damage = 0;
    int potential_damage = 0;
    if (attacker->is_monster())
    {
        if (using_weapon() || wpn_skill == SK_FIGHTING)
        {
            potential_damage = weapon_damage();
            damage += random2avg(potential_damage + 1, 3); // if we're averaging player damage let's do enemies too.

            int wpn_damage_plus = 0;
            if (weapon) // can be 0 for throwing projectiles
                wpn_damage_plus = get_weapon_plus();

            const int jewellery = attacker->as_monster()->inv[MSLOT_JEWELLERY];
            if (jewellery != NON_ITEM
                && mitm[jewellery].is_type(OBJ_JEWELLERY, RING_SLAYING))
            {
                wpn_damage_plus += mitm[jewellery].plus;
            }

            wpn_damage_plus += attacker->scan_artefacts(ARTP_SLAYING);

            if (wpn_damage_plus >= 0)
                damage += random2(wpn_damage_plus);
            else
                damage -= random2(1 - wpn_damage_plus);

            damage -= 1 + random2(3);
        }

        potential_damage += attk_damage;
        damage     += 1 + random2(attk_damage);

        damage = apply_damage_modifiers(damage);
        return apply_defender_ac(damage, potential_damage);
    }
    else if (mount_attack)
    {
        potential_damage = calc_base_unarmed_damage();

        // weapon skill-like bonus using spellpower/invocations
        switch (you.mount)
        {
        case mount_type::drake:
        case mount_type::slime:
            potential_damage *= 2500 + random2(you.skill(SK_INVOCATIONS, 100) + 1);
            break;
        case mount_type::spider:
            potential_damage *= 2500 + random2(13 * calc_spell_power(SPELL_SUMMON_SPIDER_MOUNT, true) + 1);
            break;
        case mount_type::hydra:
            potential_damage *= 2500 + random2(13 * calc_spell_power(SPELL_SUMMON_HYDRA_MOUNT, true) + 1);
            break;
        default:
            break;
        }
        potential_damage /= 2500;

        if (you.submerged() && !you.can_swim())
            potential_damage = div_rand_round(2 * damage, 3); // Spider can't swim either.

        if (you.duration[DUR_MOUNT_CORROSION])
            potential_damage -= random2(4 * you.props["mount_corrosion_amount"].get_int());

        if (you.duration[DUR_MOUNT_DRAINING])
            potential_damage = div_rand_round(4 * damage, 5);

        if (you.duration[DUR_MOUNT_WRETCHED])
            potential_damage = div_rand_round(4 * damage, 5);

        damage = random2avg(potential_damage + 1, 3);
    }
    else
    {
        potential_damage = using_weapon() ? weapon_damage() : calc_base_unarmed_damage();

        potential_damage = player_stat_modify_damage(potential_damage);
        potential_damage = player_apply_weapon_skill(potential_damage);
        potential_damage = player_apply_fighting_skill(potential_damage, false);
        potential_damage = player_apply_misc_modifiers(potential_damage);
        potential_damage = player_apply_slaying_bonuses(potential_damage, false);

        damage = random2avg(potential_damage + 1, 3); // wow variation die pls

        int stab = player_stab(damage);
        potential_damage += stab;
        damage += stab;

        // A failed stab may have awakened monsters, but that could have
        // caused the defender to cease to exist (spectral weapons with
        // missing summoners; or pacified monsters on a stair). FIXME:
        // The correct thing to do would be either to delay the call to
        // alert_nearby_monsters (currently in player_stab) until later
        // in the attack; or to avoid removing monsters in handle_behaviour.
        if (!defender->alive())
            return 0;

        damage = player_apply_final_multipliers(damage);
    }

    damage = apply_defender_ac(damage, potential_damage);

    damage = max(0, damage);
    set_attack_verb(damage);

    return damage;
}

// Only include universal monster modifiers here; melee and ranged go in their own classes.
int attack::apply_damage_modifiers(int damage)
{
    ASSERT(attacker->is_monster());
    monster *as_mon = attacker->as_monster();

    if (as_mon->submerged() && !as_mon->swimming())
        damage = div_rand_round(2 * damage, 3);

    if (damage_brand == SPWPN_MOLTEN)
        damage = div_rand_round(damage * 3, 5);

    return damage;
}

int attack::test_hit(int to_land, int ev, bool randomise_ev)
{
    int margin = AUTOMATIC_HIT;
    if (randomise_ev)
        ev = random2avg(2*ev, 2);
    if (to_land >= AUTOMATIC_HIT)
        return true;
    
    margin = to_land - ev;

#ifdef DEBUG_DIAGNOSTICS
    dprf(DIAG_COMBAT, "to hit: %d; ev: %d; result: %s (%d)",
         to_hit, ev, (margin >= 0) ? "hit" : "miss", margin);
#endif

    return margin;
}

int attack::apply_defender_ac(int damage, int damage_max) const
{
    ASSERT(defender);
    int stab_bypass = 0;
    ac_type local_ac = ac_type::normal;
    if (stab_bonus)
    {
        stab_bypass = you.skill(wpn_skill, 50) + you.skill(SK_STEALTH, 50);
        stab_bypass = random2(div_rand_round(stab_bypass, 100 * stab_bonus));
    }
    if (damage_brand == SPWPN_MOLTEN)
        local_ac = ac_type::half;
    if (attk_flavour == AF_PIERCE_AC)
        local_ac = ac_type::half;
    if (attacker->is_player() && you.form == transformation::scorpion && damage_brand != SPWPN_NORMAL)
        local_ac = ac_type::half;
    int after_ac = defender->apply_ac(damage, damage_max,
                                      local_ac, stab_bypass, true, mount_defend);
    dprf(DIAG_COMBAT, "AC: att: %s, def: %s, ac: %d, gdr: %d, dam: %d -> %d",
                 attacker->name(DESC_PLAIN, true).c_str(),
                 defender->name(DESC_PLAIN, true).c_str(),
                 defender->armour_class(),
                 defender->gdr_perc(),
                 damage,
                 after_ac);

    return after_ac;
}

/* Determine whether a block occurred
 *
 * No blocks if defender is incapacitated, would be nice to eventually expand
 * this method to handle partial blocks as well as full blocks (although this
 * would serve as a nerf to shields and - while more realistic - may not be a
 * good mechanic for shields.
 *
 * Returns (block_occurred)
 */
bool attack::attack_shield_blocked(bool verbose)
{
    if (defender == attacker)
        return false; // You can't block your own attacks!

    if (defender->incapacitated())
        return false;

    const int con_block = random2(attacker->shield_bypass_ability(to_hit)
                                  + defender->shield_block_penalty());
    int pro_block = defender->shield_bonus();

    if (!attacker->visible_to(defender))
        pro_block /= 3;

    dprf(DIAG_COMBAT, "Defender: %s, Pro-block: %d, Con-block: %d",
         def_name(DESC_PLAIN).c_str(), pro_block, con_block);

    if (pro_block >= con_block)
    {
        perceived_attack = true;

        if (ignores_shield(verbose))
            return false;

        if (needs_message && verbose)
        {
            mprf("%s %s %s attack.",
                 defender_name(false).c_str(),
                 defender->conj_verb("block").c_str(),
                 attacker == defender ? "its own"
                                      : atk_name(DESC_ITS).c_str());
        }

        defender->shield_block_succeeded(attacker);

        return true;
    }

    return false;
}

bool attack::apply_poison_damage_brand()
{
    if (!one_chance_in(4))
    {
        int old_poison;

        if (mount_defend)
            old_poison = you.duration[DUR_MOUNT_POISONING];
        else if (defender->is_player())
            old_poison = you.duration[DUR_POISONING];
        else
        {
            old_poison =
                (defender->as_monster()->get_ench(ENCH_POISON)).degree;
        }

        int splpow_bonus = 0;

        if (mount_attack && you.mount == mount_type::spider)
        {
            splpow_bonus = random2(calc_spell_power(SPELL_SUMMON_SPIDER_MOUNT, true));
            splpow_bonus /= 8;
        }

        if (mount_defend)
            poison_mount(6 + random2(8) + random2(damage_done * 3 / 2));
        else
            defender->poison(attacker, 6 + random2(8) + splpow_bonus + random2(damage_done * 3 / 2));

        if (mount_defend && old_poison < you.duration[DUR_MOUNT_POISONING] ||
            !mount_defend && defender->is_player() && old_poison < you.duration[DUR_POISONING] ||
            !defender->is_player() && old_poison < (defender->as_monster()->get_ench(ENCH_POISON)).degree)
        {
            return true;
        }
    }
    return false;
}

bool attack::apply_damage_brand(const char *what)
{
    bool brand_was_known = false;
    int brand = 0;
    bool ret = false;

    if (using_weapon())
        brand_was_known = item_brand_known(*weapon);

    special_damage = 0;
    obvious_effect = false;
    brand = damage_brand == SPWPN_CHAOS ? random_chaos_brand() : damage_brand;

    if (brand != SPWPN_MOLTEN && brand != SPWPN_FREEZING
        && brand != SPWPN_ELECTROCUTION && brand != SPWPN_VAMPIRISM
        && !defender->alive())
    {
        // Most brands have no extra effects on just killed enemies, and the
        // effect would be often inappropriate.
        return false;
    }

    if (!damage_done
        && (brand == SPWPN_MOLTEN || brand == SPWPN_FREEZING
            || brand == SPWPN_HOLY_WRATH || brand == SPWPN_ANTIMAGIC
            || brand == SPWPN_VORPAL || brand == SPWPN_VAMPIRISM
            || brand == SPWPN_SILVER || brand == SPWPN_DRAGON_SLAYING))
    {
        // These brands require some regular damage to function.
        return false;
    }
    
    const bool fae = !mount_defend && defender->is_fairy();

    switch (brand)
    {
    case SPWPN_PROTECTION:
        if (mount_attack && defender->alive()) // Should only get here on a spider with ensnare active
        {
            mprf("Your spider casts its web at %s.", defender->name(DESC_THE).c_str());
            int splpow = calc_spell_power(SPELL_SUMMON_SPIDER_MOUNT, true);
            splpow /= 10;
            ensnare(defender, splpow);
            you.increase_duration(DUR_MOUNT_BREATH, 1 + random2(30 - splpow));
            you.duration[DUR_ENSNARE] = 0;
        }
        break;

    case SPWPN_MOLTEN:
        if (!fae)
        {
            calc_elemental_brand_damage(BEAM_FIRE,
                defender->is_icy() ? "melt" : "burn",
                what);
        }
        break;

    case SPWPN_FREEZING:
        if (!fae)
            calc_elemental_brand_damage(BEAM_COLD, "freeze", what);
        break;

    case SPWPN_HOLY_WRATH:
        if (defender->holy_wrath_susceptible(mount_defend))
            special_damage = 1 + (random2(damage_done * 15) / 10);

        if (mount_defend)
        {
            special_damage_message =
                make_stringf(
                    "Your %s convulses%s",
                    you.mount_name(true).c_str(),
                    attack_strength_punctuation(special_damage).c_str());
        }
        if (special_damage && defender_visible)
        {
            special_damage_message =
                make_stringf(
                    "%s %s%s",
                    defender_name(false).c_str(),
                    defender->conj_verb("convulse").c_str(),
                    attack_strength_punctuation(special_damage).c_str());
        }
        break;

    case SPWPN_ELECTROCUTION:
    {
        int original = roll_dice(2, 4);
        special_damage = resist_adjust_damage(defender, BEAM_ELECTRICITY,
                                                original);
        if (special_damage)
        {
            if (special_damage == 1 && coinflip())
                special_damage = 0;
            else
            {
                const string punctuation =
                    attack_strength_punctuation(special_damage);
                special_damage_message =
                    defender->is_player() && !mount_defend
                    ? make_stringf("You are %s%s", special_damage < original ? "lightly shocked" :
                                                   special_damage > original ? "electrocuted"
                                                                             : "shocked", punctuation.c_str())
                    : make_stringf("Lightning %scourses through %s%s%s",
                        special_damage < original ? "weakly " :
                        special_damage > original ? "violently "
                        : "",
                        mount_defend ? "your " : "",
                        mount_defend ? you.mount_name(true).c_str() : defender->name(DESC_THE).c_str(),
                        punctuation.c_str());
                special_damage_flavour = BEAM_ELECTRICITY;
                defender->expose_to_element(BEAM_ELECTRICITY, 1);
            }
        }
    }
        break;

    case SPWPN_SILVER:
        special_damage = silver_damages_victim(defender, damage_done, special_damage_message, mount_defend);
        break;

    case SPWPN_DRAGON_SLAYING:
        if (mount_defend && is_draconic_type(mount_mons()))
        {
            special_damage = 1 + (random2(damage_done * 15) / 10);
            special_damage_message =
                make_stringf(
                    "Your %s convulses%s",
                    you.mount_name(true).c_str(),
                    attack_strength_punctuation(special_damage).c_str());
        }

        if (defender->is_dragonkind())
        {
            special_damage = 1 + (random2(damage_done * 15) / 10);
            if (defender_visible)
            {
                special_damage_message =
                    make_stringf(
                        "%s %s%s",
                        defender->name(DESC_THE).c_str(),
                        defender->conj_verb("convulse").c_str(),
                        attack_strength_punctuation(special_damage).c_str());
            }
        }
        break;

    case SPWPN_VENOM:
        obvious_effect = apply_poison_damage_brand();
        break;

    case SPWPN_DRAINING:
        drain_defender();
        break;

    case SPWPN_VORPAL:
        if (!fae)
            special_damage = 1 + random2(damage_done) / 3;
        // Note: Leaving special_damage_message empty because there isn't one.
        break;

    case SPWPN_VAMPIRISM:
    {
        if (damage_done < 1 || mount_defend
            || !actor_is_susceptible_to_vampirism(*defender)
            || attacker->stat_hp() == attacker->stat_maxhp()
            || attacker->is_player() && you.duration[DUR_DEATHS_DOOR])
        {
            break;
        }

        int hp_boost; 

        if (weapon && (is_unrandom_artefact(*weapon, UNRAND_VAMPIRES_TOOTH) ||
                       is_unrandom_artefact(*weapon, UNRAND_LEECH)))
        {
            hp_boost = damage_done;
        }
        else
            hp_boost = max(div_rand_round(roll_dice(3, damage_done), 6), 1);

        if (fae && hp_boost)
            hp_boost = 1; // Suck on my non-HP. :)

        if (hp_boost)
        {
            obvious_effect = true;

            if (attacker->is_player())
            {
                special_damage_message = 
                    make_stringf("You draw strength from %s wounds%s",
                                 defender->pronoun(PRONOUN_POSSESSIVE).c_str(),
                                 attack_strength_punctuation(hp_boost).c_str());
            }
            else if (attacker_visible)
            {
                if (defender->is_player())
                {
                    special_damage_message = 
                        make_stringf("%s draws strength from your wounds%s",
                                     attacker->name(DESC_THE).c_str(),
                                     attack_strength_punctuation(hp_boost).c_str());
                }
                else
                {
                    special_damage_message = 
                        make_stringf("%s is healed%s",
                                     attacker->name(DESC_THE).c_str(),
                                     attack_strength_punctuation(hp_boost).c_str());
                }
            }

            dprf(DIAG_COMBAT, "Vampiric Healing: damage %d, healed %d",
                 damage_done, hp_boost);
            attacker->heal(hp_boost);
        }
        break;
    }
    case SPWPN_PAIN:
        pain_affects_defender();
        break;

    case SPWPN_DISTORTION:
        ret = distortion_affects_defender();
        break;

    case SPWPN_CONFUSE:
    {
        // If a monster with a chaos weapon gets this brand, act like
        // AF_CONFUSE.
        if (attacker->is_monster())
        {
            if (one_chance_in(3) && !mount_defend)
            {
                defender->confuse(attacker,
                                  1 + random2(3+attacker->get_hit_dice()));
            }
            break;
        }

        // Also used for players in fungus form.
        if (attacker->is_player()
            && you.form == transformation::fungus
            && !you.duration[DUR_CONFUSING_TOUCH]
            && defender->is_unbreathing())
        {
            break;
        }

        if (!x_chance_in_y(melee_confuse_chance(defender->get_hit_dice()), 100)
            || defender->as_monster()->check_clarity())
        {
            break;
        }

        // Declaring these just to pass to the enchant function.
        bolt beam_temp;
        beam_temp.thrower   = attacker->is_player() ? KILL_YOU : KILL_MON;
        beam_temp.flavour   = BEAM_CONFUSION;
        beam_temp.source_id = attacker->mid;
        beam_temp.apply_enchantment_to_monster(defender->as_monster());
        obvious_effect = beam_temp.obvious_effect;

        if (attacker->is_player() && damage_brand == SPWPN_CONFUSE
            && you.duration[DUR_CONFUSING_TOUCH])
        {
            you.duration[DUR_CONFUSING_TOUCH] = 0;
            obvious_effect = false;
        }
        break;
    }

    case SPWPN_CHAOS:
        chaos_affects_defender();
        break;

    case SPWPN_ANTIMAGIC:
        antimagic_affects_defender(damage_done * 8);
        break;

    case SPWPN_ACID:
        defender->splash_with_acid(attacker, x_chance_in_y(1, 4) ? 2 : 1 );
        break;

    default:
        if (using_weapon() && is_unrandom_artefact(*weapon, UNRAND_DAMNATION))
            attacker->god_conduct(DID_EVIL, 2 + random2(3));
        break;
    }

    if (damage_brand == SPWPN_CHAOS)
    {
        if (brand != SPWPN_CHAOS && !ret
            && miscast_level == -1 && one_chance_in(20))
        {
            miscast_level  = 0;
            miscast_type   = spschool::random;
            miscast_target = random_choose(attacker, defender);
        }

        if (responsible->is_player())
            did_god_conduct(DID_CHAOS, 2 + random2(3), brand_was_known);
    }

    if (!obvious_effect)
        obvious_effect = !special_damage_message.empty();

    if (needs_message && !special_damage_message.empty())
    {
        mpr(special_damage_message);

        special_damage_message.clear();
        // Don't do message-only miscasts along with a special
        // damage message.
        if (miscast_level == 0)
            miscast_level = -1;
    }

    // Preserve Nessos's brand stacking in a hacky way -- but to be fair, it
    // was always a bit of a hack.
    if (attacker->type == MONS_NESSOS && weapon && is_range_weapon(*weapon))
        apply_poison_damage_brand();

    if (special_damage > 0)
        inflict_damage(special_damage, special_damage_flavour);

    if (obvious_effect && attacker_visible && using_weapon())
    {
        if (is_artefact(*weapon))
            artefact_learn_prop(*weapon, ARTP_BRAND);
        else
            set_ident_flags(*weapon, ISFLAG_KNOW_TYPE);
    }

    return ret;
}

/* Calculates special damage, prints appropriate combat text
 *
 * Applies a particular damage brand to the current attack, the setup and
 * calculation of base damage and other effects varies based on the type
 * of attack, but the calculation of elemental damage should be consistent.
 */
void attack::calc_elemental_brand_damage(beam_type flavour,
                                         const char *verb,
                                         const char *what)
{
    if (flavour == BEAM_FIRE)
        special_damage = damage_done;

    else if (flavour == BEAM_COLD)
        special_damage = div_rand_round(damage_done + random2(damage_done), 4);

    special_damage = resist_adjust_damage(defender, flavour, special_damage, mount_defend);

    if (needs_message && special_damage > 0 && verb)
    {
        // XXX: assumes "what" is singular
        special_damage_message = make_stringf(
            "%s %s %s%s",
            what ? what : atk_name(DESC_THE).c_str(),
            what || mount_attack ? conjugate_verb(verb, false).c_str()
                                 : attacker->conj_verb(verb).c_str(),
            // Don't allow reflexive if the subject wasn't the attacker.
            defender_name(!what).c_str(),
            attack_strength_punctuation(special_damage).c_str());
    }

    if (!mount_defend)
    {
        defender->expose_to_element(BEAM_FIRE, 2);
        if (defender->is_player())
            maybe_melt_player_enchantments(BEAM_FIRE, special_damage);
    }
}

int attack::player_stab_weapon_bonus(int damage)
{
    int stab_skill = you.skill(wpn_skill, 50) + you.skill(SK_STEALTH, 50);

    if (player_good_stab())
    {
        damage += you.dex();

        damage *= 10 + div_rand_round(stab_skill, 100 * stab_bonus);
        damage /= 10;

        // We might be unarmed if we're using the boots of the Assassin.
        if (using_weapon() && (weapon->is_type(OBJ_WEAPONS, WPN_KATAR)))
        {
            damage *= 10 + div_rand_round(stab_skill, 100 * stab_bonus);
            damage /= 10;
        }
    }

    damage *= 10 + div_rand_round(stab_skill, 100 * stab_bonus);
    damage /= 10;

    return damage;
}

int attack::player_stab(int damage)
{
    // The stabbing message looks better here:
    if (stab_attempt)
    {
        // Construct reasonable message.
        stab_message();
        practise_stabbing();
    }
    else
    {
        stab_bonus = 0;
        // Ok.. if you didn't backstab, you wake up the neighborhood.
        // I can live with that.
        alert_nearby_monsters();
    }

    if (stab_bonus)
    {
        // Let's make sure we have some damage to work with...
        damage = max(1, damage);

        damage = player_stab_weapon_bonus(damage);
    }

    return damage;
}

/* Check for stab and prepare combat for stab-values
 *
 * Grant an automatic stab if paralyzed or sleeping (with highest damage value)
 * stab_bonus is used as the divisor in damage calculations, so lower values
 * will yield higher damage. Normal stab chance is (stab_skill + dex + 1 / roll)
 * This averages out to about 1/3 chance for a non extended-endgame stabber.
 */
void attack::player_stab_check()
{
    // XXX: move into find_stab_type?
    if (you.duration[DUR_CLUMSY] || you.confused())
    {
        stab_attempt = false;
        stab_bonus = 0;
        return;
    }

    stab_type st = find_stab_type(&you, *defender);
    // Find stab type is also used for displaying information about monsters,
    // so we need to upgrade the stab type for the Spriggan's Knife here
    if (using_weapon()
        && is_unrandom_artefact(*weapon, UNRAND_SPRIGGANS_KNIFE)
        && st != STAB_NO_STAB)
    {
        st = STAB_SLEEPING;
    }
    stab_attempt = st != STAB_NO_STAB;
    stab_bonus = stab_bonus_denom(st);

    // See if we need to roll against dexterity / stabbing.
    if (stab_attempt && stab_bonus > 1)
    {
        stab_attempt = x_chance_in_y(you.skill_rdiv(wpn_skill, 1, 2)
                                     + you.skill_rdiv(SK_STEALTH, 1, 2)
                                     + you.dex() + 1,
                                     100);
    }

    if (stab_attempt)
        count_action(CACT_STAB, st);
}
