#pragma once

#include "ac-type.h"
#include "monster-type.h"
#include "spl-cast.h"

enum class mount_type
{
    none,
    drake,
    spider,
    hydra,
    slime
};

void damage_mount(int amount);
void dismount();
void cure_mount_debuffs();
bool mount_hit();
int apply_mount_ac(int amount, int max_dmg = 0, ac_type type = ac_type::normal);
monster_type mount_mons();
bool drain_mount(int strength);
bool miasma_mount();
void slow_mount(int duration);
void rot_mount(int amount, bool needs_message = true);
int mount_statuses();
int mount_hd();
bool mount_submerged();
int mount_ac();
int mount_gdr();
int mount_regen();
int mount_poison_survival();

spret gain_mount(mount_type mount, int pow, bool fail);