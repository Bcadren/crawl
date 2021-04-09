/**
 * @file
 * @brief Throwing and launching stuff.
**/

#include "AppHdr.h"

#include "throw.h"

#include <cmath>
#include <sstream>

#include "art-enum.h"
#include "artefact.h"
#include "chardump.h"
#include "command.h"
#include "coordit.h"
#include "directn.h"
#include "english.h"
#include "env.h"
#include "exercise.h"
#include "fight.h"
#include "god-conduct.h"
#include "god-passive.h" // passive_t::shadow_attacks
#include "hints.h"
#include "invent.h"
#include "item-prop.h"
#include "item-status-flag-type.h"
#include "items.h"
#include "item-use.h"
#include "macro.h"
#include "makeitem.h"
#include "message.h"
#include "mon-behv.h"
#include "output.h"
#include "prompt.h"
#include "religion.h"
#include "rot.h"
#include "shout.h"
#include "showsymb.h"
#include "skills.h"
#include "sound.h"
#include "state.h"
#include "stringutil.h"
#include "terrain.h"
#include "transform.h"
#include "traps.h"
#include "viewchar.h"
#include "view.h"

static int  _fire_prompt_for_item();
static bool _fire_validate_item(int selected, string& err);
// static int  _get_blowgun_chance(const int hd);

// BCADNOTE: Preserving unused attacker parameter in case of future self-enchantment
// for force penetration; etc.
bool is_penetrating_attack(const actor& attacker, const item_def* weapon,
                           const item_def& projectile)
{
    if (attacker.is_player())
    {
        const item_def * inside = you.slot_item(EQ_CYTOPLASM);
        if (get_weapon_brand(*inside) == SPWPN_PENETRATION || is_unrandom_artefact(*inside, UNRAND_STORM_BOW))
            return true;
    }

    return projectile.base_type == OBJ_MISSILES
            && (get_ammo_brand(projectile) == SPMSL_PENETRATION || projectile.sub_type == MI_BOLT)
           || weapon && projectile.base_type == OBJ_MISSILES && ammo_type_damage(projectile.sub_type) == 0
              && (get_weapon_brand(*weapon) == SPWPN_PENETRATION
                  || is_unrandom_artefact(*weapon, UNRAND_STORM_BOW));
}

bool item_is_quivered(const item_def &item)
{
    return in_inventory(item) && item.link == you.m_quiver.get_fire_item();
}

int get_next_fire_item(int current, int direction)
{
    vector<int> fire_order;
    you.m_quiver.get_fire_order(fire_order, true);

    if (fire_order.empty())
        return -1;

    int next = direction > 0 ? 0 : -1;
    for (unsigned i = 0; i < fire_order.size(); i++)
    {
        if (fire_order[i] == current)
        {
            next = i + direction;
            break;
        }
    }

    next = (next + fire_order.size()) % fire_order.size();
    return fire_order[next];
}

/**
 *  Chance for a needle fired by the player to affect a monster of a particular
 *  hit dice, given the player's throwing skill and blowgun enchantment.
 *
 *    @param hd     The monster's hit dice.
 *    @return       The percentage chance for the player to affect the monster,
 *                  rounded down.
 *
 *  This chance is rolled in ranged_attack::blowgun_check using this formula for
 *  success:
 *      if hd < 15, fixed 3% chance to succeed regardless of roll
 *      else, or if the 3% chance fails,
 *                      succeed if 2 + random2(4 + skill + enchantment) >= hd
 *
 *   BCADDO: Restore this function (though in a different location in the code)
 *   when adding old blowgun effects as spells.
static int _get_blowgun_chance(const int hd)
{
    const int plus = you.weapon()->plus;
    const int skill = you.skill_rdiv(SK_SLINGS);

    int chance = 10000 - 10000 * (hd - 2) / (4 + skill + plus);
    chance = min(max(chance, 0), 10000);
    if (hd < 15)
    {
        chance *= 97;
        chance /= 100;
        chance += 300; // 3% chance to ignore HD and affect enemy anyway
    }
    return chance / 100;
} */

/**
 *  Validate any item selected to be fired, and choose a target to fire at.
 *
 *  @param slot         The slot the item to be fired is in, or -1 if
 *                      an item has not yet been chosen.
 *  @param target       An empty variable of the dist class to store the
 *                      target information in.
 *  @param teleport     Does the player have portal projectile active?
 *  @return             Whether the item validation and target selection
 *                      was successful.
 */
static bool _fire_choose_target(int slot, dist& target,
                                         bool teleport = false)
{
    const bool was_chosen = (slot != -1);

    if (was_chosen)
    {
        string warn;
        if (!_fire_validate_item(slot, warn))
        {
            mpr(warn);
            return false;
        }
    }

    direction_chooser_args args;
    args.mode = TARG_HOSTILE;
    args.self = confirm_prompt_type::cancel;
    args.needs_path = !teleport;
    args.top_prompt = make_stringf("Aiming: <white>%s</white>", you.weapon(0) 
        ? you.weapon(0)->name(DESC_INVENTORY).c_str() : "Throw Junk");
    args.show_distance = true;

    direction(target, args);

    if (!target.isValid)
    {
        if (target.isCancel)
            canned_msg(MSG_OK);
        return false;
    }
    if (teleport && cell_is_solid(target.target))
    {
        const char *feat = feat_type_name(grd(target.target));
        mprf("There is %s there.", article_a(feat).c_str());
        return false;
    }

    return true;
}

// Bring up an inventory screen and have user choose an item.
// Returns an item slot, or -1 on abort/failure
// On failure, returns error text, if any.
static int _fire_prompt_for_item()
{
    if (inv_count() < 1)
        return -1;

    int slot = prompt_invent_item("Fire/throw which item? (* to show all)",
                                   menu_type::invlist,
                                   OSEL_THROWABLE, OPER_FIRE);

    if (slot == PROMPT_ABORT || slot == PROMPT_NOTHING)
        return -1;

    return slot;
}

// Returns false and err text if this item can't be fired.
static bool _fire_validate_item(int slot, string &err)
{
    if (slot == you.equip[EQ_WEAPON0]
        && is_weapon(you.inv[slot])
        && you.inv[slot].cursed())
    {
        err = "That weapon is stuck to your " + you.hand_name(false) + "!";
        return false;
    }
    else if (item_is_worn(slot))
    {
        err = "You are wearing that object!";
        return false;
    }
    return true;
}

// Returns true if warning is given.
bool fire_warn_if_impossible(bool silent)
{
    if (you.species == SP_FELID || you.species == SP_FAIRY)
    {
        if (!silent)
            mpr("You can't grasp things well enough to throw them.");
        return true;
    }

    // If you can't wield it, you can't throw it.
    if (!form_can_wield())
    {
        if (!silent)
            canned_msg(MSG_PRESENT_FORM);
        return true;
    }

    if (you.attribute[ATTR_HELD])
    {
        const item_def *weapon = you.weapon();
        if (!weapon)
        {
            if (!silent)
                mprf("You cannot throw anything while %s.", held_status());
            return true;
        }
        else
        {
            if (!silent)
            {
                mprf("You cannot shoot with your %s while %s.",
                     weapon->name(DESC_BASENAME).c_str(), held_status());
            }
            return true;
        }
        // Else shooting is possible.
    }
    if (you.berserk())
    {
        if (!silent)
            canned_msg(MSG_TOO_BERSERK);
        return true;
    }
    return false;
}

bool aiming_checks(dist &target, bool teleport)
{
    if (fire_warn_if_impossible())
    {
        flush_input_buffer(FLUSH_ON_FAILURE);
        return false;
    }

    if (!_fire_choose_target(-1, target, teleport))
        return false;

    return true;
}

// Portal Projectile requires MP per shot.
bool is_pproj_active()
{
    return !you.confused() && you.duration[DUR_PORTAL_PROJECTILE]
           && enough_mp(1, true, false);
}

// If item == -1, prompt the user.
// If item passed, it will be put into the quiver.
void fire_thing()
{
#ifdef USE_SOUND
    parse_sound(FIRE_PROMPT_SOUND);
#endif

    dist target;
    if (!aiming_checks(target, is_pproj_active()))
        return;

    if (!you.weapon(0) || check_warning_inscriptions(*you.weapon(0), OPER_FIRE))
    {
        bolt beam;
        throw_it(beam, -1, &target);
    }
}

// Basically does what throwing used to do: throw an item without changing
// the quiver.
void throw_item_no_quiver()
{
    if (fire_warn_if_impossible())
    {
        flush_input_buffer(FLUSH_ON_FAILURE);
        return;
    }

    if (inv_count() < 1)
    {
        canned_msg(MSG_NOTHING_CARRIED);
        return;
    }

    string warn;
    int slot = _fire_prompt_for_item();

    if (slot == -1)
    {
        canned_msg(MSG_OK);
        return;
    }

    if (!_fire_validate_item(slot, warn))
    {
        mpr(warn);
        return;
    }

    dist target;
    if (!_fire_choose_target(slot, target, is_pproj_active()))
        return;

    bolt beam;
    throw_it(beam, slot, &target);
}

static bool _setup_missile_beam(const actor *agent, bolt &beam, item_def &item,
                                string &ammo_name, bool &returning)
{
    const auto cglyph = get_item_glyph(item);
    beam.glyph  = cglyph.ch;
    beam.colour = cglyph.col;
    beam.was_missile = true;

    item_def *launcher  = agent->weapon(0);
    if (launcher && !item.launched_by(*launcher))
        launcher = nullptr;

    if (agent->is_player())
    {
        beam.attitude      = ATT_FRIENDLY;
        beam.thrower       = KILL_YOU_MISSILE;
    }
    else
    {
        const monster* mon = agent->as_monster();

        beam.attitude      = mons_attitude(*mon);
        beam.thrower       = KILL_MON_MISSILE;
    }

    beam.range        = you.current_vision;
    beam.source_id    = agent->mid;
    beam.item         = &item;
    beam.source       = agent->pos();
    beam.flavour      = BEAM_MISSILE;
    beam.pierce       = is_penetrating_attack(*agent, launcher, item);
    beam.aux_source.clear();

    beam.name = item.name(DESC_PLAIN, false, false, false);
    ammo_name = item.name(DESC_PLAIN);

    const unrandart_entry* entry = launcher && is_unrandom_artefact(*launcher)
        ? get_unrand_entry(launcher->unrand_idx) : nullptr;

    if (entry && entry->launch)
    {
        setup_missile_type sm =
            entry->launch(launcher, &beam, &ammo_name,
                                     &returning);

        switch (sm)
        {
        case SM_CONTINUE:
            break;
        case SM_FINISHED:
            return false;
        case SM_CANCEL:
            return true;
        }
    }

    returning = item.base_type == OBJ_MISSILES
                && get_ammo_brand(item) == SPMSL_RETURNING;

    if (item.base_type == OBJ_MISSILES
        && (get_ammo_brand(item) == SPMSL_EXPLODING
            || item.sub_type == MI_LARGE_ROCK))
    {
        bolt *expl = new bolt(beam);

        expl->is_explosion = true;
        expl->damage       = dice_def(2, 5);
        expl->ex_size      = 1;

        if (beam.flavour == BEAM_MISSILE)
        {
            expl->flavour = BEAM_FRAG;
            expl->name   += " fragments";

            const string short_name =
                item.name(DESC_BASENAME, true, false, false, false,
                          ISFLAG_IDENT_MASK | ISFLAG_COSMETIC_MASK);

            expl->name = replace_all(expl->name, item.name(DESC_PLAIN),
                                     short_name);
        }
        expl->name = "explosion of " + expl->name;

        beam.special_explosion = expl;
    }

    if (!is_artefact(item))
        ammo_name = article_a(ammo_name, true);
    else
        ammo_name = "the " + ammo_name;

    return false;
}

static void _throw_noise(actor* act, const item_def &ammo, const item_def &launcher)
{
    ASSERT(act); // XXX: change to actor &act

    if (launcher.base_type != OBJ_WEAPONS)
        return;

    if (is_launched(act, &launcher, &launcher, ammo) != launch_retval::LAUNCHED)
        return;

    // Throwing and blowguns are silent...
    int         level = 0;
    const char* msg   = nullptr;

    switch (launcher.sub_type)
    {
    case WPN_HUNTING_SLING:
        level = 1;
        msg   = "You hear a whirring sound.";
        break;
    case WPN_FUSTIBALUS:
        level = 3;
        msg   = "You hear a loud whirring sound.";
        break;
    case WPN_MANGONEL:
        level = 9;
        msg = "You hear an immense crack.";
        break;
    case WPN_SHORTBOW:
        level = 5;
        msg   = "You hear a twanging sound.";
        break;
    case WPN_LONGBOW:
        level = 6;
        msg   = "You hear a loud twanging sound.";
        break;
    case WPN_HAND_CROSSBOW:
        level = 2;
        msg   = "You hear a quiet thunk.";
        break;
    case WPN_ARBALEST:
        level = 7;
        msg   = "You hear a thunk.";
        break;
    case WPN_TRIPLE_CROSSBOW:
        level = 9;
        msg   = "You hear a triplet of thunks.";
        break;

    default:
        die("Invalid launcher '%s'",
                 launcher.name(DESC_PLAIN).c_str());
        return;
    }
    if (act->is_player() || you.can_see(*act))
        msg = nullptr;

    noisy(level, act->pos(), msg, act->mid);
}

int random_stone()
{
    missile_type branch_specific0 = MI_STONE;
    missile_type branch_specific1 = MI_STONE;
    if (you.where_are_you == BRANCH_ICE_CAVE ||
        you.where_are_you == BRANCH_COCYTUS)
    {
        branch_specific0 = MI_SNOWBALL;
        branch_specific1 = MI_SNOWBALL;
    }
    if (you.where_are_you == BRANCH_SEWER ||
        you.where_are_you == BRANCH_SWAMP)
    {
        branch_specific0 = MI_MUD;
        branch_specific1 = MI_ROOT;
    }
    if (you.where_are_you == BRANCH_ORC)
    {
        branch_specific0 = MI_GOLD;
        branch_specific1 = MI_SKULL;
    }
    if (you.where_are_you == BRANCH_ABYSS)
    {
        branch_specific0 = MI_ABYSS;
        branch_specific1 = MI_ABYSS;
    }
    if (you.where_are_you == BRANCH_PANDEMONIUM)
    {
        branch_specific0 = MI_PANDEMONIUM;
        branch_specific1 = MI_PANDEMONIUM;
    }
    if (you.where_are_you == BRANCH_SLIME)
    {
        branch_specific0 = MI_OOZE;
        branch_specific1 = MI_OOZE;
    }
    if (you.where_are_you == BRANCH_LAIR)
    {
        branch_specific0 = MI_ROOT;
        branch_specific1 = MI_BONE;
    }
    if (you.where_are_you == BRANCH_CRYPT)
    {
        branch_specific0 = MI_BONE;
        branch_specific1 = MI_SKULL;
    }
    if (you.where_are_you == BRANCH_TOMB)
    {
        branch_specific0 = MI_BANDAGE;
        branch_specific1 = MI_STONE;
    }
    if (you.where_are_you == BRANCH_SHOALS)
    {
        branch_specific0 = MI_SEASHELL;
        branch_specific1 = MI_MUD;
    }
    if (you.where_are_you == BRANCH_VAULTS ||
        you.where_are_you == BRANCH_DEPTHS)
        branch_specific0 = MI_OOZE;

    return random_choose(branch_specific0,
        branch_specific1, MI_BLADE, MI_BONE, MI_SKULL);
}

// throw_it - currently handles player throwing only. Monster
// throwing is handled in mon-act:_mons_throw()
// Note: If teleport is true, assume that pbolt is already set up,
// and teleport the projectile onto the square.
//
// Return value is only relevant if dummy_target is non-nullptr, and returns
// true if dummy_target is hit.
bool throw_it(bolt &pbolt, int throw_2, dist *target)
{
    dist thr;
    bool returning   = false;    // Item can return to pack.
    bool did_return  = false;    // Returning item actually does return to pack.
    const bool teleport = is_pproj_active();

    if (you.confused())
    {
        thr.target = you.pos();
        thr.target.x += random2(13) - 6;
        thr.target.y += random2(13) - 6;
        thr.isValid = true;
    }
    else if (target)
        thr = *target;
    else if (pbolt.target.zero())
    {
        direction_chooser_args args;
        args.mode = TARG_HOSTILE;
        direction(thr, args);

        if (!thr.isValid)
        {
            if (thr.isCancel)
                canned_msg(MSG_OK);

            return false;
        }
    }
    pbolt.set_target(thr);

    item_def * thrown = nullptr;
    int t = 0;

    if (throw_2 != -1)
        thrown = &you.inv[throw_2];
    else
    {
        int typ = MI_STONE;
        if (you.weapon(0) && is_range_weapon(*you.weapon(0)))
            typ = fires_ammo_type(*you.weapon(0));
        else typ = random_stone();
        t = items(false, OBJ_MISSILES, typ, 1);
        thrown = &mitm[t];
        thrown->quantity = 1;
        thrown->brand = SPMSL_NORMAL;
    }
    ASSERT(thrown);

    // Figure out if we're thrown or launched.
    const launch_retval projected = is_launched(&you, you.weapon(0), you.weapon(1), *thrown);

    string ammo_name;

    if (_setup_missile_beam(&you, pbolt, *thrown, ammo_name, returning))
    {
        you.turn_is_over = false;
        if (throw_2 == -1)
            destroy_item(t);
        return false;
    }

    // Get the ammo type. Convenience.
    const int               wepType  = thrown->sub_type;

    // Don't trace at all when confused.
    // Give the player a chance to be warned about helpless targets when using
    // Portaled Projectile, but obviously don't trace a path.
    bool cancelled = false;
    if (!you.confused())
    {
        // Kludgy. Ideally this would handled by the same code.
        // Perhaps some notion of a zero length bolt, with the source and
        // target both set to the target?
        if (teleport)
        {
            // This block is roughly equivalent to bolt::affect_cell for
            // normal projectiles.
            monster *m = monster_at(thr.target);
            if (m)
                cancelled = stop_attack_prompt(m, false, thr.target);

            if (!cancelled && (pbolt.is_explosion || pbolt.special_explosion))
            {
                for (adjacent_iterator ai(thr.target); ai; ++ai)
                {
                    if (cancelled)
                        break;
                    monster *am = monster_at(*ai);
                    if (am)
                        cancelled = stop_attack_prompt(am, false, *ai);
                    else if (*ai == you.pos())
                    {
                        cancelled = !yesno("That is likely to hit you. Continue anyway?",
                                           false, 'n');
                    }
                }
            }
        }
        else
        {
            // Set values absurdly high to make sure the tracer will
            // complain if we're attempting to fire through allies.
            pbolt.damage = dice_def(1, 100);

            // Init tracer variables.
            pbolt.foe_info.reset();
            pbolt.friend_info.reset();
            pbolt.foe_ratio = 100;
            pbolt.is_tracer = true;

            pbolt.fire();

            cancelled = pbolt.beam_cancelled;

            pbolt.hit    = 0;
            pbolt.damage = dice_def();
        }
    }

    // Should only happen if the player answered 'n' to one of those
    // "Fire through friendly?" prompts.
    if (cancelled)
    {
        you.turn_is_over = false;
        if (throw_2 == -1)
            destroy_item(t);
        return false;
    }

    pbolt.is_tracer = false;

    bool unwielded = false;
    if (throw_2 != -1 && (throw_2 == you.equip[EQ_WEAPON0] || throw_2 == you.equip[EQ_WEAPON1]) && thrown->quantity == 1)
    {
        if (!wield_weapon((throw_2 == you.equip[EQ_WEAPON0]), SLOT_BARE_HANDS, true, false, true, false))
            return false;

        if (!thrown->quantity)
            return false; // destroyed when unequipped (fragile)

        unwielded = true;
    }

    // Now start real firing!
    origin_set_unknown(*thrown);

    // bloodpots & chunks need special handling.
    if (thrown->quantity > 1 && is_perishable_stack(*thrown))
    {
        // Initialise thrown item with oldest item in stack.
        const int rot_timer = remove_oldest_perishable_item(*thrown)
                              - you.elapsed_time;
        thrown->props.clear();
        init_perishable_stack(*thrown, rot_timer);
    }

    // Even though direction is allowed, we're throwing so we
    // want to use tx, ty to make the missile fly to map edge.
    if (!teleport)
        pbolt.set_target(thr);

    int bow_brand        = SPWPN_NORMAL;
    const int ammo_brand = get_ammo_brand(*thrown);

    switch (projected)
    {
    case launch_retval::LAUNCHED:
    {
        item_def *launcher;
        if (you.weapon(0) && is_range_weapon(*you.weapon(0)))
            launcher = you.weapon(0);
        else
            launcher = you.weapon(1);
        ASSERT(launcher);
        practise_launching(*launcher);
        bow_brand = get_weapon_brand(*launcher);
        if (is_unrandom_artefact(*launcher)
            && get_unrand_entry(launcher->unrand_idx)->type_name)
        {
            count_action(CACT_FIRE, launcher->unrand_idx);
        }
        else
            count_action(CACT_FIRE, launcher->sub_type);
        break;
    }
    case launch_retval::THROWN:
        count_action(CACT_THROW, wepType, OBJ_MISSILES);
        break;
    case launch_retval::FUMBLED:
        break;
    case launch_retval::BUGGY:
        dprf("Unknown launch type for weapon."); // should never happen :)
        break;
    }

    // check for returning ammo
    if (teleport)
        returning = false;

    bool is_missile = (thrown->base_type == OBJ_MISSILES);

    if (!is_missile)
        you.time_taken = 10;
    else if (ammo_type_damage(wepType) == 2)
        you.time_taken = 5;
    else
        you.time_taken = you.attack_delay(thrown).roll();

    string prefix = "";

    if (is_missile)
    {
        if (wepType == MI_SNOWBALL)
            prefix += "ball up some snow and ";
        else if (wepType == MI_OOZE)
            prefix += "ball up the remains of a slime creature and ";
        else if (wepType == MI_ABYSS)
            prefix += "make a ball from the fabric of the abyss and ";
        else if (wepType == MI_BLADE)
            prefix += "pick up a piece of a broken weapon and ";
        else if (wepType == MI_ROOT)
            prefix += "rip up a gnarled root and ";
        else if (wepType == MI_BANDAGE)
            prefix += "mangle together a loose clump of bandages and embalming tools and ";
        else if (wepType == MI_PANDEMONIUM)
            prefix += "rip off a chunk of the weird stuff that makes up pandemonium and ";

        else if (ammo_type_damage(wepType) == 2)
        {
            prefix += "pick up ";
            prefix += thrown->name(DESC_A, false, false, false);
            prefix += " and ";
        }
    }

    // Create message.
    mprf("You %s%s%s %s.",
          prefix.c_str(),
          teleport ? "magically " : "",
          ((is_missile && wepType == MI_LARGE_ROCK) ? "hurl" :
           projected == launch_retval::FUMBLED ? "toss away" :
           projected == launch_retval::LAUNCHED ? "shoot" : "throw"),
          ((is_missile && ammo_type_damage(wepType) == 2)) ? "it" : ammo_name.c_str());

    // Ensure we're firing a 'missile'-type beam.
    pbolt.pierce    = false;
    pbolt.is_tracer = false;

    pbolt.loudness = thrown->base_type == OBJ_MISSILES
                   ? ammo_type_damage(wepType) / 3
                   : 0; // Maybe not accurate, but reflects the damage.

    pbolt.hit = teleport ? random2(you.attribute[ATTR_PORTAL_PROJECTILE] / 4)
                         : 0;

    bool hit = false;
    if (teleport)
    {
        // Violating encapsulation somewhat...oh well.
        pbolt.use_target_as_pos = true;
        pbolt.affect_cell();
        pbolt.affect_endpoint();
        if (!did_return)
            pbolt.drop_object();
        // Costs 1 MP per shot.
        dec_mp(1);
    }
    else
    {
        if (crawl_state.game_is_hints())
            Hints.hints_throw_counter++;

        // Dropping item copy, since the launched item might be different.
        pbolt.drop_item = !did_return;
        pbolt.fire();

        hit = !pbolt.hit_verb.empty();
    }

    if (bow_brand == SPWPN_CHAOS || ammo_brand == SPMSL_CHAOS)
        did_god_conduct(DID_CHAOS, 2 + random2(3), bow_brand == SPWPN_CHAOS);

    if (bow_brand == SPWPN_SPEED)
        did_god_conduct(DID_HASTY, 1, true);

    if (ammo_brand == SPMSL_FRENZY)
        did_god_conduct(DID_HASTY, 6 + random2(3), true);

    if (throw_2 != -1)
        dec_inv_item_quantity(throw_2, 1);
    if (unwielded)
        canned_msg(MSG_EMPTY_HANDED_NOW);

    if (you.weapon(0) && is_range_weapon(*you.weapon(0)))
        _throw_noise(&you, *thrown, *you.weapon(0));

    // BCAD NOTE: If we ever allow dual wielding ranged weapons this will take some rework.
    // ...any monster nearby can see that something has been thrown, even
    // if it didn't make any noise.
    alert_nearby_monsters();

    you.turn_is_over = true;

    if (pbolt.special_explosion != nullptr)
        delete pbolt.special_explosion;

    if (!teleport
        && projected != launch_retval::FUMBLED
        && thrown->base_type == OBJ_MISSILES)
    {
        mount_drake_breath(&pbolt);
        if (will_have_passive(passive_t::shadow_attacks))
            dithmenos_shadow_throw(thr, *thrown);
    }

    if (throw_2 == -1)
        destroy_item(t);

    return hit;
}

void setup_monster_throw_beam(monster* mons, bolt &beam)
{
    beam.range = you.current_vision;
    beam.source_id = mons->mid;

    beam.glyph   = dchar_glyph(DCHAR_FIRED_MISSILE);
    beam.flavour = BEAM_MISSILE;
    beam.thrower = KILL_MON_MISSILE;
    beam.aux_source.clear();
    beam.pierce  = false;
}

// msl is the item index of the thrown missile (or weapon).
bool mons_throw(monster* mons, bolt &beam, int msl, bool teleport)
{
    string ammo_name;

    bool returning = false;

    // Some initial convenience & initializations.
    ASSERT(mitm[msl].base_type == OBJ_MISSILES);

    const int weapon    = mons->inv[MSLOT_WEAPON];

    // Energy is already deducted for the spell cast, if using portal projectile
    // FIXME: should it use this delay and not the spell delay?
    if (!teleport)
    {
        const int energy = mons->action_energy(EUT_MISSILE);
        const int delay = mons->attack_delay(&mitm[msl]).roll();
        ASSERT(energy > 0);
        ASSERT(delay > 0);
        mons->speed_increment -= div_rand_round(energy * delay, 10);
    }

    // Dropping item copy, since the launched item might be different.
    item_def item = mitm[msl];
    item.quantity = 1;

    if (_setup_missile_beam(mons, beam, item, ammo_name, returning))
    {
        destroy_item(msl);
        return false;
    }

    beam.aimed_at_spot |= returning;

    launch_retval projected =
        is_launched(mons, mons->mslot_item(MSLOT_WEAPON), mons->mslot_item(MSLOT_WEAPON),
                    mitm[msl]);

    if (projected == launch_retval::THROWN)
        returning = returning && !teleport;

    // Identify before throwing, so we don't get different
    // messages for first and subsequent missiles.
    if (mons->observable())
    {
        if (projected == launch_retval::LAUNCHED
               && item_type_known(mitm[weapon])
            || projected == launch_retval::THROWN
               && mitm[msl].base_type == OBJ_MISSILES)
        {
            set_ident_flags(mitm[msl], ISFLAG_KNOW_TYPE);
            set_ident_flags(item, ISFLAG_KNOW_TYPE);
        }
    }

    // Now, if a monster is, for some reason, throwing something really
    // stupid, it will have baseHit of 0 and damage of 0. Ah well.
    string msg = mons->name(DESC_THE);

    if (item.sub_type == MI_SNOWBALL)
        msg += " balls up snow from the ground and";
    else if (item.sub_type == MI_OOZE)
        msg += " balls up the remains of a slime creature and";
    else if (item.sub_type == MI_ABYSS)
        msg += " makes a ball from the fabric of the abyss and";
    else if (item.sub_type == MI_BLADE)
        msg += " picks up a piece of a broken weapon and";
    else if (item.sub_type == MI_ROOT)
        msg += " rips up a gnarled root and";
    else if (item.sub_type == MI_BANDAGE)
        msg += " mangles together a loose clump of bandages and embalming tools and";
    else if (item.sub_type == MI_PANDEMONIUM)
        msg += " rips off a chunk of the weird stuff that makes up pandemonium and";
    else if (item.sub_type == MI_BONE && ((mons_genus(mons->type) == MONS_SKELETAL_WARRIOR) 
                                       || (mons_genus(mons->type) == MONS_SKELETON)))
        msg += " takes a piece of bone from its own ripcage and";

    else if (ammo_type_damage(item.sub_type) == 2)
    {
        msg += " picks up ";
        msg += item.name(DESC_A, false, false, false);
        msg += " and";
    }

    if (teleport)
        msg += " magically";
    if (item.sub_type == MI_NEEDLE)
        msg += " launches ";
    else if (item.sub_type != MI_LARGE_ROCK)
        msg += ((projected == launch_retval::LAUNCHED) ? " shoots " : " throws ");
    else
        msg += " hurls ";

    if (!beam.name.empty() && projected == launch_retval::LAUNCHED)
        msg += article_a(beam.name);
    else
    {
        // build shoot message
        if (ammo_type_damage(item.sub_type) == 2)
            msg += "it";
        else
            msg += item.name(DESC_A, false, false, false);

        // build beam name
        beam.name = item.name(DESC_PLAIN, false, false, false);
    }
    msg += ".";

    if (mons->observable())
    {
        mons->flags |= MF_SEEN_RANGED;
        mpr(msg);
    }

    if (mons->mslot_item(MSLOT_WEAPON))
        _throw_noise(mons, item, *mons->mslot_item(MSLOT_WEAPON));

    beam.drop_item = !returning;

    // Redraw the screen before firing, in case the monster just
    // came into view and the screen hasn't been updated yet.
    viewwindow();
    if (teleport)
    {
        beam.use_target_as_pos = true;
        beam.affect_cell();
        beam.affect_endpoint();
    }
    else
        beam.fire();

    if (beam.special_explosion != nullptr)
        delete beam.special_explosion;

    destroy_item(msl);
    return true;
}

bool thrown_object_destroyed(item_def *item)
{
    ASSERT(item != nullptr);

    if (item->base_type != OBJ_MISSILES)
        return false;

    if (item->sub_type == MI_THROWING_NET)
        return false;

    return true;
}
