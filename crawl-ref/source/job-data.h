#include "tag-version.h"

enum weapon_choice
{
    WCHOICE_NONE,   ///< No weapon choice
    WCHOICE_PLAIN,  ///< Normal weapon choice
    WCHOICE_GOOD,   ///< Chooses from "good" weapons
    WCHOICE_RANGED, ///< Choice of ranged weapon
    WCHOICE_STAT,   ///< Customizable Stats, using this slot.
};

struct job_def
{
    const char* abbrev; ///< Two-letter abbreviation
    const char* name; ///< Long name
    int s, i, d; ///< Starting Str, Dex, and Int
    /// Which species are good at it
    /// No recommended species = job is disabled
    vector<species_type> recommended_species;
    /// Guaranteed starting equipment. Uses vault spec syntax, with the plus:,
    /// charges:, q:, and ego: tags supported.
    vector<string> equipment;
    weapon_choice wchoice; ///< how the weapon is chosen, if any
    vector<pair<skill_type, int>> skills; ///< starting skills
};

static const map<job_type, job_def> job_data =
{

{ JOB_ABYSSAL_KNIGHT, {
    "AK", "Abyssal Knight",
    4, 4, 4,
    { SP_HILL_ORC, SP_SPRIGGAN, SP_TROLL, SP_DRACONIAN, },
    { "leather armour" },
    WCHOICE_PLAIN,
    { { SK_FIGHTING, 3 }, { SK_ARMOUR, 1 }, { SK_DODGING, 1 },
      { SK_INVOCATIONS, 2 }, { SK_WEAPON, 2 }, },
} },

{ JOB_AIR_ELEMENTALIST, {
    "AE", "Air Elementalist",
    0, 7, 5,
    { SP_DEEP_ELF, SP_TENGU, SP_DRACONIAN, SP_SILENT_SPECTRE, },
    { "robe", "book of Air" },
    WCHOICE_NONE,
    { { SK_AIR_MAGIC, 3 }, { SK_SPELLCASTING, 2 },
      { SK_DODGING, 2 }, { SK_STEALTH, 2 }, },
} },

{ JOB_ARCANE_MARKSMAN, {
    "AM", "Arcane Marksman",
    2, 5, 5,
    { SP_FORMICID, SP_DEEP_ELF, SP_KOBOLD, SP_SPRIGGAN, SP_TROLL, },
    { "robe", "book of Debilitation" },
    WCHOICE_RANGED,
    { { SK_FIGHTING, 1 }, { SK_DODGING, 2 }, { SK_SPELLCASTING, 1 },
      { SK_HEXES, 3 }, { SK_WEAPON, 2 }, },
} },

{ JOB_ARTIFICER, {
    "Ar", "Artificer",
    4, 3, 5,
    { SP_KOBOLD, SP_SPRIGGAN, SP_DRACONIAN, },
    { "club ego:spectral", "leather armour", "wand of flame charges:15",
      "wand of enslavement charges:15", "wand of random effects charges:15" },
    WCHOICE_NONE,
    { { SK_EVOCATIONS, 3 }, { SK_DODGING, 1 }, { SK_FIGHTING, 1 },
      { SK_ARMOUR, 1 }, { SK_STEALTH, 1 }, },
} },

{ JOB_BERSERKER, {
    "Be", "Berserker",
    9, -1, 4,
    { SP_HILL_ORC, SP_OGRE, SP_MINOTAUR, SP_GARGOYLE, },
    { "animal skin" },
    WCHOICE_PLAIN,
    { { SK_FIGHTING, 3 }, { SK_DODGING, 2 }, { SK_WEAPON, 3 }, },
} },

{ JOB_NOBLE,{
    "No", "Noble",
    4, 4, 4,
    { SP_HUMAN, SP_DEEP_ELF, SP_HILL_ORC, SP_LIGNIFITE, },
    { "robe" },
    WCHOICE_STAT,
    { {SK_FIGHTING, 1 } , {SK_STEALTH, 1}, {SK_SPELLCASTING, 1}, },
} },

{ JOB_CHAOS_KNIGHT, {
    "CK", "Chaos Knight",
    4, 4, 4,
    { SP_HILL_ORC, SP_TROLL, SP_MINOTAUR, SP_DRACONIAN, SP_LIGNIFITE, },
    { "leather armour plus:2" },
    WCHOICE_PLAIN,
    { { SK_FIGHTING, 3 }, { SK_ARMOUR, 1 }, { SK_DODGING, 1 },
      { SK_WEAPON, 3 } },
} },

{ JOB_XOM, { // Not a Starting Job.
    "AX", "Xom-Abandoning Deserter",
    4, 4, 4,
    {},
    {},
    WCHOICE_NONE,
    {},
}},

{ JOB_EARTH_ELEMENTALIST, {
    "EE", "Earth Elementalist",
    0, 7, 5,
    { SP_DEEP_ELF, SP_SPRIGGAN, SP_GARGOYLE, SP_GHOUL, 
      SP_OCTOPODE, SP_LIGNIFITE,},
    { "book of Geomancy", "robe", },
    WCHOICE_NONE,
    { { SK_TRANSMUTATIONS, 1 }, { SK_EARTH_MAGIC, 3 }, { SK_SPELLCASTING, 2 },
      { SK_DODGING, 2 }, { SK_STEALTH, 2 }, }
} },

{ JOB_ENCHANTER, {
    "En", "Enchanter",
    0, 7, 5,
    { SP_DEEP_ELF, SP_FELID, SP_KOBOLD, SP_SPRIGGAN, SP_SILENT_SPECTRE, },
    { "dagger plus:1", "robe", "book of Maledictions" },
    WCHOICE_NONE,
    { { SK_WEAPON, 1 }, { SK_HEXES, 3 }, { SK_SPELLCASTING, 2 },
      { SK_DODGING, 2 }, { SK_STEALTH, 3 }, },
} },

{ JOB_FIGHTER, {
    "Fi", "Fighter",
    8, 0, 4,
    { SP_HILL_ORC, SP_TROLL, SP_MINOTAUR, SP_GARGOYLE, },
    { "scale mail", "heater shield", "potion of might" },
    WCHOICE_GOOD,
    { { SK_FIGHTING, 3 }, { SK_SHIELDS, 3 }, { SK_ARMOUR, 3 },
      { SK_WEAPON, 2 } },
} },

{ JOB_FIRE_ELEMENTALIST, {
    "FE", "Fire Elementalist",
    0, 7, 5,
    { SP_DEEP_ELF, SP_HILL_ORC, SP_TENGU, SP_MOLTEN_GARGOYLE },
    { "robe", "book of Flames" },
    WCHOICE_NONE,
    { { SK_FIRE_MAGIC, 3 }, { SK_SPELLCASTING, 2 },
      { SK_DODGING, 2 }, { SK_STEALTH, 2 }, },
} },

{ JOB_GLADIATOR, {
    "Gl", "Gladiator",
    6, 0, 6,
    { SP_HILL_ORC, SP_MINOTAUR, SP_GARGOYLE, SP_SILENT_SPECTRE, SP_DRACONIAN },
    { "leather armour", "helmet", "wand of ensnaring charges:9" },
    WCHOICE_GOOD,
    { { SK_FIGHTING, 2 }, { SK_EVOCATIONS, 2 }, { SK_DODGING, 3 },
      { SK_WEAPON, 3}, },
} },

{ JOB_MERFOLK, {
    "Mf", "Merfolk",
    6, -1, 7,
    { SP_HILL_ORC, SP_MINOTAUR, SP_GARGOYLE, SP_SILENT_SPECTRE, 
      SP_DRACONIAN },
    { "leather armour", "helmet", "wand of ensnaring charges:9" },
    WCHOICE_GOOD,
    { { SK_FIGHTING, 2 }, { SK_EVOCATIONS, 2 }, { SK_DODGING, 3 },
      { SK_WEAPON, 3}, },
} },

{ JOB_VINE_STALKER, {
    "VS", "Vine Stalker",
    5, 3, 4,
    { SP_DEEP_ELF, SP_FELID, SP_KOBOLD, SP_SPRIGGAN, },
    { "dagger plus:1", "leather armour", "book of maledictions" },
    WCHOICE_NONE,
    { { SK_HEXES, 2 }, { SK_SHORT_BLADES, 2 }, { SK_DODGING, 3 },
      { SK_STEALTH, 4}, { SK_SPELLCASTING, 1}, },
} },

{ JOB_HUNTER, {
    "Hu", "Hunter",
    4, 3, 5,
    { SP_HILL_ORC, SP_KOBOLD, SP_OGRE, SP_TROLL, },
    { "short sword", "leather armour" },
    WCHOICE_RANGED,
    { { SK_FIGHTING, 2 }, { SK_DODGING, 2 }, { SK_STEALTH, 1 },
      { SK_WEAPON, 4 }, },
} },

{ JOB_ICE_ELEMENTALIST, {
    "IE", "Ice Elementalist",
    0, 7, 5,
    { SP_DEEP_ELF, SP_DRACONIAN, SP_GARGOYLE, },
    { "robe", "book of Frost" },
    WCHOICE_NONE,
    { { SK_SUMMONINGS, 1 }, { SK_ICE_MAGIC, 3 }, { SK_SPELLCASTING, 2 },
      { SK_DODGING, 2 }, { SK_STEALTH, 2 }, },
} },

{ JOB_DELVER, {
    "De", "Delver",
    4, 2, 6,
    { SP_FELID, SP_SPRIGGAN, SP_CENTAUR, SP_KOBOLD, SP_VAMPIRE, },
    { "leather armour", "scroll of fog", "scroll of magic mapping",
      "scroll of fear", "potion of haste", "wand of digging charges:3" },
    WCHOICE_PLAIN,
    { { SK_FIGHTING, 3 }, { SK_DODGING, 2 }, { SK_STEALTH, 5 }, { SK_WEAPON, 2 }, },
} },

{ JOB_MONK, {
    "Mo", "Monk",
    3, 2, 7,
    { SP_HILL_ORC, SP_TROLL, SP_GARGOYLE, },
    { "robe" },
    WCHOICE_PLAIN,
    { { SK_FIGHTING, 3 }, { SK_WEAPON, 3 }, { SK_DODGING, 3 },
      { SK_STEALTH, 2 }, },
} },

{ JOB_NECROMANCER, {
    "Ne", "Necromancer",
    0, 7, 5,
    { SP_DEEP_ELF, SP_HILL_ORC, SP_SILENT_SPECTRE, },
    { "robe", "book of Necromancy" },
    WCHOICE_NONE,
    { { SK_SPELLCASTING, 2 }, { SK_NECROMANCY, 4 }, { SK_DODGING, 2 },
      { SK_STEALTH, 2 }, },
} },

{ JOB_SKALD, {
    "Sk", "Skald",
    3, 5, 4,
    { SP_GOBLIN, SP_HUMAN, SP_HILL_ORC, },
    { "leather armour", "book of Battle" },
    WCHOICE_PLAIN,
    { { SK_FIGHTING, 2 }, { SK_ARMOUR, 1 }, { SK_DODGING, 2 },
      { SK_SPELLCASTING, 2 }, { SK_CHARMS, 3 }, { SK_WEAPON, 2 }, },
} },

{ JOB_SUMMONER, {
    "Su", "Summoner",
    0, 7, 5,
    { SP_DEEP_ELF, SP_HILL_ORC, SP_TENGU, },
    { "robe", "book of Callings" },
    WCHOICE_NONE,
    { { SK_SUMMONINGS, 4 }, { SK_SPELLCASTING, 2 }, { SK_DODGING, 2 },
      { SK_STEALTH, 2 }, },
} },

{ JOB_TRANSMUTER, {
    "Tm", "Transmuter",
    2, 5, 5,
    { SP_DRACONIAN, SP_TROLL, SP_SILENT_SPECTRE, SP_OCTOPODE, SP_FELID, SP_OOZOMORPH, },
    { "arrow q:12", "robe", "book of Changes" },
    WCHOICE_NONE,
    { { SK_FIGHTING, 1 }, { SK_UNARMED_COMBAT, 3 }, { SK_DODGING, 2 },
      { SK_SPELLCASTING, 2 }, { SK_TRANSMUTATIONS, 2 }, },
} },

{ JOB_VENOM_MAGE, {
    "VM", "Venom Mage",
    0, 7, 5,
    { SP_DEEP_ELF, SP_SPRIGGAN, SP_TENGU, SP_FELID, },
    { "robe", "Young Poisoner's Handbook" },
    WCHOICE_NONE,
    { { SK_POISON_MAGIC, 3 }, { SK_AIR_MAGIC, 1 },  { SK_SPELLCASTING, 2 },
      { SK_DODGING, 1 }, { SK_STEALTH, 2 }, },
} },

{ JOB_WANDERER, {
    "Wn", "Wanderer",
    0, 0, 0, // Randomised
    { SP_HILL_ORC, SP_SPRIGGAN, SP_DRACONIAN, SP_HUMAN, SP_LIGNIFITE, },
    { }, // Randomised
    WCHOICE_NONE,
    { }, // Randomised
} },

{ JOB_WARPER, {
    "Wr", "Warper",
    3, 5, 4,
    { SP_FELID, SP_SILENT_SPECTRE, SP_SPRIGGAN, SP_DRACONIAN, },
    { "leather armour", "book of Spatial Translocations", "scroll of blinking" },
    WCHOICE_PLAIN,
    { { SK_FIGHTING, 2 }, { SK_ARMOUR, 1 }, { SK_DODGING, 2 },
      { SK_SPELLCASTING, 2 }, { SK_TRANSLOCATIONS, 3 },
      { SK_WEAPON, 2 }, },
} },

{ JOB_WIZARD, {
    "Wz", "Wizard",
    -1, 10, 3,
    { SP_DEEP_ELF, SP_DRACONIAN, SP_OCTOPODE, SP_HUMAN, SP_SPRIGGAN, },
    { "dagger", "robe", "hat", "book of Minor Magic" },
    WCHOICE_NONE,
    { { SK_DODGING, 2 }, { SK_STEALTH, 2 }, { SK_SPELLCASTING, 3 },
      { SK_TRANSLOCATIONS, 1 }, { SK_SUMMONINGS, 2 }, },
} },

{ JOB_PRIEST,{
    "Pr", "Priest",
    4, 4, 4,
    { SP_HILL_ORC, SP_FORMICID, SP_GOBLIN, SP_MINOTAUR, },
    { "robe" },
    WCHOICE_STAT,
    { {SK_DODGING, 2} , {SK_STEALTH, 2} , {SK_UNARMED_COMBAT, 3} , {SK_INVOCATIONS, 4} ,},
} },

{ JOB_DEMIGOD,{
    "Dg", "Demigod",
    7, 7, 7,
    { SP_GNOLL, SP_SPRIGGAN, SP_TROLL, SP_MINOTAUR, SP_DRACONIAN, },
    { "leather armour" , "book of Minor Magic" },
    WCHOICE_GOOD,
    { { SK_DODGING, 3 } ,{ SK_STEALTH, 2 } ,{ SK_WEAPON,2 } ,{ SK_FIGHTING, 1 } , {SK_SPELLCASTING, 1}, { SK_FIRE_MAGIC, 2 } },
} },

{ JOB_MUMMY,{
    "Mu", "Mummy",
    3, 11, -1,
    { SP_GNOLL, SP_SPRIGGAN, SP_DRACONIAN, SP_DEEP_ELF, SP_TENGU, },
    { "leather armour" , "book of necromancy"},
    WCHOICE_PLAIN,
    { { SK_DODGING, 3 } ,{ SK_STEALTH, 2 } ,{ SK_WEAPON,2 } , {SK_SPELLCASTING, 2} , {SK_NECROMANCY,3} },
} },

{ JOB_CENTAUR, {
    "Ce", "Centaur",
    8, 4, 0,
    { SP_OCTOPODE, SP_KOBOLD, SP_OGRE, SP_TROLL, SP_GOBLIN, },
    { "short sword", "leather armour" },
    WCHOICE_RANGED,
    { { SK_FIGHTING, 2 }, { SK_DODGING, 2 }, { SK_STEALTH, 1 },
      { SK_WEAPON, 4 }, },
} },

{ JOB_NAGA, {
    "Na", "Naga",
    5, 4, 3,
    { SP_OCTOPODE, SP_KOBOLD, SP_DEEP_ELF, SP_GNOLL, },
    { "leather armour" , "Young Poisoner's Handbook"},
    WCHOICE_PLAIN,
    { { SK_DODGING, 1 }, { SK_STEALTH, 4 }, { SK_SPELLCASTING, 1}, 
      { SK_POISON_MAGIC, 2}, { SK_WEAPON, 2 }, },
} },

{ JOB_DEMONSPAWN,{
    "Ds", "Demonspawn",
    6, 4, 4,
    { SP_HILL_ORC, SP_TROLL, SP_FELID, SP_LIGNIFITE, SP_DRACONIAN, },
    { "leather armour" , "book of Minor Magic" },
    WCHOICE_GOOD,
    { { SK_DODGING, 3 } ,{ SK_STEALTH, 2 } ,{ SK_WEAPON,2 } ,{ SK_FIGHTING, 1 } ,{ SK_INVOCATIONS,3 } ,{ SK_SPELLCASTING, 1 }, { SK_FIRE_MAGIC, 2} },
} },

{ JOB_JESTER, {
    "Jr", "Jester",
    0, 6, 6,
    { SP_HUMAN, SP_DEEP_ELF, SP_KOBOLD, },
    {"robe"},
    WCHOICE_NONE,
    { { SK_DODGING, 4}, {SK_SLINGS, 4}, {SK_INVOCATIONS, 4}, {SK_FIRE_MAGIC, 4} },
} },

#if TAG_MAJOR_VERSION == 34
{ JOB_CONJURER, {
    "Cj", "Conjurer",
    0, 7, 5,
    {},
    {},
    WCHOICE_NONE,
    {},
} },

{ JOB_ASSASSIN, {
    "As", "Assassin",
    3, 3, 6,
    { },
    { },
    WCHOICE_NONE,
    { },
} },

{ JOB_DEATH_KNIGHT, {
    "DK", "Death Knight",
    0, 0, 0,
    { },
    { },
    WCHOICE_NONE,
    { },
} },

{ JOB_HEALER, {
    "He", "Healer",
    0, 0, 0,
    { },
    { },
    WCHOICE_NONE,
    { },
} },

{ JOB_STALKER, {
    "St", "Stalker",
    0, 0, 0,
    { },
    { },
    WCHOICE_NONE,
    { },
} },
#endif
};
