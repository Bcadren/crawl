#include "AppHdr.h"
#include <map>

#include "species.h"

#include "item-prop.h"
#include "mutation.h"
#include "output.h"
#include "playable.h"
#include "player.h"
#include "player-stats.h"
#include "transform.h"
#include "random.h"
#include "skills.h"
#include "stringutil.h"
#include "tiledoll.h"

#include "species-data.h"

/*
 * Get the species_def for the given species type. Asserts if the species_type
 * is not less than NUM_SPECIES.
 *
 * @param species The species type.
 * @returns The species_def of that species.
 */
const species_def& get_species_def(species_type species)
{
    if (species != SP_UNKNOWN)
        ASSERT_RANGE(species, 0, NUM_SPECIES);
    return species_data.at(species);
}

const char *get_species_abbrev(species_type which_species)
{
    return get_species_def(which_species).abbrev;
}

// Needed for debug.cc and hiscores.cc.
species_type get_species_by_abbrev(const char *abbrev)
{
    for (auto& entry : species_data)
        if (lowercase_string(abbrev) == lowercase_string(entry.second.abbrev))
            return entry.first;

    return SP_UNKNOWN;
}

// Does a case-sensitive lookup of the species name supplied.
species_type str_to_species(const string &species)
{
    species_type sp;
    if (species.empty())
        return SP_UNKNOWN;

    for (int i = 0; i < NUM_SPECIES; ++i)
    {
        sp = static_cast<species_type>(i);
        if (species == species_name(sp, SPNAME_PLAIN, false))
            return sp;
    }

    return SP_UNKNOWN;
}

/**
* Returns the current player species name as displayed in a few places
* Flavourful special cases to the normal general species name handled here.
*/
string display_sp_name()
{
    if (you.get_mutation_level(MUT_SLIME) >= 3)
    {
        if (you.undead_state())
        {
            if (you.is_insubstantial())
                return "Ectoplasm";
            return "Death Ooze";
        }
        return "Slime";
    }

    string name = species_name(you.species);

    switch (you.char_class)
    {
    case JOB_MUMMY:
        if (you.species == SP_HUMAN)
            name = "Mummy";
        else if (you.species != SP_DRACONIAN || you.experience_level < 7)
            name = make_stringf("Mummified %s", name.c_str());
        break;
    case JOB_DEMIGOD:
        if (you.species == SP_HUMAN)
            name = "Demigod";
        else if (you.species == SP_NAGA)
            name = "Nagaraja";
        break;
    case JOB_DEMONSPAWN:
        if (you.species == SP_HUMAN)
            name = "Demonspawn";
    default: break;
    }

    return name;
}

/**
 * Return the name of the given species.
 * @param speci       the species to be named.
 * @param spname_type the kind of name to get: adjectival, the genus, or plain.
 * @returns the requested name, which will just be plain if no adjective
 *          or genus is defined.
 */
string species_name(species_type speci, species_name_type spname_type, bool player)
{
    const species_def& def = get_species_def(speci);
    if (spname_type == SPNAME_GENUS && def.genus_name)
        return def.genus_name;
    else if (spname_type == SPNAME_ADJ && def.adj_name)
        return def.adj_name;
    if (speci == SP_DRACONIAN)
    {
        if (!player)
            return "Draconian";

        switch (you.drac_colour)
        {
        default:
        case DR_BROWN:
            if (you.char_class == JOB_MUMMY)
                return "Draconian";
            return "Immature Draconian";

        case DR_BLACK:          return "Black Draconian";
        case DR_BLOOD:          return "Blood Draconian";
        case DR_BLUE:           return "Blue Draconian";
        case DR_BONE:           return "Bone Draconian";
        case DR_CYAN:           return "Cyan Draconian";
        case DR_GOLDEN:         return "Golden Draconian";
        case DR_GREEN:          return "Green Draconian";
        case DR_LIME:           return "Lime Draconian";
        case DR_MAGENTA:        return "Magenta Draconian";
        case DR_OLIVE:          return "Olive Draconian";
        case DR_PEARL:          return "Pearl Draconian";
        case DR_PINK:           return "Pink Draconian";
        case DR_PLATINUM:       return "Platinum Draconian";
        case DR_PURPLE:         return "Purple Draconian";
        case DR_RED:            return "Red Draconian";
        case DR_SCINTILLATING:  return "Scintillating Draconian";
        case DR_SILVER:         return "Silver Draconian";
        case DR_TEAL:           return "Spectral Draconian";
        case DR_WHITE:          return "White Draconian";
        }
    }
    return def.name;
}

/** What walking-like thing does this species do?
 *
 *  @param sp what kind of species to look at
 *  @returns a "word" to which "-er" or "-ing" can be appended.
 */
string species_walking_verb(species_type sp)
{
    auto verb = get_species_def(sp).walking_verb;
    return verb ? verb : "Walk";
}

/**
 * Where does a given species fall on the Undead Spectrum?
 *
 * @param species   The species in question.
 * @return          What class of undead the given species falls on, if any.
 */
undead_state_type species_undead_type(species_type species)
{
    return get_species_def(species).undeadness;
}

/**
 * Is a given species undead?
 *
 * @param species   The species in question.
 * @return          Whether that species is undead.
 */
bool species_is_undead(species_type species)
{
    return species_undead_type(species) != US_ALIVE;
}

bool species_can_swim(species_type species)
{
    return get_species_def(species).habitat == HT_WATER;
}

bool species_likes_water(species_type species)
{
    return species_can_swim(species)
           || get_species_def(species).habitat == HT_AMPHIBIOUS;
}

bool species_is_elven(species_type species)
{
    return bool(get_species_def(species).flags & SPF_ELVEN);
}

bool species_is_draconian(species_type species)
{
    return bool(get_species_def(species).flags & SPF_DRACONIAN);
}

bool species_is_orcish(species_type species)
{
    return bool(get_species_def(species).flags & SPF_ORCISH);
}

bool species_has_hair(species_type species)
{
    return !bool(get_species_def(species).flags & (SPF_NO_HAIR | SPF_DRACONIAN));
}

size_type species_size(species_type species, size_part_type psize)
{

    if (species == SP_LIGNIFITE)
    {
        if (you.experience_level < 5)
            return SIZE_LITTLE;
        else if (you.experience_level < 10)
            return SIZE_SMALL;
        else if (you.experience_level < 15)
            return SIZE_MEDIUM;
        else if (you.experience_level < 20)
            return SIZE_LARGE;
        else if (you.experience_level < 25)
            return SIZE_BIG;
        else
            return SIZE_GIANT;
    }

    const size_type size = get_species_def(species).size;

    if (psize == PSIZE_TORSO
        && bool(get_species_def(species).flags & SPF_SMALL_TORSO))
    {
        return static_cast<size_type>(static_cast<int>(size) - 1);
    }
    return size;
}

bool species_recommends_job(species_type species, job_type job)
{
    return find(get_species_def(species).recommended_jobs.begin(),
                get_species_def(species).recommended_jobs.end(),
                job) != get_species_def(species).recommended_jobs.end();
}

bool species_recommends_weapon(species_type species, weapon_type wpn)
{
    const skill_type sk =
          wpn == WPN_UNARMED ? SK_UNARMED_COMBAT :
                               item_attack_skill(OBJ_WEAPONS, wpn);

    return find(get_species_def(species).recommended_weapons.begin(),
                get_species_def(species).recommended_weapons.end(),
                sk) != get_species_def(species).recommended_weapons.end();
}

monster_type player_species_to_mons_species(species_type species)
{
    if (you.species == SP_DRACONIAN)
    {
        switch (you.drac_colour)
        {
        case DR_BLACK:              return MONS_BLACK_DRACONIAN;
        case DR_BLOOD:              return MONS_BLOOD_DRACONIAN;
        case DR_BLUE:               return MONS_BLUE_DRACONIAN;
        case DR_BONE:               return MONS_BONE_DRACONIAN;
        case DR_BROWN:   default:   return MONS_DRACONIAN;
        case DR_CYAN:               return MONS_CYAN_DRACONIAN;
        case DR_GOLDEN:             return MONS_GOLDEN_DRACONIAN;
        case DR_GREEN:              return MONS_GREEN_DRACONIAN;
        case DR_LIME:               return MONS_LIME_DRACONIAN;
        case DR_MAGENTA:            return MONS_MAGENTA_DRACONIAN;
        case DR_OLIVE:              return MONS_OLIVE_DRACONIAN;
        case DR_PEARL:              return MONS_PEARL_DRACONIAN;
        case DR_PINK:               return MONS_PINK_DRACONIAN;
        case DR_PLATINUM:           return MONS_PLATINUM_DRACONIAN;
        case DR_PURPLE:             return MONS_PURPLE_DRACONIAN;
        case DR_RED:                return MONS_RED_DRACONIAN;
        case DR_SILVER:             return MONS_SILVER_DRACONIAN;
        case DR_SCINTILLATING:      return MONS_SCINTILLATING_DRACONIAN;
        case DR_TEAL:               return MONS_TEAL_DRACONIAN;
        case DR_WHITE:              return MONS_WHITE_DRACONIAN;
        }
    }

    return get_species_def(species).monster_species;
}

const vector<string>& fake_mutations(species_type species, bool terse)
{
    return terse ? get_species_def(species).terse_fake_mutations
                 : get_species_def(species).verbose_fake_mutations;
}

/**
 * What message should be printed when a character of the specified species
 * prays at an altar, if not in some form?
 * To be inserted into "You %s the altar of foo."
 *
 * @param species   The species in question.
 * @return          An action to be printed when the player prays at an altar.
 *                  E.g., "coil in front of", "kneel at", etc.
 */
string species_prayer_action(species_type species)
{
  auto action = get_species_def(species).altar_action;
  return action ? action : "kneel at";
}

const char* scale_type()
{
    if (you.species != SP_DRACONIAN)
        return "";

    switch (you.drac_colour)
    {
        case DR_RED:
            return "fiery red";
        case DR_WHITE:
            return "icy white";
        case DR_GREEN:
            return "lurid green";
        case DR_CYAN:
            return "empereal azure";
        case DR_LIME:
            return "slimy lime-green";
        case DR_SILVER:
            return "gleaming silver";
        case DR_BLUE:
            return "flickering blue";
        case DR_PURPLE:
            return "rich purple";
        case DR_PINK:
            return "fierce pink";
        case DR_MAGENTA:
            return "hazy amethyst";
        case DR_BLACK:
            return "shadowy dark";
        case DR_OLIVE:
            return "sickly drab";
        case DR_BROWN:
            return "plain brown";
        case DR_TEAL:
            return "spectral turquoise";
        case DR_GOLDEN:
            return "imposing gold";
        case DR_PEARL:
            return "opalescent pearl";
        case DR_SCINTILLATING:
            return "scintillating rainbow";
        case DR_BLOOD:
            return "gory crimson";
        case DR_PLATINUM:
            return "lustrous platinum";
        case DR_BONE: // Don't have scales, needs special casing.
        default:
            return "";
    }
}

monster_type dragon_form_dragon_type()
{
    if (you.species != SP_DRACONIAN)
        return MONS_FIRE_DRAGON;

    // BCADDO: Consider the unimplemented dragon equivalents mentioned here.
    switch (you.drac_colour)
    {
    case DR_CYAN:
        return MONS_WIND_DRAKE;
    case DR_WHITE:
        return MONS_ICE_DRAGON;
    case DR_GREEN:
        return MONS_SWAMP_DRAGON;
    case DR_LIME:
        return MONS_ACID_DRAGON;
    case DR_SILVER:
        return MONS_IRON_DRAGON;
    case DR_BLACK:
        return MONS_SHADOW_DRAGON;
    case DR_PLATINUM:
    case DR_PURPLE:
        return MONS_QUICKSILVER_DRAGON;
    case DR_MAGENTA:
        return MONS_STEAM_DRAGON;
    case DR_BONE:
        return MONS_BONE_DRAGON;
    case DR_BLUE:
        return MONS_STORM_DRAGON;
    case DR_PEARL:
        return MONS_PEARL_DRAGON;
    case DR_OLIVE:
        return MONS_DEATH_DRAKE;
    case DR_SCINTILLATING:  // Apocalypse Dragon?
    case DR_PINK:           // Fairy-tale dragon
    case DR_BLOOD:          // Demon Dragon?
    case DR_TEAL:           // Spectral Dragon?
    case DR_GOLDEN:
        return MONS_GOLDEN_DRAGON;
    case DR_BROWN:
    case DR_RED:
    default:
        return MONS_FIRE_DRAGON;
    }
}

ability_type draconian_breath(bool check_tran)
{
    if (you.species != SP_DRACONIAN)
        return ABIL_NON_ABILITY;

    if (check_tran && you.form == transformation::statue)
        return ABIL_BREATHE_METAL;

    switch (you.drac_colour)
    {
    // Basic
    case DR_BROWN:              return ABIL_BREATHE_DART;

    // Common
    case DR_GREEN:              
        if (check_tran && you.form == transformation::dragon)
            return ABIL_BREATHE_POISON; 
        return ABIL_BREATHE_MEPHITIC;

    case DR_RED:                return ABIL_BREATHE_FIRE;
    case DR_WHITE:              return ABIL_BREATHE_FROST;
    case DR_LIME:               return ABIL_BREATHE_ACID;
    case DR_BLUE:               return ABIL_BREATHE_LIGHTNING;
    case DR_PURPLE:             return ABIL_BREATHE_POWER;
    case DR_MAGENTA:            return ABIL_BREATHE_FOG;
    case DR_CYAN:               return ABIL_BREATHE_WIND;
    case DR_SILVER:             return ABIL_BREATHE_SILVER;
    case DR_PINK:               return ABIL_BREATHE_BUTTERFLIES;
    case DR_SCINTILLATING:      return ABIL_BREATHE_CHAOS;

    // Undead
    case DR_BLACK:              return ABIL_BREATHE_DRAIN;
    case DR_OLIVE:              return ABIL_BREATHE_MIASMA;
    case DR_BONE:               return ABIL_BREATHE_BONE;
    case DR_TEAL:               return ABIL_BREATHE_GHOSTLY_FLAMES;

    // Demigod/Rare.
    case DR_GOLDEN:             return ABIL_BREATHE_FIRE; // Special Cased Later.
    case DR_PEARL:              return ABIL_BREATHE_HOLY_FLAMES;
    case DR_BLOOD:              return ABIL_BREATHE_BLOOD;
    case DR_PLATINUM:           return ABIL_BREATHE_RADIATION;

    default: return ABIL_NON_ABILITY;
    }
}

bool species_is_unbreathing(species_type species)
{
    return any_of(get_species_def(species).level_up_mutations.begin(),
                  get_species_def(species).level_up_mutations.end(),
                  [](level_up_mutation lum)
                    { return lum.mut == MUT_UNBREATHING;});
}

bool species_has_claws(species_type species)
{
    return any_of(get_species_def(species).level_up_mutations.begin(),
                  get_species_def(species).level_up_mutations.end(),
                  [](level_up_mutation lum) { return lum.mut == MUT_CLAWS
                                                     && lum.xp_level == 1; });
}

void mummify()
{
    // Ineligant, but the more 'refined' way of doing it is no better and is more work.

    // Form version lets Draconians keep on transmutation; no effect on other --Mu since can't transmute.
    you.mutation[MUT_UNBREATHING_FORM] = you.innate_mutation[MUT_UNBREATHING_FORM] = 1;

    you.mutation[MUT_NEGATIVE_ENERGY_RESISTANCE] = you.innate_mutation[MUT_NEGATIVE_ENERGY_RESISTANCE] = 3;
    you.mutation[MUT_COLD_RESISTANCE] = you.innate_mutation[MUT_COLD_RESISTANCE] = (you.get_mutation_level(MUT_COLD_RESISTANCE) + 1);
    you.mutation[MUT_TORMENT_RESISTANCE] = you.innate_mutation[MUT_TORMENT_RESISTANCE] = 1;
    you.mutation[MUT_COLD_BLOODED] = you.innate_mutation[MUT_COLD_BLOODED] = 0; // Taking this back away from things that have it because it makes no sense on undead.
}

void give_basic_mutations(species_type species)
{
    // Don't perma_mutate since that gives messages.
    for (const auto& lum : get_species_def(species).level_up_mutations)
        if (lum.xp_level == 1)
            you.mutation[lum.mut] = you.innate_mutation[lum.mut] = lum.mut_level;

    if (you.char_class == JOB_DEMIGOD && you.get_mutation_level(MUT_HIGH_MAGIC) < 3)
        you.mutation[MUT_HIGH_MAGIC] = you.innate_mutation[MUT_HIGH_MAGIC] = (you.get_mutation_level(MUT_HIGH_MAGIC) + 1);
    
    if (you.char_class == JOB_MUMMY)
    {
        mummify();
        you.mutation[MUT_NECRO_ENHANCER] = you.innate_mutation[MUT_NECRO_ENHANCER] = 1;
        you.mutation[MUT_HEAT_VULNERABILITY] = you.innate_mutation[MUT_HEAT_VULNERABILITY] = 1;
    }
}

void give_level_mutations(species_type species, int xp_level)
{
    for (const auto& lum : get_species_def(species).level_up_mutations)
        if (lum.xp_level == xp_level)
        {
            perma_mutate(lum.mut, lum.mut_level,
                         species_name(species) + " growth");
        }
    if (you.char_class == JOB_MUMMY)
    {
        if (xp_level == 7 && you.species == SP_DRACONIAN)
        {
            you.mutation[MUT_HEAT_VULNERABILITY] = you.innate_mutation[MUT_HEAT_VULNERABILITY] = 0;
            string msg;
            switch (you.drac_colour)
            {
            case DR_BONE:   msg = "As your flesh falls away, so does its vulnerability to fire.";                                               break;
            case DR_TEAL:   msg = "As you leave your mummified body behind, you also leave behind its vulnerability to flames.";                break;
            default:        //Just in case
            case DR_OLIVE:
            case DR_BLACK:  msg = "As you take on an adult scale colour, your bandages fall away and you become less vulnerable to flames.";    break;
            }
            mprf(MSGCH_INTRINSIC_GAIN, "%s", msg.c_str());
        }

        if (xp_level == 13)
            perma_mutate(MUT_NECRO_ENHANCER, 2, "mummy growth");
    }

    // Ineligant, make something more refined if losing mutations with level becomes more common.
    // Also doing this way instead of perma_mutate() to use custom messaging.
    if (you.species == SP_LIGNIFITE)
    {
        if (xp_level == 5)
            you.mutation[MUT_FAST] = you.innate_mutation[MUT_FAST] = 2;
        if (xp_level == 10)
            you.mutation[MUT_FAST] = you.innate_mutation[MUT_FAST] = 1;
        if (xp_level == 15)
            you.mutation[MUT_FAST] = you.innate_mutation[MUT_FAST] = 0;
        if (xp_level == 20)
            you.mutation[MUT_SLOW] = you.innate_mutation[MUT_SLOW] = 1;
        if (xp_level == 25)
            you.mutation[MUT_SLOW] = you.innate_mutation[MUT_SLOW] = 2;
    }

    // Right now this only happens to lignifites; but leave it open to possibly happen to
    // any race later (Demonspawn get bigger/smaller as a facet? maybe).
    if (you.weapon(0) && !is_weapon_wieldable(*you.weapon(0), you.body_size(PSIZE_TORSO, true)))
        remove_one_equip(EQ_WEAPON0, false, true);
    if (you.weapon(1) && !is_weapon_wieldable(*you.weapon(1), you.body_size(PSIZE_TORSO, true)))
        remove_one_equip(EQ_WEAPON1, false, true);
}

int species_exp_modifier(species_type species)
{
    return get_species_def(species).xp_mod;
}

int species_hp_modifier(species_type species)
{
    if (you.char_class == JOB_DEMIGOD)
        return get_species_def(species).hp_mod + 1;
    if (you.species == SP_LIGNIFITE)
        return (-2 + div_round_up(you.experience_level, 5));
    return get_species_def(species).hp_mod;
}

int species_mp_modifier(species_type species)
{
    return get_species_def(species).mp_mod;
}

int species_mr_modifier(species_type species)
{
    return get_species_def(species).mr_mod;
}

/**
 *  Does this species have (relatively) low strength?
 *  Used to generate the title for UC ghosts.
 *
 *  @param species the speciecs to check.
 *  @returns whether the starting str is lower than the starting dex.
 */
bool species_has_low_str(species_type species)
{
    return get_species_def(species).d >= get_species_def(species).s;
}

void species_stat_init(species_type species)
{
    you.base_stats[STAT_STR] = get_species_def(species).s;
    you.base_stats[STAT_INT] = get_species_def(species).i;
    you.base_stats[STAT_DEX] = get_species_def(species).d;
}

void species_stat_gain(species_type species)
{
    const species_def& sd = get_species_def(species);
    if (you.char_class == JOB_MUMMY)
    {
        if (you.experience_level % (sd.how_often + 1) == 0)
            modify_stat(*random_iterator(sd.level_stats), 1, false);
    }
    else if (sd.level_stats.size() > 0 && you.experience_level % sd.how_often == 0)
        modify_stat(*random_iterator(sd.level_stats), 1, false);
}

static void _swap_equip(equipment_type a, equipment_type b)
{
    swap(you.equip[a], you.equip[b]);
    bool tmp = you.melded[a];
    you.melded.set(a, you.melded[b]);
    you.melded.set(b, tmp);
}

species_type find_species_from_string(const string &species, bool initial_only)
{
    string spec = lowercase_string(species);

    species_type sp = SP_UNKNOWN;

    for (int i = 0; i < NUM_SPECIES; ++i)
    {
        const species_type si = static_cast<species_type>(i);
        const string sp_name = lowercase_string(species_name(si, SPNAME_PLAIN, false));

        string::size_type pos = sp_name.find(spec);
        if (pos != string::npos)
        {
            if (pos == 0)
            {
                // We prefer prefixes over partial matches.
                sp = si;
                break;
            }
            else if (!initial_only)
                sp = si;
        }
    }

    return sp;
}

/**
 * Change the player's species to something else.
 *
 * This is used primarily in wizmode, but is also used for extreme
 * cases of save compatibility (see `files.cc:_convert_obsolete_species`).
 * This does *not* check for obsoleteness -- as long as it's in
 * species_data it'll do something.
 *
 * @param sp the new species.
 */
void change_species_to(species_type sp)
{
    ASSERT(sp != SP_UNKNOWN);

    // Re-scale skill-points.
    for (skill_type sk = SK_FIRST_SKILL; sk < NUM_SKILLS; ++sk)
    {
        you.skill_points[sk] *= species_apt_factor(sk, sp)
                                / species_apt_factor(sk);
    }

    species_type old_sp = you.species;
    you.species = sp;
    you.chr_species_name = species_name(sp);

    // Change permanent mutations, but preserve non-permanent ones.
    uint8_t prev_muts[NUM_MUTATIONS];

    // remove all innate mutations
    for (int i = 0; i < NUM_MUTATIONS; ++i)
    {
        if (you.has_innate_mutation(static_cast<mutation_type>(i)))
        {
            you.mutation[i] -= you.innate_mutation[i];
            you.innate_mutation[i] = 0;
        }
        prev_muts[i] = you.mutation[i];
    }
    // add the appropriate innate mutations for the new species and xl
    give_basic_mutations(sp);
    for (int i = 2; i <= you.experience_level; ++i)
        give_level_mutations(sp, i);

    for (int i = 0; i < NUM_MUTATIONS; ++i)
    {
        // TODO: why do previous non-innate mutations override innate ones?  Shouldn't this be the other way around?
        if (prev_muts[i] > you.innate_mutation[i])
            you.innate_mutation[i] = 0;
        else
            you.innate_mutation[i] -= prev_muts[i];
    }

    if (species_is_draconian(sp))
        draconian_setup();

    update_vision_range(); // for Ba, and for DS with Nightstalker

    if ((old_sp == SP_OCTOPODE) != (sp == SP_OCTOPODE))
    {
        _swap_equip(EQ_LEFT_RING, EQ_RING_ONE);
        _swap_equip(EQ_RIGHT_RING, EQ_RING_TWO);
        // All species allow exactly one amulet.
    }

    // FIXME: this checks only for valid slots, not for suitability of the
    // item in question. This is enough to make assertions happy, though.
    for (int i = EQ_FIRST_EQUIP; i < NUM_EQUIP; ++i)
        if (you_can_wear(static_cast<equipment_type>(i)) == MB_FALSE
            && you.equip[i] != -1)
        {
            mprf("%s fall%s away.",
                 you.inv[you.equip[i]].name(DESC_YOUR).c_str(),
                 you.inv[you.equip[i]].quantity > 1 ? "" : "s");
            // Unwear items without the usual processing.
            you.equip[i] = -1;
            you.melded.set(i, false);
        }

    // Sanitize skills.
    fixup_skills();

    calc_hp();
    calc_mp();

    // The player symbol depends on species.
    update_player_symbol();
#ifdef USE_TILE
    init_player_doll();
#endif
    redraw_screen();
}

// A random valid (selectable on the new game screen) species.
species_type random_starting_species()
{
    const auto species = playable_species();
    return species[random2(species.size())];
}

// Ensure the species isn't SP_RANDOM/SP_VIABLE and it has recommended jobs
// (old disabled species have none).
bool is_starting_species(species_type species)
{
    return species < NUM_SPECIES
        && !get_species_def(species).recommended_jobs.empty();
}

// A random non-base draconian colour appropriate for the player.
draconian_colour random_draconian_colour()
{
    if (you.char_class == JOB_MUMMY || one_chance_in(50))
        return random_choose(DR_BLACK, DR_OLIVE, DR_BONE, DR_TEAL);
    if (you.char_class == JOB_DEMIGOD && !one_chance_in(30))
        return random_choose(DR_GOLDEN, DR_PEARL, DR_BLOOD, DR_PLATINUM);
    return random_choose(DR_RED, DR_WHITE, DR_BLUE, DR_CYAN, DR_SILVER, 
        DR_GREEN, DR_PINK, DR_PURPLE, DR_LIME, DR_MAGENTA, DR_BLACK, DR_SCINTILLATING);
}

bool species_is_removed(species_type species)
{
    if (get_species_def(species).recommended_jobs.empty())
        return true;
    return false;
}
