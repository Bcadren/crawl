/**
 * @file
 * @brief Weapon enchantment spells.
**/

#include "AppHdr.h"

#include "spl-wpnench.h"

#include "areas.h"
#include "god-item.h"
#include "god-passive.h"
#include "invent.h"
#include "item-prop.h"
#include "items.h"
#include "message.h"
#include "player-equip.h"
#include "prompt.h"
#include "religion.h"
#include "shout.h"
#include "spl-miscast.h"

/** End your weapon branding spell.
 *
 * Returns the weapon to the previous brand, and ends DUR_EXCRUCIATING_WOUNDS.
 * @param weapon The item in question (which may have just been unwielded).
 * @param verbose whether to print a message about expiration.
 */
void end_weapon_brand(bool verbose)
{
    ASSERT(you.duration[DUR_EXCRUCIATING_WOUNDS]);
    item_def &weapon = you.inv[you.props[PAINED_WEAPON_KEY].get_short()];
    set_item_ego_type(weapon, you.props[ORIGINAL_BRAND_KEY]);
    you.props.erase(ORIGINAL_BRAND_KEY);
    you.duration[DUR_EXCRUCIATING_WOUNDS] = 0;

    if (verbose)
    {
        mprf(MSGCH_DURATION, "%s seems less pained.",
             weapon.name(DESC_YOUR).c_str());
    }

    you.wield_change = true;
    const brand_type real_brand = get_weapon_brand(weapon);
    if (real_brand == SPWPN_ANTIMAGIC)
        calc_mp();
    if (you.weapon() && is_holy_item(weapon) && you.form == transformation::lich)
    {
        mprf(MSGCH_DURATION, "%s falls away!", weapon.name(DESC_YOUR).c_str());
        unequip_item(EQ_WEAPON0);
    }
}

/**
 * Temporarily brand a weapon with pain.
 *
 * @param[in] power         Spellpower.
 * @param[in] fail          Whether you've already failed to cast.
 * @return                  Success, fail, or abort.
 */
spret cast_excruciating_wounds(int power, bool fail)
{
    const brand_type which_brand = SPWPN_PAIN;

    item_def * weapon = nullptr;
    item_def * wpn0 = you.weapon(0);
    item_def * wpn1 = you.weapon(1);

    if (wpn0 && !is_brandable_weapon(*wpn0, true))
        wpn0 = nullptr;
    if (wpn1 && !is_brandable_weapon(*wpn1, true))
        wpn1 = nullptr;

    // Can only brand melee weapons.
    if (wpn0 && is_range_weapon(*wpn0))
    {
        if (!wpn1)
        {
            mpr("You cannot brand ranged weapons with this spell.");
            return spret::abort;
        }
        else
            wpn0 = nullptr;
    }

    if (!wpn0)
        weapon = wpn1;
    else if (!wpn1)
        weapon = wpn0;
    else
    {
        const int equipn = prompt_invent_item("Embue which weapon with agonizing pain?",
                menu_type::invlist,
                OSEL_BRANDABLE_WEAPON,
                OPER_ANY,
                invprompt_flag::escape_only);

        if (prompt_failed(equipn))
            return spret::abort;

        equipment_type hand_used = item_equip_slot(you.inv[equipn]);
        if (hand_used == EQ_NONE)
        {
            mpr("You can only pain wielded weapons.");
            return spret::abort;
        }
        else if (hand_used != EQ_WEAPON0 && hand_used != EQ_WEAPON1)
        {
            mpr("That isn't a weapon.");
            return spret::abort;
        }
        weapon = you.slot_item(hand_used);
        if (is_range_weapon(*weapon))
        {
            mpr("You cannot brand ranged weapons with this spell.");
            return spret::abort;
        }
    }

    const brand_type orig_brand = get_weapon_brand(*weapon);

    const bool has_temp_brand = you.duration[DUR_EXCRUCIATING_WOUNDS] 
            && weapon->link == you.props[PAINED_WEAPON_KEY].get_short();

    const bool end_old = you.duration[DUR_EXCRUCIATING_WOUNDS] && !has_temp_brand;

    if (end_old && !yesno("This will end the effect on your other weapon. Continue anyways?", true, 0))
    {
        canned_msg(MSG_OK);
        return spret::abort;
    }

    if (!has_temp_brand && get_weapon_brand(*weapon) == which_brand)
    {
        mpr("This weapon is already branded with pain.");
        return spret::abort;
    }

    const bool dangerous_disto = orig_brand == SPWPN_DISTORTION
                                 && !have_passive(passive_t::safe_distortion)
                                 && !you.wearing_ego(EQ_GLOVES, SPARM_WIELDING);
    if (dangerous_disto)
    {
        const string prompt =
              "Really brand " + weapon->name(DESC_INVENTORY) + "?";
        if (!yesno(prompt.c_str(), false, 'n'))
        {
            canned_msg(MSG_OK);
            return spret::abort;
        }
    }

    fail_check();

    if (dangerous_disto)
        unwield_distortion(true);

    if (end_old)
        end_weapon_brand();

    noisy(spell_effect_noise(SPELL_EXCRUCIATING_WOUNDS), you.pos());
    mprf("%s %s in agony.", weapon->name(DESC_YOUR).c_str(),
                            silenced(you.pos()) ? "writhes" : "shrieks");

    if (!has_temp_brand)
    {
        you.props[ORIGINAL_BRAND_KEY] = get_weapon_brand(*weapon);
        you.props[PAINED_WEAPON_KEY] = weapon->link;
        set_item_ego_type(*weapon, which_brand);
        you.wield_change = true;
        you.redraw_armour_class = true;
        if (orig_brand == SPWPN_ANTIMAGIC)
            calc_mp();
    }

    you.increase_duration(DUR_EXCRUCIATING_WOUNDS, 8 + roll_dice(2, power), 50);

    return spret::success;
}

spret cast_confusing_touch(int power, bool fail)
{
    fail_check();
    msg::stream << you.hands_act("begin", "to glow ")
                << (you.duration[DUR_CONFUSING_TOUCH] ? "brighter" : "red")
                << "." << endl;

    you.set_duration(DUR_CONFUSING_TOUCH,
                     max(10 + random2(power) / 5,
                         you.duration[DUR_CONFUSING_TOUCH]),
                     20, nullptr);

    return spret::success;
}
