/**
 * @file
 * @brief Functions for handling player mutations.
**/

#include "AppHdr.h"

#include "mutation.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include "ability.h"
#include "cio.h"
#include "coordit.h"
#include "dactions.h"
#include "delay.h"
#include "english.h"
#include "env.h"
#include "food.h"
#include "god-abil.h"
#include "god-passive.h"
#include "hints.h"
#include "item-prop.h"
#include "items.h"
#include "libutil.h"
#include "menu.h"
#include "message.h"
#include "mon-place.h"
#include "notes.h"
#include "output.h"
#include "player-equip.h" // lose_permafly_source
#include "player-stats.h"
#include "religion.h"
#include "skills.h"
#include "species_mutation_messaging.h"
#include "state.h"
#include "stringutil.h"
#include "transform.h"
#include "unicode.h"
#include "xom.h"

using namespace ui;

static bool _delete_single_mutation_level(mutation_type mutat, const string &reason, bool transient);

struct body_facet_def
{
    equipment_type eq;
    mutation_type mut;
};

struct facet_def
{
    int tier;
    mutation_type muts[3];
    int when[3];
};

struct demon_mutation_info
{
    mutation_type mut;
    int when;
    int facet;

    demon_mutation_info(mutation_type m, int w, int f)
        : mut(m), when(w), facet(f) { }
};

enum class mutflag
{
    good    = 1 << 0, // used by benemut etc
    bad     = 1 << 1, // used by malmut etc
    jiyva   = 1 << 2, // jiyva-only muts
    qazlal  = 1 << 3, // qazlal wrath
    xom     = 1 << 4, // xom being xom

    last    = xom
};
DEF_BITFIELD(mutflags, mutflag, 4);
COMPILE_CHECK(mutflags::exponent(mutflags::last_exponent) == mutflag::last);

#include "mutation-data.h"

static const body_facet_def _body_facets[] =
{
    //{ EQ_NONE, MUT_FANGS },
    { EQ_HELMET, MUT_HORNS },
    { EQ_HELMET, MUT_ANTENNAE },
    //{ EQ_HELMET, MUT_BEAK },
    { EQ_GLOVES, MUT_CLAWS },
    { EQ_BOOTS, MUT_HOOVES },
    { EQ_BOOTS, MUT_TALONS }
};

/**
 * Conflicting mutation pairs. Entries are symmetric (so if A conflicts
 * with B, B conflicts with A in the same way).
 *
 * The third value in each entry means:
 *   0: If the new mutation is forced, remove all levels of the old
 *      mutation. Either way, keep scanning for more conflicts and
 *      do what they say (accepting the mutation if there are no
 *      further conflicts).
 *
 *  -1: If the new mutation is forced, remove all levels of the old
 *      mutation and scan for more conflicts. If it is not forced,
 *      fail at giving the new mutation.
 *
 *   1: If the new mutation is temporary, just allow the conflict.
 *      Otherwise, trade off: delete one level of the old mutation,
 *      don't give the new mutation, and consider it a success.
 *
 * It makes sense to have two entries for the same pair, one with value 0
 * and one with 1: that would replace all levels of the old mutation if
 * forced, or a single level if not forced. However, the 0 entry must
 * precede the 1 entry; so if you re-order this list, keep all the 0s
 * before all the 1s.
 */
static const int conflict[][3] =
{
    { MUT_REGENERATION,        MUT_SLOW_METABOLISM,         0},
    { MUT_REGENERATION,        MUT_INHIBITED_REGENERATION,  0},
    { MUT_ACUTE_VISION,        MUT_IMPAIRED_VISION,         0},
    { MUT_FAST,                MUT_SLOW,                    0},
#if TAG_MAJOR_VERSION == 34
    { MUT_STRONG_STIFF,        MUT_FLEXIBLE_WEAK,           1},
#endif
    { MUT_STRONG,              MUT_WEAK,                    1},
    { MUT_CLEVER,              MUT_DOPEY,                   1},
    { MUT_AGILE,               MUT_CLUMSY,                  1},
    { MUT_ROBUST,              MUT_FRAIL,                   1},
    { MUT_HIGH_MAGIC,          MUT_LOW_MAGIC,               1},
    { MUT_WILD_MAGIC,          MUT_SUBDUED_MAGIC,           1},
    { MUT_CARNIVOROUS,         MUT_HERBIVOROUS,             1},
    { MUT_SLOW_METABOLISM,     MUT_FAST_METABOLISM,         1},
    { MUT_REGENERATION,        MUT_INHIBITED_REGENERATION,  1},
    { MUT_ACUTE_VISION,        MUT_IMPAIRED_VISION,         1},
    { MUT_BERSERK,             MUT_CLARITY,                 1},
    { MUT_INSUBSTANTIAL,       MUT_THIN_SKELETAL_STRUCTURE -1},
    { MUT_INSUBSTANTIAL,       MUT_LARGE_BONE_PLATES       -1},
    { MUT_SILENT_CAST,         MUT_SHOUTITUS               -1},
    { MUT_SILENT_CAST,         MUT_IMPAIRED_VISION         -1},
    { MUT_FAST,                MUT_SLOW,                    1},
    { MUT_FANGS,               MUT_BEAK,                   -1},
    { MUT_ANTENNAE,            MUT_HORNS,                  -1}, // currently overridden by physiology_mutation_conflict
    { MUT_HOOVES,              MUT_TALONS,                 -1}, // currently overridden by physiology_mutation_conflict
    { MUT_TRANSLUCENT_SKIN,    MUT_CAMOUFLAGE,             -1},
    { MUT_MUTATION_RESISTANCE, MUT_EVOLUTION,              -1},
    { MUT_ANTIMAGIC_BITE,      MUT_ACIDIC_BITE,            -1},
    { MUT_HEAT_RESISTANCE,     MUT_HEAT_VULNERABILITY,     -1},
    { MUT_COLD_RESISTANCE,     MUT_COLD_VULNERABILITY,     -1},
    { MUT_SHOCK_RESISTANCE,    MUT_SHOCK_VULNERABILITY,    -1},
    { MUT_MAGIC_RESISTANCE,    MUT_MAGICAL_VULNERABILITY,  -1},
    { MUT_NO_REGENERATION,     MUT_INHIBITED_REGENERATION, -1},
    { MUT_NO_REGENERATION,     MUT_REGENERATION,           -1},
};

equipment_type beastly_slot(int mut)
{
    switch (mut)
    {
    case MUT_HORNS:
    case MUT_ANTENNAE:
    // Not putting MUT_BEAK here because it doesn't conflict with the other two.
        return EQ_HELMET;
    case MUT_CLAWS:
        return EQ_GLOVES;
    case MUT_HOOVES:
    case MUT_TALONS:
    case MUT_TENTACLE_SPIKE:
        return EQ_BOOTS;
    default:
        return EQ_NONE;
    }
}

static bool _mut_has_use(const mutation_def &mut, mutflag use)
{
    return bool(mut.uses & use);
}

#define MUT_BAD(mut) _mut_has_use((mut), mutflag::bad)
#define MUT_GOOD(mut) _mut_has_use((mut), mutflag::good)

static int _mut_weight(const mutation_def &mut, mutflag use)
{
    switch (use)
    {
        case mutflag::jiyva:
        case mutflag::qazlal:
        case mutflag::xom:
            return 1;
        case mutflag::good:
        case mutflag::bad:
        default:
            return mut.weight;
    }
}

static int mut_index[NUM_MUTATIONS];
static int category_mut_index[MUT_NON_MUTATION - CATEGORY_MUTATIONS];
static map<mutflag, int> total_weight;

void init_mut_index()
{
    total_weight.clear();
    for (int i = 0; i < NUM_MUTATIONS; ++i)
        mut_index[i] = -1;

    for (unsigned int i = 0; i < ARRAYSZ(mut_data); ++i)
    {
        const mutation_type mut = mut_data[i].mutation;
        ASSERT_RANGE(mut, 0, NUM_MUTATIONS);
        ASSERT(mut_index[mut] == -1);
        mut_index[mut] = i;
        for (const auto flag : mutflags::range())
        {
            if (_mut_has_use(mut_data[i], flag))
                total_weight[flag] += _mut_weight(mut_data[i], flag);
        }
    }

    // this is all a bit silly but ok
    for (int i = 0; i < MUT_NON_MUTATION - CATEGORY_MUTATIONS; ++i)
        category_mut_index[i] = -1;

    for (unsigned int i = 0; i < ARRAYSZ(category_mut_data); ++i)
    {
        const mutation_type mut = category_mut_data[i].mutation;
        ASSERT_RANGE(mut, CATEGORY_MUTATIONS, MUT_NON_MUTATION);
        ASSERT(category_mut_index[mut-CATEGORY_MUTATIONS] == -1);
        category_mut_index[mut-CATEGORY_MUTATIONS] = i;
    }
}

static const mutation_def& _get_mutation_def(mutation_type mut)
{
    ASSERT_RANGE(mut, 0, NUM_MUTATIONS);
    ASSERT(mut_index[mut] != -1);
    return mut_data[mut_index[mut]];
}

/*
 * Get the max number of possible levels for mutation `mut`. This is typically 1 or 3.
 *
 * @return the mutation cap.
 */
int get_mutation_cap(mutation_type mut)
{
    return _get_mutation_def(mut).levels;
}

static const mutation_category_def& _get_category_mutation_def(mutation_type mut)
{
    ASSERT_RANGE(mut, CATEGORY_MUTATIONS, MUT_NON_MUTATION);
    ASSERT(category_mut_index[mut-CATEGORY_MUTATIONS] != -1);
    return category_mut_data[category_mut_index[mut-CATEGORY_MUTATIONS]];
}

static bool _is_valid_mutation(mutation_type mut)
{
    return mut >= 0 && mut < NUM_MUTATIONS && mut_index[mut] != -1;
}

static const mutation_type _all_scales[] =
{
    MUT_DISTORTION_FIELD,           MUT_ICY_BLUE_SCALES,
    MUT_IRIDESCENT_SCALES,          MUT_LARGE_BONE_PLATES,
    MUT_MOLTEN_SCALES,
#if TAG_MAJOR_VERSION == 34
    MUT_ROUGH_BLACK_SCALES,
#endif
    MUT_RUGGED_BROWN_SCALES,        MUT_SLIMY_GREEN_SCALES,
    MUT_THIN_METALLIC_SCALES,       MUT_THIN_SKELETAL_STRUCTURE,
    MUT_YELLOW_SCALES,              MUT_STURDY_FRAME,
    MUT_SANGUINE_ARMOUR,
};

static bool _is_covering(mutation_type mut)
{
    return find(begin(_all_scales), end(_all_scales), mut) != end(_all_scales);
}

bool is_body_facet(mutation_type mut)
{
    return any_of(begin(_body_facets), end(_body_facets),
                  [=](const body_facet_def &facet)
                  { return facet.mut == mut; });
}

/*
 * The degree to which `mut` is suppressed by the current form.
 *
 * @param mut  the mutation to check.
 *
 * @return  mutation_activity_type::FULL: completely available.
 *          mutation_activity_type::PARTIAL: partially suppressed.
 *          mutation_activity_type::INACTIVE: completely suppressed.
 */
mutation_activity_type mutation_activity_level(mutation_type mut)
{
    // First make sure the player's form permits the mutation.
    if (!form_keeps_mutations())
    {
        if (you.form == transformation::dragon)
        {
            if (mut == MUT_DRACONIAN_DEFENSE)
                return mutation_activity_type::FULL;
            if (mut == MUT_DRACONIAN_ENHANCER)
                return mutation_activity_type::FULL;
            if (mut == MUT_ACIDIC_BITE && you.drac_colour == DR_LIME)
                return mutation_activity_type::FULL;
            if (mut == MUT_STINGER && you.drac_colour == DR_GREEN)
                return mutation_activity_type::FULL;
        }
        // Vampire bats keep their fangs.
        if (you.form == transformation::bat
            && you.species == SP_VAMPIRE
            && mut == MUT_FANGS)
        {
            return mutation_activity_type::FULL;
        }
        // Dex and HP changes are kept in all forms.
#if TAG_MAJOR_VERSION == 34
        if (mut == MUT_ROUGH_BLACK_SCALES)
            return mutation_activity_type::PARTIAL;
#endif
        if (mut == MUT_RUGGED_BROWN_SCALES)
            return mutation_activity_type::PARTIAL;
        else if (_get_mutation_def(mut).form_based)
            return mutation_activity_type::INACTIVE;
    }

    if (you.form == transformation::statue)
    {
        // Statues get all but the AC benefit from scales, but are not affected
        // by other changes in body material or speed.
        switch (mut)
        {
        case MUT_SLIME:
        case MUT_TOUGH_SKIN:
        case MUT_SHAGGY_FUR:
        case MUT_FAST:
        case MUT_SLOW:
        case MUT_IRIDESCENT_SCALES:
            return mutation_activity_type::INACTIVE;
#if TAG_MAJOR_VERSION == 34
        case MUT_ROUGH_BLACK_SCALES:
#endif
        case MUT_RUGGED_BROWN_SCALES:
            return mutation_activity_type::PARTIAL;
        case MUT_YELLOW_SCALES:
        case MUT_ICY_BLUE_SCALES:
        case MUT_MOLTEN_SCALES:
        case MUT_SLIMY_GREEN_SCALES:
        case MUT_THIN_METALLIC_SCALES:
            return you.get_base_mutation_level(mut) > 2 ? mutation_activity_type::PARTIAL :
                                                          mutation_activity_type::INACTIVE;
        default:
            break;
        }
    }

    //XXX: Should this make claws inactive too?
    if (you.form == transformation::blade_hands && mut == MUT_PAWS)
        return mutation_activity_type::INACTIVE;

    if ((you.form == transformation::tree || you.attribute[ATTR_ROOTED])
        && (mut == MUT_BLINK || mut == MUT_TELEPORT))
    {
        return mutation_activity_type::INACTIVE;
    }
#if TAG_MAJOR_VERSION == 34
    if ((you_worship(GOD_PAKELLAS) || player_under_penance(GOD_PAKELLAS))
         && (mut == MUT_MANA_LINK || mut == MUT_MANA_REGENERATION))
    {
        return mutation_activity_type::INACTIVE;
    }
#endif

    if (!form_can_bleed(you.form) && mut == MUT_SANGUINE_ARMOUR)
        return mutation_activity_type::INACTIVE;

    if (mut == MUT_DEMONIC_GUARDIAN && you.get_mutation_level(MUT_NO_LOVE))
        return mutation_activity_type::INACTIVE;

    return mutation_activity_type::FULL;
}

// Counts of various statuses/types of mutations from the current/most
// recent call to describe_mutations. TODO: eliminate
static int _num_full_suppressed = 0;
static int _num_part_suppressed = 0;
static int _num_transient = 0;

static string _annotate_form_based(string desc, bool suppressed)
{
    if (suppressed)
    {
        desc = "<darkgrey>((" + desc + "))</darkgrey>";
        ++_num_full_suppressed;
    }

    return desc + "\n";
}

/*
 * Does the player have mutation `mut` with at least one temporary level?
 *
 * Reminder: temporary mutations can coexist with innate or normal mutations.
 */
bool player::has_temporary_mutation(mutation_type mut) const
{
    return you.temp_mutation[mut] > 0;
}

/*
 * Does the player have mutation `mut` with at least one innate level?
 *
 * Reminder: innate mutations can coexist with temporary or normal mutations.
 */
bool player::has_innate_mutation(mutation_type mut) const
{
    return you.innate_mutation[mut] > 0;
}

/*
 * How much of mutation `mut` does the player have? This ignores form changes.
 * If all three bool arguments are false, this should always return 0.
 *
 * @param temp   include temporary mutation levels. defaults to true.
 * @param innate include innate mutation levels. defaults to true.
 * @param normal include normal (non-temp, non-innate) mutation levels. defaults to true.
 *
 * @return the total levels of the mutation.
 */
int player::get_base_mutation_level(mutation_type mut, bool innate, bool temp, bool normal) const
{
    ASSERT_RANGE(mut, 0, NUM_MUTATIONS);
    // you.mutation stores the total levels of all mutations
    int level = you.mutation[mut];
    if (!temp)
        level -= you.temp_mutation[mut];
    if (!innate)
        level -= you.innate_mutation[mut];
    if (!normal)
        level -= (you.mutation[mut] - (you.temp_mutation[mut] + you.innate_mutation[mut]));
    ASSERT(level >= 0);
    return level;
}

/*
 * How much of mutation `mut` does the player have innately?
 *
 */
int player::get_innate_mutation_level(mutation_type mut) const
{
    ASSERT_RANGE(mut, 0, NUM_MUTATIONS);
    return you.innate_mutation[mut];
}

/*
 * How much of mutation `mut` does the player have temporarily?
 *
 */
int player::get_temp_mutation_level(mutation_type mut) const
{
    ASSERT_RANGE(mut, 0, NUM_MUTATIONS);
    return you.temp_mutation[mut];
}

/*
 * Get the current player mutation level for `mut`, possibly incorporating information about forms.
 * See the other version of this function for the canonical usage of `minact`; some forms such as scale mutations
 * have different thresholds depending on the purpose and form and so will call this directly (e.g. ac
 * but not resistances are suppressed in statueform.)
 *
 * @param mut           the mutation to check
 * @param minact        the minimum activity level needed for the mutation to count as non-suppressed.
 *
 * @return a mutation level, 0 if the mutation doesn't exist or is suppressed.
 */
int player::get_mutation_level(mutation_type mut, mutation_activity_type minact) const
{
    ASSERT_RANGE(mut, 0, NUM_MUTATIONS);
    if (mutation_activity_level(mut) < minact)
        return 0;
    return get_base_mutation_level(mut, true, true);
}

/*
 * Get the current player mutation level for `mut`, possibly incorporating information about forms.
 *
 * @param mut           the mutation to check
 * @param check_form    whether to incorporate suppression from forms. Defaults to true.
 *
 * @return a mutation level, 0 if the mutation doesn't exist or is suppressed.
 */
int player::get_mutation_level(mutation_type mut, bool check_form) const
{
    return get_mutation_level(mut, check_form ? mutation_activity_type::PARTIAL :
                                                            mutation_activity_type::INACTIVE);
}

/*
 * Does the player have mutation `mut` in some form?
 */
bool player::has_mutation(mutation_type mut, bool check_form) const
{
    return get_mutation_level(mut, check_form) > 0;
}

/*
 * Test the validity of the player mutation structures, using ASSERTs.
 * Will crash on a failure.
 *
 * @debug_msg whether to output diagnostic `dprf`s in the process.
 */
void validate_mutations(bool debug_msg)
{
    if (debug_msg)
        dprf("Validating player mutations");
    int total_temp = 0;

    for (int i = 0; i < NUM_MUTATIONS; i++)
    {
        mutation_type mut = static_cast<mutation_type>(i);
        if (debug_msg && you.mutation[mut] > 0)
        {
            dprf("mutation %s: total %d innate %d temp %d",
                mutation_name(mut), you.mutation[mut],
                you.innate_mutation[mut], you.temp_mutation[mut]);
        }
        ASSERT(you.get_base_mutation_level(mut) == you.mutation[mut]);
        ASSERT(you.mutation[i] >= you.innate_mutation[mut] + you.temp_mutation[mut]);
        total_temp += you.temp_mutation[mut];

        const mutation_def& mdef = _get_mutation_def(mut);
        ASSERT(you.mutation[mut] <= mdef.levels);

        // reconstruct what the innate mutations should be based on Ds mutation schedule
        // TODO generalize to all innate muts
        if (you.species == SP_DEMONSPAWN || you.char_class == JOB_DEMONSPAWN)
        {
            bool is_trait = false;
            int trait_level = 0;
            // If the player has sacrificed xp, use the pre-sac xl; sac xp
            // doesn't remove Ds mutations.
            // You can still trick wizmode into crashing here.
            const int check_xl = (you.get_mutation_level(MUT_INEXPERIENCED)
                            && you.max_level <= you.get_experience_level() + 2)
                                ? you.max_level
                                : you.get_experience_level();
            for (player::demon_trait trait : you.demonic_traits)
            {
                if (trait.mutation == mut)
                {
                    is_trait = true;
                    if (check_xl >= trait.level_gained)
                        trait_level += 1;
                }
            }

            if (debug_msg && is_trait)
            {
                dprf("scheduled innate for %s: %d, actual %d", mutation_name(mut),
                     trait_level, you.innate_mutation[mut]);
            }
            if (is_trait)
                ASSERT(you.innate_mutation[mut] == trait_level);
        }
    }
    ASSERT(total_temp == you.attribute[ATTR_TEMP_MUTATIONS]);
}

string describe_breath(bool gain)
{
    ostringstream ostr;
    if (gain)
        ostr << "You lose your ability to breathe basic magical darts and instead you can breathe ";
    else
        ostr << "You can breathe ";

    if (you.form == transformation::statue)
    {
        ostr << "metallic splinters.";
        return ostr.str();
    }

    switch (you.drac_colour)
    {
    case DR_BROWN:          ostr << "rudimentary magical darts.";   break;
    case DR_BLOOD:          ostr << "clouds of vampiric fog.";      break;
    case DR_BLUE:           ostr << "wild blasts of lightning";     break;
    case DR_BONE:           ostr << "shards of bone.";              break;
    case DR_BLACK:          ostr << "bolts of negative energy.";    break;
    case DR_CYAN:           ostr << "strong gales of wind.";        break;
    case DR_GREEN:          ostr << "clouds of noxious fumes.";     break;
    case DR_LIME:           ostr << "balls of acidic spit.";        break;
    case DR_MAGENTA:        ostr << "clouds of thick fog.";         break;
    case DR_OLIVE:          ostr << "clouds of foul miasma.";       break;
    case DR_PEARL:          ostr << "bursts of cleansing flame.";   break;
    case DR_PINK:           ostr << "friendly butterflies.";        break;
    case DR_PLATINUM:       ostr << "blasts of radiation.";         break;
    case DR_PURPLE:         ostr << "bolts of dispelling energy.";  break;
    case DR_WHITE:          ostr << "puffs of frost.";              break;
    case DR_SCINTILLATING:  ostr << "bolts of chaotic energy.";     break;
    case DR_GOLDEN:         ostr << "your choice of flames, frost or noxious fumes.";                           break;
    case DR_RED:            ostr << "puffs of flames, which leave flaming clouds in their wake.";               break;
    case DR_SILVER:         ostr << "silver splinters, which deal bonus damage to chaotic creatures.";          break;
    case DR_TEAL:           ostr << "blasts of ghostly flames, which heal the undead and damage the living.";   break;
    default:                ostr << "buggy bugs of bugging";        break;
    }

    return ostr.str();
}

string describe_mutations(bool drop_title)
{
#ifdef DEBUG
    validate_mutations(true);
#endif
    string result;

    _num_full_suppressed = _num_part_suppressed = 0;
    _num_transient = 0;

    if (!drop_title)
    {
        result += "<white>";
        result += "Innate Abilities, Weirdness & Mutations";
        result += "</white>\n\n";
    }

    result += "<lightblue>";
    const string old_result = result;

    // Innate abilities which haven't been implemented as mutations yet.
    // TODO: clean these up with respect to transformations. Currently
    // we handle only Naga/Draconian AC and Yellow Draconian rAcid.
    for (const string& str : fake_mutations(you.species, false))
    {
        if (you.species == SP_MINOTAUR)
            result += _annotate_form_based(str, !form_keeps_mutations());
        else
            result += str + "\n";
    }

    if (you.racial_ac(false) >= 100)
    {
        const string race_string = you.species == SP_DRACONIAN ? string(scale_type()) + " scales" :
                                   you.species == SP_NAGA      ? "serpentine skin"                :
                                   you.species == SP_GARGOYLE  ? "stone body"                     :
                                   you.species == SP_LIGNIFITE ? "bark" : "";                

        const string job_string = you.char_class == JOB_CENTAUR ? "horsehide" : 
                                  you.char_class == JOB_NAGA    ? "serpentine skin" : "";
        const bool plural = race_string.length() && job_string.length();

        result += _annotate_form_based(
            make_stringf("Your %s%s%s %s resilient. (AC +%d)", race_string.c_str(), 
                plural ? " and " : "", 
                job_string.c_str(), plural || you.species == SP_DRACONIAN ? "are" : "is",
                you.racial_ac(false) / 100),
            player_is_shapechanged()
            && !(species_is_draconian(you.species)
                && you.form == transformation::dragon));
    }

    if (you.species == SP_LIGNIFITE)
    {
        result += _annotate_form_based(
            make_stringf("A tangle of shorter branches protects your body from attack. (+%d SH)",
                you.branch_SH(false)),
            !form_keeps_mutations());
        if (you.experience_level > 12)
            result += _annotate_form_based("You can plant your roots to grant stasis and boost your AC and Regeneration.",
                !form_keeps_mutations());
    }

    if (you.species == SP_OCTOPODE)
    {
        result += _annotate_form_based("You are amphibious.",
            !form_likes_water());

        int tents = you.usable_tentacles(false) + you.num_constricting();

        if (you.mutation[MUT_CONSTRICTING_TAIL] && you.num_constricting())
            tents--;

        const string num_tentacles = number_in_words(tents);
        const string total_tentacles = number_in_words(you.has_tentacles(false));
        result += _annotate_form_based(
            make_stringf("You can wear up to %s rings at the same time.",
                total_tentacles.c_str()),
            !get_form()->slot_available(EQ_RING_FOUR));
        result += _annotate_form_based(
            make_stringf("You can use your tentacles to constrict %s enemies at once.",
                num_tentacles.c_str()),
            !form_keeps_mutations());
    }

    if (you.species != SP_FELID && you.species != SP_FAIRY)
    {
        switch (you.body_size(PSIZE_TORSO, true))
        {
        case SIZE_LITTLE:
            result += "You are very small and have problems with some larger weapons.\n"
                "You are too small for most types of armour.\n";
            break;
        case SIZE_SMALL:
            result += "You are small and have problems with some larger weapons.\n";
            break;
        case SIZE_LARGE:
            result += "You are too large for most types of armour.\n";
            break;
        case SIZE_GIANT:
            result += "Your enormous size allows you to wade in deep water.\n"
                "You are too large for most types of armour.\n"
                "Webs and nets cannot contain a creature of your size.\n";
            break;
        default:
            break;
        }
    }

    if (you.species == SP_DRACONIAN)
    {
        if (you.undead_state(false) == US_UNDEAD)
        {
            result += "Your connection to dragons allows you to use the spell dragon form,\n"
                "despite your undead form not otherwise being capable of transmutation.\n";
        }
        result += _annotate_form_based(describe_breath(), 
                (!form_keeps_mutations() && you.form != transformation::dragon 
                                         && you.form != transformation::lich));
    }

    if (player_res_poison(false, false, false) == 3)
        result += "You are immune to poison.\n";

    result += "</lightblue>";

    // First add (non-removable) inborn abilities and demon powers.
    for (int i = 0; i < NUM_MUTATIONS; i++)
    {
        mutation_type mut_type = static_cast<mutation_type>(i);
        if (you.has_innate_mutation(mut_type))
        {
            result += mutation_desc(mut_type, -1, true,
                ((you.sacrifices[i] != 0) ? true : false));
            result += "\n";
        }
    }

    if (have_passive(passive_t::water_walk))
        result += "<green>You can walk on water.</green>\n";
    else if (you.can_water_walk())
    {
        result += "<lightgreen>You can walk on water until reaching land."
            "</lightgreen>";
    }

    if (have_passive(passive_t::frail)
        || player_under_penance(GOD_HEPLIAKLQANA))
    {
        result += "<lightred>Your life essence is reduced to manifest your ancestor. (-10% HP)"
            "</lightred>\n";
    }

    if (have_passive(passive_t::berserkitis)
        || player_under_penance(GOD_TROG))
    {
        result += "<lightred>You frequently lose your temper in combat. This godly rage surpasses even clarity. (*Rage)</lightred>\n";
    }

    // Now add removable mutations.
    for (int i = 0; i < NUM_MUTATIONS; i++)
    {
        mutation_type mut_type = static_cast<mutation_type>(i);
        if (you.get_base_mutation_level(mut_type, false, false, true) > 0
            && !you.has_innate_mutation(mut_type) && !you.has_temporary_mutation(mut_type))
        {
            result += mutation_desc(mut_type, -1, true);
            result += "\n";
        }
    }

    //Finally, temporary mutations.
    for (int i = 0; i < NUM_MUTATIONS; i++)
    {
        mutation_type mut_type = static_cast<mutation_type>(i);
        if (you.has_temporary_mutation(mut_type))
        {
            result += mutation_desc(mut_type, -1, true);
            result += "\n";
        }
    }

    if (result == old_result + "</lightblue>") // Nothing was added
        result += "You are rather mundane.\n";

    return result;
}

static formatted_string _vampire_Ascreen_footer(bool first_page)
{
    const char *text = first_page ? "<w>Mutations</w>|Blood properties"
                                  : "Mutations|<w>Blood properties</w>";
    const string fmt = make_stringf("[<w>!</w>/<w>^</w>"
#ifdef USE_TILE_LOCAL
            "|<w>Right-click</w>"
#endif
            "]: %s", text);
    return formatted_string::parse_string(fmt);
}

static int _vampire_bloodlessness()
{
    switch (you.hunger_state)
    {
    case HS_ENGORGED:
    case HS_VERY_FULL:
    case HS_FULL:
        return 1;
    case HS_SATIATED:
        return 2;
    case HS_HUNGRY:
    case HS_VERY_HUNGRY:
    case HS_NEAR_STARVING:
        return 3;
    case HS_STARVING:
    case HS_FAINTING:
        return 4;
    }
    die("bad hunger state %d", you.hunger_state);
}

static string _display_vampire_attributes()
{
    ASSERT(you.species == SP_VAMPIRE);

    string result;

    const int lines = 12;
    string column[lines][5] =
    {
        {"                     ", "<green>Full</green>       ", "Satiated   ", "<yellow>Thirsty</yellow>    ", "<lightred>Bloodless</lightred>"},
                                 //Full       Satiated      Thirsty         Bloodless
        {"Metabolism           ", "fast       ", "normal     ", "slow       ", "none  "},

        {"Regeneration         ", "fast       ", "normal     ", "slow       ", "none  "},

        {"Stealth boost        ", "none       ", "none       ", "minor      ", "major "},

        {"Hunger costs         ", "full       ", "full       ", "halved     ", "none  "},

        {"\n<w>Resistances</w>\n"
         "Poison resistance    ", "           ", "           ", "+          ", "immune"},

        {"Cold resistance      ", "           ", "           ", "+          ", "++    "},

        {"Negative resistance  ", "           ", " +         ", "++         ", "+++   "},

        {"Rotting resistance   ", "           ", "           ", "+          ", "+     "},

        {"Torment resistance   ", "           ", "           ", "           ", "+     "},

        {"\n<w>Transformations</w>\n"
         "Bat form             ", "no         ", "yes        ", "yes        ", "yes   "},

        {"Other forms and \n"
         "berserk              ", "yes        ", "yes        ", "no         ", "no    "}
    };

    int current = _vampire_bloodlessness();

    for (int y = 0; y < lines; y++)  // lines   (properties)
    {
        for (int x = 0; x < 5; x++)  // columns (hunger states)
        {
            if (y > 0 && x == current)
                result += "<w>";
            result += column[y][x];
            if (y > 0 && x == current)
                result += "</w>";
        }
        result += "\n";
    }

    trim_string_right(result);
    return result;
}

void display_mutations()
{
    string mutation_s = describe_mutations(true);

    string extra = "";
    if (_num_part_suppressed)
        extra += "<brown>()</brown>  : Partially suppressed.\n";
    if (_num_full_suppressed)
        extra += "<darkgrey>(())</darkgrey>: Completely suppressed.\n";
    if (_num_transient)
        extra += "<magenta>[]</magenta>   : Transient mutations.";

    if (!extra.empty())
    {
        mutation_s += "\n\n\n\n";
        mutation_s += extra;
    }
    trim_string_right(mutation_s);

    auto vbox = make_shared<Box>(Widget::VERT);
    vbox->set_cross_alignment(Widget::STRETCH);

    const char *title_text = "Innate Abilities, Weirdness & Mutations";
    auto title = make_shared<Text>(formatted_string(title_text, WHITE));
    auto title_hbox = make_shared<Box>(Widget::HORZ);
    title_hbox->add_child(move(title));
    title_hbox->set_main_alignment(Widget::CENTER);
    vbox->add_child(move(title_hbox));

    auto switcher = make_shared<Switcher>();

    const string vamp_s = you.species == SP_VAMPIRE ?_display_vampire_attributes() : "N/A";
    const string descs[3] =  { mutation_s, vamp_s };
    for (int i = 0; i < 2; i++)
    {
        auto scroller = make_shared<Scroller>();
        auto text = make_shared<Text>(formatted_string::parse_string(
                descs[static_cast<int>(i)]));
        text->set_wrap_text(true);
        scroller->set_child(text);
        switcher->add_child(move(scroller));
    }

    switcher->current() = 0;
    switcher->set_margin_for_sdl(20, 0, 0, 0);
    switcher->set_margin_for_crt(1, 0, 0, 0);
    switcher->expand_h = false;
    switcher->align_x = Widget::STRETCH;
#ifdef USE_TILE_LOCAL
    switcher->max_size().width = tiles.get_crt_font()->char_width()*80;
#endif
    vbox->add_child(switcher);

    auto bottom = make_shared<Text>(_vampire_Ascreen_footer(true));
    bottom->set_margin_for_sdl(20, 0, 0, 0);
    bottom->set_margin_for_crt(1, 0, 0, 0);
    if (you.species == SP_VAMPIRE)
        vbox->add_child(bottom);

    auto popup = make_shared<ui::Popup>(vbox);

    bool done = false;
    int lastch;
    popup->on_keydown_event([&](const KeyEvent& ev) {
        lastch = ev.key();
        if (you.species == SP_VAMPIRE && (lastch == '!' || lastch == CK_MOUSE_CMD || lastch == '^'))
        {
            int c = 1 - switcher->current();
            switcher->current() = c;
#ifdef USE_TILE_WEB
            tiles.json_open_object();
            tiles.json_write_int("pane", c);
            tiles.ui_state_change("mutations", 0);
#endif
            bottom->set_text(_vampire_Ascreen_footer(c));
        } else
            done = !switcher->current_widget()->on_event(ev);
        return true;
    });

#ifdef USE_TILE_WEB
    tiles.json_open_object();
    tiles.json_write_string("mutations", mutation_s);
    if (you.species == SP_VAMPIRE)
        tiles.json_write_int("vampire", _vampire_bloodlessness());
    tiles.push_ui_layout("mutations", 1);
#endif

    ui::run_layout(move(popup), done);

#ifdef USE_TILE_WEB
    tiles.pop_ui_layout();
#endif
}

static int _calc_mutation_amusement_value(mutation_type which_mutation)
{
    int amusement = 12 * (11 - _get_mutation_def(which_mutation).weight);

    if (MUT_GOOD(mut_data[which_mutation]))
        amusement /= 2;
    else if (MUT_BAD(mut_data[which_mutation]))
        amusement *= 2;
    // currently is only ever one of these, but maybe that'll change?

    return amusement;
}

static bool _accept_mutation(mutation_type mutat, bool ignore_weight = false)
{
    if (!_is_valid_mutation(mutat))
        return false;

    if (physiology_mutation_conflict(mutat))
        return false;

    const mutation_def& mdef = _get_mutation_def(mutat);

    if (you.get_base_mutation_level(mutat) >= mdef.levels)
        return false;

    if (ignore_weight)
        return true;

    // bias towards adding (non-innate) levels to existing innate mutations.
    const int weight = mdef.weight + you.get_innate_mutation_level(mutat);

    // Low weight means unlikely to choose it.
    return x_chance_in_y(weight, 10);
}

static mutation_type _get_mut_with_use(mutflag mt)
{
    const int tweight = lookup(total_weight, mt, 0);
    ASSERT(tweight);

    int cweight = random2(tweight);
    for (const mutation_def &mutdef : mut_data)
    {
        if (!_mut_has_use(mutdef, mt))
            continue;

        cweight -= _mut_weight(mutdef, mt);
        if (cweight >= 0)
            continue;

        return mutdef.mutation;
    }

    die("Error while selecting mutations");
}

static mutation_type _get_random_slime_mutation()
{
    return _get_mut_with_use(mutflag::jiyva);
}

static mutation_type _delete_random_slime_mutation()
{
    mutation_type mutat;

    while (true)
    {
        mutat = _get_random_slime_mutation();

        if (you.get_base_mutation_level(mutat) > 0)
            break;

        if (one_chance_in(500))
        {
            mutat = NUM_MUTATIONS;
            break;
        }
    }

    return mutat;
}

bool is_slime_mutation(mutation_type mut)
{
    return _mut_has_use(mut_data[mut_index[mut]], mutflag::jiyva);
}

static mutation_type _get_random_xom_mutation()
{
    mutation_type mutat = NUM_MUTATIONS;

    do
    {
        mutat = static_cast<mutation_type>(random2(NUM_MUTATIONS));

        if (one_chance_in(1000))
            return NUM_MUTATIONS;
        else if (one_chance_in(5))
            mutat = _get_mut_with_use(mutflag::xom);
    }
    while (!_accept_mutation(mutat, false));

    return mutat;
}

static mutation_type _get_random_qazlal_mutation()
{
    return _get_mut_with_use(mutflag::qazlal);
}

static mutation_type _get_random_mutation(mutation_type mutclass)
{
    mutflag mt;
    switch (mutclass)
    {
        case RANDOM_MUTATION:
            // maintain an arbitrary ratio of good to bad muts to allow easier
            // weight changes within categories - 60% good seems to be about
            // where things are right now
            mt = x_chance_in_y(3, 5) ? mutflag::good : mutflag::bad;
            break;
        case RANDOM_BAD_MUTATION:
        case RANDOM_CORRUPT_MUTATION:
            mt = mutflag::bad;
            break;
        case RANDOM_GOOD_MUTATION:
            mt = mutflag::good;
            break;
        default:
            die("invalid mutation class: %d", mutclass);
    }

    for (int attempt = 0; attempt < 100; ++attempt)
    {
        mutation_type mut = _get_mut_with_use(mt);
        if (_accept_mutation(mut, true))
            return mut;
    }

    return NUM_MUTATIONS;
}

/**
 * Does the player have a mutation that conflicts with the given mutation?
 *
 * @param mut           A mutation. (E.g. MUT_INHIBITED_REGENERATION, ...)
 * @param innate_only   Whether to only check innate mutations (from e.g. race)
 * @return              The level of the conflicting mutation.
 *                      E.g., if MUT_INHIBITED_REGENERATION is passed in and the
 *                      player has 2 levels of MUT_REGENERATION, 2 will be
 *                      returned.
 *
 *                      No guarantee is offered on ordering if there are
 *                      multiple conflicting mutations with different levels.
 */
int mut_check_conflict(mutation_type mut, bool innate_only)
{
    for (const int (&confl)[3] : conflict)
    {
        if (confl[0] != mut && confl[1] != mut)
            continue;

        const mutation_type confl_mut
           = static_cast<mutation_type>(confl[0] == mut ? confl[1] : confl[0]);

        const int level = you.get_base_mutation_level(confl_mut, true, !innate_only, !innate_only);
        if (level)
            return level;
    }

    return 0;
}

// Tries to give you the mutation by deleting a conflicting
// one, or clears out conflicting mutations if we should give
// you the mutation anyway.
// Return:
//  1 if we should stop processing (success);
//  0 if we should continue processing;
// -1 if we should stop processing (failure).
static int _handle_conflicting_mutations(mutation_type mutation,
                                         bool override,
                                         const string &reason,
                                         bool temp = false)
{
    // If we have one of the pair, delete all levels of the other,
    // and continue processing.
    for (const int (&confl)[3] : conflict)
    {
        for (int j = 0; j <= 1; ++j)
        {
            const mutation_type a = (mutation_type)confl[j];
            const mutation_type b = (mutation_type)confl[1-j];

            if (mutation == a && you.get_base_mutation_level(b) > 0)
            {
                // can never delete innate mutations. For case -1 and 0, fail if there are any, otherwise,
                // make sure there is a non-innate instance to delete.
                if (you.has_innate_mutation(b) &&
                    (confl[2] != 1
                     || you.get_base_mutation_level(b, true, false, false) == you.get_base_mutation_level(b)))
                {
                    dprf("Delete mutation failed: have innate mutation %d at level %d, you.mutation at level %d", b,
                        you.get_innate_mutation_level(b), you.get_base_mutation_level(b));
                    return -1;
                }

                // at least one level of this mutation is temporary
                const bool temp_b = you.has_temporary_mutation(b);

                // confl[2] indicates how the mutation resolution should proceed (see `conflict` a the beginning of this file):
                switch (confl[2])
                {
                case -1:
                    // Fail if not forced, otherwise override.
                    if (!override)
                        return -1;
                case 0:
                    // Ignore if not forced, otherwise override.
                    // All cases but regen:slowmeta will currently trade off.
                    if (override)
                    {
                        while (_delete_single_mutation_level(b, reason, true))
                            ;
                    }
                    break;
                case 1:
                    // If we have one of the pair, delete a level of the
                    // other, and that's it.
                    //
                    // Temporary mutations can co-exist with things they would
                    // ordinarily conflict with. But if both a and b are temporary,
                    // mark b for deletion.
                    if ((temp || temp_b) && !(temp && temp_b))
                        return 0;       // Allow conflicting transient mutations
                    else
                    {
                        _delete_single_mutation_level(b, reason, true);
                        return 1;     // Nothing more to do.
                    }

                default:
                    die("bad mutation conflict resulution");
                }
            }
        }
    }

    return 0;
}

static int _body_covered()
{
    // Check how much of your body is covered by scales, etc.
    // Note: this won't take into account forms, so is only usable for checking in general.
    int covered = 0;

    if (you.species == SP_NAGA || you.char_class == JOB_NAGA)
        covered++;

    if (species_is_draconian(you.species))
        covered += 3;

    for (mutation_type scale : _all_scales)
        covered += you.get_base_mutation_level(scale);

    return covered;
}

bool physiology_mutation_conflict(mutation_type mutat, bool ds_roll)
{
    // If demonspawn, and mutat is a scale, see if they were going
    // to get it sometime in the future anyway; otherwise, conflict.
    if ((you.species == SP_DEMONSPAWN || you.char_class == JOB_DEMONSPAWN)
        && !ds_roll && _is_covering(mutat)
        && find(_all_scales, _all_scales+ARRAYSZ(_all_scales), mutat) !=
                _all_scales+ARRAYSZ(_all_scales))
    {
        return none_of(begin(you.demonic_traits), end(you.demonic_traits),
                       [=](const player::demon_trait &t) {
                           return t.mutation == mutat;});
    }

    if (you.undead_or_demonic())
    {
        if (mutat == MUT_BERSERK || mutat == MUT_POISON_RESISTANCE)
            return true; // Poison Resist is redundant and undeads can't berserk.
    }

    // Don't give felids or trolls claws, etc.
    if (ds_roll && you.innate_mutation[mutat] > 0)
        return true;

    if (you.species == SP_FAIRY && (mutat == MUT_CLAWS || mutat == MUT_HORNS))
        return true;

    // Strict 3-scale limit.
    if (_is_covering(mutat) && _body_covered() >= 3 && !ds_roll)
        return true;

    // Only Nagas and Draconians can get this one.
    if (you.species != SP_NAGA && !species_is_draconian(you.species)
        && you.char_class != JOB_NAGA && mutat == MUT_STINGER)
    {
        return true;
    }

    // Need tentacles to grow something on them.
    if (you.species != SP_OCTOPODE && mutat == MUT_TENTACLE_SPIKE)
        return true;

    // Too squishy for horns.
    if (you.species == SP_OCTOPODE && mutat == MUT_HORNS)
        return true;

    // No feet.
    if (!player_has_feet(false, false)
        && (mutat == MUT_HOOVES || mutat == MUT_TALONS))
    {
        return true;
    }

    // Only nagas can get upgraded poison spit.
    if ((you.species != SP_NAGA || you.char_class == JOB_NAGA) && mutat == MUT_SPIT_POISON)
        return true;

    // Only Draconians (and gargoyles) can get wings.
    if (!species_is_draconian(you.species) && you.species != SP_GARGOYLE
        && mutat == MUT_BIG_WINGS)
    {
        return true;
    }

    // Felids have innate claws, and unlike trolls/ghouls, there are no
    // increases for them.
    if (you.species == SP_FELID  && mutat == MUT_CLAWS)
        return true;

    // Merfolk have no feet in the natural form, and we never allow mutations
    // that show up only in a certain transformation.
    if ((you.species == SP_MERFOLK || you.char_class == JOB_MERFOLK)
        && !ds_roll && (mutat == MUT_TALONS || mutat == MUT_HOOVES))
    {
        return true;
    }

    if (you.species == SP_FORMICID)
    {
        // Formicids have stasis and so prevent mutations that would do nothing.
        // Antennae provides SInv, so acute vision is pointless.
        if (mutat == MUT_BERSERK
            || mutat == MUT_BLINK
            || mutat == MUT_TELEPORT
            || mutat == MUT_ACUTE_VISION)
        {
            return true;
        }
    }

    // Already immune.
    if (you.species == SP_GARGOYLE && mutat == MUT_POISON_RESISTANCE)
        return true;

    // We can't use is_useless_skill() here, since species that can still wear
    // body armour can sacrifice armour skill with Ru.
    if (species_apt(SK_ARMOUR) == UNUSABLE_SKILL
        && (mutat == MUT_DEFORMED || mutat == MUT_STURDY_FRAME))
    {
        return true;
    }

    equipment_type eq_type = EQ_NONE;

    // Mutations of the same slot conflict
    if (is_body_facet(mutat))
    {
        // Find equipment slot of attempted mutation
        for (const body_facet_def &facet : _body_facets)
            if (mutat == facet.mut)
                eq_type = facet.eq;

        if (eq_type != EQ_NONE)
        {
            for (const body_facet_def &facet : _body_facets)
            {
                if (eq_type == facet.eq
                    && mutat != facet.mut
                    && you.get_base_mutation_level(facet.mut))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

static const char* _stat_mut_desc(mutation_type mut, bool gain)
{
    stat_type stat = STAT_STR;
    bool positive = gain;
    switch (mut)
    {
    case MUT_WEAK:
        positive = !positive;
    case MUT_STRONG:
        stat = STAT_STR;
        break;

    case MUT_DOPEY:
        positive = !positive;
    case MUT_CLEVER:
        stat = STAT_INT;
        break;

    case MUT_CLUMSY:
        positive = !positive;
    case MUT_AGILE:
        stat = STAT_DEX;
        break;

    default:
        die("invalid stat mutation: %d", mut);
    }
    return stat_desc(stat, positive ? SD_INCREASE : SD_DECREASE);
}

/**
 * Do a resistance check for the given mutation permanence class.
 * Does not include divine intervention!
 *
 * @param mutclass The type of mutation that is checking resistance
 * @param beneficial Is the mutation beneficial?
 *
 * @return True if a mutation is successfully resisted, false otherwise.
**/
static bool _resist_mutation(mutation_permanence_class mutclass,
                             bool beneficial)
{
    if (you.get_mutation_level(MUT_MUTATION_RESISTANCE) == 3)
        return true;

    bool you_res = you.get_mutation_level(MUT_MUTATION_RESISTANCE);

    // draconian scales:
    if (!you_res && (you.get_mutation_level(MUT_DRACONIAN_DEFENSE, true)))
    {
        if (you.drac_colour == DR_PLATINUM || you.drac_colour == DR_OLIVE || you.drac_colour == DR_SILVER)
            you_res = true;
        if (you.drac_colour == DR_SCINTILLATING && one_chance_in(3))
            you_res = true;
    }

    const int mut_resist_chance = mutclass == MUTCLASS_TEMPORARY ? 2 : 3;
    if (you_res && !one_chance_in(mut_resist_chance))
        return true;

    // To be nice, beneficial mutations go through removable sources of rMut.
    if (you.rmut_from_item() && !beneficial
        && !one_chance_in(mut_resist_chance))
    {
        return true;
    }

    return false;
}

/*
 * Does the player rot instead of mutating?
 * Right now this is coextensive with whether the player is unable to mutate.
 * For most undead, they will never mutate and always rot instead; vampires always mutate and never rot.
 *
 * @return true if so.
 */
bool undead_mutation_rot()
{
    return !you.can_safely_mutate();
}

static species_mutation_message _spmut_msg(mutation_type mutat)
{
    for (unsigned int i = 0; i < spmu_length; ++i)
    {
        if (spmu_data[i].species == you.species)
        {
            if (spmu_data[i].mutation == mutat)
                return spmu_data[i];
        }
    }
    return spmu_data[0];
}

static string _drac_def_msg()
{
    ostringstream ostr;
    ostr << "Your " << scale_type() << " scales ";
    switch (you.drac_colour)
    {
    case DR_BLACK:
        ostr << "make you much more stealthy. (Stealth++)";
        break;
    case DR_BLOOD:
        ostr << "grant partial resistance to hellfire and unholy torment. (rTorm, rHellfire)";
        break;
    case DR_BLUE:
        ostr << "grant you resistance to electric shocks. (rElec)";
        break;
    case DR_BONE: // Special case; no scales.
        return "Your tough skeletal form vastly boosts your defenses. (AC+++)\nHowever; you are also weak to shatter and Lee's Rapid Deconstruction.";
    default:
    case DR_BROWN:
        ostr << "don't do anything special. (This message shouldn't ever display.)";
        break;
    case DR_CYAN:
        ostr << "grant you immunity to the effects of clouds and air. (rCloud, rAir)";
        break;
    case DR_GOLDEN:
        ostr << "grant you resistance to fire, cold and poison. (rF+, rC+, rPois)";
        break;
    case DR_GREEN:
        ostr << "grant you resistance to poison. (rPois)";
        break;
    case DR_LIME:
        ostr << "grant you resistance to acid and corrosion. (rCorr)";
        break;
    case DR_MAGENTA:
        ostr << "passively repel missiles. (rMsl)";
        break;
    case DR_OLIVE:
        ostr << "grant you resistance to rotting caused by mutagenic radiation. (rMut)";
        break;
    case DR_PEARL:
        ostr << "boost your defenses and vastly increase your resistance to negative energy. (AC++, rN+++)";
        break;
    case DR_PINK:
        ostr << "grant you exceptional clarity of mind. (clarity)";
        break;
    case DR_PLATINUM:
        ostr << "increase your movement speed and make you resistant to mutation. (fast, rMut)";
        break;
    case DR_PURPLE:
        ostr << "protect you from hostile magic. (MR++)";
        break;
    case DR_RED:
        ostr << "grant you resistance to heat and fire. (rF+)";
        break;
    case DR_SCINTILLATING:
        ostr << "chaotically grant elemental resistances and boost your physical defenses a bit. (AC+, Chaos+)";
        break;
    case DR_SILVER:
        ostr << "boost your physical defenses and make you resistant to mutation. (AC++, rMut)";
        break;
    case DR_TEAL:
        ostr << "hold together your spectral form.";
        break;
    case DR_WHITE:
        ostr << "grant you resistance to cold and ice. (rC+)";
        break;
    }
    return ostr.str();
}

static string _drac_enhancer_msg(bool gain)
{
    ostringstream ostr;
    if (gain)
        ostr << _get_mutation_def(MUT_DRACONIAN_ENHANCER).gain[0];
    else
        ostr << _get_mutation_def(MUT_DRACONIAN_ENHANCER).have[you.get_mutation_level(MUT_DRACONIAN_ENHANCER, false) - 1];

    switch (you.drac_colour)
    {
    case DR_BLACK:      ostr << "death.";                                       break;
    case DR_BLOOD:      ostr << "death, curses and the sky.";                   break;
    case DR_BLUE:       ostr << "the sky.";                                     break;
    case DR_BONE:       ostr << "charms and the earth.";                        break;
    case DR_CYAN:       ostr << "translocation.";                               break;
    case DR_GOLDEN:     ostr << "fire, ice and venom.";                         break;
    case DR_GREEN:      ostr << "venom.";                                       break;
    case DR_LIME:       ostr << "transmutation.";                               break;
    case DR_MAGENTA:    ostr << "charms.";                                      break;
    case DR_OLIVE:      ostr << "venom and the sky.";                           break;
    case DR_PEARL:      ostr << "charms, summonings and the earth.";            break;
    case DR_PINK:       ostr << "summonings.";                                  break;
    case DR_PLATINUM:   ostr << "translocation, transmutation and hexes.";      break;
    case DR_PURPLE:     ostr << "hexes.";                                       break;
    case DR_RED:        ostr << "fire.";                                        break;
    case DR_SILVER:     ostr << "the earth.";                                   break;
    case DR_TEAL:       ostr << "translocation and transmutation.";             break;
    case DR_WHITE:      ostr << "ice.";                                         break;
    default:    // Shouldn't display ever; hopefully.
    case DR_BROWN:      ostr << "bugginess.";                                   break;
    case DR_SCINTILLATING:  // Special case; messaging unlike the others.
        if (gain)
            return "Your magic feels more chaotic and you randomly feel bursts of power when casting.";
        else if (you.get_mutation_level(MUT_DRACONIAN_ENHANCER) > 1)
            return "Your magic is chaotic and you often have strongly boosted spellpower when casting.";
        else
            return "Your magic is chaotic and you often have boosted spellpower when casting.";
    }
    return ostr.str();
}

/*
 * Try to mutate the player, along with associated bookkeeping. This accepts mutation categories as well as particular mutations.
 *
 * In many cases this will produce only 1 level of mutation at a time, but it may mutate more than one level if the mutation category is corrupt or qazlal.
 *
 * If the player is at the mutation cap, this may fail.
 *   1. If mutclass is innate, this will attempt to replace temporary and normal mutations (in that order) and will fail if this isn't possible (e.g. there are only innate levels).
 *   2. Otherwise, this will fail. This means that a temporary mutation can block a permanent mutation of the same type in some circumstances.
 *
 * If the mutation conflicts with an existing one it may fail. See `_handle_conflicting_mutations`.
 *
 * If the player is undead, this may rot instead. Rotting counts as success.
 *
 * @param which_mutation    the mutation to use.
 * @param reason            the explanation for how the player got mutated.
 * @param failMsg           whether to do any messaging if this fails.
 * @param force_mutation    whether to override mutation protection and the like.
 * @param god_gift          is this a god gift? Entails overriding mutation resistance if not forced.
 * @param mutclass          is the mutation temporary, regular, or permanent (innate)? permanent entails force_mutation.
 *
 * @return whether the mutation succeeded.
 */
bool mutate(mutation_type which_mutation, const string &reason, bool failMsg,
            bool force_mutation, bool god_gift, bool beneficial,
            mutation_permanence_class mutclass)
{
    if (which_mutation == RANDOM_BAD_MUTATION
        && mutclass == MUTCLASS_NORMAL
        && crawl_state.disables[DIS_AFFLICTIONS])
    {
        return true; // no fallbacks
    }

    god_gift |= crawl_state.is_god_acting();

    if (mutclass == MUTCLASS_INNATE)
        force_mutation = true;

    mutation_type mutat = which_mutation;

    if (!force_mutation)
    {
        // God gifts override all sources of mutation resistance other
        // than divine protection.
        if (!god_gift && _resist_mutation(mutclass, beneficial))
        {
            if (failMsg)
                mprf(MSGCH_MUTATION, "You feel odd for a moment.");
            return false;
        }

        // Zin's protection.
        if (have_passive(passive_t::resist_mutation)
            && x_chance_in_y(apply_pity(you.piety), piety_breakpoint(5)))
        {
            simple_god_message(" protects your body from mutation!");
            return false;
        }
    }

    // Undead bodies don't mutate, they fall apart. -- bwr
    if (undead_mutation_rot() && !((you.undead_state() == US_GHOST) && god_gift))
    {
        switch (mutclass)
        {
        case MUTCLASS_TEMPORARY:
            if (coinflip())
                return false;
            // fallthrough to normal mut
        case MUTCLASS_NORMAL:
            if (you.undead_state() == US_GHOST)
                mprf(MSGCH_MUTATION, "Wild energies bleed away your essence!");
            else 
                mprf(MSGCH_MUTATION, "Your body decomposes!");
            lose_stat(STAT_RANDOM, 1);
            return true;
        case MUTCLASS_INNATE:
            // You can't miss out on innate mutations just because you're
            // temporarily undead.
            break;
        default:
            die("bad fall through");
            return false;
        }
    }

    if (mutclass == MUTCLASS_NORMAL
        && (which_mutation == RANDOM_MUTATION
            || which_mutation == RANDOM_XOM_MUTATION)
        && x_chance_in_y(you.how_mutated(false, true), 15))
    {
        // God gifts override mutation loss due to being heavily
        // mutated.
        if (!one_chance_in(3) && !god_gift && !force_mutation)
            return false;
        else
            return delete_mutation(RANDOM_MUTATION, reason, failMsg,
                                   force_mutation, false);
    }

    switch (which_mutation)
    {
    case RANDOM_MUTATION:
    case RANDOM_GOOD_MUTATION:
    case RANDOM_BAD_MUTATION:
    case RANDOM_CORRUPT_MUTATION:
        mutat = _get_random_mutation(which_mutation);
        break;
    case RANDOM_XOM_MUTATION:
        mutat = _get_random_xom_mutation();
        break;
    case RANDOM_SLIME_MUTATION:
        mutat = _get_random_slime_mutation();
        break;
    case RANDOM_QAZLAL_MUTATION:
        mutat = _get_random_qazlal_mutation();
        break;
    default:
        break;
    }

    if (!_is_valid_mutation(mutat))
        return false;

    // [Cha] don't allow teleportitis in sprint
    if (mutat == MUT_TELEPORT && crawl_state.game_is_sprint())
        return false;

    if (physiology_mutation_conflict(mutat))
        return false;

    const mutation_def& mdef = _get_mutation_def(mutat);

    bool gain_msg = true;

    if (mutclass == MUTCLASS_INNATE)
    {
        // are there any non-innate instances to replace?  Prioritize temporary mutations over normal.
        // Temporarily decrement the mutation value so it can be silently regained in the while loop below.
        if (you.mutation[mutat] > you.innate_mutation[mutat])
        {
            if (you.temp_mutation[mutat] > 0)
            {
                you.temp_mutation[mutat]--;
                you.attribute[ATTR_TEMP_MUTATIONS]--;
                if (you.attribute[ATTR_TEMP_MUTATIONS] == 0)
                    you.attribute[ATTR_TEMP_MUT_XP] = 0;
            }
            you.mutation[mutat]--;
            mprf(MSGCH_MUTATION, "Your mutations feel more permanent.");
            take_note(Note(NOTE_PERM_MUTATION, mutat,
                    you.get_base_mutation_level(mutat), reason.c_str()));
            gain_msg = false;
        }
    }
    if (you.mutation[mutat] >= mdef.levels)
        return false;

    // God gifts and forced mutations clear away conflicting mutations.
    int rc = _handle_conflicting_mutations(mutat, god_gift || force_mutation,
                                           reason,
                                           mutclass == MUTCLASS_TEMPORARY);
    if (rc == 1)
        return true;
    if (rc == -1)
        return false;

    ASSERT(rc == 0);

    const unsigned int old_talents = your_talents(false).size();

    const int levels = (which_mutation == RANDOM_CORRUPT_MUTATION
                         || which_mutation == RANDOM_QAZLAL_MUTATION)
                       ? min(2, mdef.levels - you.get_base_mutation_level(mutat))
                       : 1;
    ASSERT(levels > 0); //TODO: is > too strong?

    int count = levels;

    while (count-- > 0)
    {
        // no fail condition past this point, so it is safe to do bookkeeping
        you.mutation[mutat]++;
        if (mutclass == MUTCLASS_TEMPORARY)
        {
            // do book-keeping for temporary mutations
            you.temp_mutation[mutat]++;
            you.attribute[ATTR_TEMP_MUTATIONS]++;
        }
        else if (mutclass == MUTCLASS_INNATE)
            you.innate_mutation[mutat]++;

        const int cur_base_level = you.get_base_mutation_level(mutat);

        // More than three messages, need to give them by hand.
        switch (mutat)
        {
        case MUT_STRONG: case MUT_AGILE:  case MUT_CLEVER:
        case MUT_WEAK:   case MUT_CLUMSY: case MUT_DOPEY:
            mprf(MSGCH_MUTATION, "You feel %s.", _stat_mut_desc(mutat, true));
            gain_msg = false;
            break;

        case MUT_LARGE_BONE_PLATES:
            {
                const char *arms;
                if (you.species == SP_FELID)
                    arms = "legs";
                else if (you.species == SP_OCTOPODE)
                    arms = "tentacles";
                else
                    break;
                mprf(MSGCH_MUTATION, "%s",
                     replace_all(mdef.gain[cur_base_level - 1], "arms",
                                 arms).c_str());
                gain_msg = false;
            }
            break;

        case MUT_DRACONIAN_DEFENSE:
            mprf(MSGCH_MUTATION, "%s", _drac_def_msg().c_str());
            gain_msg = false;
            break;

        case MUT_DRACONIAN_ENHANCER:
            mprf(MSGCH_MUTATION, "%s", _drac_enhancer_msg(true).c_str());
            gain_msg = false;
            break;

        case MUT_MISSING_HAND:
            {
                const char *hands;
                if (you.species == SP_FELID)
                    hands = "front paws";
                else if (you.species == SP_OCTOPODE)
                    hands = "tentacles";
                else
                    break;
                mprf(MSGCH_MUTATION, "%s",
                     replace_all(mdef.gain[cur_base_level - 1], "hands",
                                 hands).c_str());
                gain_msg = false;
            }
            break;

        case MUT_NO_LOVE:
            {
            if (you.attribute[ATTR_SKELETON])
            {
                you.attribute[ATTR_SKELETON] = 0;
                mprf(MSGCH_DURATION, "Skeletons will no longer rise from your steps.");
            }
            }
            break;

        case MUT_SPIT_POISON:
            // Breathe poison replaces spit poison (so it takes the slot).
            if (cur_base_level >= 2)
                abil_swap(ABIL_SPIT_POISON, ABIL_BREATHE_POISON);
            break;

        default:
            break;
        }

        // For all those scale mutations.
        you.redraw_armour_class = true;

        notify_stat_change();

        if (gain_msg)
        {
            species_mutation_message msg = _spmut_msg(mutat);
            if (msg.mutation == MUT_NON_MUTATION)
                mprf(MSGCH_MUTATION, "%s", mdef.gain[cur_base_level - 1]);
            else
                mprf(MSGCH_MUTATION, "%s", msg.gain[cur_base_level - 1]);
        }

        // Do post-mutation effects.
        switch (mutat)
        {
        case MUT_FRAIL:
        case MUT_ROBUST:
        case MUT_RUGGED_BROWN_SCALES:
            calc_hp();
            break;

        case MUT_LOW_MAGIC:
        case MUT_HIGH_MAGIC:
            calc_mp();
            break;

        case MUT_PASSIVE_MAPPING:
            add_daction(DACT_REAUTOMAP);
            break;

        case MUT_HOOVES:
        case MUT_TALONS:
            // Hooves and talons force boots off at 3.
            if (cur_base_level >= 3 && !you.melded[EQ_BOOTS])
                remove_one_equip(EQ_BOOTS, false, true);
            // Recheck Ashenzari bondage in case our available slots changed.
            ash_check_bondage();
            break;

        case MUT_CLAWS:
            // Claws force gloves off at 3.
            if (cur_base_level >= 3 && !you.melded[EQ_GLOVES])
                remove_one_equip(EQ_GLOVES, false, true);
            // Recheck Ashenzari bondage in case our available slots changed.
            ash_check_bondage();
            break;

        case MUT_HORNS:
        case MUT_ANTENNAE:
            // Horns & Antennae 3 removes all headgear. Same algorithm as with
            // glove removal.

            if (cur_base_level >= 3 && !you.melded[EQ_HELMET])
                remove_one_equip(EQ_HELMET, false, true);
            // Intentional fall-through
        case MUT_BEAK:
            // Horns, beaks, and antennae force hard helmets off.
            if (you.equip[EQ_HELMET] != -1
                && is_hard_helmet(you.inv[you.equip[EQ_HELMET]])
                && !you.melded[EQ_HELMET])
            {
                remove_one_equip(EQ_HELMET, false, true);
            }
            // Recheck Ashenzari bondage in case our available slots changed.
            ash_check_bondage();
            break;

        case MUT_ACUTE_VISION:
            // We might have to turn autopickup back on again.
            autotoggle_autopickup(false);
            break;

        case MUT_NIGHTSTALKER:
            update_vision_range();
            break;

        case MUT_BIG_WINGS:
#ifdef USE_TILE
            init_player_doll();
#endif
            break;

        default:
            break;
        }

        xom_is_stimulated(_calc_mutation_amusement_value(mutat));

        if (mutclass != MUTCLASS_TEMPORARY)
        {
            take_note(Note(NOTE_GET_MUTATION, mutat, cur_base_level,
                           reason.c_str()));
        }
        else
        {
            // only do this once regardless of how many levels got added
            you.attribute[ATTR_TEMP_MUT_XP] = temp_mutation_roll();
        }

        if (you.hp <= 0)
        {
            ouch(0, KILLED_BY_FRAILTY, MID_NOBODY,
                 make_stringf("gaining the %s mutation",
                              mutation_name(mutat)).c_str());
        }
    }

#ifdef USE_TILE_LOCAL
    if (your_talents(false).size() != old_talents)
    {
        tiles.layout_statcol();
        redraw_screen();
    }
#endif
    if (crawl_state.game_is_hints()
        && your_talents(false).size() > old_talents)
    {
        learned_something_new(HINT_NEW_ABILITY_MUT);
    }
#ifdef DEBUG
    if (mutclass != MUTCLASS_INNATE) // taken care of in perma_mutate. Skipping this here avoids validation issues in doing repairs.
        validate_mutations(false);
#endif
    return true;
}

/*
 * Delete a single mutation level of fixed type `mutat`.
 * If `transient` is set, allow deleting temporary mutations, and prioritize them.
 * Note that if `transient` is true and there are no temporary mutations, this can delete non-temp mutations.
 * If `transient` is false, and there are only temp mutations, this will fail; otherwise it will delete a non-temp mutation.
 *
 * @mutat     the mutation to delete
 * @reason    why is it being deleted
 * @transient whether to allow (and prioritize) deletion of temporary mutations
 *
 * @return whether a mutation was deleted.
 */
static bool _delete_single_mutation_level(mutation_type mutat,
                                          const string &reason,
                                          bool transient)
{
    // are there some non-innate mutations to delete?
    if (you.get_base_mutation_level(mutat, false, true, true) == 0)
        return false;

    bool was_transient = false;
    if (you.has_temporary_mutation(mutat))
    {
        if (transient)
            was_transient = true;
        else if (you.get_base_mutation_level(mutat, false, false, true) == 0) // there are only temporary mutations to delete
            return false;

        // fall through: there is a non-temporary mutation level that can be deleted.
    }

    const mutation_def& mdef = _get_mutation_def(mutat);

    bool lose_msg = true;

    you.mutation[mutat]--;

    switch (mutat)
    {
    case MUT_STRONG: case MUT_AGILE:  case MUT_CLEVER:
    case MUT_WEAK:   case MUT_CLUMSY: case MUT_DOPEY:
        mprf(MSGCH_MUTATION, "You feel %s.", _stat_mut_desc(mutat, false));
        lose_msg = false;
        break;

    case MUT_SPIT_POISON:
        // Breathe poison replaces spit poison (so it takes the slot).
        if (you.mutation[mutat] < 2)
            abil_swap(ABIL_BREATHE_POISON, ABIL_SPIT_POISON);
        break;

    case MUT_NIGHTSTALKER:
        update_vision_range();
        break;

    case MUT_BIG_WINGS:
        lose_permafly_source();
        break;

    case MUT_HORNS:
    case MUT_ANTENNAE:
    case MUT_BEAK:
    case MUT_CLAWS:
    case MUT_HOOVES:
    case MUT_TALONS:
        // Recheck Ashenzari bondage in case our available slots changed.
        ash_check_bondage();
        break;

    default:
        break;
    }

    // For all those scale mutations.
    you.redraw_armour_class = true;

    notify_stat_change();

    if (lose_msg)
    {
        species_mutation_message msg = _spmut_msg(mutat);
        if (msg.mutation == MUT_NON_MUTATION)
            mprf(MSGCH_MUTATION, "%s", mdef.lose[you.mutation[mutat]]);
        else
            mprf(MSGCH_MUTATION, "%s", msg.lose[you.mutation[mutat]]);
    }

    // Do post-mutation effects.
    if (mutat == MUT_FRAIL || mutat == MUT_ROBUST
        || mutat == MUT_RUGGED_BROWN_SCALES)
    {
        calc_hp();
    }
    if (mutat == MUT_LOW_MAGIC || mutat == MUT_HIGH_MAGIC)
        calc_mp();

    if (was_transient)
    {
        --you.temp_mutation[mutat];
        --you.attribute[ATTR_TEMP_MUTATIONS];
    }
    else
        take_note(Note(NOTE_LOSE_MUTATION, mutat, you.mutation[mutat], reason));

    if (you.hp <= 0)
    {
        ouch(0, KILLED_BY_FRAILTY, MID_NOBODY,
             make_stringf("losing the %s mutation", mutation_name(mutat)).c_str());
    }

    return true;
}

/*
 * Delete a mutation level, accepting random mutation types and checking mutation resistance.
 * This will not delete temporary or innate mutations.
 *
 * @param which_mutation    a mutation, including random
 * @param reason            the reason for deletion
 * @param failMsg           whether to message the player on failure
 * @param force_mutation    whether to try to override certain cases where the mutation would otherwise fail
 * @param god_gift          is the mutation a god gift?  Will also override certain cases.
 * @param disallow_mismatch for random mutations, do we override good/bad designations in `which_mutation`? (??)
 *
 * @return true iff a mutation was applied.
 */
bool delete_mutation(mutation_type which_mutation, const string &reason,
                     bool failMsg,
                     bool force_mutation, bool god_gift,
                     bool disallow_mismatch)
{
    god_gift |= crawl_state.is_god_acting();

    mutation_type mutat = which_mutation;

    if (!force_mutation)
    {
        if (!god_gift)
        {
            if (you.get_mutation_level(MUT_MUTATION_RESISTANCE) > 1
                && (you.get_mutation_level(MUT_MUTATION_RESISTANCE) == 3
                    || coinflip()))
            {
                if (failMsg)
                    mprf(MSGCH_MUTATION, "You feel rather odd for a moment.");
                return false;
            }
        }

        if (undead_mutation_rot() && !((you.undead_state() == US_GHOST) && god_gift))
            return false;
    }

    if (which_mutation == RANDOM_MUTATION
        || which_mutation == RANDOM_XOM_MUTATION
        || which_mutation == RANDOM_GOOD_MUTATION
        || which_mutation == RANDOM_BAD_MUTATION
        || which_mutation == RANDOM_NON_SLIME_MUTATION
        || which_mutation == RANDOM_CORRUPT_MUTATION
        || which_mutation == RANDOM_QAZLAL_MUTATION)
    {
        while (true)
        {
            if (one_chance_in(1000))
                return false;

            mutat = static_cast<mutation_type>(random2(NUM_MUTATIONS));

            if (you.mutation[mutat] == 0
                && mutat != MUT_STRONG
                && mutat != MUT_CLEVER
                && mutat != MUT_AGILE
                && mutat != MUT_WEAK
                && mutat != MUT_DOPEY
                && mutat != MUT_CLUMSY)
            {
                continue;
            }

            if (which_mutation == RANDOM_NON_SLIME_MUTATION
                && is_slime_mutation(mutat))
            {
                continue;
            }

            // Check whether there is a non-innate level of `mutat` to delete
            if (you.get_base_mutation_level(mutat, false, true, true) == 0)
                continue;

            // MUT_ANTENNAE is 0, and you.attribute[] is initialized to 0.
            if (mutat && mutat == you.attribute[ATTR_APPENDAGE])
                continue;

            const mutation_def& mdef = _get_mutation_def(mutat);

            if (random2(10) >= mdef.weight && !is_slime_mutation(mutat))
                continue;

            const bool mismatch =
                (which_mutation == RANDOM_GOOD_MUTATION
                 && MUT_BAD(mdef))
                    || (which_mutation == RANDOM_BAD_MUTATION
                        && MUT_GOOD(mdef));

            if (mismatch && (disallow_mismatch || !one_chance_in(10)))
                continue;

            if (you.get_base_mutation_level(mutat, true, false, true) == 0)
                continue; // No non-transient mutations in this category to cure

            break;
        }
    }
    else if (which_mutation == RANDOM_SLIME_MUTATION)
    {
        mutat = _delete_random_slime_mutation();

        if (mutat == NUM_MUTATIONS)
            return false;
    }

    return _delete_single_mutation_level(mutat, reason, false); // won't delete temp mutations
}

/*
 * Delete all (non-innate) mutations.
 *
 * If you really need to delete innate mutations as well, have a look at `change_species_to` in species.cc.
 * Changing species to human, for example, is a safe way to clear innate mutations entirely. For a
 * demonspawn, you could also use wizmode code to set the level to 1.
 *
 * @return  Whether the function found mutations to delete.
 */
bool delete_all_mutations(const string &reason)
{
    for (int i = 0; i < NUM_MUTATIONS; ++i)
    {
        while (_delete_single_mutation_level(static_cast<mutation_type>(i), reason, true))
            ;
    }
    ASSERT(you.attribute[ATTR_TEMP_MUTATIONS] == 0);
    ASSERT(you.how_mutated(false, true, false) == 0);
    you.attribute[ATTR_TEMP_MUT_XP] = 0;

    return !you.how_mutated();
}

/*
 * Delete all temporary mutations.
 *
 * @return  Whether the function found mutations to delete.
 */
bool delete_all_temp_mutations(const string &reason)
{
    bool found = false;
    for (int i = 0; i < NUM_MUTATIONS; ++i)
    {
        while (you.has_temporary_mutation(static_cast<mutation_type>(i)))
            if (_delete_single_mutation_level(static_cast<mutation_type>(i), reason, true))
                found = true;
    }
    // the rest of the bookkeeping is handled in _delete_single_mutation_level
    you.attribute[ATTR_TEMP_MUT_XP] = 0;
    return found;
}

/*
 * Delete a single level of a random temporary mutation.
 * This function does not itself do XP-related bookkeeping; see `temp_mutation_wanes()`.
 *
 * @return          Whether the function found a mutation to delete.
 */
bool delete_temp_mutation()
{
    if (you.attribute[ATTR_TEMP_MUTATIONS] > 0)
    {
        mutation_type mutat = NUM_MUTATIONS;

        int count = 0;
        for (int i = 0; i < NUM_MUTATIONS; i++)
            if (you.has_temporary_mutation(static_cast<mutation_type>(i)) && one_chance_in(++count))
                mutat = static_cast<mutation_type>(i);

#if TAG_MAJOR_VERSION == 34
        // We had a brief period (between 0.14-a0-1589-g48c4fed and
        // 0.14-a0-1604-g40af2d8) where we corrupted attributes in transferred
        // games.
        if (mutat == NUM_MUTATIONS)
        {
            mprf(MSGCH_ERROR, "Found no temp mutations, clearing.");
            you.attribute[ATTR_TEMP_MUTATIONS] = 0;
            return false;
        }
#else
        ASSERTM(mutat != NUM_MUTATIONS, "Found no temp mutations, expected %d",
                                        you.attribute[ATTR_TEMP_MUTATIONS]);
#endif

        if (_delete_single_mutation_level(mutat, "temp mutation expiry", true))
            return true;
    }

    return false;
}

// If for_display is false ignores species-specific messaging to give a neat version for use in wizmode etc.
const char* mutation_name(mutation_type mut, bool allow_category, bool for_display)
{
    if (allow_category && mut >= CATEGORY_MUTATIONS && mut < MUT_NON_MUTATION)
        return _get_category_mutation_def(mut).short_desc;

    // note -- this can produce crashes if fed invalid mutations, e.g. if allow_category is false and mut is a category mutation
    if (!_is_valid_mutation(mut))
        return nullptr;

    if (!for_display)
        return _get_mutation_def(mut).short_desc;

    if (mut == MUT_MINOR_MARTIAL_APT_BOOST || mut == MUT_MAJOR_MARTIAL_APT_BOOST || mut == MUT_DEFENSIVE_APT_BOOST)
    {
        // BCADNOTE: Brute force fix.
        skill_type skill = you.defence_skill;

        if (mut == MUT_MINOR_MARTIAL_APT_BOOST)
            skill = you.minor_skill;

        if (mut == MUT_MAJOR_MARTIAL_APT_BOOST)
            skill = you.major_skill;

        switch (skill)
        {
        case SK_ARMOUR:             return "boosted armour aptitude";
        case SK_AXES_HAMMERS:       return "boosted axes & hammers aptitude";
        case SK_BOWS:               return "boosted bows aptitude";
        case SK_CROSSBOWS:          return "boosted crossbows aptitude";
        case SK_DODGING:            return "boosted dodging aptitude";
        case SK_LONG_BLADES:        return "boosted long blades aptitude";
        case SK_MACES_STAVES:       return "boosted maces & staves aptitude";
        case SK_POLEARMS:           return "boosted polearms aptitude";
        case SK_SHIELDS:            return "boosted shields aptitude";
        case SK_SHORT_BLADES:       return "boosted short blades aptitude";
        case SK_SLINGS:             return "boosted slings aptitude";
        case SK_STEALTH:            return "boosted stealth aptitude";
        case SK_UNARMED_COMBAT:     return "boosted unarmed combat aptitude";
        case SK_WHIPS_FLAILS:       return "boosted whips and flails aptitude";
        default:                    return "bad aptitude boost (bug)";
        }
    }

    if (mut == MUT_DRACONIAN_DEFENSE)
    {
        switch (you.drac_colour)
        {
        case DR_BLACK:              return "Stlth+++";
        case DR_BLOOD:              return "rTorm, rHellfire";
        case DR_BLUE:               return "rElec";
        case DR_BONE:               return "AC+++, rShatter-";
        default:
        case DR_BROWN:              return "buggy mutation!";
        case DR_CYAN:               return "rCloud, rAir";
        case DR_GOLDEN:             return "rF+, rC+, rPois";
        case DR_GREEN:              return "rPois";
        case DR_LIME:               return "rCorr";
        case DR_MAGENTA:            return "rMsl";
        case DR_OLIVE:              return "rMut";
        case DR_PEARL:              return "AC++, rN+++";
        case DR_PINK:               return "clarity";
        case DR_PLATINUM:           return "fast, rMut";
        case DR_PURPLE:             return "MR++";
        case DR_RED:                return "rF+";
        case DR_SCINTILLATING:      return "AC+, Chaos+";
        case DR_SILVER:             return "AC++, rMut";
        case DR_TEAL:               return "spectral";
        case DR_WHITE:              return "rC+";
        }
    }

    if (mut == MUT_DRACONIAN_ENHANCER)
    {
        switch (you.drac_colour)
        {
        case DR_BLACK:              return "necromancy enhancer";
        case DR_BLOOD:              return "necromancy, hexes and air enhancers";
        case DR_BLUE:               return "air enhancer";
        case DR_BONE:               return "charms and earth enhancers";
        case DR_CYAN:               return "translocations enhancer";
        case DR_GOLDEN:             return "fire, ice and poison enhancers";
        case DR_GREEN:              return "poison enhancer";
        case DR_LIME:               return "transmutation enhancer";
        case DR_MAGENTA:            return "charms enhancer";
        case DR_OLIVE:              return "poison and air enhancers";
        case DR_PEARL:              return "charms, summoning and earth enhancers";
        case DR_PINK:               return "summoning enhancer";
        case DR_PLATINUM:           return "translocation, transmutation and hexes enhancers";
        case DR_PURPLE:             return "hexes enhancer";
        case DR_RED:                return "fire enhancer";
        case DR_SILVER:             return "earth enhancer";
        case DR_TEAL:               return "translocation and transmutation enhancers";
        case DR_WHITE:              return "ice enhancer";
        case DR_SCINTILLATING:      return "chaos magic, random archmage";
        default:    // Shouldn't display ever; hopefully.
        case DR_BROWN:              return "bugginess.";
        }
    }

    species_mutation_message msg = _spmut_msg(mut);
    if (msg.mutation == MUT_NON_MUTATION)
        return _get_mutation_def(mut).short_desc;
    return msg.short_desc;
}

const char* category_mutation_name(mutation_type mut)
{
    if (mut < CATEGORY_MUTATIONS || mut >= MUT_NON_MUTATION)
        return nullptr;
    return _get_category_mutation_def(mut).short_desc;
}

/*
 * Given some name, return a mutation type. Tries to match the short description as found in `mutation-data.h`.
 * If `partial_matches` is set, it will fill the vector with any partial matches it finds. If there is exactly one,
 * will return this mutation, otherwise, will fail.
 *
 * @param allow_category    whether to include category mutation types (e.g. RANDOM_GOOD)
 * @param partial_matches   an optional pointer to a vector, in case the consumer wants to do something
 *                          with the partial match results (e.g. show them to the user). If this is `nullptr`,
 *                          will accept only exact matches.
 *
 * @return the mutation type if succesful, otherwise NUM_MUTATIONS if it can't find a single match.
 */
mutation_type mutation_from_name(string name, bool allow_category, vector<mutation_type> *partial_matches)
{
    mutation_type mutat = NUM_MUTATIONS;

    string spec = lowercase_string(name);

    if (allow_category)
    {
        for (int i = CATEGORY_MUTATIONS; i < MUT_NON_MUTATION; ++i)
        {
            mutation_type mut = static_cast<mutation_type>(i);
            const char* mut_name_c = category_mutation_name(mut);
            if (!mut_name_c)
                continue;
            const string mut_name = lowercase_string(mut_name_c);

            if (spec == mut_name)
                return mut; // note, won't fully populate partial_matches

            if (partial_matches && strstr(mut_name.c_str(), spec.c_str()))
                partial_matches->push_back(mut);
        }
    }

    for (int i = 0; i < NUM_MUTATIONS; ++i)
    {
        mutation_type mut = static_cast<mutation_type>(i);
        const char *mut_name_c = mutation_name(mut);
        if (!mut_name_c)
            continue;
        const string mut_name = lowercase_string(mut_name_c);

        if (spec == mut_name)
        {
            mutat = mut;
            break;
        }

        if (partial_matches && strstr(mut_name.c_str(), spec.c_str()))
            partial_matches->push_back(mut);
    }

    // If only one matching mutation, use that.
    if (partial_matches && mutat == NUM_MUTATIONS && partial_matches->size() == 1)
        mutat = (*partial_matches)[0];
    return mutat;
}

/**
 * A summary of what the next level of a mutation does.
 *
 * @param mut   The mutation_type in question; e.g. MUT_FRAIL.
 * @return      The mutation's description, helpfully trimmed.
 *              e.g. "you are frail (-10% HP)".
 */
string mut_upgrade_summary(mutation_type mut)
{
    if (!_is_valid_mutation(mut))
        return nullptr;

    string mut_desc =
        lowercase_first(mutation_desc(mut, you.mutation[mut] + 1));
    strip_suffix(mut_desc, ".");
    return mut_desc;
}

int mutation_max_levels(mutation_type mut)
{
    if (!_is_valid_mutation(mut))
        return 0;

    return _get_mutation_def(mut).levels;
}

// Return a string describing the mutation.
// If colour is true, also add the colour annotation.
string mutation_desc(mutation_type mut, int level, bool colour,
        bool is_sacrifice)
{
    // Ignore the player's forms, etc.
    const bool ignore_player = (level != -1);

    const mutation_activity_type active = mutation_activity_level(mut);
    const bool partially_active = (active == mutation_activity_type::PARTIAL);
    const bool fully_inactive = (active == mutation_activity_type::INACTIVE);

    const bool temporary = you.has_temporary_mutation(mut);

    // level == -1 means default action of current level
    if (level == -1)
    {
        if (!fully_inactive)
            level = you.get_mutation_level(mut);
        else // give description of fully active mutation
            level = you.get_base_mutation_level(mut);
    }

    string result;

    const mutation_def& mdef = _get_mutation_def(mut);
    const species_mutation_message msg = _spmut_msg(mut);

    if (mut == MUT_STRONG || mut == MUT_CLEVER
        || mut == MUT_AGILE || mut == MUT_WEAK
        || mut == MUT_DOPEY || mut == MUT_CLUMSY)
    {
        level = min(level, 2);
    }
    else if (mut == MUT_PSEUDOPODS)
    {
        string brand = "Buggy";
        string effect = "";
        switch (you.pseudopod_brand)
        {
        case SPWPN_MOLTEN:
            brand = "Scorching";
            effect = "dousing the target in sticky flames.";
            break;
        case SPWPN_ACID:
            brand = "Corrosive";
            break;
        case SPWPN_ELECTROCUTION:
            brand = "Electrifying";
            effect = "malmutating the target.";
            break;
        case SPWPN_VENOM:
            brand = "Noxious";
            effect = "slowing targets, who aren't immune to poison.";
            break;
        case SPWPN_VAMPIRISM:
            brand = "Vampiric";
            if (!you_foodless(false))
                effect = "feeding you.";
            break;
        default:
            break;
        }
        ostringstream ostr;
        // Assumes no species has a unique message for this.
        ostr << brand.c_str() << mdef.have[0];
        if (effect.length())
            ostr << ", sometimes " << effect.c_str();
        else
            ostr << ".";
        result = ostr.str();
    }
    else if (mut == MUT_ICEMAIL)
    {
        ostringstream ostr;
        if (msg.mutation == MUT_NON_MUTATION)
            ostr << mdef.have[0] << player_icemail_armour_class() << ")";
        else
            ostr << msg.have[0] << player_icemail_armour_class() << ")";
        result = ostr.str();
    }
    else if (mut == MUT_SANGUINE_ARMOUR)
    {
        ostringstream ostr;
        if (msg.mutation == MUT_NON_MUTATION)
            ostr << mdef.have[level - 1] << sanguine_armour_bonus() / 100 << ")";
        else
            ostr << msg.have[level - 1] << sanguine_armour_bonus() / 100 << ")";
        result = ostr.str();
    }
    else if (mut == MUT_DRACONIAN_DEFENSE)
        result = _drac_def_msg();
    else if (mut == MUT_DRACONIAN_ENHANCER)
        result = _drac_enhancer_msg(false);
    else if (mut == MUT_MINOR_MARTIAL_APT_BOOST)
    {
        ostringstream ostr;
        ostr << mdef.have[0] << skill_name(you.minor_skill) << " (Aptitude +3).";
        result = ostr.str();
    }
    else if (mut == MUT_DEFENSIVE_APT_BOOST)
    {
        ostringstream ostr;
        ostr << mdef.have[0] << skill_name(you.defence_skill) << " (Aptitude +3).";
        result = ostr.str();
    }
    else if (mut == MUT_MAJOR_MARTIAL_APT_BOOST)
    {
        ostringstream ostr;
        string x = level > 1 ? " (Aptitude +5)." : " (Aptitude +3).";
        ostr << mdef.have[level - 1] << skill_name(you.major_skill) << x;
        result = ostr.str();
    }
    else if (!ignore_player && you.species == SP_FELID && mut == MUT_CLAWS)
        result = "You have sharp claws.";
    else if (have_passive(passive_t::no_mp_regen) && mut == MUT_ANTIMAGIC_BITE)
        result = "Your bite disrupts the magic of your enemies.";
    else if (result.empty() && level > 0)
    {
        if (msg.mutation == MUT_NON_MUTATION)
            result = mdef.have[level - 1];
        else
            result = msg.have[level - 1];
    }

    if (!ignore_player)
    {
        if (fully_inactive)
        {
            result = "((" + result + "))";
            ++_num_full_suppressed;
        }
        else if (partially_active)
        {
            result = "(" + result + ")";
            ++_num_part_suppressed;
        }
    }

    if (temporary)
    {
        result = "[" + result + "]";
        ++_num_transient;
    }

    if (colour)
    {
        const char* colourname = (MUT_BAD(mdef) ? "red" : "lightgrey");
        const bool permanent   = you.has_innate_mutation(mut);

        if (permanent)
        {
            const bool ds = (you.species == SP_DEMONSPAWN || you.char_class == JOB_DEMONSPAWN);
            bool demonspawn = false;

            if (ds)
            {
                for (unsigned int i = 0; i < you.demonic_traits.size(); i++)
                {
                    if (you.demonic_traits[i].mutation == mut &&
                        you.demonic_traits[i].level_gained <= you.experience_level)
                    {
                        demonspawn = true;
                        i += 1000;
                    }
                }
            }

            const bool extra = you.get_base_mutation_level(mut, false, true, true) > 0;

            if (fully_inactive || (mut == MUT_COLD_BLOODED && player_res_cold(false) > 0))
                colourname = "darkgrey";
            else if (is_sacrifice)
                colourname = "lightred";
            else if (partially_active)
                colourname = demonspawn ? "yellow"    : "blue";
            else if (extra)
                colourname = demonspawn ? "lightcyan" : "cyan";
            else
                colourname = demonspawn ? "cyan"      : "lightblue";
        }
        else if (fully_inactive)
            colourname = "darkgrey";
        else if (partially_active)
            colourname = "brown";
        else if (you.form == transformation::appendage && you.attribute[ATTR_APPENDAGE] == mut)
            colourname = "lightgreen";
        else if (is_slime_mutation(mut))
            colourname = "green";
        else if (temporary)
            colourname = (you.get_base_mutation_level(mut, true, false, true) > 0) ?
                         "lightmagenta" : "magenta";

        // Build the result
        ostringstream ostr;
        ostr << '<' << colourname << '>' << result
             << "</" << colourname << '>';
        result = ostr.str();
    }

    return result;
}

// The "when" numbers indicate the range of times in which the mutation tries
// to place itself; it will be approximately placed between when% and
// (when + 100)% of the way through the mutations. For example, you should
// usually get all your body slot mutations in the first 2/3 of your
// mutations and you should usually only start your tier 3 facet in the second
// half of your mutations. See _order_ds_mutations() for details.
static const facet_def _demon_facets[] =
{
    // Body Slot facets
    { 0, { MUT_CLAWS, MUT_CLAWS, MUT_CLAWS },
      { -33, -33, -33 } },
    { 0, { MUT_HORNS, MUT_HORNS, MUT_HORNS },
      { -33, -33, -33 } },
    { 0, { MUT_ANTENNAE, MUT_ANTENNAE, MUT_ANTENNAE },
      { -33, -33, -33 } },
    { 0, { MUT_HOOVES, MUT_HOOVES, MUT_HOOVES },
      { -33, -33, -33 } },
    { 0, { MUT_TALONS, MUT_TALONS, MUT_TALONS },
      { -33, -33, -33 } },
    // Scale mutations
    { 1, { MUT_DISTORTION_FIELD, MUT_DISTORTION_FIELD, MUT_DISTORTION_FIELD },
      { -33, -33, 0 } },
    { 1, { MUT_ICY_BLUE_SCALES, MUT_ICY_BLUE_SCALES, MUT_ICY_BLUE_SCALES },
      { -33, -33, 0 } },
    { 1, { MUT_IRIDESCENT_SCALES, MUT_IRIDESCENT_SCALES, MUT_IRIDESCENT_SCALES },
      { -33, -33, 0 } },
    { 1, { MUT_LARGE_BONE_PLATES, MUT_LARGE_BONE_PLATES, MUT_LARGE_BONE_PLATES },
      { -33, -33, 0 } },
    { 1, { MUT_MOLTEN_SCALES, MUT_MOLTEN_SCALES, MUT_MOLTEN_SCALES },
      { -33, -33, 0 } },
    { 1, { MUT_RUGGED_BROWN_SCALES, MUT_RUGGED_BROWN_SCALES,
           MUT_RUGGED_BROWN_SCALES },
      { -33, -33, 0 } },
    { 1, { MUT_SLIMY_GREEN_SCALES, MUT_SLIMY_GREEN_SCALES, MUT_SLIMY_GREEN_SCALES },
      { -33, -33, 0 } },
    { 1, { MUT_THIN_METALLIC_SCALES, MUT_THIN_METALLIC_SCALES,
        MUT_THIN_METALLIC_SCALES },
      { -33, -33, 0 } },
    { 1, { MUT_THIN_SKELETAL_STRUCTURE, MUT_THIN_SKELETAL_STRUCTURE,
           MUT_THIN_SKELETAL_STRUCTURE },
      { -33, -33, 0 } },
    { 1, { MUT_YELLOW_SCALES, MUT_YELLOW_SCALES, MUT_YELLOW_SCALES },
      { -33, -33, 0 } },
    { 1, { MUT_STURDY_FRAME, MUT_STURDY_FRAME, MUT_STURDY_FRAME },
      { -33, -33, 0 } },
    { 1, { MUT_SANGUINE_ARMOUR, MUT_SANGUINE_ARMOUR, MUT_SANGUINE_ARMOUR },
      { -33, -33, 0 } },
    // Tier 2 facets
    { 2, { MUT_HEAT_RESISTANCE, MUT_FLAME_CLOUD_IMMUNITY, MUT_IGNITE_BLOOD },
      { -33, 0, 0 } },
    { 2, { MUT_COLD_RESISTANCE, MUT_FREEZING_CLOUD_IMMUNITY, MUT_ICEMAIL },
      { -33, 0, 0 } },
    { 2, { MUT_POWERED_BY_DEATH, MUT_POWERED_BY_DEATH, MUT_POWERED_BY_DEATH },
      { -33, 0, 0 } },
    { 2, { MUT_DEMONIC_GUARDIAN, MUT_DEMONIC_GUARDIAN, MUT_DEMONIC_GUARDIAN },
      { -66, 17, 50 } },
    { 2, { MUT_NIGHTSTALKER, MUT_NIGHTSTALKER, MUT_NIGHTSTALKER },
      { -33, 0, 0 } },
    { 2, { MUT_SPINY, MUT_SPINY, MUT_SPINY },
      { -33, 0, 0 } },
    { 2, { MUT_POWERED_BY_PAIN, MUT_POWERED_BY_PAIN, MUT_POWERED_BY_PAIN },
      { -33, 0, 0 } },
    { 2, { MUT_ROT_IMMUNITY, MUT_FOUL_STENCH, MUT_FOUL_STENCH },
      { -33, 0, 0 } },
    { 2, { MUT_MANA_SHIELD, MUT_MANA_REGENERATION, MUT_MANA_LINK },
      { -33, 0, 0 } },
    // Tier 3 facets
    { 3, { MUT_HEAT_RESISTANCE, MUT_FLAME_CLOUD_IMMUNITY, MUT_HURL_DAMNATION },
      { 50, 50, 50 } },
    { 3, { MUT_COLD_RESISTANCE, MUT_FREEZING_CLOUD_IMMUNITY, MUT_PASSIVE_FREEZE },
      { 50, 50, 50 } },
    { 3, { MUT_ROBUST, MUT_ROBUST, MUT_ROBUST },
      { 50, 50, 50 } },
    { 3, { MUT_NEGATIVE_ENERGY_RESISTANCE, MUT_STOCHASTIC_TORMENT_RESISTANCE,
           MUT_BLACK_MARK },
      { 50, 50, 50 } },
    { 3, { MUT_AUGMENTATION, MUT_AUGMENTATION, MUT_AUGMENTATION },
      { 50, 50, 50 } },
};

static bool _works_at_tier(const facet_def& facet, int tier)
{
    return facet.tier == tier;
}

typedef decltype(facet_def().muts) mut_array_t;
static bool _slot_is_unique(const mut_array_t &mut,
                            set<const facet_def *> facets_used)
{
    set<equipment_type> eq;

    // find the equipment slot(s) used by mut
    for (const body_facet_def &facet : _body_facets)
    {
        for (mutation_type slotmut : mut)
            if (facet.mut == slotmut)
                eq.insert(facet.eq);
    }

    if (eq.empty())
        return true;

    for (const facet_def *used : facets_used)
    {
        for (const body_facet_def &facet : _body_facets)
            if (facet.mut == used->muts[0] && eq.count(facet.eq))
                return false;
    }

    return true;
}

static vector<demon_mutation_info> _select_ds_mutations()
{
    int ct_of_tier[] = { 1, 1, 2, 1 };

try_again:
    vector<demon_mutation_info> ret;

    ret.clear();
    int absfacet = 0;
    int ice_elemental = 0;
    int fire_elemental = 0;
    int cloud_producing = 0;

    set<const facet_def *> facets_used;

    for (int tier = ARRAYSZ(ct_of_tier) - 1; tier >= 0; --tier)
    {
        for (int nfacet = 0; nfacet < ct_of_tier[tier]; ++nfacet)
        {
            const facet_def* next_facet;

            do
            {
                next_facet = &RANDOM_ELEMENT(_demon_facets);
            }
            while (!_works_at_tier(*next_facet, tier)
                   || facets_used.count(next_facet)
                   || !_slot_is_unique(next_facet->muts, facets_used)
                   || physiology_mutation_conflict(next_facet->muts[2], true));

            facets_used.insert(next_facet);

            for (int i = 0; i < 3; ++i)
            {
                mutation_type m = next_facet->muts[i];

                ret.emplace_back(m, next_facet->when[i], absfacet);

                if (m == MUT_COLD_RESISTANCE)
                    ice_elemental++;

                if (m == MUT_HEAT_RESISTANCE)
                    fire_elemental++;

                if (m == MUT_ROT_IMMUNITY || m == MUT_IGNITE_BLOOD)
                    cloud_producing++;
            }

            ++absfacet;
        }
    }

    if (ice_elemental + fire_elemental > 1)
        goto try_again;

    if (cloud_producing > 1)
        goto try_again;

    return ret;
}

static vector<mutation_type>
_order_ds_mutations(vector<demon_mutation_info> muts)
{
    vector<mutation_type> out;
    vector<int> times;
    FixedVector<int, 1000> time_slots;
    time_slots.init(-1);
    for (unsigned int i = 0; i < muts.size(); i++)
    {
        int first = max(0, muts[i].when);
        int last = min(100, muts[i].when + 100);
        int k;
        do
        {
            k = 10 * first + random2(10 * (last - first));
        }
        while (time_slots[k] >= 0);
        time_slots[k] = i;
        times.push_back(k);

        // Don't reorder mutations within a facet.
        for (unsigned int j = i; j > 0; j--)
        {
            if (muts[j].facet == muts[j-1].facet && times[j] < times[j-1])
            {
                int earlier = times[j];
                int later = times[j-1];
                time_slots[earlier] = j-1;
                time_slots[later] = j;
                times[j-1] = earlier;
                times[j] = later;
            }
            else
                break;
        }
    }

    for (int time = 0; time < 1000; time++)
        if (time_slots[time] >= 0)
            out.push_back(muts[time_slots[time]].mut);

    return out;
}

static vector<player::demon_trait>
_schedule_ds_mutations(vector<mutation_type> muts)
{
    list<mutation_type> muts_left(muts.begin(), muts.end());

    list<int> slots_left;

    vector<player::demon_trait> out;

    for (int level = 2; level <= 27; ++level)
        slots_left.push_back(level);

    while (!muts_left.empty())
    {
        if (out.empty() // always give a mutation at XL 2
            || x_chance_in_y(muts_left.size(), slots_left.size()))
        {
            player::demon_trait dt;

            dt.level_gained = slots_left.front();
            dt.mutation     = muts_left.front();

            dprf("Demonspawn will gain %s at level %d",
                    _get_mutation_def(dt.mutation).short_desc, dt.level_gained);

            out.push_back(dt);

            muts_left.pop_front();
        }
        slots_left.pop_front();
    }

    return out;
}

void roll_demonspawn_mutations()
{
    // intentionally create the subgenerator either way, so that this has the
    // same impact on the current main rng for all chars.
    rng::subgenerator ds_rng;

    if (you.char_class != JOB_DEMONSPAWN)
        return;
    you.demonic_traits = _schedule_ds_mutations(
                         _order_ds_mutations(
                         _select_ds_mutations()));
}

static bool _is_ranged_skill(skill_type skill)
{
    if (skill == SK_BOWS || skill == SK_CROSSBOWS || skill == SK_SLINGS)
        return true;
    return false;
}

// Sets up variables only used by the Draconian pseudomutations.
void draconian_setup()
{
    if (!species_is_draconian(you.species))
        return;

    // Only case where you'd get here at higher than XL 7 is importing an old save (rare) but still.
    if (you.experience_level < 7)
        you.drac_colour = DR_BROWN;

    you.major_first = coinflip();

    you.major_skill = random_choose(SK_LONG_BLADES, SK_AXES_HAMMERS, SK_MACES_STAVES, SK_WHIPS_FLAILS, SK_SLINGS,
        SK_BOWS, SK_CROSSBOWS);

    you.minor_skill = you.major_skill;

    while (you.major_skill == you.minor_skill)
    {
        you.minor_skill = random_choose(SK_SHORT_BLADES, SK_LONG_BLADES, SK_AXES_HAMMERS, SK_MACES_STAVES, SK_WHIPS_FLAILS,
            SK_SLINGS, SK_BOWS, SK_CROSSBOWS, SK_UNARMED_COMBAT);
    }

    you.defence_skill = random_choose(SK_DODGING, SK_STEALTH, SK_SHIELDS);

    if (_is_ranged_skill(you.major_skill) && _is_ranged_skill(you.minor_skill))
    {
        if (coinflip())
        {
            you.minor_skill = random_choose(SK_SHORT_BLADES, SK_LONG_BLADES, SK_AXES_HAMMERS, SK_MACES_STAVES, SK_WHIPS_FLAILS,
                SK_UNARMED_COMBAT);
        }
        else
            you.major_skill = random_choose(SK_LONG_BLADES, SK_AXES_HAMMERS, SK_MACES_STAVES, SK_WHIPS_FLAILS);
    }
}

bool perma_mutate(mutation_type which_mut, int how_much, const string &reason)
{
    ASSERT(_is_valid_mutation(which_mut));

    int cap = get_mutation_cap(which_mut);
    how_much = min(how_much, cap);

    int rc = 1;
    // clear out conflicting mutations
    int count = 0;
    while (rc == 1 && ++count < 100)
        rc = _handle_conflicting_mutations(which_mut, true, reason);
    ASSERT(rc == 0);

    int levels = 0;
    while (how_much-- > 0)
    {
        dprf("Perma Mutate %s: cap %d, total %d, innate %d", mutation_name(which_mut), cap,
            you.get_base_mutation_level(which_mut), you.get_innate_mutation_level(which_mut));
        if (you.get_base_mutation_level(which_mut, true, false, false) < cap
            && !mutate(which_mut, reason, false, true, false, false, MUTCLASS_INNATE))
        {
            dprf("Innate mutation failed.");
            break;
        }
        levels++;
    }

#ifdef DEBUG
    // don't validate permamutate directly on level regain; this is so that wizmode level change
    // functions can work correctly.
    if (you.experience_level >= you.max_level)
        validate_mutations(false);
#endif
    return levels > 0;
}

bool temp_mutate(mutation_type which_mut, const string &reason)
{
    return mutate(which_mut, reason, false, false, false, false, MUTCLASS_TEMPORARY);
}

int temp_mutation_roll()
{
    return min(you.experience_level, 17) * (500 + roll_dice(5, 500)) / 17;
}

bool temp_mutation_wanes()
{
    const int starting_tmuts = you.attribute[ATTR_TEMP_MUTATIONS];
    if (starting_tmuts == 0)
        return false;

    int num_remove = min(starting_tmuts,
        max(starting_tmuts * 5 / 12 - random2(3),
        1 + random2(3)));

    mprf(MSGCH_DURATION, "You feel the corruption within you wane %s.",
        (num_remove >= starting_tmuts ? "completely" : "somewhat"));

    for (int i = 0; i < num_remove; ++i)
        delete_temp_mutation(); // chooses randomly

    if (you.attribute[ATTR_TEMP_MUTATIONS] > 0)
        you.attribute[ATTR_TEMP_MUT_XP] += temp_mutation_roll();
    else
        you.attribute[ATTR_TEMP_MUT_XP] = 0;
    ASSERT(you.attribute[ATTR_TEMP_MUTATIONS] < starting_tmuts);
    return true;
}

/**
 * How mutated is the player?
 *
 * @param innate Whether to count innate mutations (default false).
 * @param levels Whether to add up mutation levels, as opposed to just counting number of mutations (default false).
 * @param temp   Whether to count temporary mutations (default true).
 * @param ds     Whether to count innate demonspawn mutations, ignored if innate is true (default false).
 * @return Either the number of matching mutations, or the sum of their
 *         levels, depending on \c levels
 */
int player::how_mutated(bool innate, bool levels, bool temp, bool ds) const
{
    int result = 0;

    for (int i = 0; i < NUM_MUTATIONS; ++i)
    {
        if (you.mutation[i])
        {
            const int mut_level = get_base_mutation_level(static_cast<mutation_type>(i), innate || ds, temp);
            bool include = false;

            if (ds)
            {
                for (unsigned int j = 0; j < you.demonic_traits.size(); j++)
                {
                    if (you.demonic_traits[j].mutation == i &&
                        you.demonic_traits[j].level_gained <= you.experience_level)
                    {
                        include = true;
                        j += 1000;
                    }
                }
            }

            if (!ds || include)
            {
                if (levels)
                    result += mut_level;
                else if (mut_level > 0)
                    result++;
            }
        }
        if (innate && you.props.exists("num_sacrifice_muts"))
            result -= you.props["num_sacrifice_muts"].get_int();
    }

    return result;
}

// Return whether current tension is balanced
static bool _balance_demonic_guardian()
{
    // if tension is unfavorably high, perhaps another guardian should spawn
    const int mutlevel = you.get_mutation_level(MUT_DEMONIC_GUARDIAN);
    const int tension = get_tension(GOD_NO_GOD);
    return tension*3/4 <= mutlevel*6 + random2(mutlevel*mutlevel*2);
}

// Primary function to handle and balance demonic guardians, if the tension
// is unfavorably high and a guardian was not recently spawned, a new guardian
// will be made, if tension is below a threshold (determined by the mutations
// level and a bit of randomness), guardians may be dismissed in
// _balance_demonic_guardian()
void check_demonic_guardian()
{
    // Players hated by all monsters don't get guardians, so that they aren't
    // swarmed by hostile executioners whenever things get rough.
    if (you.get_mutation_level(MUT_NO_LOVE))
        return;

    const int mutlevel = you.get_mutation_level(MUT_DEMONIC_GUARDIAN);

    if (!_balance_demonic_guardian() &&
        you.duration[DUR_DEMONIC_GUARDIAN] == 0)
    {
        monster_type mt;

        switch (mutlevel)
        {
        case 1:
            mt = random_choose(MONS_QUASIT, MONS_WHITE_IMP, MONS_UFETUBUS,
                               MONS_IRON_IMP, MONS_SHADOW_IMP);
            break;
        case 2:
            mt = random_choose(MONS_ORANGE_DEMON, MONS_ICE_DEVIL,
                               MONS_SOUL_EATER, MONS_SMOKE_DEMON,
                               MONS_SIXFIRHY);
            break;
        case 3:
            mt = random_choose(MONS_EXECUTIONER, MONS_BALRUG, MONS_REAPER,
                               MONS_CACODEMON, MONS_LOROCYPROCA);
            break;
        default:
            die("Invalid demonic guardian level: %d", mutlevel);
        }

        monster *guardian = create_monster(
            mgen_data(mt, BEH_FRIENDLY, you.pos(), MHITYOU,
                      MG_FORCE_BEH | MG_AUTOFOE).set_summoned(&you, 2, 0));

        if (!guardian)
            return;

        guardian->flags |= MF_NO_REWARD;
        guardian->flags |= MF_DEMONIC_GUARDIAN;

        guardian->add_ench(ENCH_LIFE_TIMER);

        // no more guardians for mutlevel+1 to mutlevel+20 turns
        you.duration[DUR_DEMONIC_GUARDIAN] = 10*(mutlevel + random2(20));
    }
}

/**
 * Update the map knowledge based on any monster detection sources the player
 * has.
 */
void check_monster_detect()
{
    int radius = player_monster_detect_radius();
    if (radius <= 0)
        return;

    for (radius_iterator ri(you.pos(), radius, C_SQUARE); ri; ++ri)
    {
        monster* mon = monster_at(*ri);
        map_cell& cell = env.map_knowledge(*ri);
        if (!mon)
        {
            if (cell.detected_monster())
                cell.clear_monster();
            continue;
        }
        if (mons_is_firewood(*mon))
            continue;

        // [ds] If the PC remembers the correct monster at this
        // square, don't trample it with MONS_SENSED. Forgetting
        // legitimate monster memory affects travel, which can
        // path around mimics correctly only if it can actually
        // *see* them in monster memory -- overwriting the mimic
        // with MONS_SENSED causes travel to bounce back and
        // forth, since every time it leaves LOS of the mimic, the
        // mimic is forgotten (replaced by MONS_SENSED).
        // XXX: since mimics were changed, is this safe to remove now?
        const monster_type remembered_monster = cell.monster();
        if (remembered_monster == mon->type)
            continue;

        const monster_type mc = mon->friendly() ? MONS_SENSED_FRIENDLY
            : have_passive(passive_t::detect_montier)
            ? ash_monster_tier(mon)
            : MONS_SENSED;

        env.map_knowledge(*ri).set_detected_monster(mc);

        // Don't bother warning the player (or interrupting autoexplore) about
        // friendly monsters or those known to be easy, or those recently
        // warned about
        if (mc == MONS_SENSED_TRIVIAL || mc == MONS_SENSED_EASY
            || mc == MONS_SENSED_FRIENDLY || mon->wont_attack()
            || testbits(mon->flags, MF_SENSED))
        {
            continue;
        }

        for (radius_iterator ri2(mon->pos(), 2, C_SQUARE); ri2; ++ri2)
        {
            if (you.see_cell(*ri2))
            {
                mon->flags |= MF_SENSED;
                interrupt_activity(activity_interrupt::sense_monster);
                break;
            }
        }
    }
}

int augmentation_amount()
{
    int amount = 0;
    const int level = you.get_mutation_level(MUT_AUGMENTATION);

    for (int i = 0; i < level; ++i)
    {
        if (you.hp >= ((i + level) * you.hp_max) / (2 * level))
            amount++;
    }

    return amount;
}

void reset_powered_by_death_duration()
{
    const int pbd_dur = random_range(2, 5);
    you.set_duration(DUR_POWERED_BY_DEATH, pbd_dur);
}
