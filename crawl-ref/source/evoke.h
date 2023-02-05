/**
 * @file
 * @brief Functions for using some of the wackier inventory items.
**/

#pragma once

bool skill_has_manual(skill_type skill);
string manual_skill_names(bool short_text=false);

void wind_blast(actor* agent, int pow, coord_def target, int source = 0);

bool evoke_check(int slot, bool quiet = false);
bool evoke_item(int slot = -1);
int num_evoker_elementals(int surge = 0);
int wand_mp_cost();
void zap_wand(int slot = -1);

void shadow_lantern_effect();
void expire_lantern_shadows();
void black_drac_breath();
