/**
 * @file
 * @brief Monsters' initial equipment.
**/

#pragma once

#include "item-def.h"
#include "monster-type.h"

class monster;

item_def* make_item_for_monster(monster* mons, object_class_type base, int subtype,
    int level, bool on_spawn = true, int allow_uniques = 0, iflags_t flags = 0);
void give_specific_item(monster* mon, const item_def& tpl, bool on_spawn = true);
void give_specific_item(monster* mon, int thing, bool on_spawn = true);
void give_item(monster *mon, int level_number, bool mons_summoned);
int make_mons_weapon(monster_type mtyp, int level, bool melee_only = false);
void give_weapon(monster *mon, int level_number);
int make_mons_armour(monster_type mtyp, int level);
void give_armour(monster *mon, int level_number = -1);
void give_shield(monster *mon, int level_number = -1);
void give_jewels(monster *mon, int level_number = -1);
