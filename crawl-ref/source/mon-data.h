#pragma once

#define AT_NO_ATK       {AT_NONE, AF_PLAIN, 0}

#include "enum.h"
#include "tag-version.h"

/* ******************************************************************

   (see "mon-util.h" for the gory details)

 - ordering does not matter, because seekmonster() searches the entire
   array ... probably not to most efficient thing to do, but so it goes

 - Here are the rows:
    - row 1: monster id, display character, display colour, name
    - row 2: monster flags
    - row 3: monster resistance flags
    - row 4: experience modifier, genus, species, holiness, resist magic
    - row 5: damage for each of four attacks
    - row 6: hit dice, hit points
    - row 7: AC, evasion, spells, corpse effect, shouts
    - row 8: intel, habitat, speed, energy_usage
    - row 9: gmon_use class, body size, body shape
    - row 10: tile, corpse tile

 - Some further explanations:

    - colour: if COLOUR_UNDEF, a random colour will be chosen upon
              creation. Some monsters set their colour during initialization,
              and if a default colour isn't meaningful, they should also use
              COLOUR_UNDEF.
    - name: if an empty string, name generated automagically (see moname)
    - genus: base monster "type" for a classed monsters (i.e. jackal as hound)
    - species: corpse type of monster (i.e. orc for orc wizard)
    - holiness: a bitwise OR of one or more of:
       MH_HOLY       - irritates some gods when killed, immunity from
                        holy wrath weapons. Includes good priests.
       MH_NATURAL    - baseline monster type
       MH_UNDEAD     - immunity from draining, pain, torment; resistance
                        to poison; extra damage from holy wrath;
                        affected by holy word
       MH_DEMONIC    - similar to undead, but no poison resistance
                        *no* automatic hellfire resistance
       MH_ELEMENTAL  - elemental forces and weirdness of the abyss.
       MH_CONSTRUCT  - golems and other constructs
       MH_PLANT      - plants

      along with optional flags that affect some god conducts and abilities:
       MH_EVIL       - inherently evil (and not MH_UNDEAD or MH_DEMONIC)

   exp_mod: multiplies xp value after most other calculations.
            see mi_exper_value() in mon-util.cc

   resist_magic: see mons_resist_magic() in mon-util.cc
   - If -x calculate (-x * hit dice * 4/3), else simply x.

   damage [4]
   - up to 4 different attacks

   HD: like player level, used for misc things
   avg_hp_10x: average hp for the monster, * 10 for precision
               (see hit_points() for details)

   sec: if the monster has only one possible spellbook, sec is set to that book.
     If a monster has multiple possible books, sec is set to MST_NO_SPELLS. Then
     the function _mons_spellbook_list in mon-util.cc handles the books.
   TODO: replace this system ^

   TILE_CORPSE effect
   - Describes effects of eating corpses.
     CE_NOCORPSE,        leaves no corpse
     CE_CLEAN,           can be healthily eaten
     CE_NOXIOUS,         inedible to non-Ghouls

   shouts
   - various things monsters can do upon seeing you

   intel explanation:
   - How smart it is:
   I_BRAINLESS < I_ANIMAL < I_HUMAN.
   Differences here have a wide variety of small effects; tracking distance,
   behaviour around dangerous clouds, co-operation with allies, etc.

   speed
   - Increases the store of energy that the monster uses for doing things.
   less = slower. 5 = half speed, 10 = normal, 20 = double speed.

   energy usage
   - How quickly the energy granted by speed is used up. Most monsters
   should just use DEFAULT_ENERGY, where all the different types of actions
   use 10 energy units.

   gmon_use explanation:

    Essentially a long set of booleans. MU_NOTHING overrides as "Monster never uses anything"
    MU_START_ONLY means it never picks items up so what it starts with is it. MU_DOORS is set
    to define if a mon can open doors.

    Remaining flags define sorts of items the monster is allowed to pickup and use; see
    monuse-flags.h for full list.

   size:
     SIZE_TINY,              // rats/bats
     SIZE_LITTLE,            // spriggans
     SIZE_SMALL,             // halflings/kobolds
     SIZE_MEDIUM,            // humans/elves/dwarves
     SIZE_LARGE,             // trolls/ogres/centaurs/nagas
     SIZE_BIG,               // large quadrupeds
     SIZE_GIANT,             // giants

   tile:
    - a struct with up to two elements.
      - the first is the enum for the corresponding sprite in tiles builds;
        TILEP_MONS_PROGRAM_BUG for special cases.
      - the second is the way in which the tile may vary; e.g. over time,
        per-monster-instance, etc.

   corpse:
     - The enum for the corresponding sprite in tiles builds;
       TILE_ERROR for monsters without corpses. Only relevant for species mons.
*/

#define MOVE_ENERGY(x)     { x,  x, 10, 10, 10, 10, 10, 100}
#define ACTION_ENERGY(x)   {10, 10,  x,  x,  x,  x,  x, x * 10}
#define ATTACK_ENERGY(x)   {10, 10,  x, 10, 10, 10, 10, 100}
#define MISSILE_ENERGY(x)  {10, 10, 10,  x, 10, 10, 10, 100}
#define SPELL_ENERGY(x)    {10, 10, 10, 10,  x, 10, 10, 100}
#define SWIM_ENERGY(x)     {10,  x, 10, 10, 10, 10, 10, 100}

#define M_NOT_DANGEROUS (M_NO_EXP_GAIN | M_NO_THREAT)

static monsterentry mondata[] =
{

// The Thing That Should Not Be(tm)
// NOTE: Do not remove, or seekmonster will crash on unknown mc request!
// It is also a good prototype for new monsters.
{
    // id, glyph, colour, name
    MONS_PROGRAM_BUG, 'B', LIGHTRED, "program bug",
    // monster flags
    M_NOT_DANGEROUS | M_CANT_SPAWN,
    // resistance flags
    MR_NO_FLAGS,
    // xp modifier, genus, species, holiness, magic resistance
    10, MONS_PROGRAM_BUG, MONS_PROGRAM_BUG, MH_NATURAL, 10,
    // up to four attacks
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    // hit points
    0, 0,
    // AC, EV, spells, corpse type, shout type
    0, 0, MST_NO_SPELLS, CE_CLEAN, S_SILENT,
    // intelligence, habitat, speed, energy usage
    I_BRAINLESS, HT_LAND, 0, DEFAULT_ENERGY,
    // use type, body size, body shape
    MU_NOTHING, SIZE_GIANT, MON_SHAPE_MISC,
    {TILEP_MONS_PROGRAM_BUG}, TILE_ERROR
},

// Use this to replace removed monsters, to retain save compatibility.
// Please put it in #if TAG_MAJOR_VERSION == X, so they will go away
// after save compat is broken.
#define AXED_MON(id, name) \
{ \
    id, 'X', LIGHTRED, "removed " name, \
    M_NO_EXP_GAIN | M_CANT_SPAWN | M_UNFINISHED, \
    MR_NO_FLAGS, \
    10, MONS_PROGRAM_BUG, MONS_PROGRAM_BUG, MH_ELEMENTAL, 0, \
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK }, \
    0, 0, \
    0, 0, MST_NO_SPELLS, CE_CLEAN, S_SILENT, \
    I_BRAINLESS, HT_LAND, 0, DEFAULT_ENERGY, \
    MU_NOTHING, SIZE_GIANT, MON_SHAPE_MISC, \
    {TILEP_MONS_PROGRAM_BUG}, TILE_ERROR \
},

// Axed monsters.
// AXED_MON(MONS_MOTHER_IN_LAW)
#if TAG_MAJOR_VERSION == 34
    AXED_MON(MONS_BUMBLEBEE, "bumblebee")
    AXED_MON(MONS_WOOD_GOLEM, "wood golem")
    AXED_MON(MONS_ANT_LARVA, "ant larva")
    AXED_MON(MONS_LABORATORY_RAT, "laboratory rat")
    AXED_MON(MONS_PAN, "pan")
    AXED_MON(MONS_LAMIA, "lamia")
    AXED_MON(MONS_DEEP_DWARF_SCION, "deep dwarf scion")
    AXED_MON(MONS_DEEP_DWARF_ARTIFICER, "deep dwarf artificer")
    AXED_MON(MONS_DEEP_DWARF_NECROMANCER, "deep dwarf necromancer")
    AXED_MON(MONS_MOTH_OF_SUPPRESSION, "moth of suppression")
    AXED_MON(MONS_FORMICID_DRONE, "formicid drone")
    AXED_MON(MONS_SPIRIT_WOLF, "spirit wolf")
    AXED_MON(MONS_LAVA_FISH, "lava fish")
    AXED_MON(MONS_LEMURE, "lemure")
    AXED_MON(MONS_SPINY_WORM, "spiny worm")
    AXED_MON(MONS_DEEP_DWARF_BERSERKER, "deep dwarf berserker")
    AXED_MON(MONS_AGATE_SNAIL, "agate snail")
    AXED_MON(MONS_GIANT_CENTIPEDE, "giant centipede")
    AXED_MON(MONS_ANCIENT_BEAR, "ancient bear")
    AXED_MON(MONS_IGNIS, "ignis")
    AXED_MON(MONS_DJINNI, "djinni")
    AXED_MON(MONS_RAKSHASA_FAKE, "rakshasa fake")
    AXED_MON(MONS_MARA_FAKE, "mara fake")
    // GNOME_NO_MORE
    AXED_MON(MONS_GNOME, "gnome")
    // GOLEM_NO_MORE
    AXED_MON(MONS_CLAY_GOLEM, "clay golem")
    AXED_MON(MONS_STONE_GOLEM, "stone golem")
    AXED_MON(MONS_SPRIGGAN_ASSASSIN, "spriggan assassin")
    AXED_MON(MONS_THORN_LOTUS, "thorn lotus")
    AXED_MON(MONS_GIANT_GOLDFISH, "giant goldfish")
    AXED_MON(MONS_FLAMING_CORPSE, "flaming corpse")
    AXED_MON(MONS_GRIZZLY_BEAR, "grizzly bear")
    AXED_MON(MONS_SPRIGGAN_ENCHANTER, "spriggan enchanter")
    AXED_MON(MONS_PHOENIX, "phoenix")
    AXED_MON(MONS_SHEDU, "shedu")
    AXED_MON(MONS_PLAGUE_SHAMBLER, "plague shambler")
    AXED_MON(MONS_GIANT_SLUG, "giant slug")
    AXED_MON(MONS_FIREFLY, "firefly")
    AXED_MON(MONS_BIG_FISH, "big fish")
    AXED_MON(MONS_RAVENOUS_ITEM_MIMIC, "ravenous item mimic")
    AXED_MON(MONS_FEATURE_MIMIC, "feature mimic")
    AXED_MON(MONS_RAVENOUS_FEATURE_MIMIC, "ravenous feature mimic")
    AXED_MON(MONS_MNOLEG_TENTACLE, "mnoleg tentacle")
    AXED_MON(MONS_MNOLEG_TENTACLE_SEGMENT, "mnoleg tentacle segment")
    AXED_MON(MONS_UNBORN, "unborn")
    AXED_MON(MONS_GIANT_MITE, "giant mite")
    AXED_MON(MONS_BABY_ALLIGATOR, "baby alligator")
    AXED_MON(MONS_BORING_BEETLE, "boring beetle")
    AXED_MON(MONS_CHIMERA, "chimera")
    AXED_MON(MONS_RAVEN, "raven")
    AXED_MON(MONS_SINGULARITY, "singularity")
    AXED_MON(MONS_GRAND_AVATAR, "grand avatar")
    AXED_MON(MONS_SALAMANDER_FIREBRAND, "salamander firebrand")
    AXED_MON(MONS_OCTOPODE_CRUSHER, "octopode crusher")
    AXED_MON(MONS_BRAIN_WORM, "brain worm")
    AXED_MON(MONS_DEEP_ELF_SUMMONER, "deep elf summoner")
    AXED_MON(MONS_DEEP_ELF_CONJURER, "deep elf conjurer")
    AXED_MON(MONS_DEEP_ELF_PRIEST, "deep elf priest")
    AXED_MON(MONS_TRAPDOOR_SPIDER, "trapdoor spider")
    AXED_MON(MONS_SALAMANDER_STORMCALLER, "salamander stormcaller")
    AXED_MON(MONS_NORRIS, "norris")
    AXED_MON(MONS_MAUD, "maud")
    AXED_MON(MONS_WIGLAF, "wiglaf")
    AXED_MON(MONS_SHEEP, "sheep")
    AXED_MON(MONS_ANUBIS_GUARD, "anubis guard")
    AXED_MON(MONS_GRIFFON, "griffon")
    AXED_MON(MONS_PUTRID_DEMONSPAWN, "putrid demonspawn")
    AXED_MON(MONS_CHAOS_CHAMPION, "chaos champion")
    AXED_MON(MONS_WASP, "wasp")
    AXED_MON(MONS_HILL_GIANT, "hill giant")
    AXED_MON(MONS_BULTUNGIN, "bultungin")
    AXED_MON(MONS_HOMUNCULUS, "homunculus")
    AXED_MON(MONS_SOUPLING, "soupling")
    AXED_MON(MONS_BLESSED_TOE, "blessed toe")
    AXED_MON(MONS_LAVA_ORC, "lava orc")
    AXED_MON(MONS_MONSTROUS_FEATURE_MIMIC, "monstrous feature mimic")
#endif

// Used for genus monsters (which are used for grouping monsters by how they
// work and in comes-into-view messages).
#define DUMMY(id, glyph, colour, name, tile) \
{ \
    (id), (glyph), (colour), (name), \
    M_CANT_SPAWN, \
    MR_NO_FLAGS, \
    10, (id), (id), MH_ELEMENTAL, 10, \
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK }, \
    0, 0, \
    0, 0, MST_NO_SPELLS, CE_CLEAN, S_SILENT, \
    I_BRAINLESS, HT_LAND, 0, DEFAULT_ENERGY, \
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_MISC, \
    {(tile)}, TILE_ERROR \
},

// Real monsters begin here {dlb}:

// batty monsters ('b')
{
    MONS_BAT, 'b', LIGHTGREY, "bat",
    M_SEE_INVIS | M_UNBLINDABLE | M_WARM_BLOOD | M_BATTY | M_FLIES,
    MR_VUL_SLASHING,
    4, MONS_BAT, MONS_BAT, MH_NATURAL, 0,
    { {AT_HIT, AF_PLAIN, 1}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 35,
    1, 7, MST_NO_SPELLS, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_LAND, 30, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_BAT,
    {TILEP_MONS_BAT}, TILE_CORPSE_BAT
},

{
    MONS_BUTTERFLY, 'b', ETC_JEWEL, "butterfly",
    M_CONFUSED | M_NOT_DANGEROUS | M_FLIES | M_COLD_BLOOD,
    MR_VUL_POISON | MR_RES_ACID | MR_VUL_PHYSICAL,
    10, MONS_BUTTERFLY, MONS_BUTTERFLY, MH_NATURAL, 10,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 10,
    0, 25, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 25, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_TINY, MON_SHAPE_INSECT_WINGED,
    {TILEP_MONS_BUTTERFLY, TVARY_MOD}, TILE_ERROR
},

{
    MONS_GIANT_BLOWFLY, 'b', DARKGREY, "giant blowfly",
    M_CONFUSED | M_NOT_DANGEROUS | M_FLIES | M_COLD_BLOOD,
    MR_RES_ROTTING | mrd(MR_RES_POISON, 3) | MR_RES_ACID 
      | MR_VUL_SLASHING | MR_VUL_BLUDGEONING,
    10, MONS_GIANT_BLOWFLY, MONS_GIANT_BLOWFLY, MH_NATURAL, 10,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    2, 300,
    4, 35, MST_NO_SPELLS, CE_NOXIOUS, S_BUZZ,
    I_BRAINLESS, HT_LAND, 25, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_SMALL, MON_SHAPE_INSECT_WINGED,
    {TILEP_MONS_GIANT_BLOWFLY}, TILE_CORPSE_GIANT_BLOWFLY
},

{ // one vault + player transform (Vp ability)
    MONS_VAMPIRE_BAT, 'b', MAGENTA, "vampire bat",
    M_SEE_INVIS | M_WARM_BLOOD | M_BATTY | M_NO_POLY_TO | M_FLIES,
    MR_VUL_SLASHING | MR_VUL_PIERCING,
    8, MONS_BAT, MONS_VAMPIRE_BAT, MH_UNDEAD, 10,
    { {AT_BITE, AF_VAMPIRIC, 3}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    3, 105,
    1, 12, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_ANIMAL, HT_LAND, 30, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_BAT,
    {TILEP_MONS_VAMPIRE_BAT}, TILE_ERROR
},

{
    MONS_FIRE_BAT, 'b', LIGHTRED, "fire bat",
    M_SEE_INVIS | M_UNBLINDABLE | M_WARM_BLOOD | M_BATTY | M_FLIES,
    mrd(MR_RES_FIRE, 3) | MR_VUL_COLD | MR_VUL_WATER | MR_VUL_SLASHING,
    8, MONS_BAT, MONS_FIRE_BAT, MH_NATURAL, 10,
    { {AT_BITE, AF_FIRE, 6}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    5, 225,
    1, 18, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_ANIMAL, HT_LAND, 30, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_BAT,
    {TILEP_MONS_FIRE_BAT}, TILE_ERROR
},

{
    MONS_BENNU, 'b', YELLOW, "bennu",
    M_SEE_INVIS | M_WARM_BLOOD | M_FAST_REGEN | M_NO_POLY_TO | M_SPEAKS
        | M_FLIES | M_BATTY,
    mrd(MR_RES_FIRE | MR_RES_NEG, 3) | MR_RES_POISON |
        MR_RES_ROTTING | MR_VUL_WATER | MR_VUL_SLASHING,
    6, MONS_BENNU, MONS_BENNU, MH_NATURAL, 140,
    { {AT_PECK, AF_HOLY, 27}, {AT_CLAW, AF_PLAIN, 24},
      {AT_CLAW, AF_PLAIN, 24}, AT_NO_ATK },
    14, 770,
    6, 34, MST_NO_SPELLS, CE_DIVINE, S_SCREECH,
    I_HUMAN, HT_LAND, 32, DEFAULT_ENERGY,
    MU_JEWELS, SIZE_MEDIUM, MON_SHAPE_BIRD,
    {TILEP_MONS_BENNU}, TILE_CORPSE_BENNU
},

{
    MONS_SWOOPING_MAGPIE, 'b', WHITE, "swooping magpie",
    M_SEE_INVIS | M_WARM_BLOOD | M_SPEAKS | M_FLIES | M_BATTY,
    MR_VUL_FIRE | MR_VUL_SLASHING,
    12, MONS_SWOOPING_MAGPIE, MONS_SWOOPING_MAGPIE, MH_NATURAL, 40,
    { { AT_PECK, AF_SWOOP, 18 },{ AT_CLAW, AF_PLAIN, 9 },
    { AT_CLAW, AF_PLAIN, 9 }, AT_NO_ATK },
    8, 330,
    6, 22, MST_NO_SPELLS, CE_CLEAN, S_SCREECH,
    I_ANIMAL, HT_LAND, 25, DEFAULT_ENERGY,
    MU_JEWELS, SIZE_SMALL, MON_SHAPE_BIRD,
    { TILEP_MONS_SWOOPING_MAGPIE }, TILE_CORPSE_SWOOPING_MAGPIE
},

{
    MONS_CAUSTIC_SHRIKE, 'b', LIGHTGREEN, "caustic shrike",
    M_SEE_INVIS | M_UNBLINDABLE | M_WARM_BLOOD | M_FLIES | M_BATTY,
    MR_RES_COLD | MR_RES_FIRE | mrd(MR_RES_ACID, 2) | MR_VUL_SLASHING,
    15, MONS_CAUSTIC_SHRIKE, MONS_CAUSTIC_SHRIKE, MH_NATURAL, 80,
    { {AT_CLAW, AF_ACID, 36}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    18, 1080,
    8, 36, MST_NO_SPELLS, CE_CLEAN, S_SCREECH,
    I_ANIMAL, HT_LAND, 28, DEFAULT_ENERGY,
    MU_JEWELS, SIZE_TINY, MON_SHAPE_BIRD,
    {TILEP_MONS_CAUSTIC_SHRIKE}, TILE_CORPSE_CAUSTIC_SHRIKE
},

{
    MONS_SHARD_SHRIKE, 'b', LIGHTBLUE, "shard shrike",
    M_SEE_INVIS | M_UNBLINDABLE | M_WARM_BLOOD | M_BATTY | M_FLIES,
    mrd(MR_RES_COLD, 2) | MR_RES_SLASHING | MR_VUL_BLUDGEONING,
    12, MONS_SHARD_SHRIKE, MONS_SHARD_SHRIKE, MH_NATURAL, 80,
    { {AT_CLAW, AF_COLD, 21}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    21, 1050,
    2, 44, MST_SHARD_SHRIKE, CE_CLEAN, S_SCREECH,
    I_ANIMAL, HT_LAND, 30, DEFAULT_ENERGY,
    MU_JEWELS, SIZE_TINY, MON_SHAPE_BIRD,
    {TILEP_MONS_SHARD_SHRIKE}, TILE_CORPSE_SHARD_SHRIKE
},

// centaurs ('c')
{
    MONS_CENTAUR, 'c', BROWN, "centaur",
    M_WARM_BLOOD | M_ARCHER | M_SPEAKS | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_CENTAUR, MONS_CENTAUR, MH_NATURAL, 20,
    { {AT_HIT, AF_PLAIN, 10}, {AT_SHIELD, AF_PLAIN, 10}, AT_NO_ATK, AT_NO_ATK },
    4, 220,
    3, 5, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(7),
    muf(MU_MELEE | MU_WEAPON_RANGED), SIZE_LARGE, MON_SHAPE_CENTAUR,
    {TILEP_MONS_CENTAUR}, TILE_CORPSE_CENTAUR
},

{
    MONS_CENTAUR_WARRIOR, 'c', YELLOW, "centaur warrior",
    M_WARM_BLOOD | M_FIGHTER | M_ARCHER | M_SPEAKS | M_GENDERED,
    MR_VUL_SLASHING,
    12, MONS_CENTAUR, MONS_CENTAUR, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 16}, {AT_SHIELD, AF_PLAIN, 16}, AT_NO_ATK, AT_NO_ATK },
    10, 550,
    4, 7, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(7),
    MU_ALL, SIZE_LARGE, MON_SHAPE_CENTAUR,
    {TILEP_MONS_CENTAUR_WARRIOR}, TILE_ERROR
},

{
    MONS_YAKTAUR, 'c', RED, "yaktaur",
    M_WARM_BLOOD | M_ARCHER | M_SPEAKS | M_GENDERED,
    MR_VUL_SLASHING,
    9, MONS_YAKTAUR, MONS_YAKTAUR, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    8, 440,
    4, 4, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_WEAPON_RANGED | MU_WAND), SIZE_BIG, MON_SHAPE_CENTAUR,
    {TILEP_MONS_YAKTAUR}, TILE_CORPSE_YAKTAUR
},

{
    MONS_YAKTAUR_CAPTAIN, 'c', LIGHTRED, "yaktaur captain",
    M_WARM_BLOOD | M_ARCHER | M_SPEAKS | M_PREFER_RANGED | M_GENDERED,
    MR_VUL_SLASHING,
    9, MONS_YAKTAUR, MONS_YAKTAUR, MH_NATURAL, 60,
    { {AT_HIT, AF_PLAIN, 30}, {AT_SHIELD, AF_PLAIN, 30}, AT_NO_ATK, AT_NO_ATK },
    14, 770,
    5, 6, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_BIG, MON_SHAPE_CENTAUR,
    {TILEP_MONS_YAKTAUR_CAPTAIN}, TILE_ERROR
},

{
    MONS_FAUN, 'c', GREEN, "faun",
    M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_VUL_SLASHING,
    8, MONS_FAUN, MONS_FAUN, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 23}, {AT_SHIELD, AF_PLAIN, 23}, AT_NO_ATK, AT_NO_ATK },
    10, 500,
    2, 18, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_WEAPON_RANGED | MU_SHIELD | MU_WAND | MU_DOOR), SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_TAILED,
    {TILEP_MONS_FAUN}, TILE_CORPSE_FAUN
},

{
    MONS_SATYR, 'c', LIGHTGREEN, "satyr",
    M_WARM_BLOOD | M_SPEAKS | M_ARCHER | M_PREFER_RANGED | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_FAUN, MONS_SATYR, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 25}, {AT_SHIELD, AF_PLAIN, 25}, AT_NO_ATK, AT_NO_ATK },
    12, 660,
    2, 22, MST_SATYR, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_WEAPON_RANGED | MU_ARMOUR | MU_SHIELD | MU_EVOKE | MU_DOOR), SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_TAILED,
    {TILEP_MONS_SATYR}, TILE_CORPSE_SATYR
},

// draconians ('d')
{   // Base draconian.
    MONS_DRACONIAN, 'd', BROWN, "draconian",
    M_COLD_BLOOD | M_SPEAKS | M_NO_POLY_TO | M_GENDERED,
    MR_VUL_SLASHING | MR_VUL_PIERCING,
    10, MONS_DRACONIAN, MONS_DRACONIAN, MH_NATURAL, 10,
    { {AT_HIT, AF_PLAIN, 15}, {AT_SHIELD, AF_PLAIN, 15}, AT_NO_ATK, AT_NO_ATK },
    8, 640,
    10, 18, MST_NO_SPELLS, CE_CLEAN, S_ROAR, I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WEAPONS | MU_EVOKE | MU_THROW_JAVELIN | MU_THROW_NET), SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_TAILED,
    {TILEP_DRACO_BASE}, TILE_CORPSE_DRACONIAN_BROWN
},

{
    MONS_BLUE_DRACONIAN, 'd', BLUE, "blue draconian",
    M_COLD_BLOOD | M_SPEAKS | M_FLIES | M_GENDERED,
    mrd(MR_RES_ELEC, 3) | MR_VUL_SLASHING | MR_VUL_PIERCING,
    10, MONS_DRACONIAN, MONS_BLUE_DRACONIAN, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    14, 980,
    9, 16, MST_NO_SPELLS, CE_CLEAN, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN | MU_THROW_NET), SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_WINGED_TAILED,
    {TILEP_MONS_PROGRAM_BUG}, TILE_CORPSE_DRACONIAN_BLACK
},

{
    MONS_BLACK_DRACONIAN, 'd', DARKGREY, "black draconian",
    M_COLD_BLOOD | M_SPEAKS | M_FLIES | M_INVIS | M_GENDERED,
    mrd(MR_RES_NEG, 3) | MR_VUL_SLASHING | MR_VUL_PIERCING,
    10, MONS_DRACONIAN, MONS_BLACK_DRACONIAN, MH_NATURAL, 40,
    { { AT_HIT, AF_PLAIN, 20 }, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    14, 980,
    9, 16, MST_NO_SPELLS, CE_CLEAN, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN | MU_THROW_NET), SIZE_MEDIUM,
    MON_SHAPE_HUMANOID_WINGED_TAILED,
    { TILEP_MONS_PROGRAM_BUG }, TILE_CORPSE_DRACONIAN_BLACK
},

{
    MONS_PINK_DRACONIAN, 'd', LIGHTRED, "pink draconian",
    M_COLD_BLOOD | M_SPEAKS | M_FLIES | M_GENDERED,
    MR_VUL_SLASHING | MR_VUL_PIERCING,
    10, MONS_DRACONIAN, MONS_PINK_DRACONIAN, MH_NATURAL, 40,
    { { AT_HIT, AF_PLAIN, 20 }, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    14, 980,
    9, 16, MST_NO_SPELLS, CE_CLEAN, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN | MU_THROW_NET), SIZE_MEDIUM,
    MON_SHAPE_HUMANOID_WINGED_TAILED,
    { TILEP_MONS_PROGRAM_BUG }, TILE_CORPSE_DRACONIAN_PINK
},

{
    MONS_LIME_DRACONIAN, 'd', GREEN, "lime draconian",
    M_COLD_BLOOD | M_SPEAKS | M_FLIES | M_GENDERED,
    mrd(MR_RES_ACID, 2) | MR_VUL_SLASHING | MR_VUL_PIERCING,
    10, MONS_DRACONIAN, MONS_LIME_DRACONIAN, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    14, 980,
    9, 16, MST_NO_SPELLS, CE_CLEAN, S_ROAR,
    I_HUMAN, HT_SLIME, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN | MU_THROW_NET), SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_TAILED,
    {TILEP_MONS_PROGRAM_BUG}, TILE_CORPSE_DRACONIAN_LIME
},

{
    MONS_CYAN_DRACONIAN, 'd', CYAN, "cyan draconian",
    M_COLD_BLOOD | M_SPEAKS | M_FLIES | M_GENDERED,
    MR_RES_WIND | MR_VUL_SLASHING | MR_VUL_PIERCING,
    10, MONS_DRACONIAN, MONS_CYAN_DRACONIAN, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    14, 980,
    9, 16, MST_NO_SPELLS, CE_CLEAN, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN | MU_THROW_NET), SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_TAILED,
    {TILEP_MONS_PROGRAM_BUG}, TILE_CORPSE_DRACONIAN_CYAN
},

{
    MONS_GREEN_DRACONIAN, 'd', GREEN, "green draconian",
    M_COLD_BLOOD | M_SPEAKS | M_FLIES | M_GENDERED,
    MR_RES_POISON | MR_VUL_SLASHING | MR_VUL_PIERCING,
    10, MONS_DRACONIAN, MONS_GREEN_DRACONIAN, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20},
      {AT_TAIL_SLAP, AF_POISON, 15}, AT_NO_ATK },
    14, 980,
    9, 16, MST_NO_SPELLS, CE_NOXIOUS, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN | MU_THROW_NET), SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_TAILED,
    {TILEP_MONS_PROGRAM_BUG}, TILE_CORPSE_DRACONIAN_GREEN
},

{
    MONS_PURPLE_DRACONIAN, 'd', MAGENTA, "purple draconian",
    M_COLD_BLOOD | M_SPEAKS | M_FLIES | M_GENDERED,
    MR_VUL_SLASHING | MR_VUL_PIERCING,
    10, MONS_DRACONIAN, MONS_PURPLE_DRACONIAN, MH_NATURAL, 140,
    { {AT_HIT, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    14, 980,
    8, 16, MST_NO_SPELLS, CE_CLEAN, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN | MU_THROW_NET), SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_TAILED,
    {TILEP_MONS_PROGRAM_BUG}, TILE_CORPSE_DRACONIAN_PURPLE
},

{
    MONS_RED_DRACONIAN, 'd', RED, "red draconian",
    M_COLD_BLOOD | M_SPEAKS | M_FLIES | M_GENDERED,
    MR_RES_FIRE | MR_VUL_SLASHING | MR_VUL_PIERCING,
    10, MONS_DRACONIAN, MONS_RED_DRACONIAN, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    14, 980,
    9, 16, MST_NO_SPELLS, CE_CLEAN, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN | MU_THROW_NET), SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_TAILED,
    {TILEP_MONS_PROGRAM_BUG}, TILE_CORPSE_DRACONIAN_RED
},

{
    MONS_WHITE_DRACONIAN, 'd', WHITE, "white draconian",
    M_COLD_BLOOD | M_SPEAKS | M_FLIES | M_GENDERED,
    MR_RES_COLD | MR_VUL_SLASHING | MR_VUL_PIERCING,
    10, MONS_DRACONIAN, MONS_WHITE_DRACONIAN, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    14, 980,
    9, 16, MST_NO_SPELLS, CE_CLEAN, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN | MU_THROW_NET), SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_TAILED,
    {TILEP_MONS_PROGRAM_BUG}, TILE_CORPSE_DRACONIAN_WHITE
},

{
    MONS_SILVER_DRACONIAN, 'd', LIGHTGREY, "silver draconian",
    M_COLD_BLOOD | M_FLIES | M_SPEAKS | M_GENDERED,
    MR_VUL_SLASHING | MR_VUL_PIERCING,
    10, MONS_DRACONIAN, MONS_SILVER_DRACONIAN, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 25}, {AT_SHIELD, AF_PLAIN, 25}, 
      {AT_TAIL_SLAP, AF_PLAIN, 15}, AT_NO_ATK },
    14, 980,
    16, 16, MST_NO_SPELLS, CE_CLEAN, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN | MU_THROW_NET), SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_TAILED,
    {TILEP_MONS_PROGRAM_BUG}, TILE_CORPSE_DRACONIAN_SILVER
},

{
    MONS_BONE_DRACONIAN, 'd', DARKGREY, "bone draconian",
    M_FLIES | M_SPEAKS | M_GENDERED,
    MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    10, MONS_DRACONIAN, MONS_BONE_DRACONIAN, MH_UNDEAD, 40,
    { {AT_HIT, AF_PLAIN, 30}, {AT_SHIELD, AF_PLAIN, 30}, 
      {AT_TAIL_SLAP, AF_PLAIN, 15}, AT_NO_ATK },
    14, 780,
    24, 11, MST_NO_SPELLS, CE_NOCORPSE, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN | MU_THROW_NET), SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_TAILED,
    {TILEP_MONS_PROGRAM_BUG}, TILEP_ERROR
},

{
    MONS_OLIVE_DRACONIAN, 'd', BROWN, "olive draconian",
    M_FLIES | M_SPEAKS | M_GENDERED,
    MR_VUL_PIERCING,
    10, MONS_DRACONIAN, MONS_OLIVE_DRACONIAN, MH_UNDEAD, 40,
    { { AT_HIT, AF_PLAIN, 25 }, {AT_SHIELD, AF_PLAIN, 25}, AT_NO_ATK,
    AT_NO_ATK },
    14, 980,
    9, 15, MST_NO_SPELLS, CE_NOXIOUS, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN | MU_THROW_NET), SIZE_MEDIUM,
    MON_SHAPE_HUMANOID_TAILED,
    { TILEP_MONS_PROGRAM_BUG }, TILE_CORPSE_DRACONIAN_OLIVE
},

{
    MONS_TEAL_DRACONIAN, 'd', LIGHTBLUE, "turquoise draconian",
    M_FLIES | M_SPEAKS | M_INSUBSTANTIAL | M_GENDERED,
    MR_RES_PHYSICAL,
    10, MONS_DRACONIAN, MONS_TEAL_DRACONIAN, MH_UNDEAD, 40,
    { { AT_HIT, AF_PLAIN, 25 }, {AT_SHIELD, AF_PLAIN, 25}, AT_NO_ATK,
    AT_NO_ATK },
    14, 980,
    9, 27, MST_NO_SPELLS, CE_NOCORPSE, S_ROAR,
    I_HUMAN, HT_INCORPOREAL, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN | MU_THROW_NET), SIZE_MEDIUM,
    MON_SHAPE_HUMANOID_TAILED,
    { TILEP_MONS_PROGRAM_BUG }, TILEP_ERROR
},

{
    MONS_GOLDEN_DRACONIAN, 'd', YELLOW, "golden draconian",
    M_FLIES | M_SPEAKS | M_COLD_BLOOD | M_GENDERED,
    MR_RES_FIRE | MR_RES_COLD | MR_RES_POISON | MR_VUL_PIERCING,
    10, MONS_DRACONIAN, MONS_GOLDEN_DRACONIAN, MH_NATURAL, 40,
    { { AT_HIT, AF_PLAIN, 25 }, {AT_SHIELD, AF_PLAIN, 25},
      { AT_TAIL_SLAP, AF_POISON, 15}, AT_NO_ATK },
    14, 980,
    9, 21, MST_NO_SPELLS, CE_CLEAN, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN | MU_THROW_NET), SIZE_MEDIUM,
    MON_SHAPE_HUMANOID_TAILED,
    { TILEP_MONS_PROGRAM_BUG }, TILE_CORPSE_DRACONIAN_GOLDEN
},

{
    MONS_PEARL_DRACONIAN, 'd', LIGHTMAGENTA, "pearl draconian",
    M_FLIES | M_SPEAKS | M_COLD_BLOOD | M_GENDERED,
    MR_VUL_PIERCING,
    10, MONS_DRACONIAN, MONS_PEARL_DRACONIAN, MH_NATURAL | MH_HOLY, 40,
    { { AT_HIT, AF_PLAIN, 25 }, {AT_SHIELD, AF_PLAIN, 25}, AT_NO_ATK, AT_NO_ATK },
    14, 980,
    16, 19, MST_NO_SPELLS, CE_CLEAN, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN | MU_THROW_NET), SIZE_MEDIUM,
    MON_SHAPE_HUMANOID_TAILED,
    { TILEP_MONS_PROGRAM_BUG }, TILE_CORPSE_DRACONIAN_PEARL
},

{
    MONS_SCINTILLATING_DRACONIAN, 'd', LIGHTCYAN, "scintillating draconian",
    M_FLIES | M_SPEAKS | M_COLD_BLOOD | M_GENDERED,
    MR_VUL_PIERCING,
    10, MONS_DRACONIAN, MONS_SCINTILLATING_DRACONIAN, MH_NATURAL, 40,
    { { AT_HIT, AF_PLAIN, 25 }, {AT_SHIELD, AF_PLAIN, 25}, AT_NO_ATK, AT_NO_ATK },
    14, 980,
    9, 25, MST_NO_SPELLS, CE_CLEAN, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN | MU_THROW_NET), SIZE_MEDIUM,
    MON_SHAPE_HUMANOID_TAILED,
    { TILEP_MONS_PROGRAM_BUG }, TILE_CORPSE_DRACONIAN_SCINTILLATING
},

{
    MONS_BLOOD_DRACONIAN, 'd', ETC_DARK, "blood draconian",
    M_FLIES | M_SPEAKS | M_COLD_BLOOD | M_GENDERED,
    MR_RES_HELLFIRE | MR_VUL_SLASHING | MR_VUL_PIERCING,
    10, MONS_DRACONIAN, MONS_BLOOD_DRACONIAN, MH_NATURAL | MH_DEMONIC, 40,
    { { AT_HIT, AF_PLAIN, 25 }, {AT_SHIELD, AF_PLAIN, 25}, AT_NO_ATK, AT_NO_ATK },
    14, 980,
    9, 26, MST_NO_SPELLS, CE_CLEAN, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN | MU_THROW_NET), SIZE_MEDIUM,
    MON_SHAPE_HUMANOID_TAILED,
    { TILEP_MONS_PROGRAM_BUG }, TILE_CORPSE_DRACONIAN_BLOOD
},

{
    MONS_PLATINUM_DRACONIAN, 'd', ETC_SILVER, "platinum draconian",
    M_FLIES | M_SPEAKS | M_COLD_BLOOD | M_GENDERED,
    MR_VUL_PIERCING,
    10, MONS_DRACONIAN, MONS_PLATINUM_DRACONIAN, MH_NATURAL, 40,
    { { AT_HIT, AF_PLAIN, 25 }, {AT_SHIELD, AF_PLAIN, 25}, AT_NO_ATK, AT_NO_ATK },
    14, 980,
    9, 26, MST_NO_SPELLS, CE_CLEAN, S_ROAR,
    I_HUMAN, HT_LAND, 12, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN | MU_THROW_NET), SIZE_MEDIUM,
    MON_SHAPE_HUMANOID_TAILED,
    { TILEP_MONS_PROGRAM_BUG }, TILE_CORPSE_DRACONIAN_PLATINUM
},

{
    MONS_MAGENTA_DRACONIAN, 'd', LIGHTMAGENTA, "magenta draconian",
    M_FLIES | M_SPEAKS | M_COLD_BLOOD | M_GENDERED,
    MR_VUL_SLASHING | MR_VUL_PIERCING,
    10, MONS_DRACONIAN, MONS_MAGENTA_DRACONIAN, MH_NATURAL, 40,
    { { AT_HIT, AF_PLAIN, 25 }, {AT_SHIELD, AF_PLAIN, 25}, AT_NO_ATK, AT_NO_ATK },
    14, 980,
    9, 21, MST_NO_SPELLS, CE_CLEAN, S_ROAR,
    I_HUMAN, HT_LAND, 12, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN | MU_THROW_NET), SIZE_MEDIUM,
    MON_SHAPE_HUMANOID_TAILED,
    { TILEP_MONS_PROGRAM_BUG }, TILE_CORPSE_DRACONIAN_MAGENTA
},

// nonbase draconians ('q')
// Nonbase draconian AC and EV is additive with the base type.
{
    MONS_DRACONIAN_STORMCALLER, 'q', BROWN, "draconian stormcaller",
    M_COLD_BLOOD | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    MR_NO_FLAGS,
    10, MONS_DRACONIAN, MONS_DRACONIAN, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    16, 880,
    0, 3, MST_DRACONIAN_STORMCALLER, CE_CLEAN, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_TAILED,
    {TILEP_DRACO_STORMCALLER}, TILE_ERROR
},

{
    MONS_DRACONIAN_MONK, 'q', BLUE, "draconian monk",
    M_FIGHTER | M_COLD_BLOOD | M_SPEAKS | M_GENDERED,
    MR_NO_FLAGS,
    10, MONS_DRACONIAN, MONS_DRACONIAN, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 35}, {AT_SHIELD, AF_PLAIN, 35},
      {AT_KICK, AF_PLAIN, 30}, AT_NO_ATK },
    16, 1200,
    -3, 18, MST_NO_SPELLS, CE_CLEAN, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN | MU_THROW_NET), SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_TAILED,
    {TILEP_DRACO_MONK}, TILE_ERROR
},

{
    MONS_DRACONIAN_SHIFTER, 'q', LIGHTCYAN, "draconian shifter",
    M_COLD_BLOOD | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    MR_NO_FLAGS,
    10, MONS_DRACONIAN, MONS_DRACONIAN, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    16, 960,
    -1, 9, MST_DRACONIAN_SHIFTER, CE_CLEAN, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN | MU_THROW_NET), SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_TAILED,
    {TILEP_DRACO_SHIFTER}, TILE_ERROR
},

{
    MONS_DRACONIAN_SHARPSHOOTER, 'q', LIGHTGREEN, "draconian sharpshooter",
    M_COLD_BLOOD | M_SPEAKS | M_SPELLCASTER | M_PREFER_RANGED | M_GENDERED,
    MR_NO_FLAGS,
    10, MONS_DRACONIAN, MONS_DRACONIAN, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 35}, {AT_SHIELD, AF_PLAIN, 35}, AT_NO_ATK, AT_NO_ATK },
    16, 800,
    -3, 12, MST_DRACONIAN_SHARPSHOOTER, CE_CLEAN, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID_TAILED,
    {TILEP_DRACO_SHARPSHOOTER}, TILE_ERROR
},

{
    MONS_DRACONIAN_ANNIHILATOR, 'q', LIGHTBLUE, "draconian annihilator",
    M_COLD_BLOOD | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    MR_NO_FLAGS,
    10, MONS_DRACONIAN, MONS_DRACONIAN, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 15}, {AT_SHIELD, AF_PLAIN, 15}, AT_NO_ATK, AT_NO_ATK },
    16, 800,
    -1, 2, MST_ANNIHILATOR, CE_CLEAN, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_TAILED,
    {TILEP_DRACO_ANNIHILATOR}, TILE_ERROR
},

{
    MONS_DRACONIAN_KNIGHT, 'q', CYAN, "draconian knight",
    M_FIGHTER | M_COLD_BLOOD | M_SPEAKS | M_GENDERED,
    MR_RES_SLASHING,
    10, MONS_DRACONIAN, MONS_DRACONIAN, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 27}, {AT_SHIELD, AF_PLAIN, 27}, AT_NO_ATK, AT_NO_ATK },
    16, 1120,
    5, 4, MST_DRACONIAN_KNIGHT, CE_CLEAN, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_NET), SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_TAILED,
    {TILEP_DRACO_KNIGHT}, TILE_ERROR
},

{
    MONS_DRACONIAN_SCORCHER, 'q', LIGHTRED, "draconian scorcher",
    M_COLD_BLOOD | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    MR_NO_FLAGS,
    10, MONS_DRACONIAN, MONS_DRACONIAN, MH_NATURAL | MH_EVIL, 40,
    { {AT_HIT, AF_PLAIN, 15}, {AT_SHIELD, AF_PLAIN, 15}, AT_NO_ATK, AT_NO_ATK },
    16, 800,
    -1, 2, MST_DRACONIAN_SCORCHER, CE_CLEAN, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_TAILED,
    {TILEP_DRACO_SCORCHER}, TILE_ERROR
},

// elves ('e')
{
    MONS_ELF, 'e', LIGHTRED, "elf",
    M_WARM_BLOOD | M_SPEAKS | M_NO_POLY_TO | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_ELF, MONS_ELF, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 10}, {AT_SHIELD, AF_PLAIN, 10}, AT_NO_ATK, AT_NO_ATK },
    8, 360,
    3, 11, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_WEAPON_RANGED), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ELF}, TILE_CORPSE_ELF
},

{
    MONS_DEEP_ELF_KNIGHT, 'e', CYAN, "deep elf knight",
    M_WARM_BLOOD | M_FIGHTER | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_ELF, MONS_ELF, MH_NATURAL, 80,
    { {AT_HIT, AF_PLAIN, 21}, {AT_SHIELD, AF_PLAIN, 21}, AT_NO_ATK, AT_NO_ATK },
    11, 495,
    0, 21, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DEEP_ELF_KNIGHT}, TILE_ERROR
},

{
    MONS_DEEP_ELF_ARCHER, 'e', BROWN, "deep elf archer",
    M_WARM_BLOOD | M_ARCHER | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_ELF, MONS_ELF, MH_NATURAL, 80,
    { {AT_HIT, AF_PLAIN, 21}, {AT_SHIELD, AF_PLAIN, 21}, AT_NO_ATK, AT_NO_ATK },
    10, 500,
    0, 18, MST_DEEP_ELF_ARCHER, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DEEP_ELF_ARCHER}, TILE_ERROR
},

{
    MONS_DEEP_ELF_BLADEMASTER, 'e', LIGHTCYAN, "deep elf blademaster",
    M_WARM_BLOOD | M_FIGHTER | M_TWO_WEAPONS | M_SPEAKS | M_GENDERED,
    MR_VUL_PIERCING | MR_RES_SLASHING,
    10, MONS_ELF, MONS_ELF, MH_NATURAL, 120,
    { {AT_HIT, AF_PLAIN, 25}, {AT_HIT, AF_PLAIN, 25}, AT_NO_ATK, AT_NO_ATK },
    16, 1040,
    0, 38, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 15, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DEEP_ELF_BLADEMASTER}, TILE_ERROR
},

{
    MONS_DEEP_ELF_MASTER_ARCHER, 'e', LIGHTGREY, "deep elf master archer",
    M_WARM_BLOOD | M_ARCHER | M_PREFER_RANGED | M_SPEAKS | M_GENDERED,
    MR_VUL_PIERCING | MR_RES_SLASHING,
    30, MONS_ELF, MONS_ELF, MH_NATURAL, 100,
    // Attack damage gets rolled into their ranged attacks.
    { {AT_HIT, AF_PLAIN, 25}, {AT_SHIELD, AF_PLAIN, 25}, AT_NO_ATK, AT_NO_ATK },
    15, 750,
    0, 31, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, MISSILE_ENERGY(5),
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DEEP_ELF_MASTER_ARCHER}, TILE_ERROR
},

{
    MONS_DEEP_ELF_MAGE, 'e', MAGENTA, "deep elf mage",
    M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_VUL_SLASHING,
    12, MONS_ELF, MONS_ELF, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 5}, {AT_SHIELD, AF_PLAIN, 5}, AT_NO_ATK, AT_NO_ATK },
    9, 405,
    0, 9, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DEEP_ELF_MAGE}, TILE_ERROR
},

{
    MONS_DEEP_ELF_HIGH_PRIEST, 'e', LIGHTGREEN, "deep elf high priest",
    M_SPEAKS | M_WARM_BLOOD | M_SEE_INVIS | M_SPELLCASTER | M_GENDERED,
    MR_VUL_SLASHING,
    15, MONS_ELF, MONS_ELF, MH_NATURAL | MH_EVIL, 80,
    { {AT_HIT, AF_PLAIN, 14}, {AT_SHIELD, AF_PLAIN, 14}, AT_NO_ATK, AT_NO_ATK },
    11, 495,
    3, 18, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN | MU_THROW_NET), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DEEP_ELF_HIGH_PRIEST}, TILE_ERROR
},

{
    MONS_DEEP_ELF_DEMONOLOGIST, 'e', YELLOW, "deep elf demonologist",
    M_WARM_BLOOD | M_SEE_INVIS | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    MR_VUL_SLASHING,
    20, MONS_ELF, MONS_ELF, MH_NATURAL | MH_EVIL, 100,
    { {AT_HIT, AF_PLAIN, 12}, {AT_SHIELD, AF_PLAIN, 12}, AT_NO_ATK, AT_NO_ATK },
    12, 540,
    0, 18, MST_DEEP_ELF_DEMONOLOGIST, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_NET), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DEEP_ELF_DEMONOLOGIST}, TILE_ERROR
},

{
    MONS_DEEP_ELF_ANNIHILATOR, 'e', LIGHTBLUE, "deep elf annihilator",
    M_WARM_BLOOD | M_SEE_INVIS | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_ELF, MONS_ELF, MH_NATURAL, 120,
    { {AT_HIT, AF_PLAIN, 12}, {AT_SHIELD, AF_PLAIN, 12}, AT_NO_ATK, AT_NO_ATK },
    15, 675,
    0, 21, MST_ANNIHILATOR, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DEEP_ELF_ANNIHILATOR}, TILE_ERROR
},

{
    MONS_DEEP_ELF_SORCERER, 'e', LIGHTMAGENTA, "deep elf sorcerer",
    M_WARM_BLOOD | M_SEE_INVIS | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    MR_VUL_SLASHING,
    17, MONS_ELF, MONS_ELF, MH_NATURAL | MH_EVIL, 120,
    { {AT_HIT, AF_PLAIN, 12}, {AT_SHIELD, AF_PLAIN, 12}, AT_NO_ATK, AT_NO_ATK },
    14, 630,
    0, 18, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DEEP_ELF_SORCERER}, TILE_ERROR
},

{
    MONS_DEEP_ELF_DEATH_MAGE, 'e', WHITE, "deep elf death mage",
    M_WARM_BLOOD | M_SEE_INVIS | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_ELF, MONS_ELF, MH_NATURAL | MH_EVIL, 120,
    { {AT_HIT, AF_PLAIN, 12}, {AT_SHIELD, AF_PLAIN, 12}, AT_NO_ATK, AT_NO_ATK },
    15, 675,
    0, 18, MST_DEEP_ELF_DEATH_MAGE, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DEEP_ELF_DEATH_MAGE}, TILE_ERROR
},

{
    MONS_DEEP_ELF_ELEMENTALIST, 'e', LIGHTRED, "deep elf elementalist",
    M_WARM_BLOOD | M_SEE_INVIS | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    MR_VUL_SLASHING,
    17, MONS_ELF, MONS_ELF, MH_NATURAL, 120,
    { {AT_HIT, AF_PLAIN, 12}, {AT_SHIELD, AF_PLAIN, 12}, AT_NO_ATK, AT_NO_ATK },
    14, 630,
    0, 22, MST_DEEP_ELF_ELEMENTALIST, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DEEP_ELF_ELEMENTALIST, TVARY_CYCLE}, TILE_ERROR
},

// Mobile plants and fungi ('f')
{
    MONS_WANDERING_MUSHROOM, 'f', BROWN, "wandering mushroom",
    M_NO_FLAGS,
    MR_RES_POISON | MR_VUL_ACID | MR_RES_PIERCING | MR_VUL_SLASHING,
    10, MONS_FUNGUS, MONS_WANDERING_MUSHROOM, MH_PLANT, 40,
    { {AT_SPORE, AF_CONFUSE, 20}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    8, 440,
    5, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_FUNGUS,
    {TILEP_MONS_WANDERING_MUSHROOM}, TILE_ERROR
},

{
    MONS_DEATHCAP, 'f', LIGHTMAGENTA, "deathcap",
    M_NO_FLAGS,
    MR_RES_COLD | MR_VUL_ACID | MR_RES_PIERCING | MR_VUL_SLASHING,
    6, MONS_FUNGUS, MONS_WANDERING_MUSHROOM, MH_UNDEAD, 80,
    { {AT_SPORE, AF_CONFUSE, 33}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    13, 520,
    5, 0, MST_DEATHCAP, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_FUNGUS,
    {TILEP_MONS_DEATHCAP}, TILE_ERROR
},

{
    MONS_THORN_HUNTER, 'f', WHITE, "thorn hunter",
    M_SEE_INVIS,
    MR_RES_POISON | MR_VUL_FIRE | MR_RES_ACID 
      | MR_RES_BLUDGEONING | MR_VUL_SLASHING | MR_RES_PIERCING,
    14, MONS_THORN_HUNTER, MONS_THORN_HUNTER, MH_PLANT, 100,
    { {AT_HIT, AF_PLAIN, 27}, {AT_HIT, AF_PLAIN, 23}, AT_NO_ATK,
       AT_NO_ATK },
    15, 975,
    9, 23, MST_THORN_HUNTER, CE_NOCORPSE, S_SILENT,
    I_ANIMAL, HT_AMPHIBIOUS, 12, SWIM_ENERGY(12),
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_PLANT,
    {TILEP_MONS_THORN_HUNTER}, TILE_ERROR
},

{
    MONS_SHAMBLING_MANGROVE, 'f', LIGHTRED, "shambling mangrove",
    M_NO_FLAGS,
    MR_RES_POISON | MR_RES_ACID | MR_RES_PIERCING,
    12, MONS_SHAMBLING_MANGROVE, MONS_SHAMBLING_MANGROVE, MH_PLANT, 100,
    { {AT_HIT, AF_PLAIN, 41}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    13, 910,
    13, 2, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_AMPHIBIOUS, 8, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_BIG, MON_SHAPE_PLANT,
    {TILEP_MONS_TREANT}, TILE_ERROR
},

{
    MONS_VINE_STALKER, 'f', GREEN, "vine stalker",
    M_SPEAKS | M_NO_POLY_TO | M_FAST_REGEN | M_GENDERED,
    MR_RES_ACID | MR_RES_PIERCING | MR_VUL_SLASHING,
    10, MONS_VINE_STALKER, MONS_VINE_STALKER, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 10}, {AT_SHIELD, AF_PLAIN, 10}, 
      {AT_BITE, AF_ANTIMAGIC, 10}, AT_NO_ATK },
    8, 360,
    2, 12, MST_NO_SPELLS, CE_NOCORPSE, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_WEAPON_RANGED), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_VINE_STALKER}, TILE_ERROR
},

// goblins and other small humanoids ('g')
{
    MONS_GOBLIN, 'g', LIGHTGREY, "goblin",
    M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_GOBLIN, MONS_GOBLIN, MH_NATURAL, 0,
    { {AT_HIT, AF_PLAIN, 4}, {AT_SHIELD, AF_PLAIN, 4}, AT_NO_ATK, AT_NO_ATK },
    1, 40,
    0, 6, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_WEAPON_RANGED | MU_THROW_ROCK | MU_CONSUMABLES), SIZE_SMALL, MON_SHAPE_HUMANOID,
    {TILEP_MONS_GOBLIN}, TILE_CORPSE_GOBLIN
},

{
    MONS_HOBGOBLIN, 'g', BROWN, "hobgoblin",
    M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_GOBLIN, MONS_HOBGOBLIN, MH_NATURAL, 0,
    { {AT_HIT, AF_PLAIN, 5}, {AT_SHIELD, AF_PLAIN, 5}, AT_NO_ATK, AT_NO_ATK },
    1, 55,
    2, 5, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_THROW_ROCK | MU_CONSUMABLES), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_HOBGOBLIN}, TILE_CORPSE_HOBGOBLIN
},

{
    MONS_GNOLL, 'g', YELLOW, "gnoll",
    M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_GNOLL, MONS_GNOLL, MH_NATURAL, 10,
    { {AT_HIT, AF_PLAIN, 9}, {AT_SHIELD, AF_PLAIN, 9}, AT_NO_ATK, AT_NO_ATK },
    2, 130,
    2, 5, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_THROW_NET), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_GNOLL}, TILE_CORPSE_GNOLL
},

{
    MONS_GNOLL_SHAMAN, 'g', WHITE, "gnoll shaman",
    M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_GNOLL, MONS_GNOLL, MH_NATURAL, 20,
    { {AT_HIT, AF_PLAIN, 10}, {AT_SHIELD, AF_PLAIN, 10}, AT_NO_ATK, AT_NO_ATK },
    3, 195,
    2, 6, MST_GNOLL_SHAMAN, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_GNOLL_SHAMAN}, TILE_ERROR
},

{
    MONS_GNOLL_SERGEANT, 'g', CYAN, "gnoll sergeant",
    M_FIGHTER | M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_VUL_PIERCING | MR_RES_SLASHING,
    10, MONS_GNOLL, MONS_GNOLL, MH_NATURAL, 20,
    { {AT_HIT, AF_PLAIN, 11}, {AT_SHIELD, AF_PLAIN, 11}, AT_NO_ATK, AT_NO_ATK },
    4, 260,
    2, 6, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_NET), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_GNOLL_SERGEANT}, TILE_ERROR
},

{
    MONS_BOGGART, 'g', MAGENTA, "boggart",
    M_SEE_INVIS | M_WARM_BLOOD | M_SPELLCASTER | M_GENDERED,
    MR_VUL_PHYSICAL,
    14, MONS_BOGGART, MONS_BOGGART, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 5}, {AT_SHIELD, AF_PLAIN, 5}, AT_NO_ATK, AT_NO_ATK },
    4, 200,
    0, 27, MST_BOGGART, CE_NOXIOUS, S_SHOUT,
    I_HUMAN, HT_LAND, 12, DEFAULT_ENERGY,
    muf(MU_WEAPONS | MU_DOOR), SIZE_LITTLE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_BOGGART}, TILE_CORPSE_BOGGART
},

{ // Another dummy monster.
    MONS_DWARF, 'g', GREEN, "dwarf",
    M_WARM_BLOOD | M_SPEAKS | M_NO_POLY_TO | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_DWARF, MONS_DWARF, MH_NATURAL, 20,
    { {AT_HIT, AF_PLAIN, 10}, {AT_SHIELD, AF_PLAIN, 10}, AT_NO_ATK, AT_NO_ATK },
    5, 275,
    2, 12, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_MELEE, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DWARF}, TILE_CORPSE_DWARF
},

{
    MONS_DEEP_DWARF, 'g', LIGHTBLUE, "deep dwarf",
    M_WARM_BLOOD | M_SPEAKS | M_NO_REGEN | M_NO_POLY_TO | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_DWARF, MONS_DEEP_DWARF, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 10}, {AT_SHIELD, AF_PLAIN, 10}, AT_NO_ATK, AT_NO_ATK },
    5, 275,
    2, 12, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_MELEE, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DEEP_DWARF}, TILE_CORPSE_DEEP_DWARF
},

// carnivorous quadrupeds ('h')
{
    MONS_JACKAL, 'h', BROWN, "jackal",
    M_WARM_BLOOD | M_BLOOD_SCENT,
    MR_VUL_SLASHING,
    10, MONS_HOUND, MONS_JACKAL, MH_NATURAL, 0,
    { {AT_BITE, AF_PLAIN, 3}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 55,
    2, 6, MST_NO_SPELLS, CE_CLEAN, S_BARK,
    I_ANIMAL, HT_LAND, 14, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_JACKAL}, TILE_CORPSE_JACKAL
},

{
    MONS_HOUND, 'h', YELLOW, "hound",
    M_SEE_INVIS | M_UNBLINDABLE | M_WARM_BLOOD | M_BLOOD_SCENT,
    MR_VUL_SLASHING,
    10, MONS_HOUND, MONS_HOUND, MH_NATURAL, 10,
    { {AT_BITE, AF_PLAIN, 6}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    3, 165,
    2, 9, MST_NO_SPELLS, CE_CLEAN, S_BARK,
    I_ANIMAL, HT_LAND, 15, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_HOUND}, TILE_CORPSE_HOUND
},

{
    MONS_HOWLER_MONKEY, 'h', LIGHTGREEN, "howler monkey",
    M_WARM_BLOOD,
    MR_VUL_SLASHING,
    15, MONS_HOWLER_MONKEY, MONS_HOWLER_MONKEY, MH_NATURAL, 10,
    { {AT_HIT, AF_PLAIN, 8}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    3, 185,
    1, 11, MST_HOWLER_MONKEY, CE_CLEAN, S_HOWL,
    I_ANIMAL, HT_LAND, 12, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND), SIZE_SMALL, MON_SHAPE_HUMANOID_TAILED,
    {TILEP_MONS_HOWLER_MONKEY}, TILE_CORPSE_HOWLER_MONKEY
},

{
    MONS_WARG, 'h', WHITE, "warg",
    M_SEE_INVIS | M_UNBLINDABLE | M_WARM_BLOOD | M_BLOOD_SCENT,
    MR_RES_POISON | MR_VUL_SLASHING | MR_RES_PIERCING,
    12, MONS_HOUND, MONS_WARG, MH_NATURAL, 40,
    { {AT_BITE, AF_PLAIN, 18}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    5, 325,
    4, 14, MST_NO_SPELLS, CE_CLEAN, S_HOWL,
    I_ANIMAL, HT_LAND, 13, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_WARG}, TILE_CORPSE_WARG
},

{
    MONS_WAR_DOG, 'h', DARKGREY, "war dog",
    M_SEE_INVIS | M_UNBLINDABLE | M_WARM_BLOOD | M_BLOOD_SCENT 
         | M_SPINY | M_NO_POLY_TO,
    MR_RES_POISON | MR_VUL_SLASHING | MR_RES_PIERCING,
    12, MONS_HOUND, MONS_WAR_DOG, MH_NATURAL, 40,
    { { AT_BITE, AF_PLAIN, 12 }, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    6, 260,
    6, 18, MST_NO_SPELLS, CE_CLEAN, S_HOWL,
    I_ANIMAL, HT_LAND, 13, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_QUADRUPED,
    { TILEP_MONS_WAR_DOG }, TILE_CORPSE_WAR_DOG
},

{
    MONS_BALLOON_DOG, 'h', ETC_JEWEL, "balloon dog",
    M_SEE_INVIS | M_UNBLINDABLE | M_BLOOD_SCENT | M_FLIES,
    MR_RES_COLD | MR_VUL_FIRE | MR_VUL_PIERCING | MR_RES_BLUDGEONING,
    12, MONS_HOUND, MONS_BALLOON_DOG, MH_CONSTRUCT, 40,
    { {AT_BITE, AF_CHAOTIC, 40}, {AT_CLAW, AF_BLINK, 12}, 
    AT_NO_ATK, AT_NO_ATK },
    7, 295,
    2, 31, MST_BLINKER, CE_NOCORPSE, S_HOWL,
    I_ANIMAL, HT_LAND, 26, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LITTLE, MON_SHAPE_QUADRUPED,
    { TILEP_MONS_BALLOON_DOG, TVARY_RANDOM }, TILE_ERROR
},

{
    MONS_WOLF, 'h', LIGHTGREY, "wolf",
    M_SEE_INVIS | M_UNBLINDABLE | M_WARM_BLOOD | M_BLOOD_SCENT,
    MR_VUL_SLASHING,
    12, MONS_HOUND, MONS_WOLF, MH_NATURAL, 20,
    { {AT_BITE, AF_PLAIN, 12}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    4, 220,
    4, 17, MST_NO_SPELLS, CE_CLEAN, S_HOWL,
    I_ANIMAL, HT_LAND, 17, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_WOLF}, TILE_CORPSE_WOLF
},

{
    MONS_HOG, 'h', LIGHTMAGENTA, "hog",
    M_WARM_BLOOD,
    MR_VUL_SLASHING,
    10, MONS_HOG, MONS_HOG, MH_NATURAL, 20,
    { {AT_BITE, AF_PLAIN, 14}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    6, 330,
    2, 7, MST_NO_SPELLS, CE_CLEAN, S_SQUEAL,
    I_ANIMAL, HT_LAND, 13, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_HOG}, TILE_CORPSE_HOG
},

{
    MONS_HELL_HOUND, 'h', CYAN, "hell hound",
    M_SEE_INVIS | M_UNBLINDABLE | M_BLOOD_SCENT,
    MR_RES_POISON | mrd(MR_RES_FIRE, 3) | MR_VUL_COLD | MR_VUL_SLASHING,
    10, MONS_HOUND, MONS_HELL_HOUND, MH_DEMONIC, 20,
    { {AT_BITE, AF_PLAIN, 13}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    5, 275,
    6, 21, MST_HELL_HOUND, CE_NOXIOUS, S_BARK,
    I_ANIMAL, HT_LAND, 15, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_HELL_HOUND}, TILE_CORPSE_HELL_HOUND
},

{
    MONS_DOOM_HOUND, 'h', GREEN, "doom hound",
    M_SEE_INVIS | M_UNBLINDABLE | M_BLOOD_SCENT,
    mrd(MR_RES_NEG, 3) | MR_VUL_SLASHING,
    10, MONS_HOUND, MONS_DOOM_HOUND, MH_DEMONIC, 20,
    { {AT_BITE, AF_PLAIN, 45}, {AT_CLAW, AF_PLAIN, 30}, AT_NO_ATK, AT_NO_ATK },
    20, 1100,
    6, 26, MST_DOOM_HOUND, CE_CLEAN, S_BARK,
    I_ANIMAL, HT_LAND, 13, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_DOOM_HOUND}, TILE_CORPSE_DOOM_HOUND
},

{
    MONS_RAIJU, 'h', LIGHTCYAN, "raiju",
    M_SEE_INVIS | M_UNBLINDABLE | M_BLOOD_SCENT,
    mrd(MR_RES_ELEC, 3) | MR_VUL_SLASHING,
    12, MONS_HOUND, MONS_RAIJU, MH_NATURAL, 20,
    { {AT_BITE, AF_ELEC, 11}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    7, 385,
    4, 29, MST_RAIJU, CE_CLEAN, S_BARK,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_RAIJU}, TILE_CORPSE_RAIJU
},

{
    MONS_HELL_HOG, 'h', LIGHTRED, "hell hog",
    M_NO_FLAGS,
    mrd(MR_RES_FIRE, 2) | MR_VUL_SLASHING,
    10, MONS_HOG, MONS_HELL_HOG, MH_DEMONIC, 40,
    { {AT_BITE, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    11, 595,
    2, 9, MST_HELL_HOG, CE_NOXIOUS, S_SQUEAL,
    I_ANIMAL, HT_LAND, 14, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_HELL_HOG}, TILE_CORPSE_HELL_HOG
},

{   // effect of porkalator cast on holies
    MONS_HOLY_SWINE, 'h', YELLOW, "holy swine",
    M_FLIES,
    MR_VUL_SLASHING,
    10, MONS_HOG, MONS_HOLY_SWINE, MH_HOLY, 40,
    { {AT_BITE, AF_HOLY, 20}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    11, 605,
    2, 9, MST_NO_SPELLS, CE_DIVINE, S_SQUEAL,
    I_ANIMAL, HT_LAND, 14, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_HOLY_SWINE,}, TILE_CORPSE_HOLY_SWINE
},

{ // a dummy monster for recolouring
    MONS_FELID, 'h', MAGENTA, "felid",
    M_SEE_INVIS | M_WARM_BLOOD | M_SPEAKS | M_NO_POLY_TO | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_FELID, MONS_FELID, MH_NATURAL, 40,
    { {AT_CLAW, AF_PLAIN, 10}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    5, 175,
    2, 18, MST_NO_SPELLS, CE_CLEAN, S_HISS,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(8),
    muf(MU_DOOR | MU_EVOKE), SIZE_LITTLE, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_FELID}, TILE_CORPSE_FELID
},

DUMMY(MONS_BEAR, 'h', LIGHTGREY, "bear", TILEP_MONS_BLACK_BEAR)

{
    MONS_POLAR_BEAR, 'h', LIGHTBLUE, "polar bear",
    M_WARM_BLOOD,
    MR_RES_COLD | MR_VUL_PIERCING | MR_RES_SLASHING,
    12, MONS_BEAR, MONS_POLAR_BEAR, MH_NATURAL, 20,
    { {AT_BITE, AF_PLAIN, 20}, {AT_CLAW, AF_PLAIN, 5}, {AT_CLAW, AF_PLAIN, 5},
       AT_NO_ATK },
    7, 455,
    7, 11, MST_BEAR, CE_CLEAN, S_GROWL,
    I_ANIMAL, HT_AMPHIBIOUS, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_QUADRUPED_TAILLESS,
    {TILEP_MONS_POLAR_BEAR}, TILE_CORPSE_POLAR_BEAR
},

{
    MONS_BLACK_BEAR, 'h', BLUE, "black bear",
    M_WARM_BLOOD,
    MR_VUL_PIERCING,
    9, MONS_BEAR, MONS_BLACK_BEAR, MH_NATURAL, 20,
    { {AT_BITE, AF_PLAIN, 9}, {AT_CLAW, AF_PLAIN, 5}, {AT_CLAW, AF_PLAIN, 5},
       AT_NO_ATK },
    6, 270,
    2, 8, MST_BEAR, CE_CLEAN, S_GROWL,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_QUADRUPED_TAILLESS,
    {TILEP_MONS_BLACK_BEAR}, TILE_CORPSE_BLACK_BEAR
},

// spriggans ('i')
{
    MONS_SPRIGGAN, 'i', LIGHTGREY, "spriggan",
    M_WARM_BLOOD | M_SPEAKS | M_SEE_INVIS | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_SPRIGGAN, MONS_SPRIGGAN, MH_NATURAL, 60,
    { {AT_HIT, AF_PLAIN, 15}, {AT_SHIELD, AF_PLAIN, 15}, AT_NO_ATK, AT_NO_ATK },
    7, 245,
    1, 18, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(6),
    muf(MU_MELEE | MU_EVOKE), SIZE_LITTLE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_SPRIGGAN}, TILE_CORPSE_SPRIGGAN
},

{   // both the guy and his ride as one monster
    MONS_SPRIGGAN_RIDER, 'i', LIGHTBLUE, "spriggan rider",
    M_WARM_BLOOD | M_SPEAKS | M_SEE_INVIS | M_FIGHTER | M_FLIES | M_GENDERED,
    MR_VUL_POISON | MR_RES_PIERCING | MR_VUL_BLUDGEONING, // the mount
    10, MONS_SPRIGGAN, MONS_SPRIGGAN, MH_NATURAL, 100,
    { {AT_HIT, AF_PLAIN, 27}, {AT_SHIELD, AF_PLAIN, 27}, AT_NO_ATK, AT_NO_ATK },
    11, 605,
    1, 22, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(6),
    muf(MU_MELEE | MU_EVOKE | MU_THROW_BLOWGUN), SIZE_SMALL, MON_SHAPE_HUMANOID, // ??
    {TILEP_MONS_SPRIGGAN_RIDER}, TILE_ERROR
},

{
    MONS_SPRIGGAN_DRUID, 'i', GREEN, "spriggan druid",
    M_WARM_BLOOD | M_SPEAKS | M_SEE_INVIS | M_SPELLCASTER | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_SPRIGGAN, MONS_SPRIGGAN, MH_NATURAL, 100,
    { {AT_HIT, AF_PLAIN, 18}, {AT_SHIELD, AF_PLAIN, 18}, AT_NO_ATK, AT_NO_ATK },
    11, 440,
    1, 24, MST_SPRIGGAN_DRUID, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(6),
    muf(MU_MELEE | MU_EVOKE), SIZE_LITTLE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_SPRIGGAN_DRUID}, TILE_ERROR
},

{
    MONS_SPRIGGAN_BERSERKER, 'i', LIGHTRED, "spriggan berserker",
    M_WARM_BLOOD | M_SPEAKS | M_SEE_INVIS | M_FIGHTER | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_SPRIGGAN, MONS_SPRIGGAN, MH_NATURAL, 120,
    { {AT_HIT, AF_PLAIN, 27}, {AT_SHIELD, AF_PLAIN, 27}, AT_NO_ATK, AT_NO_ATK },
    12, 540,
    2, 28, MST_SPRIGGAN_BERSERKER, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(6),
    muf(MU_MELEE | MU_EVOKE), SIZE_LITTLE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_SPRIGGAN_BERSERKER}, TILE_ERROR
},

{
    MONS_SPRIGGAN_AIR_MAGE, 'i', LIGHTCYAN, "spriggan air mage",
    M_WARM_BLOOD | M_SPEAKS | M_SEE_INVIS | M_FLIES | M_SPELLCASTER | M_GENDERED,
    MR_RES_WIND | MR_VUL_SLASHING,
    10, MONS_SPRIGGAN, MONS_SPRIGGAN, MH_NATURAL, 140,
    { {AT_HIT, AF_PLAIN, 16}, {AT_SHIELD, AF_PLAIN, 16}, AT_NO_ATK, AT_NO_ATK },
    14, 490,
    1, 37, MST_SPRIGGAN_AIR_MAGE, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(6),
    muf(MU_MELEE | MU_EVOKE), SIZE_LITTLE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_SPRIGGAN_AIR_MAGE}, TILE_ERROR
},

{
    MONS_SPRIGGAN_DEFENDER, 'i', YELLOW, "spriggan defender",
    M_WARM_BLOOD | M_SPEAKS | M_SEE_INVIS | M_FIGHTER | M_GENDERED,
    MR_VUL_PIERCING | MR_RES_SLASHING,
    10, MONS_SPRIGGAN, MONS_SPRIGGAN, MH_NATURAL, 140,
    { {AT_HIT, AF_PLAIN, 30}, {AT_SHIELD, AF_PLAIN, 30}, AT_NO_ATK, AT_NO_ATK },
    15, 675,
    3, 31, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(6),
    muf(MU_MELEE | MU_EVOKE), SIZE_LITTLE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_SPRIGGAN_DEFENDER}, TILE_ERROR
},

// drakes ('k')
DUMMY(MONS_DRAKE, 'k', LIGHTGREY, "drake", TILEP_MONS_SWAMP_DRAKE)

{
    MONS_SWAMP_DRAKE, 'k', BROWN, "swamp drake",
    M_WARM_BLOOD | M_FLIES,
    MR_RES_POISON | MR_RES_SLASHING | MR_VUL_PIERCING,
    20, MONS_DRAKE, MONS_SWAMP_DRAKE, MH_NATURAL, 20,
    { {AT_BITE, AF_PLAIN, 14}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    4, 300,
    3, 16, MST_SWAMP_DRAKE, CE_NOXIOUS, S_ROAR,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_QUADRUPED_WINGED,
    {TILEP_MONS_SWAMP_DRAKE}, TILE_CORPSE_SWAMP_DRAKE
},

{
    MONS_RIME_DRAKE, 'k', BLUE, "rime drake",
    M_WARM_BLOOD | M_FLIES,
    MR_RES_COLD | MR_RES_SLASHING | MR_VUL_PIERCING,
    22, MONS_DRAKE, MONS_RIME_DRAKE, MH_NATURAL, 20,
    { {AT_BITE, AF_PLAIN, 8}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    6, 330,
    3, 15, MST_RIME_DRAKE, CE_CLEAN, S_HISS,
    I_ANIMAL, HT_LAND, 12, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_QUADRUPED_WINGED,
    {TILEP_MONS_RIME_DRAKE}, TILE_CORPSE_RIME_DRAKE
},

{
    MONS_WIND_DRAKE, 'k', WHITE, "wind drake",
    M_WARM_BLOOD | M_FLIES,
    MR_RES_WIND | MR_RES_SLASHING | MR_VUL_PIERCING,
    6, MONS_DRAKE, MONS_WIND_DRAKE, MH_NATURAL, 40,
    { {AT_BITE, AF_PLAIN, 12}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    8, 600,
    3, 22, MST_WIND_DRAKE, CE_CLEAN, S_HISS,
    I_ANIMAL, HT_LAND, 12, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_QUADRUPED_WINGED,
    {TILEP_MONS_WIND_DRAKE}, TILE_CORPSE_WIND_DRAKE
},

{
    MONS_LINDWURM, 'k', LIGHTRED, "lindwurm",
    M_WARM_BLOOD,
    MR_RES_SLASHING,
    13, MONS_DRAKE, MONS_LINDWURM, MH_NATURAL, 40,
    { {AT_BITE, AF_PLAIN, 20}, {AT_CLAW, AF_PLAIN, 10},
      {AT_CLAW, AF_PLAIN, 10}, AT_NO_ATK },
    9, 495,
    8, 6, MST_LINDWURM, CE_CLEAN, S_ROAR,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_WAND, SIZE_LARGE, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_LINDWURM}, TILE_CORPSE_LINDWURM
},

{
    MONS_DEATH_DRAKE, 'k', LIGHTGREY, "death drake",
    M_COLD_BLOOD | M_FLIES,
    MR_RES_POISON | MR_RES_ROTTING | MR_RES_SLASHING,
    10, MONS_DRAKE, MONS_DEATH_DRAKE, MH_NATURAL, 40,
    { {AT_BITE, AF_PLAIN, 12}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    9, 765,
    6, 21, MST_DEATH_DRAKE, CE_NOXIOUS, S_HISS,
    I_ANIMAL, HT_LAND, 13, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_QUADRUPED_WINGED,
    {TILEP_MONS_DEATH_DRAKE}, TILE_CORPSE_DEATH_DRAKE
},

// lizards ('l')
DUMMY(MONS_GIANT_LIZARD, 'l', LIGHTGREY, "giant lizard", TILEP_MONS_LEOPARD_GECKO)

{
    MONS_CINDER_NEWT, 'l', RED, "cinder newt",
    M_COLD_BLOOD,
    MR_RES_FIRE | MR_RES_SLASHING | MR_VUL_BLUDGEONING,
    10, MONS_GIANT_LIZARD, MONS_CINDER_NEWT, MH_NATURAL, 0,
    { {AT_BITE, AF_PURE_FIRE, 2}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    2, 115,
    0, 11, MST_NO_SPELLS, CE_CLEAN, S_HISS,
    I_ANIMAL, HT_AMPHIBIOUS, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_CINDER_NEWT}, TILE_CORPSE_FRILLED_LIZARD
},

{
    MONS_LEOPARD_GECKO, 'l', YELLOW, "leopard gecko",
    M_COLD_BLOOD,
    MR_RES_SLASHING | MR_VUL_BLUDGEONING,
    16, MONS_GIANT_LIZARD, MONS_LEOPARD_GECKO, MH_NATURAL, 0,
    { {AT_BITE, AF_PLAIN, 5}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 55,
    1, 8, MST_NO_SPELLS, CE_CLEAN, S_HISS,
    I_ANIMAL, HT_LAND, 12, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LITTLE, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_LEOPARD_GECKO}, TILE_CORPSE_LEOPARD_GECKO
},

{
    MONS_IGUANA, 'l', BLUE, "iguana",
    M_COLD_BLOOD,
    MR_RES_SLASHING | MR_VUL_BLUDGEONING,
    13, MONS_GIANT_LIZARD, MONS_IGUANA, MH_NATURAL, 10,
    { {AT_BITE, AF_PLAIN, 15}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    3, 165,
    5, 7, MST_NO_SPELLS, CE_CLEAN, S_HISS,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_IGUANA}, TILE_CORPSE_IGUANA
},

{
    MONS_BASILISK, 'l', MAGENTA, "basilisk",
    M_COLD_BLOOD,
    MR_RES_SLASHING | MR_VUL_BLUDGEONING,
    10, MONS_GIANT_LIZARD, MONS_BASILISK, MH_NATURAL, 20,
    { {AT_BITE, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    6, 360,
    3, 16, MST_BASILISK, CE_NOXIOUS, S_HISS,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LITTLE, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_BASILISK}, TILE_CORPSE_BASILISK
},

{
    MONS_WYVERN, 'l', LIGHTGREEN, "wyvern",
    M_WARM_BLOOD | M_FLIES,
    MR_RES_SLASHING | MR_VUL_PIERCING,
    15, MONS_WYVERN, MONS_WYVERN, MH_NATURAL, 20,
    { {AT_BITE, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    5, 275,
    5, 22, MST_NO_SPELLS, CE_CLEAN, S_HISS,
    I_ANIMAL, HT_LAND, 15, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_QUADRUPED_WINGED,
    {TILEP_MONS_WYVERN}, TILE_CORPSE_WYVERN
},

{
    MONS_KOMODO_DRAGON, 'l', LIGHTRED, "komodo dragon",
    M_COLD_BLOOD,
    MR_RES_SLASHING | MR_VUL_PIERCING,
    10, MONS_GIANT_LIZARD, MONS_KOMODO_DRAGON, MH_NATURAL, 40,
    { {AT_BITE, AF_PLAIN, 34}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    8, 440,
    7, 7, MST_NO_SPELLS, CE_CLEAN, S_HISS,
    I_ANIMAL, HT_AMPHIBIOUS, 10, SWIM_ENERGY(6),
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_KOMODO_DRAGON}, TILE_CORPSE_KOMODO_DRAGON
},

// merfolk ('m')
{
    MONS_MERFOLK, 'm', LIGHTRED, "merfolk",
    M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_NO_FLAGS,
    4, MONS_MERFOLK, MONS_MERFOLK, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 22}, {AT_SHIELD, AF_PLAIN, 22}, AT_NO_ATK, AT_NO_ATK },
    10, 550,
    4, 12, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_AMPHIBIOUS, 10, SWIM_ENERGY(6),
    muf(MU_MELEE | MU_THROW_NET | MU_THROW_JAVELIN), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_MERFOLK, TVARY_WATER}, TILE_CORPSE_MERFOLK
},

{
    MONS_MERFOLK_IMPALER, 'm', YELLOW, "merfolk impaler",
    M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_NO_FLAGS,
    13, MONS_MERFOLK, MONS_MERFOLK, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 25}, {AT_SHIELD, AF_PLAIN, 25}, AT_NO_ATK, AT_NO_ATK },
    12, 780,
    // Impalers prefer light armour, and are dodging experts.
    0, 27, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_AMPHIBIOUS, 10, {10, 6, 6, 10, 10, 10, 10, 100},
    muf(MU_MELEE | MU_EVOKE | MU_THROW_NET), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_MERFOLK_IMPALER, TVARY_WATER}, TILE_ERROR
},

{
    MONS_MERFOLK_JAVELINEER, 'm', WHITE, "merfolk javelineer",
    M_WARM_BLOOD | M_ARCHER | M_PREFER_RANGED | M_SPEAKS | M_GENDERED,
    MR_NO_FLAGS,
    12, MONS_MERFOLK, MONS_MERFOLK, MH_NATURAL, 60,
    { {AT_HIT, AF_PLAIN, 17}, {AT_SHIELD, AF_PLAIN, 17}, AT_NO_ATK, AT_NO_ATK },
    13, 780,
    0, 31, MST_MERFOLK_JAVELINEER, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_AMPHIBIOUS, 10, SWIM_ENERGY(6),
    muf(MU_MELEE | MU_EVOKE | MU_THROW_JAVELIN), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_MERFOLK_JAVELINEER, TVARY_WATER}, TILE_ERROR
},

{
    MONS_MERFOLK_AQUAMANCER, 'm', GREEN, "merfolk aquamancer",
    M_WARM_BLOOD | M_SPEAKS | M_SEE_INVIS | M_SPELLCASTER | M_GENDERED,
    MR_NO_FLAGS,
    8, MONS_MERFOLK, MONS_MERFOLK, MH_NATURAL, 80,
    { {AT_HIT, AF_PLAIN, 15}, {AT_SHIELD, AF_PLAIN, 15}, AT_NO_ATK, AT_NO_ATK },
    14, 630,
    0, 22, MST_MERFOLK_AQUAMANCER, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_AMPHIBIOUS, 10, SWIM_ENERGY(6),
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_MERFOLK_AQUAMANCER, TVARY_WATER}, TILE_ERROR
},

{
    MONS_MERFOLK_SIREN, 'm', CYAN, "merfolk siren",
    M_WARM_BLOOD | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    MR_NO_FLAGS,
    10, MONS_MERFOLK, MONS_MERFOLK, MH_NATURAL, 60,
    { {AT_HIT, AF_PLAIN, 19}, {AT_SHIELD, AF_PLAIN, 19}, AT_NO_ATK, AT_NO_ATK },
    9, 405,
    4, 21, MST_MERFOLK_SIREN, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_AMPHIBIOUS, 10, SWIM_ENERGY(6),
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_MERFOLK_SIREN, TVARY_WATER}, TILE_ERROR
},

{
    MONS_MERFOLK_AVATAR, 'm', LIGHTCYAN, "merfolk avatar",
    M_WARM_BLOOD | M_SPEAKS | M_NO_GEN_DERIVED | M_SPELLCASTER | M_GENDERED,
    MR_NO_FLAGS,
    10, MONS_MERFOLK, MONS_MERFOLK, MH_NATURAL, 120,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    13, 845,
    4, 21, MST_MERFOLK_AVATAR, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_AMPHIBIOUS, 10, SWIM_ENERGY(6),
    muf(MU_DOOR | MU_ARMOUR | MU_SHIELD), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_MERFOLK_AVATAR, TVARY_WATER}, TILE_ERROR
},

{
    MONS_WATER_NYMPH, 'm', MAGENTA, "water nymph",
    M_WARM_BLOOD | M_SPEAKS | M_SEE_INVIS | M_GENDERED | M_ABSORB_WATER,
    MR_VUL_SLASHING | MR_RES_PIERCING,
    10, MONS_WATER_NYMPH, MONS_WATER_NYMPH, MH_NATURAL, 100,
    { {AT_TOUCH, AF_DROWN, 12}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    10, 400,
    2, 22, MST_WATER_NYMPH, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_AMPHIBIOUS, 10, DEFAULT_ENERGY,
    MU_DOOR, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_WATER_NYMPH}, TILE_CORPSE_WATER_NYMPH
},

// rotting monsters ('n')
{
    MONS_BOG_BODY, 'n', GREEN, "bog body",
    M_NO_ZOMBIE,
    MR_RES_FIRE | MR_RES_COLD | MR_RES_PIERCING,
    16, MONS_GHOUL, MONS_BOG_BODY, MH_UNDEAD, 40,
    { {AT_HIT, AF_PLAIN, 25}, {AT_TOUCH, AF_COLD, 4}, AT_NO_ATK, AT_NO_ATK },
    6, 390,
    1, 7, MST_BOG_BODY, CE_NOXIOUS, S_SILENT,
    I_HUMAN, HT_AMPHIBIOUS, 10, SWIM_ENERGY(14),
    muf(MU_DOOR | MU_ARMOUR), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_BOG_BODY}, TILE_CORPSE_BOG_BODY
},

{
    MONS_NECROPHAGE, 'n', LIGHTGREY, "necrophage",
    M_NO_ZOMBIE,
    MR_RES_COLD | MR_RES_PIERCING,
    12, MONS_GHOUL, MONS_NECROPHAGE, MH_UNDEAD, 40,
    { {AT_HIT, AF_ROT, 8}, {AT_SHIELD, AF_PLAIN, 8}, AT_NO_ATK, AT_NO_ATK },
    5, 275,
    2, 10, MST_NO_SPELLS, CE_NOXIOUS, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_ARMOUR | MU_SHIELD | MU_WAND), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_NECROPHAGE}, TILE_CORPSE_NECROPHAGE
},

{
    MONS_GHOUL, 'n', RED, "ghoul",
    M_NO_ZOMBIE | M_GENDERED,
    MR_RES_COLD | MR_RES_PIERCING,
    10, MONS_GHOUL, MONS_GHOUL, MH_UNDEAD, 80,
    { {AT_CLAW, AF_PLAIN, 30}, {AT_CLAW, AF_PLAIN, 30}, {AT_SHIELD, AF_PLAIN, 30}, AT_NO_ATK },
    14, 1470,
    4, 18, MST_NO_SPELLS, CE_NOXIOUS, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_ARMOUR | MU_SHIELD | MU_WAND), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_GHOUL}, TILE_CORPSE_GHOUL
},

// orcs ('o')
{
    MONS_ORC, 'o', LIGHTRED, "orc",
    M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_VUL_SLASHING,
    15, MONS_ORC, MONS_ORC, MH_NATURAL, 0,
    { {AT_HIT, AF_PLAIN, 5}, {AT_SHIELD, AF_PLAIN, 5}, AT_NO_ATK, AT_NO_ATK },
    1, 70,
    0, 5, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_WEAPON_RANGED | MU_CONSUMABLES), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ORC}, TILE_CORPSE_ORC
},

{
    MONS_ORC_WIZARD, 'o', MAGENTA, "orc wizard",
    M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_ORC, MONS_ORC, MH_NATURAL, 20,
    { {AT_HIT, AF_PLAIN, 5}, {AT_SHIELD, AF_PLAIN, 5}, AT_NO_ATK, AT_NO_ATK },
    3, 150,
    1, 7, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ORC_WIZARD}, TILE_ERROR
},

{
    MONS_ORC_PRIEST, 'o', GREEN, "orc priest",
    M_WARM_BLOOD | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_ORC, MONS_ORC, MH_NATURAL | MH_EVIL, 20,
    { {AT_HIT, AF_PLAIN, 6}, {AT_SHIELD, AF_PLAIN, 6}, AT_NO_ATK, AT_NO_ATK },
    3, 150,
    1, 6, MST_ORC_PRIEST, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ORC_PRIEST}, TILE_ERROR
},

{
    MONS_ORC_WARRIOR, 'o', YELLOW, "orc warrior",
    M_FIGHTER | M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_VUL_SLASHING,
    16, MONS_ORC, MONS_ORC, MH_NATURAL, 20,
    { {AT_HIT, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    4, 280,
    0, 14, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ORC_WARRIOR}, TILE_ERROR
},

{
    MONS_ORC_KNIGHT, 'o', CYAN, "orc knight",
    M_FIGHTER | M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_RES_SLASHING | MR_VUL_PIERCING,
    9, MONS_ORC, MONS_ORC, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 25}, {AT_SHIELD, AF_PLAIN, 25}, AT_NO_ATK, AT_NO_ATK },
    9, 675,
    2, 18, MST_ORC_KNIGHT, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ORC_KNIGHT}, TILE_ERROR
},

{
    MONS_ORC_WARLORD, 'o', LIGHTCYAN, "orc warlord",
    M_FIGHTER | M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_RES_SLASHING | MR_VUL_PIERCING,
    11, MONS_ORC, MONS_ORC, MH_NATURAL, 60,
    { {AT_HIT, AF_PLAIN, 32}, {AT_SHIELD, AF_PLAIN, 32}, AT_NO_ATK, AT_NO_ATK },
    15, 1125,
    3, 22, MST_BATTLECRY, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ORC_WARLORD}, TILE_ERROR
},

{
    MONS_ORC_SORCERER, 'o', LIGHTMAGENTA, "orc sorcerer",
    M_SEE_INVIS | M_SPEAKS | M_WARM_BLOOD | M_SPELLCASTER | M_GENDERED,
    MR_VUL_SLASHING,
    12, MONS_ORC, MONS_ORC, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 7}, {AT_SHIELD, AF_PLAIN, 7}, AT_NO_ATK, AT_NO_ATK },
    9, 315,
    5, 21, MST_ORC_SORCERER, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ORC_SORCERER}, TILE_ERROR
},

{
    MONS_ORC_HIGH_PRIEST, 'o', LIGHTGREEN, "orc high priest",
    M_SEE_INVIS | M_SPEAKS | M_WARM_BLOOD | M_SPELLCASTER | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_ORC, MONS_ORC, MH_NATURAL | MH_EVIL, 60,
    { {AT_HIT, AF_PLAIN, 7}, {AT_SHIELD, AF_PLAIN, 7}, AT_NO_ATK, AT_NO_ATK },
    11, 550,
    1, 21, MST_ORC_HIGH_PRIEST, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ORC_HIGH_PRIEST}, TILE_ERROR
},

{
    MONS_ORC_CLERIC, 'o', MAGENTA, "orcish cleric",
    M_SEE_INVIS | M_SPEAKS | M_WARM_BLOOD | M_SPELLCASTER | M_GENDERED,
    MR_RES_SLASHING | MR_VUL_PIERCING,
    10, MONS_ORC, MONS_ORC, MH_NATURAL | MH_EVIL, 60,
    { { AT_HIT, AF_PLAIN, 7 }, {AT_SHIELD, AF_PLAIN, 7}, AT_NO_ATK, AT_NO_ATK },
    14, 550,
    1, 27, MST_ORC_CLERIC, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    { TILEP_MONS_ORC_CLERIC }, TILE_ERROR
},

// humans ('p')
{
    MONS_HUMAN, 'p', LIGHTGREY, "human",
    M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 20,
    { {AT_HIT, AF_PLAIN, 10}, {AT_SHIELD, AF_PLAIN, 10}, AT_NO_ATK, AT_NO_ATK },
    6, 330,
    3, 8, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_HUMAN, TVARY_MOD}, TILE_CORPSE_HUMAN
},

{
    MONS_SLAVE, 'p', WHITE, "slave",
    M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 20,
    { {AT_HIT, AF_PLAIN, 10}, {AT_SHIELD, AF_PLAIN, 10}, AT_NO_ATK, AT_NO_ATK },
    5, 275,
    2, 10, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_START_ONLY | MU_MELEE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_SLAVE}, TILE_ERROR
},

{
    MONS_HELL_KNIGHT, 'p', RED, "hell knight",
    M_FIGHTER | M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    mrd(MR_RES_FIRE, 3) | MR_RES_SLASHING,
    10, MONS_HUMAN, MONS_HUMAN, MH_NATURAL | MH_EVIL, 40,
    { {AT_HIT, AF_PLAIN, 26}, {AT_SHIELD, AF_PLAIN, 26}, AT_NO_ATK, AT_NO_ATK },
    10, 650,
    0, 18, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE | MU_THROW_ROCK), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_HELL_KNIGHT}, TILE_ERROR
},

{
    MONS_DEATH_KNIGHT, 'p', GREEN, "death knight",
    M_WARM_BLOOD | M_FIGHTER | M_SPEAKS | M_GENDERED,
    MR_RES_SLASHING | MR_VUL_PIERCING,
    12, MONS_HUMAN, MONS_HUMAN, MH_NATURAL | MH_EVIL, 80,
    { {AT_HIT, AF_PLAIN, 28}, {AT_SHIELD, AF_PLAIN, 28}, AT_NO_ATK, AT_NO_ATK },
    10, 800,
    2, 23, MST_DEATH_KNIGHT, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DEATH_KNIGHT}, TILE_ERROR
},

{
    MONS_NECROMANCER, 'p', WHITE, "necromancer",
    M_WARM_BLOOD | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_HUMAN, MONS_HUMAN, MH_NATURAL | MH_EVIL, 60,
    { {AT_HIT, AF_PLAIN, 6}, {AT_SHIELD, AF_PLAIN, 6}, AT_NO_ATK, AT_NO_ATK },
    10, 400,
    0, 18, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_NECROMANCER}, TILE_ERROR
},

{
    MONS_WIZARD, 'p', MAGENTA, "wizard",
    M_SPEAKS | M_WARM_BLOOD | M_SPELLCASTER | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_HUMAN, MONS_HUMAN, MH_NATURAL | MH_EVIL, 60,
    { {AT_HIT, AF_PLAIN, 6}, {AT_SHIELD, AF_PLAIN, 6}, AT_NO_ATK, AT_NO_ATK },
    10, 400,
    0, 18, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_WIZARD}, TILE_ERROR
},

{
    MONS_VAULT_GUARD, 'p', CYAN, "vault guard",
    M_FIGHTER | M_WARM_BLOOD | M_SEE_INVIS | M_SPEAKS | M_GENDERED,
    MR_RES_SLASHING | MR_VUL_PIERCING,
    10, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 60,
    { {AT_HIT, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    13, 715,
    1, 18, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_VAULT_GUARD}, TILE_ERROR
},

{
    MONS_VAULT_SENTINEL, 'p', LIGHTBLUE, "vault sentinel",
    M_FIGHTER | M_WARM_BLOOD | M_SEE_INVIS | M_SPEAKS | M_GENDERED,
    MR_RES_SLASHING | MR_VUL_PIERCING,
    10, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 15}, {AT_SHIELD, AF_PLAIN, 15}, AT_NO_ATK, AT_NO_ATK },
    8, 520,
    1, 18, MST_VAULT_SENTINEL, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_VAULT_SENTINEL}, TILE_ERROR
},

{
    MONS_VAULT_WARDEN, 'p', LIGHTRED, "vault warden",
    M_FIGHTER | M_WARM_BLOOD | M_SEE_INVIS | M_SPEAKS | M_GENDERED,
    MR_RES_SLASHING | MR_VUL_PIERCING,
    12, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 60,
    { {AT_HIT, AF_PLAIN, 36}, {AT_SHIELD, AF_PLAIN, 36}, AT_NO_ATK, AT_NO_ATK },
    16, 880,
    1, 18, MST_VAULT_WARDEN, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_VAULT_WARDEN}, TILE_ERROR
},

{
    MONS_IRONBRAND_CONVOKER, 'p', YELLOW, "ironbrand convoker",
    M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_RES_SLASHING | MR_VUL_PIERCING,
    12, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    9, 495,
    0, 17, MST_IRONBRAND_CONVOKER, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_IRONBRAND_CONVOKER}, TILE_ERROR
},

{
    MONS_IMPERIAL_MYRMIDON, 'p', LIGHTCYAN, "imperial myrmidon",
    M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_RES_SLASHING | MR_VUL_PIERCING,
    12, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 60,
    { {AT_HIT, AF_VULN, 30}, {AT_SHIELD, AF_PLAIN, 30}, AT_NO_ATK, AT_NO_ATK },
    16, 750,
    1, 33, MST_IMPERIAL_MYRMIDON, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 12, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_IMPERIAL_MYRMIDON}, TILE_ERROR
},

{
    MONS_SERVANT_OF_WHISPERS, 'p', BLUE, "servant of whispers",
    M_SPEAKS | M_WARM_BLOOD | M_SPELLCASTER | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 60,
    { {AT_HIT, AF_PLAIN, 10}, {AT_SHIELD, AF_PLAIN, 10}, AT_NO_ATK, AT_NO_ATK },
    12, 800,
    1, 18, MST_SERVANT_OF_WHISPERS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_SERVANT_OF_WHISPERS}, TILE_ERROR
},

{
    MONS_RAGGED_HIEROPHANT, 'p', LIGHTMAGENTA, "ragged hierophant",
    M_SPEAKS | M_WARM_BLOOD | M_SEE_INVIS | M_GENDERED,
    MR_VUL_SLASHING,
    15, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 12}, {AT_SHIELD, AF_PLAIN, 12}, AT_NO_ATK, AT_NO_ATK },
    9, 1200,
    0, 8, MST_RAGGED_HIEROPHANT, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_RAGGED_HIEROPHANT}, TILE_ERROR
},

{
    MONS_IRONHEART_PRESERVER, 'p', LIGHTGREEN, "ironheart preserver",
    M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_RES_SLASHING | MR_VUL_PIERCING,
    12, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 25}, {AT_SHIELD, AF_PLAIN, 15}, AT_NO_ATK, AT_NO_ATK },
    14, 1120,
    0, 6, MST_IRONHEART_PRESERVER, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_IRONHEART_PRESERVER}, TILE_ERROR
},

{
    MONS_SHAPESHIFTER, 'p', LIGHTRED, "shapeshifter",
    M_NO_SKELETON | M_NO_ZOMBIE,
    MR_VUL_PHYSICAL,
    10, MONS_SHAPESHIFTER, MONS_SHAPESHIFTER, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 5}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    7, 385,
    0, 20, MST_NO_SPELLS, CE_NOXIOUS, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_DOOR, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_SHAPESHIFTER}, TILE_CORPSE_SHAPESHIFTER
},

{
    MONS_GLOWING_SHAPESHIFTER, 'p', LIGHTRED, "glowing shapeshifter",
    M_NO_SKELETON | M_NO_ZOMBIE,
    MR_VUL_PHYSICAL,
    10, MONS_SHAPESHIFTER, MONS_GLOWING_SHAPESHIFTER, MH_NATURAL, 80,
    { {AT_HIT, AF_PLAIN, 15}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    10, 550,
    0, 20, MST_NO_SPELLS, CE_NOXIOUS, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_DOOR, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_GLOWING_SHAPESHIFTER}, TILE_CORPSE_GLOWING_SHAPESHIFTER
},

{
    MONS_KILLER_KLOWN, 'p', ETC_RANDOM, "Killer Klown",
    M_SEE_INVIS | M_SPEAKS | M_WARM_BLOOD | M_NO_POLY_TO | M_NO_GEN_DERIVED | M_GENDERED,
    MR_VUL_SLASHING | MR_RES_BLUDGEONING,
    17, MONS_HUMAN, MONS_KILLER_KLOWN, MH_NATURAL, 160,
    { {AT_HIT, AF_PLAIN, 30}, {AT_SHIELD, AF_PLAIN, 30}, AT_NO_ATK, AT_NO_ATK },
    20, 1500,
    10, 35, MST_KILLER_KLOWN, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 13, DEFAULT_ENERGY,
    muf(MU_START_ONLY | MU_ALL), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_KILLER_KLOWN, TVARY_RANDOM}, TILE_CORPSE_KILLER_KLOWN
},

{ // dummy; spawns in a single vault.
    MONS_DEMIGOD, 'p', YELLOW, "demigod",
    M_WARM_BLOOD | M_SPEAKS | M_NO_POLY_TO | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_HUMAN, MONS_DEMIGOD, MH_NATURAL, 20,
    { {AT_HIT, AF_PLAIN, 10}, {AT_SHIELD, AF_PLAIN, 10}, AT_NO_ATK, AT_NO_ATK },
    5, 375,
    2, 12, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_MELEE, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DEMIGOD}, TILE_CORPSE_HUMAN
},

// BCADDO: Dummy for a player race mirror enemy of a player race that no longer exists, can delete?
{ // dummy... literally; single vault
    MONS_HALFLING, 'p', LIGHTGREY, "halfling",
    M_WARM_BLOOD | M_SPEAKS | M_NO_POLY_TO | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_HALFLING, MONS_HALFLING, MH_NATURAL, 10,
    { {AT_HIT, AF_PLAIN, 6}, {AT_SHIELD, AF_PLAIN, 6}, AT_NO_ATK, AT_NO_ATK },
    3, 105,
    2, 12, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_MELEE, SIZE_SMALL, MON_SHAPE_HUMANOID,
    {TILEP_MONS_HALFLING}, TILE_CORPSE_HALFLING
},

// rodents ('r')
{
    MONS_RAT, 'r', BROWN, "rat",
    M_WARM_BLOOD,
    MR_VUL_SLASHING | MR_VUL_BLUDGEONING,
    1, MONS_RAT, MONS_RAT, MH_NATURAL, 0,
    { {AT_BITE, AF_PLAIN, 3}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 25,
    1, 6, MST_NO_SPELLS, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_RAT}, TILE_CORPSE_RAT
},

{
    MONS_QUOKKA, 'r', WHITE, "quokka",
    M_WARM_BLOOD,
    MR_VUL_SLASHING | MR_VUL_BLUDGEONING,
    10, MONS_QUOKKA, MONS_QUOKKA, MH_NATURAL, 0,
    { {AT_BITE, AF_PLAIN, 5}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 55,
    2, 8, MST_NO_SPELLS, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_LAND, 12, DEFAULT_ENERGY,
    MU_WAND, SIZE_LITTLE, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_QUOKKA}, TILE_CORPSE_QUOKKA
},

{
    MONS_SEWER_RAT, 'r', LIGHTGREEN, "sewer rat",
    M_WARM_BLOOD,
    MR_VUL_SLASHING | MR_VUL_BLUDGEONING,
    13, MONS_RAT, MONS_SEWER_RAT, MH_NATURAL, 10,
    { {AT_BITE, AF_PLAIN, 6}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    2, 110,
    5, 11, MST_NO_SPELLS, CE_NOXIOUS, S_SILENT,
    I_ANIMAL, HT_AMPHIBIOUS, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_RIVER_RAT}, TILE_CORPSE_GREEN_RAT
},

{
    MONS_HELL_RAT, 'r', LIGHTRED, "hell rat",
    M_WARM_BLOOD,
    mrd(MR_RES_NEG, 1) | MR_VUL_SLASHING | MR_VUL_BLUDGEONING,
    10, MONS_RAT, MONS_HELL_RAT, MH_DEMONIC, 20,
    { {AT_BITE, AF_DRAIN_XP, 20}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    3, 165,
    7, 17, MST_NO_SPELLS, CE_NOXIOUS, S_LOUD_ROAR,
    I_ANIMAL, HT_LAND, 12, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LITTLE, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_ORANGE_RAT}, TILE_CORPSE_ORANGE_RAT
},

{
    MONS_PORCUPINE, 'r', BLUE, "porcupine",
    M_WARM_BLOOD | M_SPINY,
    MR_RES_SLASHING | MR_VUL_PIERCING,
    26, MONS_RAT, MONS_PORCUPINE, MH_NATURAL, 20,
    { {AT_BITE, AF_PLAIN, 16}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    4, 160,
    2, 14, MST_NO_SPELLS, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_LAND, 12, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_PORCUPINE}, TILE_CORPSE_PORCUPINE
},

// arachnids ('s')
{
    MONS_SCORPION, 's', BLUE, "scorpion",
    M_NO_SKELETON | M_COLD_BLOOD,
    MR_VUL_POISON | MR_RES_ACID | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    13, MONS_SCORPION, MONS_SCORPION, MH_NATURAL, 20,
    { {AT_PINCER, AF_PIERCE_AC, 4}, {AT_PINCER, AF_PIERCE_AC, 4},
      {AT_BITE, AF_PLAIN, 8},   {AT_STING, AF_POISON, 10} },
    6, 180,
    5, 7, MST_NO_SPELLS, CE_NOXIOUS, S_SILENT,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_ARACHNID,
    {TILEP_MONS_SCORPION}, TILE_CORPSE_SCORPION
},

{
    MONS_BONE_SCORPION, 's', MAGENTA, "bone scorpion",
    M_NO_FLAGS,
    MR_RES_COLD | MR_RES_ELEC | MR_VUL_ACID 
      | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    13, MONS_SCORPION, MONS_BONE_SCORPION, MH_UNDEAD, 40,
    { { AT_CLAW, AF_PIERCE_AC, 6 },{ AT_CLAW, AF_PIERCE_AC, 6 },
    { AT_STING, AF_DRAIN_XP, 15}, { AT_STING, AF_DRAIN_STR, 15} },
    6, 180,
    15, 4, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_ARACHNID,
    { TILEP_MONS_BONE_SCORPION }, TILE_ERROR
},

{
    MONS_CEILING_CRAWLER, 's', GREEN, "ceiling crawler",
    M_NO_SKELETON | M_COLD_BLOOD,
    MR_VUL_POISON | MR_RES_ACID | MR_RES_PIERCING,
    13, MONS_SCORPION, MONS_CEILING_CRAWLER, MH_NATURAL, 45,
    { { AT_BITE, AF_PLAIN, 28 }, { AT_HIT, AF_PLAIN, 6 },
    { AT_HIT, AF_PLAIN, 6 }, { AT_CONSTRICT, AF_CRUSH, 9 } },
    11, 810,
    14, 22, MST_NO_SPELLS, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_LAND, 12, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_GIANT, MON_SHAPE_ARACHNID,
    { TILEP_MONS_CEILING_CRAWLER }, TILE_CORPSE_CEILING_CRAWLER
},

{
    MONS_EMPEROR_SCORPION, 's', LIGHTGREY, "emperor scorpion",
    M_NO_SKELETON | M_COLD_BLOOD,
    MR_RES_POISON | MR_RES_ACID | MR_RES_PIERCING,
    13, MONS_SCORPION, MONS_EMPEROR_SCORPION, MH_NATURAL, 60,
    { {AT_PINCER, AF_PIERCE_AC, 10}, {AT_PINCER, AF_PIERCE_AC, 10},
      {AT_BITE, AF_PLAIN, 15},   {AT_STING, AF_POISON, 30} },
    14, 1190,
    20, 12, MST_NO_SPELLS, CE_NOXIOUS, S_SILENT,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_GIANT, MON_SHAPE_ARACHNID,
    {TILEP_MONS_EMPEROR_SCORPION}, TILE_CORPSE_EMPEROR_SCORPION
},

DUMMY(MONS_SPIDER, 's', CYAN, "spider", TILEP_MONS_REDBACK)

{
    MONS_TARANTELLA, 's', LIGHTMAGENTA, "tarantella",
    M_NO_SKELETON | M_WEB_SENSE | M_COLD_BLOOD,
    MR_VUL_POISON | MR_RES_ACID | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    6, MONS_SPIDER, MONS_TARANTELLA, MH_NATURAL, 20,
    { {AT_BITE, AF_CONFUSE, 19}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    8, 280,
    5, 25, MST_NO_SPELLS, CE_CLEAN, S_HISS,
    I_ANIMAL, HT_LAND, 15, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_ARACHNID,
    {TILEP_MONS_TARANTELLA}, TILE_CORPSE_TARANTELLA
},

{
    MONS_JUMPING_SPIDER, 's', LIGHTBLUE, "jumping spider",
    M_NO_SKELETON | M_WEB_SENSE | M_COLD_BLOOD,
    MR_VUL_POISON | MR_RES_ACID | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    8, MONS_SPIDER, MONS_JUMPING_SPIDER, MH_NATURAL, 20,
    { {AT_POUNCE, AF_ENSNARE, 20}, {AT_BITE, AF_PLAIN, 8}, AT_NO_ATK,
       AT_NO_ATK },
    8, 320,
    7, 31, MST_JUMPING_SPIDER, CE_NOXIOUS, S_HISS,
    I_ANIMAL, HT_LAND, 15, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_ARACHNID,
    {TILEP_MONS_JUMPING_SPIDER}, TILE_CORPSE_JUMPING_SPIDER
},

{
    MONS_WOLF_SPIDER, 's', WHITE, "wolf spider",
    M_NO_SKELETON | M_WEB_SENSE | M_COLD_BLOOD,
    MR_VUL_POISON | MR_RES_ACID | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    4, MONS_SPIDER, MONS_WOLF_SPIDER, MH_NATURAL, 20,
    { {AT_HIT, AF_PLAIN, 25}, {AT_BITE, AF_POISON, 15}, AT_NO_ATK,
       AT_NO_ATK },
    11, 550,
    12, 18, MST_NO_SPELLS, CE_NOXIOUS, S_HISS,
    I_ANIMAL, HT_LAND, 15, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_ARACHNID,
    {TILEP_MONS_WOLF_SPIDER}, TILE_CORPSE_WOLF_SPIDER
},

{
    MONS_REDBACK, 's', LIGHTRED, "redback",
    M_NO_SKELETON | M_WEB_SENSE | M_COLD_BLOOD,
    MR_VUL_POISON | MR_RES_ACID | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    5, MONS_SPIDER, MONS_REDBACK, MH_NATURAL, 20,
    { {AT_BITE, AF_POISON_STRONG, 18}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    9, 270,
    4, 17, MST_NO_SPELLS, CE_NOXIOUS, S_SILENT,
    I_ANIMAL, HT_LAND, 15, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_ARACHNID,
    {TILEP_MONS_REDBACK}, TILE_CORPSE_REDBACK
},

{
    MONS_DEMONIC_CRAWLER, 's', LIGHTGREEN, "demonic crawler",
    M_NO_SKELETON | M_SEE_INVIS,
    MR_RES_ELEC | MR_RES_POISON | MR_RES_COLD | MR_RES_FIRE | mrd(MR_RES_NEG, 1)
        | MR_RES_ACID | MR_RES_PIERCING | MR_VUL_SLASHING,
    4, MONS_DEMONIC_CRAWLER, MONS_DEMONIC_CRAWLER, MH_DEMONIC, 100,
    { {AT_HIT, AF_PLAIN, 13}, {AT_HIT, AF_PLAIN, 13}, {AT_HIT, AF_PLAIN, 13},
       AT_NO_ATK },
    9, 585,
    10, 6, MST_NO_SPELLS, CE_NOXIOUS, S_SCREAM,
    I_ANIMAL, HT_LAND, 13, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_BIG, MON_SHAPE_CENTIPEDE,
    {TILEP_MONS_DEMONIC_CRAWLER}, TILE_CORPSE_DEMONIC_CRAWLER
},

{
    MONS_ORB_SPIDER, 's', CYAN, "orb spider",
    M_NO_SKELETON | M_WEB_SENSE | M_MAINTAIN_RANGE | M_COLD_BLOOD,
    MR_VUL_POISON | MR_RES_COLD | MR_RES_ACID 
      | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    20, MONS_SPIDER, MONS_ORB_SPIDER, MH_NATURAL, 40,
    { {AT_BITE, AF_PLAIN, 5}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    7, 420,
    3, 19, MST_ORB_SPIDER, CE_CLEAN, S_HISS,
    I_ANIMAL, HT_LAND, 12, SPELL_ENERGY(20),
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_ARACHNID,
    {TILEP_MONS_ORB_SPIDER}, TILE_CORPSE_ORB_SPIDER
},

// testudines and crocodiles ('t')
{
    MONS_CROCODILE, 't', BROWN, "crocodile",
    M_COLD_BLOOD,
    MR_RES_SLASHING | MR_VUL_PIERCING,
    10, MONS_CROCODILE, MONS_CROCODILE, MH_NATURAL, 20,
    { {AT_BITE, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    5, 275,
    4, 6, MST_NO_SPELLS, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_AMPHIBIOUS, 10, SWIM_ENERGY(6),
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_CROCODILE}, TILE_CORPSE_CROCODILE
},

{
    MONS_ALLIGATOR, 't', LIGHTBLUE, "alligator",
    M_COLD_BLOOD,
    MR_RES_SLASHING | MR_VUL_PIERCING,
    10, MONS_CROCODILE, MONS_ALLIGATOR, MH_NATURAL, 40,
    { {AT_BITE, AF_PLAIN, 30}, {AT_TAIL_SLAP, AF_PLAIN, 15}, AT_NO_ATK,
       AT_NO_ATK },
    12, 720,
    5, 5, MST_ALLIGATOR, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_AMPHIBIOUS, 10, {10, 6, 8, 8, 8, 8, 8, 80},
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_ALLIGATOR}, TILE_CORPSE_ALLIGATOR
},

{
    MONS_SNAPPING_TURTLE, 't', GREEN, "snapping turtle",
    M_COLD_BLOOD,
    MR_RES_ACID | MR_RES_SLASHING | MR_VUL_PIERCING 
      | MR_RES_BLUDGEONING,
    10, MONS_SNAPPING_TURTLE, MONS_SNAPPING_TURTLE, MH_NATURAL, 40,
    { {AT_BITE, AF_REACH, 30}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    10, 600,
    16, 5, MST_NO_SPELLS, CE_CLEAN, S_HISS,
    I_ANIMAL, HT_AMPHIBIOUS, 8, {10, 6, 8, 8, 8, 8, 8, 80},
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_QUADRUPED_TAILLESS,
    {TILEP_MONS_SNAPPING_TURTLE}, TILE_CORPSE_SNAPPING_TURTLE
},

{
    MONS_ALLIGATOR_SNAPPING_TURTLE, 't', LIGHTGREEN, "alligator snapping turtle",
    M_COLD_BLOOD,
    MR_RES_ACID | MR_RES_SLASHING | MR_VUL_PIERCING
     | MR_RES_BLUDGEONING,
    10, MONS_SNAPPING_TURTLE, MONS_ALLIGATOR_SNAPPING_TURTLE,
        MH_NATURAL, 60,
    { {AT_BITE, AF_REACH, 50}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    16, 1440,
    19, 1, MST_NO_SPELLS, CE_CLEAN, S_HISS,
    I_ANIMAL, HT_AMPHIBIOUS, 8, SWIM_ENERGY(6),
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_QUADRUPED_TAILLESS,
    {TILEP_MONS_ALLIGATOR_SNAPPING_TURTLE}, TILE_CORPSE_ALLIGATOR_SNAPPING_TURTLE
},

DUMMY(MONS_CRAB, 't', LIGHTGREY, "crab", TILEP_MONS_FIRE_CRAB)

{
    MONS_FIRE_CRAB, 't', LIGHTRED, "fire crab",
    M_NO_SKELETON,
    MR_VUL_POISON | mrd(MR_RES_FIRE, 3) | MR_RES_ACID 
      | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    25, MONS_CRAB, MONS_FIRE_CRAB, MH_NATURAL, 40,
    { {AT_BITE, AF_FIRE, 15}, {AT_CLAW, AF_FIRE, 15}, AT_NO_ATK, AT_NO_ATK },
    8, 520,
    9, 6, MST_FIRE_CRAB, CE_NOXIOUS, S_SILENT,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_QUADRUPED, // ??
    {TILEP_MONS_FIRE_CRAB}, TILE_CORPSE_FIRE_CRAB
},

{
    MONS_GHOST_CRAB, 't', LIGHTGREY, "ghost crab",
    M_NO_SKELETON,
    MR_RES_ACID | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    25, MONS_CRAB, MONS_GHOST_CRAB, MH_UNDEAD, 40,
    { {AT_BITE, AF_DRAIN_XP, 20}, {AT_CLAW, AF_DRAIN_XP, 15}, AT_NO_ATK, AT_NO_ATK },
    9, 585,
    9, 6, MST_GHOST_CRAB, CE_NOXIOUS, S_SILENT,
    I_ANIMAL, HT_AMPHIBIOUS, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_GHOST_CRAB}, TILE_CORPSE_GHOST_CRAB
},

{
    MONS_APOCALYPSE_CRAB, 't', WHITE, "apocalypse crab",
    M_NO_SKELETON | M_SEE_INVIS | M_COLD_BLOOD,
    MR_VUL_POISON | mrd(MR_RES_FIRE | MR_RES_COLD, 2) | MR_RES_ACID
      | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    13, MONS_CRAB, MONS_APOCALYPSE_CRAB, MH_DEMONIC, 60,
    { {AT_BITE, AF_CHAOTIC, 15}, {AT_CLAW, AF_CHAOTIC, 15}, AT_NO_ATK, AT_NO_ATK },
    8, 520,
    11, 18, MST_APOCALYPSE_CRAB, CE_NOXIOUS, S_SILENT,
    I_ANIMAL, HT_AMPHIBIOUS, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_APOCALYPSE_CRAB}, TILE_CORPSE_APOCALYPSE_CRAB
},

{
    MONS_PLUTONIUM_CRAB, 't', LIGHTMAGENTA, "plutonium crab",
    M_NO_SKELETON | M_COLD_BLOOD,
    MR_VUL_POISON | MR_RES_FIRE | MR_RES_ACID | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    25, MONS_CRAB, MONS_PLUTONIUM_CRAB, MH_NATURAL, 40,
    { {AT_BITE, AF_CONTAM, 30}, {AT_CLAW, AF_FIRE, 15}, {AT_CLAW, AF_FIRE, 15}, AT_NO_ATK },
    12, 1220,
    12, 18, MST_PLUTONIUM_CRAB, CE_NOXIOUS, S_SILENT, // BCADDO: Bring back mutating meat.
    I_ANIMAL, HT_AMPHIBIOUS, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_PLUTONIUM_CRAB}, TILE_CORPSE_PLUTONIUM_CRAB
},

// ugly things ('u')
// NOTE: ugly things are ghost demons and thus their stats vary treacherously
// from those below. beware!
{
    MONS_UGLY_THING, 'u', COLOUR_UNDEF, "ugly thing",
    M_WARM_BLOOD | M_HERD | M_NO_GEN_DERIVED | M_GHOST_DEMON,
    MR_NO_FLAGS,
    6, MONS_UGLY_THING, MONS_UGLY_THING, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 17}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    12, 660,
    4, 22, MST_NO_SPELLS, CE_NOXIOUS, S_GURGLE,
    I_ANIMAL, HT_LAND, 10, MOVE_ENERGY(9),
    muf(MU_DOOR | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_MISC,
    {TILEP_MONS_PROGRAM_BUG}, TILE_CORPSE_UGLY_THING
},

{
    MONS_VERY_UGLY_THING, 'u', COLOUR_UNDEF, "very ugly thing",
    M_WARM_BLOOD | M_HERD | M_NO_GEN_DERIVED | M_GHOST_DEMON,
    MR_NO_FLAGS,
    10, MONS_UGLY_THING, MONS_VERY_UGLY_THING, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 27}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    18, 990,
    6, 28, MST_NO_SPELLS, CE_NOXIOUS, S_GURGLE,
    I_ANIMAL, HT_LAND, 10, MOVE_ENERGY(9),
    muf(MU_DOOR | MU_EVOKE), SIZE_LARGE, MON_SHAPE_MISC,
    {TILEP_MONS_PROGRAM_BUG}, TILE_CORPSE_VERY_UGLY_THING
},

// vortices ('v')
{
    MONS_FIRE_VORTEX, 'v', RED, "fire vortex",
    M_CONFUSED | M_INSUBSTANTIAL | M_CONJURED | M_FLIES | M_NO_EXP_GAIN,
    mrd(MR_RES_FIRE | MR_RES_ACID, 3) | MR_VUL_COLD | MR_RES_ELEC | MR_RES_PHYSICAL,
    5, MONS_FIRE_VORTEX, MONS_FIRE_VORTEX, MH_ELEMENTAL, MAG_IMMUNE,
    { {AT_HIT, AF_PURE_FIRE, 0}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    10, 200,
    0, 5, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 15, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_MISC,
    {TILEP_MONS_FIRE_VORTEX, TVARY_CYCLE}, TILE_ERROR
},

{
    MONS_SPATIAL_VORTEX, 'v', ETC_RANDOM, "spatial vortex",
    M_CONFUSED | M_INSUBSTANTIAL | M_CONJURED | M_FLIES,
    MR_RES_FIRE | MR_RES_COLD | MR_RES_ELEC | mrd(MR_RES_ACID, 3) | MR_RES_PHYSICAL,
    5, MONS_FIRE_VORTEX, MONS_SPATIAL_VORTEX, MH_ELEMENTAL, MAG_IMMUNE,
    { {AT_HIT, AF_DISTORT, 30}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    6, 360,
    0, 8, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 15, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_MISC,
    {TILEP_MONS_SPATIAL_VORTEX, TVARY_CYCLE}, TILE_ERROR
},

{
    MONS_CHAOS_VORTEX, 'v', ETC_JEWEL, "chaos vortex",
    M_CONFUSED | M_INSUBSTANTIAL | M_CONJURED | M_FLIES | M_NO_EXP_GAIN,
    MR_RES_FIRE | MR_RES_COLD | MR_RES_ELEC | mrd(MR_RES_ACID, 3) | MR_RES_PHYSICAL,
    5, MONS_FIRE_VORTEX, MONS_CHAOS_VORTEX, MH_ELEMENTAL, MAG_IMMUNE,
    { { AT_HIT, AF_PURE_CHAOS, 20 }, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    6, 360,
    0, 5, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 15, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_MISC,
    { TILEP_MONS_CHAOS_VORTEX, TVARY_CYCLE }, TILE_ERROR
},

{
    MONS_SPATIAL_MAELSTROM, 'v', YELLOW, "spatial maelstrom",
    M_BATTY | M_INSUBSTANTIAL | M_FLIES,
    mrd(MR_RES_FIRE | MR_RES_COLD, 2) | MR_RES_ELEC | mrd(MR_RES_ACID, 3)
    | MR_RES_PHYSICAL,
    5, MONS_SPATIAL_MAELSTROM, MONS_SPATIAL_MAELSTROM,
        MH_ELEMENTAL, MAG_IMMUNE,
    { {AT_HIT, AF_DISTORT, 20}, {AT_HIT, AF_DISTORT, 20}, AT_NO_ATK,
       AT_NO_ATK },
    10, 600,
    0, 11, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 16, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_BIG, MON_SHAPE_MISC,
    {TILEP_MONS_SPATIAL_MAELSTROM, TVARY_CYCLE}, TILE_ERROR
},

{
    MONS_INSUBSTANTIAL_WISP, 'v', LIGHTGREY, "insubstantial wisp",
    M_INSUBSTANTIAL | M_FLIES,
    mrd(MR_RES_ELEC | MR_RES_ACID, 3) | mrd(MR_RES_FIRE | MR_RES_COLD, 2) 
      | MR_RES_PHYSICAL,
    8, MONS_INSUBSTANTIAL_WISP, MONS_INSUBSTANTIAL_WISP,
        MH_ELEMENTAL, MAG_IMMUNE,
    { {AT_HIT, AF_BLINK, 12}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    8, 160,
    4, 34, MST_INSUBSTANTIAL_WISP, CE_NOCORPSE, S_MOAN,
    I_BRAINLESS, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_MISC,
    {TILEP_MONS_INSUBSTANTIAL_WISP}, TILE_ERROR
},

{ // miscast only
    MONS_TWISTER, 'v', ETC_AIR, "twister",
    M_CONFUSED | M_INSUBSTANTIAL | M_BATTY | M_NO_EXP_GAIN | M_NO_POLY_TO
        | M_FLIES,
    MR_RES_FIRE | MR_RES_COLD | mrd(MR_RES_ELEC | MR_RES_ACID, 3) | MR_RES_WIND | MR_RES_PHYSICAL,
    5, MONS_FIRE_VORTEX, MONS_TWISTER, MH_ELEMENTAL, MAG_IMMUNE,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    12, 120000,
    0, 5, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_MISC,
    {TILEP_MONS_TWISTER, TVARY_CYCLE}, TILE_ERROR
},

// worms and slugs ('w')
{
    MONS_WORM, 'w', LIGHTRED, "worm",
    M_NO_SKELETON | M_FAST_REGEN | M_COLD_BLOOD,
    MR_RES_ACID | MR_VUL_SLASHING | MR_VUL_BLUDGEONING | MR_RES_PIERCING,
    3, MONS_WORM, MONS_WORM, MH_NATURAL, 10,
    { {AT_BITE, AF_PLAIN, 12}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    4, 180,
    1, 3, MST_NO_SPELLS, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_LAND, 6, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_SNAKE,
    {TILEP_MONS_WORM}, TILE_CORPSE_WORM
},

{
    MONS_ROCK_WORM, 'w', BROWN, "rock worm",
    M_NO_SKELETON | M_COLD_BLOOD,
    MR_VUL_POISON | MR_RES_FIRE | MR_RES_COLD | MR_RES_ELEC 
     | MR_RES_PIERCING,
    12, MONS_WORM, MONS_ROCK_WORM, MH_NATURAL, 40,
    { { AT_BITE, AF_PLAIN, 22 }, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    8, 750, 
    3, 18, MST_NO_SPELLS, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_ROCK, 12, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_SNAKE,
    {TILEP_MONS_ROCK_WORM}, TILE_CORPSE_ROCK_WORM
},

{
    MONS_SWAMP_WORM, 'w', BROWN, "swamp worm",
    M_NO_SKELETON | M_COLD_BLOOD,
    MR_RES_ACID | MR_VUL_SLASHING | MR_VUL_BLUDGEONING | MR_RES_PIERCING,
    3, MONS_WORM, MONS_SWAMP_WORM, MH_NATURAL, 10,
    { {AT_BITE, AF_PLAIN, 26}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    10, 350,
    3, 18, MST_SWAMP_WORM, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_AMPHIBIOUS, 12, MOVE_ENERGY(12),
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_SNAKE,
    {TILEP_MONS_SWAMP_WORM, TVARY_WATER}, TILE_CORPSE_SWAMP_WORM
},

{
    MONS_LAVA_WORM, 'w', LIGHTRED, "lava worm",
    M_NO_SKELETON | M_COLD_BLOOD,
    mrd(MR_RES_FIRE, 3) | MR_VUL_COLD | MR_RES_ACID 
     | MR_VUL_SLASHING | MR_VUL_BLUDGEONING | MR_RES_PIERCING,
    6, MONS_WORM, MONS_LAVA_WORM, MH_NATURAL, 40,
    { { AT_BITE, AF_FIRE, 36 }, { AT_CONSTRICT, AF_CRUSH, 9 },
        AT_NO_ATK, AT_NO_ATK },
    17, 480,
    5, 24, MST_SWAMP_WORM, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_LAVA, 11, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_SNAKE,
    { TILEP_MONS_LAVA_WORM }, TILE_CORPSE_LAVA_WORM
},

{
    MONS_TYRANT_LEECH, 'w', RED, "tyrant leech",
    M_NO_SKELETON | M_BLOOD_SCENT | M_COLD_BLOOD,
    MR_RES_ACID | MR_VUL_SLASHING | MR_VUL_BLUDGEONING | MR_RES_PIERCING,
    8, MONS_TYRANT_LEECH, MONS_TYRANT_LEECH, MH_NATURAL, 40,
    { {AT_BITE, AF_VAMPIRIC, 35}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    12, 600,
    5, 22, MST_NO_SPELLS, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_AMPHIBIOUS, 8, SWIM_ENERGY(6),
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_SNAKE,
    {TILEP_MONS_TYRANT_LEECH}, TILE_CORPSE_TYRANT_LEECH
},

{
    MONS_DART_SLUG, 'w', CYAN, "dart slug",
    M_NO_SKELETON | M_COLD_BLOOD,
    MR_VUL_POISON | MR_RES_ACID | MR_VUL_SLASHING | MR_VUL_BLUDGEONING
      | MR_RES_PIERCING,
    40, MONS_ELEPHANT_SLUG, MONS_DART_SLUG, MH_NATURAL, 0,
    { {AT_BITE, AF_PLAIN, 3}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 100,
    1, 1, MST_DART_SLUG, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_LAND, 7, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LITTLE, MON_SHAPE_SNAIL,
    {TILEP_MONS_DART_SLUG}, TILE_CORPSE_DART_SLUG
},

{
    MONS_TORPOR_SNAIL, 'w', GREEN, "torpor snail",
    M_NO_SKELETON | M_COLD_BLOOD,
    MR_VUL_SLASHING | MR_RES_PIERCING,
    20, MONS_ELEPHANT_SLUG, MONS_TORPOR_SNAIL, MH_NATURAL, 40,
    { {AT_BITE, AF_PLAIN, 25}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    10, 600,
    8, 1, MST_NO_SPELLS, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_AMPHIBIOUS, 7, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_SNAIL,
    {TILEP_MONS_TORPOR_SNAIL}, TILE_CORPSE_TORPOR_SNAIL
},

// dummy genus monster; not using DUMMY since it's also a dummy species
{
    MONS_ELEPHANT_SLUG, 'w', WHITE, "elephant slug",
    M_NO_SKELETON | M_CANT_SPAWN | M_COLD_BLOOD,
    MR_VUL_POISON | MR_VUL_SLASHING | MR_RES_PIERCING,
    2, MONS_ELEPHANT_SLUG, MONS_ELEPHANT_SLUG, MH_NATURAL, 80,
    { {AT_BITE, AF_PLAIN, 40}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    20, 1300,
    2, 1, MST_NO_SPELLS, CE_NOXIOUS, S_SILENT,
    I_ANIMAL, HT_LAND, 4, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_BIG, MON_SHAPE_SNAIL,
    {TILEP_MONS_DART_SLUG}, TILE_CORPSE_ELEPHANT_SLUG
},

// small abominations ('x')
{
    MONS_UNSEEN_HORROR, 'x', MAGENTA, "unseen horror",
    M_SEE_INVIS | M_INVIS | M_BATTY,
    MR_VUL_SLASHING | MR_RES_PIERCING,
    12, MONS_UNSEEN_HORROR, MONS_UNSEEN_HORROR, MH_NATURAL, 20,
    { {AT_HIT, AF_PLAIN, 12}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    7, 385,
    5, 21, MST_NO_SPELLS, CE_NOCORPSE, S_GIBBERING,
    I_ANIMAL, HT_LAND, 30, DEFAULT_ENERGY,
    MU_CONSUMABLES, SIZE_MEDIUM, MON_SHAPE_MISC,
    {TILEP_MONS_UNSEEN_HORROR}, TILE_ERROR
},

{
    MONS_ABOMINATION_SMALL, 'x', LIGHTRED, "small abomination",
    M_NO_REGEN,
    MR_RES_PIERCING | MR_RES_SLASHING,
    10, MONS_ABOMINATION_SMALL, MONS_ABOMINATION_SMALL, MH_UNDEAD, 40,
    { {AT_HIT, AF_PLAIN, 23}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    6, 270,
    0, 0, MST_NO_SPELLS, CE_NOCORPSE, S_GIBBERING,
    I_BRAINLESS, HT_LAND, 11, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_MISC,
    {TILEP_MONS_ABOMINATION_SMALL, TVARY_MOD}, TILE_ERROR
},

{
    MONS_CRAWLING_CORPSE, 'x', BROWN, "crawling corpse",
    M_NO_EXP_GAIN | M_NO_REGEN,
    mrd(MR_RES_COLD, 2) | MR_RES_PIERCING | MR_RES_SLASHING,
    8, MONS_MACABRE_MASS, MONS_CRAWLING_CORPSE, MH_UNDEAD, 40,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 10,
    1, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 8, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_MISC,
    {TILEP_MONS_CRAWLING_CORPSE}, TILE_ERROR
},

{
    MONS_MACABRE_MASS, 'x', BROWN, "macabre mass",
    M_NO_EXP_GAIN | M_NO_REGEN,
    mrd(MR_RES_COLD, 2) | MR_RES_PIERCING | MR_RES_SLASHING,
    8, MONS_MACABRE_MASS, MONS_MACABRE_MASS, MH_UNDEAD, 40,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 30,
    1, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 5, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_MISC,
    {TILEP_MONS_MACABRE_MASS}, TILE_ERROR
},

{
    MONS_WORLDBINDER, 'x', CYAN, "worldbinder",
    M_SEE_INVIS | M_FLIES | M_SPELLCASTER,
    mrd(MR_RES_ACID, 3) | MR_RES_PHYSICAL,
    13, MONS_WORLDBINDER, MONS_WORLDBINDER, MH_ELEMENTAL, 60,
    { {AT_HIT, AF_PLAIN, 8}, {AT_HIT, AF_PLAIN, 8},
       AT_NO_ATK, AT_NO_ATK },
    8, 400,
    12, 4, MST_WORLDBINDER, CE_NOCORPSE, S_SILENT,
    I_ANIMAL, HT_LAND, 14, DEFAULT_ENERGY,
    MU_DOOR, SIZE_SMALL, MON_SHAPE_MISC,
    {TILEP_MONS_WORLDBINDER}, TILE_ERROR
},

// flying insects ('y')
{
    MONS_KILLER_BEE, 'y', BROWN, "killer bee",
    M_NO_SKELETON | M_FLIES | M_COLD_BLOOD,
    MR_VUL_POISON | MR_RES_ACID | MR_RES_PIERCING 
     | MR_VUL_BLUDGEONING,
    11, MONS_KILLER_BEE, MONS_KILLER_BEE, MH_NATURAL, 10,
    { {AT_STING, AF_POISON, 10}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    3, 165,
    2, 21, MST_NO_SPELLS, CE_NOXIOUS, S_BUZZ,
    I_ANIMAL, HT_LAND, 20, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_INSECT_WINGED,
    {TILEP_MONS_KILLER_BEE}, TILE_CORPSE_KILLER_BEE
},

{
    MONS_QUEEN_BEE, 'y', LIGHTMAGENTA, "queen bee",
    M_NO_SKELETON | M_FLIES | M_COLD_BLOOD,
    MR_VUL_POISON | MR_RES_ACID | MR_RES_PIERCING
     | MR_VUL_BLUDGEONING,
    14, MONS_KILLER_BEE, MONS_QUEEN_BEE, MH_NATURAL, 20,
    { {AT_STING, AF_POISON, 20}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    7, 385,
    10, 12, MST_QUEEN_BEE, CE_NOXIOUS, S_BUZZ,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LITTLE, MON_SHAPE_INSECT_WINGED,
    {TILEP_MONS_QUEEN_BEE}, TILE_CORPSE_QUEEN_BEE
},

{
    MONS_VAMPIRE_MOSQUITO, 'y', LIGHTGREY, "vampire mosquito",
    M_NO_SKELETON | M_BLOOD_SCENT | M_FLIES,
    MR_RES_ACID | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    10, MONS_VAMPIRE_MOSQUITO, MONS_VAMPIRE_MOSQUITO, MH_UNDEAD, 20,
    { {AT_BITE, AF_VAMPIRIC, 13}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    5, 275,
    2, 23, MST_NO_SPELLS, CE_NOXIOUS, S_BUZZ,
    I_ANIMAL, HT_LAND, 19, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LITTLE, MON_SHAPE_INSECT_WINGED,
    {TILEP_MONS_VAMPIRE_MOSQUITO}, TILE_CORPSE_VAMPIRE_MOSQUITO
},

{
    MONS_HORNET, 'y', YELLOW, "hornet",
    M_NO_SKELETON | M_FLIES | M_COLD_BLOOD,
    MR_VUL_POISON | MR_RES_ACID | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    13, MONS_HORNET, MONS_HORNET, MH_NATURAL, 30,
    { {AT_STING, AF_POISON_PETRIFY, 18}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    6, 330,
    6, 21, MST_NO_SPELLS, CE_NOXIOUS, S_BUZZ,
    I_ANIMAL, HT_LAND, 15, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_INSECT_WINGED,
    {TILEP_MONS_HORNET}, TILE_CORPSE_HORNET
},

{
    MONS_MELIAI, 'y', GREEN, "meliai",
    M_NO_SKELETON | M_FLIES | M_SPELLCASTER | M_FIGHTER | M_COLD_BLOOD | M_GENDERED,
    MR_VUL_POISON | MR_RES_ACID | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    6, MONS_KILLER_BEE, MONS_MELIAI, MH_NATURAL, 20,
    { {AT_HIT, AF_PLAIN, 14}, {AT_SHIELD, AF_PLAIN, 14}, {AT_STING, AF_POISON, 12}, AT_NO_ATK },
    7, 245,
    2, 23, MST_MELIAI, CE_NOXIOUS, S_BUZZ,
    I_HUMAN, HT_LAND, 20, DEFAULT_ENERGY,
    MU_MELEE, SIZE_SMALL, MON_SHAPE_INSECT_WINGED,
    {TILEP_MONS_MELIAI}, TILE_CORPSE_MELIAI
},

{
    MONS_SPARK_WASP, 'y', LIGHTCYAN, "spark wasp",
    M_NO_SKELETON | M_FLIES | M_COLD_BLOOD,
    mrd(MR_RES_ELEC, 3) | MR_VUL_POISON | MR_RES_ACID 
     | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    12, MONS_SPARK_WASP, MONS_SPARK_WASP, MH_NATURAL, 40,
    { {AT_STING, AF_ELEC, 33}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    12, 660,
    9, 28, MST_SPARK_WASP, CE_NOXIOUS, S_BUZZ,
    I_ANIMAL, HT_LAND, 15, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_INSECT_WINGED,
    {TILEP_MONS_SPARK_WASP}, TILE_CORPSE_SPARK_WASP
},

DUMMY(MONS_MOTH, 'y', WHITE, "moth", TILEP_MONS_MOTH_OF_WRATH)

{
    MONS_GHOST_MOTH, 'y', MAGENTA, "ghost moth",
    M_NO_SKELETON | M_INVIS | M_FLIES | M_COLD_BLOOD,
    MR_RES_POISON | MR_RES_COLD | MR_VUL_PHYSICAL,
    18, MONS_MOTH, MONS_GHOST_MOTH, MH_NATURAL, 100,
    { {AT_HIT, AF_DRAIN_STAT, 18}, {AT_HIT, AF_DRAIN_STAT, 18},
      {AT_STING, AF_POISON_STRONG, 12}, AT_NO_ATK },
    13, 715,
    8, 17, MST_DRAIN_GAZE, CE_NOXIOUS, S_SILENT,
    I_ANIMAL, HT_LAND, 12, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_INSECT_WINGED,
    {TILEP_MONS_GHOST_MOTH}, TILE_CORPSE_GHOST_MOTH
},

{
    MONS_MOTH_OF_WRATH, 'y', LIGHTRED, "moth of wrath",
    M_NO_SKELETON | M_FLIES | M_COLD_BLOOD,
    MR_VUL_PHYSICAL,
    10, MONS_MOTH, MONS_MOTH_OF_WRATH, MH_NATURAL, 40,
    { {AT_BITE, AF_RAGE, 25}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    9, 495,
    0, 17, MST_MOTH_OF_WRATH, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_LAND, 12, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_INSECT_WINGED,
    {TILEP_MONS_MOTH_OF_WRATH}, TILE_CORPSE_MOTH_OF_WRATH
},

{
    MONS_SPHINX_MOTH, 'y', LIGHTGREEN, "sphinx moth",
    M_NO_SKELETON | M_FLIES | M_COLD_BLOOD,
    MR_VUL_PHYSICAL,
    10, MONS_MOTH, MONS_SPHINX_MOTH, MH_NATURAL, 40,
    { { AT_BITE, AF_PLAIN, 25 }, { AT_CLAW, AF_PLAIN, 15}, { AT_CLAW, AF_PLAIN, 15 }, AT_NO_ATK },
    10, 695,
    5, 21, MST_SPHINX_MOTH, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_LAND, 12, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_QUADRUPED_WINGED,
    { TILEP_MONS_SPHINX_MOTH }, TILE_CORPSE_SPHINX_MOTH
},

{
    MONS_CHAOS_BUTTERFLY, 'y', CYAN, "chaos butterfly",
    M_NO_SKELETON | M_FLIES | M_NO_POLY_TO | M_COLD_BLOOD,
    MR_VUL_PHYSICAL,
    33, MONS_MOTH, MONS_CHAOS_BUTTERFLY, MH_NATURAL, 40,
    { {AT_BITE, AF_CHAOTIC, 18}, { AT_TOUCH, AF_BLINK, 8 }, AT_NO_ATK, AT_NO_ATK },
    9, 645,
    3, 27, MST_CHAOS_BUTTERFLY, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_LAND, 12, SPELL_ENERGY(6),
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_INSECT_WINGED,
    {TILEP_MONS_CHAOS_BUTTERFLY}, TILE_CORPSE_CHAOS_BUTTERFLY
},

// corporeal (semi-)humanoid undead ('z')
{
    MONS_WIGHT, 'z', GREEN, "wight",
    M_GENDERED,
    mrd(MR_RES_COLD, 2) | MR_VUL_ACID | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    16, MONS_WIGHT, MONS_WIGHT, MH_UNDEAD, 20,
    { {AT_HIT, AF_DRAIN_XP, 8}, {AT_SHIELD, AF_PLAIN, 8}, AT_NO_ATK, AT_NO_ATK },
    3, 165,
    4, 9, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE | MU_THROW_ROCK), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_WIGHT}, TILE_ERROR
},

{
    MONS_SKELETAL_WARRIOR, 'z', CYAN, "skeletal warrior",
    M_FIGHTER | M_GENDERED,
    MR_RES_COLD | MR_VUL_ACID | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    10, MONS_SKELETAL_WARRIOR, MONS_SKELETAL_WARRIOR, MH_UNDEAD, 100,
    { {AT_HIT, AF_PLAIN, 25}, {AT_SHIELD, AF_PLAIN, 25}, AT_NO_ATK, AT_NO_ATK },
    10, 650,
    15, 11, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE | MU_THROW_JAVELIN), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_SKELETAL_WARRIOR}, TILE_ERROR
},

{
    MONS_ANCIENT_CHAMPION, 'z', LIGHTCYAN, "ancient champion",
    M_FIGHTER | M_SPELLCASTER | M_GENDERED,
    MR_RES_COLD | MR_VUL_ACID | MR_RES_PIERCING | MR_VUL_BLUDGEONING | MR_RES_SLASHING,
    24, MONS_ANCIENT_CHAMPION, MONS_ANCIENT_CHAMPION, MH_UNDEAD, 120,
    { {AT_HIT, AF_PLAIN, 32}, {AT_SHIELD, AF_PLAIN, 32}, AT_NO_ATK, AT_NO_ATK },
    14, 700,
    15, 18, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ANCIENT_CHAMPION}, TILE_ERROR
},

{
    MONS_FLYING_SKULL, 'z', WHITE, "flying skull",
    M_FLIES, // duh
    MR_RES_FIRE | MR_RES_COLD | MR_RES_ELEC | MR_VUL_ACID
     | MR_RES_PIERCING | MR_VUL_BLUDGEONING | MR_RES_SLASHING,
    10, MONS_FLYING_SKULL, MONS_FLYING_SKULL, MH_UNDEAD, 20,
    { {AT_HIT, AF_PLAIN, 14}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    7, 210,
    10, 24, MST_NO_SPELLS, CE_NOCORPSE, S_SCREAM,
    I_ANIMAL, HT_LAND, 15, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_MISC,
    {TILEP_MONS_FLYING_SKULL}, TILE_ERROR
},

{
    MONS_CURSE_SKULL, 'z', LIGHTMAGENTA, "curse skull",
    M_SEE_INVIS | M_SPEAKS | M_FLIES | M_SPELLCASTER | M_GENDERED,
    mrd(MR_RES_FIRE | MR_RES_COLD, 2) | MR_RES_ELEC | MR_VUL_ACID
     | MR_RES_PIERCING | MR_VUL_BLUDGEONING | MR_RES_SLASHING,
    20, MONS_CURSE_SKULL, MONS_CURSE_SKULL, MH_UNDEAD, 180,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    13, 520,
    35, 3, MST_CURSE_SKULL, CE_NOCORPSE, S_MOAN,
    I_HUMAN, HT_LAND, 15, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_MISC,
    {TILEP_MONS_CURSE_SKULL}, TILE_ERROR
},

{
    MONS_CURSE_TOE, 'z', LIGHTGREEN, "curse toe",
    M_SEE_INVIS | M_SPEAKS | M_FLIES | M_SPELLCASTER,
    MR_RES_ELEC | MR_RES_COLD | MR_RES_PIERCING,
    60, MONS_LICH, MONS_CURSE_TOE, MH_UNDEAD, 180,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    14, 980,
    25, 1, MST_CURSE_TOE, CE_NOCORPSE, S_MOAN,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(14),
    MU_NOTHING, SIZE_TINY, MON_SHAPE_MISC,
    {TILEP_MONS_CURSE_TOE}, TILE_ERROR
},

// angelic beings ('A')
{
    MONS_ANGEL, 'A', WHITE, "angel",
    M_FIGHTER | M_SEE_INVIS | M_SPEAKS | M_FLIES | M_GENDERED,
    MR_RES_POISON | MR_RES_ELEC | MR_RES_PIERCING | MR_RES_SLASHING,
    10, MONS_ANGEL, MONS_ANGEL, MH_HOLY, 120,
    { {AT_HIT, AF_PLAIN, 25}, {AT_SHIELD, AF_PLAIN, 25}, {AT_HIT, AF_PLAIN, 10},
       AT_NO_ATK },
    12, 1020,
    10, 33, MST_ANGEL, CE_DIVINE, S_SHOUT,
    I_HUMAN, HT_LAND, 15, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_WINGED,
    {TILEP_MONS_ANGEL}, TILE_CORPSE_ANGEL
},

{
    MONS_CHERUB, 'A', LIGHTBLUE, "cherub",
    M_FIGHTER | M_ARCHER | M_SEE_INVIS | M_SPEAKS | M_FLIES | M_GENDERED,
    MR_RES_POISON | MR_RES_ELEC | MR_RES_FIRE 
     | MR_RES_PIERCING | MR_RES_SLASHING,
    10, MONS_ANGEL, MONS_CHERUB, MH_HOLY, 100,
    { {AT_HIT, AF_PLAIN, 15}, {AT_SHIELD, AF_PLAIN, 15}, 
      {AT_CHERUB, AF_PLAIN, 8}, AT_NO_ATK },
    9, 765,
    10, 33, MST_BATTLECRY, CE_DIVINE, S_CHERUB,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_WINGED,
    {TILEP_MONS_CHERUB}, TILE_CORPSE_CHERUB
},

{
    MONS_SERAPH, 'A', LIGHTMAGENTA, "seraph",
    M_FIGHTER | M_SEE_INVIS | M_SPEAKS | M_FLIES | M_GENDERED,
    MR_RES_POISON | MR_RES_ELEC | mrd(MR_RES_FIRE, 3) 
     | MR_RES_PIERCING | MR_RES_SLASHING,
    10, MONS_ANGEL, MONS_SERAPH, MH_HOLY, 160,
    { {AT_HIT, AF_PLAIN, 50}, {AT_SHIELD, AF_PLAIN, 50}, 
      {AT_HIT, AF_PLAIN, 20}, AT_NO_ATK },
    25, 2125,
    10, 33, MST_SERAPH, CE_DIVINE, S_SHOUT,
    I_HUMAN, HT_LAND, 15, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_WINGED,
    {TILEP_MONS_SERAPH}, TILE_CORPSE_SERAPH
},

{
    MONS_DAEVA, 'A', YELLOW, "daeva",
    M_FIGHTER | M_SEE_INVIS | M_SPEAKS | M_FLIES | M_GENDERED,
    MR_RES_POISON | MR_RES_PIERCING | MR_RES_SLASHING,
    12, MONS_ANGEL, MONS_DAEVA, MH_HOLY, 140,
    { {AT_HIT, AF_PLAIN, 25}, {AT_SHIELD, AF_PLAIN, 25}, 
      {AT_HIT, AF_PLAIN, 10}, AT_NO_ATK },
    14, 1190,
    10, 24, MST_DAEVA, CE_DIVINE, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_WINGED,
    {TILEP_MONS_DAEVA}, TILE_CORPSE_DAEVA
},

{
    MONS_PROFANE_SERVITOR, 'A', RED, "profane servitor",
    M_FIGHTER | M_SEE_INVIS | M_SPEAKS | M_FLIES | M_GENDERED,
    MR_RES_COLD | MR_RES_ELEC | MR_RES_PIERCING | MR_RES_SLASHING,
    10, MONS_ANGEL, MONS_PROFANE_SERVITOR, MH_UNDEAD, 140,
    { {AT_HIT, AF_VAMPIRIC, 25}, {AT_SHIELD, AF_PLAIN, 25}, 
      {AT_HIT, AF_DRAIN_XP, 10}, AT_NO_ATK},
    18, 1530,
    10, 33, MST_NO_SPELLS, CE_DIVINE, S_SILENT,
    I_HUMAN, HT_LAND, 15, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_WINGED,
    {TILEP_MONS_PROFANE_SERVITOR}, TILE_CORPSE_PROFANE_SERVITOR
},

// beetles and other insects('B')
DUMMY(MONS_BEETLE, 'B', BROWN, "beetle", TILEP_MONS_BOULDER_BEETLE)

{
    MONS_GIANT_COCKROACH, 'B', BROWN, "giant cockroach",
    M_NO_SKELETON | M_COLD_BLOOD,
    MR_VUL_POISON | MR_RES_ACID | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    10, MONS_GIANT_COCKROACH, MONS_GIANT_COCKROACH, MH_NATURAL, 0,
    { {AT_BITE, AF_PLAIN, 2}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    3, 50,
    3, 8, MST_NO_SPELLS, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_LAND, 12, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LITTLE, MON_SHAPE_INSECT,
    {TILEP_MONS_GIANT_COCKROACH}, TILE_CORPSE_GIANT_COCKROACH
},

{
    MONS_WORKER_ANT, 'B', RED, "worker ant",
    M_NO_SKELETON | M_COLD_BLOOD,
    MR_VUL_POISON | MR_RES_ACID | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    10, MONS_WORKER_ANT, MONS_WORKER_ANT, MH_NATURAL, 10,
    { {AT_BITE, AF_POISON, 8}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    3, 165,
    4, 12, MST_NO_SPELLS, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_LAND, 12, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_INSECT,
    {TILEP_MONS_WORKER_ANT}, TILE_CORPSE_WORKER_ANT
},

{
    MONS_QUEEN_ANT, 'B', LIGHTRED, "queen ant",
    M_NO_SKELETON | M_COLD_BLOOD,
    MR_VUL_POISON | MR_RES_ACID | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    10, MONS_WORKER_ANT, MONS_QUEEN_ANT, MH_NATURAL, 60,
    { {AT_STING, AF_POISON_STRONG, 20}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    13, 715,
    14, 3, MST_NO_SPELLS, CE_NOXIOUS, S_SILENT,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_INSECT,
    {TILEP_MONS_QUEEN_ANT}, TILE_CORPSE_QUEEN_ANT
},

{
    MONS_SOLDIER_ANT, 'B', WHITE, "soldier ant",
    M_NO_SKELETON | M_COLD_BLOOD,
    MR_VUL_POISON | MR_RES_ACID | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    10, MONS_WORKER_ANT, MONS_SOLDIER_ANT, MH_NATURAL, 20,
    { {AT_STING, AF_POISON, 14}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    6, 330,
    8, 14, MST_NO_SPELLS, CE_NOXIOUS, S_SILENT,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_INSECT,
    {TILEP_MONS_SOLDIER_ANT}, TILE_CORPSE_SOLDIER_ANT
},

// dummy for recolouring
{
    MONS_FORMICID, 'B', GREEN, "formicid",
    M_WARM_BLOOD | M_SEE_INVIS | M_SPEAKS | M_NO_SKELETON | M_NO_POLY_TO
        | M_NO_GEN_DERIVED | M_GENDERED,
    MR_RES_ACID | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    10, MONS_FORMICID, MONS_FORMICID, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 15}, {AT_SHIELD, AF_PLAIN, 15}, AT_NO_ATK, AT_NO_ATK },
    6, 420,
    3, 10, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_FORMICID}, TILE_CORPSE_FORMICID
},

{
    MONS_ENTROPY_WEAVER, 'B', LIGHTCYAN, "entropy weaver",
    M_WARM_BLOOD | M_NO_SKELETON | M_FIGHTER | M_SEE_INVIS | M_SPEAKS | M_GENDERED,
    mrd(MR_RES_ACID, 2) | MR_VUL_SLASHING,
    10, MONS_ENTROPY_WEAVER, MONS_ENTROPY_WEAVER, MH_NATURAL, 60,
    { {AT_HIT, AF_PLAIN, 17}, {AT_HIT, AF_PLAIN, 17}, {AT_HIT, AF_PLAIN, 17}, {AT_HIT, AF_PLAIN, 17} },
    13, 715,
    7, 22, MST_ENTROPY_WEAVER, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_SLIME, 12, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_ARMOUR | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ENTROPY_WEAVER}, TILE_CORPSE_ENTROPY_WEAVER
},

{
    MONS_BOULDER_BEETLE, 'B', LIGHTGREY, "boulder beetle",
    M_NO_SKELETON | M_COLD_BLOOD,
    MR_VUL_POISON | MR_RES_ACID | MR_RES_PIERCING,
    8, MONS_BEETLE, MONS_BOULDER_BEETLE, MH_NATURAL, 40,
    { { AT_BITE, AF_PLAIN, 45 }, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    18, 850,
    9, 6, MST_BOULDER_BEETLE, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_LAND, 7, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_BIG, MON_SHAPE_INSECT,
    { TILEP_MONS_BOULDER_BEETLE }, TILE_CORPSE_BOULDER_BEETLE
},

{
    MONS_DEATH_SCARAB, 'B', BLUE, "death scarab",
    M_NO_SKELETON | M_BATTY,
    MR_RES_ACID | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    12, MONS_BEETLE, MONS_DEATH_SCARAB, MH_UNDEAD, 60,
    { { AT_BITE, AF_SCARAB, 33 }, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    8, 320,
    7, 22, MST_NO_SPELLS, CE_NOXIOUS, S_SILENT,
    I_ANIMAL, HT_LAND, 30, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LITTLE, MON_SHAPE_INSECT,
    { TILEP_MONS_DEATH_SCARAB }, TILE_CORPSE_DEATH_SCARAB
},

// cyclopes and giants ('C')
DUMMY(MONS_GIANT, 'C', LIGHTGREY, "giant", TILEP_MONS_STONE_GIANT)

{
    MONS_CYCLOPS, 'C', YELLOW, "cyclops",
    M_WARM_BLOOD | M_SPEAKS | M_ARCHER | M_GENDERED,
    MR_VUL_SLASHING,
    14, MONS_GIANT, MONS_CYCLOPS, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 35}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    9, 495,
    5, 3, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 7, DEFAULT_ENERGY,
    muf(MU_ARMOUR | MU_DOOR | MU_WEAPON_RANGED | MU_EVOKE), SIZE_GIANT, MON_SHAPE_HUMANOID,
    {TILEP_MONS_CYCLOPS}, TILE_CORPSE_CYCLOPS
},

{
    MONS_ETTIN, 'C', BROWN, "ettin",
    M_WARM_BLOOD | M_TWO_WEAPONS | M_SPEAKS | M_GENDERED,
    MR_VUL_SLASHING,
    12, MONS_GIANT, MONS_ETTIN, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 45}, {AT_HIT, AF_PLAIN, 45}, AT_NO_ATK, AT_NO_ATK },
    12, 660,
    9, 4, MST_NO_SPELLS, CE_CLEAN, S_SHOUT2,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_WEAPON_MELEE | MU_SHIELD | MU_EVOKE | MU_DOOR), SIZE_GIANT, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ETTIN}, TILE_CORPSE_ETTIN
},

{
    MONS_FIRE_GIANT, 'C', RED, "fire giant",
    M_FIGHTER | M_WARM_BLOOD | M_SEE_INVIS | M_SPEAKS | M_GENDERED,
    mrd(MR_RES_FIRE, 2) | MR_VUL_SLASHING,
    13, MONS_GIANT, MONS_FIRE_GIANT, MH_NATURAL, 80,
    { {AT_HIT, AF_PLAIN, 30}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    16, 960,
    8, 4, MST_FIRE_GIANT, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_WEAPON_MELEE | MU_EVOKE | MU_DOOR | MU_ARMOUR), SIZE_GIANT, MON_SHAPE_HUMANOID,
    {TILEP_MONS_FIRE_GIANT}, TILE_CORPSE_FIRE_GIANT
},

{
    MONS_FROST_GIANT, 'C', LIGHTBLUE, "frost giant",
    M_FIGHTER | M_WARM_BLOOD | M_SEE_INVIS | M_SPEAKS | M_GENDERED,
    mrd(MR_RES_COLD, 2) | MR_VUL_SLASHING,
    11, MONS_GIANT, MONS_FROST_GIANT, MH_NATURAL, 80,
    { {AT_HIT, AF_PLAIN, 35}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    16, 1040,
    9, 3, MST_FROST_GIANT, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_WEAPON_MELEE | MU_EVOKE | MU_DOOR | MU_ARMOUR), SIZE_GIANT, MON_SHAPE_HUMANOID,
    {TILEP_MONS_FROST_GIANT}, TILE_CORPSE_FROST_GIANT
},

{
    MONS_STONE_GIANT, 'C', LIGHTGREY, "stone giant",
    M_WARM_BLOOD | M_SPEAKS | M_ARCHER | M_GENDERED,
    MR_RES_BLUDGEONING,
    9, MONS_GIANT, MONS_STONE_GIANT, MH_NATURAL, 80,
    { {AT_HIT, AF_PLAIN, 45}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    16, 880,
    12, 2, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_WEAPON_RANGED | MU_EVOKE | MU_DOOR | MU_ARMOUR), SIZE_GIANT, MON_SHAPE_HUMANOID,
    {TILEP_MONS_STONE_GIANT}, TILE_CORPSE_STONE_GIANT
},

{
    MONS_TITAN, 'C', MAGENTA, "titan",
    M_FIGHTER | M_WARM_BLOOD | M_SEE_INVIS | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    mrd(MR_RES_ELEC, 3) | MR_VUL_SLASHING,
    12, MONS_GIANT, MONS_TITAN, MH_NATURAL, 180,
    { {AT_HIT, AF_PLAIN, 55}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    20, 1100,
    10, 3, MST_TITAN, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_WEAPONS | MU_EVOKE | MU_DOOR | MU_ARMOUR), SIZE_GIANT, MON_SHAPE_HUMANOID,
    {TILEP_MONS_TITAN}, TILE_CORPSE_TITAN
},

{
    MONS_JUGGERNAUT, 'C', LIGHTGREEN, "juggernaut",
    M_FIGHTER | M_WARM_BLOOD | M_SEE_INVIS | M_SPEAKS | M_GENDERED,
    MR_RES_SLASHING | MR_VUL_PIERCING,
    12, MONS_GIANT, MONS_JUGGERNAUT, MH_NATURAL, 120,
    { {AT_HIT, AF_PLAIN, 80}, {AT_HIT, AF_PLAIN, 40}, AT_NO_ATK, AT_NO_ATK },
    20, 1700,
    20, 5, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 15, ATTACK_ENERGY(45),
    muf(MU_DOOR | MU_EVOKE | MU_ARMOUR), SIZE_GIANT, MON_SHAPE_HUMANOID,
    {TILEP_MONS_JUGGERNAUT}, TILE_CORPSE_JUGGERNAUT
},

{
    MONS_IRON_GIANT, 'C', CYAN, "iron giant",
    M_WARM_BLOOD | M_SPEAKS | M_FIGHTER | M_SEE_INVIS | M_SPELLCASTER | M_GENDERED,
    MR_RES_POISON | MR_RES_FIRE | MR_RES_COLD | MR_VUL_ACID | MR_RES_BLUDGEONING,
    10, MONS_GIANT, MONS_IRON_GIANT, MH_NATURAL, 120,
    { {AT_HIT, AF_PLAIN, 65}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    18, 1800,
    18, 2, MST_IRON_GIANT, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_WEAPON_MELEE | MU_EVOKE | MU_DOOR | MU_ARMOUR), SIZE_GIANT, MON_SHAPE_HUMANOID,
    {TILEP_MONS_IRON_GIANT}, TILE_CORPSE_IRON_GIANT
},

// dragons ('D')
DUMMY(MONS_DRAGON, 'D', GREEN, "dragon", TILEP_MONS_FIRE_DRAGON)

{
    MONS_FIRE_DRAGON, 'D', LIGHTRED, "fire dragon",
    M_WARM_BLOOD | M_FLIES,
    MR_RES_POISON | mrd(MR_RES_FIRE, 2) | MR_VUL_COLD
     | MR_RES_SLASHING | MR_VUL_PIERCING,
    12, MONS_DRAGON, MONS_FIRE_DRAGON, MH_NATURAL, 60,
    { {AT_BITE, AF_PLAIN, 20}, {AT_CLAW, AF_PLAIN, 13},
      {AT_TRAMPLE, AF_TRAMPLE, 13}, AT_NO_ATK },
    12, 900,
    10, 11, MST_FIRE_DRAGON_BREATH, CE_CLEAN, S_LOUD_ROAR,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_WAND, SIZE_GIANT, MON_SHAPE_QUADRUPED_WINGED,
    {TILEP_MONS_FIRE_DRAGON}, TILE_CORPSE_FIRE_DRAGON
},

{
    MONS_HYDRA, 'D', LIGHTGREEN, "hydra",
    M_COLD_BLOOD | M_FAST_REGEN,
    MR_RES_POISON | MR_VUL_PIERCING,
    11, MONS_HYDRA, MONS_HYDRA, MH_NATURAL, 60,
    { {AT_MULTIBITE, AF_PLAIN, 18}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    13, 715,
    0, 5, MST_NO_SPELLS, CE_NOXIOUS, S_ROAR,
    I_ANIMAL, HT_AMPHIBIOUS, 10, SWIM_ENERGY(6),
    MU_NOTHING, SIZE_BIG, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_PROGRAM_BUG}, TILE_CORPSE_HYDRA
},

{
    MONS_ICE_DRAGON, 'D', WHITE, "ice dragon",
    M_COLD_BLOOD | M_FLIES,
    MR_RES_POISON | MR_VUL_FIRE | mrd(MR_RES_COLD, 2) 
     | MR_RES_SLASHING | MR_VUL_PIERCING,
    10, MONS_DRAGON, MONS_ICE_DRAGON, MH_NATURAL, 40,
    { {AT_BITE, AF_PLAIN, 17}, {AT_CLAW, AF_PLAIN, 17},
      {AT_TRAMPLE, AF_TRAMPLE, 17}, AT_NO_ATK },
    12, 900,
    10, 11, MST_ICE_DRAGON_BREATH, CE_CLEAN, S_LOUD_ROAR,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_WAND, SIZE_GIANT, MON_SHAPE_QUADRUPED_WINGED,
    {TILEP_MONS_ICE_DRAGON}, TILE_CORPSE_ICE_DRAGON
},

{
    MONS_STEAM_DRAGON, 'D', BLUE, "steam dragon",
    M_WARM_BLOOD | M_FLIES,
    MR_RES_SLASHING | MR_VUL_PIERCING,
    29, MONS_DRAGON, MONS_STEAM_DRAGON, MH_NATURAL, 20,
    { {AT_BITE, AF_PLAIN, 12}, {AT_CLAW, AF_PLAIN, 6}, AT_NO_ATK, AT_NO_ATK },
    4, 300,
    5, 17, MST_STEAM_DRAGON, CE_CLEAN, S_HISS, // just for the pun
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_WAND, SIZE_BIG, MON_SHAPE_QUADRUPED_WINGED,
    {TILEP_MONS_STEAM_DRAGON}, TILE_CORPSE_STEAM_DRAGON
},

{
    MONS_SWAMP_DRAGON, 'D', BROWN, "swamp dragon",
    M_WARM_BLOOD | M_FLIES,
    MR_RES_POISON | MR_RES_SLASHING | MR_VUL_PIERCING,
    11, MONS_DRAGON, MONS_SWAMP_DRAGON, MH_NATURAL, 40,
    { {AT_BITE, AF_PLAIN, 20}, {AT_CLAW, AF_PLAIN, 11},
      {AT_TRAMPLE, AF_TRAMPLE, 11}, AT_NO_ATK },
    9, 675,
    7, 10, MST_SWAMP_DRAGON, CE_NOXIOUS, S_LOUD_ROAR,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_WAND, SIZE_GIANT, MON_SHAPE_QUADRUPED_WINGED,
    {TILEP_MONS_SWAMP_DRAGON}, TILE_CORPSE_SWAMP_DRAGON
},

{
    MONS_ACID_DRAGON, 'D', GREEN, "acid dragon",
    M_WARM_BLOOD | M_FLIES,
    MR_RES_POISON | mrd(MR_RES_ACID, 2) | MR_RES_SLASHING | MR_VUL_PIERCING,
    16, MONS_DRAGON, MONS_ACID_DRAGON, MH_NATURAL, 20,
    { {AT_BITE, AF_PLAIN, 15}, {AT_CLAW, AF_PLAIN, 6}, AT_NO_ATK, AT_NO_ATK },
    5, 275,
    5, 18, MST_ACID_DRAGON, CE_NOXIOUS, S_ROAR,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_WAND, SIZE_BIG, MON_SHAPE_QUADRUPED_WINGED,
    {TILEP_MONS_ACID_DRAGON}, TILE_CORPSE_ACID_DRAGON
},

{
    MONS_QUICKSILVER_DRAGON, 'D', LIGHTCYAN, "quicksilver dragon",
    M_SEE_INVIS | M_WARM_BLOOD | M_FLIES,
    MR_RES_SLASHING | MR_VUL_PIERCING,
    14, MONS_DRAGON, MONS_QUICKSILVER_DRAGON, MH_NATURAL, 140,
    { {AT_BITE, AF_PLAIN, 25}, {AT_CLAW, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    16, 880,
    10, 26, MST_QUICKSILVER_DRAGON, CE_CLEAN, S_LOUD_ROAR,
    I_ANIMAL, HT_LAND, 15, DEFAULT_ENERGY,
    MU_WAND, SIZE_GIANT, MON_SHAPE_QUADRUPED_WINGED,
    {TILEP_MONS_QUICKSILVER_DRAGON}, TILE_CORPSE_QUICKSILVER_DRAGON
},

{
    MONS_IRON_DRAGON, 'D', CYAN, "iron dragon",
    M_SEE_INVIS | M_WARM_BLOOD | M_UNBREATHING,
    MR_RES_POISON | MR_RES_FIRE | MR_RES_COLD | MR_VUL_ACID
     | MR_RES_SLASHING | MR_RES_BLUDGEONING,
    14, MONS_DRAGON, MONS_IRON_DRAGON, MH_NATURAL, 160,
    { {AT_BITE, AF_PLAIN, 25}, {AT_CLAW, AF_PLAIN, 25},
      {AT_TRAMPLE, AF_TRAMPLE, 25}, AT_NO_ATK },
    18, 1170,
    20, 6, MST_SPLINTER_BREATH, CE_CLEAN, S_LOUD_ROAR,
    I_ANIMAL, HT_LAND, 8, DEFAULT_ENERGY,
    MU_WAND, SIZE_GIANT, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_IRON_DRAGON}, TILE_CORPSE_IRON_DRAGON
},

{
    MONS_STORM_DRAGON, 'D', LIGHTBLUE, "storm dragon",
    M_WARM_BLOOD | M_FLIES,
    mrd(MR_RES_ELEC, 3) | MR_RES_COLD 
     | MR_RES_SLASHING | MR_VUL_PIERCING,
    13, MONS_DRAGON, MONS_STORM_DRAGON, MH_NATURAL, 100,
    { {AT_BITE, AF_PLAIN, 25}, {AT_CLAW, AF_PLAIN, 15},
      {AT_TRAMPLE, AF_TRAMPLE, 15}, AT_NO_ATK },
    14, 1050,
    13, 17, MST_STORM_DRAGON, CE_CLEAN, S_LOUD_ROAR,
    I_ANIMAL, HT_LAND, 12, DEFAULT_ENERGY,
    MU_WAND, SIZE_GIANT, MON_SHAPE_QUADRUPED_WINGED,
    {TILEP_MONS_STORM_DRAGON}, TILE_CORPSE_STORM_DRAGON
},

{
    MONS_GOLDEN_DRAGON, 'D', YELLOW, "golden dragon",
    M_SEE_INVIS | M_WARM_BLOOD | M_FLIES,
    MR_RES_ELEC | MR_RES_POISON | MR_RES_FIRE | MR_RES_COLD 
     | MR_RES_SLASHING | MR_VUL_PIERCING,
    26, MONS_DRAGON, MONS_GOLDEN_DRAGON, MH_NATURAL, 180,
    { {AT_BITE, AF_PLAIN, 40}, {AT_CLAW, AF_PLAIN, 20},
      {AT_TRAMPLE, AF_TRAMPLE, 20}, AT_NO_ATK },
    18, 1080,
    15, 11, MST_GOLDEN_DRAGON, CE_NOXIOUS, S_LOUD_ROAR,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_WAND, SIZE_GIANT, MON_SHAPE_QUADRUPED_WINGED,
    {TILEP_MONS_GOLDEN_DRAGON}, TILE_CORPSE_GOLDEN_DRAGON
},

{
    MONS_SHADOW_DRAGON, 'D', MAGENTA, "shadow dragon",
    M_SEE_INVIS | M_COLD_BLOOD | M_FLIES | M_INVIS,
    MR_RES_POISON | mrd(MR_RES_COLD, 2) | mrd(MR_RES_NEG, 3) 
     | MR_RES_SLASHING | MR_VUL_PIERCING,
    12, MONS_DRAGON, MONS_SHADOW_DRAGON, MH_NATURAL, 120,
    { {AT_BITE, AF_DRAIN_XP, 20}, {AT_CLAW, AF_PLAIN, 15},
      {AT_CLAW, AF_PLAIN, 15}, AT_NO_ATK },
    17, 1275,
    15, 18, MST_SHADOW_DRAGON, CE_NOXIOUS, S_SILENT, // stealth dragon
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_WAND, SIZE_GIANT, MON_SHAPE_QUADRUPED_WINGED,
    {TILEP_MONS_SHADOW_DRAGON}, TILE_CORPSE_SHADOW_DRAGON
},

{
    MONS_BONE_DRAGON, 'D', LIGHTGREY, "bone dragon",
    M_SEE_INVIS | M_FLIES,
    MR_RES_FIRE | MR_RES_COLD | MR_RES_ELEC | MR_VUL_ACID
     | MR_RES_PIERCING | MR_RES_SLASHING,
    12, MONS_DRAGON, MONS_BONE_DRAGON, MH_UNDEAD, 100,
    { {AT_BITE, AF_PLAIN, 30}, {AT_CLAW, AF_PLAIN, 20},
      {AT_TRAMPLE, AF_TRAMPLE, 20}, AT_NO_ATK },
    20, 1800,
    20, 4, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_WAND, SIZE_GIANT, MON_SHAPE_QUADRUPED_WINGED,
    {TILEP_MONS_BONE_DRAGON}, TILE_ERROR
},

{
    MONS_PEARL_DRAGON, 'D', ETC_HOLY, "pearl dragon",
    M_SEE_INVIS | M_WARM_BLOOD | M_FLIES,
    MR_RES_SLASHING | MR_VUL_PIERCING,
    16, MONS_DRAGON, MONS_PEARL_DRAGON, MH_HOLY, 160,
    { {AT_BITE, AF_HOLY, 35}, {AT_CLAW, AF_HOLY, 20}, AT_NO_ATK, AT_NO_ATK },
    18, 1170,
    10, 31, MST_PEARL_DRAGON, CE_CLEAN, S_LOUD_ROAR,
    I_ANIMAL, HT_LAND, 12, DEFAULT_ENERGY,
    MU_WAND, SIZE_GIANT, MON_SHAPE_QUADRUPED_WINGED,
    {TILEP_MONS_PEARL_DRAGON}, TILE_CORPSE_PEARL_DRAGON
},

// elementals (E)
DUMMY(MONS_ELEMENTAL, 'E', LIGHTGREY, "elemental", TILEP_MONS_WATER_ELEMENTAL)

{
    MONS_EARTH_ELEMENTAL, 'E', ETC_EARTH, "earth elemental",
    M_NO_FLAGS,
    mrd(MR_RES_FIRE | MR_RES_COLD | MR_RES_ELEC, 3)
        | MR_RES_PETRIFY | MR_VUL_ACID | MR_RES_PHYSICAL,
    13, MONS_ELEMENTAL, MONS_EARTH_ELEMENTAL, MH_ELEMENTAL, MAG_IMMUNE,
    { {AT_HIT, AF_PLAIN, 40}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    6, 450,
    14, 4, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_ROCK, 6, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_BIG, MON_SHAPE_MISC,
    {TILEP_MONS_EARTH_ELEMENTAL}, TILE_ERROR
},

{
    MONS_FIRE_ELEMENTAL, 'E', ETC_FIRE, "fire elemental",
    M_INSUBSTANTIAL,
    mrd(MR_RES_FIRE, 4) | mrd(MR_RES_ACID, 3) | MR_VUL_COLD | MR_RES_ELEC
     | MR_VUL_WATER | MR_RES_PHYSICAL,
    10, MONS_ELEMENTAL, MONS_FIRE_ELEMENTAL, MH_ELEMENTAL, MAG_IMMUNE,
    { {AT_HIT, AF_PURE_FIRE, 0}, { AT_HIT, AF_PURE_FIRE, 0 }, AT_NO_ATK, AT_NO_ATK },
    6, 330,
    4, 18, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_AMPHIBIOUS_LAVA, 13, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_MISC,
    {TILEP_MONS_FIRE_ELEMENTAL}, TILE_ERROR
},

{
    MONS_WATER_ELEMENTAL, 'E', ETC_WATER, "water elemental",
    M_ABSORB_WATER,
    MR_VUL_FIRE | MR_RES_ELEC | mrd(MR_RES_ACID, 3) | MR_RES_PHYSICAL,
    12, MONS_ELEMENTAL, MONS_WATER_ELEMENTAL, MH_ELEMENTAL, MAG_IMMUNE,
    { {AT_HIT, AF_ENGULF, 22}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    6, 420,
    4, 7, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_AMPHIBIOUS, 10, SWIM_ENERGY(6),
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_MISC,
    {TILEP_MONS_WATER_ELEMENTAL}, TILE_ERROR
},

{
    MONS_AIR_ELEMENTAL, 'E', ETC_AIR, "air elemental",
    M_SEE_INVIS | M_INSUBSTANTIAL | M_FLIES,
    mrd(MR_RES_ELEC | MR_RES_ACID, 3) | MR_RES_WIND | MR_RES_PHYSICAL,
    6, MONS_ELEMENTAL, MONS_AIR_ELEMENTAL, MH_ELEMENTAL, MAG_IMMUNE,
    { {AT_HIT, AF_PLAIN, 15}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    6, 330,
    2, 31, MST_AIR_ELEMENTAL, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 25, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_MISC,
    {TILEP_MONS_AIR_ELEMENTAL}, TILE_ERROR
},

{
    MONS_ROT_ELEMENTAL, 'E', ETC_DECAY, "rot elemental",
    M_SEE_INVIS,
    mrd(MR_RES_ACID, 3) | MR_RES_COLD | MR_RES_PETRIFY | MR_RES_PIERCING,
    14, MONS_ELEMENTAL, MONS_ROT_ELEMENTAL, MH_ELEMENTAL, MAG_IMMUNE,
    { { AT_HIT, AF_DECAY, 15 }, { AT_HIT, AF_DECAY, 15 }, AT_NO_ATK, AT_NO_ATK },
    7, 650,
    8, 3, MST_ROT_ELEMENTAL, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 9, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_MISC,
    { TILEP_MONS_ROT_ELEMENTAL }, TILE_ERROR
},

{
    MONS_PAIN_ELEMENTAL, 'E', ETC_BLOOD, "pain elemental",
    M_SEE_INVIS,
    mrd(MR_RES_NEG, 4) | mrd(MR_RES_ACID, 3) | MR_RES_COLD | MR_RES_PHYSICAL,
    11, MONS_ELEMENTAL, MONS_PAIN_ELEMENTAL, MH_ELEMENTAL, MAG_IMMUNE,
    { { AT_SLASH, AF_PAIN, 25 }, { AT_TOUCH, AF_PURE_PAIN, 0 },
      { AT_TOUCH, AF_PURE_PAIN, 0 }, AT_NO_ATK },
    8, 480,
    0, 21, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 12, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_MISC,
    { TILEP_MONS_PAIN_ELEMENTAL }, TILE_ERROR
},

{
    MONS_CHAOS_ELEMENTAL, 'E', ETC_RANDOM, "chaos elemental",
    M_SEE_INVIS,
    MR_RES_PHYSICAL,
    12, MONS_ELEMENTAL, MONS_CHAOS_ELEMENTAL, MH_ELEMENTAL, 60,
    { { AT_RANDOM, AF_CHAOTIC, 19 }, { AT_RANDOM, AF_MUTATE, 12 },
      { AT_RANDOM, AF_PURE_CHAOS, 8 }, AT_NO_ATK },
    6, 330,
    7, 21, MST_NO_SPELLS, CE_NOCORPSE, S_RANDOM,
    I_ANIMAL, HT_INCORPOREAL, 10, DEFAULT_ENERGY,
    MU_DOOR, SIZE_BIG, MON_SHAPE_MISC,
    { TILEP_MONS_CHAOS_ELEMENTAL, TVARY_RANDOM }, TILE_ERROR
},

{
    MONS_IRON_ELEMENTAL, 'E', ETC_IRON, "iron elemental",
    M_NO_FLAGS,
    mrd(MR_RES_FIRE | MR_RES_COLD | MR_RES_ELEC, 3) | MR_VUL_ACID | MR_RES_PHYSICAL,
    13, MONS_ELEMENTAL, MONS_IRON_ELEMENTAL, MH_ELEMENTAL, MAG_IMMUNE,
    { {AT_HIT, AF_PLAIN, 40}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    12, 1080,
    20, 2, MST_IRON_ELEMENTAL, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 6, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_BIG, MON_SHAPE_MISC,
    {TILEP_MONS_IRON_ELEMENTAL}, TILE_ERROR
},

{
    MONS_ELEMENTAL_WELLSPRING, 'E', LIGHTCYAN, "elemental wellspring",
    M_FAST_REGEN | M_ABSORB_WATER,
    MR_RES_ELEC | mrd(MR_RES_ACID, 3) | MR_RES_PHYSICAL,
    13, MONS_ELEMENTAL, MONS_ELEMENTAL_WELLSPRING, MH_ELEMENTAL, MAG_IMMUNE,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    18, 620,
    8, 22, MST_ELEMENTAL_WELLSPRING, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_WATER, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_BIG, MON_SHAPE_MISC,
    {TILEP_MONS_ELEMENTAL_WELLSPRING}, TILE_ERROR
},

// frogs ('F')
DUMMY(MONS_FROG, 'F', LIGHTGREEN, "giant frog", TILEP_MONS_BULLFROG)

{
    MONS_BULLFROG, 'F', GREEN, "bullfrog",
    M_COLD_BLOOD,
    MR_VUL_ACID | MR_VUL_SLASHING | MR_VUL_PIERCING,
    10, MONS_FROG, MONS_BULLFROG, MH_NATURAL, 20,
    { {AT_BITE, AF_FROG, 0}, {AT_SLAM, AF_PLAIN, 9}, AT_NO_ATK, AT_NO_ATK },
    2, 180,
    0, 12, MST_TONGUE_LASH, CE_CLEAN, S_CROAK,
    I_ANIMAL, HT_AMPHIBIOUS, 7, SWIM_ENERGY(6),
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_QUADRUPED_TAILLESS,
    {TILEP_MONS_BULLFROG}, TILE_CORPSE_BULLFROG
},

{
    MONS_SPINY_FROG, 'F', YELLOW, "spiny frog",
    M_COLD_BLOOD | M_SPINY,
    MR_RES_POISON | MR_VUL_ACID | MR_VUL_PIERCING,
    16, MONS_FROG, MONS_SPINY_FROG, MH_NATURAL, 20,
    { { AT_BITE, AF_FROG, 0 }, { AT_SLAM, AF_PLAIN, 26}, AT_NO_ATK, AT_NO_ATK },
    7, 345,
    6, 9, MST_TONGUE_LASH, CE_NOXIOUS, S_CROAK,
    I_ANIMAL, HT_AMPHIBIOUS, 7, SWIM_ENERGY(6),
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_QUADRUPED_TAILLESS,
    {TILEP_MONS_SPINY_FROG}, TILE_CORPSE_SPINY_FROG
},

{
    MONS_BLINK_FROG, 'F', LIGHTGREEN, "blink frog",
    M_COLD_BLOOD,
    MR_VUL_ACID | MR_VUL_SLASHING | MR_VUL_PIERCING,
    13, MONS_FROG, MONS_BLINK_FROG, MH_NATURAL, 40,
    { { AT_BITE, AF_FROG, 0 }, { AT_SLAM, AF_BLINK, 20}, AT_NO_ATK, AT_NO_ATK },
    4, 280,
    0, 24, MST_BLINK_FROG, CE_CLEAN, S_CROAK,
    I_ANIMAL, HT_AMPHIBIOUS, 8, SWIM_ENERGY(6),
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_QUADRUPED_TAILLESS,
    {TILEP_MONS_BLINK_FROG}, TILE_CORPSE_BLINK_FROG
},

{
    MONS_GOLIATH_FROG, 'F', LIGHTGREY, "goliath frog",
    M_COLD_BLOOD,
    MR_NO_FLAGS, // it's tough out there for a frog.
    10, MONS_FROG, MONS_GOLIATH_FROG, MH_NATURAL, 80,
    { { AT_HIT, AF_REACH_TONGUE, 30 }, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    12, 500,
    4, 18, MST_NO_SPELLS, true, S_SILENT, // IRL, goliath frogs have no vocals!
    I_ANIMAL, HT_AMPHIBIOUS, 14, SWIM_ENERGY(6),
    MONUSE_NOTHING, SIZE_LITTLE /* ~13" */, MON_SHAPE_QUADRUPED_TAILLESS,
    {TILEP_MONS_GOLIATH_FROG}, TILE_CORPSE_GOLIATH_FROG
},

// dummy for recolouring
{
    MONS_BARACHI, 'F', LIGHTCYAN, "barachi",
    M_SPEAKS | M_NO_POLY_TO | M_GENDERED,
    MR_VUL_ACID | MR_VUL_SLASHING | MR_VUL_PIERCING,
    10, MONS_FROG, MONS_BARACHI, MH_NATURAL, 10,
    { {AT_HIT, AF_PLAIN, 6}, {AT_SHIELD, AF_PLAIN, 6}, 
      AT_NO_ATK, AT_NO_ATK },
    3, 180,
    0, 10, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_AMPHIBIOUS, 8, DEFAULT_ENERGY,
    MU_MELEE, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_BULLFROG}, TILE_CORPSE_BULLFROG
},

// eyes ('G')
{
    MONS_FLOATING_EYE, 'G', WHITE, "floating eye",
    M_SEE_INVIS | M_FLIES,
    MR_VUL_PIERCING | MR_RES_BLUDGEONING,
    6, MONS_FLOATING_EYE, MONS_FLOATING_EYE, MH_NATURAL, 10,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    9, 165,
    0, 1, MST_FLOATING_EYE, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_ORB,
    {TILEP_MONS_FLOATING_EYE}, TILE_ERROR
},

{
    MONS_EYE_OF_DRAINING, 'G', LIGHTGREY, "eye of draining",
    M_SEE_INVIS | M_FLIES,
    MR_VUL_PIERCING | MR_RES_BLUDGEONING,
    10, MONS_FLOATING_EYE, MONS_EYE_OF_DRAINING, MH_NATURAL, 60,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    7, 385,
    3, 1, MST_DRAIN_GAZE, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 5, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_ORB,
    {TILEP_MONS_EYE_OF_DRAINING}, TILE_ERROR
},

{
    MONS_GLOWING_ORANGE_BRAIN, 'G', LIGHTRED, "glowing orange brain",
    M_WARM_BLOOD | M_SEE_INVIS | M_FLIES | M_SPELLCASTER,
    MR_VUL_SLASHING | MR_VUL_BLUDGEONING,
    13, MONS_GLOWING_ORANGE_BRAIN, MONS_GLOWING_ORANGE_BRAIN, MH_NATURAL, 100,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    10, 550,
    2, 4, MST_GLOWING_ORANGE_BRAIN, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_DOOR, SIZE_MEDIUM, MON_SHAPE_ORB,
    {TILEP_MONS_GLOWING_ORANGE_BRAIN}, TILE_ERROR
},

{
    MONS_GREAT_ORB_OF_EYES, 'G', LIGHTGREEN, "great orb of eyes",
    M_SEE_INVIS | M_FLIES,
    MR_RES_POISON | MR_RES_ACID | MR_VUL_PIERCING | MR_VUL_PIERCING 
     | MR_RES_BLUDGEONING,
    13, MONS_FLOATING_EYE, MONS_GREAT_ORB_OF_EYES, MH_NATURAL, 120,
    { {AT_BITE, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    12, 660,
    10, 3, MST_GREAT_ORB_OF_EYES, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_ORB,
    {TILEP_MONS_GREAT_ORB_OF_EYES}, TILE_ERROR
},

{
    MONS_SHINING_EYE, 'G', LIGHTMAGENTA, "shining eye",
    M_SEE_INVIS | M_FLIES | M_MAINTAIN_RANGE,
    MR_RES_ACID | MR_VUL_PIERCING | MR_RES_BLUDGEONING,
    14, MONS_FLOATING_EYE, MONS_SHINING_EYE, MH_NATURAL, 100,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    10, 550,
    3, 1, MST_SHINING_EYE, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 7, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_ORB,
    {TILEP_MONS_SHINING_EYE}, TILE_ERROR
},

{
    MONS_EYE_OF_DEVASTATION, 'G', YELLOW, "eye of devastation",
    M_SEE_INVIS | M_FLIES,
    MR_RES_ACID | MR_VUL_PIERCING | MR_RES_BLUDGEONING,
    11, MONS_FLOATING_EYE, MONS_EYE_OF_DEVASTATION,
        MH_NATURAL, 100,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    10, 550,
    12, 1, MST_EYE_OF_DEVASTATION, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 7, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_ORB,
    {TILEP_MONS_EYE_OF_DEVASTATION}, TILE_ERROR
},

{
    MONS_GOLDEN_EYE, 'G', ETC_GOLD, "golden eye",
    M_SEE_INVIS | M_BATTY | M_FLIES,
    MR_RES_ACID | MR_VUL_PIERCING | MR_RES_BLUDGEONING,
    17, MONS_FLOATING_EYE, MONS_GOLDEN_EYE, MH_NATURAL, 60,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    6, 120,
    0, 33, MST_GOLDEN_EYE, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 13, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_ORB,
    {TILEP_MONS_GOLDEN_EYE}, TILE_ERROR
},

{
    MONS_OPHAN, 'G', RED, "ophan",
    M_SEE_INVIS | M_UNBREATHING | M_FLIES | M_SPELLCASTER | M_GENDER_NEUTRAL,
    MR_VUL_PIERCING | MR_RES_BLUDGEONING | MR_RES_SLASHING,
    14, MONS_ANGEL, MONS_OPHAN, MH_HOLY, 140,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    15, 975,
    10, 21, MST_OPHAN, CE_DIVINE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_ORB,
    {TILEP_MONS_OPHAN}, TILE_CORPSE_OPHAN
},

// hybrids ('H')
{
    MONS_HIPPOGRIFF, 'H', BROWN, "hippogriff",
    M_WARM_BLOOD | M_FLIES,
    MR_VUL_PIERCING,
    8, MONS_HIPPOGRIFF, MONS_HIPPOGRIFF, MH_NATURAL, 25,
    { {AT_BITE, AF_PLAIN, 12}, {AT_PECK, AF_PLAIN, 9}, {AT_CLAW, AF_PLAIN, 9},
       AT_NO_ATK },
    8, 435,
    2, 7, MST_NO_SPELLS, CE_CLEAN, S_SCREECH,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_QUADRUPED_WINGED,
    {TILEP_MONS_HIPPOGRIFF}, TILE_CORPSE_HIPPOGRIFF
},

{
    MONS_MANTICORE, 'H', RED, "manticore",
    M_WARM_BLOOD | M_FLIES,
    MR_VUL_SLASHING,
    10, MONS_MANTICORE, MONS_MANTICORE, MH_NATURAL, 40,
    { {AT_BITE, AF_PLAIN, 26}, {AT_CLAW, AF_PLAIN, 14}, {AT_CLAW, AF_PLAIN, 14},
      {AT_TAIL_SLAP, AF_BARBS, 10} },
    9, 495,
    5, 7, MST_MANTICORE, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_MANTICORE}, TILE_CORPSE_MANTICORE
},

{
    MONS_MINOTAUR, 'H', LIGHTRED, "minotaur",
    M_FIGHTER | M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_MINOTAUR, MONS_MINOTAUR, MH_NATURAL, 100,
    { {AT_HIT, AF_PLAIN, 35}, {AT_SHIELD, AF_PLAIN, 35}, {AT_GORE, AF_PLAIN, 20}, AT_NO_ATK },
    14, 780,
    6, 18, MST_NO_SPELLS, CE_CLEAN, S_BELLOW,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_MINOTAUR}, TILE_CORPSE_MINOTAUR
},

{
    MONS_SPHINX, 'H', LIGHTGREY, "sphinx",
    M_SEE_INVIS | M_WARM_BLOOD | M_SPEAKS | M_FLIES | M_SPELLCASTER | M_GENDERED,
    MR_VUL_SLASHING,
    12, MONS_SPHINX, MONS_SPHINX, MH_NATURAL, 60,
    { {AT_HIT, AF_PLAIN, 25}, {AT_HIT, AF_PLAIN, 12}, {AT_HIT, AF_PLAIN, 12},
       AT_NO_ATK },
    16, 880,
    5, 5, MST_SPHINX, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 11, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND | MU_JEWELS), SIZE_BIG, MON_SHAPE_QUADRUPED_WINGED,
    {TILEP_MONS_SPHINX}, TILE_CORPSE_SPHINX
},

{
    MONS_HARPY, 'H', GREEN, "harpy",
    M_WARM_BLOOD | M_BATTY | M_GENDERED | M_FLIES,
    MR_RES_POISON | MR_VUL_SLASHING,
    9, MONS_HARPY, MONS_HARPY, MH_NATURAL, 20,
    { {AT_CLAW, AF_PLAIN, 19}, {AT_CLAW, AF_PLAIN, 14}, AT_NO_ATK,
       AT_NO_ATK },
    7, 385,
    2, 24, MST_NO_SPELLS, CE_CLEAN, S_SCREECH,
    I_HUMAN, HT_LAND, 25, DEFAULT_ENERGY,
    muf(MU_WAND | MU_JEWELS), SIZE_MEDIUM, MON_SHAPE_HUMANOID_WINGED,
    {TILEP_MONS_HARPY}, TILE_CORPSE_HARPY
},

// Mutant beasts - variable traits
{
    MONS_MUTANT_BEAST, 'H', MAGENTA, "mutant beast",
    M_NO_POLY_TO | M_NO_GEN_DERIVED,
    MR_NO_FLAGS,
    11, MONS_MUTANT_BEAST, MONS_MUTANT_BEAST, MH_NATURAL, -3,
    { {AT_HIT, AF_PLAIN, 27}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    15, 825,
    8, 8, MST_NO_SPELLS, CE_NOXIOUS, S_RANDOM,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_QUADRUPED,
    {TILEP_MUTANT_BEAST_BASE}, TILE_CORPSE_MUTANT_BEAST
},


// Hepliaklqana's ancestor - variable traits
// BCADDO: Should Hep ancestors have a chance of AT_SHIELD?
{
    MONS_ANCESTOR, 'R', LIGHTCYAN, "ancestor",
    M_ANCESTOR | M_NO_POLY_TO | M_NO_GEN_DERIVED | M_FLIES | M_FAST_REGEN | M_INSUBSTANTIAL | M_SEE_INVIS,
    MR_RES_FIRE | MR_RES_COLD | MR_RES_ACID,
    1, MONS_ANCESTOR, MONS_ANCESTOR, MH_ELEMENTAL, -5,
    { {AT_HIT, AF_PLAIN, 27}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 1,
    5, 10, MST_NO_SPELLS, CE_NOCORPSE, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_START_ONLY | MU_ALL), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ANCESTOR, TVARY_SPECIES}, TILE_ERROR
},

{
    MONS_ANCESTOR_KNIGHT, 'R', LIGHTCYAN, "knight",
    M_ANCESTOR | M_NO_POLY_TO | M_NO_GEN_DERIVED | M_FLIES | M_FAST_REGEN | M_INSUBSTANTIAL | M_SEE_INVIS,
    MR_RES_FIRE | MR_RES_COLD | MR_RES_ACID,
    1, MONS_ANCESTOR, MONS_ANCESTOR_KNIGHT, MH_ELEMENTAL, -5,
    { {AT_HIT, AF_PLAIN, 27}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 1,
    5, 10, MST_NO_SPELLS, CE_NOCORPSE, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_START_ONLY | MU_ALL), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ANCESTOR_KNIGHT, TVARY_SPECIES }, TILE_ERROR
},
{
    MONS_ANCESTOR_BATTLEMAGE, 'R', LIGHTCYAN, "battlemage",
    M_ANCESTOR | M_NO_POLY_TO | M_NO_GEN_DERIVED | M_FLIES | M_FAST_REGEN | M_INSUBSTANTIAL | M_SEE_INVIS | M_SPELLCASTER,
    MR_RES_FIRE | MR_RES_COLD | MR_RES_ACID,
    1, MONS_ANCESTOR, MONS_ANCESTOR_BATTLEMAGE, MH_ELEMENTAL, -5,
    { {AT_HIT, AF_PLAIN, 27}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 1,
    5, 10, MST_NO_SPELLS, CE_NOCORPSE, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_START_ONLY | MU_ALL), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ANCESTOR_BATTLEMAGE, TVARY_SPECIES }, TILE_ERROR
},
{
    MONS_ANCESTOR_HEXER, 'R', LIGHTCYAN, "hexer",
    M_ANCESTOR | M_NO_POLY_TO | M_NO_GEN_DERIVED | M_FLIES | M_FAST_REGEN | M_INSUBSTANTIAL | M_SEE_INVIS | M_SPELLCASTER,
    MR_RES_FIRE | MR_RES_COLD | MR_RES_ACID,
    1, MONS_ANCESTOR, MONS_ANCESTOR_HEXER, MH_ELEMENTAL, -5,
    { {AT_HIT, AF_PLAIN, 27}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 1,
    5, 10, MST_NO_SPELLS, CE_NOCORPSE, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_START_ONLY | MU_ALL), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ANCESTOR_HEXER, TVARY_SPECIES }, TILE_ERROR
},

// jellies ('J')
{
    MONS_OOZE, 'J', LIGHTGREY, "ooze",
    M_SEE_INVIS | M_UNBLINDABLE,
    MR_RES_POISON | mrd(MR_RES_ACID, 3) | MR_RES_PIERCING 
     | MR_RES_SLASHING | MR_VUL_BLUDGEONING,
    3, MONS_JELLY, MONS_OOZE, MH_NATURAL, 20,
    { {AT_HIT, AF_PLAIN, 5}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    3, 120,
    1, 3, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_SLIME, 10, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_LITTLE, MON_SHAPE_BLOB,
    {TILEP_MONS_OOZE}, TILE_ERROR
},

{
    MONS_PULSATING_LUMP, 'J', LIGHTMAGENTA, "pulsating lump",
    M_UNBLINDABLE | M_SEE_INVIS,
    MR_RES_POISON | MR_RES_PIERCING | MR_RES_SLASHING | MR_VUL_BLUDGEONING,
    3, MONS_PULSATING_LUMP, MONS_PULSATING_LUMP, MH_NATURAL, 20,
    { {AT_HIT, AF_MUTATE, 50} , {AT_HIT, AF_ACID, 30}, {AT_ENGULF, AF_CRUSH, 10}, AT_NO_ATK },
    12, 660,
    3, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 6, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_LARGE, MON_SHAPE_BLOB,
    {TILEP_MONS_PULSATING_LUMP, TVARY_RANDOM}, TILE_ERROR
},

{
    MONS_FETID_CYST, 'J', RED, "fetid cyst",
    M_UNBLINDABLE | M_SEE_INVIS,
    MR_RES_COLD | MR_RES_PIERCING | MR_RES_SLASHING | MR_VUL_BLUDGEONING,
    3, MONS_PULSATING_LUMP, MONS_FETID_CYST, MH_UNDEAD, 20,
    { { AT_HIT, AF_DRAIN_SPEED, 50 } ,{ AT_HIT, AF_DRAIN_XP, 30 },{ AT_ENGULF, AF_CRUSH, 10 }, AT_NO_ATK },
    12, 660,
    3, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 6, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_LARGE, MON_SHAPE_BLOB,
    { TILEP_MONS_FETID_CYST }, TILE_ERROR
},

{
    MONS_GIBBERING_MOUND, 'J', BLUE, "gibbering mound",
    M_SEE_INVIS | M_UNBLINDABLE | M_ACID_SPLASH | M_EAT_DOORS,
    MR_RES_POISON | mrd(MR_RES_ACID, 3) | MR_RES_NEG  | MR_RES_COLD 
     | MR_RES_FIRE | MR_RES_PIERCING | MR_RES_SLASHING | MR_VUL_BLUDGEONING,
    19, MONS_JELLY, MONS_GIBBERING_MOUND, MH_DEMONIC, 60,
    { { AT_MULTIBITE, AF_RANDOM, 15}, AT_NO_ATK, AT_NO_ATK , AT_NO_ATK },
    11, 480,
    7, 3, MST_MOUND, CE_NOCORPSE, S_GIBBERING,
    I_BRAINLESS, HT_SLIME, 13, DEFAULT_ENERGY,
        MU_NO_GOLD, SIZE_GIANT, MON_SHAPE_BLOB,
    { TILEP_MONS_GIBBERING_MOUND }, TILE_ERROR
},

{
    MONS_JELLY, 'J', LIGHTRED, "jelly",
    M_SEE_INVIS | M_UNBLINDABLE | M_SPLITS | M_ACID_SPLASH | M_EAT_DOORS,
    MR_RES_POISON | mrd(MR_RES_ACID, 3) | MR_RES_PIERCING | MR_RES_SLASHING
    | MR_VUL_BLUDGEONING,
    15, MONS_JELLY, MONS_JELLY, MH_NATURAL, 10,
    { {AT_HIT, AF_ACID, 3}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    3, 165,
    0, 2, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_SLIME, 10, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_SMALL, MON_SHAPE_BLOB,
    {TILEP_MONS_JELLY}, TILE_ERROR
},

{
    MONS_SLIME_CREATURE, 'J', LIGHTGREEN, "slime creature",
    M_FAST_REGEN | M_HERD,
    MR_RES_POISON | MR_RES_ACID | MR_RES_PIERCING | MR_RES_SLASHING
     | MR_VUL_BLUDGEONING,
    3, MONS_JELLY, MONS_SLIME_CREATURE, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 22}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    11, 605,
    1, 4, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_SLIME, 10, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_SMALL, MON_SHAPE_BLOB,
    {TILEP_MONS_PROGRAM_BUG}, TILE_ERROR
},

{
    MONS_JELLYFISH, 'J', ETC_JEWEL, "jellyfish",
    M_CONFUSED | M_NO_SKELETON | M_NOT_DANGEROUS,
    MR_RES_POISON | MR_RES_ACID | MR_RES_PIERCING | MR_VUL_SLASHING,
    0, MONS_JELLYFISH, MONS_JELLYFISH, MH_NATURAL, 40,
    { { AT_STING, AF_POISON_STR, 1 }, { AT_STING, AF_POISON_STR, 1 },
      AT_NO_ATK, AT_NO_ATK },
    1, 120, 
    1, 12, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_WATER, 10, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_LITTLE, MON_SHAPE_BLOB,
    {TILEP_MONS_JELLYFISH, TVARY_MOD }, TILE_ERROR
},

DUMMY(MONS_MERGED_SLIME_CREATURE, 'J', LIGHTGREEN, "merged slime creature",
      TILEP_MONS_PROGRAM_BUG)

{
    MONS_AZURE_JELLY, 'J', LIGHTBLUE, "azure jelly",
    M_SEE_INVIS | M_UNBLINDABLE | M_EAT_DOORS,
    MR_RES_POISON | MR_RES_COLD | MR_VUL_FIRE | MR_RES_ELEC
     | mrd(MR_RES_ACID, 3) | MR_RES_PIERCING | MR_RES_SLASHING,
    14, MONS_JELLY, MONS_AZURE_JELLY, MH_NATURAL, 80,
    { {AT_HIT, AF_COLD, 12}, {AT_HIT, AF_COLD, 12}, {AT_HIT, AF_TRAMPLE, 12},
      {AT_HIT, AF_TRAMPLE, 12} },
    15, 825,
    5, 17, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_SLIME, 12, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_SMALL, MON_SHAPE_BLOB,
    {TILEP_MONS_AZURE_JELLY}, TILE_ERROR
},

{
    MONS_DEATH_OOZE, 'J', MAGENTA, "death ooze",
    M_SEE_INVIS | M_UNBLINDABLE | M_EAT_DOORS,
    MR_RES_COLD | mrd(MR_RES_ACID, 3) | MR_RES_PIERCING | MR_RES_SLASHING,
    15, MONS_JELLY, MONS_DEATH_OOZE, MH_UNDEAD, 120,
    { {AT_HIT, AF_ROT, 32}, {AT_HIT, AF_PLAIN, 32}, AT_NO_ATK, AT_NO_ATK },
    11, 495,
    2, 4, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_SLIME, 12, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_LITTLE, MON_SHAPE_BLOB,
    {TILEP_MONS_DEATH_OOZE}, TILE_ERROR
},

{
    MONS_ECTOPLASMIC_ORB, 'J', CYAN, "ectoplasmic emanation",
    M_SEE_INVIS | M_UNBLINDABLE | M_INVIS | M_INSUBSTANTIAL | M_FLIES,
    MR_RES_COLD | mrd(MR_RES_ACID, 3) | MR_RES_PHYSICAL,
    15, MONS_JELLY, MONS_ECTOPLASMIC_ORB, MH_UNDEAD, 120,
    { { AT_HIT, AF_PLAIN, 35 }, { AT_ENGULF, AF_ENGULF, 10 }, AT_NO_ATK, AT_NO_ATK },
    12, 695,
    0, 16, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_INCORPOREAL, 10, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_LITTLE, MON_SHAPE_BLOB,
    { TILEP_MONS_ECTOPLASMIC_ORB }, TILE_ERROR
},

{
    MONS_HOLY_FLAN, 'J', YELLOW, "holy flan",
    M_SEE_INVIS | M_UNBLINDABLE | M_EAT_DOORS | M_FAST_REGEN,
    MR_RES_PIERCING | MR_RES_SLASHING,
    10, MONS_JELLY, MONS_HOLY_FLAN, MH_HOLY, 80,
    { { AT_HIT, AF_HOLY, 30} , { AT_HIT, AF_TRAMPLE, 10 }, AT_NO_ATK, AT_NO_ATK },
    6, 1250,
    3, 2, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 12, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_LITTLE, MON_SHAPE_BLOB,
    { TILEP_MONS_HOLY_FLAN }, TILE_ERROR
},

{
    MONS_ACID_BLOB, 'J', LIGHTCYAN, "acid blob",
    M_SEE_INVIS | M_UNBLINDABLE | M_ACID_SPLASH | M_EAT_DOORS,
    MR_RES_POISON | mrd(MR_RES_ACID, 4) | MR_RES_PIERCING | MR_RES_SLASHING,
    12, MONS_JELLY, MONS_ACID_BLOB, MH_NATURAL, 160,
    { {AT_HIT, AF_ACID, 42}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    18, 990,
    1, 3, MST_ACID_SPIT, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_SLIME, 12, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_SMALL, MON_SHAPE_BLOB,
    {TILEP_MONS_ACID_BLOB}, TILE_ERROR
},

{
    MONS_VERDANT_MOULD, 'J', GREEN, "verdant mould",
    M_SEE_INVIS | M_UNBLINDABLE | M_EAT_DOORS,
    MR_RES_POISON | mrd(MR_RES_ACID, 3) | MR_RES_PIERCING | MR_RES_SLASHING,
    12, MONS_JELLY, MONS_VERDANT_MOULD, MH_PLANT, 120,
    { { AT_MULTIBITE, AF_ACID, 12 }, { AT_BITE, AF_VAMPIRIC, 22 }, { AT_CONSTRICT, AF_CRUSH, 9}, AT_NO_ATK },
    18, 866,
    1, 3, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_SLIME, 11, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_SMALL, MON_SHAPE_BLOB,
    { TILEP_MONS_VERDANT_MOULD }, TILE_ERROR
},

{
    MONS_LAVA_GLOB, 'J', LIGHTRED, "lava glob",
    M_SEE_INVIS | M_UNBLINDABLE | M_EAT_DOORS,
    mrd(MR_RES_POISON, 3) | mrd(MR_RES_ACID, 2) | mrd(MR_RES_FIRE, 3)
     | MR_VUL_COLD | MR_VUL_WATER | mrd(MR_RES_NEG, 3) | MR_RES_PIERCING | MR_RES_SLASHING,
    12, MONS_JELLY, MONS_LAVA_GLOB, MH_ELEMENTAL, 160,
    { {AT_HIT, AF_FIRE, 42}, {AT_HIT, AF_STICKY_FLAME, 20}, 
    AT_NO_ATK, AT_NO_ATK},
    22, 990,
    8, 3, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_AMPHIBIOUS_LAVA, 12, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_SMALL, MON_SHAPE_BLOB,
    {TILEP_MONS_LAVA_GLOB}, TILE_ERROR
},

{
    MONS_PRINCESS_SLIME, 'J', LIGHTMAGENTA, "princess slime",
    M_SEE_INVIS | M_UNBLINDABLE | M_ACID_SPLASH | M_EAT_DOORS | M_NO_POLY_TO,
    MR_RES_POISON | mrd(MR_RES_ACID, 3) | MR_RES_PIERCING,
    20, MONS_JELLY, MONS_PRINCESS_SLIME, MH_NATURAL, 180,
    { {AT_HIT, AF_ACID, 30}, {AT_HIT, AF_TRAMPLE, 20}, AT_NO_ATK, AT_NO_ATK },
    15, 680,
    6, 21, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_SLIME, 14, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_LARGE, MON_SHAPE_BLOB,
    {TILEP_MONS_PRINCESS_SLIME}, TILE_ERROR
},

{
    MONS_SLIME_REMNANT, 'J', WHITE, "remnant of a dead god",
    M_SEE_INVIS | M_UNBLINDABLE | M_EAT_DOORS | M_NO_POLY_TO,
    MR_RES_POISON | mrd(MR_RES_ACID, 3) | mrd(MR_RES_NEG, 3)
     | MR_RES_PIERCING,
    20, MONS_JELLY, MONS_SLIME_REMNANT, MH_ELEMENTAL, 180,
    { {AT_HIT, AF_PLAIN, 65}, {AT_HIT, AF_PLAIN, 35}, 
      {AT_HIT, AF_TRAMPLE, 10}, { AT_HIT, AF_TRAMPLE, 10 } },
    2, 1240,
    14, 1, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 12, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_SMALL, MON_SHAPE_BLOB,
    {TILEP_MONS_SLIME_REMNANT}, TILE_ERROR
},

{
    MONS_ANCIENT_ZYME, 'J', LIGHTGREY, "ancient zyme",
    M_SEE_INVIS | M_FLIES,
    MR_VUL_ACID | MR_RES_PIERCING | MR_RES_SLASHING,
    8, MONS_ANCIENT_ZYME, MONS_ANCIENT_ZYME, MH_ELEMENTAL, 60,
    { { AT_HIT, AF_PLAIN, 16 },{ AT_HIT, AF_PLAIN, 16 }, AT_NO_ATK, AT_NO_ATK },
    8, 520,
    6, 6, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_MEDIUM, MON_SHAPE_BLOB,
    { TILEP_MONS_ANCIENT_ZYME }, TILE_ERROR
},

// kobolds ('K')
{
    MONS_KOBOLD, 'K', BROWN, "kobold",
    M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_KOBOLD, MONS_KOBOLD, MH_NATURAL, 0,
    { {AT_HIT, AF_PLAIN, 4}, {AT_SHIELD, AF_PLAIN, 4}, AT_NO_ATK, AT_NO_ATK },
    1, 35,
    2, 7, MST_NO_SPELLS, CE_NOXIOUS, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_WEAPON_RANGED | MU_CONSUMABLES | MU_THROW_ROCK | MU_THROW_BLOWGUN),
        SIZE_SMALL, MON_SHAPE_HUMANOID,
    {TILEP_MONS_KOBOLD}, TILE_CORPSE_KOBOLD
},

{
    MONS_BIG_KOBOLD, 'K', YELLOW, "big kobold",
    M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_VUL_SLASHING,
    7, MONS_KOBOLD, MONS_BIG_KOBOLD, MH_NATURAL, 20,
    { {AT_HIT, AF_PLAIN, 7}, {AT_SHIELD, AF_PLAIN, 7}, AT_NO_ATK, AT_NO_ATK },
    5, 275,
    3, 6, MST_NO_SPELLS, CE_NOXIOUS, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_WEAPON_RANGED | MU_CONSUMABLES | MU_THROW_ROCK | MU_THROW_BLOWGUN),
        SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_BIG_KOBOLD}, TILE_CORPSE_BIG_KOBOLD
},

{
    MONS_KOBOLD_DEMONOLOGIST, 'K', MAGENTA, "kobold demonologist",
    M_WARM_BLOOD | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    MR_VUL_SLASHING,
    18, MONS_KOBOLD, MONS_KOBOLD, MH_NATURAL | MH_EVIL, 40,
    { {AT_HIT, AF_PLAIN, 4}, {AT_SHIELD, AF_PLAIN, 4}, AT_NO_ATK, AT_NO_ATK },
    6, 390,
    2, 17, MST_KOBOLD_DEMONOLOGIST, CE_NOXIOUS, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_SMALL, MON_SHAPE_HUMANOID,
    {TILEP_MONS_KOBOLD_DEMONOLOGIST}, TILE_ERROR
},

// liches ('L')
{
    MONS_LICH, 'L', LIGHTGREY, "lich",
    M_SEE_INVIS | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    mrd(MR_RES_COLD, 2) | MR_VUL_ACID | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    18, MONS_LICH, MONS_LICH, MH_UNDEAD, 160,
    { {AT_TOUCH, AF_DRAIN_XP, 15}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    20, 800,
    10, 17, MST_NO_SPELLS, CE_NOCORPSE, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_ARMOUR | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_LICH, TVARY_XMAS}, TILE_ERROR
},

{
    MONS_ANCIENT_LICH, 'L', WHITE, "ancient lich",
    M_SEE_INVIS | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    mrd(MR_RES_COLD, 2) | MR_RES_FIRE | MR_RES_ELEC | MR_VUL_ACID
     | MR_RES_PIERCING | MR_RES_SLASHING,
    24, MONS_LICH, MONS_LICH, MH_UNDEAD, 180,
    { {AT_TOUCH, AF_DRAIN_XP, 20}, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    27, 1080,
    20, 23, MST_NO_SPELLS, CE_NOCORPSE, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_ARMOUR | MU_EVOKE | MU_SHIELD), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ANCIENT_LICH, TVARY_XMAS}, TILE_ERROR
},

{
    MONS_REVENANT, 'L', CYAN, "revenant",
    M_SEE_INVIS | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    mrd(MR_RES_COLD, 2) | MR_VUL_ACID | MR_RES_PIERCING
     | MR_VUL_BLUDGEONING,
    18, MONS_REVENANT, MONS_REVENANT, MH_UNDEAD, 180,
    { {AT_HIT, AF_PLAIN, 26}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    18, 810,
    8, 23, MST_REVENANT, CE_NOCORPSE, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_REVENANT}, TILE_ERROR
},

{
    MONS_HALAZID_WARLOCK, 'L', GREEN, "halazid warlock",
    M_SEE_INVIS | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    mrd(MR_RES_COLD, 2) | MR_VUL_ACID | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    10, MONS_LICH, MONS_LICH, MH_UNDEAD, 100,
    { {AT_TOUCH, AF_PAIN, 20}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    18, 750,
    8, 18, MST_HALAZID_WARLOCK, CE_NOCORPSE, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_HALAZID_WARLOCK}, TILE_ERROR
},

// mummies ('M')
{
    MONS_MUMMY, 'M', LIGHTGREY, "mummy",
    M_NO_ZOMBIE | M_GENDERED,
    MR_VUL_FIRE | MR_RES_COLD | MR_RES_PIERCING | MR_VUL_SLASHING,
    21, MONS_MUMMY, MONS_MUMMY, MH_UNDEAD, 20,
    { {AT_HIT, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    3, 195,
    3, 6, MST_NO_SPELLS, CE_NOXIOUS, S_SILENT,
    I_HUMAN, HT_LAND, 6, DEFAULT_ENERGY,
    MU_DOOR, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_MUMMY}, TILE_CORPSE_MUMMY
},

{
    MONS_GUARDIAN_MUMMY, 'M', YELLOW, "guardian mummy",
    M_FIGHTER | M_SEE_INVIS | M_NO_ZOMBIE | M_GENDERED,
    MR_RES_COLD | MR_RES_PIERCING,
    13, MONS_MUMMY, MONS_MUMMY, MH_UNDEAD, 40,
    { {AT_HIT, AF_PLAIN, 30}, {AT_SHIELD, AF_PLAIN, 30}, AT_NO_ATK, AT_NO_ATK },
    7, 455,
    6, 9, MST_NO_SPELLS, CE_NOXIOUS, S_SILENT,
    I_HUMAN, HT_LAND, 8, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_WAND | MU_THROW_NET), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_GUARDIAN_MUMMY}, TILE_CORPSE_MUMMY
},

{
    MONS_GREATER_MUMMY, 'M', WHITE, "greater mummy",
    M_SEE_INVIS | M_SPEAKS | M_SPELLCASTER | M_NO_ZOMBIE | M_GENDERED,
    MR_RES_COLD | MR_RES_ELEC | MR_RES_ACID | MR_RES_PIERCING | MR_RES_SLASHING,
    24, MONS_MUMMY, MONS_MUMMY, MH_UNDEAD, 160,
    { {AT_HIT, AF_PLAIN, 35}, {AT_SHIELD, AF_PLAIN, 35}, AT_NO_ATK, AT_NO_ATK },
    15, 1950,
    10, 6, MST_NO_SPELLS, CE_NOXIOUS, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_GREATER_MUMMY}, TILE_CORPSE_MUMMY
},

{
    MONS_MUMMY_PRIEST, 'M', RED, "mummy priest",
    M_SEE_INVIS | M_SPEAKS | M_SPELLCASTER | M_NO_ZOMBIE | M_GENDERED,
    MR_RES_COLD | MR_RES_ELEC | MR_RES_PIERCING | MR_VUL_SLASHING,
    20, MONS_MUMMY, MONS_MUMMY, MH_UNDEAD, 120,
    { {AT_HIT, AF_PLAIN, 30}, {AT_SHIELD, AF_PLAIN, 30}, AT_NO_ATK, AT_NO_ATK },
    10, 650,
    8, 7, MST_NO_SPELLS, CE_NOXIOUS, S_SILENT,
    I_HUMAN, HT_LAND, 8, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_MUMMY_PRIEST}, TILE_CORPSE_MUMMY
},

// nagas ('N')
{
    MONS_NAGA, 'N', GREEN, "naga",
    M_SEE_INVIS | M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_RES_POISON,
    13, MONS_NAGA, MONS_NAGA, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 17}, {AT_SHIELD, AF_PLAIN, 17},
      {AT_CONSTRICT, AF_CRUSH, 3}, AT_NO_ATK },
    5, 275,
    6, 10, MST_NAGA, CE_NOXIOUS, S_SHOUT,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(14),
    MU_ALL, SIZE_LARGE, MON_SHAPE_NAGA,
    {TILEP_MONS_NAGA}, TILE_CORPSE_NAGA
},

{
    MONS_NAGA_MAGE, 'N', MAGENTA, "naga mage",
    M_SEE_INVIS | M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_RES_POISON,
    14, MONS_NAGA, MONS_NAGA, MH_NATURAL, 60,
    { {AT_HIT, AF_PLAIN, 17}, {AT_SHIELD, AF_PLAIN, 17},
      {AT_CONSTRICT, AF_CRUSH, 4}, AT_NO_ATK },
    7, 385,
    6, 17, MST_NAGA_MAGE, CE_NOXIOUS, S_SHOUT,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(14),
    muf(MU_MELEE | MU_EVOKE), SIZE_LARGE, MON_SHAPE_NAGA,
    {TILEP_MONS_NAGA_MAGE}, TILE_ERROR
},

{
    MONS_NAGA_SHARPSHOOTER, 'N', LIGHTGRAY, "naga sharpshooter",
    M_SEE_INVIS | M_WARM_BLOOD
        | M_ARCHER | M_PREFER_RANGED | M_SPEAKS | M_GENDERED,
    MR_RES_POISON,
    14, MONS_NAGA, MONS_NAGA, MH_NATURAL, 80,
    { {AT_HIT, AF_PLAIN, 17}, {AT_SHIELD, AF_PLAIN, 17}, 
      {AT_CONSTRICT, AF_CRUSH, 5}, AT_NO_ATK },
    9, 720,
    6, 22, MST_NAGA_SHARPSHOOTER, CE_NOXIOUS, S_SHOUT,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(14),
    MU_ALL, SIZE_LARGE, MON_SHAPE_NAGA,
    {TILEP_MONS_NAGA_SHARPSHOOTER}, TILE_ERROR
},

{
    MONS_NAGA_RITUALIST, 'N', BROWN, "naga ritualist",
    M_SEE_INVIS | M_WARM_BLOOD | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    MR_RES_POISON,
    14, MONS_NAGA, MONS_NAGA, MH_NATURAL, 60,
    { {AT_HIT, AF_PLAIN, 14}, {AT_SHIELD, AF_PLAIN, 14}, 
      {AT_CONSTRICT, AF_CRUSH, 4}, AT_NO_ATK },
    8, 600,
    6, 17, MST_NAGA_RITUALIST, CE_NOXIOUS, S_SHOUT,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(14),
    muf(MU_MELEE | MU_EVOKE), SIZE_LARGE, MON_SHAPE_NAGA,
    {TILEP_MONS_NAGA_RITUALIST}, TILE_ERROR
},

{
    MONS_NAGA_WARRIOR, 'N', BLUE, "naga warrior",
    M_FIGHTER | M_SEE_INVIS | M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_RES_POISON,
    11, MONS_NAGA, MONS_NAGA, MH_NATURAL, 80,
    { {AT_HIT, AF_PLAIN, 28}, {AT_SHIELD, AF_PLAIN, 28}, 
      {AT_CONSTRICT, AF_CRUSH, 6}, AT_NO_ATK },
    10, 1000,
    6, 24, MST_NAGA, CE_NOXIOUS, S_SHOUT,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(14),
    muf(MU_MELEE | MU_EVOKE), SIZE_LARGE, MON_SHAPE_NAGA,
    {TILEP_MONS_NAGA_WARRIOR}, TILE_ERROR
},

{
    MONS_NAGARAJA, 'N', LIGHTMAGENTA, "nagaraja",
    M_FIGHTER | M_SEE_INVIS | M_WARM_BLOOD | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    MR_RES_POISON | MR_RES_SLASHING,
    15, MONS_NAGA, MONS_NAGA, MH_NATURAL, 140,
    { {AT_HIT, AF_PLAIN, 27}, {AT_SHIELD, AF_PLAIN, 27}, 
      {AT_CONSTRICT, AF_CRUSH, 7}, AT_NO_ATK },
    15, 825,
    6, 28, MST_NAGA_MAGE, CE_NOXIOUS, S_SHOUT,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(14),
    MU_ALL, SIZE_LARGE, MON_SHAPE_NAGA,
    {TILEP_MONS_NAGARAJA}, TILE_ERROR
},

// ogres ('O')
{
    MONS_OGRE, 'O', BROWN, "ogre",
    M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_VUL_SLASHING,
    9, MONS_OGRE, MONS_OGRE, MH_NATURAL, 20,
    { {AT_HIT, AF_PLAIN, 17}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    5, 275,
    1, 6, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_WEAPON_MELEE | MU_ARMOUR | MU_DOOR), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_OGRE}, TILE_CORPSE_OGRE
},

// These guys get understated because the experience code can't see
// that they wield two weapons... I'm raising their xp modifier. - bwr
{
    MONS_TWO_HEADED_OGRE, 'O', LIGHTRED, "two-headed ogre",
    M_WARM_BLOOD | M_TWO_WEAPONS | M_SPEAKS | M_GENDERED,
    MR_VUL_SLASHING,
    15, MONS_OGRE, MONS_TWO_HEADED_OGRE, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 20}, {AT_HIT, AF_PLAIN, 15}, AT_NO_ATK, AT_NO_ATK },
    10, 550,
    3, 6, MST_NO_SPELLS, CE_CLEAN, S_SHOUT2,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_WEAPON_MELEE | MU_DOOR), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_TWO_HEADED_OGRE}, TILE_CORPSE_TWO_HEADED_OGRE
},

{
    MONS_OGRE_MAGE, 'O', MAGENTA, "ogre mage",
    M_SEE_INVIS | M_WARM_BLOOD | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    MR_VUL_SLASHING,
    16, MONS_OGRE, MONS_OGRE, MH_NATURAL | MH_EVIL, 80,
    { {AT_HIT, AF_PLAIN, 12}, {AT_SHIELD, AF_PLAIN, 12}, AT_NO_ATK, AT_NO_ATK },
    10, 550,
    1, 7, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_OGRE_MAGE}, TILE_ERROR
},

// immobile plants and fungi ('P')
{
    MONS_PLANT, 'P', GREEN, "plant",
    M_STATIONARY | M_NOT_DANGEROUS,
    MR_RES_POISON | MR_VUL_ACID | MR_VUL_SLASHING 
     | MR_RES_PIERCING | MR_RES_BLUDGEONING,
    10, MONS_PLANT, MONS_PLANT, MH_PLANT, MAG_IMMUNE,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    10, 550,
    10, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 0, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_PLANT,
    {TILEP_MONS_PLANT, TVARY_MOD}, TILE_ERROR
},

{
    MONS_DEMONIC_PLANT, 'P', ETC_RANDOM, "demonic plant",
    M_STATIONARY | M_NOT_DANGEROUS,
    MR_RES_POISON | MR_VUL_ACID | MR_VUL_SLASHING
     | MR_RES_PIERCING | MR_RES_BLUDGEONING,
    10, MONS_PLANT, MONS_DEMONIC_PLANT, MH_PLANT, MAG_IMMUNE,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    10, 550,
    10, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 0, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_PLANT,
    {TILEP_MONS_DEMONIC_PLANT}, TILE_ERROR
},

{
    MONS_WITHERED_PLANT, 'P', DARKGREY, "withered plant",
    M_STATIONARY | M_NOT_DANGEROUS,
    mrd(MR_RES_POISON, 3) | MR_VUL_ACID | MR_VUL_SLASHING
     | MR_RES_PIERCING | MR_RES_BLUDGEONING,
    10, MONS_PLANT, MONS_WITHERED_PLANT, MH_PLANT, MAG_IMMUNE,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    10, 550,
    10, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 0, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_PLANT,
    {TILEP_MONS_WITHERED_PLANT}, TILE_ERROR
},

{
    MONS_OKLOB_SAPLING, 'P', LIGHTCYAN, "oklob sapling",
    M_STATIONARY,
    MR_RES_POISON | mrd(MR_RES_ACID, 3) | MR_VUL_SLASHING
     | MR_RES_PIERCING | MR_RES_BLUDGEONING,
    10, MONS_OKLOB_PLANT, MONS_OKLOB_PLANT, MH_PLANT, 20,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    4, 220,
    10, 0, MST_OKLOB_SAPLING, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_SLIME, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_PLANT,
    {TILEP_MONS_OKLOB_SAPLING}, TILE_ERROR
},

{
    MONS_OKLOB_PLANT, 'P', LIGHTGREEN, "oklob plant",
    M_STATIONARY,
    MR_RES_POISON | mrd(MR_RES_ACID, 3) | MR_VUL_SLASHING
     | MR_RES_PIERCING | MR_RES_BLUDGEONING,
    10, MONS_OKLOB_PLANT, MONS_OKLOB_PLANT, MH_PLANT, 40,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    10, 550,
    10, 0, MST_ACID_SPIT, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_SLIME, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_PLANT,
    {TILEP_MONS_OKLOB_PLANT}, TILE_ERROR
},

{
    MONS_BUSH, 'P', BROWN, "bush",
    M_STATIONARY | M_NOT_DANGEROUS,
    MR_RES_POISON | MR_VUL_FIRE | MR_VUL_ACID | MR_VUL_SLASHING
     | MR_RES_PIERCING | MR_RES_BLUDGEONING,
    10, MONS_PLANT, MONS_BUSH, MH_PLANT, MAG_IMMUNE,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    20, 1100,
    15, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 0, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_BIG, MON_SHAPE_PLANT,
    {TILEP_MONS_BUSH, TVARY_MOD}, TILE_ERROR
},

{
    MONS_BURNING_BUSH, 'P', RED, "burning bush",
    M_STATIONARY | M_SEE_INVIS,
    MR_RES_POISON | MR_RES_FIRE | MR_VUL_ACID | MR_VUL_SLASHING
     | MR_RES_PIERCING | MR_RES_BLUDGEONING,
    10, MONS_PLANT, MONS_BUSH, MH_PLANT, 40,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    8, 440,
    10, 0, MST_BURNING_BUSH, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_BIG, MON_SHAPE_PLANT,
    {TILEP_MONS_BUSH_BURNING}, TILE_ERROR
},

{
    MONS_TOADSTOOL, 'P', COLOUR_UNDEF, "toadstool",
    M_NOT_DANGEROUS | M_STATIONARY,
    MR_RES_POISON | MR_VUL_ACID | MR_VUL_SLASHING
     | MR_RES_PIERCING | MR_RES_BLUDGEONING,
    10, MONS_FUNGUS, MONS_TOADSTOOL, MH_PLANT, MAG_IMMUNE,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 30,
    1, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 0, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_FUNGUS,
    {TILEP_MONS_TOADSTOOL, TVARY_MOD}, TILE_ERROR
},

{
    MONS_FUNGUS, 'P', LIGHTGREY, "fungus",
    M_NOT_DANGEROUS | M_STATIONARY,
    MR_RES_POISON | MR_VUL_ACID | MR_VUL_SLASHING
     | MR_RES_PIERCING | MR_RES_BLUDGEONING,
    10, MONS_FUNGUS, MONS_FUNGUS, MH_PLANT, MAG_IMMUNE,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    8, 440,
    1, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 0, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_FUNGUS,
    {TILEP_MONS_FUNGUS, TVARY_MOD}, TILE_ERROR
},

{
    MONS_BALLISTOMYCETE, 'P', MAGENTA, "ballistomycete",
    M_NOT_DANGEROUS | M_STATIONARY,
    MR_RES_POISON | MR_VUL_ACID 
     | MR_RES_PIERCING | MR_RES_BLUDGEONING,
    10, MONS_FUNGUS, MONS_BALLISTOMYCETE, MH_PLANT, MAG_IMMUNE,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    4, 260,
    1, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 0, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_FUNGUS,
    {TILEP_MONS_BALLISTOMYCETE_INACTIVE}, TILE_ERROR
},

{
    MONS_HYPERACTIVE_BALLISTOMYCETE, 'P', LIGHTRED, "hyperactive ballistomycete",
    M_NO_EXP_GAIN | M_STATIONARY | M_NO_POLY_TO,
    MR_RES_POISON | MR_VUL_ACID | MR_RES_PIERCING | MR_RES_BLUDGEONING,
    10, MONS_FUNGUS, MONS_BALLISTOMYCETE, MH_PLANT, MAG_IMMUNE,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    // expected explosion damage: 25, expected HP: 60
    6, 600,
    1, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 0, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_FUNGUS,
    {TILEP_MONS_HYPERACTIVE_BALLISTOMYCETE}, TILE_ERROR
},

{
    MONS_BRIAR_PATCH, 'P', YELLOW, "briar patch",
    M_STATIONARY | M_NOT_DANGEROUS | M_SPINY,
    MR_RES_POISON | MR_VUL_FIRE | MR_VUL_ACID
     | MR_VUL_SLASHING | MR_RES_PIERCING | MR_RES_BLUDGEONING,
    10, MONS_PLANT, MONS_BRIAR_PATCH, MH_PLANT, MAG_IMMUNE,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    10, 150,
    10, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_AMPHIBIOUS, 0, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_PLANT,
    {TILEP_MONS_BRIAR_PATCH}, TILE_ERROR
},

// tengu ('Q')
{
    MONS_TENGU, 'Q', LIGHTBLUE, "tengu",
    M_WARM_BLOOD | M_SPEAKS | M_FLIES | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_TENGU, MONS_TENGU, MH_NATURAL, 20,
    { {AT_HIT, AF_PLAIN, 10}, {AT_SHIELD, AF_PLAIN, 10},
      {AT_PECK, AF_PLAIN, 5}, {AT_CLAW, AF_PLAIN, 5} },
    5, 225,
    2, 18, MST_NO_SPELLS, CE_CLEAN /*chicken*/, S_SHOUT,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(9),
    MU_MELEE, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_TENGU}, TILE_CORPSE_TENGU
},

{
    MONS_TENGU_CONJURER, 'Q', BLUE, "tengu conjurer",
    M_SEE_INVIS | M_WARM_BLOOD | M_SPEAKS | M_FLIES | M_GENDERED,
    MR_VUL_SLASHING,
    13, MONS_TENGU, MONS_TENGU, MH_NATURAL, 20,
    { {AT_HIT, AF_PLAIN, 10}, {AT_SHIELD, AF_PLAIN, 10},
      {AT_PECK, AF_PLAIN, 5}, {AT_CLAW, AF_PLAIN, 5} },
    7, 315,
    2, 28, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(9),
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_TENGU_CONJURER}, TILE_ERROR
},

{
    MONS_TENGU_WARRIOR, 'Q', CYAN, "tengu warrior",
    M_FIGHTER | M_ARCHER | M_WARM_BLOOD | M_SPEAKS | M_FLIES | M_GENDERED,
    MR_VUL_SLASHING,
    13, MONS_TENGU, MONS_TENGU, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 16}, {AT_SHIELD, AF_PLAIN, 16}, 
      {AT_PECK, AF_PLAIN, 8}, {AT_CLAW, AF_PLAIN, 8} },
    10, 600,
    2, 31, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(9),
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_TENGU_WARRIOR}, TILE_ERROR
},

{
    MONS_TENGU_REAVER, 'Q', LIGHTMAGENTA, "tengu reaver",
    M_FIGHTER | M_SEE_INVIS | M_WARM_BLOOD | M_SPEAKS | M_FLIES | M_SPELLCASTER | M_GENDERED,
    MR_RES_SLASHING | MR_VUL_PIERCING,
    13, MONS_TENGU, MONS_TENGU, MH_NATURAL, 60,
    { {AT_HIT, AF_PLAIN, 27}, {AT_SHIELD, AF_PLAIN, 27}, 
      {AT_PECK, AF_PLAIN, 11}, {AT_CLAW, AF_PLAIN, 11} },
    17, 850,
    2, 33, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(9),
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_TENGU_REAVER}, TILE_ERROR
},

// spiritual beings ('R')
{
    MONS_EFREET, 'R', RED, "efreet",
    M_SPEAKS | M_FLIES | M_GENDERED,
    MR_RES_POISON | mrd(MR_RES_FIRE, 4) | MR_VUL_COLD | MR_VUL_WATER | MR_RES_BLUDGEONING,
    12, MONS_EFREET, MONS_EFREET, MH_ELEMENTAL, 20,
    { {AT_HIT, AF_PLAIN, 17}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    7, 385,
    10, 13, MST_EFREET, CE_DIVINE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_WEAPONS | MU_DOOR | MU_EVOKE), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_EFREET}, TILE_CORPSE_EFREET
},

{
    MONS_REAPER, 'R', LIGHTGREY, "reaper",
    M_FIGHTER | M_SEE_INVIS | M_SPEAKS,
    MR_RES_POISON | MR_RES_COLD | mrd(MR_RES_NEG, 4) 
     | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    14, MONS_REAPER, MONS_REAPER, MH_ELEMENTAL, 100,
    { {AT_HIT, AF_PLAIN, 45}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    14, 770,
    15, 24, MST_VAULT_WARDEN, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_STEEL, 10, DEFAULT_ENERGY,
    muf(MU_START_ONLY | MU_ALL), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_REAPER}, TILE_CORPSE_REAPER
},

{
    MONS_RAKSHASA, 'R', YELLOW, "rakshasa",
    M_SEE_INVIS | M_GENDERED,
    MR_RES_POISON | MR_RES_BLUDGEONING,
    15, MONS_RAKSHASA, MONS_RAKSHASA, MH_DEMONIC, 140,
    { {AT_HIT, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    11, 660,
    6, 21, MST_RAKSHASA, CE_DIVINE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_WEAPONS | MU_DOOR), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_RAKSHASA}, TILE_CORPSE_RAKSHASA
},

{
    MONS_DRYAD, 'R', LIGHTGREEN, "kael",
    M_WARM_BLOOD | M_SPEAKS | M_NO_POLY_TO | M_NO_GEN_DERIVED | M_STATIONARY | M_GENDERED,
    MR_VUL_FIRE | MR_RES_ACID | MR_RES_BLUDGEONING | MR_RES_PIERCING,
    10, MONS_DRYAD, MONS_DRYAD, MH_PLANT, 80,
    { {AT_RAKE, AF_PLAIN, 35}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    16, 1266,
    18, 1, MST_DRYAD, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 8, DEFAULT_ENERGY,
    MU_DOOR, SIZE_GIANT, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DRYAD}, TILE_CORPSE_DRYAD
},

{
    MONS_ELEIONOMA, 'R', GREEN, "eleionoma",
    M_WARM_BLOOD | M_SPEAKS,
    MR_NO_FLAGS,
    10, MONS_ELEIONOMA, MONS_ELEIONOMA, MH_NATURAL, 100,
    { {AT_HIT, AF_PLAIN, 25}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    16, 800,
    2, 10, MST_ELEIONOMA, true, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MONUSE_NOTHING, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ELEIONOMA}, TILE_CORPSE_ELEIONOMA
},

{
    MONS_SNAPLASHER_VINE, 'w', LIGHTGREEN, "snaplasher vine",
    M_NO_EXP_GAIN | M_STATIONARY | M_NO_POLY_TO,
    MR_RES_POISON | MR_VUL_ACID | MR_VUL_SLASHING | MR_RES_PIERCING,
    10, MONS_SNAPLASHER_VINE, MONS_SNAPLASHER_VINE, MH_PLANT, 40,
    { {AT_CONSTRICT, AF_CRUSH, 0}, {AT_HIT, AF_PLAIN, 14}, AT_NO_ATK, AT_NO_ATK },
    12, 240,
    4, 7, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 13, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_SNAKE,
    {TILEP_MONS_PROGRAM_BUG}, TILE_ERROR
},

{
    MONS_SNAPLASHER_VINE_SEGMENT, '*', LIGHTGREEN, "snaplasher vine segment",
    M_NOT_DANGEROUS | M_STATIONARY | M_NO_POLY_TO,
    MR_RES_POISON | MR_VUL_ACID | MR_VUL_SLASHING | MR_RES_PIERCING,
    10, MONS_SNAPLASHER_VINE, MONS_SNAPLASHER_VINE, MH_PLANT, 40,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    12, 360,
    6, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_MISC,
    {TILEP_MONS_PROGRAM_BUG}, TILE_ERROR
},

// snakes ('S')
DUMMY(MONS_SNAKE, 'S', LIGHTGREEN, "snake", TILEP_MONS_ADDER)

{
    MONS_BALL_PYTHON, 'S', GREEN, "ball python",
    M_COLD_BLOOD,
    MR_VUL_SLASHING | MR_RES_PIERCING,
    13, MONS_SNAKE, MONS_BALL_PYTHON, MH_NATURAL, 0,
    { {AT_BITE, AF_PLAIN, 2}, {AT_CONSTRICT, AF_CRUSH, 1},
       AT_NO_ATK, AT_NO_ATK },
    1, 35,
    0, 5, MST_NO_SPELLS, CE_CLEAN, S_HISS,
    I_ANIMAL, HT_AMPHIBIOUS, 12, SWIM_ENERGY(6),
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_SNAKE,
    {TILEP_MONS_BALL_PYTHON}, TILE_CORPSE_BALL_PYTHON
},

{
    MONS_ADDER, 'S', LIGHTGREEN, "adder",
    M_COLD_BLOOD,
    MR_VUL_SLASHING | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    10, MONS_SNAKE, MONS_ADDER, MH_NATURAL, 10,
    { {AT_BITE, AF_POISON, 5}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    2, 110,
    1, 9, MST_NO_SPELLS, CE_CLEAN, S_HISS,
    I_ANIMAL, HT_AMPHIBIOUS, 13, SWIM_ENERGY(6),
    MU_NOTHING, SIZE_LITTLE, MON_SHAPE_SNAKE,
    {TILEP_MONS_ADDER}, TILE_CORPSE_ADDER
},

{
    MONS_WATER_MOCCASIN, 'S', BROWN, "water moccasin",
    M_COLD_BLOOD,
    MR_RES_POISON | MR_VUL_SLASHING | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    11, MONS_SNAKE, MONS_WATER_MOCCASIN, MH_NATURAL, 20,
    { {AT_BITE, AF_POISON, 10}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    5, 275,
    2, 11, MST_NO_SPELLS, CE_NOXIOUS, S_HISS,
    I_ANIMAL, HT_AMPHIBIOUS, 14, SWIM_ENERGY(6),
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_SNAKE,
    {TILEP_MONS_WATER_MOCCASIN}, TILE_CORPSE_WATER_MOCCASIN
},

{
    MONS_BLACK_MAMBA, 'S', BLUE, "black mamba",
    M_COLD_BLOOD,
    MR_RES_POISON | MR_VUL_SLASHING | MR_RES_PIERCING,
    12, MONS_SNAKE, MONS_BLACK_MAMBA, MH_NATURAL, 20,
    { {AT_BITE, AF_POISON, 20}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    7, 385,
    4, 19, MST_NO_SPELLS, CE_NOXIOUS, S_HISS,
    I_ANIMAL, HT_LAND, 18, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_SNAKE,
    {TILEP_MONS_BLACK_MAMBA}, TILE_CORPSE_BLACK_MAMBA
},

{
    MONS_SEA_SNAKE, 'S', LIGHTCYAN, "sea snake",
    M_COLD_BLOOD,
    MR_VUL_SLASHING | MR_RES_PIERCING,
    10, MONS_SNAKE, MONS_SEA_SNAKE, MH_NATURAL, 40,
    { {AT_BITE, AF_POISON_STRONG, 24}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    10, 650,
    2, 22, MST_NO_SPELLS, CE_NOXIOUS, S_HISS,
    I_ANIMAL, HT_AMPHIBIOUS, 12, SWIM_ENERGY(4),
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_SNAKE,
    {TILEP_MONS_SEA_SNAKE}, TILE_CORPSE_SEA_SNAKE
},

{
    MONS_ANACONDA, 'S', LIGHTGREY, "anaconda",
    M_COLD_BLOOD,
    MR_RES_PIERCING,
    10, MONS_SNAKE, MONS_ANACONDA, MH_NATURAL, 40,
    { {AT_CONSTRICT, AF_CRUSH, 6}, {AT_BITE, AF_PLAIN, 20},
       AT_NO_ATK, AT_NO_ATK },
    11, 605,
    4, 31, MST_NO_SPELLS, CE_CLEAN, S_HISS,
    I_ANIMAL, HT_AMPHIBIOUS, 18, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_SNAKE,
    {TILEP_MONS_ANACONDA}, TILE_CORPSE_ANACONDA
},

{
    MONS_GUARDIAN_SERPENT, 'S', WHITE, "guardian serpent",
    M_SEE_INVIS | M_WARM_BLOOD | M_SPEAKS | M_SPELLCASTER,
    MR_RES_POISON,
    10, MONS_GUARDIAN_SERPENT, MONS_GUARDIAN_SERPENT, MH_NATURAL, 60,
    { {AT_HIT, AF_PLAIN, 26}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    8, 440,
    6, 21, MST_GUARDIAN_SERPENT, CE_NOXIOUS, S_SHOUT,
    I_HUMAN, HT_LAND, 15, DEFAULT_ENERGY,
    MU_WAND, SIZE_LARGE, MON_SHAPE_SNAKE,
    {TILEP_MONS_GUARDIAN_SERPENT}, TILE_CORPSE_GUARDIAN_SERPENT
},

{
    MONS_SHOCK_SERPENT, 'S', LIGHTBLUE, "shock serpent",
    M_COLD_BLOOD,
    mrd(MR_RES_ELEC, 3) | MR_VUL_SLASHING | MR_RES_PIERCING,
    12, MONS_SNAKE, MONS_SHOCK_SERPENT, MH_NATURAL, 40,
    { {AT_BITE, AF_ELEC, 20}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    10, 550,
    2, 23, MST_SHOCK_SERPENT, CE_CLEAN, S_HISS,
    I_ANIMAL, HT_LAND, 15, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_SNAKE,
    {TILEP_MONS_SHOCK_SERPENT}, TILE_CORPSE_SHOCK_SERPENT
},

{
    MONS_MANA_VIPER, 'S', MAGENTA, "mana viper",
    M_COLD_BLOOD | M_SEE_INVIS,
    MR_RES_POISON | MR_VUL_SLASHING | MR_RES_PIERCING,
    10, MONS_SNAKE, MONS_MANA_VIPER, MH_NATURAL, 100,
    { {AT_BITE, AF_ANTIMAGIC, 23}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    9, 495,
    3, 26, MST_NO_SPELLS, CE_NOCORPSE, S_HISS,
    I_ANIMAL, HT_LAND, 14, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_SNAKE,
    {TILEP_MONS_MANA_VIPER}, TILE_ERROR
},

// trolls ('T')
{
    MONS_TROLL, 'T', GREEN, "troll",
    M_WARM_BLOOD | M_SPEAKS | M_FAST_REGEN | M_GENDERED,
    MR_VUL_ACID | MR_RES_BLUDGEONING,
    10, MONS_TROLL, MONS_TROLL, MH_NATURAL, 20,
    { {AT_BITE, AF_PLAIN, 20}, {AT_CLAW, AF_PLAIN, 15},
      {AT_CLAW, AF_PLAIN, 15}, {AT_SHIELD, AF_PLAIN, 15} },
    7, 385,
    5, 10, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_CONSUMABLES | MU_ARMOUR | MU_SHIELD | MU_WEAPON_RANGED), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_TROLL}, TILE_CORPSE_TROLL
},

// dummy, though it's rude to say so
{
    MONS_ROCK_TROLL, 'T', BROWN, "rock troll",
    M_WARM_BLOOD | M_SPEAKS | M_FAST_REGEN | M_NO_POLY_TO | M_GENDERED,
    MR_VUL_ACID | MR_RES_BLUDGEONING,
    10, MONS_TROLL, MONS_ROCK_TROLL, MH_NATURAL, 10,
    { { AT_HIT, AF_PLAIN, 18 },{ AT_TAIL_SLAP, AF_PLAIN, 12 }, AT_NO_ATK,
    AT_NO_ATK },
    8, 450,
    15, 5, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_CONSUMABLES | MU_ARMOUR | MU_SHIELD | MU_WEAPON_RANGED), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    { TILEP_MONS_TROLL }, TILE_CORPSE_TROLL
},

{
    MONS_IRON_TROLL, 'T', CYAN, "iron troll",
    M_WARM_BLOOD | M_SPEAKS | M_FAST_REGEN | M_GENDERED,
    MR_RES_FIRE | MR_RES_COLD | MR_VUL_ACID | MR_RES_BLUDGEONING,
    10, MONS_TROLL, MONS_IRON_TROLL, MH_NATURAL, 100,
    { {AT_BITE, AF_PLAIN, 35}, {AT_CLAW, AF_PLAIN, 25},
      {AT_CLAW, AF_PLAIN, 25}, {AT_SHIELD, AF_PLAIN, 25} },
    16, 880,
    20, 7, MST_NO_SPELLS, CE_NOXIOUS, S_SHOUT,
    I_HUMAN, HT_LAND, 7, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_EVOKE | MU_ARMOUR | MU_SHIELD | MU_WEAPON_RANGED), SIZE_BIG, MON_SHAPE_HUMANOID,
    {TILEP_MONS_IRON_TROLL}, TILE_CORPSE_IRON_TROLL
},

{
    MONS_DEEP_TROLL, 'T', YELLOW, "deep troll",
    M_WARM_BLOOD | M_SEE_INVIS | M_UNBLINDABLE | M_SPEAKS | M_FAST_REGEN | M_GENDERED,
    MR_VUL_ACID | MR_RES_BLUDGEONING,
    9, MONS_TROLL, MONS_DEEP_TROLL, MH_NATURAL, 40,
    { {AT_BITE, AF_PLAIN, 27}, {AT_CLAW, AF_PLAIN, 20},
      {AT_CLAW, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20} },
    10, 550,
    11, 22, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_EVOKE | MU_ARMOUR | MU_SHIELD | MU_WEAPON_RANGED), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DEEP_TROLL}, TILE_CORPSE_DEEP_TROLL
},

{
    MONS_DEEP_TROLL_EARTH_MAGE, 'T', MAGENTA, "deep troll earth mage",
    M_WARM_BLOOD | M_SEE_INVIS | M_UNBLINDABLE | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    MR_VUL_ACID | MR_RES_BLUDGEONING,
    10, MONS_TROLL, MONS_DEEP_TROLL, MH_NATURAL, 40,
    { {AT_BITE, AF_PLAIN, 27}, {AT_CLAW, AF_PLAIN, 20},
      {AT_CLAW, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20} },
    12, 480,
    // the extra AC is essentially a perma-stoneskin
    // BCADDO: Actual monster stoneskin?
    14, 22, MST_DEEP_TROLL_EARTH_MAGE, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_EVOKE | MU_ARMOUR | MU_SHIELD), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DEEP_TROLL_EARTH_MAGE}, TILE_ERROR
},

{
    MONS_DEEP_TROLL_SHAMAN, 'T', WHITE, "deep troll shaman",
    M_WARM_BLOOD | M_SEE_INVIS | M_UNBLINDABLE | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    MR_VUL_ACID | MR_RES_BLUDGEONING,
    12, MONS_TROLL, MONS_DEEP_TROLL, MH_NATURAL, 40,
    { {AT_BITE, AF_PLAIN, 27}, {AT_CLAW, AF_PLAIN, 20},
      {AT_CLAW, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20} },
    12, 480,
    11, 25, MST_DEEP_TROLL_SHAMAN, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_EVOKE | MU_ARMOUR | MU_SHIELD), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DEEP_TROLL_SHAMAN}, TILE_ERROR
},

// vampires ('V')
{
    MONS_VAMPIRE, 'V', RED, "vampire",
    M_SEE_INVIS | M_SPEAKS | M_BLOOD_SCENT | M_WARM_BLOOD | M_GENDERED,
    MR_RES_COLD | MR_VUL_PIERCING | MR_RES_BLUDGEONING,
    11, MONS_VAMPIRE, MONS_VAMPIRE, MH_UNDEAD, 40,
    { {AT_HIT, AF_PLAIN, 15}, {AT_SHIELD, AF_PLAIN, 15}, 
      {AT_BITE, AF_VAMPIRIC, 15}, AT_NO_ATK },
    6, 330,
    10, 10, MST_VAMPIRE, CE_CLEAN, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_CONSUMABLES), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_VAMPIRE}, TILE_CORPSE_VAMPIRE
},

{
    MONS_VAMPIRE_KNIGHT, 'V', CYAN, "vampire knight",
    M_FIGHTER | M_SEE_INVIS | M_SPEAKS
        | M_BLOOD_SCENT | M_WARM_BLOOD | M_GENDERED,
    MR_RES_COLD | MR_VUL_PIERCING | MR_RES_BLUDGEONING,
    16, MONS_VAMPIRE, MONS_VAMPIRE, MH_UNDEAD, 80,
    { {AT_HIT, AF_PLAIN, 33}, {AT_SHIELD, AF_PLAIN, 33}, 
      {AT_BITE, AF_VAMPIRIC, 15}, AT_NO_ATK },
    11, 715,
    10, 22, MST_VAMPIRE_KNIGHT, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_VAMPIRE_KNIGHT}, TILE_CORPSE_VAMPIRE
},

{
    MONS_VAMPIRE_MAGE, 'V', MAGENTA, "vampire mage",
    M_SEE_INVIS | M_SPEAKS | M_BLOOD_SCENT | M_WARM_BLOOD | M_FLIES | M_SPELLCASTER | M_GENDERED,
    MR_RES_COLD | MR_VUL_PIERCING | MR_RES_BLUDGEONING,
    15, MONS_VAMPIRE, MONS_VAMPIRE, MH_UNDEAD, 80,
    { {AT_HIT, AF_PLAIN, 15}, {AT_SHIELD, AF_PLAIN, 15},
      {AT_BITE, AF_VAMPIRIC, 15}, AT_NO_ATK },
    10, 550,
    10, 18, MST_VAMPIRE_MAGE, CE_CLEAN, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_VAMPIRE_MAGE}, TILE_CORPSE_VAMPIRE
},

{
    MONS_JIANGSHI, 'V', YELLOW, "jiangshi",
    M_SEE_INVIS | M_BLOOD_SCENT | M_FIGHTER | M_NO_ZOMBIE | M_GENDERED,
    MR_RES_COLD | MR_VUL_PIERCING | MR_RES_BLUDGEONING,
    10, MONS_JIANGSHI, MONS_JIANGSHI, MH_UNDEAD, 80,
    { {AT_CLAW, AF_VAMPIRIC, 27}, {AT_CLAW, AF_VAMPIRIC, 27}, AT_NO_ATK,
       AT_NO_ATK },
    10, 650,
    10, 10, MST_NO_SPELLS, CE_NOXIOUS, S_SILENT,
    I_ANIMAL, HT_LAND, 18, MOVE_ENERGY(6),
    MU_WAND, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_JIANGSHI}, TILE_CORPSE_JIANGSHI
},

// incorporeal undead ('W')
{ // dummy genus monster, but also used for removed monsters
    MONS_GHOST, 'W', WHITE, "ghost",
    M_INSUBSTANTIAL | M_NO_POLY_TO | M_FLIES,
    MR_NO_FLAGS,
    0, MONS_GHOST, MONS_GHOST, MH_UNDEAD, 0,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 10,
    0, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_DOOR, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_GHOST}, TILE_ERROR
},

// XP modifier is 5 for these, because they really aren't all that
// dangerous, but still come out at 200+ XP
{
    MONS_PHANTOM, 'W', BLUE, "phantom",
    M_INSUBSTANTIAL | M_FLIES | M_GENDERED,
    mrd(MR_RES_COLD, 2) | mrd(MR_RES_ACID, 3) | MR_RES_PHYSICAL,
    5, MONS_PHANTOM, MONS_PHANTOM, MH_UNDEAD, 40,
    { {AT_HIT, AF_BLINK, 10}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    7, 385,
    3, 13, MST_BLINKER, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_INCORPOREAL, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_PHANTOM}, TILE_ERROR
},

{
    MONS_HUNGRY_GHOST, 'W', GREEN, "hungry ghost",
    M_SEE_INVIS | M_INSUBSTANTIAL | M_SPEAKS | M_FLIES | M_GENDERED,
    mrd(MR_RES_COLD, 2) | mrd(MR_RES_ACID, 3) | MR_RES_PHYSICAL,
    8, MONS_PHANTOM, MONS_HUNGRY_GHOST, MH_UNDEAD, 40,
    { {AT_HIT, AF_HUNGER, 5}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    7, 385,
    0, 17, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_INCORPOREAL, 10, DEFAULT_ENERGY,
    MU_DOOR, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_HUNGRY_GHOST}, TILE_ERROR
},

{
    MONS_FLAYED_GHOST, 'W', RED, "flayed ghost",
    M_INSUBSTANTIAL | M_SPEAKS | M_FLIES | M_GENDERED,
    mrd(MR_RES_ACID, 3) | MR_RES_PHYSICAL,
    10, MONS_PHANTOM, MONS_FLAYED_GHOST, MH_UNDEAD, 60,
    { {AT_HIT, AF_PLAIN, 30}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    11, 605,
    0, 21, MST_FLAYED_GHOST, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_INCORPOREAL, 10, DEFAULT_ENERGY,
    MU_DOOR, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_FLAYED_GHOST}, TILE_ERROR
},

// player ghost - stats are stored in ghost struct
// BCADDO: Be sure the res physical actually applies
{
    MONS_PLAYER_GHOST, 'W', WHITE, "player ghost",
    M_FIGHTER | M_SPEAKS | M_INSUBSTANTIAL | M_NO_POLY_TO | M_FLIES | M_GHOST_DEMON,
    MR_RES_PHYSICAL,
    15, MONS_PHANTOM, MONS_PLAYER_GHOST, MH_UNDEAD, -5,
    { {AT_HIT, AF_PLAIN, 5}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    4, 140,
    1, 2, MST_GHOST, CE_NOCORPSE, S_SHOUT,
    I_HUMAN, HT_INCORPOREAL, 10, DEFAULT_ENERGY,
    MU_DOOR, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_PLAYER_GHOST}, TILE_ERROR
},

{
    MONS_SHADOW, 'W', MAGENTA, "shadow",
    M_SEE_INVIS | M_INSUBSTANTIAL | M_FLIES,
    mrd(MR_RES_COLD | MR_RES_ACID, 3) | MR_RES_PHYSICAL,
    18, MONS_WRAITH, MONS_SHADOW, MH_ELEMENTAL, 40,
    { {AT_HIT, AF_SHADOWSTAB, 14}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    6, 240,
    7, 18, MST_SHADOW, CE_NOCORPSE, S_SILENT,
    I_ANIMAL, HT_INCORPOREAL, 10, DEFAULT_ENERGY,
    MU_DOOR, SIZE_SMALL, MON_SHAPE_HUMANOID,
    {TILEP_MONS_SHADOW}, TILE_ERROR
},

{
    MONS_SILENT_SPECTRE, 'W', CYAN, "silent spectre",
    M_SPEAKS /* uh... */ | M_SEE_INVIS | M_INSUBSTANTIAL | M_FLIES | M_GENDERED,
    mrd(MR_RES_COLD | MR_RES_ACID, 3) | MR_RES_PHYSICAL,
    10, MONS_WRAITH, MONS_SILENT_SPECTRE, MH_UNDEAD, 40,
    { {AT_HIT, AF_PLAIN, 15}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    8, 440,
    5, 23, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_INCORPOREAL, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_SILENT_SPECTRE}, TILE_ERROR
},

{
    MONS_DROWNED_SOUL, 'W', LIGHTCYAN, "drowned soul",
    M_SEE_INVIS | M_INSUBSTANTIAL | M_SPEAKS | M_FLIES,
    mrd(MR_RES_COLD | MR_RES_ACID, 3) | MR_RES_PHYSICAL,
    8, MONS_PHANTOM, MONS_DROWNED_SOUL, MH_UNDEAD, 60,
    { {AT_TOUCH, AF_DROWN, 0}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    13, 195,
    0, 19, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_INCORPOREAL, 12, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DROWNED_SOUL}, TILE_ERROR
},

{
    MONS_WRAITH, 'W', LIGHTGREY, "wraith",
    M_SEE_INVIS | M_INSUBSTANTIAL | M_FLIES | M_GENDERED,
    MR_RES_COLD | mrd(MR_RES_ACID, 3) | MR_RES_PHYSICAL,
    14, MONS_WRAITH, MONS_WRAITH, MH_UNDEAD, 60,
    { {AT_HIT, AF_DRAIN_SPEED, 15}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    6, 330,
    10, 9, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_INCORPOREAL, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_WRAITH}, TILE_ERROR
},

{
    MONS_SHADOW_WRAITH, 'W', LIGHTMAGENTA, "shadow wraith",
    M_SEE_INVIS | M_INVIS | M_INSUBSTANTIAL | M_FLIES | M_GENDERED,
    mrd(MR_RES_ACID, 3) | MR_RES_PHYSICAL,
    15, MONS_WRAITH, MONS_SHADOW_WRAITH, MH_UNDEAD, 100,
    { {AT_HIT, AF_DRAIN_SPEED, 27}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    10, 550,
    7, 16, MST_NO_SPELLS, CE_NOCORPSE, S_MOAN,
    I_HUMAN, HT_INCORPOREAL, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_SHADOW_WRAITH}, TILE_ERROR
},

{
    MONS_FREEZING_WRAITH, 'W', LIGHTBLUE, "freezing wraith",
    M_SEE_INVIS | M_INSUBSTANTIAL | M_FLIES | M_GENDERED,
    MR_VUL_FIRE | mrd(MR_RES_COLD | MR_RES_ACID, 3) | MR_RES_PHYSICAL,
    10, MONS_WRAITH, MONS_FREEZING_WRAITH, MH_UNDEAD, 40,
    { {AT_HIT, AF_COLD, 16}, {AT_HIT, AF_DRAIN_SPEED, 15}, AT_NO_ATK, AT_NO_ATK },
    8, 440,
    12, 16, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_INCORPOREAL, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_FREEZING_WRAITH}, TILE_ERROR
},

{
    MONS_EIDOLON, 'W', BROWN, "eidolon",
    M_SEE_INVIS | M_INSUBSTANTIAL | M_FLIES | M_SPELLCASTER | M_GENDERED,
    MR_RES_COLD | mrd(MR_RES_ACID, 3) | MR_RES_PHYSICAL,
    14, MONS_WRAITH, MONS_EIDOLON, MH_UNDEAD, 140,
    { {AT_HIT, AF_DRAIN_SPEED, 27}, {AT_HIT, AF_DRAIN_STAT, 17}, AT_NO_ATK,
       AT_NO_ATK },
    13, 715,
    12, 18, MST_EIDOLON, CE_NOCORPSE, S_MOAN,
    I_HUMAN, HT_INCORPOREAL, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_EIDOLON}, TILE_ERROR
},

{
    MONS_PHANTASMAL_WARRIOR, 'W', LIGHTGREEN, "phantasmal warrior",
    M_FIGHTER | M_SEE_INVIS | M_INSUBSTANTIAL | M_FLIES | M_GENDERED,
    MR_RES_COLD | mrd(MR_RES_ACID, 3) | MR_RES_PHYSICAL,
    13, MONS_WRAITH, MONS_PHANTASMAL_WARRIOR, MH_UNDEAD, 80,
    { {AT_HIT, AF_VULN, 39}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    9, 495,
    12, 26, MST_PHANTASMAL_WARRIOR, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_INCORPOREAL, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_PHANTASMAL_WARRIOR}, TILE_ERROR
},

// large abominations ('X')
{
    MONS_ABOMINATION_LARGE, 'X', LIGHTRED, "large abomination",
    M_NO_REGEN | M_TALL_TILE,
    MR_RES_PIERCING | MR_RES_BLUDGEONING,
    10, MONS_ABOMINATION_SMALL, MONS_ABOMINATION_LARGE, MH_UNDEAD, 100,
    { {AT_HIT, AF_PLAIN, 40}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    11, 495,
    0, 0, MST_NO_SPELLS, CE_NOCORPSE, S_GIBBERING,
    I_BRAINLESS, HT_LAND, 11, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_BIG, MON_SHAPE_MISC,
    {TILEP_MONS_ABOMINATION_LARGE, TVARY_MOD}, TILE_ERROR
},

{
    MONS_THRASHING_HORROR, 'X', YELLOW, "thrashing horror",
    M_BATTY | M_NO_POLY_TO,
    MR_RES_FIRE | MR_RES_COLD | MR_RES_ELEC 
     | MR_RES_PIERCING | MR_RES_BLUDGEONING,
    10, MONS_THRASHING_HORROR, MONS_THRASHING_HORROR, MH_ELEMENTAL, 60,
    { {AT_TRAMPLE, AF_TRAMPLE, 17}, {AT_TRAMPLE, AF_TRAMPLE, 9},
       AT_NO_ATK, AT_NO_ATK },
    9, 495,
    5, 10, MST_THRASHING_HORROR, CE_NOCORPSE, S_ROAR,
    I_ANIMAL, HT_LAND, 25, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_BIG, MON_SHAPE_MISC,
    {TILEP_MONS_THRASHING_HORROR}, TILE_ERROR
},

{
    MONS_TENTACLED_MONSTROSITY, 'X', GREEN, "tentacled monstrosity",
    M_SEE_INVIS,
    MR_RES_POISON | MR_RES_FIRE | MR_RES_COLD | MR_RES_ELEC | mrd(MR_RES_NEG, 1)
     | MR_RES_PIERCING | MR_RES_BLUDGEONING | MR_VUL_SLASHING,
    10, MONS_TENTACLED_MONSTROSITY, MONS_TENTACLED_MONSTROSITY,
        MH_DEMONIC, 160,
    { {AT_TENTACLE_SLAP, AF_PLAIN, 22}, {AT_TENTACLE_SLAP, AF_PLAIN, 17},
      {AT_TENTACLE_SLAP, AF_PLAIN, 13}, {AT_CONSTRICT, AF_CRUSH, 9} },
    23, 1265,
    5, 5, MST_NO_SPELLS, CE_DIVINE, S_SILENT,
    I_HUMAN, HT_AMPHIBIOUS, 10, DEFAULT_ENERGY,
    MU_DOOR, SIZE_GIANT, MON_SHAPE_MISC,
    {TILEP_MONS_TENTACLED_MONSTROSITY}, TILE_CORPSE_TENTACLED_MONSTROSITY
},

{
    MONS_ORB_GUARDIAN, 'X', MAGENTA, "Orb Guardian",
    M_FIGHTER | M_SEE_INVIS | M_NO_POLY_TO,
    MR_NO_FLAGS,
    20, MONS_ORB_GUARDIAN, MONS_ORB_GUARDIAN, MH_NATURAL, 120,
    { {AT_HIT, AF_PLAIN, 45}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    15, 825,
    13, 21, MST_NO_SPELLS, CE_DIVINE, S_SILENT,
    I_HUMAN, HT_LAND, 14, DEFAULT_ENERGY,
    MU_DOOR, SIZE_GIANT, MON_SHAPE_MISC,
    {TILEP_MONS_ORB_GUARDIAN}, TILE_CORPSE_ORB_GUARDIAN
},

{
    MONS_TENTACLED_STARSPAWN, 'X', LIGHTCYAN, "tentacled starspawn",
    M_SEE_INVIS,
    MR_RES_PIERCING | MR_RES_BLUDGEONING | MR_VUL_SLASHING,
    20, MONS_TENTACLED_STARSPAWN, MONS_TENTACLED_STARSPAWN, MH_ELEMENTAL, 120,
    { {AT_BITE, AF_PLAIN, 40}, {AT_ENGULF, AF_PLAIN, 25}, AT_NO_ATK,
       AT_NO_ATK },
    16, 880,
    5, 5, MST_TENTACLED_STARSPAWN, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_DOOR, SIZE_GIANT, MON_SHAPE_MISC,
    {TILEP_MONS_TENTACLED_STARSPAWN}, TILE_ERROR
},

{
    MONS_STARSPAWN_TENTACLE, 'w', LIGHTCYAN, "starspawn tentacle",
    M_NO_EXP_GAIN | M_STATIONARY | M_NO_POLY_TO | M_FLIES | M_SEE_INVIS,
    MR_RES_PIERCING | MR_VUL_SLASHING,
    10, MONS_TENTACLED_STARSPAWN, MONS_STARSPAWN_TENTACLE,
        MH_ELEMENTAL, MAG_IMMUNE,
    { {AT_CONSTRICT, AF_CRUSH, 3}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    11, 220,
    8, 2, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_SNAKE,
    {TILEP_MONS_PROGRAM_BUG}, TILE_ERROR
},

{
    MONS_STARSPAWN_TENTACLE_SEGMENT, '*', LIGHTCYAN, "starspawn tentacle segment",
    M_NOT_DANGEROUS | M_STATIONARY | M_NO_POLY_TO | M_FLIES | M_SEE_INVIS,
    MR_RES_PIERCING | MR_VUL_SLASHING,
    10, MONS_TENTACLED_STARSPAWN, MONS_STARSPAWN_TENTACLE_SEGMENT,
        MH_ELEMENTAL, MAG_IMMUNE,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    11, 220,
    8, 2, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_MISC,
    {TILEP_MONS_PROGRAM_BUG}, TILE_ERROR
},

{
    MONS_STARCURSED_MASS, 'X', BLUE, "starcursed mass",
    M_SEE_INVIS | M_FAST_REGEN,
    MR_RES_PIERCING | MR_RES_SLASHING,
    12, MONS_STARCURSED_MASS, MONS_STARCURSED_MASS, MH_ELEMENTAL, 100,
    { {AT_ENGULF, AF_PLAIN, 16}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    12, 1500,
    10, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_GIANT, MON_SHAPE_MISC,
    {TILEP_MONS_STARCURSED_MASS}, TILE_ERROR
},

// Summoning miscast, monster is scaled based on miscast strength
{
    MONS_NAMELESS, 'X', CYAN, "nameless horror",
    M_SEE_INVIS | M_NO_POLY_TO | M_NO_GEN_DERIVED,
    MR_NO_FLAGS,
    11, MONS_NAMELESS, MONS_NAMELESS, MH_ELEMENTAL, MAG_IMMUNE,
    { {AT_HIT, AF_ANTIMAGIC, 30}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    15, 600,
    8, 2, MST_NAMELESS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_GIANT, MON_SHAPE_MISC,
    { TILEP_MONS_NAMELESS_HORROR }, TILE_ERROR
},

// yaks, sheep, elephants, and other beasts ('Y')
{
    MONS_DREAM_SHEEP, 'Y', CYAN, "dream sheep",
    M_WARM_BLOOD | M_HERD,
    MR_VUL_SLASHING | MR_RES_PIERCING,
    10, MONS_DREAM_SHEEP, MONS_DREAM_SHEEP, MH_NATURAL, 30,
    { {AT_BITE, AF_PLAIN, 13}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    9, 220,
    2, 7, MST_DREAM_SHEEP, CE_CLEAN, S_BLEAT,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_QUADRUPED_TAILLESS,
    {TILEP_MONS_DREAM_SHEEP}, TILE_CORPSE_DREAM_SHEEP,
},

{
    MONS_YAK, 'Y', BROWN, "yak",
    M_WARM_BLOOD | M_HERD,
    MR_VUL_SLASHING | MR_RES_PIERCING,
    9, MONS_YAK, MONS_YAK, MH_NATURAL, 20,
    { {AT_GORE, AF_PLAIN, 18}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    7, 385,
    4, 7, MST_NO_SPELLS, CE_CLEAN, S_BELLOW,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_YAK}, TILE_CORPSE_YAK
},

{
    MONS_DEATH_YAK, 'Y', RED, "death yak",
    M_WARM_BLOOD | M_HERD,
    MR_VUL_SLASHING | MR_RES_BLUDGEONING,
    8, MONS_YAK, MONS_DEATH_YAK, MH_NATURAL, 100,
    { {AT_GORE, AF_PLAIN, 30}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    14, 770,
    9, 5, MST_NO_SPELLS, CE_NOXIOUS, S_BELLOW,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_BIG, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_DEATH_YAK}, TILE_CORPSE_DEATH_YAK
},

{
    MONS_GIRAFFE, 'Y', YELLOW, "giraffe",
    M_WARM_BLOOD | M_HERD | M_TALL_TILE,
    MR_VUL_SLASHING,
    9, MONS_GIRAFFE, MONS_GIRAFFE, MH_NATURAL, 30,
    { { AT_GORE, AF_REACH, 10 }, { AT_KICK, AF_TRAMPLE, 12 } , AT_NO_ATK, AT_NO_ATK },
    14, 305,
    7, 4, MST_NO_SPELLS, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_LAND, 12, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_GIANT, MON_SHAPE_QUADRUPED,
    { TILEP_MONS_GIRAFFE }, TILE_CORPSE_GIRAFFE
},

{
    MONS_CATOBLEPAS, 'Y', MAGENTA, "catoblepas",
    M_WARM_BLOOD,
    MR_RES_PETRIFY | MR_VUL_SLASHING | MR_RES_BLUDGEONING,
    8, MONS_YAK, MONS_CATOBLEPAS, MH_NATURAL, 100,
    { {AT_GORE, AF_PLAIN, 36}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    14, 770,
    10, 2, MST_CATOBLEPAS, CE_CLEAN, S_BELLOW,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_BIG, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_CATOBLEPAS}, TILE_CORPSE_CATOBLEPAS
},

{
    MONS_ELEPHANT, 'Y', GREEN, "elephant",
    M_WARM_BLOOD,
    MR_VUL_SLASHING | MR_RES_BLUDGEONING,
    9, MONS_ELEPHANT, MONS_ELEPHANT, MH_NATURAL, 60,
    { {AT_TRAMPLE, AF_TRAMPLE, 20}, {AT_TRUNK_SLAP, AF_PLAIN, 5}, AT_NO_ATK,
       AT_NO_ATK },
    9, 675,
    8, 2, MST_NO_SPELLS, CE_CLEAN, S_TRUMPET,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_GIANT, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_ELEPHANT}, TILE_CORPSE_ELEPHANT
},

{
    MONS_DIRE_ELEPHANT, 'Y', BLUE, "dire elephant",
    M_WARM_BLOOD,
    MR_RES_BLUDGEONING,
    8, MONS_ELEPHANT, MONS_DIRE_ELEPHANT, MH_NATURAL, 100,
    { {AT_TRAMPLE, AF_TRAMPLE, 40}, {AT_TRUNK_SLAP, AF_PLAIN, 15}, AT_NO_ATK,
       AT_NO_ATK },
    15, 1125,
    13, 7, MST_NO_SPELLS, CE_CLEAN, S_TRUMPET,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_GIANT, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_DIRE_ELEPHANT}, TILE_CORPSE_DIRE_ELEPHANT
},

{
    MONS_HELLEPHANT, 'Y', LIGHTRED, "hellephant",
    M_WARM_BLOOD,
    mrd(MR_RES_NEG, 1) | mrd(MR_RES_FIRE, 2) | MR_RES_BLUDGEONING
     | MR_RES_PIERCING,
    10, MONS_ELEPHANT, MONS_HELLEPHANT, MH_DEMONIC, 140,
    { {AT_TRAMPLE, AF_TRAMPLE, 45}, {AT_BITE, AF_PLAIN, 20},
      {AT_GORE, AF_PLAIN, 15 }, AT_NO_ATK },
    20, 1700,
    13, 19, MST_HELLEPHANT, CE_CLEAN, S_TRUMPET,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_GIANT, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_HELLEPHANT}, TILE_CORPSE_HELLEPHANT
},

{
    MONS_APIS, 'Y', WHITE, "apis",
    M_WARM_BLOOD,
    MR_VUL_BLUDGEONING,
    8, MONS_APIS, MONS_APIS, MH_HOLY, 100,
    { {AT_GORE, AF_HOLY, 40}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    16, 1200,
    9, 19, MST_NO_SPELLS, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_BIG, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_APIS}, TILE_CORPSE_APIS
},

{
    MONS_ICE_BEAST, 'Y', LIGHTBLUE, "ice beast",
    M_NO_FLAGS,
    MR_RES_POISON | MR_VUL_FIRE | mrd(MR_RES_COLD, 4) | MR_RES_SLASHING,
    13, MONS_ICE_BEAST, MONS_ICE_BEAST, MH_NATURAL | MH_CONSTRUCT, 20,
    { {AT_HIT, AF_COLD, 5}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    5, 275,
    5, 10, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_ANIMAL, HT_AMPHIBIOUS, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_ICE_BEAST}, TILE_ERROR
},

{
    MONS_SKY_BEAST, 'Y', LIGHTCYAN, "sky beast",
    M_NO_SKELETON | M_FLIES,
    mrd(MR_RES_ELEC, 4) | MR_RES_ACID | MR_RES_PIERCING | MR_RES_BLUDGEONING,
    13, MONS_SKY_BEAST, MONS_SKY_BEAST, MH_NATURAL, 20,
    { {AT_HIT, AF_ELEC, 5}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    5, 275,
    3, 13, MST_NO_SPELLS, CE_NOXIOUS, S_SILENT,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_QUADRUPED_WINGED,
    {TILEP_MONS_SKY_BEAST}, TILE_CORPSE_SKY_BEAST
},

// zombies and derived undead ('Z')
// zombie, skeleton and simulacra species depend on corpse species,
// or else are chosen randomly
{
    MONS_ZOMBIE, 'Z', BROWN, "zombie",
    M_NO_REGEN,
    mrd(MR_RES_COLD, 2) | MR_RES_PIERCING
     | MR_VUL_SLASHING,
    9, MONS_ZOMBIE, MONS_ZOMBIE, MH_UNDEAD, -1,
    { {AT_HIT, AF_PLAIN, 10}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 75,
    0, 4, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 5, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_MISC,
    {TILEP_MONS_ZOMBIE_SMALL}, TILE_ERROR
},

{
    MONS_SKELETON, 'Z', LIGHTGREY, "skeleton",
    M_NO_REGEN,
    mrd(MR_RES_COLD, 2) | MR_VUL_ACID 
     | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    9, MONS_SKELETON, MONS_SKELETON, MH_UNDEAD, -1,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    0, 0,
    0, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 5, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_MISC,
    {TILEP_MONS_SKELETON_SMALL}, TILE_ERROR
},

{
    MONS_SIMULACRUM, 'Z', LIGHTBLUE, "simulacrum",
    M_NO_REGEN,
    MR_VUL_FIRE | mrd(MR_RES_COLD, 3)
     | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    9, MONS_SIMULACRUM, MONS_SIMULACRUM, MH_UNDEAD, -1,
    { {AT_HIT, AF_PLAIN, 6}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    2, 110,
    10, 4, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 7, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_MISC,
    {TILEP_MONS_SIMULACRUM_SMALL}, TILE_ERROR
},

{
    MONS_ZOMBIE_SMALL, 'Z', BROWN, "small zombie",
    M_NO_REGEN,
    mrd(MR_RES_COLD, 2) | MR_RES_PIERCING
     | MR_VUL_SLASHING,
    9, MONS_ZOMBIE, MONS_ZOMBIE, MH_UNDEAD, -1,
    { {AT_HIT, AF_PLAIN, 10}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 75,
    0, 4, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 5, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_MISC,
    {TILEP_MONS_ZOMBIE_SMALL}, TILE_ERROR
},

{
    MONS_SKELETON_SMALL, 'Z', LIGHTGREY, "small skeleton",
    M_NO_REGEN,
    mrd(MR_RES_COLD, 2) | MR_VUL_ACID
     | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    9, MONS_SKELETON, MONS_SKELETON, MH_UNDEAD, -1,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    0, 0,
    0, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 5, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_MISC,
    {TILEP_MONS_SKELETON_SMALL}, TILE_ERROR
},

{
    MONS_SIMULACRUM_SMALL, 'Z', LIGHTBLUE, "small simulacrum",
    M_NO_REGEN,
    MR_VUL_FIRE | mrd(MR_RES_COLD, 3)
     | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    9, MONS_SIMULACRUM, MONS_SIMULACRUM, MH_UNDEAD, -1,
    { {AT_HIT, AF_PLAIN, 6}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    2, 110,
    10, 4, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 7, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_MISC,
    {TILEP_MONS_SIMULACRUM_SMALL}, TILE_ERROR
},

{
    MONS_ZOMBIE_LARGE, 'Z', YELLOW, "large zombie",
    M_NO_REGEN,
    mrd(MR_RES_COLD, 2) | MR_RES_PIERCING
     | MR_VUL_SLASHING,
    9, MONS_ZOMBIE, MONS_ZOMBIE, MH_UNDEAD, -1,
    { {AT_HIT, AF_PLAIN, 23}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    6, 330,
    8, 5, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 5, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_MISC,
    {TILEP_MONS_ZOMBIE_LARGE}, TILE_ERROR
},

{
    MONS_SKELETON_LARGE, 'Z', WHITE, "large skeleton",
    M_NO_REGEN,
    mrd(MR_RES_COLD, 2) | MR_VUL_ACID 
     | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    9, MONS_SKELETON, MONS_SKELETON, MH_UNDEAD, -1,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    0, 0,
    0, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 5, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_MISC,
    {TILEP_MONS_SKELETON_LARGE}, TILE_ERROR
},

{
    MONS_SIMULACRUM_LARGE, 'Z', LIGHTCYAN, "large simulacrum",
    M_NO_REGEN,
    MR_VUL_FIRE | mrd(MR_RES_COLD, 3) 
     | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    9, MONS_SIMULACRUM, MONS_SIMULACRUM, MH_UNDEAD, -1,
    { {AT_HIT, AF_PLAIN, 14}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    5, 275,
    10, 5, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 7, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_MISC,
    {TILEP_MONS_SIMULACRUM_LARGE}, TILE_ERROR
},

// spectral thing - similar to zombies/skeletons
{
    MONS_SPECTRAL_THING, 'Z', GREEN, "spectral thing",
    M_SEE_INVIS | M_INSUBSTANTIAL | M_FLIES,
    MR_RES_COLD | mrd(MR_RES_ACID, 3) | MR_RES_PHYSICAL,
    11, MONS_WRAITH, MONS_SPECTRAL_THING, MH_UNDEAD, 160,
    { {AT_HIT, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    8, 440,
    8, 5, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_INCORPOREAL, 7, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_MISC,
    {TILEP_MONS_SPECTRAL_LARGE}, TILE_ERROR
},

// water monsters
{
    MONS_ELECTRIC_EEL, ';', LIGHTBLUE, "electric eel",
    M_NO_GEN_DERIVED | M_COLD_BLOOD,
    mrd(MR_RES_ELEC, 3) | MR_VUL_SLASHING,
    19, MONS_ELECTRIC_EEL, MONS_ELECTRIC_EEL, MH_NATURAL, 10,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    3, 165,
    1, 22, MST_ZAPPER, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_WATER, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_SNAKE,
    {TILEP_MONS_ELECTRIC_EEL}, TILE_CORPSE_ELECTRIC_EEL
},

{
    MONS_SHARK, ';', WHITE, "shark",
    M_NO_GEN_DERIVED | M_COLD_BLOOD,
    MR_RES_PIERCING,
    10, MONS_SHARK, MONS_SHARK, MH_NATURAL, 40,
    { { AT_BITE, AF_PLAIN, 30}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    8, 440,
    3, 27, MST_NO_SPELLS, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_WATER, 12, SWIM_ENERGY(6),
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_FISH,
    { TILEP_MONS_SHARK }, TILE_CORPSE_SHARK
},

{   // BCADNOTE: Fakes being a derived undead but is its own thing
    MONS_PUFFERFISH, ';', LIGHTGREY, "puffer fish skeleton",
    M_SPINY | M_NO_POLY_TO | M_NO_REGEN,
    MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    5, MONS_PUFFERFISH, MONS_PUFFERFISH, MH_UNDEAD, 40,
    { { AT_BITE, AF_PLAIN, 8 }, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    3, 125,
    3, 8, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_ANIMAL, HT_WATER, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_FISH,
    { TILEP_MONS_SKELETON_PUFFERFISH }, TILE_ERROR
},

{
    MONS_SLIMEHEAD, ';', GREEN, "slimehead",
    M_COLD_BLOOD,
    mrd(MR_RES_ACID, 3) | mrd(MR_RES_POISON, 3) | MR_RES_BLUDGEONING,
    5, MONS_SLIMEHEAD, MONS_SLIMEHEAD, MH_NATURAL, 40,
    { { AT_BITE, AF_PLAIN, 15 }, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    6, 625,
    8, 4, MST_NO_SPELLS, CE_NOXIOUS, S_SILENT,
    I_ANIMAL, HT_WATER, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_FISH,
    {TILEP_MONS_SLIMEHEAD}, TILE_CORPSE_SLIMEHEAD
},

{
    MONS_RAINBOW_TROUT, ';', CYAN, "rainbow trout",
    M_COLD_BLOOD,
    MR_VUL_SLASHING,
    1, MONS_RAINBOW_TROUT, MONS_RAINBOW_TROUT, MH_NATURAL, 10,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    2, 300,
    1, 4, MST_NO_SPELLS, CE_CLEAN, S_SILENT,
    I_ANIMAL, HT_WATER, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_FISH,
    { TILEP_MONS_RAINBOW_TROUT }, TILE_CORPSE_RAINBOW_TROUT
},

// A kraken and its tentacles get a random colour from ETC_KRAKEN.
{
    MONS_KRAKEN, 'X', LIGHTMAGENTA, "kraken",
    M_NO_SKELETON | M_COLD_BLOOD,
    MR_RES_BLUDGEONING,
    6, MONS_KRAKEN, MONS_KRAKEN, MH_NATURAL, 60,
    { {AT_BITE, AF_PLAIN, 50}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    16, 2080,
    20, 0, MST_KRAKEN, CE_NOXIOUS, S_SILENT,
    I_ANIMAL, HT_WATER, 14, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_GIANT, MON_SHAPE_MISC,
    {TILEP_MONS_KRAKEN_HEAD}, TILE_CORPSE_KRAKEN
},

{
    MONS_KRAKEN_TENTACLE, 'w', LIGHTMAGENTA, "tentacle",
    M_COLD_BLOOD | M_NO_EXP_GAIN | M_STATIONARY | M_NO_POLY_TO | M_FLIES,
    MR_VUL_SLASHING | MR_RES_PIERCING,
    10, MONS_KRAKEN, MONS_KRAKEN_TENTACLE, MH_NATURAL, MAG_IMMUNE,
    { {AT_TENTACLE_SLAP, AF_PLAIN, 29}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    12, 480,
    5, 7, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_ANIMAL, HT_AMPHIBIOUS, 17, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_SNAKE,
    {TILEP_MONS_PROGRAM_BUG}, TILE_ERROR
},

{
    MONS_KRAKEN_TENTACLE_SEGMENT, '*', LIGHTMAGENTA, "tentacle segment",
    M_COLD_BLOOD | M_NOT_DANGEROUS | M_STATIONARY | M_NO_POLY_TO | M_FLIES,
    MR_VUL_SLASHING | MR_RES_PIERCING,
    10, MONS_KRAKEN, MONS_KRAKEN_TENTACLE_SEGMENT, MH_NATURAL, MAG_IMMUNE,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    12, 480,
    5, 7, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_ANIMAL, HT_AMPHIBIOUS, 18, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_MISC,
    {TILEP_MONS_PROGRAM_BUG}, TILE_ERROR
},

// Octopode race, not a 'normal' octopus.
// Dummy monster for recolouring.
{
    MONS_OCTOPODE, 'x', LIGHTCYAN, "octopode",
    M_NO_SKELETON | M_SPEAKS | M_NO_POLY_TO | M_GENDERED,
    MR_VUL_SLASHING | MR_RES_PIERCING | MR_RES_BLUDGEONING,
    10, MONS_OCTOPODE, MONS_OCTOPODE, MH_NATURAL, 10,
    { {AT_HIT, AF_PLAIN, 24}, {AT_SHIELD, AF_PLAIN, 24}, 
      {AT_CONSTRICT, AF_CRUSH, 5}, AT_NO_ATK },
    8, 560,
    1, 5, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_AMPHIBIOUS, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_MISC,
    {TILEP_MONS_OCTOPODE}, TILE_CORPSE_OCTOPODE
},

// lava monsters
{
    MONS_LAVA_SNAKE, 'S', LIGHTRED, "lava snake",
    M_WARM_BLOOD,
    mrd(MR_RES_FIRE, 3) | MR_VUL_COLD | MR_VUL_SLASHING | MR_RES_PIERCING,
    17, MONS_SNAKE, MONS_LAVA_SNAKE, MH_NATURAL, 10,
    { {AT_BITE, AF_FIRE, 7}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    3, 165,
    2, 23, MST_LAVA_SNAKE, CE_CLEAN, S_HISS,
    I_ANIMAL, HT_LAVA, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_SMALL, MON_SHAPE_SNAKE,
    {TILEP_MONS_LAVA_SNAKE}, TILE_CORPSE_LAVA_SNAKE
},

{
    MONS_SALAMANDER, 'N', LIGHTRED, "salamander",
    M_FIGHTER | M_WARM_BLOOD | M_GENDERED,
    mrd(MR_RES_FIRE, 3) | MR_VUL_COLD,
    10, MONS_NAGA, MONS_SALAMANDER, MH_NATURAL, 40,
    { {AT_HIT, AF_FIRE, 17}, {AT_SHIELD, AF_PLAIN, 17}, AT_NO_ATK, AT_NO_ATK },
    8, 640,
    5, 12, MST_NO_SPELLS, CE_CLEAN, S_SILENT,
    I_HUMAN, HT_AMPHIBIOUS_LAVA, 10, SWIM_ENERGY(7),
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_NAGA,
    {TILEP_MONS_SALAMANDER}, TILE_CORPSE_SALAMANDER
},

{
    MONS_SALAMANDER_MYSTIC, 'N', YELLOW, "salamander mystic",
    M_WARM_BLOOD | M_SPELLCASTER | M_GENDERED,
    mrd(MR_RES_FIRE, 3) | MR_VUL_COLD,
    10, MONS_NAGA, MONS_SALAMANDER, MH_NATURAL, 60,
    { {AT_HIT, AF_FIRE, 10}, {AT_SHIELD, AF_PLAIN, 10}, AT_NO_ATK, AT_NO_ATK },
    10, 650,
    5, 17, MST_SALAMANDER_MYSTIC, CE_CLEAN, S_SILENT,
    I_HUMAN, HT_AMPHIBIOUS_LAVA, 10, SWIM_ENERGY(7),
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_NAGA,
    {TILEP_MONS_SALAMANDER_MYSTIC}, TILE_ERROR
},

// Semi-unique humans ('@')
{
    MONS_HELLBINDER, '@', ETC_FIRE, "Hellbinder",
    M_SPEAKS | M_WARM_BLOOD | M_SEE_INVIS | M_SPELLCASTER | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 100,
    { {AT_HIT, AF_PLAIN, 6}, {AT_SHIELD, AF_PLAIN, 6}, AT_NO_ATK, AT_NO_ATK },
    20, 1500,
    0, 17, MST_HELLBINDER, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_HELLBINDER}, TILE_ERROR
},

{
    MONS_CLOUD_MAGE, '@', ETC_SILVER, "Cloud Mage",
    M_SPEAKS | M_WARM_BLOOD | M_SEE_INVIS | M_SPELLCASTER | M_GENDERED | M_FLIES,
    MR_RES_WIND | MR_VUL_SLASHING,
    10, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 100,
    { {AT_HIT, AF_PLAIN, 6}, {AT_SHIELD, AF_PLAIN, 6}, AT_NO_ATK, AT_NO_ATK },
    20, 1500,
    0, 23, MST_CLOUD_MAGE, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_CLOUD_MAGE}, TILE_ERROR
},

// 'dummy' a fairly accurate description
DUMMY(MONS_PLAYER, '@', LIGHTGREY, "player", TILEP_MONS_PLAYER_GHOST)

// BCADDO: Look into this thing?
// player illusion (Mara) - stats are stored in ghost struct. Undead/demonic
// flags are set based on the current player's species!
{
    MONS_PLAYER_ILLUSION, '@', WHITE, "player illusion",
    M_FIGHTER | M_SPEAKS | M_INSUBSTANTIAL | M_NO_POLY_TO | M_FLIES | M_GHOST_DEMON,
    MR_RES_POISON,
    15, MONS_PLAYER_ILLUSION, MONS_PLAYER_ILLUSION, MH_NATURAL, -5,
    { {AT_HIT, AF_PLAIN, 5}, {AT_SHIELD, AF_PLAIN, 5}, AT_NO_ATK, AT_NO_ATK },
    4, 140,
    1, 2, MST_GHOST, CE_NOCORPSE, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_DOOR, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_PLAYER_GHOST}, TILE_ERROR
},

{
    MONS_PLAYER_SHADOW, 'W', MAGENTA, "shadow",
    M_CANT_SPAWN | M_NO_EXP_GAIN | M_INSUBSTANTIAL | M_FLIES,
    MR_NO_FLAGS,
    10, MONS_HUMAN, MONS_HUMAN, MH_ELEMENTAL, -3,
    { {AT_HIT, AF_PLAIN, 0}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 10,
    3, 13, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_PROGRAM_BUG}, TILE_ERROR
},
// dancing weapon
// These are named more explicitly when they attack, also when you use 'x'
// to examine them.
{
    MONS_DANCING_WEAPON, '(', COLOUR_UNDEF, "dancing weapon",
    M_FIGHTER | M_FLIES | M_GHOST_DEMON,
    mrd(MR_RES_FIRE | MR_RES_COLD, 2) | mrd(MR_RES_ELEC, 3) | MR_VUL_ACID
     | MR_RES_BLUDGEONING | MR_RES_SLASHING,
    10, MONS_DANCING_WEAPON, MONS_DANCING_WEAPON, MH_CONSTRUCT, 180,
    { {AT_HIT, AF_PLAIN, 30}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    15, 150,
    10, 30, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 15, DEFAULT_ENERGY,
    muf(MU_WEAPON_MELEE | MU_START_ONLY), SIZE_SMALL, MON_SHAPE_MISC,
    {TILE_UNSEEN_WEAPON}
},

// spectral weapon, for skalds!
{
    MONS_SPECTRAL_WEAPON, '(', GREEN, "spectral weapon",
    M_INSUBSTANTIAL | M_NO_REGEN | M_AVATAR | M_NO_EXP_GAIN | M_FLIES
        | M_GHOST_DEMON,
    MR_RES_FIRE | MR_RES_COLD |  MR_RES_ELEC | mrd(MR_RES_ACID, 3)
     | MR_RES_PHYSICAL,
    11, MONS_WRAITH, MONS_SPECTRAL_WEAPON, MH_ELEMENTAL, MAG_IMMUNE,
    { {AT_HIT, AF_PLAIN, 6}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    5, 150,
    5, 18, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_INCORPOREAL, 30, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_MISC,
    {TILEP_MONS_SPECTRAL_SBL}, TILE_ERROR
},

// Demonic tentacle things.
{
    MONS_ELDRITCH_TENTACLE, 'w', COLOUR_UNDEF, "eldritch tentacle",
    M_NO_POLY_TO | M_STATIONARY | M_SEE_INVIS | M_FLIES,
    mrd(MR_RES_FIRE | MR_RES_COLD | MR_RES_ELEC
        | MR_RES_ACID, 3) | MR_RES_STICKY_FLAME | MR_RES_PIERCING,
    10, MONS_ELDRITCH_TENTACLE, MONS_ELDRITCH_TENTACLE,
        MH_ELEMENTAL, MAG_IMMUNE,
    { {AT_TENTACLE_SLAP, AF_CHAOTIC, 30}, {AT_CLAW, AF_CHAOTIC, 40}, AT_NO_ATK,
       AT_NO_ATK },
    16, 1200,
    13, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_ANIMAL, HT_AMPHIBIOUS, 12, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_GIANT, MON_SHAPE_SNAKE,
    {TILEP_MONS_PROGRAM_BUG}, TILE_ERROR
},

{
    MONS_ELDRITCH_TENTACLE_SEGMENT, '*', COLOUR_UNDEF, "eldritch tentacle segment",
    M_NOT_DANGEROUS | M_STATIONARY | M_NO_POLY_TO | M_SEE_INVIS | M_FLIES,
    mrd(MR_RES_FIRE | MR_RES_COLD | MR_RES_ELEC
        | MR_RES_ACID, 3) | MR_RES_STICKY_FLAME | MR_RES_PIERCING,
    10, MONS_ELDRITCH_TENTACLE, MONS_ELDRITCH_TENTACLE_SEGMENT,
        MH_ELEMENTAL, MAG_IMMUNE,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    16, 1200,
    13, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_ANIMAL, HT_AMPHIBIOUS, 12, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_GIANT, MON_SHAPE_MISC,
    {TILEP_MONS_PROGRAM_BUG}, TILE_ERROR
},

// demonspawn ('6')
{
    MONS_DEMONSPAWN, '6', BROWN, "demonspawn",
    M_WARM_BLOOD | M_SPEAKS | M_NO_POLY_TO | M_GENDERED,
    MR_VUL_SLASHING,
    10, MONS_DEMONSPAWN, MONS_DEMONSPAWN, MH_NATURAL | MH_DEMONIC, 20,
    { {AT_HIT, AF_PLAIN, 10}, {AT_SHIELD, AF_PLAIN, 10}, AT_NO_ATK, AT_NO_ATK },
    6, 330,
    3, 21, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_MELEE, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DEMONSPAWN}, TILE_CORPSE_DEMONSPAWN
},

{
    MONS_MONSTROUS_DEMONSPAWN, '6', LIGHTGRAY, "monstrous demonspawn",
    M_WARM_BLOOD | M_SPEAKS | M_SEE_INVIS | M_NO_POLY_TO | M_GENDERED,
    MR_VUL_SLASHING | MR_RES_BLUDGEONING,
    10, MONS_DEMONSPAWN, MONS_MONSTROUS_DEMONSPAWN, MH_NATURAL | MH_DEMONIC, 40,
    { {AT_HIT, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20},
      {AT_CLAW, AF_PLAIN, 21}, {AT_GORE, AF_PLAIN, 21} },
    10, 1000,
    3, 18, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_MONSTROUS_DEMONSPAWN}, TILE_CORPSE_MONSTROUS_DEMONSPAWN
},

{
    MONS_GELID_DEMONSPAWN, '6', BLUE, "gelid demonspawn",
    M_WARM_BLOOD | M_SPEAKS | M_NO_POLY_TO | M_GENDERED,
    mrd(MR_RES_COLD, 3) | MR_VUL_SLASHING,
    10, MONS_DEMONSPAWN, MONS_GELID_DEMONSPAWN, MH_NATURAL | MH_DEMONIC, 40,
    { {AT_HIT, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    10, 550,
    6, 23, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_GELID_DEMONSPAWN}, TILE_CORPSE_GELID_DEMONSPAWN
},

{
    MONS_INFERNAL_DEMONSPAWN, '6', RED, "infernal demonspawn",
    M_WARM_BLOOD | M_SPEAKS | M_NO_POLY_TO | M_GENDERED,
    mrd(MR_RES_FIRE, 3) | MR_VUL_SLASHING,
    10, MONS_DEMONSPAWN, MONS_INFERNAL_DEMONSPAWN, MH_NATURAL | MH_DEMONIC, 40,
    { {AT_HIT, AF_FIRE, 20}, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    10, 550,
    6, 23, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_INFERNAL_DEMONSPAWN}, TILE_CORPSE_INFERNAL_DEMONSPAWN
},

{
    MONS_TORTUROUS_DEMONSPAWN, '6', MAGENTA, "torturous demonspawn",
    M_WARM_BLOOD | M_SPEAKS | M_NO_POLY_TO | M_SPINY | M_GENDERED,
    mrd(MR_RES_NEG, 3) | MR_RES_PIERCING | MR_VUL_SLASHING,
    10, MONS_DEMONSPAWN, MONS_TORTUROUS_DEMONSPAWN, MH_NATURAL | MH_DEMONIC, 40,
    { {AT_HIT, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    10, 550,
    3, 23, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_TORTUROUS_DEMONSPAWN}, TILE_CORPSE_TORTUROUS_DEMONSPAWN
},

// The nonbase demonspawn's hit dice HP values, AC, and EV are added to
// those of the relevant base demonspawn type.
{
    MONS_BLOOD_SAINT, '6', LIGHTBLUE, "blood saint",
    M_WARM_BLOOD | M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    MR_NO_FLAGS,
    10, MONS_DEMONSPAWN, MONS_DEMONSPAWN, MH_NATURAL | MH_DEMONIC, 60,
    { {AT_HIT, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    15, 325,
    0, 2, MST_DEMONSPAWN_BLOOD_SAINT, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_NET), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_BLOOD_SAINT}, TILE_ERROR
},

{
    MONS_WARMONGER, '6', LIGHTCYAN, "warmonger",
    M_WARM_BLOOD | M_SPEAKS | M_FIGHTER | M_GENDERED,
    MR_NO_FLAGS,
    10, MONS_DEMONSPAWN, MONS_DEMONSPAWN, MH_NATURAL | MH_DEMONIC, 60,
    { {AT_HIT, AF_PLAIN, 40}, {AT_SHIELD, AF_PLAIN, 40}, AT_NO_ATK, AT_NO_ATK },
    15, 600,
    2, 6, MST_DEMONSPAWN_WARMONGER, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_WARMONGER}, TILE_ERROR
},

{
    MONS_CORRUPTER, '6', LIGHTGREEN, "corrupter",
    M_WARM_BLOOD | M_SPEAKS | M_GENDERED,
    MR_NO_FLAGS,
    10, MONS_DEMONSPAWN, MONS_DEMONSPAWN, MH_NATURAL | MH_DEMONIC, 60,
    { {AT_HIT, AF_PLAIN, 25}, {AT_SHIELD, AF_PLAIN, 25}, AT_NO_ATK, AT_NO_ATK },
    15, 375,
    0, 5, MST_DEMONSPAWN_CORRUPTER, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_CORRUPTER}, TILE_ERROR
},

{
    MONS_BLACK_SUN, '6', LIGHTMAGENTA, "black sun",
    M_WARM_BLOOD | M_SPEAKS | M_SEE_INVIS | M_SPELLCASTER | M_GENDERED,
    MR_NO_FLAGS,
    10, MONS_DEMONSPAWN, MONS_DEMONSPAWN, MH_NATURAL | MH_DEMONIC, 60,
    { {AT_HIT, AF_PLAIN, 25}, {AT_SHIELD, AF_PLAIN, 25}, AT_NO_ATK, AT_NO_ATK },
    15, 375,
    0, 5, MST_DEMONSPAWN_BLACK_SUN, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_BLACK_SUN}, TILE_ERROR
},

// minor demons: imps, etc. ('5')
{
    MONS_LLAIBWCA, '5', RED, "llaibwca",
    M_SPEAKS | M_FAST_REGEN | M_FLIES | M_GENDERED,
    MR_RES_POISON | MR_RES_FIRE | MR_VUL_COLD | MR_VUL_BLUDGEONING,
    13, MONS_LLAIBWCA, MONS_LLAIBWCA, MH_DEMONIC, 40,
    { {AT_HIT, AF_PLAIN, 4}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    3, 135,
    3, 9, MST_BLINKER, CE_DIVINE, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_WEAPONS | MU_DOOR | MU_WAND), SIZE_LITTLE,
         MON_SHAPE_HUMANOID_WINGED,
    {TILEP_MONS_LLAIBWCA}, TILE_CORPSE_LLAIBWCA
},

{
    MONS_QUASIT, '5', LIGHTGREY, "quasit",
    M_GENDERED,
    MR_RES_POISON | MR_RES_FIRE | MR_RES_COLD | MR_VUL_SLASHING,
    13, MONS_QUASIT, MONS_QUASIT, MH_DEMONIC, 10,
    { {AT_BITE, AF_DRAIN_DEX, 3}, {AT_CLAW, AF_DRAIN_DEX, 2},
      {AT_CLAW, AF_DRAIN_DEX, 2}, AT_NO_ATK },
    3, 150,
    5, 12, MST_NO_SPELLS, CE_DIVINE, S_MOAN,
    I_HUMAN, HT_LAND, 13, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND), SIZE_SMALL, MON_SHAPE_HUMANOID,
    {TILEP_MONS_QUASIT}, TILE_CORPSE_QUASIT
},

{
    MONS_GWYBWCA, '5', WHITE, "gwybwca",
    M_SPEAKS | M_FLIES | M_GENDERED,
    MR_RES_POISON | mrd(MR_RES_COLD, 2) | MR_VUL_FIRE | MR_VUL_BLUDGEONING,
    10, MONS_GWYBWCA, MONS_GWYBWCA, MH_DEMONIC, 10,
    { {AT_HIT, AF_COLD, 4}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    2, 110,
    4, 10, MST_GWYBWCA, CE_DIVINE, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND), SIZE_LITTLE,
         MON_SHAPE_HUMANOID_WINGED,
    {TILEP_MONS_GWYBWCA}, TILE_CORPSE_GWYBWCA
},

{
    MONS_UFETUBUS, '5', LIGHTCYAN, "ufetubus",
    M_GENDERED,
    MR_VUL_FIRE | MR_RES_COLD | MR_VUL_SLASHING,
    28, MONS_UFETUBUS, MONS_UFETUBUS, MH_DEMONIC, 10,
    { {AT_HIT, AF_PLAIN, 5}, {AT_HIT, AF_PLAIN, 5}, AT_NO_ATK, AT_NO_ATK },
    1, 70,
    2, 11, MST_NO_SPELLS, CE_DIVINE, S_SHOUT,
    I_HUMAN, HT_LAND, 15, DEFAULT_ENERGY,
    MU_DOOR, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_UFETUBUS}, TILE_CORPSE_UFETUBUS
},

{
    MONS_HAEARBWCA, '5', CYAN, "haearbwca",
    M_SPEAKS | M_GENDERED,
    MR_RES_POISON | MR_RES_ELEC | mrd(MR_RES_FIRE, 3) | MR_RES_COLD 
     | mrd(MR_RES_NEG, 1) | MR_VUL_ACID | MR_RES_BLUDGEONING | MR_RES_PIERCING,
    14, MONS_HAEARBWCA, MONS_HAEARBWCA, MH_DEMONIC, 10,
    { {AT_HIT, AF_PLAIN, 12}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    3, 165,
    6, 8, MST_NO_SPELLS, CE_DIVINE, S_SHOUT,
    I_HUMAN, HT_STEEL, 8, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_WAND), SIZE_SMALL, MON_SHAPE_HUMANOID,
    {TILEP_MONS_HAEARBWCA}, TILE_CORPSE_HAEARBWCA
},

{
    MONS_CYSGOBWCA, '5', MAGENTA, "cysgobwca",
    M_SEE_INVIS | M_SPEAKS | M_INVIS | M_GENDERED,
    MR_RES_POISON | mrd(MR_RES_COLD, 2) | mrd(MR_RES_NEG, 2)
     | MR_VUL_BLUDGEONING,
    11, MONS_CYSGOBWCA, MONS_CYSGOBWCA, MH_DEMONIC, 10,
    { {AT_HIT, AF_PLAIN, 6}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    2, 110,
    3, 11, MST_CYSGOBWCA, CE_DIVINE, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND), SIZE_LITTLE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_CYSGOBWCA}, TILE_CORPSE_CYSGOBWA
},

// devils etc. ('4')
{
    MONS_TAERINGU, '4', BROWN, "taeringu",
    M_GENDERED,
    MR_RES_POISON | MR_RES_ELEC | mrd(MR_RES_FIRE, 3) | MR_RES_COLD 
     | mrd(MR_RES_NEG, 1) | MR_VUL_ACID | MR_RES_BLUDGEONING,
    15, MONS_TAERINGU, MONS_TAERINGU, MH_DEMONIC, 60,
    { {AT_HIT, AF_CORRODE, 12}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    8, 440,
    10, 8, MST_NO_SPELLS, CE_DIVINE, S_SCREECH,
    I_HUMAN, HT_STEEL, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_TAERINGU}, TILE_CORPSE_TAERINGU
},

{
    MONS_NYCHDOD, '4', LIGHTRED, "nychdod",
    M_GENDERED,
    MR_VUL_SLASHING | MR_RES_PIERCING,
    12, MONS_NYCHDOD, MONS_NYCHDOD, MH_DEMONIC, 60,
    { {AT_STING, AF_REACH_STING, 15}, {AT_HIT, AF_WEAKNESS, 8}, AT_NO_ATK,
       AT_NO_ATK },
    8, 520,
    3, 13, MST_NO_SPELLS, CE_DIVINE, S_SCREECH,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_DOOR, SIZE_MEDIUM, MON_SHAPE_HUMANOID_TAILED,
    {TILEP_MONS_NYCHDOD}, TILE_CORPSE_NYCHDOD
},

{
    MONS_CYTHRAUL, '4', RED, "cythraul",
    M_FIGHTER | M_FLIES | M_GENDERED,
    MR_RES_POISON | mrd(MR_RES_FIRE, 3) | MR_VUL_COLD | MR_VUL_SLASHING,
    13, MONS_CYTHRAUL, MONS_CYTHRAUL, MH_DEMONIC, 60,
    { {AT_HIT, AF_KITE, 19}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    7, 315,
    7, 19, MST_NO_SPELLS, CE_DIVINE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_WEAPONS | MU_WAND), SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_WINGED,
    {TILEP_MONS_CYTHRAUL}, TILE_CORPSE_CYTHRAUL
},

{
    MONS_VLERK, '4', LIGHTGREY, "vlerk",
    M_FAST_REGEN | M_FLIES | M_GENDERED,
    MR_RES_POISON | MR_RES_BLUDGEONING,
    12, MONS_VLERK, MONS_VLERK, MH_DEMONIC, 60,
    { {AT_HIT, AF_SWOOP, 17}, {AT_HIT, AF_PLAIN, 10}, AT_NO_ATK, AT_NO_ATK },
    7, 455,
    16, 21, MST_NO_SPELLS, CE_DIVINE, S_MOAN,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND), SIZE_LARGE, MON_SHAPE_HUMANOID_WINGED,
    {TILEP_MONS_VLERK}, TILE_CORPSE_VLERK
},

{
    MONS_RYPUIWEL, '4', WHITE, "rypuiwel",
    M_GENDERED,
    MR_RES_POISON | MR_VUL_FIRE | mrd(MR_RES_COLD, 3) 
     | MR_VUL_SLASHING | MR_VUL_BLUDGEONING,
    11, MONS_RYPUIWEL, MONS_RYPUIWEL, MH_DEMONIC, 80,
    { {AT_HIT, AF_COLD, 16}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    8, 440,
    12, 17, MST_NO_SPELLS, CE_DIVINE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_WEAPONS | MU_WAND), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_RYPUIWEL}, TILE_CORPSE_RYPUIWEL
},

// middle demons ('3')
{
    MONS_STURNULOS, '3', YELLOW, "sturnulos",
    M_SEE_INVIS | M_FLIES | M_GENDERED,
    MR_RES_ELEC | MR_RES_POISON | MR_VUL_COLD | mrd(MR_RES_FIRE, 3)
     | MR_VUL_WATER | mrd(MR_RES_NEG, 1) | MR_RES_ACID | MR_RES_PHYSICAL,
    14, MONS_STURNULOS, MONS_STURNULOS, MH_DEMONIC, 80,
    { {AT_HIT, AF_FIRE, 30}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    10, 550,
    10, 23, MST_NO_SPELLS, CE_DIVINE, S_SHOUT,
    I_HUMAN, HT_INCORPOREAL, 12, DEFAULT_ENERGY,
    MU_DOOR, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_STURNULOS}, TILE_CORPSE_STURNULOS
},

{
    MONS_SAELNEYTANDI, '3', MAGENTA, "saelneytandi",
    M_SEE_INVIS | M_FLIES | M_GENDERED,
    MR_RES_POISON | MR_RES_COLD | mrd(MR_RES_NEG, 3) | MR_RES_PHYSICAL,
    13, MONS_SAELNEYTANDI, MONS_SAELNEYTANDI, MH_DEMONIC, 140,
    { {AT_HIT, AF_DRAIN_XP, 25}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    11, 605,
    18, 19, MST_SAELNEYTANDI, CE_DIVINE, S_SILENT,
    I_HUMAN, HT_INCORPOREAL, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_SAELNEYTANDI}, TILE_CORPSE_SAELNEYTANDI
},

{
    MONS_MYGDARTH, '3', LIGHTGREY, "mygdarth",
    M_INSUBSTANTIAL | M_UNBREATHING | M_FLIES | M_SPELLCASTER | M_GENDERED,
    MR_RES_POISON | mrd(MR_RES_FIRE, 2) | mrd(MR_RES_ACID,3) | MR_RES_PHYSICAL,
    15, MONS_MYGDARTH, MONS_MYGDARTH, MH_DEMONIC, 60,
    { {AT_HIT, AF_PLAIN, 8}, {AT_HIT, AF_PLAIN, 5}, {AT_HIT, AF_PLAIN, 5},
       AT_NO_ATK },
    7, 385,
    5, 27, MST_MYGDARTH, CE_DIVINE, S_ROAR,
    I_HUMAN, HT_INCORPOREAL, 10, DEFAULT_ENERGY,
    MU_DOOR, SIZE_SMALL, MON_SHAPE_HUMANOID,
    {TILEP_MONS_MYGDARTH}, TILE_CORPSE_MYGDARTH
},

{
    MONS_NEQOXEC, '3', LIGHTMAGENTA, "neqoxec",
    M_FLIES | M_SPELLCASTER | M_GENDERED,
    MR_RES_POISON | MR_RES_ACID | MR_VUL_SLASHING,
    12, MONS_NEQOXEC, MONS_NEQOXEC, MH_DEMONIC, 40,
    { {AT_HIT, AF_PLAIN, 15}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    6, 330,
    4, 17, MST_NEQOXEC, CE_DIVINE, S_SHOUT,
    I_HUMAN, HT_SLIME, 10, DEFAULT_ENERGY,
    muf(MU_WEAPONS | MU_WAND), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_NEQOXEC}, TILE_CORPSE_NEQOXEC
},

{
    MONS_YNOXINUL, '3', LIGHTCYAN, "ynoxinul",
    M_SEE_INVIS | M_FLIES | M_SPELLCASTER | M_GENDERED,
    MR_RES_ELEC | MR_RES_POISON | MR_RES_COLD | MR_VUL_SLASHING,
    13, MONS_YNOXINUL, MONS_YNOXINUL, MH_DEMONIC, 40,
    { {AT_HIT, AF_PLAIN, 12}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    6, 330,
    3, 17, MST_YNOXINUL, CE_DIVINE, S_BELLOW,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND), SIZE_MEDIUM, MON_SHAPE_HUMANOID_WINGED,
    {TILEP_MONS_YNOXINUL}, TILE_CORPSE_YNOXINUL
},

{
    MONS_SIXFIRHY, '3', LIGHTBLUE, "sixfirhy",
    M_GENDERED,
    mrd(MR_RES_ELEC, 3) | MR_VUL_BLUDGEONING,
    6, MONS_SIXFIRHY, MONS_SIXFIRHY, MH_DEMONIC, 60,
    { {AT_HIT, AF_ELEC, 15}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    9, 385,
    2, 33, MST_NO_SPELLS, CE_DIVINE, S_SILENT,
    I_HUMAN, HT_LAND, 40, MOVE_ENERGY(6), // speed is cut to 1/3 later
    MU_DOOR, SIZE_LITTLE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_SIXFIRHY}, TILE_CORPSE_SIXFIRHY
},

// greater demons ('2')

{
    MONS_CYSGODOL, '2', MAGENTA, "cysgodol",
    M_SEE_INVIS | M_INVIS | M_SPELLCASTER | M_GENDERED,
    MR_RES_POISON | mrd(MR_RES_COLD, 2) | mrd(MR_RES_NEG, 2)
     | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    13, MONS_CYSGODOL, MONS_CYSGODOL, MH_DEMONIC, 100,
    { {AT_HIT, AF_PLAIN, 21}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    10, 550,
    7, 21, MST_CYSGODOL, CE_DIVINE, S_CROAK,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND), SIZE_SMALL, MON_SHAPE_HUMANOID,
    {TILEP_MONS_CYSGODOL}, TILE_CORPSE_CYSGODOL
},

{
    MONS_GWYRDD, '2', GREEN, "gwyrdd",
    M_SEE_INVIS | M_SPELLCASTER | M_GENDERED,
    MR_RES_POISON | MR_VUL_SLASHING,
    14, MONS_GWYRDD, MONS_GWYRDD, MH_DEMONIC, 160,
    { {AT_HIT, AF_PLAIN, 32}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    13, 715,
    5, 13, MST_GWYRDD, CE_DIVINE, S_ROAR,
    I_HUMAN, HT_SLIME, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_GWYRDD}, TILE_CORPSE_GWYRDD
},

{
    MONS_CENYSUS, '2', LIGHTBLUE, "cenysus",
    M_SEE_INVIS | M_FLIES | M_SPELLCASTER | M_GENDERED,
    MR_RES_POISON | MR_VUL_FIRE | mrd(MR_RES_COLD, 2) | mrd(MR_RES_ELEC, 3) 
     | MR_RES_ACID | MR_RES_PHYSICAL,
    16, MONS_CENYSUS, MONS_CENYSUS, MH_DEMONIC, 140,
    { {AT_HIT, AF_PLAIN, 20}, {AT_HIT, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    12, 660,
    10, 31, MST_CENYSUS, CE_DIVINE, S_SHOUT,
    I_HUMAN, HT_INCORPOREAL, 10, DEFAULT_ENERGY,
    MU_DOOR, SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_CENYSUS}, TILE_CORPSE_CENYSUS
},

{
    MONS_BALRUG, '2', RED, "balrug",
    M_FIGHTER | M_SEE_INVIS | M_FLIES | M_SPELLCASTER | M_GENDERED,
    MR_RES_POISON | mrd(MR_RES_FIRE, 3) | MR_VUL_COLD | MR_VUL_WATER 
     | MR_VUL_SLASHING | MR_RES_PIERCING,
    12, MONS_BALRUG, MONS_BALRUG, MH_DEMONIC, 160,
    { {AT_HIT, AF_FIRE, 25}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    14, 770,
    5, 21, MST_BALRUG, CE_DIVINE, S_SHOUT,
    I_HUMAN, HT_LAND, 12, DEFAULT_ENERGY,
    muf(MU_WEAPONS | MU_WAND | MU_DOOR), SIZE_LARGE, MON_SHAPE_HUMANOID_WINGED,
    {TILEP_MONS_BALRUG}, TILE_CORPSE_BALRUG
},

{
    MONS_TITIVILUS, '2', YELLOW, "titivilus",
    M_SEE_INVIS | M_FAST_REGEN | M_FLIES | M_SPELLCASTER | M_FIGHTER | M_GENDERED,
    MR_RES_POISON | MR_RES_ELEC | MR_RES_ACID | MR_RES_BLUDGEONING,
    16, MONS_TITIVILUS, MONS_TITIVILUS, MH_DEMONIC, 160,
    { {AT_HIT, AF_PLAIN, 22}, {AT_HIT, AF_PLAIN, 22}, AT_NO_ATK, AT_NO_ATK },
    13, 975,
    11, 21, MST_TITIVILUS, CE_DIVINE, S_SHOUT,
    I_HUMAN, HT_SLIME, 10, DEFAULT_ENERGY,
    muf(MU_WEAPONS | MU_WAND | MU_DOOR), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_TITIVILUS}, TILE_CORPSE_TITIVILUS
},

{
    MONS_HELVITYR, '2', BROWN, "helvityr",
    M_FIGHTER | M_GENDERED,
    MR_VUL_SLASHING,
    17, MONS_HELVITYR, MONS_HELVITYR, MH_DEMONIC, 20,
    { {AT_BITE, AF_PLAIN, 28}, {AT_TRAMPLE, AF_TRAMPLE, 20}, AT_NO_ATK,
       AT_NO_ATK },
    7, 840,
    5, 21, MST_NO_SPELLS, CE_DIVINE, S_RANDOM,
    I_HUMAN, HT_LAND, 15, DEFAULT_ENERGY,
    MU_DOOR, SIZE_LARGE, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_HELVITYR}, TILE_CORPSE_HELVITYR
},

{
    MONS_HELLION, '2', LIGHTRED, "hellion",
    M_SPELLCASTER | M_GENDERED,
    MR_RES_POISON | MR_RES_HELLFIRE | MR_VUL_PIERCING 
     | MR_RES_BLUDGEONING,
    12, MONS_HELLION, MONS_HELLION, MH_DEMONIC, 60,
    { {AT_HIT, AF_PLAIN, 10}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    7, 385,
    5, 23, MST_HELLION, CE_DIVINE, S_SCREAM,
    I_HUMAN, HT_LAND, 12, DEFAULT_ENERGY,
    MU_DOOR, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_HELLION}, TILE_CORPSE_HELLION
},

{
    MONS_LOROCYPROCA, '2', BLUE, "lorocyproca",
    M_SEE_INVIS | M_INVIS | M_GENDERED,
    MR_RES_POISON | MR_RES_COLD | MR_RES_FIRE | MR_RES_ELEC
     | MR_RES_NEG | MR_RES_PHYSICAL,
    14, MONS_LOROCYPROCA, MONS_LOROCYPROCA, MH_DEMONIC, 140,
    { {AT_HIT, AF_DISPEL, 40}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    14, 770,
    10, 18, MST_NO_SPELLS, CE_DIVINE, S_MOAN,
    I_HUMAN, HT_INCORPOREAL, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_LOROCYPROCA}, TILE_CORPSE_LOROCYPROCA
},

{
    MONS_POENYDIO, '2', LIGHTMAGENTA, "poenydio",
    M_SPEAKS | M_SPELLCASTER | M_GENDERED,
    MR_RES_POISON | MR_RES_FIRE | mrd(MR_RES_NEG, 3) | MR_RES_BLUDGEONING,
    10, MONS_POENYDIO, MONS_POENYDIO, MH_DEMONIC, 60,
    { {AT_HIT, AF_PAIN, 8}, {AT_HIT, AF_PAIN, 8}, AT_NO_ATK, AT_NO_ATK },
    7, 385,
    12, 18, MST_POENYDIO, CE_DIVINE, S_ROAR,
    I_HUMAN, HT_LAND, 13, DEFAULT_ENERGY,
    MU_DOOR, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_POENYDIO}, TILE_CORPSE_POENYDIO
},

// fiends, etc. ('1')
{
    MONS_RHUDDGOCH, '1', LIGHTRED, "Rhuddgoch",
    M_SEE_INVIS | M_FLIES | M_SPELLCASTER | M_GENDERED,
    MR_RES_POISON | MR_RES_HELLFIRE | mrd(MR_RES_FIRE, 3) | MR_VUL_COLD
     | MR_VUL_WATER | MR_VUL_SLASHING,
    17, MONS_RHUDDGOCH, MONS_RHUDDGOCH, MH_DEMONIC, 180,
    { {AT_HIT, AF_PLAIN, 25}, {AT_HIT, AF_PLAIN, 15}, {AT_HIT, AF_PLAIN, 15},
       AT_NO_ATK },
    18, 990,
    15, 33, MST_RHUDDGOCH, CE_DIVINE, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_EVOKE), SIZE_LARGE, MON_SHAPE_HUMANOID_WINGED,
    {TILEP_MONS_RHUDDGOCH}, TILE_CORPSE_RHUDDGOCH
},

{
    MONS_VETRIFL, '1', WHITE, "Vetrifl",
    M_SEE_INVIS | M_FLIES | M_SPELLCASTER | M_FIGHTER | M_GENDERED,
    MR_RES_POISON | MR_VUL_FIRE | mrd(MR_RES_COLD, 3) | MR_VUL_SLASHING,
    17, MONS_VETRIFL, MONS_VETRIFL, MH_DEMONIC, 180,
    { {AT_CLAW, AF_COLD, 25}, {AT_CLAW, AF_COLD, 25}, AT_NO_ATK, AT_NO_ATK },
    18, 990,
    15, 33, MST_VETRIFL, CE_DIVINE, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR| MU_EVOKE | MU_WEAPONS), SIZE_LARGE, MON_SHAPE_HUMANOID_WINGED,
    {TILEP_MONS_VETRIFL}, TILE_CORPSE_VETRIFL
},

{
    MONS_CACODEMON, '1', CYAN, "Cacodemon",
    M_SEE_INVIS | M_FLIES | M_SPELLCASTER | M_FIGHTER | M_GENDERED,
    MR_RES_POISON | MR_RES_FIRE | mrd(MR_RES_ELEC, 3)
     | MR_RES_BLUDGEONING | MR_VUL_PIERCING,
    17, MONS_CACODEMON, MONS_CACODEMON, MH_DEMONIC, 180,
    { { AT_BITE, AF_ELEC, 55 }, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    18, 666,
    8, 48, MST_CACODEMON, CE_DIVINE, S_ROAR,
    I_HUMAN, HT_LAND, 12, MOVE_ENERGY(12),
    muf(MU_DOOR), SIZE_GIANT, MON_SHAPE_ORB,
    { TILEP_MONS_CACODEMON }, TILE_CORPSE_CACODEMON
},

{
    MONS_TZITZIMITL, '1', MAGENTA, "Tzitzimitl",
    M_SEE_INVIS | M_FLIES | M_SPELLCASTER | M_FIGHTER | M_GENDERED,
    MR_RES_POISON | mrd(MR_RES_COLD, 2) | MR_RES_ELEC | mrd(MR_RES_NEG, 3)
     | MR_VUL_BLUDGEONING,
    13, MONS_TZITZIMITL, MONS_TZITZIMITL, MH_DEMONIC, 180,
    { {AT_ENGULF, AF_DRAIN_SPEED, 25}, {AT_BITE, AF_ROT, 25},
      AT_NO_ATK, AT_NO_ATK },
    22, 1050,
    12, 38, MST_TZITZIMITL, CE_DIVINE, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_EVOKE), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_TZITZIMITL}, TILE_CORPSE_TZITZIMITL
},

{
    MONS_GWARCHODWR, '1', BROWN, "Gwarchodwr",
    M_SEE_INVIS | M_SPINY | M_SPELLCASTER | M_FIGHTER | M_GENDERED,
    MR_RES_HELLFIRE | MR_VUL_ACID | MR_VUL_PIERCING | MR_RES_BLUDGEONING
     | mrd(MR_RES_FIRE | MR_RES_POISON | MR_RES_COLD | MR_RES_ELEC | MR_RES_NEG, 3),
    10, MONS_GWARCHODWR, MONS_GWARCHODWR, MH_DEMONIC, 180,
    { {AT_HIT, AF_PLAIN, 40}, {AT_HIT, AF_PLAIN, 25}, AT_NO_ATK, AT_NO_ATK },
    19, 1425,
    25, 3, MST_GWARCHODWR, CE_DIVINE, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_EVOKE), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_GWARCHODWR}, TILE_CORPSE_GWARCHODWR
},

{
    MONS_OXARMORDTH, '1', LIGHTGREY, "Oxarmordth",
    M_FIGHTER | M_SEE_INVIS | M_GENDERED,
    MR_RES_ELEC | MR_RES_FIRE | MR_RES_COLD | MR_RES_POISON | mrd(MR_RES_NEG , 3)
     | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    18, MONS_OXARMORDTH, MONS_OXARMORDTH, MH_DEMONIC, 140,
    { {AT_HIT, AF_PLAIN, 30}, {AT_HIT, AF_PLAIN, 10}, {AT_HIT, AF_PLAIN, 10},
       AT_NO_ATK },
    12, 660,
    10, 41, MST_OXARMORDTH, CE_DIVINE, S_SCREAM,
    I_HUMAN, HT_STEEL, 20, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_EVOKE), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_OXARMORDTH}, TILE_CORPSE_OXARMORDTH
},

// trees and related creatures ('7')
DUMMY(MONS_ANIMATED_TREE, '7', ETC_TREE, "animated tree", TILEP_MONS_TREANT)

// BCADDO: Move Kael?

// non-living creatures
// golems ('8')
DUMMY(MONS_GOLEM, '8', LIGHTGREY, "golem", TILEP_MONS_IRON_GOLEM)

{
    MONS_SALTLING, '8', WHITE, "saltling",
    M_NO_FLAGS,
    MR_VUL_ACID | MR_RES_SLASHING | MR_VUL_BLUDGEONING,
    10, MONS_GOLEM, MONS_SALTLING, MH_CONSTRUCT, 160,
    { {AT_HIT, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    8, 270,
    15, 5, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 12, DEFAULT_ENERGY,
    MU_DOOR, SIZE_SMALL, MON_SHAPE_HUMANOID,
    {TILEP_MONS_SALTLING}, TILE_ERROR
},

{
    MONS_PEACEKEEPER, '8', YELLOW, "peacekeeper",
    M_SEE_INVIS,
    MR_RES_FIRE | MR_RES_COLD | MR_RES_ELEC | MR_VUL_ACID
     | MR_RES_SLASHING | MR_RES_BLUDGEONING,
    10, MONS_GOLEM, MONS_PEACEKEEPER, MH_CONSTRUCT, 180,
    { {AT_BITE, AF_PLAIN, 25}, {AT_CLAW, AF_PLAIN, 15}, AT_NO_ATK, AT_NO_ATK },
    12, 450,
    20, 3, MST_PEACEKEEPER, CE_NOCORPSE, S_ROAR,
    I_BRAINLESS, HT_LAND, 15, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_QUADRUPED_TAILLESS,
    {TILEP_MONS_PEACEKEEPER}, TILE_ERROR
},

{
    MONS_IRON_GOLEM, '8', CYAN, "iron golem",
    M_NO_FLAGS,
    mrd(MR_RES_FIRE | MR_RES_COLD | MR_RES_ELEC, 3) | MR_VUL_ACID
     | MR_RES_SLASHING | MR_RES_BLUDGEONING,
    10, MONS_GOLEM, MONS_IRON_GOLEM, MH_CONSTRUCT, 180,
    { {AT_HIT, AF_PLAIN, 35}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    15, 1350,
    15, 3, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 7, DEFAULT_ENERGY,
    MU_DOOR, SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_IRON_GOLEM}, TILE_ERROR
},

{
    MONS_CRYSTAL_GUARDIAN, '8', GREEN, "crystal guardian",
    M_SEE_INVIS | M_SPEAKS,
    mrd(MR_RES_FIRE | MR_RES_COLD | MR_RES_ELEC | MR_RES_ACID, 3)
     | MR_RES_SLASHING | MR_RES_BLUDGEONING,
    10, MONS_GOLEM, MONS_CRYSTAL_GUARDIAN, MH_CONSTRUCT, 200,
    { {AT_HIT, AF_PLAIN, 35}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    16, 640,
    22, 0, MST_CRYSTAL_GUARDIAN, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 10, DEFAULT_ENERGY,
    MU_DOOR, SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_CRYSTAL_GUARDIAN}, TILE_ERROR
},

{
    MONS_TOENAIL_GOLEM, '8', RED, "toenail golem",
    M_NO_FLAGS,
    MR_RES_FIRE | MR_RES_COLD | MR_RES_ELEC | MR_VUL_ACID
    | MR_RES_SLASHING | MR_VUL_BLUDGEONING,
    10, MONS_GOLEM, MONS_TOENAIL_GOLEM, MH_CONSTRUCT, 140,
    { {AT_HIT, AF_PLAIN, 13}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    9, 585,
    8, 5, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 10, DEFAULT_ENERGY,
    MU_DOOR, SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_TOENAIL_GOLEM}, TILE_ERROR
},

{
    MONS_ELECTRIC_GOLEM, '8', LIGHTCYAN, "electric golem",
    M_SEE_INVIS | M_INSUBSTANTIAL | M_SPEAKS,
    mrd(MR_RES_ELEC, 4) | mrd(MR_RES_ACID, 3) | MR_RES_FIRE | MR_RES_COLD
     | MR_RES_PHYSICAL,
    12, MONS_GOLEM, MONS_ELECTRIC_GOLEM, MH_ELEMENTAL, 180,
    { {AT_HIT, AF_ELEC, 15}, {AT_HIT, AF_ELEC, 15}, {AT_HIT, AF_PLAIN, 15},
      {AT_HIT, AF_PLAIN, 15} },
    15, 1350,
    5, 33, MST_ELECTRIC_GOLEM, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_INCORPOREAL, 16, DEFAULT_ENERGY,
    MU_DOOR, SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ELECTRIC_GOLEM}, TILE_ERROR
},

{
    MONS_USHABTI, '8', BROWN, "ushabti",
    M_FIGHTER | M_SEE_INVIS,
    MR_RES_FIRE | MR_RES_COLD | MR_RES_ELEC | MR_VUL_ACID
     | MR_RES_SLASHING,
    10, MONS_GOLEM, MONS_USHABTI, MH_CONSTRUCT, 160,
    { {AT_HIT, AF_PLAIN, 30}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    7, 455,
    9, 6, MST_USHABTI, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_LAND, 8, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LITTLE, MON_SHAPE_MISC,
    {TILEP_MONS_USHABTI}, TILE_ERROR
},

{
    MONS_GUARDIAN_GOLEM, '8', LIGHTGREEN, "guardian golem",
    M_NO_EXP_GAIN,
    MR_RES_FIRE | MR_RES_COLD | MR_RES_ELEC | MR_RES_ACID
     | MR_RES_SLASHING | MR_RES_BLUDGEONING,
    10, MONS_GOLEM, MONS_GUARDIAN_GOLEM, MH_CONSTRUCT, 160,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    5, 400,
    10, 2, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 10, DEFAULT_ENERGY,
    MU_DOOR, SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_GUARDIAN_GOLEM}, TILE_ERROR
},

{
    MONS_SPELLFORGED_SERVITOR, '8', LIGHTMAGENTA, "spellforged servitor",
    M_NO_POLY_TO | M_FLIES | M_NO_EXP_GAIN | M_SPELLCASTER,
    mrd(MR_RES_FIRE | MR_RES_COLD | MR_RES_ACID, 2) | MR_RES_ELEC
     | MR_RES_SLASHING | MR_RES_BLUDGEONING,
    10, MONS_GOLEM, MONS_SPELLFORGED_SERVITOR, MH_CONSTRUCT, 160,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    8, 800,
    10, 17, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_SPELLFORGED_SERVITOR}, TILE_ERROR
},

// statues and statue-like things (also '8')
{
    MONS_ICE_STATUE, '8', LIGHTBLUE, "ice statue",
    M_STATIONARY | M_SPEAKS | M_NO_REGEN | M_SPELLCASTER,
    MR_VUL_FIRE | mrd(MR_RES_COLD, 4) | MR_RES_ELEC | MR_RES_PETRIFY
     | MR_RES_SLASHING | MR_VUL_BLUDGEONING,
    10, MONS_STATUE, MONS_ICE_STATUE, MH_CONSTRUCT, 160,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    8, 720,
    12, 1, MST_ICE_STATUE, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_AMPHIBIOUS, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ICE_STATUE}, TILE_ERROR
},

{
    MONS_OBSIDIAN_STATUE, '8', MAGENTA, "obsidian statue",
    M_STATIONARY | M_SPEAKS | M_SEE_INVIS | M_NO_REGEN,
    mrd(MR_RES_FIRE | MR_RES_COLD, 2) | MR_RES_ELEC | MR_RES_PETRIFY 
     | MR_VUL_ACID | MR_RES_SLASHING | MR_RES_BLUDGEONING,
    10, MONS_STATUE, MONS_OBSIDIAN_STATUE, MH_CONSTRUCT | MH_EVIL, 160,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    10, 700,
    12, 1, MST_OBSIDIAN_STATUE, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_OBSIDIAN_STATUE}, TILE_ERROR
},

{
    MONS_ORANGE_STATUE, '8', LIGHTRED, "orange crystal statue",
    M_STATIONARY | M_SPEAKS | M_SEE_INVIS | M_NO_REGEN,
    mrd(MR_RES_FIRE | MR_RES_COLD, 2) | MR_RES_ELEC | MR_RES_PETRIFY
     | MR_VUL_ACID | MR_RES_SLASHING | MR_RES_BLUDGEONING,
    10, MONS_STATUE, MONS_ORANGE_STATUE, MH_CONSTRUCT, 180,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    10, 700,
    12, 1, MST_ORANGE_CRYSTAL_STATUE, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ORANGE_STATUE}, TILE_ERROR
},

{ // always redefined
    MONS_STATUE, '8', LIGHTGREY, "statue",
    M_STATIONARY | M_SPEAKS | M_ARCHER | M_PREFER_RANGED | M_NO_POLY_TO | M_NO_REGEN,
    mrd(MR_RES_FIRE | MR_RES_COLD, 2) | MR_RES_ELEC | MR_RES_PETRIFY | MR_VUL_ACID
     | MR_RES_SLASHING | MR_RES_BLUDGEONING,
    10, MONS_STATUE, MONS_STATUE, MH_CONSTRUCT, 120,
    { {AT_WEAP_ONLY, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    8, 720,
    12, 1, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_START_ONLY | MU_ALL), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_OBSIDIAN_STATUE}, TILE_ERROR
},

{
    MONS_TRAINING_DUMMY, '8', LIGHTGREY, "training dummy",
    M_STATIONARY,
    MR_RES_PETRIFY,
    10, MONS_TRAINING_DUMMY, MONS_TRAINING_DUMMY, MH_CONSTRUCT, 90,
    { {AT_WEAP_ONLY, AF_PLAIN, 1}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 60,
    0, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_START_ONLY | MU_ALL), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_TRAINING_DUMMY}, TILE_ERROR
},

{
    MONS_PILLAR_OF_SALT, '8', WHITE, "pillar of salt",
    M_NOT_DANGEROUS | M_STATIONARY,
    MR_VUL_ACID | MR_RES_SLASHING | MR_VUL_BLUDGEONING,
    10, MONS_PILLAR_OF_SALT, MONS_PILLAR_OF_SALT, MH_CONSTRUCT, MAG_IMMUNE,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 10,
    1, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_MISC,
    {TILEP_MONS_PILLAR_OF_SALT}, TILE_ERROR
},

{
    MONS_BLOCK_OF_ICE, '8', ETC_ICE, "block of ice",
    M_NOT_DANGEROUS | M_STATIONARY,
    mrd(MR_RES_COLD, 3) | MR_VUL_FIRE | MR_RES_SLASHING 
     | MR_VUL_BLUDGEONING,
    10, MONS_BLOCK_OF_ICE, MONS_BLOCK_OF_ICE, MH_CONSTRUCT, MAG_IMMUNE,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    3, 285,
    15, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_MISC,
    {TILEP_MONS_BLOCK_OF_ICE, TVARY_MOD}, TILE_ERROR
},

{
    MONS_DIAMOND_OBELISK, '8', WHITE, "diamond obelisk",
    M_STATIONARY | M_NO_EXP_GAIN | M_NO_POLY_TO,
    MR_RES_PETRIFY | mrd(MR_RES_ACID, 3) | MR_RES_SLASHING | MR_RES_BLUDGEONING,
    10, MONS_DIAMOND_OBELISK, MONS_DIAMOND_OBELISK, MH_CONSTRUCT, MAG_IMMUNE,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    8, 96000,
    12, 1, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_LARGE, MON_SHAPE_MISC,
    {TILEP_MONS_DIAMOND_OBELISK}, TILE_ERROR
},

{
    MONS_LIGHTNING_SPIRE, '8', ETC_ELECTRICITY, "lightning spire",
    M_STATIONARY | M_NO_POLY_TO,
    mrd(MR_RES_FIRE | MR_RES_COLD, 2) | mrd(MR_RES_ELEC, 3) | MR_RES_PETRIFY
        | MR_VUL_ACID | MR_RES_SLASHING,
    10, MONS_LIGHTNING_SPIRE, MONS_LIGHTNING_SPIRE, MH_CONSTRUCT, 160,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    3, 210,
    13, 3, MST_ZAPPER, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_SMALL, MON_SHAPE_MISC,
    {TILEP_MONS_LIGHTNING_SPIRE}, TILE_ERROR
},

// gargoyles ('9')
{
    MONS_GARGOYLE, '9', LIGHTGREY, "gargoyle",
    M_FLIES | M_GENDERED,
    MR_RES_ELEC | MR_RES_PETRIFY | MR_VUL_ACID | MR_RES_BLUDGEONING | MR_RES_SLASHING,
    26, MONS_GARGOYLE, MONS_GARGOYLE, MH_CONSTRUCT | MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    6, 270,
    18, 6, MST_GARGOYLE, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_WAND), SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_WINGED_TAILED,
    {TILEP_MONS_GARGOYLE}, TILE_ERROR
},

{
    MONS_WAR_GARGOYLE, '9', CYAN, "war gargoyle",
    M_SEE_INVIS | M_FIGHTER | M_FLIES | M_GENDERED,
    MR_RES_ELEC | MR_RES_FIRE | MR_RES_COLD | MR_RES_PETRIFY
     | MR_VUL_ACID | MR_RES_BLUDGEONING | MR_RES_SLASHING,
    18, MONS_GARGOYLE, MONS_WAR_GARGOYLE, MH_CONSTRUCT | MH_NATURAL, 100,
    { {AT_HIT, AF_PLAIN, 30}, {AT_SHIELD, AF_PLAIN, 30}, AT_NO_ATK, AT_NO_ATK },
    13, 520,
    22, 4, MST_SPLINTER_BREATH, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_LAND, 14, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_WAND), SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_WINGED_TAILED,
    {TILEP_MONS_WAR_GARGOYLE}, TILE_ERROR
},

{
    MONS_MOLTEN_GARGOYLE, '9', RED, "molten gargoyle",
    M_FLIES | M_GENDERED,
    MR_RES_ELEC | mrd(MR_RES_FIRE, 3) | MR_RES_PETRIFY 
     | MR_VUL_ACID | MR_RES_BLUDGEONING | MR_RES_SLASHING,
    18, MONS_GARGOYLE, MONS_MOLTEN_GARGOYLE, MH_CONSTRUCT | MH_NATURAL, 60,
    { {AT_HIT, AF_FIRE, 20}, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    7, 315,
    14, 11, MST_MOLTEN_GARGOYLE, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_WAND), SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_WINGED_TAILED,
    {TILEP_MONS_MOLTEN_GARGOYLE}, TILE_ERROR
},

// major demons ('&')
// Random demon in pan - only one per level. Stats are stored in ghost struct.
// BCADDO: Randomize physical resists too?
{
    MONS_PANDEMONIUM_LORD, '&', COLOUR_UNDEF, "pandemonium lord",
    // See invis is also set in ghost.cc
    M_FIGHTER | M_SPEAKS | M_SEE_INVIS | M_GHOST_DEMON | M_TALL_TILE | M_GENDERED,
    MR_RES_POISON,
    14, MONS_PANDEMONIUM_LORD, MONS_PANDEMONIUM_LORD, MH_DEMONIC, -5,
    { {AT_HIT, AF_PLAIN, 5}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    19, 1710,
    1, 2, MST_GHOST, CE_DIVINE, S_DEMON_TAUNT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND | MU_EVOKE), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_PANDEMONIUM_LORD}, TILE_CORPSE_PANDEMONIUM_LORD // BCADDO: Can random TILE_CORPSE?
},

DUMMY(MONS_HELL_LORD, '&', COLOUR_UNDEF, "hell lord", TILEP_MONS_PROGRAM_BUG)

// explodey things / orb of fire ('*')
{
    MONS_BALL_LIGHTNING, '*', LIGHTCYAN, "ball lightning",
    M_INSUBSTANTIAL | M_CONJURED | M_FLIES,
    mrd(MR_RES_ELEC | MR_RES_FIRE | MR_RES_COLD | MR_RES_ACID, 3) | MR_RES_WIND 
     | MR_RES_PHYSICAL,
    20, MONS_BALL_LIGHTNING, MONS_BALL_LIGHTNING, MH_ELEMENTAL, MAG_IMMUNE,
    { {AT_HIT, AF_PLAIN, 5}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 10,
    0, 18, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 20, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_LITTLE, MON_SHAPE_ORB,
    {TILEP_MONS_BALL_LIGHTNING}, TILE_ERROR
},

{
    MONS_ENTROPIC_SPHERE, '*', ETC_CHAOS, "entropic sphere",
    M_INSUBSTANTIAL | M_CONJURED | M_FLIES,
    mrd(MR_RES_ELEC | MR_RES_FIRE | MR_RES_COLD | MR_RES_ACID, 3) | MR_RES_WIND
     | MR_RES_PHYSICAL,
    20, MONS_BALL_LIGHTNING, MONS_BALL_LIGHTNING, MH_ELEMENTAL, MAG_IMMUNE,
    { { AT_HIT, AF_PLAIN, 5 }, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 10,
    0, 19, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 20, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_LITTLE, MON_SHAPE_ORB,
    { TILEP_MONS_ENTROPIC_SPHERE }, TILE_ERROR
},

{
    MONS_BALLISTOMYCETE_SPORE, '*', GREEN, "ballistomycete spore",
    M_FLIES | M_NO_EXP_GAIN,
    MR_RES_POISON,
    10, MONS_FUNGUS, MONS_BALLISTOMYCETE_SPORE, MH_PLANT, MAG_IMMUNE,
    { {AT_HIT, AF_PLAIN, 1}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 10,
    0, 11, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 15, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_SMALL, MON_SHAPE_ORB,
    {TILEP_MONS_BALLISTOMYCETE_SPORE}, TILE_ERROR
},

{
    MONS_LOST_SOUL, '*', LIGHTGREEN, "lost soul",
    M_INSUBSTANTIAL | M_MAINTAIN_RANGE | M_FLIES | M_NO_EXP_GAIN | M_SEE_INVIS,
    MR_RES_ELEC | MR_RES_FIRE | MR_RES_COLD | mrd(MR_RES_ACID, 3) | MR_RES_PHYSICAL,
    2, MONS_LOST_SOUL, MONS_LOST_SOUL, MH_UNDEAD, 120,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    10, 250,
    0, 21, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_INCORPOREAL, 13, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_SMALL, MON_SHAPE_ORB,
    {TILEP_MONS_LOST_SOUL}, TILE_ERROR
},

{
    MONS_LURKING_HORROR, '*', BLUE, "lurking horror",
    M_INSUBSTANTIAL | M_FLIES | M_NO_EXP_GAIN,
    MR_RES_PHYSICAL,
    10, MONS_LURKING_HORROR, MONS_LURKING_HORROR, MH_ELEMENTAL, 10,
    { {AT_HIT, AF_PLAIN, 1}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 10,
    0, 21, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_INCORPOREAL, 12, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_SMALL, MON_SHAPE_ORB,
    {TILEP_MONS_LURKING_HORROR}, TILE_ERROR
},

{
    MONS_ORB_OF_FIRE, '*', RED, "orb of fire",
    M_SEE_INVIS | M_INSUBSTANTIAL | M_FLIES | M_SPELLCASTER,
    mrd(MR_RES_FIRE, 4) | mrd(MR_RES_ELEC, 3) | MR_VUL_COLD | MR_VUL_WATER 
     | MR_RES_ACID | MR_RES_PHYSICAL,
    13, MONS_ORB_OF_FIRE, MONS_ORB_OF_FIRE, MH_ELEMENTAL, 190,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    30, 1400,
    18, 41, MST_ORB_OF_FIRE, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_LAND, 15, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LITTLE, MON_SHAPE_ORB,
    {TILEP_MONS_ORB_OF_FIRE}, TILE_ERROR
},

{   // Monster equivalent of Player Fairy Race for Vaults.
    MONS_FAIRY, '*', ETC_JEWEL, "fairy",
    M_FLIES | M_NOT_DANGEROUS | M_CONFUSED | M_NO_POLY_TO | M_GENDERED,
    MR_VUL_POISON | MR_RES_ACID,
    10, MONS_FAIRY, MONS_FAIRY, MH_HOLY, 10,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 35,
    0, 25, MST_NO_SPELLS, CE_NOCORPSE, S_SHOUT,
    I_HUMAN, HT_LAND, 8, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_INSECT_WINGED,
    {TILEP_MONS_FAIRY, TVARY_MOD}, TILE_ERROR
},

{ // not an actual monster, used by a spell
    MONS_ORB_OF_DESTRUCTION, '*', WHITE, "ball of snow and ice",
    M_INSUBSTANTIAL | M_NOT_DANGEROUS | M_NO_POLY_TO | M_PROJECTILE | M_FLIES,
    mrd(MR_RES_COLD | MR_RES_ELEC | MR_RES_ACID | MR_RES_FIRE | MR_RES_ACID, 3)
     | MR_RES_HELLFIRE | MR_RES_STICKY_FLAME | MR_RES_PHYSICAL,
    0, MONS_ORB_OF_DESTRUCTION, MONS_ORB_OF_DESTRUCTION,
     MH_ELEMENTAL, MAG_IMMUNE,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    5, 50000, /* unkillable */
    0, 10, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 30, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_LITTLE, MON_SHAPE_ORB,
    {TILEP_MONS_ORB_OF_DESTRUCTION, TVARY_RANDOM}, TILE_ERROR
},

{ // not an actual monster, used by a spell
    MONS_ORB_OF_CHAOS, '*', WHITE, "ball of raw entropy",
    M_INSUBSTANTIAL | M_NOT_DANGEROUS | M_NO_POLY_TO | M_PROJECTILE | M_FLIES,
    mrd(MR_RES_COLD | MR_RES_ELEC | MR_RES_ACID | MR_RES_FIRE | MR_RES_ACID, 3)
     | MR_RES_HELLFIRE | MR_RES_STICKY_FLAME | MR_RES_PHYSICAL,
    0, MONS_ORB_OF_DESTRUCTION, MONS_ORB_OF_CHAOS,
     MH_ELEMENTAL, MAG_IMMUNE,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    5, 50000, /* unkillable */
    0, 10, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 30, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_LITTLE, MON_SHAPE_ORB,
    {TILEP_MONS_ORB_OF_CHAOS, TVARY_RANDOM}, TILE_ERROR
},

{ // not an actual monster, used by a spell
    MONS_FOXFIRE, '*', LIGHTRED, "foxfire",
    M_SEE_INVIS | M_INSUBSTANTIAL | M_NOT_DANGEROUS | M_NO_POLY_TO
     | M_CONJURED | M_FLIES,
    mrd(MR_RES_FIRE, 3) | MR_RES_STICKY_FLAME | MR_RES_PHYSICAL,
    0, MONS_FOXFIRE, MONS_FOXFIRE, MH_ELEMENTAL, MAG_IMMUNE,
    { {AT_HIT, AF_PLAIN, 1}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 10, /* unkillable */
    0, 10, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 50, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_LITTLE, MON_SHAPE_ORB,
    {TILEP_MONS_FOXFIRE}, TILE_ERROR
},

{ // not an actual monster, used by a spell
    MONS_EPHEMERAL_SPIRIT, '*', ETC_JEWEL, "ephemeral spirit",
    M_SEE_INVIS | M_INSUBSTANTIAL | M_NOT_DANGEROUS | M_NO_POLY_TO
     | M_CONJURED | M_FLIES,
    mrd(MR_RES_FIRE, 3) | MR_RES_STICKY_FLAME | MR_RES_PHYSICAL,
    0, MONS_FOXFIRE, MONS_EPHEMERAL_SPIRIT, MH_ELEMENTAL, MAG_IMMUNE,
    { { AT_HIT, AF_PLAIN, 1 }, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 10, /* unkillable */
    0, 10, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 50, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_LITTLE, MON_SHAPE_ORB,
    { TILEP_MONS_EPHEMERAL_SPIRIT }, TILE_ERROR
},

{ // not an actual monster, used by a spell
    MONS_FULMINANT_PRISM, '*', ETC_MAGIC, "fulminant prism",
    M_NO_POLY_TO | M_STATIONARY | M_CONJURED | M_NO_EXP_GAIN
     | M_FLIES,
    MR_RES_FIRE | MR_RES_COLD | MR_RES_ELEC | MR_RES_ACID | MR_VUL_BLUDGEONING,
    0, MONS_FULMINANT_PRISM, MONS_FULMINANT_PRISM, MH_CONSTRUCT, MAG_IMMUNE,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    5, 150,
    3, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_LITTLE, MON_SHAPE_ORB,
    {TILEP_MONS_FULMINANT_PRISM, TVARY_RANDOM}, TILE_ERROR
},

{
    MONS_WRETCHED_STAR, '*', MAGENTA, "wretched star",
    M_SEE_INVIS | M_INSUBSTANTIAL | M_FLIES | M_SPELLCASTER,
    MR_RES_ELEC | MR_RES_FIRE | MR_RES_COLD | mrd(MR_RES_ACID, 2) | MR_RES_SLASHING,
    13, MONS_WRETCHED_STAR, MONS_WRETCHED_STAR, MH_ELEMENTAL | MH_CONSTRUCT, 160,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    10, 700,
    10, 21, MST_WRETCHED_STAR, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(8),
    MU_NOTHING, SIZE_LITTLE, MON_SHAPE_ORB,
    {TILEP_MONS_WRETCHED_STAR}, TILE_ERROR
},

{
    MONS_SILVER_STAR, '*', ETC_SILVER, "silver star",
    M_SEE_INVIS | M_FLIES | M_SPELLCASTER,
    mrd(MR_RES_FIRE | MR_RES_COLD | MR_RES_ELEC, 3) | MR_VUL_ACID | MR_RES_SLASHING,
    13, MONS_SILVER_STAR, MONS_SILVER_STAR, MH_ELEMENTAL | MH_CONSTRUCT, 180,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    12, 420,
    30, 16, MST_SILVER_STAR, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(8),
    MU_NOTHING, SIZE_LITTLE, MON_SHAPE_ORB,
    {TILEP_MONS_SILVER_STAR}, TILE_ERROR
},

{
    MONS_BATTLESPHERE, '*', ETC_MAGIC, "battlesphere",
    M_SEE_INVIS | M_INSUBSTANTIAL | M_NO_EXP_GAIN | M_NO_POLY_TO
        | M_MAINTAIN_RANGE | M_AVATAR | M_FLIES | M_UNBLINDABLE,
    mrd(MR_RES_COLD | MR_RES_FIRE, 2) | mrd(MR_RES_ELEC, 3)
        | MR_RES_ACID | MR_RES_STICKY_FLAME | MR_RES_PHYSICAL,
    0, MONS_BATTLESPHERE, MONS_BATTLESPHERE, MH_CONSTRUCT, MAG_IMMUNE,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    5, 150,
    0, 5, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_LAND, 30, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_LITTLE, MON_SHAPE_ORB,
    {TILEP_MONS_BATTLESPHERE}, TILE_ERROR
},

// other symbols
{
    MONS_DEATH_COB, 'z', YELLOW, "death cob",
    M_SPEAKS,
    MR_RES_COLD | MR_RES_PIERCING | MR_VUL_SLASHING,
    10, MONS_DEATH_COB, MONS_DEATH_COB, MH_UNDEAD, 40,
    { {AT_HIT, AF_DRAIN_SPEED, 35}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    14, 910,
    10, 24, MST_NO_SPELLS, CE_NOCORPSE, S_MOAN,
    I_HUMAN, HT_LAND, 25, DEFAULT_ENERGY,
    MU_DOOR, SIZE_TINY, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DEATH_COB}, TILE_ERROR
},

// non-human uniques
// "A"ngels.
{
    MONS_MENNAS, 'A', LIGHTCYAN, "Mennas",
    M_FIGHTER | M_SEE_INVIS | M_SPEAKS | M_UNIQUE | M_MALE | M_FLIES,
    MR_RES_POISON | MR_RES_ELEC | MR_RES_PIERCING | MR_RES_SLASHING,
    18, MONS_ANGEL, MONS_ANGEL, MH_HOLY, 160,
    { {AT_HIT, AF_PLAIN, 30}, {AT_SHIELD, AF_PLAIN, 30}, 
      {AT_HIT, AF_PLAIN, 20}, AT_NO_ATK },
    19, 1520,
    15, 44, MST_MENNAS, CE_DIVINE, S_SILENT,
    I_HUMAN, HT_LAND, 15, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_WAND), SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_WINGED,
    {TILEP_MONS_MENNAS}, TILE_ERROR
},

// "c"entaurs.
{
    MONS_NESSOS, 'c', MAGENTA, "Nessos",
    M_UNIQUE | M_WARM_BLOOD | M_ARCHER | M_SPEAKS | M_MALE,
    MR_VUL_SLASHING,
    18, MONS_CENTAUR, MONS_CENTAUR, MH_NATURAL, 20,
    { {AT_HIT, AF_PLAIN, 16}, {AT_SHIELD, AF_PLAIN, 16}, AT_NO_ATK, AT_NO_ATK },
    9, 720,
    4, 11, MST_NESSOS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(7),
    MU_ALL, SIZE_LARGE, MON_SHAPE_CENTAUR,
    {TILEP_MONS_NESSOS}, TILE_ERROR
},

// "C"yclopes and giants.
{
    MONS_CHUCK, 'C', WHITE, "Chuck",
    M_WARM_BLOOD | M_SPEAKS | M_UNIQUE | M_MALE | M_ARCHER,
    MR_RES_BLUDGEONING,
    10, MONS_GIANT, MONS_STONE_GIANT, MH_NATURAL, 100,
    { {AT_HIT, AF_PLAIN, 45}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    18, 1170,
    14, 2, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_START_ONLY | MU_ALL), SIZE_GIANT, MON_SHAPE_HUMANOID,
    {TILEP_MONS_CHUCK}, TILE_ERROR
},

{
    MONS_POLYPHEMUS, 'C', GREEN, "Polyphemus",
    M_UNIQUE | M_WARM_BLOOD | M_SPEAKS | M_MALE | M_ARCHER,
    MR_VUL_SLASHING,
    10, MONS_GIANT, MONS_CYCLOPS, MH_NATURAL, 60,
    { {AT_HIT, AF_PLAIN, 45}, {AT_HIT, AF_PLAIN, 30}, AT_NO_ATK, AT_NO_ATK },
    16, 1520,
    10, 3, MST_POLYPHEMUS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ARMOUR | MU_EVOKE | MU_WEAPON_RANGED | MU_DOOR), SIZE_GIANT, MON_SHAPE_HUMANOID,
    {TILEP_MONS_POLYPHEMUS}, TILE_ERROR
},

{
    MONS_ANTAEUS, 'C', LIGHTCYAN, "Antaeus",
    M_UNIQUE | M_WARM_BLOOD | M_FIGHTER | M_SEE_INVIS | M_MALE
        | M_SPEAKS | M_TALL_TILE,
    mrd(MR_RES_ELEC, 3) | mrd(MR_RES_COLD, 2) | MR_VUL_FIRE 
     | MR_RES_BLUDGEONING | MR_RES_PIERCING,
    10, MONS_GIANT, MONS_TITAN, MH_DEMONIC, 180,
    { {AT_HIT, AF_COLD, 75}, {AT_SHIELD, AF_PLAIN, 75}, {AT_HIT, AF_COLD, 30}, AT_NO_ATK },
    22, 6820,
    28, 4, MST_ANTAEUS, CE_DIVINE, S_DEMON_TAUNT,
    I_HUMAN, HT_AMPHIBIOUS, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE | MU_THROW_NET), SIZE_GIANT, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ANTAEUS}, TILE_CORPSE_ANTAEUS
},

// "d"raconians.
{
    MONS_LIVARRA, 'd', COLOUR_UNDEF, "Livarra",
    M_UNIQUE | M_SEE_INVIS | M_COLD_BLOOD | M_SPEAKS | M_FEMALE | M_FLIES,
    MR_RES_POISON | MR_VUL_PIERCING,
    10, MONS_DRACONIAN, MONS_DRACONIAN, MH_NATURAL, 140,
    { {AT_HIT, AF_PLAIN, 60}, {AT_SHIELD, AF_PLAIN, 60}, 
      {AT_TAIL_SLAP, AF_PLAIN, 45}, AT_NO_ATK },
    22, 3850,
    // Gets her breath in
    30, 21, MST_NO_SPELLS, CE_NOCORPSE, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN), SIZE_MEDIUM,
        MON_SHAPE_HUMANOID_WINGED_TAILED,
    {TILEP_MONS_PROGRAM_BUG}, TILE_ERROR
},

{
    MONS_BAI_SUZHEN, 'd', LIGHTBLUE, "Bai Suzhen",
    M_UNIQUE | M_SEE_INVIS | M_COLD_BLOOD | M_SPEAKS | M_FEMALE,
    mrd(MR_RES_ELEC, 3) | MR_RES_COLD | MR_RES_POISON 
     | MR_VUL_SLASHING | MR_VUL_PIERCING,
    16, MONS_DRAGON, MONS_STORM_DRAGON, MH_NATURAL, 100,
    { {AT_HIT, AF_PLAIN, 24}, {AT_SHIELD, AF_PLAIN, 24},
      {AT_TAIL_SLAP, AF_PLAIN, 14}, AT_NO_ATK },
    20, 1850,
    14, 14, MST_BAI_SUZHEN, CE_NOCORPSE, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID_TAILED,
    {TILEP_MONS_BAI_SUZHEN}, TILE_ERROR
},

// "D"ragons and hydras.
{
    MONS_XTAHUA, 'D', RED, "Xtahua",
    M_UNIQUE | M_SEE_INVIS | M_WARM_BLOOD | M_SPEAKS | M_FEMALE
     | M_CRASH_DOORS | M_FLIES | M_ALWAYS_WAND,
    MR_RES_POISON | mrd(MR_RES_FIRE, 2) | MR_VUL_COLD 
     | MR_VUL_PIERCING,
    18, MONS_DRAGON, MONS_FIRE_DRAGON, MH_NATURAL, 180,
    { {AT_BITE, AF_PLAIN, 35}, {AT_CLAW, AF_PLAIN, 17},
      {AT_TRAMPLE, AF_TRAMPLE, 20}, AT_NO_ATK },
    20, 1330,
    15, 22, MST_XTAHUA, CE_CLEAN, S_LOUD_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_WAND, SIZE_GIANT, MON_SHAPE_QUADRUPED_WINGED,
    {TILEP_MONS_XTAHUA}, TILE_ERROR
},

{
    MONS_LERNAEAN_HYDRA, 'D', YELLOW, "the Lernaean hydra",
    M_UNIQUE | M_COLD_BLOOD | M_FAST_REGEN | M_TALL_TILE,
    MR_RES_POISON | MR_VUL_PIERCING,
    11, MONS_HYDRA, MONS_HYDRA, MH_NATURAL, 120,
    { {AT_BITE, AF_PLAIN, 18}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    30, 1500,
    0, 5, MST_NO_SPELLS, CE_NOXIOUS, S_LOUD_ROAR,
    I_ANIMAL, HT_AMPHIBIOUS, 10, SWIM_ENERGY(6),
    MU_NOTHING, SIZE_GIANT, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_PROGRAM_BUG}, TILE_ERROR
},

{
    MONS_SERPENT_OF_HELL, 'D', ETC_FIRE, "the Serpent of Hell",
    M_SEE_INVIS | M_UNIQUE | M_CRASH_DOORS | M_FLIES | M_TALL_TILE,
    MR_RES_POISON | MR_RES_HELLFIRE | mrd(MR_RES_FIRE, 3) | MR_RES_ACID
     | MR_RES_BLUDGEONING | MR_VUL_PIERCING,
    21, MONS_DRAGON, MONS_SERPENT_OF_HELL, MH_DEMONIC, 180,
    { {AT_MULTIBITE, AF_FIRE, 35}, {AT_CLAW, AF_PLAIN, 15},
      {AT_TRAMPLE, AF_TRAMPLE, 15}, AT_NO_ATK },
    17, 2805,
    16, 18, MST_SERPENT_OF_HELL_GEH, CE_DIVINE, S_LOUD_ROAR,
    I_HUMAN, HT_LAND, 14, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_GIANT, MON_SHAPE_QUADRUPED_WINGED,
    {TILEP_MONS_SERPENT_OF_HELL_GEHENNA}, TILE_CORPSE_SERPENT_OF_HELL_GEH
},

{
    MONS_SERPENT_OF_HELL_COCYTUS, 'D', ETC_ICE, "the Serpent of Hell",
    M_SEE_INVIS | M_UNIQUE | M_CRASH_DOORS | M_FLIES | M_TALL_TILE,
    MR_RES_POISON | mrd(MR_RES_COLD, 3) | MR_RES_ACID
     | MR_RES_BLUDGEONING | MR_VUL_PIERCING,
    21, MONS_DRAGON, MONS_SERPENT_OF_HELL, MH_DEMONIC, 180,
    { {AT_MULTIBITE, AF_COLD, 35}, {AT_CLAW, AF_PLAIN, 15},
      {AT_TRAMPLE, AF_TRAMPLE, 15}, AT_NO_ATK },
    17, 2805,
    20, 18, MST_SERPENT_OF_HELL_COC, CE_DIVINE, S_LOUD_ROAR,
    I_HUMAN, HT_LAND, 14, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_GIANT, MON_SHAPE_QUADRUPED_WINGED,
    {TILEP_MONS_SERPENT_OF_HELL_COCYTUS}, TILE_CORPSE_SERPENT_OF_HELL_COC
},

{
    MONS_SERPENT_OF_HELL_DIS, 'D', ETC_IRON, "the Serpent of Hell",
    M_SEE_INVIS | M_UNIQUE | M_CRASH_DOORS | M_TALL_TILE,
    MR_RES_POISON | MR_RES_FIRE | MR_RES_COLD | mrd(MR_RES_NEG , 1)
     | MR_VUL_ACID | MR_RES_BLUDGEONING | MR_VUL_PIERCING,
    16, MONS_DRAGON, MONS_SERPENT_OF_HELL, MH_DEMONIC, 180,
    { {AT_MULTIBITE, AF_REACH, 35}, {AT_CLAW, AF_PLAIN, 25},
      {AT_TRAMPLE, AF_TRAMPLE, 25}, {AT_TAIL_SLAP, AF_PLAIN, 15} },
    17, 3230,
    30, 11, MST_SERPENT_OF_HELL_DIS, CE_DIVINE, S_LOUD_ROAR,
    I_HUMAN, HT_LAND, 14, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_GIANT, MON_SHAPE_QUADRUPED_WINGED,
    {TILEP_MONS_SERPENT_OF_HELL_DIS}, TILE_CORPSE_SERPENT_OF_HELL_DIS
},

{
    MONS_SERPENT_OF_HELL_TARTARUS, 'D', ETC_DEATH, "the Serpent of Hell",
    M_SEE_INVIS | M_UNIQUE | M_CRASH_DOORS | M_FLIES | M_TALL_TILE,
    MR_RES_POISON | mrd(MR_RES_COLD, 2) | mrd(MR_RES_NEG , 3)
     | MR_RES_BLUDGEONING | MR_VUL_PIERCING,
    18, MONS_DRAGON, MONS_SERPENT_OF_HELL, MH_DEMONIC, 180,
    { {AT_MULTIBITE, AF_DRAIN_XP, 35}, {AT_CLAW, AF_PLAIN, 15},
      {AT_TRAMPLE, AF_TRAMPLE, 15}, AT_NO_ATK },
    17, 3230,
    16, 18, MST_SERPENT_OF_HELL_TAR, CE_DIVINE, S_LOUD_ROAR,
    I_HUMAN, HT_LAND, 14, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_GIANT, MON_SHAPE_QUADRUPED_WINGED,
    {TILEP_MONS_SERPENT_OF_HELL_TARTARUS}, TILE_CORPSE_SERPENT_OF_HELL_TAR
},

{
    MONS_BAI_SUZHEN_DRAGON, 'D', ETC_ELECTRICITY, "Bai Suzhen",
    M_UNIQUE | M_SEE_INVIS | M_COLD_BLOOD | M_SPEAKS | M_FEMALE | M_FLIES
        | M_THUNDER_RING,
    mrd(MR_RES_ELEC, 3) | MR_RES_COLD | MR_RES_POISON | MR_VUL_PIERCING,
    18, MONS_DRAGON, MONS_STORM_DRAGON, MH_NATURAL, 100,
    { {AT_BITE, AF_PLAIN, 30}, {AT_CLAW, AF_PLAIN, 16},
      {AT_TRAMPLE, AF_TRAMPLE, 16}, AT_NO_ATK },
    20, 1850,
    22, 22, MST_BAI_SUZHEN_DRAGON, CE_NOCORPSE, S_LOUD_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND), SIZE_GIANT, MON_SHAPE_SNAKE,
    {TILEP_MONS_BAI_SUZHEN_DRAGON}, TILE_ERROR
},

// "e"lves
{
    MONS_DUVESSA, 'e', BLUE, "Duvessa",
    M_UNIQUE | M_FIGHTER | M_WARM_BLOOD | M_SPEAKS | M_FEMALE,
    MR_VUL_SLASHING,
    15, MONS_ELF, MONS_ELF, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 10}, {AT_SHIELD, AF_PLAIN, 10}, AT_NO_ATK, AT_NO_ATK },
    4, 360,
    2, 9, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_CONSUMABLES), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DUVESSA}, TILE_ERROR
},

{
    MONS_DOWAN, 'e', RED, "Dowan",
    M_UNIQUE | M_WARM_BLOOD | M_SPEAKS | M_MALE | M_SPELLCASTER,
    MR_VUL_SLASHING,
    15, MONS_ELF, MONS_ELF, MH_NATURAL, 20,
    { {AT_HIT, AF_PLAIN, 5}, {AT_SHIELD, AF_PLAIN, 5}, AT_NO_ATK, AT_NO_ATK },
    3, 240,
    0, 11, MST_DOWAN, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_CONSUMABLES), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DOWAN}, TILE_ERROR
},

{
    MONS_FANNAR, 'e', LIGHTBLUE, "Fannar",
    M_UNIQUE | M_WARM_BLOOD | M_SPEAKS | M_GENDER_NEUTRAL | M_SPELLCASTER,
    MR_VUL_SLASHING,
    16, MONS_ELF, MONS_ELF, MH_NATURAL, 80,
    { {AT_HIT, AF_PLAIN, 8}, {AT_SHIELD, AF_PLAIN, 8}, AT_NO_ATK, AT_NO_ATK },
    10, 800,
    0, 15, MST_FANNAR, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_FANNAR}, TILE_ERROR
},

// "F"rogs.
{
    MONS_PRINCE_RIBBIT, 'F', LIGHTCYAN, "Prince Ribbit",
    M_UNIQUE | M_COLD_BLOOD | M_SPEAKS | M_MALE,
    MR_VUL_SLASHING | MR_VUL_PIERCING,
    12, MONS_FROG, MONS_HUMAN, MH_NATURAL, 40,
    { { AT_BITE, AF_FROG, 0 }, { AT_SLAM, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    3, 360,
    0, 12, MST_PRINCE_RIBBIT, CE_CLEAN, S_CROAK,
    I_HUMAN, HT_AMPHIBIOUS, 8, SWIM_ENERGY(6),
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_QUADRUPED_TAILLESS,
    {TILEP_MONS_PRINCE_RIBBIT}, TILE_ERROR
},

// "g"oblins, gnolls, and dwarves.
{
    MONS_IJYB, 'g', BLUE, "Ijyb",
    M_UNIQUE | M_WARM_BLOOD | M_SPEAKS | M_FEMALE | M_ALWAYS_WAND,
    MR_VUL_SLASHING,
    64, MONS_GOBLIN, MONS_GOBLIN, MH_NATURAL, 10,
    { {AT_HIT, AF_PLAIN, 4}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    2, 245,
    2, 9, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_WAND | MU_DOOR), SIZE_SMALL, MON_SHAPE_HUMANOID,
    {TILEP_MONS_IJYB}, TILE_ERROR
},

{
    MONS_ROBIN, 'g', LIGHTCYAN, "Robin",
    M_UNIQUE | M_WARM_BLOOD | M_SPEAKS | M_FEMALE,
    MR_VUL_SLASHING,
    10, MONS_GOBLIN, MONS_HOBGOBLIN, MH_NATURAL, 10,
    { {AT_HIT, AF_PLAIN, 5}, {AT_SHIELD, AF_PLAIN, 5}, AT_NO_ATK, AT_NO_ATK },
    2, 160,
    1, 8, MST_ROBIN, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_MELEE, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ROBIN}, TILE_ERROR
},

{
    MONS_GRUM, 'g', LIGHTRED, "Grum",
    M_UNIQUE | M_WARM_BLOOD | M_SPEAKS | M_MALE,
    MR_RES_SLASHING | MR_VUL_PIERCING,
    19, MONS_GNOLL, MONS_GNOLL, MH_NATURAL, 10,
    { {AT_HIT, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    4, 400,
    2, 9, MST_NO_SPELLS, CE_CLEAN, S_GROWL,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_WAND | MU_THROW_NET), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_GRUM}, TILE_ERROR
},

{
    MONS_CRAZY_YIUF, 'g', COLOUR_UNDEF, "Crazy Yiuf",
    M_WARM_BLOOD | M_SPEAKS | M_UNIQUE | M_MALE | M_ALWAYS_WAND,
    MR_VUL_SLASHING,
    10, MONS_GNOLL, MONS_GNOLL, MH_NATURAL, 10,
    { {AT_HIT, AF_PLAIN, 9}, {AT_SHIELD, AF_PLAIN, 9}, AT_NO_ATK, AT_NO_ATK },
    3, 195,
    2, 9, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_START_ONLY | MU_ALL), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_CRAZY_YIUF}, TILE_ERROR
},

{
    MONS_JORGRUN, 'g', LIGHTMAGENTA, "Jorgrun",
    M_UNIQUE | M_WARM_BLOOD | M_SPEAKS | M_MALE | M_SPELLCASTER,
    MR_VUL_SLASHING,
    18, MONS_DWARF, MONS_DWARF, MH_NATURAL, 120,
    { {AT_HIT, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    15, 1200,
    2, 22, MST_JORGRUN, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_JORGRUN}, TILE_ERROR
},

// "h" - carnivorous quadrupeds
{
    MONS_NATASHA, 'h', MAGENTA, "Natasha",
    M_SEE_INVIS | M_WARM_BLOOD | M_SPEAKS | M_UNIQUE | M_FEMALE | M_SPELLCASTER,
    MR_VUL_SLASHING,
    10, MONS_FELID, MONS_FELID, MH_NATURAL | MH_EVIL, 20,
    { {AT_CLAW, AF_PLAIN, 10}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    3, 150,
    2, 13, MST_NATASHA, CE_CLEAN, S_HISS,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(8),
    muf(MU_WAND | MU_DOOR), SIZE_LITTLE, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_NATASHA}, TILE_ERROR
},

// "H"ybrids.
{
    MONS_ARACHNE, 'H', LIGHTCYAN, "Arachne",
    M_UNIQUE | M_WARM_BLOOD | M_SPEAKS | M_SEE_INVIS | M_WEB_SENSE | M_FEMALE | M_SPELLCASTER,
    MR_RES_PIERCING | MR_VUL_SLASHING, // no rPois- (breathes through the human half)
    10, MONS_SPIDER, MONS_ARACHNE, MH_NATURAL, 60,
    { {AT_HIT, AF_PLAIN, 30}, {AT_SHIELD, AF_PLAIN, 30}, AT_NO_ATK, AT_NO_ATK },
    17, 2040,
    3, 18, MST_ARACHNE, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 15, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_LARGE, MON_SHAPE_ARACHNID,
    {TILEP_MONS_ARACHNE}, TILE_CORPSE_ARACHNE
},

{
    MONS_ASTERION, 'H', LIGHTBLUE, "Asterion",
    M_WARM_BLOOD | M_SPEAKS | M_SEE_INVIS | M_UNIQUE | M_MALE | M_SPELLCASTER | M_FIGHTER,
    MR_VUL_SLASHING,
    15, MONS_MINOTAUR, MONS_MINOTAUR, MH_NATURAL | MH_EVIL, 100,
    { {AT_HIT, AF_PLAIN, 35}, {AT_SHIELD, AF_PLAIN, 35}, 
      {AT_GORE, AF_PLAIN, 20}, AT_NO_ATK },
    15, 1275,
    4, 4, MST_ASTERION, CE_CLEAN, S_BELLOW,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ASTERION}, TILE_ERROR
},

// Spr"i"ggans.
{
    MONS_AGNES, 'i', LIGHTCYAN, "Agnes",
    M_UNIQUE | M_FIGHTER | M_WARM_BLOOD | M_SPEAKS | M_SEE_INVIS | M_FEMALE,
    MR_VUL_SLASHING,
    20, MONS_SPRIGGAN, MONS_SPRIGGAN, MH_NATURAL, 100,
    { {AT_HIT, AF_PLAIN, 30}, {AT_SHIELD, AF_PLAIN, 30}, AT_NO_ATK, AT_NO_ATK },
    11, 990,
    0, 33, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 18, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_LITTLE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_AGNES}, TILE_ERROR
},

{
    MONS_THE_ENCHANTRESS, 'i', LIGHTMAGENTA, "the Enchantress",
    M_WARM_BLOOD | M_SPEAKS | M_SEE_INVIS | M_UNIQUE | M_FEMALE | M_SPELLCASTER,
    MR_VUL_SLASHING,
    35, MONS_SPRIGGAN, MONS_SPRIGGAN, MH_NATURAL, 160,
    { {AT_HIT, AF_PLAIN, 26}, {AT_SHIELD, AF_PLAIN, 26}, AT_NO_ATK, AT_NO_ATK },
    15, 975,
    1, 41, MST_THE_ENCHANTRESS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(6),
    muf(MU_MELEE | MU_EVOKE), SIZE_LITTLE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_THE_ENCHANTRESS}, TILE_ERROR
},

// "J"ellies.
{
    MONS_ROYAL_JELLY, 'J', YELLOW, "the Royal Jelly",
    M_SEE_INVIS | M_UNBLINDABLE | M_ACID_SPLASH | M_UNIQUE | M_EAT_DOORS
     | M_TALL_TILE,
    MR_RES_POISON | mrd(MR_RES_ACID, 3) | MR_RES_SLASHING | MR_RES_PIERCING,
    8, MONS_JELLY, MONS_JELLY, MH_NATURAL, 180,
    { {AT_HIT, AF_ACID, 50}, {AT_HIT, AF_ACID, 30}, AT_NO_ATK, AT_NO_ATK },
    21, 2310,
    8, 4, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_SLIME, 14, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_LARGE, MON_SHAPE_BLOB,
    {TILEP_MONS_ROYAL_JELLY}, TILE_ERROR
},

{
    MONS_DISSOLUTION, 'J', LIGHTMAGENTA, "Dissolution",
    M_UNIQUE | M_SEE_INVIS | M_UNBLINDABLE | M_ACID_SPLASH | M_BURROWS
     | M_SPEAKS | M_FAST_REGEN | M_FEMALE | M_EAT_DOORS | M_SPELLCASTER,
    MR_RES_POISON | mrd(MR_RES_ACID, 3) | MR_RES_SLASHING | MR_RES_PIERCING,
    60, MONS_JELLY, MONS_JELLY, MH_NATURAL, 120,
    { {AT_HIT, AF_ACID, 50}, {AT_HIT, AF_ACID, 30}, AT_NO_ATK, AT_NO_ATK },
    16, 1760,
    10, 1, MST_DISSOLUTION, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_SLIME, 10, DEFAULT_ENERGY,
    MU_NO_GOLD, SIZE_MEDIUM, MON_SHAPE_BLOB,
    {TILEP_MONS_DISSOLUTION}, TILE_ERROR
},

// "K"obolds.
{
    MONS_SONJA, 'K', RED, "Sonja",
    M_UNIQUE | M_WARM_BLOOD | M_SPEAKS | M_FEMALE | M_ARCHER,
    MR_VUL_SLASHING,
    40, MONS_KOBOLD, MONS_KOBOLD, MH_NATURAL, 10,
    { {AT_HIT, AF_PLAIN, 9}, {AT_SHIELD, AF_PLAIN, 5}, 
      {AT_HIT, AF_PLAIN, 5}, {AT_HIT, AF_PLAIN, 5} },
    6, 300,
    2, 31, MST_SONJA, CE_NOXIOUS, S_SHOUT,
    I_HUMAN, HT_LAND, 14, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_BLOWGUN), SIZE_SMALL, MON_SHAPE_HUMANOID,
    {TILEP_MONS_SONJA}, TILE_ERROR
},

{
    // XP modifier is very high to compensate for 4 created-friendly humans
    MONS_PIKEL, 'K', BLUE, "Pikel",
    M_WARM_BLOOD | M_SPEAKS | M_UNIQUE | M_MALE,
    MR_VUL_SLASHING,
    32, MONS_KOBOLD, MONS_BIG_KOBOLD, MH_NATURAL, 20,
    { {AT_HIT, AF_PLAIN, 9}, {AT_SHIELD, AF_PLAIN, 9}, AT_NO_ATK, AT_NO_ATK },
    6, 390,
    4, 9, MST_NO_SPELLS, CE_NOXIOUS, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_PIKEL}, TILE_ERROR
},

// "L"iches.
{
    // May be re-spawned after his death.
    MONS_BORIS, 'L', RED, "Boris",
    M_UNIQUE | M_SEE_INVIS | M_SPEAKS | M_MALE | M_SPELLCASTER,
    mrd(MR_RES_COLD, 2) | MR_RES_ELEC | MR_VUL_ACID | MR_RES_PIERCING,
    15, MONS_LICH, MONS_LICH, MH_UNDEAD, 200,
    { {AT_HIT, AF_PLAIN, 25}, {AT_SHIELD, AF_PLAIN, 25}, 
      {AT_TOUCH, AF_DRAIN_XP, 15}, AT_NO_ATK },
    22, 1540,
    12, 22, MST_BORIS, CE_NOCORPSE, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_BORIS}, TILE_ERROR
},

// "M"ummies.
{
    MONS_MENKAURE, 'M', MAGENTA, "Menkaure",
    M_UNIQUE | M_SPEAKS | M_SEE_INVIS | M_MALE | M_NO_ZOMBIE,
    MR_VUL_FIRE | MR_RES_COLD | MR_RES_PIERCING | MR_VUL_SLASHING,
    48, MONS_MUMMY, MONS_MUMMY, MH_UNDEAD, 20,
    { {AT_HIT, AF_PLAIN, 25}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    3, 240,
    3, 6, MST_MENKAURE, CE_NOXIOUS, S_SHOUT,
    I_HUMAN, HT_LAND, 8, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_ARMOUR), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_MENKAURE}, TILE_CORPSE_MUMMY
},

{
    MONS_KHUFU, 'M', LIGHTRED, "Khufu",
    M_SEE_INVIS | M_SPEAKS | M_UNIQUE | M_MALE | M_SPELLCASTER | M_NO_ZOMBIE,
    MR_RES_COLD | MR_RES_ELEC | MR_RES_ACID | MR_RES_PIERCING | MR_VUL_SLASHING,
    20, MONS_MUMMY, MONS_MUMMY, MH_UNDEAD, 160,
    { {AT_HIT, AF_PLAIN, 35}, {AT_SHIELD, AF_PLAIN, 35}, AT_NO_ATK, AT_NO_ATK },
    18, 2430,
    10, 6, MST_KHUFU, CE_NOXIOUS, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_WAND), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_KHUFU}, TILE_CORPSE_MUMMY
},

// "m"erfolk.
{
    MONS_ILSUIW, 'm', LIGHTGREEN, "Ilsuiw",
    M_UNIQUE | M_WARM_BLOOD | M_SPEAKS | M_FEMALE | M_SPELLCASTER,
    MR_NO_FLAGS,
    10, MONS_MERFOLK, MONS_MERFOLK, MH_NATURAL, 140,
    { {AT_HIT, AF_PLAIN, 10}, {AT_SHIELD, AF_PLAIN, 10}, AT_NO_ATK, AT_NO_ATK },
    16, 1520,
    5, 36, MST_ILSUIW, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_AMPHIBIOUS, 10, SWIM_ENERGY(6),
    muf(MU_MELEE | MU_EVOKE | MU_THROW_NET), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ILSUIW, TVARY_WATER}, TILE_ERROR
},

// "N"agas.
{
    MONS_VASHNIA, 'N', LIGHTCYAN, "Vashnia",
    M_SEE_INVIS | M_WARM_BLOOD | M_FEMALE
     | M_ARCHER | M_PREFER_RANGED | M_SPEAKS | M_UNIQUE,
    MR_RES_POISON,
    27, MONS_NAGA, MONS_NAGA, MH_NATURAL, 120,
    { {AT_HIT, AF_PLAIN, 25}, {AT_SHIELD, AF_PLAIN, 25},
      {AT_CONSTRICT, AF_CRUSH, 7}, AT_NO_ATK },
    16, 1600,
    6, 31, MST_VASHNIA, CE_NOXIOUS, S_SHOUT,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(14),
    MU_ALL, SIZE_LARGE, MON_SHAPE_NAGA,
    {TILEP_MONS_VASHNIA}, TILE_ERROR
},

// "O"gres.
{
    MONS_EROLCHA, 'O', LIGHTBLUE, "Erolcha",
    M_UNIQUE | M_SEE_INVIS | M_WARM_BLOOD | M_SPEAKS | M_FEMALE | M_SPELLCASTER,
    MR_VUL_SLASHING,
    50, MONS_OGRE, MONS_OGRE, MH_NATURAL, 60,
    { {AT_HIT, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    6, 540,
    3, 7, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_WAND), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_EROLCHA}, TILE_ERROR
},

// "o"rcs.
{
    MONS_BLORK_THE_ORC, 'o', BROWN, "Blork the orc",
    M_UNIQUE | M_WARM_BLOOD | M_SPEAKS | M_MALE,
    MR_VUL_SLASHING,
    15, MONS_ORC, MONS_ORC, MH_NATURAL, 20,
    { {AT_HIT, AF_PLAIN, 7}, {AT_SHIELD, AF_PLAIN, 7}, AT_NO_ATK, AT_NO_ATK },
    3, 315,
    0, 9, MST_ORC_WIZARD_III, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_MELEE, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_BLORK_THE_ORC}, TILE_ERROR
},

{
    MONS_URUG, 'o', RED, "Urug",
    M_UNIQUE | M_ARCHER | M_WARM_BLOOD | M_SPEAKS | M_FEMALE,
    MR_VUL_SLASHING,
    15, MONS_ORC, MONS_ORC, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 25}, {AT_SHIELD, AF_PLAIN, 25}, AT_NO_ATK, AT_NO_ATK },
    9, 675,
    2, 19, MST_MERFOLK_JAVELINEER, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_THROW_JAVELIN), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_URUG}, TILE_ERROR
},

{
    MONS_NERGALLE, 'o', WHITE, "Nergalle",
    M_UNIQUE | M_SEE_INVIS | M_WARM_BLOOD | M_SPEAKS | M_FEMALE | M_SPELLCASTER,
    MR_VUL_SLASHING,
    12, MONS_ORC, MONS_ORC, MH_NATURAL | MH_EVIL, 60,
    { {AT_HIT, AF_PLAIN, 6}, {AT_SHIELD, AF_PLAIN, 6}, AT_NO_ATK, AT_NO_ATK },
    10, 600,
    9, 18, MST_NERGALLE, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_NERGALLE}, TILE_ERROR
},

{
    MONS_SAINT_ROKA, 'o', LIGHTBLUE, "Saint Roka",
    M_UNIQUE | M_FIGHTER | M_WARM_BLOOD | M_SPEAKS | M_MALE,
    MR_RES_SLASHING | MR_VUL_PIERCING,
    15, MONS_ORC, MONS_ORC, MH_NATURAL | MH_EVIL, 80,
    { {AT_HIT, AF_PLAIN, 35}, {AT_SHIELD, AF_PLAIN, 35}, AT_NO_ATK, AT_NO_ATK },
    18, 1980,
    3, 23, MST_SAINT_ROKA, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_JAVELIN | MU_THROW_NET), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_SAINT_ROKA}, TILE_ERROR
},

// Tengu.
{
    MONS_SOJOBO, 'Q', LIGHTGREEN, "Sojobo",
    M_FIGHTER | M_SEE_INVIS | M_WARM_BLOOD | M_SPEAKS | M_UNIQUE | M_FEMALE
     | M_FLIES | M_SPELLCASTER,
    MR_RES_WIND | MR_VUL_SLASHING,
    20, MONS_TENGU, MONS_TENGU, MH_NATURAL, 140,
    { {AT_HIT, AF_PLAIN, 28}, {AT_SHIELD, AF_PLAIN, 28},
      {AT_PECK, AF_PLAIN, 14}, {AT_CLAW, AF_PLAIN, 14} },
    20, 1500,
    2, 38, MST_SOJOBO, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, MOVE_ENERGY(9),
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_SOJOBO}, TILE_ERROR
},

// Rakshasas and demons.
{
    MONS_MAALIK, 'R', LIGHTRED, "Maalik",
    M_UNIQUE | M_SPEAKS | M_MALE | M_FLIES | M_SPELLCASTER,
    MR_RES_POISON | MR_RES_HELLFIRE | mrd(MR_RES_FIRE, 4) | MR_VUL_COLD
     | MR_VUL_WATER | MR_RES_ACID | MR_RES_BLUDGEONING,
    12, MONS_EFREET, MONS_EFREET, MH_ELEMENTAL, 40,
    { {AT_HIT, AF_PLAIN, 17}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    11, 880,
    10, 17, MST_MAALIK, CE_DIVINE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WEAPONS | MU_EVOKE), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_MAALIK}, TILE_ERROR
},

{
    MONS_MARA, 'R', LIGHTMAGENTA, "Mara",
    M_SEE_INVIS | M_SPEAKS | M_UNIQUE | M_MALE,
    MR_RES_POISON | mrd(MR_RES_FIRE, 2) | MR_RES_BLUDGEONING,
    35, MONS_RAKSHASA, MONS_RAKSHASA, MH_DEMONIC, 140,
    { {AT_HIT, AF_PLAIN, 30}, {AT_SHIELD, AF_PLAIN, 30}, AT_NO_ATK, AT_NO_ATK },
    18, 1440,
    10, 31, MST_MARA, CE_DIVINE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_MARA}, TILE_ERROR
},

// "S"nakes and guardian serpents.
{
    MONS_AIZUL, 'S', LIGHTMAGENTA, "Aizul",
    M_SEE_INVIS | M_WARM_BLOOD | M_SPEAKS | M_UNIQUE | M_FEMALE | M_SPELLCASTER,
    MR_RES_POISON,
    10, MONS_GUARDIAN_SERPENT, MONS_GUARDIAN_SERPENT, MH_NATURAL, 120,
    { {AT_HIT, AF_PLAIN, 25}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    14, 1400,
    8, 22, MST_AIZUL, CE_NOXIOUS, S_SHOUT,
    I_HUMAN, HT_LAND, 15, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_SNAKE,
    {TILEP_MONS_AIZUL}, TILE_ERROR
},

// "T"rolls.
{
    // Snorg can go berserk.
    MONS_SNORG, 'T', LIGHTGREEN, "Snorg",
    M_UNIQUE | M_WARM_BLOOD | M_SPEAKS | M_FAST_REGEN | M_FEMALE,
    MR_VUL_ACID | MR_RES_BLUDGEONING,
    25, MONS_TROLL, MONS_TROLL, MH_NATURAL, 60,
    { {AT_BITE, AF_PLAIN, 20}, {AT_CLAW, AF_PLAIN, 15},
      {AT_CLAW, AF_PLAIN, 15}, AT_NO_ATK },
    8, 960,
    0, 9, MST_SNORG, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_ARMOUR | MU_EVOKE), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_SNORG}, TILE_ERROR
},

{
    MONS_PURGY, 'T', GREEN, "Purgy",
    M_UNIQUE | M_WARM_BLOOD | M_SPEAKS | M_FAST_REGEN | M_MALE,
    MR_VUL_ACID | MR_RES_BLUDGEONING,
    10, MONS_TROLL, MONS_TROLL, MH_NATURAL, 10,
    { {AT_BITE, AF_PLAIN, 9}, {AT_CLAW, AF_PLAIN, 4},
      {AT_CLAW, AF_PLAIN, 4}, {AT_SHIELD, AF_PLAIN, 4} },
    5, 350,
    2, 8, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_THROW_ROCK | MU_ARMOUR | MU_SHIELD), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_PURGY}, TILE_ERROR
},

// Semi-unique in the moon base wizlab
{
    MONS_MOON_TROLL, 'T', LIGHTCYAN, "moon troll",
    M_WARM_BLOOD | M_SPEAKS | M_FAST_REGEN | M_SEE_INVIS
     | M_NO_POLY_TO | M_NO_GEN_DERIVED,
    MR_RES_FIRE | MR_RES_COLD | MR_RES_POISON 
     | MR_VUL_ACID | MR_RES_BLUDGEONING,
    10, MONS_TROLL, MONS_MOON_TROLL, MH_NATURAL, 140,
    { {AT_BITE, AF_PLAIN, 35}, {AT_CLAW, AF_PLAIN, 25},
      {AT_CLAW, AF_PLAIN, 25}, {AT_SHIELD, AF_PLAIN, 25} },
    18, 990,
    20, 4, MST_MOON_TROLL, CE_NOCORPSE, S_SHOUT,
    I_HUMAN, HT_LAND, 12, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_ARMOUR | MU_SHIELD | MU_EVOKE), SIZE_BIG, MON_SHAPE_HUMANOID,
    {TILEP_MONS_MOON_TROLL}, TILE_ERROR // BCADDO: Should have TILE_CORPSE.
},

// "V"ampires.
{
    MONS_JORY, 'V', LIGHTRED, "Jory",
    M_FIGHTER | M_SEE_INVIS | M_SPEAKS | M_MALE
     | M_BLOOD_SCENT | M_WARM_BLOOD | M_UNIQUE | M_SPELLCASTER,
    MR_RES_COLD | MR_VUL_PIERCING | MR_VUL_BLUDGEONING,
    13, MONS_VAMPIRE, MONS_VAMPIRE, MH_UNDEAD, 160,
    { {AT_HIT, AF_PLAIN, 40}, {AT_SHIELD, AF_PLAIN, 40}, 
      {AT_BITE, AF_VAMPIRIC, 15}, AT_NO_ATK },
    18, 1800,
    10, 31, MST_JORY, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_JORY}, TILE_CORPSE_VAMPIRE
},

// Snails and other gastropods.
{
    MONS_GASTRONOK, 'w', MAGENTA, "Gastronok",
    M_UNIQUE | M_SEE_INVIS | M_SPEAKS | M_MALE | M_COLD_BLOOD,
    MR_VUL_ACID | MR_VUL_SLASHING | MR_RES_PIERCING | MR_RES_BLUDGEONING,
    16, MONS_ELEPHANT_SLUG, MONS_ELEPHANT_SLUG, MH_NATURAL, 80,
    { {AT_BITE, AF_PLAIN, 40}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    20, 1500,
    2, 1, MST_GASTRONOK, CE_NOXIOUS, S_GURGLE,
    I_HUMAN, HT_AMPHIBIOUS, 5, DEFAULT_ENERGY,
    muf(MU_START_ONLY | MU_ARMOUR), SIZE_BIG, MON_SHAPE_SNAIL,
    {TILEP_MONS_GASTRONOK}, TILE_ERROR
},

// Abominations
{
    MONS_GAMMAFUNK, 'X', LIGHTRED, "Gammafunk",
    M_UNIQUE | M_SEE_INVIS | M_SPEAKS | M_GENDER_NEUTRAL,
    MR_RES_POISON | MR_RES_FIRE | MR_RES_COLD | MR_RES_ELEC | mrd(MR_RES_NEG, 1)
     | MR_RES_PIERCING | MR_RES_BLUDGEONING | MR_VUL_SLASHING,
    10, MONS_TENTACLED_MONSTROSITY, MONS_TENTACLED_MONSTROSITY,
        MH_DEMONIC, 160,
    { {AT_TENTACLE_SLAP, AF_PLAIN, 18}, {AT_TENTACLE_SLAP, AF_PLAIN, 14},
      {AT_TENTACLE_SLAP, AF_PLAIN, 8}, {AT_CONSTRICT, AF_CRUSH, 4} },
    15, 1265,
    5, 5, MST_NO_SPELLS, CE_DIVINE, S_MOAN,
    I_HUMAN, HT_AMPHIBIOUS, 10, DEFAULT_ENERGY,
    MU_DOOR, SIZE_GIANT, MON_SHAPE_MISC,
    {TILEP_MONS_GAMMAFUNK}, TILE_CORPSE_GAMMAFUNK
},

// Elephants.
{
    MONS_NELLIE, 'Y', LIGHTMAGENTA, "Nellie",
    M_WARM_BLOOD | M_UNIQUE | M_SPEAKS | M_FEMALE,
    mrd(MR_RES_NEG, 1) | mrd(MR_RES_FIRE, 2) | MR_RES_BLUDGEONING
     | MR_RES_PIERCING,
    8, MONS_ELEPHANT, MONS_HELLEPHANT, MH_DEMONIC, 140,
    { {AT_TRAMPLE, AF_TRAMPLE, 45}, {AT_BITE, AF_PLAIN, 20},
      {AT_GORE, AF_PLAIN, 15 }, AT_NO_ATK },
    20, 2400,
    13, 21, MST_HELLEPHANT, CE_CLEAN, S_TRUMPET,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_GIANT, MON_SHAPE_QUADRUPED,
    {TILEP_MONS_NELLIE}, TILE_ERROR
},

// Skulls and "z" undead.
{
    MONS_MURRAY, 'z', LIGHTRED, "Murray",
    M_UNIQUE | M_SEE_INVIS | M_SPEAKS | M_MALE | M_SPELLCASTER,
    mrd(MR_RES_FIRE | MR_RES_COLD, 2) | MR_RES_ELEC | MR_VUL_ACID
     | MR_RES_PIERCING | MR_VUL_BLUDGEONING | MR_RES_SLASHING,
    15, MONS_CURSE_SKULL, MONS_CURSE_SKULL, MH_UNDEAD, 200,
    { {AT_BITE, AF_PLAIN, 20}, {AT_BITE, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    14, 1610,
    30, 10, MST_MURRAY, CE_NOCORPSE, S_MOAN,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_TINY, MON_SHAPE_MISC,
    {TILEP_MONS_MURRAY}, TILE_ERROR
},

// numbers!
{
    MONS_IGNACIO, '1', LIGHTMAGENTA, "Ignacio",
    M_UNIQUE | M_FIGHTER | M_SEE_INVIS | M_SPEAKS | M_MALE,
    MR_RES_ELEC | MR_RES_FIRE | MR_RES_COLD | MR_RES_POISON | mrd(MR_RES_NEG,3)
     | MR_RES_ACID | MR_RES_PIERCING | MR_VUL_BLUDGEONING,
    14, MONS_OXARMORDTH, MONS_OXARMORDTH, MH_DEMONIC, 160,
    { {AT_HIT, AF_PLAIN, 20}, {AT_HIT, AF_PLAIN, 10}, {AT_HIT, AF_PLAIN, 10},
      {AT_HIT, AF_PLAIN, 5} },
    18, 2520,
    10, 33, MST_IGNACIO, CE_DIVINE, S_SHOUT,
    I_HUMAN, HT_LAND, 20, DEFAULT_ENERGY,
    muf(MU_START_ONLY | MU_WEAPON_MELEE | MU_DOOR), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_IGNACIO}, TILE_CORPSE_IGNACIO
},

{
    MONS_GRINDER, '5', BLUE, "Grinder",
    M_UNIQUE | M_SEE_INVIS | M_SPEAKS | M_FEMALE,
    MR_RES_POISON | mrd(MR_RES_COLD, 2) | mrd(MR_RES_NEG,3)
     | MR_VUL_BLUDGEONING,
    11, MONS_CYSGOBWCA, MONS_CYSGOBWCA, MH_DEMONIC, 20,
    { {AT_HIT, AF_PAIN, 11}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    6, 390,
    3, 8, MST_GRINDER, CE_DIVINE, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_DOOR, SIZE_LITTLE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_GRINDER}, TILE_ERROR
},

{
    // Roxanne obviously can't use items, but we want to equip her with
    // a spellbook, so MU_START_ONLY is necessary.
    MONS_ROXANNE, '8', BLUE, "Roxanne",
    M_UNIQUE | M_STATIONARY | M_SPEAKS | M_FEMALE | M_NO_REGEN | M_SPELLCASTER,
    mrd(MR_RES_FIRE | MR_RES_COLD, 2) | MR_RES_ELEC | MR_RES_PETRIFY 
     | MR_VUL_ACID | MR_RES_SLASHING | MR_RES_BLUDGEONING,
    10, MONS_STATUE, MONS_STATUE, MH_CONSTRUCT, 160,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    14, 1820,
    20, 0, MST_ROXANNE, CE_NOCORPSE, S_SILENT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_START_ONLY, SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ROXANNE}, TILE_ERROR
},

// human uniques
{
    MONS_TERENCE, '@', LIGHTCYAN, "Terence",
    M_UNIQUE | M_WARM_BLOOD | M_SPEAKS | M_MALE,
    MR_RES_SLASHING | MR_VUL_PIERCING,
    20, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 10,
    { {AT_HIT, AF_PLAIN, 5}, {AT_SHIELD, AF_PLAIN, 5}, AT_NO_ATK, AT_NO_ATK },
    2, 200,
    0, 10, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_MELEE, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_TERENCE}, TILE_ERROR
},

{
    MONS_JESSICA, '@', LIGHTGREY, "Jessica",
    M_UNIQUE | M_WARM_BLOOD | M_SPEAKS | M_FEMALE | M_SPELLCASTER,
    MR_VUL_SLASHING,
    125, MONS_HUMAN, MONS_HUMAN, MH_NATURAL | MH_EVIL, 10,
    { {AT_HIT, AF_PLAIN, 5}, {AT_SHIELD, AF_PLAIN, 5}, AT_NO_ATK, AT_NO_ATK },
    1, 95,
    0, 10, MST_JESSICA, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_CONSUMABLES), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_JESSICA}, TILE_ERROR
},

{
    MONS_SIGMUND, '@', YELLOW, "Sigmund",
    M_UNIQUE | M_WARM_BLOOD | M_SPEAKS | M_MALE,
    MR_VUL_SLASHING,
    50, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 10,
    { {AT_HIT, AF_PLAIN, 5}, {AT_SHIELD, AF_PLAIN, 5}, AT_NO_ATK, AT_NO_ATK },
    3, 300,
    0, 11, MST_ORC_WIZARD_II, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_CONSUMABLES), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_SIGMUND}, TILE_ERROR
},

{
    MONS_EDMUND, '@', RED, "Edmund",
    M_UNIQUE | M_FIGHTER | M_WARM_BLOOD | M_SPEAKS | M_MALE,
    MR_VUL_SLASHING,
    15, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 20,
    { {AT_HIT, AF_PLAIN, 6}, {AT_SHIELD, AF_PLAIN, 6}, AT_NO_ATK, AT_NO_ATK },
    4, 440,
    0, 10, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_EDMUND}, TILE_ERROR
},

{
    MONS_PSYCHE, '@', LIGHTMAGENTA, "Psyche",
    M_UNIQUE | M_WARM_BLOOD | M_SPEAKS | M_FEMALE,
    MR_VUL_SLASHING,
    20, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 20,
    { {AT_HIT, AF_PLAIN, 7}, {AT_SHIELD, AF_PLAIN, 7}, AT_NO_ATK, AT_NO_ATK },
    5, 400,
    0, 12, MST_ORC_WIZARD_III, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_PSYCHE}, TILE_ERROR
},

{
    MONS_DONALD, '@', BLUE, "Donald",
    M_UNIQUE | M_FIGHTER | M_WARM_BLOOD | M_SPEAKS | M_MALE,
    MR_VUL_SLASHING,
    20, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 100,
    { {AT_HIT, AF_PLAIN, 26}, {AT_SHIELD, AF_PLAIN, 26}, AT_NO_ATK, AT_NO_ATK },
    14, 840,
    3, 10, MST_DONALD, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_AMPHIBIOUS, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE | MU_THROW_JAVELIN), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DONALD}, TILE_ERROR
},

{
    MONS_JOSEPH, '@', CYAN, "Joseph",
    M_UNIQUE | M_FIGHTER | M_ARCHER | M_WARM_BLOOD | M_SPEAKS | M_MALE,
    MR_VUL_SLASHING,
    15, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 40,
    { {AT_HIT, AF_PLAIN, 15}, {AT_SHIELD, AF_PLAIN, 15}, AT_NO_ATK, AT_NO_ATK },
    6, 660,
    0, 10, MST_NO_SPELLS, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_JOSEPH}, TILE_ERROR
},

{
    MONS_ERICA, '@', MAGENTA, "Erica",
    M_UNIQUE | M_WARM_BLOOD | M_SPEAKS | M_FEMALE | M_SPELLCASTER | M_FIGHTER,
    MR_VUL_SLASHING,
    20, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 60,
    { {AT_HIT, AF_PLAIN, 10}, {AT_SHIELD, AF_PLAIN, 10}, AT_NO_ATK, AT_NO_ATK },
    9, 630,
    0, 11, MST_ERICA, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ERICA}, TILE_ERROR
},

{
    MONS_JOSEPHINE, '@', WHITE, "Josephine",
    M_UNIQUE | M_WARM_BLOOD | M_SPEAKS | M_FEMALE | M_SPELLCASTER,
    mrd(MR_RES_NEG, 3) | MR_VUL_SLASHING,
    20, MONS_HUMAN, MONS_HUMAN, MH_NATURAL | MH_EVIL, 60,
    { {AT_HIT, AF_PLAIN, 11}, {AT_SHIELD, AF_PLAIN, 11}, AT_NO_ATK, AT_NO_ATK },
    10, 700,
    0, 21, MST_JOSEPHINE, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_JOSEPHINE}, TILE_ERROR
},

{
    MONS_HAROLD, '@', LIGHTGREEN, "Harold",
    M_UNIQUE | M_FIGHTER | M_WARM_BLOOD | M_MALE | M_SPEAKS,
    MR_VUL_SLASHING,
    20, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 60,
    { {AT_HIT, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    9, 765,
    0, 8, MST_HAROLD, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_ALL | MU_THROW_NET | MU_THROW_JAVELIN), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_HAROLD}, TILE_ERROR
},

{
    MONS_LOUISE, '@', LIGHTMAGENTA, "Louise",
    M_UNIQUE | M_WARM_BLOOD | M_SPEAKS | M_FEMALE,
    MR_VUL_SLASHING,
    15, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 80,
    { {AT_HIT, AF_PLAIN, 17}, {AT_SHIELD, AF_PLAIN, 17}, AT_NO_ATK, AT_NO_ATK },
    13, 1040,
    0, 10, MST_LOUISE, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_LOUISE}, TILE_ERROR
},

{
    MONS_FRANCES, '@', YELLOW, "Frances",
    M_UNIQUE | M_WARM_BLOOD | M_SEE_INVIS | M_FEMALE | M_SPEAKS,
    MR_VUL_SLASHING,
    15, MONS_HUMAN, MONS_HUMAN, MH_NATURAL | MH_EVIL, 100,
    { {AT_HIT, AF_PLAIN, 29}, {AT_SHIELD, AF_PLAIN, 29}, AT_NO_ATK, AT_NO_ATK },
    14, 1330,
    0, 18, MST_FRANCES, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_FRANCES}, TILE_ERROR
},

{
    MONS_RUPERT, '@', LIGHTRED, "Rupert",
    M_UNIQUE | M_WARM_BLOOD | M_SEE_INVIS | M_SPEAKS | M_MALE,
    MR_VUL_SLASHING,
    18, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 100,
    { {AT_HIT, AF_PLAIN, 21}, {AT_SHIELD, AF_PLAIN, 21}, AT_NO_ATK, AT_NO_ATK },
    16, 1200,
    0, 22, MST_RUPERT, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_RUPERT}, TILE_ERROR
},

{
    MONS_KIRKE, '@', LIGHTBLUE, "Circe",
    M_UNIQUE | M_WARM_BLOOD | M_SEE_INVIS | M_SPEAKS | M_FEMALE | M_SPELLCASTER,
    MR_VUL_SLASHING,
    15, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 100,
    { {AT_HIT, AF_PLAIN, 18}, {AT_SHIELD, AF_PLAIN, 18}, AT_NO_ATK, AT_NO_ATK },
    16, 1120,
    0, 10, MST_KIRKE, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_KIRKE}, TILE_ERROR
},

{
    MONS_FREDERICK, '@', GREEN, "Frederick",
    M_UNIQUE | M_WARM_BLOOD | M_SEE_INVIS | M_SPEAKS | M_MALE,
    MR_VUL_SLASHING,
    15, MONS_DEMIGOD, MONS_DEMIGOD, MH_NATURAL, 140,
    { {AT_HIT, AF_PLAIN, 27}, {AT_SHIELD, AF_PLAIN, 27}, AT_NO_ATK, AT_NO_ATK },
    21, 1575,
    0, 21, MST_FREDERICK, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_FREDERICK}, TILE_ERROR
},

{
    MONS_MAGGIE, '@', LIGHTRED, "Maggie",
    M_UNIQUE | M_WARM_BLOOD | M_SPEAKS | M_FEMALE,
    MR_VUL_SLASHING,
    20, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 20,
    { {AT_HIT, AF_PLAIN, 10}, {AT_SHIELD, AF_PLAIN, 10}, AT_NO_ATK, AT_NO_ATK },
    5, 600,
    0, 10, MST_MAGGIE, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE | MU_THROW_JAVELIN), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_MAGGIE}, TILE_ERROR
},

{
    MONS_MARGERY, '@', LIGHTRED, "Margery",
    M_UNIQUE | M_WARM_BLOOD | M_SEE_INVIS | M_SPEAKS | M_FEMALE,
    MR_VUL_SLASHING,
    15, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 140,
    { {AT_HIT, AF_PLAIN, 30}, {AT_SHIELD, AF_PLAIN, 30}, AT_NO_ATK, AT_NO_ATK },
    22, 1650,
    0, 17, MST_MARGERY, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE | MU_THROW_JAVELIN), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_MARGERY}, TILE_ERROR
},

{
    MONS_EUSTACHIO, '@', BLUE, "Eustachio",
    M_UNIQUE | M_WARM_BLOOD | M_SPEAKS | M_MALE | M_SPELLCASTER,
    MR_VUL_SLASHING,
    20, MONS_HUMAN, MONS_HUMAN, MH_NATURAL | MH_EVIL, 20,
    { {AT_HIT, AF_PLAIN, 6}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    4, 400,
    0, 13, MST_EUSTACHIO, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_WAND), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_EUSTACHIO}, TILE_ERROR
},

{
    MONS_MAURICE, '@', GREEN, "Maurice",
    M_UNIQUE | M_WARM_BLOOD | M_SPEAKS | M_MALE | M_ALWAYS_WAND,
    MR_VUL_SLASHING,
    24, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 20,
    { {AT_HIT, AF_STEAL, 9}, {AT_SHIELD, AF_PLAIN, 9}, AT_NO_ATK, AT_NO_ATK },
    5, 600,
    1, 13, MST_MAURICE, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_WAND | MU_MISC | MU_WEAPON_RANGED | MU_THROW_JAVELIN), 
        // Don't give him MU_CONSUMABLES specifically; don't want him drinking stolen potions.
        SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_MAURICE}, TILE_ERROR
},

{
    MONS_NIKOLA, '@', LIGHTCYAN, "Nikola",
    M_UNIQUE | M_WARM_BLOOD | M_SEE_INVIS | M_MALE | M_SPEAKS | M_SPELLCASTER,
    MR_VUL_SLASHING, // Xom would hate MR_RES_ELEC here.
    13, MONS_HUMAN, MONS_HUMAN, MH_NATURAL, 120,
    { {AT_HIT, AF_PLAIN, 20}, {AT_SHIELD, AF_PLAIN, 20}, AT_NO_ATK, AT_NO_ATK },
    18, 1890,
    1, 9, MST_NIKOLA, CE_CLEAN, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_MEDIUM, MON_SHAPE_HUMANOID,
    {TILEP_MONS_NIKOLA}, TILE_ERROR
},

// unique major demons ('&')
{
    MONS_MNOLEG, '&', LIGHTGREEN, "Mnoleg",
    M_UNIQUE | M_FIGHTER | M_SEE_INVIS | M_SPEAKS | M_TALL_TILE
     | M_SPELLCASTER | M_GENDERED,
    MR_RES_ELEC | MR_RES_POISON | MR_RES_FIRE | mrd(MR_RES_ACID, 2)
     | MR_RES_BLUDGEONING | MR_VUL_SLASHING,
    15, MONS_PANDEMONIUM_LORD, MONS_MNOLEG, MH_DEMONIC, 180,
    { {AT_CLAW, AF_PLAIN, 40}, {AT_TENTACLE_SLAP, AF_MUTATE, 35},
      {AT_GORE, AF_PLAIN, 30}, {AT_KICK, AF_BLINK, 23} },
    17, 3485,
    10, 50, MST_MNOLEG, CE_DIVINE, S_BUZZ,
    I_HUMAN, HT_SLIME, 13, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_MNOLEG}, TILE_CORPSE_MNOLEG
},

{
    MONS_LOM_LOBON, '&', LIGHTBLUE, "Lom Lobon",
    M_UNIQUE | M_SPELLCASTER | M_SEE_INVIS | M_SPEAKS | M_FLIES | M_TALL_TILE
     | M_GENDER_NEUTRAL,
    MR_RES_POISON | MR_RES_FIRE | mrd(MR_RES_COLD | MR_RES_ELEC, 3)
     | MR_RES_ACID | MR_VUL_PIERCING | MR_RES_BLUDGEONING,
    15, MONS_PANDEMONIUM_LORD, MONS_LOM_LOBON, MH_DEMONIC, 180,
    { {AT_HIT, AF_ANTIMAGIC, 40}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    19, 3895,
    10, 44, MST_LOM_LOBON, CE_DIVINE, S_SCREAM,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_LOM_LOBON, TVARY_FOOL}, TILE_CORPSE_LOM_LOBON
},

{
    MONS_CEREBOV, '&', RED, "Cerebov",
    M_UNIQUE | M_FIGHTER | M_SEE_INVIS | M_SPEAKS | M_TALL_TILE
     | M_MALE | M_SPELLCASTER,
    MR_RES_POISON | MR_RES_HELLFIRE | mrd(MR_RES_FIRE, 3) | MR_RES_PIERCING,
    15, MONS_PANDEMONIUM_LORD, MONS_CEREBOV, MH_DEMONIC, 180,
    { {AT_HIT, AF_PLAIN, 60}, {AT_SHIELD, AF_PLAIN, 60}, AT_NO_ATK, AT_NO_ATK },
    21, 6510,
    30, 12, MST_CEREBOV, CE_DIVINE, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE), SIZE_GIANT, MON_SHAPE_HUMANOID,
    {TILEP_MONS_CEREBOV}, TILE_CORPSE_CEREBOV
},

{
    MONS_GLOORX_VLOQ, '&', LIGHTGREY, "Gloorx Vloq",
    M_UNIQUE | M_FIGHTER | M_SEE_INVIS | M_SPEAKS | M_FLIES | M_TALL_TILE
     | M_FEMALE | M_SPELLCASTER,
    MR_RES_POISON | MR_RES_COLD | MR_RES_ELEC | MR_RES_BLUDGEONING 
     | mrd(MR_RES_NEG | MR_RES_ACID, 3),
    15, MONS_PANDEMONIUM_LORD, MONS_GLOORX_VLOQ, MH_DEMONIC, 180,
    { {AT_HIT, AF_PLAIN, 45}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    16, 3520,
    10, 22, MST_GLOORX_VLOQ, CE_DIVINE, S_MOAN,
    I_HUMAN, HT_LAND, 20, DEFAULT_ENERGY,
    muf(MU_DOOR | MU_WAND), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_GLOORX_VLOQ}, TILE_CORPSE_GLOORX_VLOQ
},

{
    MONS_GERYON, '&', GREEN, "Geryon",
    M_UNIQUE | M_FIGHTER | M_SEE_INVIS | M_SPEAKS | M_MALE | M_FLIES
     | M_TALL_TILE,
    MR_VUL_SLASHING | MR_RES_BLUDGEONING,
    15, MONS_HELL_LORD, MONS_GERYON, MH_DEMONIC, 120,
    { {AT_TAIL_SLAP, AF_REACH, 35}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    15, 3000,
    15, 6, MST_GERYON, CE_DIVINE, S_ROAR,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    muf(MU_WEAPONS | MU_EVOKE | MU_DOOR), SIZE_GIANT,
        MON_SHAPE_HUMANOID_WINGED,
    {TILEP_MONS_GERYON}, TILE_CORPSE_GERYON
},

{
    MONS_DISPATER, '&', MAGENTA, "Dispater",
    M_UNIQUE | M_FIGHTER | M_SEE_INVIS | M_SPEAKS | M_TALL_TILE
     | M_MALE | M_SPELLCASTER,
    mrd(MR_RES_ELEC | MR_RES_FIRE, 3) | MR_RES_POISON | MR_RES_HELLFIRE
     | MR_RES_COLD | mrd(MR_RES_NEG, 1) | MR_VUL_ACID | MR_RES_BLUDGEONING
     | MR_RES_PIERCING,
    15, MONS_HELL_LORD, MONS_DISPATER, MH_DEMONIC, 200,
    { {AT_HIT, AF_PLAIN, 50}, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    16, 4480,
    40, 3, MST_DISPATER, CE_DIVINE, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_DISPATER}, TILE_CORPSE_DISPATER // BCADDO: STAVELESS
},

{
    MONS_ASMODEUS, '&', LIGHTRED, "Asmodeus",
    M_UNIQUE | M_FIGHTER | M_SEE_INVIS | M_SPEAKS | M_MALE | M_FLIES
     | M_TALL_TILE | M_SPELLCASTER | M_FIRE_RING,
    MR_RES_ELEC | MR_RES_POISON | MR_RES_HELLFIRE | mrd(MR_RES_FIRE, 3)
     | MR_RES_PIERCING,
    25, MONS_HELL_LORD, MONS_ASMODEUS, MH_DEMONIC, 200,
    { {AT_HIT, AF_PLAIN, 50}, {AT_SHIELD, AF_PLAIN, 50}, AT_NO_ATK, AT_NO_ATK },
    17, 4505,
    30, 11, MST_ASMODEUS, CE_DIVINE, S_SHOUT,
    I_HUMAN, HT_LAND, 10, DEFAULT_ENERGY,
    MU_ALL, SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ASMODEUS}, TILE_CORPSE_ASMODEUS // BCADDO: STAVELESS
},

{
    MONS_ERESHKIGAL, '&', WHITE, "Ereshkigal",
    M_UNIQUE | M_SEE_INVIS | M_SPEAKS | M_FEMALE | M_TALL_TILE | M_SPELLCASTER,
    MR_RES_ELEC | MR_RES_POISON | MR_RES_COLD | mrd(MR_RES_NEG,3)
     | MR_RES_ACID | MR_VUL_SLASHING | MR_RES_PIERCING,
    15, MONS_HELL_LORD, MONS_ERESHKIGAL, MH_DEMONIC, 200,
    { {AT_HIT, AF_DRAIN_XP, 40}, {AT_SHIELD, AF_PLAIN, 40}, AT_NO_ATK, AT_NO_ATK },
    18, 3510,
    10, 47, MST_ERESHKIGAL, CE_DIVINE, S_SHOUT,
    I_HUMAN, HT_LAND, 14, DEFAULT_ENERGY,
    muf(MU_MELEE | MU_EVOKE | MU_THROW_NET), SIZE_LARGE, MON_SHAPE_HUMANOID,
    {TILEP_MONS_ERESHKIGAL}, TILE_CORPSE_ERESHKIGAL
},

// Impossible to hit, impossible to damage, immune to everything,
// unkillable, just sits there doing nothing but casting Shadow Creatures
// over and over.
{
    MONS_TEST_SPAWNER, 'X', WHITE, "test spawner",
    M_STATIONARY | M_INSUBSTANTIAL | M_NO_POLY_TO | M_FAST_REGEN,
    mrd(MR_RES_ELEC | MR_RES_FIRE | MR_RES_COLD
        | MR_RES_ACID, 4) | MR_RES_ROTTING | MR_RES_STICKY_FLAME,
    15, MONS_TEST_SPAWNER, MONS_TEST_SPAWNER, MH_ELEMENTAL, MAG_IMMUNE,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1000, 10000000,
    127, 127, MST_TEST_SPAWNER, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_AMPHIBIOUS, 14, SWIM_ENERGY(6),
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_MISC,
    {TILEP_MONS_TEST_SPAWNER}, TILE_ERROR
},

// tons of hp, just sits there doing nothing except being a target.
{
    MONS_TEST_STATUE, 'X', WHITE, "test statue",
    M_STATIONARY | M_NO_POLY_TO | M_FAST_REGEN | M_NO_THREAT,
    MR_NO_FLAGS,
    15, MONS_TEST_STATUE, MONS_TEST_STATUE, MH_ELEMENTAL, 20,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1000, 10000000,
    0, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_BRAINLESS, HT_AMPHIBIOUS, 14, SWIM_ENERGY(6),
    MU_NOTHING, SIZE_LARGE, MON_SHAPE_MISC,
    {TILEP_MONS_TEST_SPAWNER}, TILE_ERROR
},


// an unspecified monster
{
    MONS_SENSED, '{', RED, "sensed monster",
    M_CANT_SPAWN,
    MR_NO_FLAGS,
    0, MONS_SENSED, MONS_SENSED, MH_ELEMENTAL, 0,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 10,
    0, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_MISC,
    {TILE_UNSEEN_MONSTER}
},

{
    MONS_SENSED_FRIENDLY, '{', GREEN, "friendly sensed monster",
    M_CANT_SPAWN,
    MR_NO_FLAGS,
    0, MONS_SENSED, MONS_SENSED, MH_ELEMENTAL, 0,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 10,
    0, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_MISC,
    {TILE_MONS_SENSED_FRIENDLY}, TILE_ERROR
},

{
    MONS_SENSED_TRIVIAL, '{', BLUE, "trivial sensed monster",
    M_CANT_SPAWN,
    MR_NO_FLAGS,
    0, MONS_SENSED, MONS_SENSED, MH_ELEMENTAL, 0,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 10,
    0, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_MISC,
    {TILE_MONS_SENSED_TRIVIAL}, TILE_ERROR
},

{
    MONS_SENSED_EASY, '{', LIGHTGRAY, "easy sensed monster",
    M_CANT_SPAWN,
    MR_NO_FLAGS,
    0, MONS_SENSED, MONS_SENSED, MH_ELEMENTAL, 0,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 10,
    0, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_MISC,
    {TILE_MONS_SENSED_EASY}, TILE_ERROR
},

{
    MONS_SENSED_TOUGH, '{', YELLOW, "tough sensed monster",
    M_CANT_SPAWN,
    MR_NO_FLAGS,
    0, MONS_SENSED, MONS_SENSED, MH_ELEMENTAL, 0,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 10,
    0, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_MISC,
    {TILE_MONS_SENSED_TOUGH}, TILE_ERROR
},

{
    MONS_SENSED_NASTY, '{', LIGHTRED, "nasty sensed monster",
    M_CANT_SPAWN,
    MR_NO_FLAGS,
    0, MONS_SENSED, MONS_SENSED, MH_ELEMENTAL, 0,
    { AT_NO_ATK, AT_NO_ATK, AT_NO_ATK, AT_NO_ATK },
    1, 10,
    0, 0, MST_NO_SPELLS, CE_NOCORPSE, S_SILENT,
    I_ANIMAL, HT_LAND, 10, DEFAULT_ENERGY,
    MU_NOTHING, SIZE_MEDIUM, MON_SHAPE_MISC,
    {TILE_MONS_SENSED_NASTY}, TILE_ERROR
},

};