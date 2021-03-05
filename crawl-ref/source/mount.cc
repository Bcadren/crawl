/**
* @file
* @brief Player mount related functions.
**/

#include "AppHdr.h"

#include "mount.h"

#include <cstring>

#include "env.h"
#include "message.h"
#include "mpr.h"
#include "output.h"
#include "player.h"
#include "prompt.h"
#include "random.h"
#include "state.h"
#include "transform.h"

spret gain_mount(mount_type mount, int pow, bool fail)
{
    if (you.transform_uncancellable)
    {
        mprf("You can't mount anything outside of your normal form.");
        return spret::abort;
    }
    else if (you.form != transformation::none)
    {
        if (!crawl_state.disables[DIS_CONFIRMATIONS] &&
            !yesno("You have to untransform in order to properly ride a mount. Continue?", true, 0))
        {
            canned_msg(MSG_OK);
            return spret::abort;
        }
        untransform();
    }

    fail_check();
    int dur = 0;
    int mount_hp = 0;
    bool were_flying = you.airborne();
    bool already_mount = you.mounted();

    switch (mount)
    {
    case mount_type::drake:
        mount_hp = 28 + random2(10);
        break;
    case mount_type::slime:
    case mount_type::hydra:
        mount_hp = 60 + random2(12);
        break;
    default:
    case mount_type::spider:
        mount_hp = 47 + random2(9);
        break;
    }

    if (pow == 0)
    {
        dur = 100 + you.skill(SK_INVOCATIONS) * 2 + random2(you.skill(SK_INVOCATIONS));
        mount_hp *= 10 + you.skill(SK_INVOCATIONS);
        mount_hp = div_rand_round(mount_hp, 10);
    }
    else
    {
        dur = 100 + pow / 2 + random2(pow / 2);
        mount_hp *= 100 + pow;
        mount_hp = div_rand_round(mount_hp, 100);
    }

    if (you.mount != mount)
    {
        if (mount == mount_type::hydra)
        {
            you.mount_heads = 4 + max(0, div_round_up(pow - 50, 13));
        }
        else
            you.mount_heads = 1;
    }

    if (already_mount)
    {
        if (you.mount == mount)
        {
            bool extend = you.duration[DUR_MOUNTED] < dur;
            bool heal = (you.mount_hp != you.mount_hp_max) || (mount_statuses() > 0);
            if (!extend)
            {
                if (!heal)
                    mprf(MSGCH_DURATION, "You fail to extend your %s's duration any further.", you.mount_name().c_str());
                else
                    mprf(MSGCH_DURATION, "You heal your %s's injuries.", you.mount_name().c_str());
            }
            else
            {
                if (!heal)
                    mprf(MSGCH_DURATION, "You increase the time your %s has in this world.", you.mount_name().c_str());
                else
                    mprf(MSGCH_DURATION, "You heal your %s and increase the time it has in this world.", you.mount_name().c_str());
            }
            cure_mount_debuffs();
            you.mount_hp = you.mount_hp_max;
        }
        else
        {
            mprf(MSGCH_DURATION, "You dismiss your previous mount to call another one.");
            dismount();
            you.mount = mount;
            you.mount_hp = you.mount_hp_max = mount_hp;
        }
    }
    else
    {
        you.mount = mount;
        you.mount_hp = you.mount_hp_max = mount_hp;
        you.mount_energy = 10;
    }

    dur = max(you.duration[DUR_MOUNTED], dur);
    you.set_duration(DUR_MOUNTED, dur, 200);

    if (!already_mount)
    {
        mprf(MSGCH_DURATION, "You summon a %s and ride upon its back.", you.mount_name().c_str());

        if (were_flying)
            land_player();

        merfolk_check_swimming();

        you.redraw_hit_points = true;
        redraw_screen();
    }

    return spret::success;
}

void damage_mount(int amount)
{
    you.mount_hp -= amount;
    you.redraw_hit_points = true;
    if (you.mount_hp <= 0)
    {
        mprf(MSGCH_DANGER, "Your %s dies.", you.mount_name().c_str());
        dismount();
    }
}

void dismount()
{
    cure_mount_debuffs();
    you.mount = mount_type::none;
    you.mount_hp = you.mount_hp_max = you.mount_hp_regen = 0;
    redraw_screen();
    merfolk_check_swimming();
}

// Ends all mount debuffs when a mount is dismissed or resummoned.
void cure_mount_debuffs()
{
    if (you.duration[DUR_MOUNTED])
        you.duration[DUR_MOUNTED] = 0;
    if (you.duration[DUR_MOUNT_POISONING])
        you.duration[DUR_MOUNT_POISONING] = 0;
    if (you.duration[DUR_MOUNT_CORROSION])
    {
        you.duration[DUR_MOUNT_CORROSION] = 0;
        you.props["mount_corrosion_amount"] = 0;
    }
    if (you.duration[DUR_MOUNT_DRAINING])
        you.duration[DUR_MOUNT_DRAINING] = 0;
    if (you.duration[DUR_MOUNT_BREATH])
        you.duration[DUR_MOUNT_BREATH] = 0;
    if (you.duration[DUR_ENSNARE])
        you.duration[DUR_ENSNARE] = 0;
    if (you.duration[DUR_MOUNT_WRETCHED])
        you.duration[DUR_MOUNT_WRETCHED] = 0;
    if (you.duration[DUR_MOUNT_SLOW])
        you.duration[DUR_MOUNT_SLOW] = 0;
    if (you.petrifying(true))
        you.duration[DUR_MOUNT_PETRIFYING] = 0;
    if (you.petrified(true))
        you.duration[DUR_MOUNT_PETRIFIED] = 0;
}

// Returns whether a hit should hit you (false) or a mount (true)
bool mount_hit()
{
    if (!you.mounted())
        return false;
    return !x_chance_in_y(you.body_size(PSIZE_BODY), you.body_size(PSIZE_BODY) + 5);
}

int apply_mount_ac(int damage, int max_dmg, ac_type type)
{
    return you.apply_ac(damage, max_dmg, type, 0, true, true);
}

bool poison_mount(int amount, bool force)
{
    ASSERT(!crawl_state.game_is_arena());

    if (crawl_state.disables[DIS_AFFLICTIONS])
        return false;

    if (!force && (you.mount == mount_type::hydra) && !one_chance_in(3))
        return false;

    const int old_value = you.duration[DUR_MOUNT_POISONING];
    const bool was_fatal = (you.duration[DUR_MOUNT_POISONING] / 1000) >= you.mount_hp;

    if (you.mount == mount_type::spider)
        amount *= 2;

    you.duration[DUR_MOUNT_POISONING] += amount * 1000;

    if (you.duration[DUR_MOUNT_POISONING] > old_value)
    {
        if ((you.duration[DUR_MOUNT_POISONING] / 1000) >= you.mount_hp && !was_fatal)
            mprf(MSGCH_DANGER, "Your mount is lethally poisoned!");
        else
        {
            mprf(MSGCH_WARN, "Your mount is %spoisoned.",
                old_value > 0 ? "more " : "");
        }
    }

    // Display the poisoned segment of our health, in case we take no damage
    you.redraw_hit_points = true;

    return amount;
}

int mount_regen()
{
    bool regen_mod = 1;
    if (you.mount == mount_type::hydra)
        regen_mod = 2;
    return (10 + you.mount_hp_max / 3) * regen_mod;
}

int mount_poison_survival()
{
    if (!you.duration[DUR_MOUNT_POISONING])
        return you.mount_hp;
    const int rr = mount_regen();
    const int amount = you.duration[DUR_MOUNT_POISONING];
    const double full_aut = poison_dur_to_aut(amount);
    // Calculate the poison amount at which regen starts to beat poison.
    double min_poison_rate = 0.25;

    int regen_beats_poison;
    if (rr <= (int)(100.0 * min_poison_rate))
        regen_beats_poison = 0;
    else
        regen_beats_poison = 150 * rr;

    if (rr == 0)
        return min(you.mount_hp, you.mount_hp - amount / 1000 + regen_beats_poison / 1000);

    // Calculate the amount of time until regen starts to beat poison.
    double poison_duration = full_aut - poison_dur_to_aut(regen_beats_poison);

    if (poison_duration < 0)
        poison_duration = 0;

    // Worst case scenario is right before natural regen gives you a point of
    // HP, so consider the nearest two such points.
    const int predicted_regen = (int)((((double)you.mount_hp_regen) + rr * poison_duration / 10.0) / 100.0);
    double test_aut1 = (100.0 * predicted_regen - 1.0 - ((double)you.mount_hp_regen)) / (rr / 10.0);
    double test_aut2 = (100.0 * predicted_regen + 99.0 - ((double)you.mount_hp_regen)) / (rr / 10.0);

    const int test_amount1 = poison_aut_to_dur(full_aut - test_aut1);
    const int test_amount2 = poison_aut_to_dur(full_aut - test_aut2);

    int prediction1 = you.mount_hp;
    int prediction2 = you.mount_hp;

    // Don't look backwards in time.
    if (test_aut1 > 0)
        prediction1 -= (amount / 1000 - test_amount1 / 1000 - (predicted_regen - 1));
    prediction2 -= (amount / 1000 - test_amount2 / 1000 - predicted_regen);

    return min(prediction1, prediction2);
}

monster_type mount_mons()
{
    switch (you.mount)
    {
    case mount_type::drake:
        return MONS_RIME_DRAKE;
    case mount_type::hydra:
        return MONS_HYDRA;
    case mount_type::spider:
        return MONS_JUMPING_SPIDER;
    case mount_type::slime:
        return MONS_SLIME_CREATURE;
    default:
        break;
    }
    return MONS_PROGRAM_BUG;
}

int mount_statuses()
{
    int retval = 0;

    if (you.duration[DUR_MOUNT_POISONING])
        retval++;

    if (you.duration[DUR_MOUNT_CORROSION])
        retval++;

    if (you.duration[DUR_MOUNT_DRAINING])
        retval++;

    if (you.duration[DUR_MOUNT_WRETCHED])
        retval++;

    if (you.duration[DUR_MOUNT_SLOW])
        retval++;

    if (you.duration[DUR_MOUNT_BREATH])
        retval++;

    if (you.duration[DUR_ENSNARE])
        retval++;

    if (you.duration[DUR_MOUNT_PETRIFYING])
        retval++;

    if (you.duration[DUR_MOUNT_PETRIFIED])
        retval++;

    if (you.duration[DUR_MOUNT_BARBS])
        retval++;

    if (you.duration[DUR_MOUNT_FROZEN])
        retval++;

    return retval;
}

bool drain_mount(int strength)
{
    if (you.res_negative_energy(false, true) >= 3)
        return false;

    mprf("Your %s is drained.", you.mount_name(true).c_str());

    you.increase_duration(DUR_MOUNT_DRAINING, strength);

    return true;
}

void slow_mount(int duration)
{
    if (!you.duration[DUR_MOUNT_SLOW])
        mprf("Your %s seems to slow down.", you.mount_name(true).c_str());

    you.increase_duration(DUR_MOUNT_SLOW, duration, 25);
}

bool miasma_mount()
{
    if (you.res_rotting(true, true))
        return false;

    bool success = poison_mount(5 + roll_dice(3, 12));

    if (one_chance_in(3))
    {
        slow_mount(10 + random2(5));
        success = true;
    }

    if (coinflip())
    {
        rot_mount(1);
        success = true;
    }

    return success;
}

void rot_mount(int amount, bool needs_message)
{
    if (you.res_rotting(true, true))
        return;

    if (needs_message)
        mprf("Some of your %s's flesh rots away.", you.mount_name(true).c_str());

    you.mount_hp_max -= amount;
    if (you.mount_hp > you.mount_hp_max)
        you.mount_hp = you.mount_hp_max;

    if (you.mount_hp <= 0) // This will never happen, right? O_O;
    {
        mprf("Your %s dies!", you.mount_name(true).c_str());
        dismount();
    }
}

int mount_hd()
{
    switch (you.mount)
    {
    case mount_type::hydra:
        return div_rand_round(calc_spell_power(SPELL_SUMMON_HYDRA_MOUNT, true), 10);
    case mount_type::spider:
        return div_rand_round(calc_spell_power(SPELL_SUMMON_SPIDER_MOUNT, true), 10);
    case mount_type::drake:
    case mount_type::slime:
        return (2 + div_rand_round(you.skill(SK_INVOCATIONS) * 2, 3));
    default:
        break;
    }
    return 1;
}

bool mount_submerged()
{
    if (you.airborne())
        return false;

    return (grd(you.pos()) == DNGN_DEEP_WATER || grd(you.pos()) == DNGN_DEEP_SLIMY_WATER);
}

int mount_gdr()
{
    int gdr = mount_ac();

    return max(min(gdr, 50), 0);
}

int mount_ac()
{
    ASSERT(you.mounted());

    int ac = 0;

    switch (you.mount)
    {
    case mount_type::slime:
        ac = 1;
        break;
    case mount_type::hydra:
        ac = 3;
        break;
    case mount_type::spider:
        ac = 9;
        break;
    case mount_type::drake:
        ac = 6;
        break;
    default:
        mprf(MSGCH_ERROR, "Unhandled Mount AC.");
        break;
    }

    if (you.duration[DUR_MOUNT_PETRIFYING])
        ac *= 2;

    if (you.duration[DUR_MOUNT_PETRIFIED])
        ac *= 3;

    if (you.submerged(true))
        ac += 4;

    if (you.duration[DUR_QAZLAL_AC])
        ac += 3;

    if (you.duration[DUR_MOUNT_CORROSION])
        ac -= 4 * you.props["mount_corrosion_amount"].get_int();

    if (you.duration[DUR_ICY_ARMOUR])
        ac += 5 + you.props[ICY_ARMOUR_KEY].get_int() / 25;

    if (you.duration[DUR_PHYS_VULN])
        ac -= 4;

    return 0;
}
