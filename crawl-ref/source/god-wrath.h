/**
 * @file
 * @brief Divine retribution.
**/

#pragma once

#include "god-type.h"

bool divine_retribution(god_type god, bool no_bonus = false, bool force = false);
void reduce_xp_penance(god_type god, int amount);

void maybe_gozag_incite(vector<monster*> monsters);
void beogh_idol_revenge();
void gozag_incite(monster *mon);

bool drain_wands();

bool spell_retribution(spell_type spell, god_type god, const char* message = nullptr);
