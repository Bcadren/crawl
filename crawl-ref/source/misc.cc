/**
 * @file
 * @brief Misc functions.
**/

#include "AppHdr.h"

#include "misc.h"
#include "mpr.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifdef UNIX
#include <unistd.h>
#endif

#include "database.h"
#include "english.h"
#include "items.h"
#include "libutil.h"
#include "monster.h"
#include "state.h"
#include "terrain.h"
#include "tileview.h"
#include "traps.h"

string weird_glowing_colour()
{
    return getMiscString("glowing_colour_name");
}

// Make the player swap positions with a given monster.
void swap_with_monster(monster* mon_to_swap)
{
    monster& mon(*mon_to_swap);
    ASSERT(mon.alive());
    const coord_def newpos = mon.pos();

    if (you.stasis())
    {
        mpr("Your stasis prevents you from teleporting.");
        return;
    }

    // Be nice: no swapping into uninhabitable environments.
    if (!you.is_habitable(newpos) || !mon.is_habitable(you.pos()))
    {
        mpr("You spin around.");
        return;
    }

    const bool mon_caught = mon.caught();
    const bool you_caught = you.attribute[ATTR_HELD];

    mprf("You swap places with %s.", mon.name(DESC_THE).c_str());

    mon.move_to_pos(you.pos(), true, true);

    // XXX: destroy ammo == 1 webs if they don't catch the mons? very rare case

    if (you_caught)
    {
        // XXX: this doesn't correctly handle web traps
        check_net_will_hold_monster(&mon);
        if (!mon_caught)
            stop_being_held();
    }

    // Move you to its previous location.
    move_player_to_grid(newpos, false);

    if (mon_caught)
    {
        // XXX: destroy ammo == 1 webs? (rare case)

        if (you.body_size(PSIZE_BODY) >= SIZE_GIANT) // e.g. dragonform
        {
            int net = get_trapping_net(you.pos());
            if (net != NON_ITEM)
            {
                destroy_item(net);
                mpr("The net rips apart!");
            }

            if (you_caught)
                stop_being_held();
        }
        else // XXX: doesn't handle e.g. spiderform swapped into webs
        {
            you.attribute[ATTR_HELD] = 1;
            if (get_trapping_net(you.pos()) != NON_ITEM)
                mpr("You become entangled in the net!");
            else
                mpr("You get stuck in the web!");
            you.redraw_quiver = true; // Account for being in a net.
            you.redraw_evasion = true;
        }

        if (!you_caught)
            mon.del_ench(ENCH_HELD, true);
    }
}

void handle_real_time(chrono::time_point<chrono::system_clock> now)
{
    const chrono::milliseconds elapsed =
     chrono::duration_cast<chrono::milliseconds>(now - you.last_keypress_time);
    you.real_time_delta = min<chrono::milliseconds>(
      elapsed,
      (chrono::milliseconds)(IDLE_TIME_CLAMP * 1000));
    you.real_time_ms += you.real_time_delta;
    you.last_keypress_time = now;
}

unsigned int breakpoint_rank(int val, const int breakpoints[],
                             unsigned int num_breakpoints)
{
    unsigned int result = 0;
    while (result < num_breakpoints && val >= breakpoints[result])
        ++result;

    return result;
}

void counted_monster_list::add(const monster* mons)
{
    const string name = mons->name(DESC_PLAIN);
    for (auto &entry : list)
    {
        if (entry.first->name(DESC_PLAIN) == name)
        {
            entry.second++;
            return;
        }
    }
    list.emplace_back(mons, 1);
}

int counted_monster_list::count()
{
    int nmons = 0;
    for (const auto &entry : list)
        nmons += entry.second;
    return nmons;
}

string counted_monster_list::describe(description_level_type desc)
{
    string out;

    for (auto i = list.begin(); i != list.end();)
    {
        const counted_monster &cm(*i);
        if (i != list.begin())
        {
            ++i;
            out += (i == list.end() ? " and " : ", ");
        }
        else
            ++i;

        out += cm.second > 1
               ? pluralise_monster(cm.first->name(desc, false, true))
               : cm.first->name(desc);
    }
    return out;
}

bool tobool(maybe_bool mb, bool def)
{
    switch (mb)
    {
    case MB_TRUE:
        return true;
    case MB_FALSE:
        return false;
    case MB_MAYBE:
    default:
        return def;
    }
}

maybe_bool frombool(bool b)
{
    return b ? MB_TRUE : MB_FALSE;
}

const string maybe_to_string(const maybe_bool mb)
{
    switch (mb)
    {
    case MB_TRUE:
        return "true";
    case MB_FALSE:
        return "false";
    case MB_MAYBE:
    default:
        return "maybe";
    }
}
