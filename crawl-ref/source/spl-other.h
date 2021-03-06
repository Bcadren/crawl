#pragma once

#include "god-type.h"
#include "mount.h"
#include "spl-cast.h"

spret cast_projected_noise(int pow, bool fail, coord_def pos);
spret cast_sublimation_of_blood(int pow, bool fail);
spret cast_death_channel(int pow, god_type god, bool fail);

enum class recall_t
{
    spell,
    yred,
    beogh,
};

struct passwall_path
{
    passwall_path(const actor &act, const coord_def& dir, const int max_range);
    coord_def start;
    coord_def delta;
    int range;
    bool dest_found;

    coord_def actual_dest;
    vector<coord_def> path;

    bool spell_succeeds() const;
    int max_walls() const;
    int actual_walls() const;
    bool is_valid(string *fail_msg) const;
    bool check_moveto() const;
    vector <coord_def> possible_dests() const;
};

spret cast_recall(bool fail);
void start_recall(recall_t type);
void recall_orders(monster *mons);
bool try_recall(mid_t mid);
void do_recall(int time);
void end_recall();

spret cast_passwall(const coord_def& delta, int pow, bool fail);
spret cast_intoxicate(int pow, bool fail);
spret cast_darkness(int pow, bool fail);
spret cast_SMD(const coord_def& target, int pow, bool fail);
