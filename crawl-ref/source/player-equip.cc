#include "AppHdr.h"

#include "player-equip.h"

#include <cmath>

#include "act-iter.h"
#include "areas.h"
#include "artefact.h"
#include "art-enum.h"
#include "attack.h" // attack_strength_punctuation
#include "delay.h"
#include "english.h" // conjugate_verb
#include "evoke.h"
#include "food.h"
#include "god-abil.h"
#include "god-conduct.h"
#include "god-item.h"
#include "god-passive.h"
#include "hints.h"
#include "invent.h"
#include "item-name.h"
#include "item-prop.h"
#include "item-status-flag-type.h"
#include "items.h"
#include "item-use.h"
#include "libutil.h"
#include "macro.h" // command_to_string
#include "mon-place.h"
#include "monster.h"
#include "message.h"
#include "nearby-danger.h"
#include "notes.h"
#include "player-stats.h"
#include "religion.h"
#include "shopping.h"
#include "spl-summoning.h"
#include "spl-transloc.h"
#include "spl-wpnench.h"
#include "stringutil.h"
#include "tag-version.h"
#include "xom.h"

static void _mark_unseen_monsters();

/**
 * Recalculate the player's max hp and set the current hp based on the %change
 * of max hp. This has resulted from our having equipped an artefact that
 * changes max hp.
 */
static void _calc_hp_artefact()
{
    calc_hp();
    if (you.hp_max <= 0) // Borgnjor's abusers...
        ouch(0, KILLED_BY_DRAINING);
}

// Fill an empty equipment slot.
void equip_item(equipment_type slot, int item_slot, bool msg)
{
    ASSERT_RANGE(slot, EQ_FIRST_EQUIP, NUM_EQUIP);
    ASSERT(you.equip[slot] == -1);
    ASSERT(!you.melded[slot]);

    you.equip[slot] = item_slot;

    equip_effect(slot, item_slot, false, msg);

    ash_check_bondage();
    if (you.equip[slot] != -1 && you.inv[you.equip[slot]].cursed())
        auto_id_inventory();
    you.gear_change = true;
}

// Clear an equipment slot (possibly melded).
bool unequip_item(equipment_type slot, bool msg)
{
    ASSERT_RANGE(slot, EQ_FIRST_EQUIP, NUM_EQUIP);
    ASSERT(!you.melded[slot] || you.equip[slot] != -1);

    const int item_slot = you.equip[slot];
    if (item_slot == -1)
        return false;
    else if (you.inv[item_slot].soul_bound())
        return false; // Should never get here; but extra security hurts nothing.
    else
    {
        you.activated.set(slot, false);
        you.equip[slot] = -1;

        if (!you.melded[slot])
            unequip_effect(slot, item_slot, false, msg);
        else
            you.melded.set(slot, false);
        ash_check_bondage();
        you.last_unequip = item_slot;
        you.gear_change = true;
        return true;
    }
}

// Meld a slot (if equipped).
// Does not handle unequip effects, since melding should be simultaneous (so
// you should call all unequip effects after all melding is done)
bool meld_slot(equipment_type slot)
{
    ASSERT_RANGE(slot, EQ_FIRST_EQUIP, NUM_EQUIP);
    ASSERT(!you.melded[slot] || you.equip[slot] != -1);

    if (you.equip[slot] != -1 && !you.melded[slot])
    {
        you.melded.set(slot);
        you.gear_change = true;
        return true;
    }
    return false;
}

// Does not handle equip effects, since unmelding should be simultaneous (so
// you should call all equip effects after all unmelding is done)
bool unmeld_slot(equipment_type slot)
{
    ASSERT_RANGE(slot, EQ_FIRST_EQUIP, NUM_EQUIP);
    ASSERT(!you.melded[slot] || you.equip[slot] != -1);

    if (you.equip[slot] != -1 && you.melded[slot])
    {
        you.melded.set(slot, false);
        you.gear_change = true;
        return true;
    }
    return false;
}

static void _equip_weapon_effect(item_def& item, bool showMsgs, bool unmeld, equipment_type slot);
static void _unequip_weapon_effect(item_def& item, bool showMsgs, bool meld, equipment_type slot);
static void _equip_armour_effect(item_def& arm, bool unmeld,
                                 equipment_type slot);
static void _unequip_armour_effect(item_def& item, bool meld,
                                   equipment_type slot);
static void _equip_jewellery_effect(item_def &item, bool unmeld,
                                    equipment_type slot);
static void _unequip_jewellery_effect(item_def &item, bool mesg, bool meld,
                                      equipment_type slot);
static void _equip_use_warning(const item_def& item);
static void _equip_regeneration_item(const item_def& item);

static void _assert_valid_slot(equipment_type eq, equipment_type slot)
{
    if (slot == EQ_CYTOPLASM)
        return;
    if (eq == slot)
        return;
    if (eq == EQ_RINGS) // all other slots are unique
    {
        if (you.species == SP_FAIRY && slot == EQ_FAIRY_JEWEL)
            return;
        equipment_type r1 = EQ_LEFT_RING, r2 = EQ_RIGHT_RING;
        if (you.species == SP_OCTOPODE)
            r1 = EQ_RING_ONE, r2 = EQ_RING_EIGHT;
        if (slot >= r1 && slot <= r2)
            return;
        if (you.get_mutation_level(MUT_TENDRILS) &&
            (slot == EQ_RING_LEFT_TENDRIL || slot == EQ_RING_RIGHT_TENDRIL))
        {
            return;
        }
        if (const item_def* amu = you.slot_item(EQ_AMULET, true))
            if (is_unrandom_artefact(*amu, UNRAND_FINGER_AMULET) && slot == EQ_RING_AMULET)
                return;
    }
    if (eq == EQ_AMULET && you.species == SP_FAIRY && slot == EQ_FAIRY_JEWEL)
        return;
    if ((eq == EQ_WEAPON0 || eq == EQ_WEAPON1) && (slot == EQ_WEAPON0 || slot == EQ_WEAPON1))
        return;
}

void equip_effect(equipment_type slot, int item_slot, bool unmeld, bool msg)
{
    item_def& item = you.inv[item_slot];
    equipment_type eq = get_item_slot(item);

    _assert_valid_slot(eq, slot);

    if (msg)
        _equip_use_warning(item);

    const interrupt_block block_unmeld_interrupts(unmeld);

    switch (item.base_type)
    {
    case OBJ_ARMOURS:
        _equip_armour_effect(item, unmeld, slot);
        break;
    case OBJ_SHIELDS:
        if (!is_hybrid(item.sub_type))
            _equip_armour_effect(item, unmeld, slot);
        else
            _equip_weapon_effect(item, msg, unmeld, slot);
        break;
    case OBJ_MISCELLANY: // Lantern of shadows
    case OBJ_WEAPONS:
    case OBJ_STAVES:
        _equip_weapon_effect(item, msg, unmeld, slot);
        break;
    case OBJ_JEWELLERY:
        _equip_jewellery_effect(item, unmeld, slot);
        break;
    default: // No other item classes are currently wieldable.
        break;
    }
}

void unequip_effect(equipment_type slot, int item_slot, bool meld, bool msg)
{
    item_def& item = you.inv[item_slot];
    equipment_type eq = get_item_slot(item);

    _assert_valid_slot(eq, slot);

    const interrupt_block block_meld_interrupts(meld);

    switch (item.base_type)
    {
    case OBJ_ARMOURS:
        _unequip_armour_effect(item, meld, slot);
        break;
    case OBJ_SHIELDS:
        if (!is_hybrid(item.sub_type))
            _unequip_armour_effect(item, meld, slot);
        else
            _unequip_weapon_effect(item, msg, meld, slot);
        break;
    case OBJ_MISCELLANY: // Lantern of shadows
    case OBJ_WEAPONS:
    case OBJ_STAVES:
        _unequip_weapon_effect(item, msg, meld, slot);
        break;
    case OBJ_JEWELLERY:
        _unequip_jewellery_effect(item, msg, meld, slot);
        break;
    default: // No other item classes are currently wieldable.
        break;
    }
}

///////////////////////////////////////////////////////////
// Actual equip and unequip effect implementation below
//

static void _equip_artefact_effect(item_def &item, bool *show_msgs, bool unmeld,
    equipment_type slot)
{
    ASSERT(is_artefact(item) || item.cursed());

    // Call unrandart equip function first, so that it can modify the
    // artefact's properties before they're applied.
    if (is_unrandom_artefact(item))
    {
        const unrandart_entry *entry = get_unrand_entry(item.unrand_idx);

        if (entry->equip_func)
            entry->equip_func(&item, show_msgs, unmeld);

        if (entry->world_reacts_func)
            you.unrand_reacts.set(slot);
    }

    const bool alreadyknown = item_type_known(item);
    const bool dangerous = player_in_a_dangerous_place();
    const bool msg = !show_msgs || *show_msgs;

    artefact_properties_t  proprt;
    artefact_known_props_t known;
    artefact_properties_t curses;
    if (is_artefact(item))
    {
        artefact_properties(item, proprt);
        artefact_known_properties(item, known);
    }
    if (item.cursed())
    {
        curse_desc_properties(item, curses);
        if (is_artefact(item))
            for (unsigned int i = 0; i < ART_PROPERTIES; ++i)
                proprt[i] |= curses[i];
        else
            proprt = curses;
    }

    if (proprt[ARTP_AC] || proprt[ARTP_SHIELDING])
        you.redraw_armour_class = true;

    if (proprt[ARTP_EVASION])
        you.redraw_evasion = true;

    if (proprt[ARTP_IMPROVED_VISION])
        autotoggle_autopickup(false);

    if (proprt[ARTP_FIRE] || proprt[ARTP_COLD] || proprt[ARTP_NEGATIVE_ENERGY]
        || proprt[ARTP_MAGIC_RESISTANCE] || proprt[ARTP_POISON] || proprt[ARTP_ELECTRICITY]
        || proprt[ARTP_RCORR])
    {
        you.redraw_resists = true;
    }

    if (proprt[ARTP_MAGICAL_POWER] && !known[ARTP_MAGICAL_POWER] && msg)
    {
        canned_msg(proprt[ARTP_MAGICAL_POWER] > 0 ? MSG_MANA_INCREASE
                                                  : MSG_MANA_DECREASE);
    }

    if (proprt[ARTP_REGENERATION] && !unmeld
        // If regen is an intrinsic property too, don't double print messages
        && !armour_type_prop(item.sub_type, ARMF_REGENERATION))
    {
        _equip_regeneration_item(item);
    }

    // Modify ability scores.
    notify_stat_change(STAT_STR, proprt[ARTP_STRENGTH],
                       !(msg && proprt[ARTP_STRENGTH] && !unmeld));
    notify_stat_change(STAT_INT, proprt[ARTP_INTELLIGENCE],
                       !(msg && proprt[ARTP_INTELLIGENCE] && !unmeld));
    notify_stat_change(STAT_DEX, proprt[ARTP_DEXTERITY],
                       !(msg && proprt[ARTP_DEXTERITY] && !unmeld));

    if (proprt[ARTP_CONTAM] && msg && !unmeld)
        mpr("You feel a build-up of mutagenic energy.");

    if (!unmeld)
    {
        if (proprt[ARTP_CURSE])
        {
            const int XP = exp_needed(min<int>(you.max_level, 27) + 2)
                - exp_needed(min<int>(you.max_level, 27));
            item.props[SOUL_BIND_XP_KEY].get_int() = XP;

            mpr("It binds to your soul!");
        }
    }

    if (proprt[ARTP_RAMPAGING] && msg && !unmeld)
        mpr("You feel ready to rampage towards enemies.");

    if (!alreadyknown && dangerous)
    {
        // Xom loves it when you use an unknown random artefact and
        // there is a dangerous monster nearby...
        xom_is_stimulated(100);
    }

    if (proprt[ARTP_HP])
        _calc_hp_artefact();

    // Let's try this here instead of up there.
    if (proprt[ARTP_MAGICAL_POWER])
        calc_mp();

    if (!fully_identified(item))
    {
        set_ident_type(item, true);
        set_ident_flags(item, ISFLAG_IDENT_MASK);
    }
}

/**
 * If player removes evocable invis and we need to clean things up, set
 * remaining Invis duration to 1 AUT and give the player contam equal to the
 * amount the player would receive if they waited the invis out.
 */
static void _unequip_invis()
{
    if (you.duration[DUR_INVIS] > 1
        && you.evokable_invis() == 0
        && !you.attribute[ATTR_INVIS_UNCANCELLABLE])
    {

        // scale up contam by 120% just to ensure that ending invis early is
        // worse than just resting it off.
        mpr("You absorb a burst of magical contamination as your invisibility "
             "abruptly ends!");
        const int invis_duration_left = you.duration[DUR_INVIS] * 120 / 100;
        const int remaining_contam = div_rand_round(
            invis_duration_left * INVIS_CONTAM_PER_TURN, BASELINE_DELAY
        );
        contaminate_player(remaining_contam, true);
        you.duration[DUR_INVIS] = 0;
    }
}

static void _unequip_fragile_artefact(item_def& item, bool meld)
{
    ASSERT(is_artefact(item) || item.cursed());

    if (artefact_property(item, ARTP_FRAGILE) && !meld)
    {
        mprf("%s crumbles to dust!", item.name(DESC_THE).c_str());
        dec_inv_item_quantity(item.link, 1);
    }
}

static void _unequip_artefact_effect(item_def &item,
                                     bool *show_msgs, bool meld,
                                     equipment_type slot,
                                     bool weapon,
                                     bool wielding_wield = false)
{
    ASSERT(is_artefact(item) || item.cursed());

    artefact_properties_t proprt;
    artefact_known_props_t known;
    artefact_properties_t curse;
    if (is_artefact(item))
    {
        artefact_properties(item, proprt);
        artefact_known_properties(item, known);
    }
    if (item.cursed())
    {
        curse_desc_properties(item, curse);
        if (is_artefact(item))
            for (unsigned int i = 0; i < ART_PROPERTIES; ++i)
                proprt[i] |= curse[i];
        else
            proprt = curse;
    }

    const bool msg = !show_msgs || *show_msgs;

    if (proprt[ARTP_AC] || proprt[ARTP_SHIELDING])
        you.redraw_armour_class = true;

    if (proprt[ARTP_EVASION])
        you.redraw_evasion = true;

    if (proprt[ARTP_FIRE] || proprt[ARTP_COLD] || proprt[ARTP_NEGATIVE_ENERGY]
        || proprt[ARTP_MAGIC_RESISTANCE] || proprt[ARTP_POISON] || proprt[ARTP_ELECTRICITY]
        || proprt[ARTP_RCORR])
    {
        you.redraw_resists = true;
    }

    if (proprt[ARTP_HP])
        _calc_hp_artefact();

    if (proprt[ARTP_MAGICAL_POWER] && !known[ARTP_MAGICAL_POWER] && msg && !wielding_wield)
    {
        canned_msg(proprt[ARTP_MAGICAL_POWER] > 0 ? MSG_MANA_DECREASE
                                                  : MSG_MANA_INCREASE);
    }

    if (!wielding_wield)
    {
        notify_stat_change(STAT_STR, -proprt[ARTP_STRENGTH], true);
        notify_stat_change(STAT_INT, -proprt[ARTP_INTELLIGENCE], true);
        notify_stat_change(STAT_DEX, -proprt[ARTP_DEXTERITY], true);
    }

    if (proprt[ARTP_FLY] != 0 && you.cancellable_flight()
        && !you.evokable_flight())
    {
        you.duration[DUR_FLIGHT] = 0;
        land_player();
    }

    if (proprt[ARTP_INVISIBLE] != 0)
        _unequip_invis();

    if (proprt[ARTP_MAGICAL_POWER])
        calc_mp();

    if (proprt[ARTP_CONTAM] && !meld && !(weapon && you.wearing_ego(EQ_GLOVES, SPARM_WIELDING)))
    {
        mpr("Mutagenic energies flood into your body!");
        contaminate_player(7000, true);
    }

    if (proprt[ARTP_RAMPAGING] && !you.rampaging() && msg && !meld)
        mpr("You no longer feel able to rampage towards enemies.");

    if (proprt[ARTP_DRAIN] && !meld && !(weapon && you.wearing_ego(EQ_GLOVES, SPARM_WIELDING)))
        drain_player(150, true, true);

    if (!wielding_wield)
    {
        if (proprt[ARTP_IMPROVED_VISION])
            _mark_unseen_monsters();

        if (is_unrandom_artefact(item))
        {
            const unrandart_entry *entry = get_unrand_entry(item.unrand_idx);

            if (entry->unequip_func)
                entry->unequip_func(&item, show_msgs);

            if (entry->world_reacts_func)
                you.unrand_reacts.set(slot, false);
        }
    }

    // If the item is a weapon, then we call it from unequip_weapon_effect
    // separately, to make sure the message order makes sense.
    if (!weapon)
        _unequip_fragile_artefact(item, meld);
}

static void _equip_use_warning(const item_def& item)
{
    if (is_holy_item(item) && you_worship(GOD_YREDELEMNUL))
        mpr("You really shouldn't be using a holy item like this.");
    else if (is_corpse_violating_item(item) && you_worship(GOD_FEDHAS))
        mpr("You really shouldn't be using a corpse-violating item like this.");
    else if (is_evil_item(item) && is_good_god(you.religion))
        mpr("You really shouldn't be using an evil item like this.");
    else if (is_unclean_item(item) && you_worship(GOD_ZIN))
        mpr("You really shouldn't be using an unclean item like this.");
    else if (is_chaotic_item(item) && you_worship(GOD_ZIN))
        mpr("You really shouldn't be using a chaotic item like this.");
    else if (is_hasty_item(item) && you_worship(GOD_CHEIBRIADOS))
        mpr("You really shouldn't be using a hasty item like this.");
    else if (is_wizardly_item(item) && you_worship(GOD_TROG))
        mpr("You really shouldn't be using a wizardly item like this.");
}

// Provide a function for handling initial wielding of 'special'
// weapons, or those whose function is annoying to reproduce in
// other places *cough* auto-butchering *cough*.    {gdl}
static void _equip_weapon_effect(item_def& item, bool showMsgs, bool unmeld, equipment_type slot)
{
    you.wield_change = true;
    quiver::on_weapon_changed();
    int special = 0;

    const bool artefact     = is_artefact(item);

    // And here we finally get to the special effects of wielding. {dlb}
    if (item.is_type(OBJ_MISCELLANY, MISC_LANTERN_OF_SHADOWS))
    {
        if (showMsgs)
            mprf(MSGCH_DURATION, "The area is filled with flickering shadows.");

        you.attribute[ATTR_SHADOWS] = 1;
        you.attribute[ATTR_SHADOW_DELAY] = 2 + random2(5);
        update_vision_range();
    }

    if (item.base_type == OBJ_STAVES && !unmeld)
    {
        set_ident_flags(item, ISFLAG_IDENT_MASK);
        set_ident_type(OBJ_STAVES, item.sub_type, true);
        const int dur = 2 + random2(10);
        you.increase_duration(DUR_STAFF, dur, 15);
        if (get_staff_facet(item) == SPSTF_SHIELD)
            you.increase_duration(DUR_STFSHIELD_COOLDOWN, dur + 1 + random2(3), 15);

        mprf(MSGCH_DURATION, "You need a moment to attune to your staff, before it can enhance your spells.");

        if (is_unrandom_artefact(item, UNRAND_MAJIN))
        {
            if (you.undead_state() == US_ALIVE
                && !you_foodless()
                && !unmeld
                && !you.wearing_ego(EQ_GLOVES, SPARM_WIELDING))
            {
                make_hungry(4500, false);
            }
        }
    }

    else if ((item.base_type == OBJ_SHIELDS && is_hybrid(item.sub_type)) || item.base_type == OBJ_WEAPONS)
    {
        // Note that if the unrand equip prints a message, it will
        // generally set showMsgs to false.
        if (artefact || item.cursed())
            _equip_artefact_effect(item, &showMsgs, unmeld, slot);

        const bool was_known = item_type_known(item);

        set_ident_flags(item, ISFLAG_IDENT_MASK);

        special = item.brand;

        if (artefact)
        {
            special = artefact_property(item, ARTP_BRAND);

            if (!was_known && !(item.flags & ISFLAG_NOTED_ID))
            {
                item.flags |= ISFLAG_NOTED_ID;

                // Make a note of it.
                take_note(Note(NOTE_ID_ITEM, 0, 0, item.name(DESC_A),
                    origin_desc(item)));
            }
        }

        if (special != SPWPN_NORMAL)
        {
            // message first
            if (showMsgs)
            {
                const string item_name = item.name(DESC_YOUR);
                switch (special)
                {
                case SPWPN_MOLTEN:
                    if (fires_ammo_type(item) == MI_BOLT)
                        mprf("As you load %s, your bolt melts into a column of liquid metal!", item_name.c_str());
                    if (fires_ammo_type(item) == MI_TRIPLE_BOLT)
                        mprf("As you load %s, each of your bolts melts into a column of liquid metal!", item_name.c_str());
                    else if (fires_ammo_type(item) == MI_SLING_BULLET)
                        mprf("As you load %s, your sling bullet melts into a ball of liquid metal!", item_name.c_str());
                    else if (fires_ammo_type(item) == MI_LARGE_ROCK)
                        mprf("As you load %s, your boulder melts into a mass of molten lava!", item_name.c_str());
                    else if (fires_ammo_type(item) == MI_ARROW)
                        mprf("As you load %s, the head of your arrow melts into a liquid metal spear!", item_name.c_str());
                    else if (fires_ammo_type(item) == MI_PIE)
                        mprf("As you load %s, the kream in your pie becomes boiling hot!", item_name.c_str());
                    else
                        mprf("The surface of %s melts into red hot liquid metal!", item_name.c_str());
                    break;

                case SPWPN_FREEZING:
                    mprf("%s %s", item_name.c_str(),
                        is_range_weapon(item) ?
                        "is covered in frost." :
                        "glows with a cold blue light!");
                    break;

                case SPWPN_HOLY_WRATH:
                case SPWPN_SILVER:
                    mprf("%s softly glows with a divine radiance!",
                        item_name.c_str());
                    break;

                case SPWPN_ELECTROCUTION:
                    if (!silenced(you.pos()))
                    {
                        mprf(MSGCH_SOUND,
                            "You hear the crackle of electricity.");
                    }
                    else
                        mpr("You see sparks fly.");
                    break;

                case SPWPN_DRAGON_SLAYING:
                    mpr("You feel a sudden desire to slay dragons!");
                    break;

                case SPWPN_VENOM:
                    mprf("%s begins to drip with poison!", item_name.c_str());
                    break;

                case SPWPN_PROTECTION:
                    mprf("%s protects you from harm!", item_name.c_str());
                    break;

                case SPWPN_DRAINING:
                    mpr("You sense an unholy aura.");
                    break;

                case SPWPN_SPEED:
                    mpr(you.hands_act("tingle", "!"));
                    break;

                case SPWPN_VAMPIRISM:
                    if (you.species == SP_VAMPIRE)
                        mpr("You feel a bloodthirsty glee!");
                    else if (you.undead_state() == US_ALIVE && !you_foodless() 
                        && !you.wearing_ego(EQ_GLOVES, SPARM_WIELDING))
                        mpr("You feel a dreadful hunger.");
                    else
                        mpr("You feel an empty sense of dread.");
                    break;

                case SPWPN_ORC_SLAYING:
                    if (species_is_orcish(you.species))
                        mpr("You feel like smiting the unbelievers!");
                    else
                        mpr("You feel like slaying orcs.");
                    break;

                case SPWPN_PAIN:
                {
                    const string your_arm = you.arm_name(false);
                    if (you.skill(SK_NECROMANCY) == 0)
                        mpr("You have a feeling of ineptitude.");
                    else if (you.skill(SK_NECROMANCY) <= 6)
                    {
                        mprf("Pain shudders through your %s!",
                            your_arm.c_str());
                    }
                    else
                    {
                        mprf("A searing pain shoots up your %s!",
                            your_arm.c_str());
                    }
                    break;
                }

                case SPWPN_CHAOS:
                    mprf("%s is briefly surrounded by a scintillating aura of "
                        "random colours.", item_name.c_str());
                    break;

                case SPWPN_PENETRATION:
                {
                    // FIXME: make hands_act take a pre-verb adverb so we can
                    // use it here.
                    bool plural = true;
                    string hand = you.hand_name(true, &plural);

                    mprf("Your %s briefly %s through it before you manage "
                        "to get a firm grip on it.",
                        hand.c_str(), conjugate_verb("pass", plural).c_str());
                    break;
                }

                case SPWPN_REAPING:
                    mprf("%s is briefly surrounded by shifting shadows.",
                        item_name.c_str());
                    break;

                case SPWPN_ANTIMAGIC:
                    // Even if your maxmp is 0.
                    if (!you.wearing_ego(EQ_GLOVES, SPARM_WIELDING))
                        mpr("You feel magic leave you.");
                    else
                        mpr("Your gloves protect you from the negative effects of your weapon.");
                    break;

                case SPWPN_DISTORTION:
                    mpr("Space warps around you for a moment!");
                    break;

                case SPWPN_ACID:
                    mprf("%s begins to ooze corrosive slime!", item_name.c_str());
                    break;
                case SPWPN_SPECTRAL:
                    if (you.skill(SK_EVOCATIONS) == 0)
                        mpr("You have a feeling of ineptitude.");
                    else if (you.skill(SK_EVOCATIONS) <= 6)
                        mprf("You feel a bond with your %s.", item_name.c_str());
                    else
                    {
                        mprf("You feel a deep bond with your %s!",
                             item_name.c_str());
                    }
                    break;

                default:
                    break;
                }
            }

            // effect second
            switch (special)
            {
            case SPWPN_VAMPIRISM:
                if (you.undead_state() == US_ALIVE
                    && !you_foodless()
                    && !unmeld
                    && !you.wearing_ego(EQ_GLOVES, SPARM_WIELDING))
                {
                    make_hungry(4500, false);
                }
                break;

            case SPWPN_ANTIMAGIC:
                calc_mp();
                break;

            default:
                break;
            }
        }
    }
}

static void _unequip_weapon_effect(item_def& real_item, bool showMsgs,
                                   bool meld, equipment_type slot)
{
    you.wield_change = true;
    quiver::on_weapon_changed();

    // Fragile artefacts may be destroyed, so make a copy
    item_def item = real_item;

    // Call this first, so that the unrandart func can set showMsgs to
    // false if it does its own message handling.
    if (is_artefact(item) || item.cursed())
    {
        _unequip_artefact_effect(real_item, &showMsgs, meld, slot,
                                 true);
    }

    if (item.is_type(OBJ_MISCELLANY, MISC_LANTERN_OF_SHADOWS))
    {
        you.attribute[ATTR_SHADOWS] = 0;
        you.attribute[ATTR_SHADOW_DELAY] = 0;
        update_vision_range();
        expire_lantern_shadows();
    }
    else if (item.base_type == OBJ_WEAPONS || (item.base_type == OBJ_SHIELDS && is_hybrid(item.sub_type)))
    {
        const int brand = get_weapon_brand(item);

        // Spectral's effect is handled on all unwield in case of spell version.
        if (brand != SPWPN_NORMAL && brand != SPWPN_SPECTRAL)
        {
            const string msg = item.name(DESC_YOUR);

            switch (brand)
            {
            case SPWPN_MOLTEN:
                if (showMsgs)
                    if (is_range_weapon(item))
                        mprf("As you unwield %s, your ammo resolidies.", msg.c_str());
                    else
                        mprf("%s resolidifies.", msg.c_str());
                break;

            case SPWPN_FREEZING:
            case SPWPN_HOLY_WRATH:
            case SPWPN_SILVER:
                if (showMsgs)
                    mprf("%s stops glowing.", msg.c_str());
                break;

            case SPWPN_ELECTROCUTION:
                if (showMsgs)
                    mprf("%s stops crackling.", msg.c_str());
                break;

            case SPWPN_VENOM:
                if (showMsgs)
                    mprf("%s stops dripping with poison.", msg.c_str());
                break;

            case SPWPN_PROTECTION:
                if (showMsgs)
                    mpr("You feel less protected.");
                break;

            case SPWPN_VAMPIRISM:
                if (showMsgs)
                {
                    if (you.species == SP_VAMPIRE)
                        mpr("You feel your glee subside.");
                    else
                        mpr("You feel the dreadful sensation subside.");
                }
                break;

            case SPWPN_ORC_SLAYING:
                if (showMsgs)
                {
                    if (species_is_orcish(you.species))
                        mpr("You feel your religious fervour subside.");
                    else
                        mpr("Your lust for orcish blood subsides.");
                }
                break;

            case SPWPN_DISTORTION:
                if (!meld)
                    unwield_distortion();
                break;

            case SPWPN_ANTIMAGIC:
                calc_mp();
                if (!you.wearing_ego(EQ_GLOVES, SPARM_WIELDING))
                    mpr("You feel magic returning to you.");
                else
                    mpr("Your antimagic aura subsides.");
                break;

                // NOTE: When more are added here, *must* duplicate unwielding
                // effect in brand weapon scroll effect in read_scroll.

            case SPWPN_ACID:
                mprf("%s stops oozing corrosive slime.", msg.c_str());
                break;
            }

            if (you.duration[DUR_EXCRUCIATING_WOUNDS] && item.link == you.props[PAINED_WEAPON_KEY].get_short())
                end_weapon_brand(true);
        }
    }

    if (is_artefact(item) || item.cursed())
        _unequip_fragile_artefact(item, meld);

    // Unwielding dismisses an active spectral weapon
    monster *spectral_weapon = find_spectral_weapon(&you);
    if (spectral_weapon && (spectral_weapon->weapon(0)->base_type == item.base_type))
    {
        mprf("Your spectral weapon disappears as %s.",
             meld ? "your weapon melds" : "you unwield");
        end_spectral_weapon(spectral_weapon, false, true);
    }

    if (item.base_type == OBJ_STAVES && item.sub_type == STAFF_TRANSMUTATION && you.constricting)
    {
        for (const auto &entry : *you.constricting)
        {
            monster *whom = monster_by_mid(entry.first);

            if (!whom)
                continue;
            if (!whom->is_directly_constricted())
                continue;
            if (whom->body_size() > you.body_size() 
                || (you.species != SP_OCTOPODE && you.species != SP_NAGA && you.char_class != JOB_NAGA))
            {
                you.stop_constricting(entry.first, true, true);
                mprf("The constricting tendrils return into %s as you unwield.", item.name(DESC_YOUR).c_str());
                return;
            }
        }
    }
}

static void _wielding_wear_effects(bool unwield, bool unmeld)
{
    string wpn0 = "";
    if (you.weapon(0))
        wpn0 = you.weapon(0)->name(DESC_BASENAME, true);

    string wpn1 = "";
    if (you.weapon(1))
        wpn1 = you.weapon(1)->name(DESC_BASENAME, true);
    const bool vampiric_vuln = you.undead_state() == US_ALIVE && !you_foodless();

    if (unmeld)
    {
        // No bad effects from form changes; just a printed message.
        if (unwield && (you.weapon(0) || you.weapon(1)))
            mprf("You lose your grip on your weapon%s as your gloves meld.",
                you.weapon(0) && you.weapon(1) ? "s" : "");
        else if (unwield)
            mpr("You feel less capable with melee weapons.");
        else
            mpr("You regain your grip.");
    }

    // Trigger bad effects from brands when the gloves are first put on to prevent them being an 
    // easy swap out instead of a committed brand.
    else
    {
        if (you.wearing_ego(EQ_WEAPON0, SPWPN_ANTIMAGIC) || you.wearing_ego(EQ_WEAPON1, SPWPN_ANTIMAGIC))
        {
            if (unwield)
                mpr("You feel magic leave you.");
            else
                mpr("You feel magic returning to you.");
            calc_mp();
        }

        if (you.wearing_ego(EQ_WEAPON0, SPWPN_DISTORTION))
        {
            if (unwield)
            {
                mprf("The distortion field from your %s is now closer to your body. %s",
                    wpn0.c_str(), have_passive(passive_t::safe_distortion) ? "" : "It is no longer safe to unwield.");
            }

            else if (!have_passive(passive_t::safe_distortion))
            {
                mprf("As you don your protective gloves, your %s of distortion lashes out!", wpn0.c_str());
                unwield_distortion();
            }
        }

        if (you.wearing_ego(EQ_WEAPON1, SPWPN_DISTORTION))
        {
            if (unwield)
            {
                mprf("The distortion field from your %s is now closer to your body. %s",
                    wpn1.c_str(), have_passive(passive_t::safe_distortion) ? "" : "It is no longer safe to unwield.");
            }

            else if (!have_passive(passive_t::safe_distortion))
            {
                mprf("As you don your protective gloves, your %s of distortion lashes out!", wpn1.c_str());
                unwield_distortion();
            }
        }

        if (you.char_class == JOB_DEMONSPAWN)
        {
            if (you.wearing_ego(EQ_WEAPON0, SPWPN_SILVER) && unwield)
            {
                mprf("You remove your protective gloves you can no longer wield your %s.", wpn0.c_str());
                unwield_item();
            }
            if (you.wearing_ego(EQ_WEAPON1, SPWPN_SILVER) && unwield)
            {
                mprf("You remove your protective gloves you can no longer wield your %s.", wpn1.c_str());
                unwield_item(false);
            }
        }

        if (vampiric_vuln)
        {
            if (you.wearing_ego(EQ_WEAPON0, SPWPN_VAMPIRISM))
            {
                if (unwield)
                    mprf("You're filled with a deep hunger from your vampiric %s as it comes closer to your body!", wpn0.c_str());
                else
                    mprf("You're filled with a deep hunger from your vampiric %s as you don your gloves!", wpn0.c_str());
                make_hungry(4500, false);
            }
            else if (you.weapon(0) && is_unrandom_artefact(*you.weapon(0), UNRAND_MAJIN))
            {
                if (unwield)
                    mpr("You're filled with a deep hunger from the Majin-Bo as it comes closer to your body!");
                else
                    mpr("You're filled with a deep hunger from the Majin-Bo as you don your gloves!");
                make_hungry(4500, false);
            }

            if (you.wearing_ego(EQ_WEAPON1, SPWPN_VAMPIRISM))
            {
                if (unwield)
                    mprf("You're filled with a deep hunger from your vampiric %s as it comes closer to your body!", wpn1.c_str());
                else
                    mprf("You're filled with a deep hunger from your vampiric %s as you don your gloves!", wpn1.c_str());
                if (you.hunger >= 5000)
                    make_hungry(4500, false);
                else
                    make_hungry(you.hunger - 500, false);
                    // Two of these in a row can outright kill a player. This is a failsafe.
            }

            else if (you.weapon(1) && is_unrandom_artefact(*you.weapon(1), UNRAND_MAJIN))
            {
                if (unwield)
                    mpr("You're filled with a deep hunger from the Majin-Bo as it comes closer to your body!");
                else
                    mpr("You're filled with a deep hunger from the Majin-Bo as you don your gloves!");
                if (you.hunger >= 5000)
                    make_hungry(4500, false);
                else
                    make_hungry(you.hunger - 500, false);
                // Two of these in a row can outright kill a player. This is a failsafe.
            }
        }
        
        bool * dummy = nullptr;
        if (you.weapon(0) && (is_artefact(*you.weapon(0)) || you.weapon(0)->cursed()))
            _unequip_artefact_effect(*you.weapon(0), dummy, false, EQ_WEAPON0, true, true);
        if (you.weapon(1) && (is_artefact(*you.weapon(1)) || you.weapon(1)->cursed()))
            _unequip_artefact_effect(*you.weapon(1), dummy, false, EQ_WEAPON1, true, true);
    }
}

static void _spirit_shield_message(bool unmeld)
{
    if (!unmeld && you.spirit_shield() < 2)
    {
        dec_mp(you.magic_points);
        mpr("You feel your power drawn to a protective spirit.");
#if TAG_MAJOR_VERSION == 34
        if (you.species == SP_DEEP_DWARF
            && !(have_passive(passive_t::no_mp_regen)
                 || player_under_penance(GOD_PAKELLAS)))
        {
            mpr("Now linked to your health, your magic stops regenerating.");
        }
#endif
    }
    else if (!unmeld && you.get_mutation_level(MUT_MANA_SHIELD))
        mpr("You feel the presence of a powerless spirit.");
    else if (!you.get_mutation_level(MUT_MANA_SHIELD))
        mpr("You feel spirits watching over you.");
}

static void _equip_armour_effect(item_def& arm, bool unmeld,
                                 equipment_type slot)
{
    int ego = get_armour_ego_type(arm);
    if (ego != SPARM_NORMAL)
    {
        switch (ego)
        {
        case SPARM_RUNNING:
            if (!you.fishtail)
                mpr("You feel quick.");
            break;

        case SPARM_FIRE_RESISTANCE:
            mpr("You feel resistant to fire.");
            break;

        case SPARM_COLD_RESISTANCE:
            mpr("You feel resistant to cold.");
            break;

        case SPARM_POISON_RESISTANCE:
            if (player_res_poison(false, false, false) < 3)
                mpr("You feel resistant to poison.");
            break;

        case SPARM_IMPROVED_VISION:
            if (you.innate_vision() < 1)
                mpr("Your vision improves.");
            autotoggle_autopickup(false);
            break;

        case SPARM_INVISIBILITY:
            if (!you.duration[DUR_INVIS])
                mpr("You become transparent for a moment.");
            break;

        case SPARM_STRENGTH:
            notify_stat_change(STAT_STR, 3, false);
            break;

        case SPARM_DEXTERITY:
            notify_stat_change(STAT_DEX, 3, false);
            break;

        case SPARM_INTELLIGENCE:
            notify_stat_change(STAT_INT, 3, false);
            break;

        case SPARM_PONDEROUSNESS:
            mpr("You feel rather ponderous.");
            break;

        case SPARM_MAGIC_RESISTANCE:
            mpr("You feel resistant to hostile enchantments.");
            break;

        case SPARM_PROTECTION:
            mpr("You feel protected.");
            break;

        case SPARM_STEALTH:
            if (!you.get_mutation_level(MUT_NO_STEALTH))
                mpr("You feel stealthy.");
            break;

        case SPARM_SOFT:
            mpr("You feel very soft and fluffy.");
            break;

        case SPARM_INSULATION:
            mpr("You feel insulated against electric shocks.");
            break;

        case SPARM_RESISTANCE:
            mpr("You feel resistant to extremes of temperature.");
            break;

        case SPARM_POSITIVE_ENERGY:
            mpr("You feel more protected from negative energy.");
            break;

        case SPARM_ARCHMAGI:
            if (!you.skill(SK_SPELLCASTING))
                mpr("You feel strangely lacking in power.");
            else
                mpr("You feel powerful.");
            break;

        case SPARM_HIGH_PRIEST:
            if (you.species == SP_DEMIGOD || you.char_class == JOB_DEMIGOD)
                mpr("You feel like worshipping yourself.");
            else if (you_worship(GOD_NO_GOD))
                mpr("You feel like seeking a god to worship.");
            else
                mprf(MSGCH_GOD, "You feel a surge of divine power.");
            break;

        case SPARM_SPIRIT_SHIELD:
            _spirit_shield_message(unmeld);
            break;

        case SPARM_ARCHERY:
            mpr("You feel that your aim is more steady.");
            break;

        case SPARM_WIELDING:
            _wielding_wear_effects(false, unmeld);
            break;

        case SPARM_STURDY:
            mpr("You feel more steady on your feet.");
            break;

        case SPARM_REPULSION:
            mpr("You are surrounded by a repulsion field.");
            break;

        case SPARM_CLOUD_IMMUNE:
            // player::cloud_immunity checks the scarf + passives, so can't
            // call it here.
            if (have_passive(passive_t::cloud_immunity))
                mpr("Your immunity to the effects of clouds is unaffected.");
            else
                mpr("You feel immune to the effects of clouds.");
            break;

        case SPARM_SHADOWS:
            mpr("It gets dark.");
            update_vision_range();
            break;

        case SPARM_RAMPAGING:
            mpr("You feel ready to rampage towards enemies.");
            break;

        default:
            mpr("You don't feel anything in particular.");
            break;
        }
    }

    if (armour_type_prop(arm.sub_type, ARMF_REGENERATION) && !unmeld)
        _equip_regeneration_item(arm);

    if (is_artefact(arm) || arm.cursed())
    {
        bool show_msgs = true;
        _equip_artefact_effect(arm, &show_msgs, unmeld, slot);
    }

    you.redraw_armour_class = true;
    you.redraw_evasion = true;
    you.redraw_resists = true;
}

/**
 * The player lost a source of permafly. End their flight if there was
 * no other source, evoking a ring of flight "for free" if possible.
 */
void lose_permafly_source()
{
    const bool had_perm_flight = you.attribute[ATTR_PERM_FLIGHT];

    if (had_perm_flight
        && !you.racial_permanent_flight())
    {
        you.attribute[ATTR_PERM_FLIGHT] = 0;
        if (you.evokable_flight())
        {
            fly_player(
                player_adjust_evoc_power(you.skill(SK_EVOCATIONS, 2) + 30),
                true);
        }
    }

    // since a permflight item can keep tempflight evocations going
    // we should check tempflight here too
    if (you.cancellable_flight() && !you.evokable_flight())
        you.duration[DUR_FLIGHT] = 0;

    if (had_perm_flight)
        land_player(); // land_player() has a check for airborne()
}

static void _unequip_armour_effect(item_def& item, bool meld,
                                   equipment_type slot)
{
    you.redraw_armour_class = true;
    you.redraw_evasion = true;
    you.redraw_resists = true;

    switch (get_armour_ego_type(item))
    {
    case SPARM_RUNNING:
        if (!you.fishtail)
            mpr("You feel rather sluggish.");
        break;

    case SPARM_FIRE_RESISTANCE:
        mpr("You feel less resistant to fire.");
        break;

    case SPARM_COLD_RESISTANCE:
        mpr("You feel less resistant to cold.");
        break;

    case SPARM_POISON_RESISTANCE:
        if (player_res_poison() <= 0)
            mpr("You no longer feel resistant to poison.");
        break;

    case SPARM_IMPROVED_VISION:
        if (you.innate_vision() != 1)
        {
            mpr("Your vision dulls.");
            _mark_unseen_monsters();
        }
        break;

    case SPARM_INVISIBILITY:
        _unequip_invis();
        break;

    case SPARM_STRENGTH:
        notify_stat_change(STAT_STR, -3, false);
        break;

    case SPARM_DEXTERITY:
        notify_stat_change(STAT_DEX, -3, false);
        break;

    case SPARM_INTELLIGENCE:
        notify_stat_change(STAT_INT, -3, false);
        break;

    case SPARM_PONDEROUSNESS:
        mpr("You feel like a heavy weight has been lifted.");
        break;

    case SPARM_MAGIC_RESISTANCE:
        mpr("You feel less resistant to hostile enchantments.");
        break;

    case SPARM_PROTECTION:
        mpr("You feel less protected.");
        break;

    case SPARM_STEALTH:
        if (!you.get_mutation_level(MUT_NO_STEALTH))
            mpr("You feel less stealthy.");
        break;

    case SPARM_SOFT:
        mpr("You feel harder and more rigid.");
        break;

    case SPARM_INSULATION:
        mpr("You feel conductive to electric shocks once more.");
        break;

    case SPARM_RESISTANCE:
        mpr("You feel hot and cold all over.");
        break;

    case SPARM_POSITIVE_ENERGY:
        mpr("You feel less protected from negative energy.");
        break;

    case SPARM_ARCHMAGI:
        mpr("You feel strangely numb.");
        break;

    case SPARM_HIGH_PRIEST:
        if (you.species == SP_DEMIGOD || you.char_class == JOB_DEMIGOD)
            mpr("You feel less interested in self-worship.");
        else 
             mprf(MSGCH_GOD,"Your divine fervour dies down.");
        break;

    case SPARM_SPIRIT_SHIELD:
        if (!you.spirit_shield())
        {
            mpr("You feel strangely alone.");
            if (you.species == SP_DEEP_DWARF)
                mpr("Your magic begins regenerating once more.");
        }
        break;

    case SPARM_ARCHERY:
        mpr("Your aim is not that steady anymore.");
        break;

    case SPARM_WIELDING:
        _wielding_wear_effects(true, meld);
        break;

    case SPARM_STURDY:
        mpr("You feel less sure of your steps.");
        break;

    case SPARM_REPULSION:
        mpr("The haze of the repulsion field disappears.");
        break;

    case SPARM_CLOUD_IMMUNE:
        if (!you.cloud_immune())
            mpr("You feel vulnerable to the effects of clouds.");
        break;

    case SPARM_SHADOWS:
        mpr("The dungeon's light returns to normal.");
        update_vision_range();
        break;

    case SPARM_RAMPAGING:
        if (!you.rampaging())
            mpr("You no longer feel able to rampage towards enemies.");
        break;

    default:
        break;
    }

    if (is_artefact(item) || item.cursed())
        _unequip_artefact_effect(item, nullptr, meld, slot, false);
}

static void _remove_amulet_of_faith(item_def &item)
{
#ifndef DEBUG_DIAGNOSTICS
    UNUSED(item);
#endif
    if (you_worship(GOD_RU))
    {
        // next sacrifice is going to be delaaaayed.
        if (you.piety < piety_breakpoint(5))
        {
#ifdef DEBUG_DIAGNOSTICS
            const int cur_delay = you.props[RU_SACRIFICE_DELAY_KEY].get_int();
#endif
            ru_reject_sacrifices(true);
            dprf("prev delay %d, new delay %d", cur_delay,
                 you.props[RU_SACRIFICE_DELAY_KEY].get_int());
        }
    }
    else if (!you_worship(GOD_NO_GOD)
             && !you_worship(GOD_XOM)
             && !you_worship(GOD_GOZAG))
    {
        simple_god_message(" seems less interested in you.");

        const int piety_loss = div_rand_round(you.piety, 3);
        // Piety penalty for removing the Amulet of Faith.
        if (you.piety - piety_loss > 10)
        {
            mprf(MSGCH_GOD, "You feel less pious.");
            dprf("%s: piety drain: %d",
                 item.name(DESC_PLAIN).c_str(), piety_loss);
            lose_piety(piety_loss);
        }
    }
}

static void _remove_amulet_of_harm()
{
    if (you.undead_state() == US_ALIVE)
        mpr("The amulet drains your life force as you remove it!");
    else
        mpr("The amulet drains your animating force as you remove it!");

    drain_player(150, false, true);
}

// Inefficient, but both not terribly so and called rarely.
// Assumes we want to activate all copies of the same item at the same type.
// Fair assumption, but worth noting.
void activate_item(int base_type, int sub_type)
{
    for (int i = EQ_WEAPON0; i < NUM_EQUIP; i++)
    {
        equipment_type item_slot = static_cast<equipment_type>(i);
        item_def * item;

        if ((item = you.slot_item(item_slot))
            && item->is_type(base_type, sub_type)
            && !you.activated[item_slot])
        {
            you.activated.set(item_slot);
        }
    }
}

static void _equip_regeneration_item(const item_def &item)
{
    equipment_type eq_slot = item_equip_slot(item);
    // currently regen is only on the amulet and armour
    bool plural = eq_slot == EQ_GLOVES || eq_slot == EQ_BOOTS;
    string item_name = is_artefact(item) ? get_artefact_name(item)
                                         : eq_slot == EQ_AMULET
                                         ? "amulet"
                                         : eq_slot == EQ_BODY_ARMOUR
                                         ? "armour"
                                         : item_slot_name(eq_slot);

    if (you.get_mutation_level(MUT_NO_REGENERATION))
    {
        mprf("The %s feel%s cold and inert.", item_name.c_str(),
             plural ? "" : "s");
        return;
    }
    if (you.hp == you.hp_max)
    {
        mprf("The %s throb%s to your uninjured body.", item_name.c_str(),
             plural ? " as they attune themselves" : "s as it attunes itself");
        activate_item(item.base_type, item.sub_type);
        return;
    }
    mprf("The %s cannot attune %s to your injured body.", item_name.c_str(),
         plural ? "themselves" : "itself");
    return;
}

static void _equip_amulet_of_the_acrobat()
{
    if (you.hp == you.hp_max)
    {
        activate_item(OBJ_JEWELLERY, AMU_ACROBAT);
        mpr("You feel ready to tumble and roll out of harm's way.");
    }
    else
        mpr("Your injuries prevent the amulet from attuning itself.");
}

bool acrobat_boost_active()
{
    return you.wearing(EQ_AMULET, AMU_ACROBAT, true, true, true)
           && you.duration[DUR_ACROBAT]
           && (!you.caught())
           && (!you.is_constricted());
}

static void _equip_amulet_of_mana_regeneration()
{
    if (!player_regenerates_mp())
        mpr("The amulet feels cold and inert.");
    else if (you.magic_points == you.max_magic_points)
    {
        you.props[MANA_REGEN_AMULET_ACTIVE] = 1;
        mpr("The amulet hums as it attunes itself to your energized body.");
    }
    else
    {
        mpr("The amulet cannot attune itself to your exhausted body.");
        you.props[MANA_REGEN_AMULET_ACTIVE] = 0;
    }
}

static void _equip_amulet_of_reflection()
{
    if (you.hp == you.hp_max)
    {
        activate_item(OBJ_JEWELLERY, AMU_REFLECTION);
        you.redraw_armour_class = true;
        mpr("You feel a shielding aura gather around you.");
    }
    else
        mpr("The amulet cannot attune itself to your injured body.");
}

static void _equip_jewellery_effect(item_def &item, bool unmeld,
                                    equipment_type slot)
{
    const bool artefact     = is_artefact(item);

    switch (item.sub_type)
    {
    case RING_FIRE:
        mpr("You feel more attuned to fire.");
        you.redraw_resists = true;
        break;

    case RING_ICE:
        mpr("You feel more attuned to ice.");
        // fallthrough
    case RING_RESIST_CORROSION:
    case RING_POISON_RESISTANCE:
    case RING_LIFE_PROTECTION:
    case RING_PROTECTION_FROM_MAGIC:
        you.redraw_resists = true;
        break;

    case RING_PROTECTION:
        you.redraw_armour_class = true;
        break;

    case RING_EVASION:
        you.redraw_evasion = true;
        break;

    case RING_STRENGTH:
        notify_stat_change(STAT_STR, 5, false);
        break;

    case RING_DEXTERITY:
        notify_stat_change(STAT_DEX, 5, false);
        break;

    case RING_INTELLIGENCE:
        notify_stat_change(STAT_INT, 5, false);
        break;

    case RING_MAGICAL_POWER:
        canned_msg(MSG_MANA_INCREASE);
        calc_mp();
        break;

    case RING_TELEPORTATION:
        if (you.no_tele())
            mpr("You feel a slight, muted jump rush through you.");
        else
            // keep in sync with player_teleport
            mprf("You feel slightly %sjumpy.",
                 (player_teleport(false) > 8) ? "more " : "");
        break;

    case AMU_FAITH:
        if (you.species == SP_DEMIGOD || you.char_class == JOB_DEMIGOD)
            mpr("You feel a surge of self-confidence.");
        else if (you_worship(GOD_RU) && you.piety >= piety_breakpoint(5))
        {
            simple_god_message(" says: An ascetic of your devotion"
                               " has no use for such trinkets.");
        }
        else if (you_worship(GOD_GOZAG))
            simple_god_message(" cares for nothing but gold!");
        else
        {
            mprf(MSGCH_GOD, "You feel a %ssurge of divine interest.",
                            you_worship(GOD_NO_GOD) ? "strange " : "");
        }

        break;

    case AMU_THE_GOURMAND:
        if (you_foodless() // Mummy or in lichform
            || you.get_mutation_level(MUT_HERBIVOROUS) > 0) // Spriggan
        {
            mpr("After a brief, frighteningly intense craving, "
                "your appetite remains unchanged.");
        }
        else if (you.get_mutation_level(MUT_CARNIVOROUS) > 0  // Fe, Ko, Gh
                 || you.get_mutation_level(MUT_GOURMAND) > 0) // Troll
        {
            mpr("After a brief, strange feeling in your gut, "
                "your appetite remains unchanged.");
        }
        else
        {
            mpr("You feel a craving for the dungeon's cuisine.");
            you.duration[DUR_GOURMAND] = 0;
        }
        break;

    case AMU_REGENERATION:
        if (!unmeld)
            _equip_regeneration_item(item);
        break;

    case AMU_ACROBAT:
        if (!unmeld)
            _equip_amulet_of_the_acrobat();
        break;

    case AMU_MANA_REGENERATION:
        if (!unmeld)
            _equip_amulet_of_mana_regeneration();
        break;

    case AMU_REFLECTION:
        if (!unmeld)
            _equip_amulet_of_reflection();
        break;

    case AMU_GUARDIAN_SPIRIT:
        _spirit_shield_message(unmeld);
        break;
    }

    bool new_ident = false;
    // Artefacts have completely different appearance than base types
    // so we don't allow them to make the base types known.
    if (artefact || item.cursed())
    {
        bool show_msgs = true;
        _equip_artefact_effect(item, &show_msgs, unmeld, slot);

        set_ident_flags(item, ISFLAG_KNOW_PROPERTIES);
    }
    else
    {
        new_ident = set_ident_type(item, true);
        set_ident_flags(item, ISFLAG_IDENT_MASK);
    }

    if (!unmeld)
        mprf_nocap("%s", item.name(DESC_INVENTORY_EQUIP).c_str());
    if (new_ident)
        auto_assign_item_slot(item);
}

static void _unequip_jewellery_effect(item_def &item, bool mesg, bool meld,
                                      equipment_type slot)
{
    // The ring/amulet must already be removed from you.equip at this point.
    switch (item.sub_type)
    {
    case RING_ATTENTION:
    case RING_SLAYING:
    case RING_STEALTH:
    case RING_TELEPORTATION:
    case RING_WIZARDRY:
    case AMU_REGENERATION:
    case AMU_ACROBAT:
        break;

    case RING_FIRE:
    case RING_ICE:
    case RING_LIFE_PROTECTION:
    case RING_POISON_RESISTANCE:
    case RING_PROTECTION_FROM_MAGIC:
        you.redraw_resists = true;
        break;

    case RING_PROTECTION:
        you.redraw_armour_class = true;
        break;

    case AMU_REFLECTION:
        you.redraw_armour_class = true;
        break;

    case RING_EVASION:
        you.redraw_evasion = true;
        break;

    case RING_STRENGTH:
        notify_stat_change(STAT_STR, -5, false);
        break;

    case RING_DEXTERITY:
        notify_stat_change(STAT_DEX, -5, false);
        break;

    case RING_INTELLIGENCE:
        notify_stat_change(STAT_INT, -5, false);
        break;

    case RING_MAGICAL_POWER:
        canned_msg(MSG_MANA_DECREASE);
        break;

    case AMU_THE_GOURMAND:
        you.duration[DUR_GOURMAND] = 0;
        break;

    case AMU_FAITH:
        if (!meld)
            _remove_amulet_of_faith(item);
        break;

    case AMU_HARM:
        if (!meld)
            _remove_amulet_of_harm();
        break;

    case AMU_GUARDIAN_SPIRIT:
        if (you.species == SP_DEEP_DWARF && player_regenerates_mp())
            mpr("Your magic begins regenerating once more.");
        break;
    }

    if (is_artefact(item) || item.cursed())
        _unequip_artefact_effect(item, &mesg, meld, slot, false);

    // Must occur after ring is removed. -- bwr
    calc_mp();
}

bool unwield_item(bool handedness, bool showMsgs)
{
    equipment_type slot;
    int item = -1;

    if (handedness)
    {
        if (!you.weapon(0))
            return false; 
        slot = EQ_WEAPON0;
        item = 0;
    }
    else
    {
        if (!you.weapon(1))
            return false;
        slot = EQ_WEAPON1;
        item = 1;
    }

    const bool is_weapon = get_item_slot(*you.weapon(item)) == slot;

    if (is_weapon && !safe_to_remove(*you.weapon(item)))
        return false;

    unequip_item(slot, showMsgs);

    you.wield_change     = true;
    quiver::set_needs_redraw();

    return true;
}

static void _mark_unseen_monsters()
{

    for (monster_iterator mi; mi; ++mi)
    {
        if (testbits((*mi)->flags, MF_WAS_IN_VIEW) && !you.can_see(**mi))
        {
            (*mi)->went_unseen_this_turn = true;
            (*mi)->unseen_pos = (*mi)->pos();
        }

    }
}

// This brand is supposed to be dangerous because it does large
// bonus damage, as well as banishment and other side effects,
// and you can rely on the occasional spatial bonus to mow down
// some opponents. It's far too powerful without a real risk.
// -- bwr [ed: ebering]
// 1/3 Nothing. 
// 1/6 3-12 Damage + Blink (mostly harmless)
// 1/6 Banish
// 1/6 Summon Spatial Vortices
// 1/6 Contam (Not as random as before, always Yellow).
void unwield_distortion(bool brand)
{
    if (have_passive(passive_t::safe_distortion))
    {
        simple_god_message(make_stringf(" absorbs the residual spatial "
                           "distortion as you %s your "
                           "weapon.", brand ? "rebrand" : "unwield").c_str());
        return;
    }

    if (you.wearing_ego(EQ_GLOVES, SPARM_WIELDING))
    {
        mprf("Your gloves protect you from the residual spatial "
            "distortion as you %s your weapon.", brand ? "rebrand" : "unwield");
        return;
    }

    switch (random2(6))
    {
    default:
    case 0:
    case 1:
        // BCADDO: More flavourful messages.
        canned_msg(MSG_NOTHING_HAPPENS);
        return;
    case 2:
    {
        int dam = 3 + random2(10);
        mprf("Space twists around you%s", attack_strength_punctuation(dam).c_str());
        you.blink();
        return;
    }
    case 3:
        you.banish(nullptr,
            make_stringf("%sing a weapon of distortion",
                brand ? "rebrand" : "unwield").c_str(),
            you.get_experience_level(), true);
        return;
    case 4:
    {
        bool success = false;
        for (int i = 1 + random2(3); i > 0; --i)
        {
            mgen_data data = mgen_data::hostile_at(MONS_SPATIAL_VORTEX, true, you.pos());
            data.set_summoned(nullptr, 3, SPELL_NO_SPELL, GOD_LUGONU);
            data.set_non_actor_summoner(make_stringf("%sing a weapon of distortion",
                brand ? "rebrand" : "unwield").c_str());

            data.extra_flags |= (MF_NO_REWARD | MF_HARD_RESET);

            monster * created = create_monster(data, false);
            success |= created && created->defined();
        }

        if (success)
        {
            mpr("Space twists in upon itself!");
            return;
        }
        // else fallthrough
    }
    case 5:
        mpr("Space warps into you!");
        contaminate_player(5000 + random2avg(5000, 3), true);
        return;
    }
}
