/**
 * @file
 * @brief Functions for handling player mutations.
**/

#pragma once

#include <string>
#include <vector>

#include "equipment-type.h"
#include "mutation-type.h"

using std::vector;

class formatted_string;

enum class mutation_activity_type
{
    INACTIVE, // form-based mutations in most forms
    PARTIAL,  // scales on statues
    FULL,     // other mutations
};

enum mutation_permanence_class
{
    // Temporary mutations wear off after awhile
    MUTCLASS_TEMPORARY,
    // Normal mutations, permanent unless cured
    MUTCLASS_NORMAL,
    // Innate, permanent traits, like draconian breath
    MUTCLASS_INNATE,
    // Miscast-caused mutations, longer-lasting temporary
    MUTCLASS_MISCAST,
};

void init_mut_index();

bool is_body_facet(mutation_type mut);
bool is_slime_mutation(mutation_type mut);
bool undead_mutation_rot(bool god_gift);
bool suppress_mutation(bool temp = false, bool zin = false, bool check = false);

bool mutate(mutation_type which_mutation, const string &reason,
            bool failMsg = true,
            bool force_mutation = false, bool god_gift = false,
            bool beneficial = false,
            mutation_permanence_class mutclass = MUTCLASS_NORMAL);

void display_mutations();
int mut_check_conflict(mutation_type mut, bool innate_only = false);
mutation_activity_type mutation_activity_level(mutation_type mut);
string describe_mutations(bool center_title);
int get_mutation_cap(mutation_type mut);
void validate_mutations(bool debug_msg=false);

string describe_breath(bool gain = false);
bool delete_mutation(mutation_type which_mutation, const string &reason,
                     bool failMsg = true,
                     bool force_mutation = false, bool god_gift = false,
                     bool disallow_mismatch = false);

bool delete_all_mutations(const string &reason);
bool remove_slime_mutations();
bool is_valid_mutation(mutation_type mut);

void display_mutation_name(mutation_type mut, string &name);
const char* mutation_name(mutation_type mut, bool allow_category = false);
const char* category_mutation_name(mutation_type mut);
mutation_type mutation_from_name(string name, bool allow_category, vector<mutation_type> *partial_matches = nullptr);
mutation_type get_scales();

string mut_upgrade_summary(mutation_type mut);
int mutation_max_levels(mutation_type mut);
string mutation_desc(mutation_type which_mutat, int level = -1,
                          bool colour = false, bool is_sacrifice = false);

void roll_demonspawn_mutations();

void draconian_setup();

bool perma_mutate(mutation_type which_mut, int how_much, const string &reason);
bool temp_mutate(mutation_type which_mut, const string &reason);
bool miscast_mutate(mutation_type which_mut, int xp, const string &reason);
int temp_mutation_roll(int base_xp = 500);
bool temp_mutation_wanes();

void check_demonic_guardian();
void check_monster_detect();
bool physiology_mutation_conflict(mutation_type mutat, bool ds_roll = false);
int augmentation_amount();
void reset_powered_by_death_duration();

bool delete_all_temp_mutations(const string &reason);
bool delete_temp_mutation();
bool delete_miscast_mutation(mutation_type which_mut);
