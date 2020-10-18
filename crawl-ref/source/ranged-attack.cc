/**
 * @file
 * @brief ranged_attack class and associated ranged_attack methods
 */

#include "AppHdr.h"

#include "ranged-attack.h"

#include "areas.h"
#include "beam.h"
#include "chardump.h"
#include "coord.h"
#include "coordit.h"
#include "directn.h"
#include "english.h"
#include "env.h"
#include "fprop.h"
#include "god-conduct.h"
#include "items.h"
#include "item-prop.h"
#include "makeitem.h"
#include "message.h"
#include "mon-behv.h"
#include "monster.h"
#include "mon-util.h"
#include "monster.h"
#include "player.h"
#include "stringutil.h"
#include "teleport.h"
#include "throw.h"
#include "traps.h"

ranged_attack::ranged_attack(actor *attk, actor *defn, item_def *proj,
                             bool tele, actor *blame)
    : ::attack(attk, defn, blame), range_used(0), reflected(false),
      projectile(proj), teleport(tele), orig_to_hit(0),
      should_alert_defender(true), launch_type(launch_retval::BUGGY)
{
    init_attack(SK_FIGHTING, 0);
    kill_type = KILLED_BY_BEAM;

    string proj_name = projectile->name(DESC_PLAIN);
    // init launch type early, so we can use it later in the constructor
    launch_type = is_launched(attacker, weapon, weapon, *projectile);

    // [dshaligram] When changing bolt names here, you must edit
    // hiscores.cc (scorefile_entry::terse_missile_cause()) to match.
    if (attacker->is_player())
    {
        kill_type = KILLED_BY_SELF_AIMED;
        aux_source = proj_name;
    }
    else if (launch_type == launch_retval::LAUNCHED)
    {
        aux_source = make_stringf("Shot with a%s %s by %s",
                 (is_vowel(proj_name[0]) ? "n" : ""), proj_name.c_str(),
                 attacker->name(DESC_A).c_str());
    }
    else
    {
        aux_source = make_stringf("Hit by a%s %s thrown by %s",
                 (is_vowel(proj_name[0]) ? "n" : ""), proj_name.c_str(),
                 attacker->name(DESC_A).c_str());
    }

    needs_message = defender_visible;

    if (!using_weapon())
        wpn_skill = SK_FIGHTING;
}

int ranged_attack::calc_to_hit(bool random, bool player_aux)
{
    UNUSED(player_aux); // Needs to get passed because melee-attack needs it and both inherit from attack.
    orig_to_hit = attack::calc_to_hit(random);

    if (orig_to_hit == AUTOMATIC_HIT)
        return AUTOMATIC_HIT;
    float hit = orig_to_hit;

    if (teleport)
    {
        if (attacker->is_player())
        {
            hit *= 10 + (you.attribute[ATTR_PORTAL_PROJECTILE] / 4);
            hit /= 10;
        }

        else
        {
            hit *= 10 + (attacker->as_monster()->get_hit_dice());
            hit /= 10;
        }
    }

    const int lrange = force_range ? force_range : grid_distance(attacker->pos(), defender->pos());

    if (lrange > 9)
        return 0;

    hit *= (20 - lrange);
    hit /= 10;

    const int defl = defender->missile_deflection();
    if (defl)
    {
        if (random)
            hit = random2(hit / defl);
        else
            hit = (hit - 1) / (2 * defl);
    }

    int blockers = 0;

    for (rectangle_iterator ri(attacker->pos(), 1, true); ri; ++ri)
    {
        monster * mon = monster_at(*ri);
        if (mon && !mon->incapacitated()
            && !mons_is_firewood(*mon))
        {
            if (attacker->is_player() && !mon->friendly())
                blockers++;
            if (attacker->is_monster() && attacker->temp_attitude() != mon->temp_attitude())
                blockers++;
            if (!blocker.compare(""))
                blocker = mon->name(DESC_THE);
            else if (coinflip())
                blocker = mon->name(DESC_THE);
        }
    }

    if (blockers == 1)
        hit *= 0.7;
    else if (blockers < 7)
        hit *= (1 - 0.2 - (0.1 * blockers));
    else if (blockers != 0)
        hit *= 0.2;

    if (hit <= 0)
        return 0;

    return rand_round(hit);
}

bool ranged_attack::attack()
{
    if (!handle_phase_attempted())
        return false;

    // XXX: Can this ever happen?
    if (!defender->alive())
    {
        handle_phase_end();
        handle_phase_killed();
        return true;
    }

    attack_count = 1;
    if (projectile->base_type == OBJ_MISSILES)
    {
        if (projectile->sub_type == MI_TRIPLE_BOLT)
            attack_count = 3;
        if (projectile->sub_type == MI_DOUBLE_BOLT)
            attack_count = 2;
    }

    const int ev = defender->evasion(ev_ignore::none, attacker);
    if (defender->is_monster() && defender->type == MONS_CEILING_CRAWLER &&
        (the_path.target != defender->pos() || !the_path.aimed_at_spot) && !one_chance_in(4))
    {
        ev_margin = -1000; // Override for later use.
    }
    else if (the_path.target != defender->pos() && the_path.aimed_at_spot)
    {
        monster * pritarg = monster_at(the_path.target);
        if (pritarg && pritarg->type == MONS_CEILING_CRAWLER && !one_chance_in(4))
            ev_margin = -2000;
    }
    else
        ev_margin = test_hit(to_hit, ev);
    bool shield_blocked = attack_shield_blocked(false);

    god_conduct_trigger conducts[3];
    if (attacker->is_player() && attacker != defender && should_alert_defender)
    {
        set_attack_conducts(conducts, *defender->as_monster(),
                            you.can_see(*defender));
    }

    if (shield_blocked)
        handle_phase_blocked();
    else
    {
        if (ev_margin >= 0)
        {
            if (!handle_phase_hit())
            {
                handle_phase_end();

                if (!defender->alive())
                    handle_phase_killed();

                return false;
            }
        }
        else
            handle_phase_dodged();
    }

    if (should_alert_defender)
        alert_defender();

    if (attacker->is_player() && defender->is_monster()
        && !shield_blocked && ev_margin >= 0)
    {
        print_wounds(*defender->as_monster());
    }

    handle_phase_end();

    if (!defender->alive())
        handle_phase_killed();

    return attack_occurred;
}

// XXX: Are there any cases where this might fail?
bool ranged_attack::handle_phase_attempted()
{
    attacker->attacking(defender, true);
    attack_occurred = true;

    return true;
}

void ranged_attack::set_path(bolt path)
{
    the_path = path;
}

bool ranged_attack::handle_phase_end()
{
    const int remaining_range = you.current_vision - grid_distance(attacker->pos(), defender->pos());

    if (projectile->base_type == OBJ_MISSILES &&
       (projectile->sub_type == MI_TRIPLE_BOLT || projectile->sub_type == MI_DOUBLE_BOLT) &&
        !the_path.aimed_at_spot && !invalid_monster(defender->as_monster()) 
        && (remaining_range >= 1))
    {
        bolt continuation = the_path;
        continuation.range = remaining_range;
        const int x0 = the_path.source.x;
        const int x1 = the_path.target.x;
        const int y0 = the_path.source.y;
        const int y1 = the_path.target.y;
        continuation.target = coord_def(x0 + (x1 - x0) * 5, y0 + (y1 - y0) * 5);
        range_used = BEAM_STOP;
        continuation.source = defender->pos();
        int x = MI_BOLT;
        if (attack_count > 1)
            x = MI_DOUBLE_BOLT;
        int i = items(false, OBJ_MISSILES, x, 1);
        item_def item = env.item[i];
        item.quantity = 1;
        continuation.item = &item;
        continuation.aux_source.clear();
        continuation.name = item.name(DESC_PLAIN, false, false, false);

        continuation.fire();
        destroy_item(i);
    }

    if (projectile->is_type(OBJ_MISSILES, MI_SLING_BULLET) && !reflected && (remaining_range >= 1) && did_hit && one_chance_in(3) &&
        (defender->is_player() || !(mons_is_firewood(*defender->as_monster()) && !invalid_monster(defender->as_monster()))))
    {
        bolt continuation = the_path;
        continuation.range = 3;
        continuation.source = defender->pos();
        int i = items(false, OBJ_MISSILES, MI_SLING_BULLET, 1);
        item_def item = env.item[i];
        item.quantity = 1;
        continuation.item = &item;
        continuation.aux_source.clear();
        continuation.name = item.name(DESC_PLAIN, false, false, false);
        continuation.effect_known = false;
        continuation.effect_wanton = false;
        continuation.source_name = "a ricochet";
        vector<coord_def> coord_list;
        for (rectangle_iterator ri(defender->pos(), 3, true); ri; ++ri)
        {
            if (defender->see_cell_no_trans(*ri))
            {
                coord_def cp(*ri);

                if ((cp != defender->pos()) && (cp == you.pos() || monster_at(*ri)))
                    coord_list.emplace_back(cp);
            }
        }

        if (coord_list.size() > 0)
        {
            coord_def target = coord_list[random2(coord_list.size())];
            dist dest;
            dest.target.x = target.x;
            dest.target.y = target.y;
            dest.isValid = true;
            continuation.set_target(dest);
            mpr("The sling bullet ricochets!");
            continuation.fire();
        }

        destroy_item(i);
    }

    // XXX: this kind of hijacks the shield block check
    if (!is_penetrating_attack(*attacker, weapon, *projectile) && did_hit && attack_count <= 1)
        range_used = BEAM_STOP;

    return attack::handle_phase_end();
}

bool ranged_attack::handle_phase_blocked()
{
    ASSERT(!ignores_shield(false));
    string punctuation = ".";
    string verb = "block";

    const bool reflected_by_shield = defender_shield
                                     && is_shield(*defender_shield)
                                     && shield_reflects(*defender_shield);
    if (reflected_by_shield || defender->reflection())
    {
        reflected = true;
        verb = "reflect";
        if (defender->observable())
        {
            if (reflected_by_shield)
            {
                punctuation = " off " + defender->pronoun(PRONOUN_POSSESSIVE)
                              + " " + defender_shield->name(DESC_PLAIN).c_str()
                              + "!";
                ident_reflector(defender_shield);
            }
            else
            {
                punctuation = " off an invisible shield around "
                            + defender->pronoun(PRONOUN_OBJECTIVE) + "!";

                item_def *amulet = defender->slot_item(EQ_AMULET, false);
                if (amulet)
                   ident_reflector(amulet);
            }
        }
        else
            punctuation = "!";
    }
    else
        attack_count = 0;

    if (needs_message)
    {
        mprf("%s %s %s%s",
             defender_name(false).c_str(),
             defender->conj_verb(verb).c_str(),
             projectile->name(DESC_THE).c_str(),
             punctuation.c_str());
    }

    return attack::handle_phase_blocked();
}

bool ranged_attack::handle_phase_dodged()
{
    did_hit = false;

    const int orig_ev_margin = ev_margin;

    if (ev_margin < -500) // Will never happen normally
    {
        mprf("%s passes %s %s!",
            projectile->name(DESC_THE).c_str(),
            ev_margin < -1500 ? "over" : "under",
            defender->name(DESC_THE).c_str());

        return true;
    }

    if (defender->missile_deflection() && orig_ev_margin >= 0
        && (!blocker.compare("") || coinflip()))
    {
        if (needs_message && defender_visible)
        {
            if (defender->missile_deflection() >= 2)
            {
                mprf("%s %s %s!",
                     defender->name(DESC_THE).c_str(),
                     defender->conj_verb("deflect").c_str(),
                     projectile->name(DESC_THE).c_str());
            }
            else
                mprf("%s is repelled.", projectile->name(DESC_THE).c_str());

            defender->ablate_deflection();
        }

        if (defender->is_player())
            count_action(CACT_DODGE, DODGE_DEFLECT);

        return true;
    }
    else if (orig_ev_margin >= 0)
    {
        if (needs_message && (blocker.length() > 0))
        {
            if (attacker->is_player())
                mprf("%s makes you miss your attack.", blocker.c_str());
            else if (attacker->as_monster()->friendly())
            {
                mprf("%s makes %s miss %s attack.",
                    blocker.c_str(),
                    attacker->as_monster()->name(DESC_THE).c_str(),
                    attacker->pronoun(PRONOUN_POSSESSIVE).c_str());
            }
            needs_message = false;
        }
    }

    if (defender->is_player())
        count_action(CACT_DODGE, DODGE_EVASION);

    if (needs_message)
    {
        mprf("%s%s misses %s%s",
             projectile->name(DESC_THE).c_str(),
             evasion_margin_adverb().c_str(),
             defender_name(false).c_str(),
             attack_strength_punctuation(damage_done).c_str());
    }

    return true;
}

bool ranged_attack::handle_phase_hit()
{
    did_hit = true;
    damage_type = get_damage_type(*projectile);

    if (projectile->is_type(OBJ_MISSILES, MI_NEEDLE))
    {
        damage_done = blowgun_duration_roll(get_ammo_brand(*projectile));
        set_attack_verb(0);
        announce_hit();

        if (!handle_phase_damaged())
            return false;
        if (apply_missile_brand())
            return false;
    }
    else if (projectile->is_type(OBJ_MISSILES, MI_THROWING_NET))
    {
        set_attack_verb(0);
        announce_hit();
        if (defender->is_player())
            player_caught_in_net();
        else
            monster_caught_in_net(defender->as_monster());
    }
    else
    {
        for (; attack_count > 0; --attack_count)
        {
            const int bdam = calc_damage();

            if (bdam > 0)
            {
                // Sweetspotting!
                const int lrange = force_range ? force_range : grid_distance(attacker->pos(), defender->pos());
                int multiplier = 5;
                if (lrange > 5)
                {
                    multiplier -= (lrange - 5);
                    multiplier = max(multiplier, 3); // Bu special case
                }
                else if (lrange < 5)
                    multiplier -= (5 - lrange);
                multiplier = max(multiplier, 1);
                damage_done = div_rand_round(bdam * multiplier, 4);
                set_attack_verb(damage_done);
                if (!handle_phase_damaged())
                    return false;
            }
            else if (needs_message)
            {
                set_attack_verb(0);
                mprf("%s %s %s but does no damage.",
                    projectile->name(DESC_THE).c_str(),
                    attack_verb.c_str(),
                    defender->name(DESC_THE).c_str());
            }

            if (using_weapon() || launch_type == launch_retval::THROWN)
            {
                if (using_weapon()
                    && apply_damage_brand(projectile->name(DESC_THE).c_str()))
                {
                    return false;
                }

                if (apply_missile_brand())
                    return false;

                // Crude Hard code; but running low on time.
                if (projectile->is_type(OBJ_MISSILES, MI_PIE) && defender->is_monster())
                {
                    monster* mon = defender->as_monster();
                    const int bonus = attacker->is_monster() ? attacker->get_hit_dice() / 2
                        : you.skill(item_attack_skill(*weapon)) / 2;

                    if (x_chance_in_y(19 - mon->get_hit_dice() * 2 + bonus, 20))
                    {
                        mprf("%s gets pie all over %s eyes and can't see.", mon->name(DESC_THE).c_str(), 
                            mon->pronoun(PRONOUN_POSSESSIVE).c_str());

                        mon->add_ench(mon_enchant(ENCH_BLIND, 1, attacker,
                            random_range(4, 8) * BASELINE_DELAY));
                    }
                }
            }
        }
    }

    // XXX: unify this with melee_attack's code
    if (attacker->is_player() && defender->is_monster())
    {
        if (should_alert_defender || defender->as_monster()->temp_attitude() == ATT_HOSTILE)
            behaviour_event(defender->as_monster(), ME_WHACK, attacker,
                            coord_def());
    }

    return true;
}

void ranged_attack::ricochet()
{
    should_alert_defender = false;
}

bool ranged_attack::using_weapon() const
{
    return weapon && (launch_type == launch_retval::LAUNCHED
                     || launch_type == launch_retval::BUGGY // not initialized
                         && is_launched(attacker, weapon, weapon, *projectile)
                            == launch_retval::LAUNCHED);
}

int ranged_attack::weapon_damage()
{
    int dam = property(*projectile, PWPN_DAMAGE);
    if (projectile->base_type == OBJ_MISSILES
        && get_ammo_brand(*projectile) == SPMSL_STEEL)
    {
        if (dam)
            dam = div_rand_round(dam * 13, 10);
        else
            dam += 2;
    }
    if (using_weapon())
        dam += attacker->weapon_damage(*weapon);
    else if (attacker->is_player())
        dam += calc_base_unarmed_damage();

    return dam;
}

/**
 * For ranged attacks, "unarmed" is throwing damage.
 */
int ranged_attack::calc_base_unarmed_damage()
{
    return 2;
}

int ranged_attack::calc_mon_to_hit_base(bool random)
{
    return calc_mon_to_hit(attacker->as_monster(), true, attack_number, random);
}

int ranged_attack::apply_damage_modifiers(int damage)
{
    ASSERT(attacker->is_monster());
    if (attacker->as_monster()->is_archer())
    {
        const int bonus = attacker->get_hit_dice() * 4 / 3;
        damage += random2avg(bonus, 2);
    }

    return attack::apply_damage_modifiers(damage);
}

int ranged_attack::player_apply_misc_modifiers(int damage)
{
    if (apply_starvation_penalties())
        damage -= random2(5);

    if (damage_brand == SPWPN_MOLTEN)
        damage = div_rand_round(damage * 3, 4);

    return damage;
}

bool ranged_attack::ignores_shield(bool verbose)
{
    if (defender->is_player() && player_omnireflects())
        return false;

    if (is_penetrating_attack(*attacker, weapon, *projectile))
    {
        if (verbose)
        {
            mprf("%s pierces through %s %s!",
                 projectile->name(DESC_THE).c_str(),
                 apostrophise(defender_name(false)).c_str(),
                 defender_shield ? defender_shield->name(DESC_PLAIN).c_str()
                                 : "shielding");
        }
        return true;
    }
    if (damage_brand == SPWPN_MOLTEN && coinflip())
    {
        if (verbose)
        {
            mprf("The molten %s melts a hole through %s %s!",
                projectile->name(DESC_BASENAME).c_str(),
                apostrophise(defender_name(false)).c_str(),
                defender_shield ? defender_shield->name(DESC_PLAIN).c_str()
                : "shielding");
        }
        return true;
    }
    return false;
}

bool ranged_attack::apply_damage_brand(const char *what)
{
    if (!weapon || !is_range_weapon(*weapon))
        return false;

    damage_brand = get_weapon_brand(*weapon);
    return attack::apply_damage_brand(what);
}

special_missile_type ranged_attack::random_chaos_missile_brand()
{
    special_missile_type brand = SPMSL_NORMAL;
    // Assuming the chaos to be mildly intelligent, try to avoid brands
    // that clash with the most basic resists of the defender,
    // i.e. its holiness.
    while (true)
    {
        brand = (random_choose_weighted(
                    10, SPMSL_FLAME,
                    10, SPMSL_FROST,
                    10, SPMSL_POISONED,
                    10, SPMSL_CHAOS,
                     5, SPMSL_PETRIFICATION,
                     5, SPMSL_SLEEP,
                     5, SPMSL_FRENZY,
                     2, SPMSL_CURARE,
                     2, SPMSL_CONFUSION,
                     2, SPMSL_DISPERSAL));

        if (one_chance_in(3))
            break;

        bool susceptible = true;
        switch (brand)
        {
        case SPMSL_FLAME:
            if (defender->is_fiery())
                susceptible = false;
            break;
        case SPMSL_FROST:
            if (defender->is_icy())
                susceptible = false;
            break;
        case SPMSL_POISONED:
            if (defender->holiness() & MH_UNDEAD)
                susceptible = false;
            break;
        case SPMSL_DISPERSAL:
            if (defender->no_tele(true, false, true))
                susceptible = false;
            break;
        case SPMSL_CONFUSION:
            if (defender->holiness() & MH_PLANT)
            {
                susceptible = false;
                break;
            }
            // fall through
        case SPMSL_SLEEP:
            if (defender->holiness() & (MH_UNDEAD | MH_ELEMENTAL | MH_CONSTRUCT))
                susceptible = false;
            break;
        case SPMSL_PETRIFICATION:
            if (defender->holiness() & MH_CONSTRUCT)
                susceptible = false;
            break;
        case SPMSL_FRENZY:
            if (defender->holiness() & (MH_UNDEAD | MH_ELEMENTAL | MH_CONSTRUCT)
                || defender->is_player()
                   && !you.can_go_berserk(false, false, false)
                || defender->is_monster()
                   && !defender->as_monster()->can_go_frenzy())
            {
                susceptible = false;
            }
            break;
        default:
            break;
        }

        if (susceptible)
            break;
    }
#ifdef NOTE_DEBUG_CHAOS_BRAND
    string brand_name = "CHAOS missile: ";
    switch (brand)
    {
    case SPMSL_NORMAL:          brand_name += "(plain)"; break;
    case SPMSL_FLAME:           brand_name += "flame"; break;
    case SPMSL_FROST:           brand_name += "frost"; break;
    case SPMSL_POISONED:        brand_name += "poisoned"; break;
    case SPMSL_CURARE:          brand_name += "curare"; break;
    case SPMSL_CHAOS:           brand_name += "chaos"; break;
    case SPMSL_DISPERSAL:       brand_name += "dispersal"; break;
    case SPMSL_SLEEP:           brand_name += "sleep"; break;
    case SPMSL_CONFUSION:       brand_name += "confusion"; break;
    case SPMSL_FRENZY:          brand_name += "frenzy"; break;
    default:                    brand_name += "(other)"; break;
    }

    // Pretty much duplicated by the chaos effect note,
    // which will be much more informative.
    if (brand != SPMSL_CHAOS)
        take_note(Note(NOTE_MESSAGE, 0, 0, brand_name), true);
#endif
    return brand;
}

bool ranged_attack::blowgun_check(special_missile_type type)
{
    mon_holy_type holy = mount_defend ? mons_class_holiness(mount_mons())
                                      : defender->holiness();

    if (holy & (MH_UNDEAD | MH_ELEMENTAL | MH_CONSTRUCT))
    {
        if (needs_message)
        {
            if (mount_defend)
                mprf("Your %s is unaffected.", you.mount_name(true).c_str());
            else if (defender->is_monster())
            {
                simple_monster_message(*defender->as_monster(),
                                       " is unaffected.");
            }
            else
                canned_msg(MSG_YOU_UNAFFECTED);
        }
        return false;
    }

    int defense_hd = 0;

    if (mount_defend)
        defense_hd = mount_hd();
    else
        defense_hd = defender->get_hit_dice();

    if (attacker->is_monster())
    {
        int chance = 85 - ((defense_hd
                            - attacker->get_hit_dice()) * 5 / 2);
        chance = min(95, chance);

        if (type == SPMSL_FRENZY)
            chance = chance / 2;
        else if (type == SPMSL_PETRIFICATION || type == SPMSL_SLEEP)
            chance = chance * 4 / 5;

        return x_chance_in_y(chance, 100);
    }

    // BCADDO: This code never executes as the player doesn't use any of these
    // brands anymore. Restore in the future with fixedarts/rare launcher brands.
    const int skill = you.skill_rdiv(SK_SLINGS);

    // You have a really minor chance of hitting with no skills or good
    // enchants.
    if (defender->get_hit_dice() < 15 && random2(100) <= 2)
        return true;

    const int resist_roll = 2 + random2(4 + skill);

    dprf(DIAG_COMBAT, "Brand rolled %d against defender HD: %d.",
         resist_roll, defender->get_hit_dice());

    if (resist_roll < defender->get_hit_dice())
    {
        if (needs_message)
        {
            if (defender->is_monster())
                simple_monster_message(*defender->as_monster(), " resists.");
            else
                canned_msg(MSG_YOU_RESIST);
        }
        return false;
    }

    return true;

}

int ranged_attack::blowgun_duration_roll(special_missile_type type)
{
    if (type == SPMSL_CURARE)
        return 2;

    const int base_power = attacker->get_hit_dice();
    // BCADNOTE: If/when player blowgun/darts return add a calculation for player power here.

    if (type == SPMSL_POISONED)
        return random2(6 + base_power * 2);
    else
        return 5 + random2(base_power);
}

bool ranged_attack::apply_missile_brand()
{
    if (projectile->base_type != OBJ_MISSILES)
        return false;

    special_damage = 0;
    special_missile_type brand = get_ammo_brand(*projectile);
    if (brand == SPMSL_CHAOS)
        brand = random_chaos_missile_brand();

    switch (brand)
    {
    default:
        break;
    case SPMSL_FLAME:
        calc_elemental_brand_damage(BEAM_FIRE,
                                    defender->is_icy() ? "melt" : "burn",
                                    projectile->name(DESC_THE).c_str());

        defender->expose_to_element(BEAM_FIRE, 2);
        if (defender->is_player())
            maybe_melt_player_enchantments(BEAM_FIRE, special_damage);
        break;
    case SPMSL_FROST:
        calc_elemental_brand_damage(BEAM_COLD, "freeze",
                                    projectile->name(DESC_THE).c_str());
        defender->expose_to_element(BEAM_COLD, 2);
        break;
    case SPMSL_POISONED:
        if (projectile->is_type(OBJ_MISSILES, MI_NEEDLE)
                && damage_done > 0
            || !one_chance_in(4))
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

            int pois = projectile->is_type(OBJ_MISSILES, MI_NEEDLE)
                ? damage_done
                : 6 + random2(8) + random2(damage_done * 3 / 2);

            if (mount_defend)
            {
                poison_mount(pois);

                if (old_poison < you.duration[DUR_MOUNT_POISONING])
                    obvious_effect = true;

                break;
            }

            defender->poison(attacker, pois);

            if (defender->is_player()
                   && old_poison < you.duration[DUR_POISONING]
                || !defender->is_player()
                   && old_poison <
                      (defender->as_monster()->get_ench(ENCH_POISON)).degree)
            {
                obvious_effect = true;
            }
        }
        break;
    case SPMSL_CURARE:
        obvious_effect = curare_actor(attacker, defender,
                                      damage_done,
                                      projectile->name(DESC_PLAIN),
                                      atk_name(DESC_PLAIN),
                                      mount_defend);
        break;
    case SPMSL_CHAOS:
        chaos_affects_defender();
        break;
    case SPMSL_DISPERSAL:
        if (damage_done > 0)
        {
            if (defender->no_tele(true, true))
            {
                if (defender->is_player())
                    canned_msg(MSG_STRANGE_STASIS);
            }
            else
            {
                coord_def pos, pos2;
                const bool no_sanct = defender->kill_alignment() == KC_OTHER;
                if (random_near_space(defender, defender->pos(), pos, false,
                                      no_sanct, false)
                    && random_near_space(defender, defender->pos(), pos2, false,
                                         no_sanct, false))
                {
                    const coord_def from = attacker->pos();
                    if (grid_distance(pos2, from) > grid_distance(pos, from))
                        pos = pos2;

                    if (defender->is_player())
                        defender->blink_to(pos);
                    else
                        defender->as_monster()->blink_to(pos, false, false);
                }
            }
        }
        break;
    case SPMSL_SILVER:
        special_damage = silver_damages_victim(defender, damage_done,
                                               special_damage_message, mount_defend);
        break;
    case SPMSL_PETRIFICATION:
        if (!blowgun_check(brand))
            break;
        defender->petrify(attacker, false, mount_defend);
        break;
    case SPMSL_SLEEP:
        if (!blowgun_check(brand))
            break;
        if (mount_defend)
            mprf("Your %s falls asleep momentarily.", you.mount_name(true).c_str());
        else
        {
            defender->put_to_sleep(attacker, damage_done);
            should_alert_defender = false;
        }
        break;
    case SPMSL_CONFUSION:
        if (!blowgun_check(brand))
            break;
        if (mount_defend)
            mprf("Your %s appears momentarily confused.", you.mount_name(true).c_str());
        else
            defender->confuse(attacker, damage_done);
        break;
    case SPMSL_FRENZY:
        if (!blowgun_check(brand))
            break;
        if (defender->is_monster())
        {
            monster* mon = defender->as_monster();
            // Wake up the monster so that it can frenzy.
            if (mon->behaviour == BEH_SLEEP)
                mon->behaviour = BEH_WANDER;
            mon->go_frenzy(attacker);
        }
        else
            defender->go_berserk(false);
        break;
    case SPMSL_BLINDING:
        break;
    }

    if (needs_message && !special_damage_message.empty())
    {
        mpr(special_damage_message);

        special_damage_message.clear();
    }

    if (special_damage > 0)
        inflict_damage(special_damage, special_damage_flavour);

    if (mount_defend)
        return you.mounted();

    return !defender->alive();
}

bool ranged_attack::check_unrand_effects()
{
    return false;
}

bool ranged_attack::mons_attack_effects()
{
    return true;
}

void ranged_attack::player_stab_check()
{
    stab_attempt = false;
    stab_bonus = 0;
}

bool ranged_attack::player_good_stab()
{
    return false;
}

void ranged_attack::set_attack_verb(int damage)
{
    int dmgtyp = get_damage_type(*projectile);
    if (is_penetrating_attack(*attacker, weapon, *projectile))
    {
        if (dmgtyp == DAM_PIERCE)
        {
            attack_verb = (damage < 5)  ? "stabs through"
                        : (damage < 20) ? "pierces through"
                                        : "bores through";
        }
        else if (dmgtyp == DAM_SLICE)
        {
            attack_verb = (damage < 5)  ? "tumbles over"
                        : (damage < 20) ? "slices through"
                                        : "divides";
        }
        else // DAM_BLUDGEON
        {
            attack_verb = (damage < 5)  ? "tumbles over"
                        : (damage < 20) ? "rolls over"
                                        : "bowls through";
        }
    }
    else
    {
        if (projectile->is_type(OBJ_MISSILES, MI_NEEDLE))
            attack_verb = "jabs";
        else if (projectile->is_type(OBJ_MISSILES, MI_THROWING_NET))
            attack_verb = "ensnares";
        else if (dmgtyp == DAM_PIERCE)
        {
            attack_verb = (damage < 5)  ? "pokes"
                        : (damage < 20) ? "punctures"
                                        : "impales";
        }
        else if (dmgtyp == DAM_SLICE)
        {
            attack_verb = (damage < 5)  ? "chops"
                        : (damage < 20) ? "pummels"
                        : (damage < 50) ? "slices"
                                        : "mangles";
        }
        else // DAM_BLUDGEON
        {
            attack_verb = (damage < 5)  ? "hits"
                        : (damage < 20) ? "strikes"
                        : (damage < 50) ? "crushes"
                                        : "pulverizes";
        }
    }
}

void ranged_attack::announce_hit()
{
    if (!needs_message)
        return;

    mprf("%s %s %s%s%s",
         projectile->name(DESC_THE).c_str(),
         attack_verb.c_str(),
         defender_name(false).c_str(),
         attack_strength_punctuation(damage_done).c_str(), 
         resist_message.c_str());
}
