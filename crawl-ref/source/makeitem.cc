/**
 * @file
 * @brief Item creation routines.
**/

#include "AppHdr.h"

#include "makeitem.h"

#include <algorithm>

#include "art-enum.h" // unrand -> magic staff silliness
#include "artefact.h"
#include "colour.h"
#include "decks.h"
#include "describe.h"
#include "dungeon.h"
#include "item-name.h"
#include "item-prop.h"
#include "item-status-flag-type.h"
#include "items.h"
#include "libutil.h" // map_find
#include "randbook.h"
#include "season.h"
#include "spl-book.h"
#include "state.h"
#include "stepdown.h"
#include "stringutil.h"

int create_item_named(string name, coord_def pos, string *error)
{
    trim_string(name);

    item_list ilist;
    const string err = ilist.add_item(name, false);
    if (!err.empty())
    {
        if (error)
            *error = err;
        return NON_ITEM;
    }

    item_spec ispec = ilist.get_item(0);
    int item = dgn_place_item(ispec, pos);
    if (item != NON_ITEM)
        link_items();
    else if (error)
        *error = "Failed to create item '" + name + "'";

    return item;
}

bool got_curare_roll(const int item_level)
{
    return one_chance_in(item_level > 27 ? 6   :
                         item_level < 2  ? 15  :
                         (364 - 7 * item_level) / 25);
}

/// A mapping from randomly-described object types to their per-game descript
static map<object_class_type, item_description_type> _type_to_idesc = {
    {OBJ_WANDS, IDESC_WANDS},
    {OBJ_POTIONS, IDESC_POTIONS},
    {OBJ_JEWELLERY, IDESC_RINGS},
    {OBJ_SCROLLS, IDESC_SCROLLS},
//    {OBJ_STAVES, IDESC_STAVES}, BCADNOTE: Can't use this while they have brands.
};

/**
 * Initialize the randomized appearance of a given item.
 *
 * For e.g. wand names, potions colors, helmet tiles...
 *
 * XXX: could this be moved into a constructor or reset method...?
 *
 * @param item  The item to have its appearance initialized.
 */
void item_colour(item_def &item)
{
    // Compute random tile/colour choice.
    item.rnd = 1 + random2(255); // reserve 0 for uninitialized

    // reserve the high bit for marking 1 in 10 books "visually special"
    if (item.base_type == OBJ_BOOKS)
    {
        if (one_chance_in(10))
            item.rnd |= 128;
        else
            item.rnd = 1 + random2(127); // can't just trim the high bit,
                                         // since it might be the only set bit
    }

    if (is_unrandom_artefact(item) || item.base_type == OBJ_STAVES)
        return; // don't stomp on item.special!

    // initialize item appearance.
    // we don't actually *need* to store this now, but we *do* need to store
    // it in appearance at some point, since sub_type isn't public information
    // for un-id'd items, and therefore can't be used to do a lookup at the
    // time item names/colours are calculated.
    // it would probably be better to store this at the time that item_info is
    // generated (get_item_info), but that requires some save compat work (and
    // would be wrong if we ever try to get item colour/names directly...?)
    // possibly a todo for a later date.

    if (auto idesc = map_find(_type_to_idesc, item.base_type))
        item.subtype_rnd = you.item_description[*idesc][item.sub_type];
}

// Does Xom consider an item boring?
static bool _is_boring_item(int type, int sub_type)
{
    switch (type)
    {
    case OBJ_POTIONS:
        return sub_type == POT_CURE_MUTATION;
    case OBJ_SCROLLS:
        // These scrolls increase knowledge and thus reduce risk.
        switch (sub_type)
        {
        case SCR_MAGIC_MAPPING:
            return true;
        default:
            break;
        }
        break;
    case OBJ_JEWELLERY:
        return sub_type == AMU_NOTHING;
    default:
        break;
    }
    return false;
}

static weapon_type _determine_weapon_subtype(int item_level)
{
    if (item_level > 6 && one_chance_in(30)
        && x_chance_in_y(10 + item_level, 100))
    {
        return random_choose(WPN_LAJATANG,
                             WPN_FUSTIBALUS,
                             WPN_TRIPLE_CROSSBOW,
                             WPN_DEMON_WHIP,
                             WPN_DEMON_BLADE,
                             WPN_DEMON_TRIDENT,
                             WPN_DOUBLE_SWORD,
                             WPN_EVENINGSTAR,
                             WPN_EXECUTIONERS_AXE,
                             WPN_TANTO,
                             WPN_TRIPLE_SWORD);
    }
    else if (x_chance_in_y(item_level, 20))
    {
        // Pick a weapon based on rarity.
        while (true)
        {
            const int wpntype = random2(NUM_WEAPONS);

            if (x_chance_in_y(weapon_rarity(wpntype), 10))
                return static_cast<weapon_type>(wpntype);
        }
    }
    else if (x_chance_in_y(item_level, item_level+7))
    {
        return random_choose(WPN_QUARTERSTAFF,
                             WPN_FALCHION,
                             WPN_LONG_SWORD,
                             WPN_WAR_AXE,
                             WPN_TRIDENT,
                             WPN_FLAIL,
                             WPN_RAPIER);
    }
    else
    {
        return random_choose(WPN_HUNTING_SLING,
                             WPN_SPEAR,
                             WPN_HAND_AXE,
                             WPN_MACE,
                             // Not worth _weighted for one doubled type.
                             WPN_DAGGER, WPN_DAGGER,
                             WPN_CLUB,
                             WPN_WHIP,
                             WPN_SHORT_SWORD);
    }
}

static bool _try_make_item_unrand(item_def& item, int force_type, int agent)
{
    if (player_in_branch(BRANCH_PANDEMONIUM) && agent == NO_AGENT)
        return false;

    if (force_type == ARM_CAP)
        force_type = ARM_HAT;

    int idx = find_okay_unrandart(item.base_type, force_type,
                                  player_in_branch(BRANCH_ABYSS)
                                      && agent == NO_AGENT);

    if (idx != -1 && make_item_unrandart(item, idx))
        return true;

    return false;
}

// Return whether we made an artefact.
// BCADDO: use the agent int; have some gods give staves (be a nice buff to Kiku, Xom or Veh).
static bool _try_make_staff_artefact(item_def& item, int force_type,
                                      int item_level, bool force_randart,
                                      int agent)
{
    if (item_level > 2 && x_chance_in_y(101 + item_level * 3, 4000)
        || force_randart)
    {
        if (one_chance_in(item_level == ISPEC_GOOD_ITEM ? 7 : 18)
            && !force_randart)
        {
            int old_force_type = force_type;
            if (force_type != STAFF_POISON && force_type != STAFF_EARTH)
                force_type = OBJ_RANDOM; // Only two unique staves are nothing.
            if (_try_make_item_unrand(item, force_type, agent))
                return true;
            force_type = old_force_type;
        }

        // Mean enchantment +6.
        item.plus = 12 - biased_random2(7,2);
        item.plus -= biased_random2(7,2);
        item.plus -= biased_random2(7,2);

        bool cursed = false;
        if (one_chance_in(4))
            cursed = true;
        else if (item.plus > 8 && !one_chance_in(3))
            cursed = true;

        // The rest are normal randarts.
        make_item_randart(item);

        if (cursed)
            curse_item(item);

        return true;
    }

    return false;
}


// Return whether we made an artefact.
static bool _try_make_weapon_artefact(item_def& item, int force_type,
                                      int item_level, bool force_randart,
                                      int agent)
{
    if (item_level > 2 && x_chance_in_y(101 + item_level * 3, 4000)
        || force_randart)
    {
        // Make a randart or unrandart.

        // 1 in 18 randarts are unrandarts.
        if (one_chance_in(item_level == ISPEC_GOOD_ITEM ? 7 : 18)
            && !force_randart)
        {
            if (_try_make_item_unrand(item, force_type, agent))
                return true;
        }

        // Mean enchantment +6.
        item.plus = 12 - biased_random2(7,2);
        item.plus -= biased_random2(7,2);
        item.plus -= biased_random2(7,2);

        bool cursed = false;
        if (one_chance_in(4))
            cursed = true;
        else if (item.plus > 8 && !one_chance_in(3))
            cursed = true;

        // The rest are normal randarts.
        make_item_randart(item);

        if (cursed && get_weapon_brand(item) != SPWPN_HOLY_WRATH)
            do_curse_item(item);

        return true;
    }

    return false;
}

// Return whether we made an artefact.
static bool _try_make_shield_artefact(item_def& item, int force_type,
                                      int item_level, bool force_randart,
                                      int agent)
{
    if (item_level > 2 && x_chance_in_y(101 + item_level * 3, 4000)
        || force_randart)
    {
        // Make a randart or unrandart.

        // 1 in 18 randarts are unrandarts.
        if (one_chance_in(item_level == ISPEC_GOOD_ITEM ? 7 : 18)
            && !force_randart)
        {
            if (_try_make_item_unrand(item, force_type, agent))
                return true;
        }

        // Mean enchantment +6.
        item.plus = 12 - biased_random2(7,2) - biased_random2(7,2) - biased_random2(7,2);

        bool cursed = false;
        if (one_chance_in(4))
            cursed = true;
        else if (item.plus > 8 && !one_chance_in(3))
            cursed = true;

        // On weapons, an enchantment of less than 0 is never viable.
        item.plus = max(static_cast<int>(item.plus), random2(2));

        // The rest are normal randarts.
        make_item_randart(item);

        if (!(is_hybrid(item.sub_type) && get_weapon_brand(item) == SPWPN_HOLY_WRATH)
            && cursed)
            do_curse_item(item);
        return true;
    }

    return false;
}


/**
 * The number of times to try finding a brand for a given item.
 *
 * Result may vary from call to call.
 */
static int _num_brand_tries(const item_def& item, int item_level)
{
    if (item_level >= ISPEC_GIFT)
        return 5;
    if (is_demonic(item) || x_chance_in_y(101 + item_level, 300))
        return 1;
    return 0;
}

// Adding shields.
brand_type determine_weapon_brand(const item_def& item, int item_level)
{
    // Forced ego.
    if (item.brand != 0)
        return static_cast<brand_type>(item.brand);

    weapon_type wpn_type = WPN_CLUB;
    if (item.base_type == OBJ_SHIELDS)
    {
        switch (item.sub_type)
        {
        case SHD_SAI:
            wpn_type = WPN_DAGGER;
        case SHD_NUNCHAKU:
            wpn_type = WPN_QUARTERSTAFF;
        case SHD_TARGE:
            wpn_type = WPN_DIRE_FLAIL;
        }
    }
    else if (item.base_type == OBJ_ARMOURS)
        // Just clawed Gauntlets right now.
        wpn_type = WPN_WHIP;
    else
    {
        wpn_type = static_cast<weapon_type>(item.sub_type);
    }
    const int tries       = _num_brand_tries(item, item_level);
    brand_type rc         = SPWPN_NORMAL;

    for (int count = 0; count < tries && rc == SPWPN_NORMAL; ++count)
        rc = choose_weapon_brand(wpn_type);

    ASSERT(is_weapon_brand_ok(wpn_type, rc, true));
    return rc;
}

// Reject brands which are outright bad for the item. Unorthodox combinations
// are ok, since they can happen on randarts.
bool is_weapon_brand_ok(int type, int brand, bool /*strict*/)
{
    item_def item;
    item.base_type = OBJ_WEAPONS;
    item.sub_type = type;

    if (brand <= SPWPN_NORMAL)
        return true;

    if (type == WPN_TANTO && brand == SPWPN_SPEED)
        return false;

    switch ((brand_type)brand)
    {
    // Universal brands.
    case SPWPN_NORMAL:
    case SPWPN_VENOM:
    case SPWPN_PROTECTION:
    case SPWPN_SPEED:
    case SPWPN_VORPAL:
    case SPWPN_CHAOS:
    case SPWPN_HOLY_WRATH:
    case SPWPN_ELECTROCUTION:
    case SPWPN_MOLTEN:
    case SPWPN_FREEZING:
    case SPWPN_ACID: // Melee-only, except punk.
    case SPWPN_DRAGON_SLAYING: // Normally polearm-only but Bahamut can put on whatever.
        break;

    // Melee-only brands.
    case SPWPN_DRAINING: // Rare Brand, only placed by Yred-related things.
    case SPWPN_VAMPIRISM:
    case SPWPN_PAIN:
    case SPWPN_DISTORTION:
    case SPWPN_ANTIMAGIC:
    case SPWPN_SILVER:
    case SPWPN_REAPING: // only exists on Sword of Zonguldrok
        if (is_range_weapon(item))
            return false;
        break;

    // Ranged-only brands.
    case SPWPN_PENETRATION:
        if (!is_range_weapon(item))
            return false;
        if (item_attack_skill(item) == SK_CROSSBOWS)
            return false;
        break;

#if TAG_MAJOR_VERSION == 34
    // Removed brands.
    case SPWPN_RETURNING:
    case SPWPN_REACHING:
    case SPWPN_ORC_SLAYING:
    case SPWPN_FLAME:
    case SPWPN_FROST:
    case SPWPN_EVASION:
        return false;
#endif

    case SPWPN_CONFUSE:
    case SPWPN_FORBID_BRAND:
    case SPWPN_DEBUG_RANDART:
    case NUM_SPECIAL_WEAPONS:
    case NUM_REAL_SPECIAL_WEAPONS:
        die("invalid brand %d on weapon %d (%s)", brand, type,
            item.name(DESC_PLAIN).c_str());
        break;
    }

    return true;
}

static void _roll_weapon_type(item_def& item, int item_level)
{
    for (int i = 0; i < 1000; ++i)
    {
        item.sub_type = _determine_weapon_subtype(item_level);
        if (is_weapon_brand_ok(item.sub_type, item.brand, true))
            return;
    }

    item.brand = SPWPN_NORMAL; // fall back to no brand
}

static void _roll_shield_type(item_def& item)
{
    while (true)
    {
        const int shieldtype = random2(NUM_SHIELDS);

        if (x_chance_in_y(shield_rarity(shieldtype), 100))
        {
            item.sub_type = static_cast<shield_type>(shieldtype);
            return;
        }
    }
}

/// Plusses for a non-artefact weapon with positive plusses.
int determine_nice_weapon_plusses(int item_level)
{
    const int chance = (item_level >= ISPEC_GIFT ? 200 : item_level);

    // Odd-looking, but this is how the algorithm compacts {dlb}.
    int plus = 0;
    for (int i = 0; i < 4; ++i)
    {
        plus += random2(3);

        if (random2(425) > 35 + chance)
            break;
    }

    return plus;
}

static void _generate_weapon_item(item_def& item, bool allow_uniques,
                                  int force_type, int item_level,
                                  int agent = NO_AGENT)
{
    // Determine weapon type.
    if (force_type != OBJ_RANDOM)
        item.sub_type = force_type;
    else
        _roll_weapon_type(item, item_level);

    // Forced randart.
    if (item_level == ISPEC_RANDART)
    {
        int ego = item.brand;
        for (int i = 0; i < 100; ++i)
            if (_try_make_weapon_artefact(item, force_type, 0, true, agent)
                && is_artefact(item))
            {
                if (ego > SPWPN_NORMAL)
                    item.props[ARTEFACT_PROPS_KEY].get_vector()[ARTP_BRAND].get_short() = ego;
                if (randart_is_bad(item)) // recheck, the brand changed
                {
                    force_type = item.sub_type;
                    item.clear();
                    item.quantity = 1;
                    item.base_type = OBJ_WEAPONS;
                    item.sub_type = force_type;
                    continue;
                }
                return;
            }
        // fall back to an ordinary item
        item_level = ISPEC_GOOD_ITEM;
    }

    // If we make the unique roll, no further generation necessary.
    if (allow_uniques
        && _try_make_weapon_artefact(item, force_type, item_level, false, agent))
    {
        return;
    }

    ASSERT(!is_artefact(item));

    // Artefacts handled, let's make a normal item.
    const bool force_good = item_level >= ISPEC_GIFT;
    const bool forced_ego = item.brand > 0;
    const bool no_brand   = item.brand == SPWPN_FORBID_BRAND;

    if (no_brand)
        item.brand = SPWPN_NORMAL;

    // If it's forced to be a good item, reroll clubs.
    while (force_good && force_type == OBJ_RANDOM && item.sub_type == WPN_CLUB)
        _roll_weapon_type(item, item_level);

    item.plus = 0;

    if (item_level < 0)
    {
        // Thoroughly damaged, could had been good once.
        if (!no_brand && (forced_ego || one_chance_in(4)))
        {
            // Brand is set as for "good" items.
            item.brand = determine_weapon_brand(item, 2 + 2 * env.absdepth0);
        }
        item.plus -= 1 + random2(3);

        if (item_level == ISPEC_BAD)
            do_curse_item(item);
    }
    else if ((force_good || is_demonic(item) || forced_ego
                    || x_chance_in_y(51 + item_level, 200))
                && (!item.is_mundane() || force_good))
    {
        // Make a better item (possibly ego).
        if (!no_brand)
            item.brand = determine_weapon_brand(item, item_level);

        // if acquired item still not ego... enchant it up a bit.
        if (force_good && item.brand == SPWPN_NORMAL)
            item.plus += 2 + random2(3);

        item.plus += determine_nice_weapon_plusses(item_level);

        if (one_chance_in(4))
            do_curse_item(item);
        else if (item.plus > 6 && !one_chance_in(3))
            do_curse_item(item);

        // squash boring items.
        if (!force_good && item.brand == SPWPN_NORMAL && item.plus < 3)
            item.plus = 0;
    }
    else
    {
        if (one_chance_in(6))
        {
            // Make a boring item.
            item.plus = 0;
            item.brand = SPWPN_NORMAL;
        }
    }
}

special_armour_type defensive_shield_brand (int force_brand)
{
    if (force_brand > 0)
        return (special_armour_type)force_brand;

    return random_choose_weighted(1, SPARM_RESISTANCE,
                                  3, SPARM_FIRE_RESISTANCE,
                                  3, SPARM_COLD_RESISTANCE,
                                  3, SPARM_POISON_RESISTANCE,
                                  3, SPARM_POSITIVE_ENERGY,
                                  6, SPARM_REFLECTION,
                                 12, SPARM_PROTECTION);
}

static void _generate_shield_item(item_def& item, bool allow_uniques,
    int force_type, int item_level,
    int agent = NO_AGENT)
{
    // Determine shield type.
    if (force_type != OBJ_RANDOM)
        item.sub_type = force_type;
    else
        _roll_shield_type(item);

    item.quantity = 1;
    
    // Forced randart.
    if (item_level == ISPEC_RANDART)
    {
        int ego = item.brand;
        for (int i = 0; i < 100; ++i)
            if (_try_make_shield_artefact(item, force_type, 0, true, agent)
                && is_artefact(item))
            {
                if (is_hybrid (item.sub_type) && ego > SPWPN_NORMAL)
                    item.props[ARTEFACT_PROPS_KEY].get_vector()[ARTP_BRAND].get_short() = ego;
                if (randart_is_bad(item)) // recheck, the brand changed
                {
                    force_type = item.sub_type;
                    item.clear();
                    item.quantity = 1;
                    item.base_type = OBJ_SHIELDS;
                    item.sub_type = force_type;
                    continue;
                }
                return;
            }
        // fall back to an ordinary item
        item_level = ISPEC_GOOD_ITEM;
    }

    // If we make the unique roll, no further generation necessary.
    if (allow_uniques
        && _try_make_shield_artefact(item, force_type, item_level, false, agent))
    {
        return;
    }

    ASSERT(!is_artefact(item));

    // Artefacts handled, let's make a normal item.

    const bool force_good = item_level >= ISPEC_GIFT;
    const bool forced_ego = item.brand > 0;
    const bool no_brand = item.brand == SPWPN_FORBID_BRAND;

    if (no_brand)
        item.brand = SPWPN_NORMAL;

    item.plus = 0;

    if (item_level < 0)
    {
        // Thoroughly damaged, could had been good once.
        if (!no_brand && (forced_ego || one_chance_in(4)))
        {
            // Brand is set as for "good" items.
            if (is_hybrid(item.sub_type))
                item.brand = determine_weapon_brand(item, 2 + 2 * env.absdepth0);
            else
                item.brand = defensive_shield_brand(item.brand);
        }
        item.plus -= 1 + random2(3);

        if (item_level == ISPEC_BAD)
            do_curse_item(item);
    }
    else if ((force_good || forced_ego
        || x_chance_in_y(51 + item_level, 200))
        && (!item.is_mundane() || force_good))
    {
        // Make a better item (possibly ego).
        if (!no_brand)
        {
            if (is_hybrid(item.sub_type))
                item.brand = determine_weapon_brand(item, item_level);
            else
                item.brand = defensive_shield_brand(item.brand);
        }

        // if acquired item still not ego... enchant it up a bit.
        if (force_good && item.brand == SPWPN_NORMAL)
            item.plus += 2 + random2(3);

        item.plus += determine_nice_weapon_plusses(item_level);

        if (one_chance_in(4))
            do_curse_item(item);
        else if (item.plus > 5 && !one_chance_in(3))
            do_curse_item(item);

        // squash boring items.
        if (!force_good && item.brand == SPWPN_NORMAL && item.plus < 3)
            item.plus = 0;
    }
    else
    {
        if (one_chance_in(6))
        {
            // Make a boring item.
            item.plus = 0;
            item.brand = SPWPN_NORMAL;
        }
    }
}

// Current list is based on dpeg's original post to the Wiki, found at the
// page: <http://crawl.develz.org/wiki/doku.php?id=DCSS%3Aissue%3A41>.
// Remember to update the code in is_missile_brand_ok if adding or altering
// brands that are applied to missiles. {due}
static special_missile_type _determine_missile_brand(const item_def& item,
                                                     int item_level)
{
    // Forced ego.
    if (item.brand != 0)
        return static_cast<special_missile_type>(item.brand);

    const bool force_good = item_level >= ISPEC_GIFT;
    special_missile_type rc = SPMSL_NORMAL;

    // "Normal weight" of SPMSL_NORMAL.
    int nw = force_good ? 0 : random2(2000 - 55 * item_level);

    switch (item.sub_type)
    {
#if TAG_MAJOR_VERSION == 34
    case MI_DART:
#endif
    case MI_STONE:
    case MI_THROWING_NET:
    case MI_LARGE_ROCK:
    case MI_SLING_BULLET:
    case MI_ARROW:
    case MI_BOLT:
    default:
        rc = SPMSL_NORMAL;
        break;
    case MI_NEEDLE:
        // Curare is special cased, all the others aren't.
        if (got_curare_roll(item_level))
        {
            rc = SPMSL_CURARE;
            break;
        }

        rc = random_choose_weighted(30, SPMSL_SLEEP,
                                    30, SPMSL_CONFUSION,
                                    10, SPMSL_PETRIFICATION,
                                    10, SPMSL_FRENZY,
                                    nw, SPMSL_POISONED);
        break;
    case MI_JAVELIN:
        rc = random_choose_weighted(30, SPMSL_RETURNING,
                                    32, SPMSL_PENETRATION,
                                    32, SPMSL_POISONED,
                                    21, SPMSL_STEEL,
                                    20, SPMSL_SILVER,
                                    nw, SPMSL_NORMAL);
        break;
    case MI_TOMAHAWK:
        rc = random_choose_weighted(15, SPMSL_POISONED,
                                    10, SPMSL_SILVER,
                                    10, SPMSL_STEEL,
                                    12, SPMSL_DISPERSAL,
                                    28, SPMSL_RETURNING,
                                    15, SPMSL_EXPLODING,
                                    nw, SPMSL_NORMAL);
        break;
    }

    ASSERT(is_missile_brand_ok(item.sub_type, rc, true));

    return rc;
}

bool is_staff_brand_ok(int type, int brand, bool /*strict*/)
{
    switch (brand)
    {
    case SPSTF_SHIELD:
        if (type == STAFF_POISON)
            return false;
        // fallthrough
    case SPSTF_FLAY:
        if (type == STAFF_DEATH)
            return false;
        // fallthrough
    case SPSTF_MENACE:
    case SPSTF_SCOPED:
    case SPSTF_ACCURACY:
    case SPSTF_WARP:
        if (type == STAFF_SUMMONING)
            return false;
        if (brand == SPSTF_SHIELD)
            return true;
        // fallthrough
    case SPSTF_CHAOS:
        if (type == STAFF_TRANSMUTATION)
            return false;
        // fallthrough
    default:
        break;
    }

    if (brand >= NUM_SPECIAL_STAVES || brand < 0)
        return false;

    return true;
}

bool is_missile_brand_ok(int type, int brand, bool strict)
{
    // Launcher ammo can never be branded.
    if ((type == MI_LARGE_ROCK
        || type == MI_SLING_BULLET
        || type == MI_ARROW
        || type == MI_BOLT)
        && brand != SPMSL_NORMAL
        && strict)
    {
        return false;
    }

    // Never generates, only used for chaos-branded missiles.
    if (brand == SPMSL_FLAME || brand == SPMSL_FROST)
        return false;

    // In contrast, needles should always be branded.
    // And all of these brands save poison are unique to needles.
    switch (brand)
    {
    case SPMSL_POISONED:
        if (type == MI_NEEDLE)
            return true;
        break;

    case SPMSL_CURARE:
    case SPMSL_PETRIFICATION:
#if TAG_MAJOR_VERSION == 34
    case SPMSL_SLOW:
#endif
    case SPMSL_SLEEP:
    case SPMSL_CONFUSION:
#if TAG_MAJOR_VERSION == 34
    case SPMSL_SICKNESS:
#endif
    case SPMSL_FRENZY:
        return type == MI_NEEDLE;

#if TAG_MAJOR_VERSION == 34
    case SPMSL_BLINDING:
        // possible on ex-pies
        return type == MI_TOMAHAWK && !strict;
#endif

    default:
        if (type == MI_NEEDLE)
            return false;
    }

    // Everything else doesn't matter.
    if (brand == SPMSL_NORMAL)
        return true;

    // In non-strict mode, everything other than needles is mostly ok.
    if (!strict)
        return true;

    // Not a missile?
    if (type == 0)
        return true;

    // Specifics
    switch (brand)
    {
    case SPMSL_POISONED:
        return type == MI_JAVELIN || type == MI_TOMAHAWK;
    case SPMSL_RETURNING:
        return type == MI_JAVELIN || type == MI_TOMAHAWK;
    case SPMSL_CHAOS:
        return type == MI_TOMAHAWK || type == MI_JAVELIN;
    case SPMSL_PENETRATION:
        return type == MI_JAVELIN;
    case SPMSL_DISPERSAL:
        return type == MI_TOMAHAWK;
    case SPMSL_EXPLODING:
        return type == MI_TOMAHAWK;
    case SPMSL_STEEL: // deliberate fall through
    case SPMSL_SILVER:
        return type == MI_JAVELIN || type == MI_TOMAHAWK;
    default: break;
    }

    // Assume no, if we've gotten this far.
    return false;
}

static void _generate_missile_item(item_def& item, int force_type,
                                   int item_level)
{
    const bool no_brand = (item.brand == SPMSL_FORBID_BRAND);
    if (no_brand)
        item.brand = SPMSL_NORMAL;

    item.plus = 0;

    if (force_type != OBJ_RANDOM)
        item.sub_type = force_type;
    else
    {
        item.sub_type =
            random_choose_weighted(32, MI_SLING_BULLET,
                                   30, MI_ARROW,
                                   22, MI_BOLT,
                                   10, MI_NEEDLE,
                                   3,  MI_TOMAHAWK,
                                   2,  MI_JAVELIN,
                                   1,  MI_THROWING_NET,
                                   1,  MI_LARGE_ROCK);
    }

    item.quantity = 1;

    // No fancy rocks -- break out before we get to special stuff.
    if (item.sub_type == MI_LARGE_ROCK || item.sub_type == MI_THROWING_NET) 
        return;

    if (!no_brand)
        item.brand = _determine_missile_brand(item, item_level);
}

static bool _armour_disallows_randart(int sub_type)
{
    // Scarves are never randarts. BCADDO: Change this.
    return sub_type == ARM_SCARF;
}

static bool _try_make_armour_artefact(item_def& item, int force_type,
                                      int item_level, bool force_randart,
                                      int agent)
{
    if (item_level > 2 && x_chance_in_y(101 + item_level * 3, 4000)
        || force_randart)
    {
        // Make a randart or unrandart.

        // 1 in 20 randarts are unrandarts.
        if (one_chance_in(item_level == ISPEC_GOOD_ITEM ? 7 : 20)
            && !force_randart)
        {
            if (_try_make_item_unrand(item, force_type, agent))
                return true;
        }

        if (_armour_disallows_randart(item.sub_type))
            return false;

        // The rest are normal randarts.

        // 10% of boots become barding.
        if (item.sub_type == ARM_BOOTS && one_chance_in(10) && (you.chapter != CHAPTER_NONDUNGEON_START))
            item.sub_type = random_choose(ARM_NAGA_BARDING, ARM_CENTAUR_BARDING);

        // Determine enchantment and cursedness.
        if (one_chance_in(5))
            item.plus = 0;
        else
        {
            int max_plus = armour_max_enchant(item);
            item.plus = random2(max_plus + 1);

            if (one_chance_in(5))
                item.plus += random2(max_plus + 6) / 2;

            if (one_chance_in(6))
                item.plus -= random2(max_plus + 6);

            if (item.plus > 5 && !one_chance_in(3))
                do_curse_item(item);
            else if (one_chance_in(4))
                do_curse_item(item);
        }

        // On body armour, an enchantment of less than 0 is never viable.
        if (get_armour_slot(item) == EQ_BODY_ARMOUR)
            item.plus = max(static_cast<int>(item.plus), random2(2));

        // Needs to be done after the barding chance else we get randart
        // bardings named Boots of xy.
        make_item_randart(item);

        // Don't let unenchantable armour get minuses.
        if (!armour_is_enchantable(item))
            item.plus = 0;

        return true;
    }

    return false;
}

/**
 * Generate an appropriate ego for a type of armour.
 *
 * @param item          The type of armour in question.
 * @return              An ego appropriate to the item type.
 *                      May be SPARM_NORMAL.
 */
special_armour_type generate_armour_type_ego(armour_type type)
{
    // TODO: move this into data
    switch (type)
    {
    case ARM_SCARF:
        return random_choose_weighted(1, SPARM_SPIRIT_SHIELD,
                                      1, SPARM_RESISTANCE,
                                      1, SPARM_REPULSION,
                                      1, SPARM_CLOUD_IMMUNE);

    case ARM_CLOAK:
        return random_choose(SPARM_POISON_RESISTANCE,
                             SPARM_INVISIBILITY,
                             SPARM_MAGIC_RESISTANCE,
                             SPARM_SOFT);

    case ARM_CAP:
        if (one_chance_in(4))
            return SPARM_COLD_RESISTANCE;
        // fall-through.
    case ARM_HAT:
        return random_choose_weighted(5, SPARM_NORMAL,
                                      3, SPARM_MAGIC_RESISTANCE,
                                      2, SPARM_INTELLIGENCE,
                                      2, SPARM_IMPROVED_VISION);

    case ARM_HELMET:
        return random_choose(SPARM_IMPROVED_VISION, SPARM_INTELLIGENCE);

    case ARM_CLAW:
        return SPARM_DEXTERITY; // Actual value set with weapon brands.

    case ARM_GLOVES:
        return random_choose_weighted(3, SPARM_DEXTERITY, 
                                      3, SPARM_STRENGTH,
                                      3, SPARM_INSULATION,
                                      1, SPARM_ARCHERY,
                                      1, SPARM_WIELDING);

    case ARM_BOOTS:
        return random_choose_weighted(1, SPARM_RUNNING, 
                                      2, SPARM_STURDY,
                                      3, SPARM_INSULATION,
                                      4, SPARM_STEALTH);

    case ARM_NAGA_BARDING:
    case ARM_CENTAUR_BARDING:
        return random_choose(SPARM_STEALTH,
                             SPARM_COLD_RESISTANCE, SPARM_FIRE_RESISTANCE);

    case ARM_ROBE:
        return random_choose_weighted(1, SPARM_RESISTANCE,
                                      1, SPARM_ARCHMAGI,
                                      1, SPARM_HIGH_PRIEST,
                                      2, SPARM_NORMAL,
                                      2, SPARM_SOFT,
                                      2, SPARM_COLD_RESISTANCE,
                                      2, SPARM_FIRE_RESISTANCE,
                                      2, SPARM_POSITIVE_ENERGY,
                                      4, SPARM_MAGIC_RESISTANCE);

    case ARM_PLATE_ARMOUR:
        return random_choose_weighted(26, SPARM_FIRE_RESISTANCE,
                                      26, SPARM_COLD_RESISTANCE,
                                      19, SPARM_POISON_RESISTANCE,
                                      15, SPARM_MAGIC_RESISTANCE,
                                      15, SPARM_HIGH_PRIEST,
                                       7, SPARM_ARCHMAGI,
                                       7, SPARM_POSITIVE_ENERGY,
                                       7, SPARM_PONDEROUSNESS);

    // other body armour
    default:
        break;
    }

    // dragon/troll armour, animal hides, and crystal plate are never generated
    // with egos. (unless they're artefacts, but those aren't handled here.)
    // TODO: deduplicate with armour_is_special() (same except for animal skin)
    if (armour_type_is_hide(type)
        || type == ARM_ANIMAL_SKIN
        || type == ARM_CRYSTAL_PLATE_ARMOUR)
    {
        return SPARM_NORMAL;
    }

    return random_choose_weighted(7, SPARM_FIRE_RESISTANCE,
                                  7, SPARM_COLD_RESISTANCE,
                                  5, SPARM_POISON_RESISTANCE,
                                  4, SPARM_MAGIC_RESISTANCE,
                                  2, SPARM_POSITIVE_ENERGY);
}

/**
 * Generate an appropriate ego for a piece of armour.
 *
 * @param item          The item in question.
 * @return              The item's current ego, if it already has one;
 *                      otherwise, an ego appropriate to the item.
 *                      May be SPARM_NORMAL.
 */
static special_armour_type _generate_armour_ego(const item_def& item)
{
    if (item.brand != SPARM_NORMAL)
        return static_cast<special_armour_type>(item.brand);

    const special_armour_type ego
        = generate_armour_type_ego(static_cast<armour_type>(item.sub_type));

    if (is_armour_brand_ok(item.sub_type, ego, true))
        return ego;
    if (item.base_type == OBJ_SHIELDS)
        return ego;
    // Hacky if it causes weird behavior we may need to restore the assert.
    return SPARM_NORMAL;
}

/**
* Generate an appropriate facet for a magical staff.
*
* @param item          The item in question.
* @param item_level    A 'level' of item to generate.
* @return              The item's current ego, if it already has one;
*                      otherwise, an ego appropriate to the item.
*                      May be SPSTF_NORMAL.
*/
facet_type generate_staff_facet(const item_def& item,
    int item_level)
{
    if (x_chance_in_y(500 - item_level, 500))
        return SPSTF_NORMAL;

    int type = item.sub_type;

        // Total Weight: 42 (Arbitrary).
    return random_choose_weighted(
        is_staff_brand_ok(type, SPSTF_MENACE)   ? 8 : 0, SPSTF_MENACE,
        is_staff_brand_ok(type, SPSTF_SHIELD)   ? 6 : 0, SPSTF_SHIELD,
        is_staff_brand_ok(type, SPSTF_FLAY)     ? 6 : 0, SPSTF_FLAY,
        is_staff_brand_ok(type, SPSTF_SCOPED)   ? 4 : 0, SPSTF_SCOPED,
                                                      4, SPSTF_WIZARD,
                                                      4, SPSTF_REAVER,
                                                      3, SPSTF_ENERGY,
        is_staff_brand_ok(type, SPSTF_ACCURACY) ? 3 : 0, SPSTF_ACCURACY,
        is_staff_brand_ok(type, SPSTF_WARP)     ? 2 : 0, SPSTF_WARP,
        is_staff_brand_ok(type, SPSTF_CHAOS)    ? 2 : 0, SPSTF_CHAOS);
}

bool is_armour_brand_ok(int type, int brand, bool strict)
{
    equipment_type slot = get_armour_slot((armour_type)type);

    // Currently being too restrictive results in asserts, being too
    // permissive will generate such items on "any armour ego:XXX".
    // The latter is definitely so much safer -- 1KB
    switch ((special_armour_type)brand)
    {
    case SPARM_FORBID_EGO:
    case SPARM_NORMAL:
        return true;

    case SPARM_INSULATION:
        if (slot == EQ_GLOVES)
            return true;
        // deliberate fall-through
    case SPARM_RUNNING:
    case SPARM_STEALTH:
    case SPARM_STURDY:
        return slot == EQ_BOOTS || slot == EQ_BARDING;

    case SPARM_ARCHMAGI:
    case SPARM_HIGH_PRIEST:
        return !strict || type == ARM_ROBE
            || type == ARM_PLATE_ARMOUR;

    case SPARM_PONDEROUSNESS:
        return true;
#if TAG_MAJOR_VERSION == 34
    case SPARM_PRESERVATION:
        if (type == ARM_PLATE_ARMOUR && !strict)
            return true;
        // deliberate fall-through
#endif
    case SPARM_SOFT:
        if (type == ARM_ROBE)
            return true;
        // deliberate fall-through

    case SPARM_INVISIBILITY:
        return slot == EQ_CLOAK;

    case SPARM_STRENGTH:
    case SPARM_DEXTERITY:
        if (!strict)
            return true;
        // deliberate fall-through
    case SPARM_ARCHERY:
    case SPARM_WIELDING:
        return slot == EQ_GLOVES;

    case SPARM_IMPROVED_VISION:
    case SPARM_INTELLIGENCE:
        return slot == EQ_HELMET;

    case SPARM_FIRE_RESISTANCE:
    case SPARM_COLD_RESISTANCE:
    case SPARM_RESISTANCE:
        if (type == ARM_FIRE_DRAGON_ARMOUR
            || type == ARM_ICE_DRAGON_ARMOUR
            || type == ARM_GOLD_DRAGON_ARMOUR
            || type == ARM_SALAMANDER_HIDE_ARMOUR)
        {
            return false; // contradictory or redundant
        }
        return true; // in portal vaults, these can happen on every slot

    case SPARM_MAGIC_RESISTANCE:
        if (type == ARM_HAT || type == ARM_CAP)
            return true;
        // deliberate fall-through
    case SPARM_POISON_RESISTANCE:
    case SPARM_POSITIVE_ENERGY:
        if (type == ARM_PEARL_DRAGON_ARMOUR && brand == SPARM_POSITIVE_ENERGY)
            return false; // contradictory or redundant

        return slot == EQ_BODY_ARMOUR || slot == EQ_CLOAK || !strict;

    case SPARM_SPIRIT_SHIELD:
        return
#if TAG_MAJOR_VERSION == 34
               type == ARM_HAT ||
#endif
               type == ARM_SCARF || !strict;

    case SPARM_REPULSION:
    case SPARM_CLOUD_IMMUNE:
        return type == ARM_SCARF;

    default:
        return false; // Shouldn't happen; but we come here if a shield-unique brand ends up on normal armour.

    case NUM_SPECIAL_ARMOURS:
    case NUM_REAL_SPECIAL_ARMOURS:
        die("invalid armour brand");
    }


    return true;
}

/**
 * Return the number of plusses required for a type of armour to be notable.
 * (From plus alone.)
 *
 * @param armour_type   The type of armour being considered.
 * @return              The armour plus value required to be interesting.
 */
static int _armour_plus_threshold(equipment_type armour_type)
{
    switch (armour_type)
    {
        // body armour is very common; squelch most of it
        case EQ_BODY_ARMOUR:
            return 3;
        // aux armour is relatively uncommon
        default:
            return 1;
    }
}

/**
 * Pick an armour type (ex. plate armour), based on item_level
 *
 * @param item_level The rough power level of the item.
 *
 * @return The selected armour type.
 */
static armour_type _get_random_armour_type(int item_level)
{

    // Dummy value for initilization, always changed by the conditional
    // (and not changing it would trigger an ASSERT)
    armour_type armtype = NUM_ARMOURS;
    bool christmas = is_christmas();

    // Secondary armours.
    if (one_chance_in(5))
    {
        // Total weight is 48, each slot has a weight of 12
        armtype = random_choose_weighted(12, ARM_BOOTS,
                                         // Glove slot
                                         10, ARM_GLOVES,
                                          2, ARM_CLAW,
                                         // Cloak slot
                                         9, ARM_CLOAK,
                                         3, ARM_SCARF,
                                         // Head slot
                                         christmas ? 8 : 10, ARM_HELMET,
                                         christmas ? 3 :  0, ARM_CAP,
                                         christmas ? 1 :  2, ARM_HAT);
    }
    else if (x_chance_in_y(11 + item_level, 10000))
    {
        // High level dragon scales
        armtype = random_choose(ARM_STEAM_DRAGON_ARMOUR,
                                ARM_ACID_DRAGON_ARMOUR,
                                ARM_STORM_DRAGON_ARMOUR,
                                ARM_GOLD_DRAGON_ARMOUR,
                                ARM_SWAMP_DRAGON_ARMOUR,
                                ARM_PEARL_DRAGON_ARMOUR,
                                ARM_SHADOW_DRAGON_ARMOUR,
                                ARM_QUICKSILVER_DRAGON_ARMOUR);
    }
    else if (x_chance_in_y(11 + item_level, 8000))
    {
        // Crystal plate, some armours which are normally gained by butchering
        // monsters for hides.
        armtype = random_choose(ARM_CRYSTAL_PLATE_ARMOUR,
                                ARM_TROLL_LEATHER_ARMOUR,
                                ARM_IRON_TROLL_LEATHER_ARMOUR,
                                ARM_DEEP_TROLL_LEATHER_ARMOUR,
                                ARM_FIRE_DRAGON_ARMOUR,
                                ARM_ICE_DRAGON_ARMOUR,
                                ARM_SALAMANDER_HIDE_ARMOUR);

    }
    else if (x_chance_in_y(11 + item_level, 60))
    {
        // All the "mundane" armours. Generally the player will find at least
        // one copy of these by the Lair.
        armtype = random_choose(ARM_ROBE,
                                ARM_LEATHER_ARMOUR,
                                ARM_RING_MAIL,
                                ARM_SCALE_MAIL,
                                ARM_CHAIN_MAIL,
                                ARM_PLATE_ARMOUR);
    }
    else if (x_chance_in_y(11 + item_level, 35))
    {
        // All the "mundane" amours except plate.
        armtype = random_choose(ARM_ROBE,
                                ARM_LEATHER_ARMOUR,
                                ARM_RING_MAIL,
                                ARM_SCALE_MAIL,
                                ARM_CHAIN_MAIL);
    }
    else
    {
        // Default (lowest-level) armours.
        armtype = random_choose(ARM_ROBE,
                                ARM_LEATHER_ARMOUR,
                                ARM_RING_MAIL);
    }

    ASSERT(armtype != NUM_ARMOURS);

    return armtype;
}

static void _generate_armour_item(item_def& item, bool allow_uniques,
                                  int force_type, int item_level,
                                  int agent = NO_AGENT)
{
    if (force_type != OBJ_RANDOM)
        item.sub_type = force_type;
    else
    {
        for (int i = 0; i < 1000; ++i)
        {
            item.sub_type = _get_random_armour_type(item_level);
            if (is_armour_brand_ok(item.sub_type, item.brand, true))
                break;
        }
    }

    // Fall back to an ordinary item if artefacts not allowed for this type.
    if (item_level == ISPEC_RANDART && _armour_disallows_randart(item.sub_type))
        item_level = ISPEC_GOOD_ITEM;

    // Forced randart.
    if (item_level == ISPEC_RANDART)
    {
        int ego = item.brand;
        for (int i = 0; i < 100; ++i)
            if (_try_make_armour_artefact(item, force_type, 0, true, agent)
                && is_artefact(item))
            {
                // borrowed from similar code for weapons -- is this really the
                // best way to force an ego??
                if (ego > SPARM_NORMAL)
                {
                    item.props[ARTEFACT_PROPS_KEY].get_vector()[ARTP_BRAND].get_short() = ego;
                    if (randart_is_bad(item)) // recheck, the brand changed
                    {
                        force_type = item.sub_type;
                        item.clear();
                        item.quantity = 1;
                        item.base_type = OBJ_ARMOURS;
                        item.sub_type = force_type;
                        continue;
                    }
                }
                return;
            }
        // fall back to an ordinary item
        item_level = ISPEC_GOOD_ITEM;
    }

    if (allow_uniques
        && _try_make_armour_artefact(item, force_type, item_level, false, agent))
    {
        return;
    }

    if (item.sub_type == ARM_BOOTS && (you.chapter != CHAPTER_NONDUNGEON_START))
    {
        if (one_chance_in(8))
            item.sub_type = ARM_NAGA_BARDING;
        else if (one_chance_in(7))
            item.sub_type = ARM_CENTAUR_BARDING;
    }

    const bool force_good = item_level >= ISPEC_GIFT;
    const bool forced_ego = (item.brand > 0);
    const bool no_ego     = (item.brand == SPARM_FORBID_EGO);

    if (no_ego)
        item.brand = SPARM_NORMAL;

    if (item_level < 0)
    {
        // Thoroughly damaged, could have been good once.
        if (!no_ego && (forced_ego || one_chance_in(4)))
            item.brand = _generate_armour_ego(item);
            // Brand is set as for "good" items.

        item.plus -= 1 + random2(3);

        if (item_level == ISPEC_BAD)
            do_curse_item(item);
    }
    // Scarves always get an ego.
    else if (item.sub_type == ARM_SCARF)
        item.brand = _generate_armour_ego(item);
    else if ((forced_ego || item.sub_type == ARM_HAT || item.sub_type == ARM_CAP
                    || x_chance_in_y(51 + item_level, 250))
                && !item.is_mundane() || force_good)
    {
        // Make a good item...
        item.plus += random2(3);

        if (item.sub_type <= ARM_PLATE_ARMOUR
            && x_chance_in_y(21 + item_level, 300))
        {
            item.plus += random2(3);
        }

        if (!no_ego && x_chance_in_y(31 + item_level, 350))
        {
            // ...an ego item, in fact.
            item.brand = _generate_armour_ego(item);

            if (get_armour_ego_type(item) == SPARM_PONDEROUSNESS)
                item.plus += 3 + random2(8);
        }
    }
    else if (one_chance_in(12))
    {
        // Make a bad (cursed) item.
        do_curse_item(item);

        if (one_chance_in(5))
            item.plus -= random2(3);

        item.brand = SPARM_NORMAL;
    }

    if (item.sub_type == ARM_CLAW && item.brand != SPARM_NORMAL)
        item.brand = determine_weapon_brand(item, item_level);

    // Don't overenchant items.
    if (item.plus > armour_max_enchant(item))
        item.plus = armour_max_enchant(item);

    // Don't let unenchantable armour get minuses.
    if (!armour_is_enchantable(item))
        item.plus = 0;

    // Never give brands to scales or hides, in case of misbehaving vaults.
    if (armour_type_is_hide(static_cast<armour_type>(item.sub_type)))
        item.brand = SPARM_NORMAL;

    // squash boring items.
    if (!force_good && item.brand == SPARM_NORMAL && item.plus > 0
        && item.plus < _armour_plus_threshold(get_armour_slot(item)))
    {
        item.plus = 0;
    }
}

static monster_type _choose_random_monster_corpse()
{
    for (int count = 0; count < 1000; ++count)
    {
        monster_type spc = mons_species(static_cast<monster_type>(
                                        random2(NUM_MONSTERS)));
        if (mons_class_flag(spc, M_NO_POLY_TO | M_CANT_SPAWN))
            continue;
        if (mons_class_can_leave_corpse(spc))
            return spc;
    }

    return MONS_RAT;          // if you can't find anything else...
}

/**
 * Choose a random wand subtype for ordinary wand generation.
 *
 * Some wands [mostly more powerful ones] are less common than others.
 * Attack wands are more common, mostly to preserve historical wand freqs.
 *
 * @return      A random wand_type.
 */
static int _random_wand_subtype()
{
    // total weight 74 [arbitrary]
    return random_choose_weighted(10, WAND_FLAME,
                                  10, WAND_ICEBLAST,
                                  8, WAND_RANDOM_EFFECTS,
                                  8, WAND_POLYMORPH,
                                  8, WAND_ACID,
                                  6, WAND_DISINTEGRATION,
                                  6, WAND_HASTING,
                                  5, WAND_ENSLAVEMENT,
                                  5, WAND_ENSNARE,
                                  3, WAND_CLOUDS,
                                  3, WAND_SCATTERSHOT,
                                  2, WAND_HEAL_WOUNDS);
}

/**
 * Should wands of this type NOT spawn extremely early on? (At very low
 * item_level, or in the hands of very low HD monsters?)
 *
 * @param type      The wand_type in question.
 * @return          Whether it'd be excessively mean for this wand to be used
 *                  against very early players.
 */
bool is_high_tier_wand(int type)
{
    switch (type)
    {
    case WAND_ENSLAVEMENT:
    case WAND_ACID:
    case WAND_ICEBLAST:
    case WAND_DISINTEGRATION:
    case WAND_CLOUDS:
    case WAND_SCATTERSHOT:
    case WAND_HASTING:
    case WAND_HEAL_WOUNDS:
        return true;
    default:
        return false;
    }
}

static void _generate_wand_item(item_def& item, int force_type, int item_level)
{
    if (force_type != OBJ_RANDOM)
        item.sub_type = force_type;
    else
        item.sub_type = _random_wand_subtype();

    // Add wand charges and ensure we have at least one charge.
    item.charges = 1 + random2avg(wand_charge_value(item.sub_type), 3);

    // Don't let monsters pickup early high-tier wands
    if (item_level < 2 && is_high_tier_wand(item.sub_type))
        item.flags |= ISFLAG_NO_PICKUP;
}

static void _generate_food_item(item_def& item, int force_quant, int force_type)
{
    // Determine sub_type:
    if (force_type == OBJ_RANDOM)
        item.sub_type = FOOD_RATION;
    else
        item.sub_type = force_type;

    // Happens with ghoul food acquirement -- use place_chunks() outherwise
    if (item.sub_type == FOOD_CHUNK)
    {
        // Set chunk flavour:
        item.plus = _choose_random_monster_corpse();
        item.orig_monnum = item.plus;
        // Set duration.
        item.freshness = (10 + random2(11)) * 10;
    }

    // Determine quantity.
    item.quantity = force_quant > 1 ? force_quant : 1;
}

static void _generate_potion_item(item_def& item, int force_type,
                                  int item_level, int agent)
{
    item.quantity = 1;

    if (one_chance_in(18))
        item.quantity++;

    if (one_chance_in(25))
        item.quantity++;

    if (force_type != OBJ_RANDOM)
        item.sub_type = force_type;
    else
    {
        int stype;
        int tries = 500;

        // If created by Xom, keep going until an approved potion is chosen
        // Currently does nothing, until we come up with a boring potion.
        do
        {
            // total weight: 906
            stype = random_choose_weighted(192, POT_CURING,
                                           105, POT_HEAL_WOUNDS,
                                            73, POT_LIGNIFY,
                                            73, POT_HASTE,
                                            66, POT_MUTATION,
                                            66, POT_MIGHT,
                                            66, POT_AGILITY,
                                            66, POT_BRILLIANCE,
                                            53, POT_DEGENERATION,
                                            46, POT_MUTATION,
                                            35, POT_INVISIBILITY,
                                            35, POT_RESISTANCE,
                                            35, POT_MAGIC,
                                            35, POT_BERSERK_RAGE,
                                            35, POT_CANCELLATION,
                                            35, POT_AMBROSIA,
                                            29, POT_CURE_MUTATION,
                                            11, POT_BENEFICIAL_MUTATION,
                                             2, POT_EXPERIENCE);
        }
        while (agent == GOD_XOM
               && _is_boring_item(OBJ_POTIONS, stype)
               && --tries > 0);

        item.sub_type = stype;
    }
    // don't let monsters pickup early dangerous potions
    if (item_level < 2 && item.sub_type == POT_BERSERK_RAGE)
        item.flags |= ISFLAG_NO_PICKUP;
}

static void _generate_scroll_item(item_def& item, int force_type,
                                  int item_level, int agent)
{
    // determine sub_type:
    if (force_type != OBJ_RANDOM)
        item.sub_type = force_type;
    else
    {
        int tries = 500;

        // If this item is created by Xom, keep looping until an
        // interesting scroll is discovered (as determined by
        // _is_boring_item). Otherwise just weighted-choose a scroll.
        do
        {
            // total weight:    574
            //                  474  in sprint
            item.sub_type = random_choose_weighted(
                112, SCR_RANDOM_USELESSNESS,
                 // [Cha] don't generate teleportation scrolls if in sprint
                100, (crawl_state.game_is_sprint() ? NUM_SCROLLS
                                                   : SCR_TELEPORTATION),
                 45, SCR_AMNESIA,
                 70, SCR_ENCHANT,
                 40, SCR_MAGIC_MAPPING,
                 32, SCR_FEAR,
                 32, SCR_FOG,
                 32, SCR_BLINKING,
                 32, SCR_IMMOLATION,
                 // Higher-level scrolls.
                 36, SCR_BLESS_ITEM,
                 27, SCR_VULNERABILITY,
                 17, SCR_SUMMONING,
                 15, SCR_ACQUIREMENT,
                 15, SCR_NOISE,
                 15, SCR_SILENCE,
                 15, SCR_TORMENT,
                 15, SCR_HOLY_WORD);
        }
        while (item.sub_type == NUM_SCROLLS
               || agent == GOD_XOM
                  && _is_boring_item(OBJ_SCROLLS, item.sub_type)
                  && --tries > 0);
    }

    item.quantity = random_choose_weighted(46, 1,
                                            1, 2,
                                            1, 3);

    item.plus = 0;
}

/// Choose a random spellbook type for the given level.
static book_type _choose_book_type(int item_level)
{
    vector<pair<book_type, int>> weights;

    for (int i = 0; i < NUM_FIXED_BOOKS; i++)
    {
        const book_type book = static_cast<book_type>(i);
        if (item_type_removed(OBJ_BOOKS, book))
            continue;

        const int rarity = book_rarity(book);
        ASSERT(rarity != 100);

        const int weight = 100 - book_rarity(book) + item_level;

        const pair<book_type, int> weight_pair = { book, weight };
        weights.push_back(weight_pair);
    }

    const book_type book = *random_choose_weighted(weights);
    return book;
}

/// Choose a random skill for a manual to be generated for.
static skill_type _choose_manual_skill(bool force_magic)
{
    // spell skill (or invo/evo)
    if (one_chance_in(3) || force_magic)
    {
        skill_type skill = SK_CONJURATIONS;
        while (skill == SK_CONJURATIONS)
            skill = static_cast<skill_type>(
                SK_SPELLCASTING + random2(NUM_SKILLS - SK_SPELLCASTING));
        return skill;
    }

    // mundane skill
#if TAG_MAJOR_VERSION == 34
    skill_type skill = SK_TRAPS;
    while (skill == SK_TRAPS || skill == SK_STABBING || skill == SK_THROWING)
        skill = static_cast<skill_type>(random2(SK_LAST_MUNDANE+1));
    return skill;
#else
    return static_cast<skill_type>(random2(SK_LAST_MUNDANE + 1));
#endif
}

static void _generate_manual_item(item_def& item, bool allow_uniques,
                                  int force_type, int agent)
{
    if (force_type != OBJ_RANDOM)
        item.sub_type = force_type;

    else
    {
        // BCADDO: Artefact chance later.
        item.sub_type = random_choose_weighted(8, MAN_SMALL, 
                                               3, MAN_NORMAL, 
                                               1, MAN_LARGE);
    }
     
    item.skill = _choose_manual_skill(agent == GOD_SIF_MUNA);
    
    // Set number of bonus skill points.
    item.skill_points = random_range(2500, 3000);
    if (item.sub_type == MAN_SMALL)
        item.skill_points /= 8;
    else if (item.sub_type == MAN_LARGE)
        item.skill_points *= 3;

    // Preidentify.
    set_ident_type(item, true);
    set_ident_flags(item, ISFLAG_IDENT_MASK);
    return;
}

static void _generate_book_item(item_def& item, bool allow_uniques,
                                int force_type, int item_level)
{
    if (force_type != OBJ_RANDOM)
        item.sub_type = force_type;
    else
        item.sub_type = _choose_book_type(item_level);

    // Only randomly generate randart books for OBJ_RANDOM, since randart
    // spellbooks aren't merely of-the-same-type-but-better, but
    // have an entirely different set of spells.
    if (allow_uniques && item_level > 2 && force_type == OBJ_RANDOM
        && x_chance_in_y(101 + item_level * 3, 4000))
    {
        int choice = random_choose_weighted(
            29, BOOK_RANDART_THEME,
             1, BOOK_RANDART_LEVEL);

        item.sub_type = choice;
    }

    if (item.sub_type == BOOK_RANDART_THEME)
        build_themed_book(item, capped_spell_filter(20));
    else if (item.sub_type == BOOK_RANDART_LEVEL)
    {
        int max_level  = min(9, max(1, item_level / 3));
        int spl_level  = random_range(1, max_level);
        make_book_level_randart(item, spl_level);
    }
}

static void _generate_staff_item(item_def& item, bool allow_uniques,
                                 int force_type, int item_level, int agent, int force_ego)
{
    if (force_type == OBJ_RANDOM)
    {
        do
        {
            item.sub_type = random2(NUM_STAVES);
        }
        while (item_type_removed(OBJ_STAVES, item.sub_type));
    } 
    else
        item.sub_type = force_type;

    if (item_level == ISPEC_RANDART || (allow_uniques && one_chance_in(5)))
    {
        for (int i = 0; i < 100; ++i)
            if (_try_make_staff_artefact(item, force_type, item_level, true, agent)
                && is_artefact(item))
            {
                return;
            }
        // fall back to an ordinary item
        item_level = ISPEC_GOOD_ITEM;
    }

    ASSERT(!is_artefact(item));

    item.plus = -6;
    item.plus += roll_dice(2, 4);

    if (item_level >= ISPEC_GIFT)
        item.plus += roll_dice(2, 4);

    if (force_ego != 0)
        item.brand = force_ego;
    else
        item.brand = generate_staff_facet(item, item_level);

    if (!is_staff_brand_ok(item.sub_type, item.brand))
        item.brand = generate_staff_facet(item, item_level);
}

static void _generate_rune_item(item_def& item, int force_type)
{
    if (force_type == OBJ_RANDOM)
    {
        vector<int> possibles;
        for (int i = 0; i < NUM_RUNE_TYPES; i++)
            if (!item_type_removed(OBJ_RUNES, i) && !you.runes[i])
                possibles.push_back(i);

        item.sub_type = possibles.empty()
                      ? RUNE_DEMONIC
                      : *random_iterator(possibles);
    }
    else
        item.sub_type = force_type;
}

static bool _try_make_jewellery_unrandart(item_def& item, int force_type,
                                          int item_level, int agent)
{
    int type = (force_type == NUM_RINGS)     ? get_random_ring_type() :
               (force_type == NUM_JEWELLERY) ? get_random_amulet_type()
                                             : force_type;
    if (item_level > 2
        && one_chance_in(20)
        && x_chance_in_y(101 + item_level * 3, 2000))
    {
        if (_try_make_item_unrand(item, type, agent))
            return true;
    }

    return false;
}

static void _generate_jewellery_item(item_def& item, bool allow_uniques,
                                     int force_type, int item_level,
                                     int agent)
{
    if (allow_uniques && item_level != ISPEC_RANDART
        && _try_make_jewellery_unrandart(item, force_type, item_level, agent))
    {
        return;
    }

    // Determine subtype.
    // Note: removed double probability reduction for some subtypes
    if (force_type != OBJ_RANDOM
        && force_type != NUM_RINGS
        && force_type != NUM_JEWELLERY)
    {
        item.sub_type = force_type;
    }
    else
    {
        int tries = 500;
        do
        {
            if (force_type == NUM_RINGS)
                item.sub_type = get_random_ring_type();
            else if (force_type == NUM_JEWELLERY)
                item.sub_type = get_random_amulet_type();
            else
                item.sub_type = (one_chance_in(4) ? get_random_amulet_type()
                                                  : get_random_ring_type());
        }
        while (agent == GOD_XOM
               && _is_boring_item(OBJ_JEWELLERY, item.sub_type)
               && --tries > 0);
    }

    // All jewellery base types should now work. - bwr
    if (item_level == ISPEC_RANDART
        || allow_uniques && item_level > 2
        && x_chance_in_y(101 + item_level * 3, 4000))
    {
        make_item_randart(item);
        if (coinflip())
            do_curse_item(item);
    }
    else if (item.sub_type == RING_ATTENTION
        || item.sub_type == RING_TELEPORTATION
        || item.sub_type == AMU_INACCURACY
        || one_chance_in(50))
    {
        // Bad jewellery is always cursed {dlb}:
        do_curse_item(item);
    }
    else if (one_chance_in(3))
        do_curse_item(item);
}

static void _generate_misc_item(item_def& item, int force_type, int force_ego)
{
    if (force_type != OBJ_RANDOM)
        item.sub_type = force_type;
    else
    {
        item.sub_type = random_choose(MISC_FAN_OF_GALES,
                                      MISC_LAMP_OF_FIRE,
                                      MISC_PHIAL_OF_FLOODS,
                                      MISC_LIGHTNING_ROD,
                                      MISC_BOX_OF_BEASTS,
                                      MISC_SACK_OF_SPIDERS,
                                      MISC_CRYSTAL_BALL_OF_ENERGY,
                                      MISC_LANTERN_OF_SHADOWS,
                                      MISC_PHANTOM_MIRROR);
    }

    if (is_deck(item))
    {
        item.initial_cards = random_range(MIN_STARTING_CARDS,
                                          MAX_STARTING_CARDS);

        if (force_ego >= DECK_RARITY_COMMON
            && force_ego <= DECK_RARITY_LEGENDARY)
        {
            item.deck_rarity = static_cast<deck_rarity_type>(force_ego);
        }
        else
        {
            item.deck_rarity = random_choose_weighted(8, DECK_RARITY_LEGENDARY,
                                                     20, DECK_RARITY_RARE,
                                                     72, DECK_RARITY_COMMON);
        }
        init_deck(item);
    }
}

/**
 * Alter the inputed item to have no "plusses" (mostly weapon/armour enchantment)
 *
 * @param[in,out] item_slot The item slot of the item to remove "plusses" from.
 */
void squash_plusses(int item_slot)
{
    item_def& item(mitm[item_slot]);

    ASSERT(!is_deck(item));
    item.plus         = 0;
    item.used_count   = 0;
    item.brand        = 0;
    set_equip_desc(item, ISFLAG_NO_DESC);
}

static bool _ego_unrand_only(int base_type, int ego)
{
    if (base_type == OBJ_WEAPONS)
    {
        switch (static_cast<brand_type>(ego))
        {
        case SPWPN_REAPING:
        case SPWPN_ACID:
            return true;
        default:
            return false;
        }
    }
    // all armours are ok?
    return false;
}

// Make a corresponding randart instead as a fallback.
// Attempts to use base type, sub type, and ego (for armour/weapons).
// Artefacts can explicitly specify a base type and fallback type.

// Could be even more flexible: maybe allow an item spec to describe
// complex fallbacks?
static void _setup_fallback_randart(const int unrand_id,
                                    item_def &item,
                                    int &force_type,
                                    int &item_level)
{
    ASSERT(get_unrand_entry(unrand_id));
    const unrandart_entry &unrand = *get_unrand_entry(unrand_id);
    dprf("Placing fallback randart for %s", unrand.name);

    uint8_t fallback_sub_type;
    if (unrand.fallback_base_type != OBJ_UNASSIGNED)
    {
        item.base_type = unrand.fallback_base_type;
        fallback_sub_type = unrand.fallback_sub_type;
    }
    else
    {
        item.base_type = unrand.base_type;
        fallback_sub_type = unrand.sub_type;
    }

    if (item.base_type == OBJ_STAVES
        && fallback_sub_type == STAFF_NOTHING)
    {
        force_type = NUM_STAVES;
    }
    else if (item.base_type == OBJ_JEWELLERY
             && fallback_sub_type == AMU_NOTHING)
    {
        force_type = NUM_JEWELLERY;
    }
    else
        force_type = fallback_sub_type;

    // import some brand information from the unrand. TODO: I'm doing this for
    // the sake of vault designers who make themed vaults. But it also often
    // makes the item more redundant with the unrand; maybe add a bit more
    // flexibility in item specs so that this part is optional? However, from a
    // seeding perspective, it also makes sense if the item generated is
    // very comparable.

    // unset fallback_brand is -1. In that case it will try to use the regular
    // brand if there is one. If the end result here is 0, the brand (for
    // weapons) will be chosen randomly. So to force randomness, use
    // SPWPN_NORMAL on the fallback ego, or set neither. (Randarts cannot be
    // unbranded.)
    item.brand = unrand.fallback_brand; // no type checking here...

    if (item.brand < 0
        && !_ego_unrand_only(item.base_type, unrand.prpty[ARTP_BRAND])
        && item.base_type == unrand.base_type // brand isn't well-defined for != case
        && ((item.base_type == OBJ_WEAPONS
             && is_weapon_brand_ok(item.sub_type, unrand.prpty[ARTP_BRAND], true))
            || (item.base_type == OBJ_ARMOURS
             && is_armour_brand_ok(item.sub_type, unrand.prpty[ARTP_BRAND], true))))
    {
        // maybe do jewellery too?
        item.brand = unrand.prpty[ARTP_BRAND];
    }
    if (item.brand < 0)
        item.brand = 0;

    item_level = ISPEC_RANDART;
}

/**
 * Create an item.
 *
 * Various parameters determine whether the item can be an artifact, set the
 * item class (ex. weapon, wand), set the item subtype (ex.
 * hatchet, wand of flame), set the item ego (ex. of flaming, of running), set
 * the rough power level of the item, and set the agent of the item (which
 * affects what artefacts can be generated, and also non-artefact items if the
 * agent is Xom). Item class, Item type, and Item ego can also be randomly
 * selected (by setting those parameters to OBJ_RANDOM, OBJ_RANDOM, and 0
 * respectively).
 *
 * @param allow_uniques Can the item generated be an artefact?
 * @param force_class The desired OBJECTS class (Example: OBJ_ARMOURS)
 * @param force_type The desired SUBTYPE - enum varies by OBJ
 * @param item_level How powerful the item is allowed to be
 * @param force_ego The desired ego/brand
 * @param agent The agent creating the item (Example: Xom) or -1 if NA
 *
 * @return The generated item's item slot or NON_ITEM if it fails.
 */
int items(bool allow_uniques,
          object_class_type force_class,
          int force_type,
          int item_level,
          int force_ego,
          int agent)
{
    rng::subgenerator item_rng;

    ASSERT(force_ego <= 0
           || force_class == OBJ_WEAPONS
           || force_class == OBJ_ARMOURS
           || force_class == OBJ_SHIELDS
           || force_class == OBJ_MISSILES
           || force_class == OBJ_STAVES
           || force_class == OBJ_MISCELLANY && is_deck_type(force_type));

    // Find an empty slot for the item (with culling if required).
    int p = get_mitm_slot(10);
    if (p == NON_ITEM)
        return NON_ITEM;

    item_def& item(mitm[p]);

    const bool force_good = item_level >= ISPEC_GIFT;

    item.brand = force_ego;

    if (force_ego != 0)
        allow_uniques = false;

    // cap item_level unless an acquirement-level item {dlb}:
    if (item_level > 50 && !force_good)
        item_level = 50;

    // determine base_type for item generated {dlb}:
    if (force_class != OBJ_RANDOM)
    {
        ASSERT(force_class < NUM_OBJECT_CLASSES);
        item.base_type = force_class;
    }
    else
    {
        ASSERT(force_type == OBJ_RANDOM);
        // Total weight: 1765
        item.base_type = random_choose_weighted(
                                     5, OBJ_MANUALS,
                                    10, OBJ_STAVES,
                                    30, OBJ_BOOKS,
                                    45, OBJ_JEWELLERY,
                                    45, OBJ_SHIELDS,
                                    70, OBJ_WANDS,
                                   140, OBJ_FOOD,
                                   212, OBJ_ARMOURS,
                                   212, OBJ_WEAPONS,
                                   166, OBJ_POTIONS,
                                   280, OBJ_SCROLLS,
                                   450, OBJ_GOLD);

        // misc items placement wholly dependent upon current depth {dlb}:
        if (x_chance_in_y(21 + item_level, 5000))
            item.base_type = OBJ_MISCELLANY;

        if (item_level < 7
            && (item.base_type == OBJ_BOOKS
                || item.base_type == OBJ_STAVES
                || item.base_type == OBJ_WANDS)
            && random2(7) >= item_level)
        {
            item.base_type = random_choose(OBJ_POTIONS, OBJ_SCROLLS);
        }
    }

    ASSERT(force_type == OBJ_RANDOM
           || item.base_type == OBJ_JEWELLERY && force_type == NUM_JEWELLERY
           || force_type < get_max_subtype(item.base_type));

    // make_item_randart() might do things differently based upon the
    // acquirement agent, especially for god gifts.
    if (agent != NO_AGENT && !is_stackable_item(item))
        origin_acquired(item, agent);

    item.quantity = 1;          // generally the case

    if (force_ego < SP_FORBID_EGO)
    {
        const int unrand_id = -force_ego;
        if (get_unique_item_status(unrand_id) == UNIQ_NOT_EXISTS)
        {
            make_item_unrandart(mitm[p], unrand_id);
            ASSERT(mitm[p].is_valid());
            return p;
        }

        _setup_fallback_randart(unrand_id, item, force_type, item_level);
        allow_uniques = false;
    }

    // Determine sub_type accordingly. {dlb}
    switch (item.base_type)
    {
    case OBJ_WEAPONS:
        _generate_weapon_item(item, allow_uniques, force_type,
                              item_level, agent);
        break;

    case OBJ_SHIELDS:
        _generate_shield_item(item, allow_uniques, force_type,
                              item_level, agent);
        break;

    case OBJ_MISSILES:
        _generate_missile_item(item, force_type, item_level);
        break;

    case OBJ_ARMOURS:
        _generate_armour_item(item, allow_uniques, force_type, item_level,
                              agent);
        break;

    case OBJ_WANDS:
        _generate_wand_item(item, force_type, item_level);
        break;

    case OBJ_FOOD:
        _generate_food_item(item, allow_uniques, force_type);
        break;

    case OBJ_POTIONS:
        _generate_potion_item(item, force_type, item_level, agent);
        break;

    case OBJ_SCROLLS:
        _generate_scroll_item(item, force_type, item_level, agent);
        break;

    case OBJ_JEWELLERY:
        _generate_jewellery_item(item, allow_uniques, force_type, item_level, agent);
        break;

    case OBJ_MANUALS:
        _generate_manual_item(item, allow_uniques, force_type, agent);
        break;

    case OBJ_BOOKS:
        _generate_book_item(item, allow_uniques, force_type, item_level);
        break;

    case OBJ_STAVES:
        _generate_staff_item(item, allow_uniques, force_type, item_level, agent, force_ego);
        break;

    case OBJ_ORBS:              // always forced in current setup {dlb}
    case OBJ_RUNES:
        _generate_rune_item(item, force_type);
        break;

    case OBJ_MISCELLANY:
        _generate_misc_item(item, force_type, force_ego);
        break;

    // that is, everything turns to gold if not enumerated above, so ... {dlb}
    default:
        item.base_type = OBJ_GOLD;
        if (force_good)
            item.quantity = 100 + random2(400);
        else
        {
            item.quantity = 1 + random2avg(19, 2);
            item.quantity += random2(item_level);
        }
        break;
    }

    if (item.base_type == OBJ_WEAPONS
          && !is_weapon_brand_ok(item.sub_type, get_weapon_brand(item), false)
        || item.base_type == OBJ_ARMOURS
          && !is_armour_brand_ok(item.sub_type, get_armour_ego_type(item), false)
        || item.base_type == OBJ_MISSILES
          && !is_missile_brand_ok(item.sub_type, item.brand, false)
        || item.base_type == OBJ_STAVES
          && !is_staff_brand_ok(item.sub_type, get_staff_facet(item), false))
    {
        mprf(MSGCH_ERROR, "Invalid brand on item %s, annulling.",
            item.name(DESC_PLAIN, false, true, false, false, ISFLAG_KNOW_PLUSES
                      | ISFLAG_KNOW_CURSE).c_str());
        item.brand = 0;
    }

    // Colour the item.
    item_colour(item);

    // Set brand appearance.
    item_set_appearance(item);

    // Don't place the item just yet.
    item.pos.reset();
    item.link = NON_ITEM;

    // Note that item might be invalidated now, since p could have changed.
    ASSERTM(mitm[p].is_valid(),
            "idx: %d, qty: %hd, base: %d, sub: %d, spe: %d, col: %d, rnd: %d",
            item.index(), item.quantity,
            (int)item.base_type, (int)item.sub_type, item.special,
            (int)item.get_colour(), (int)item.rnd);
    return p;
}

void reroll_brand(item_def &item, int item_level)
{
    ASSERT(!is_artefact(item));
    switch (item.base_type)
    {
    case OBJ_WEAPONS:
        item.brand = determine_weapon_brand(item, item_level);
        break;
    case OBJ_MISSILES:
        item.brand = _determine_missile_brand(item, item_level);
        break;
    case OBJ_ARMOURS:
        item.brand = _generate_armour_ego(item);
        break;
    case OBJ_STAVES:
        item.brand = generate_staff_facet(item, item_level);
        break;
    default:
        die("can't reroll brands of this type");
    }
    item_set_appearance(item);
}

static bool _weapon_is_visibly_special(const item_def &item)
{
    const int brand = get_weapon_brand(item);
    const bool visibly_branded = (brand != SPWPN_NORMAL);

    if (get_equip_desc(item) != ISFLAG_NO_DESC)
        return false;

    if (visibly_branded || is_artefact(item) || item.plus > 0)
        return true;

    if (item.is_mundane())
        return false;

    if (item.flags & ISFLAG_CURSED && one_chance_in(3))
        return true;

    return false;
}

static bool _armour_is_visibly_special(const item_def &item)
{
    const int brand = get_armour_ego_type(item);
    const bool visibly_branded = (brand != SPARM_NORMAL);

    if (get_equip_desc(item) != ISFLAG_NO_DESC)
        return false;

    if (visibly_branded || is_artefact(item) || item.plus > 0)
        return true;

    if (item.is_mundane())
        return false;

    if (item.flags & ISFLAG_CURSED && one_chance_in(3))
        return true;

    return false;
}

static bool _shield_is_visibly_special(const item_def &item)
{
    if (is_hybrid(item.sub_type))
        return _weapon_is_visibly_special(item);

    else
        return _armour_is_visibly_special(item);

    return false;
}

jewellery_type get_random_amulet_type()
{
    int res;
    do
    {
        res = random_range(AMU_FIRST_AMULET, NUM_JEWELLERY - 1);
    }
    // Do not generate removed item types.
    while (item_type_removed(OBJ_JEWELLERY, res));

    return jewellery_type(res);
}

static jewellery_type _get_raw_random_ring_type()
{
    jewellery_type ring;
    do
    {
        ring = (jewellery_type)(random_range(RING_FIRST_RING, NUM_RINGS - 1));
    }
    while (ring == RING_TELEPORTATION && crawl_state.game_is_sprint()
           || item_type_removed(OBJ_JEWELLERY, ring));
    return ring;
}

jewellery_type get_random_ring_type()
{
    const jewellery_type j = _get_raw_random_ring_type();
    // Adjusted distribution here. - bwr
    if (j == RING_SLAYING && !one_chance_in(4))
        return _get_raw_random_ring_type();

    return j;
}

// Sets item appearance to match brands, if any.
void item_set_appearance(item_def &item)
{
    // Artefact appearance overrides cosmetic flags anyway.
    if (is_artefact(item))
        return;

    if (get_equip_desc(item) != ISFLAG_NO_DESC)
        return;

    switch (item.base_type)
    {
    case OBJ_WEAPONS:
        if (_weapon_is_visibly_special(item))
            set_equip_desc(item, random_choose(ISFLAG_GLOWING, ISFLAG_RUNED));
        break;

    case OBJ_SHIELDS:
        if (_shield_is_visibly_special(item))
            set_equip_desc(item, ISFLAG_RUNED);
        break;

    case OBJ_ARMOURS:
        if (_armour_is_visibly_special(item))
        {
            set_equip_desc(item, random_choose(ISFLAG_GLOWING, ISFLAG_RUNED,
                                               ISFLAG_EMBROIDERED_SHINY));
        }
        break;

    default:
        break;
    }
}

#if defined(DEBUG_DIAGNOSTICS) || defined(DEBUG_TESTS)
static int _test_item_level()
{
    switch (random2(10))
    {
    case 0:
        return ISPEC_GOOD_ITEM;
    case 1:
        return ISPEC_DAMAGED;
    case 2:
        return ISPEC_BAD;
    case 3:
        return ISPEC_RANDART;
    default:
        return random2(50);
    }
}

void makeitem_tests()
{
    int i, level;
    item_def item;

    mpr("Running generate_weapon_item tests.");
    for (i = 0; i < 10000; ++i)
    {
        item.clear();
        level = _test_item_level();
        item.base_type = OBJ_WEAPONS;
        item.brand = coinflip() ? SPWPN_NORMAL
                                : random2(NUM_REAL_SPECIAL_WEAPONS);
#if TAG_MAJOR_VERSION == 34
        if (item.special == SPWPN_ORC_SLAYING
            || item.special == SPWPN_REACHING
            || item.special == SPWPN_RETURNING
            || item.special == SPWPN_CONFUSE)
        {
            item.brand = SPWPN_FORBID_BRAND;
        }
#endif
        auto weap_type = coinflip() ? OBJ_RANDOM : random2(NUM_WEAPONS);
        _generate_weapon_item(item,
                              coinflip(),
                              weap_type,
                              level);
    }

    mpr("Running generate_armour_item tests.");
    for (i = 0; i < 10000; ++i)
    {
        item.clear();
        level = _test_item_level();
        item.base_type = OBJ_ARMOURS;
        item.brand = coinflip() ? SPARM_NORMAL
                                : random2(NUM_REAL_SPECIAL_ARMOURS);
        int type = coinflip() ? OBJ_RANDOM : random2(NUM_ARMOURS);
#if TAG_MAJOR_VERSION == 34
        if (type == ARM_BUCKLER || type == ARM_SHIELD || type == ARM_LARGE_SHIELD)
            type = ARM_ROBE;
#endif
        _generate_armour_item(item,
                              coinflip(),
                              type,
                              level);
    }
}
#endif
