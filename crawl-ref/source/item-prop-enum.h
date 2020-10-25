#pragma once

#include <vector>

#include "tag-version.h"

using std::vector;

/* Don't change the order of any enums in this file unless you are breaking
 * save compatibility. See ../docs/develop/save_compatibility.txt for
 * more details, including how to schedule both the current and future
 * enum orders.
 *
 * If you do break compatibility and change the order, be sure to change
 * rltiles/dc-item.txt to match.
 */

enum armour_type
{
    ARM_ROBE, // order of mundane armour matters to _upgrade_body_armour
    ARM_FIRST_MUNDANE_BODY = ARM_ROBE,
    ARM_LEATHER_ARMOUR,
    ARM_RING_MAIL,
    ARM_SCALE_MAIL,
    ARM_CHAIN_MAIL,
    ARM_PLATE_ARMOUR,
    ARM_LAST_MUNDANE_BODY = ARM_PLATE_ARMOUR,
#if TAG_MAJOR_VERSION > 34
    ARM_CRYSTAL_PLATE_ARMOUR,
#endif

    ARM_CLOAK,
#if TAG_MAJOR_VERSION > 34
    ARM_SCARF,
#endif

    ARM_SKULL,
    ARM_HAT,
    ARM_HELMET,

    ARM_GLOVES,
    ARM_CLAW,

    ARM_BOOTS,

#if TAG_MAJOR_VERSION == 34
    ARM_BUCKLER, // order of shields matters
    ARM_FIRST_SHIELD = ARM_BUCKLER,
    ARM_SHIELD,
    ARM_LARGE_SHIELD,
    ARM_LAST_SHIELD = ARM_LARGE_SHIELD,

    ARM_CRYSTAL_PLATE_ARMOUR,
#endif

    ARM_ANIMAL_SKIN,

    ARM_CAP,
    ARM_TROLL_LEATHER_ARMOUR,

#if TAG_MAJOR_VERSION == 34
    ARM_FIRE_DRAGON_HIDE,
#endif
    ARM_FIRE_DRAGON_ARMOUR,
#if TAG_MAJOR_VERSION == 34
    ARM_ICE_DRAGON_HIDE,
#endif
    ARM_ICE_DRAGON_ARMOUR,
#if TAG_MAJOR_VERSION == 34
    ARM_STEAM_DRAGON_HIDE,
#endif
    ARM_STEAM_DRAGON_ARMOUR,
#if TAG_MAJOR_VERSION == 34
    ARM_ACID_DRAGON_HIDE,
#endif
    ARM_ACID_DRAGON_ARMOUR,
#if TAG_MAJOR_VERSION == 34
    ARM_STORM_DRAGON_HIDE,
#endif
    ARM_STORM_DRAGON_ARMOUR,
#if TAG_MAJOR_VERSION == 34
    ARM_GOLD_DRAGON_HIDE,
#endif
    ARM_GOLD_DRAGON_ARMOUR,
#if TAG_MAJOR_VERSION == 34
    ARM_SWAMP_DRAGON_HIDE,
#endif
    ARM_SWAMP_DRAGON_ARMOUR,
#if TAG_MAJOR_VERSION == 34
    ARM_PEARL_DRAGON_HIDE,
#endif
    ARM_PEARL_DRAGON_ARMOUR,
#if TAG_MAJOR_VERSION > 34
    ARM_SHADOW_DRAGON_ARMOUR,
    ARM_QUICKSILVER_DRAGON_ARMOUR,
#endif

    ARM_CENTAUR_BARDING,
    ARM_NAGA_BARDING,

#if TAG_MAJOR_VERSION == 34
    ARM_SHADOW_DRAGON_HIDE,
    ARM_SHADOW_DRAGON_ARMOUR,
    ARM_QUICKSILVER_DRAGON_HIDE,
    ARM_QUICKSILVER_DRAGON_ARMOUR,
    ARM_SCARF,
#endif

    ARM_SALAMANDER_HIDE_ARMOUR,
    ARM_DEEP_TROLL_LEATHER_ARMOUR,
    ARM_IRON_TROLL_LEATHER_ARMOUR,

    NUM_ARMOURS
};

enum shield_type
{
    SHD_BUCKLER,
    SHD_FIRST_NORM = SHD_BUCKLER,
    SHD_SHIELD,
    SHD_LARGE_SHIELD,
    SHD_LAST_NORM = SHD_LARGE_SHIELD,
    SHD_SAI,
    SHD_TARGE,
    SHD_NUNCHAKU,
    NUM_SHIELDS
};

enum manual_type
{
    MAN_SMALL,
    MAN_NORMAL,
    MAN_LARGE,
    MAN_ARTEFACT = 10,
    NUM_MANUALS
};

enum armour_property_type
{
    PARM_AC,
    PARM_EVASION
};

const int SP_FORBID_EGO   = -1;
const int SP_FORBID_BRAND = -1;
const int SP_UNKNOWN_BRAND = 31; // seen_weapon is a 32-bit bitfield
const int SP_UNKNOWN_EGO = 63; // seen_armour is a 64-bit bitfield
// BCADDO: Seen bitfield for staff facets?

enum facet_type // item_def.special
{
    SPSTF_FORBID_FACET = -1, // Not sure I need this for anything but to match the others.
    SPSTF_NORMAL,
    SPSTF_SHIELD,
    SPSTF_FLAY,
    SPSTF_WARP,
    SPSTF_ENERGY,
    SPSTF_REAVER,
    SPSTF_WIZARD,
    SPSTF_SCOPED,
    SPSTF_MENACE,
    SPSTF_ACCURACY,
    SPSTF_CHAOS,
    NUM_SPECIAL_STAVES,
};

// Be sure to update _debug_acquirement_stats and _str_to_ego to match.
enum brand_type // item_def.special
{
    SPWPN_FORBID_BRAND = -1,
    SPWPN_NORMAL,
    SPWPN_MOLTEN,
    SPWPN_FREEZING,
    SPWPN_HOLY_WRATH,
    SPWPN_ELECTROCUTION,
#if TAG_MAJOR_VERSION == 34
    SPWPN_ORC_SLAYING,
#endif
    SPWPN_DRAGON_SLAYING,
    SPWPN_VENOM,
    SPWPN_PROTECTION,
    SPWPN_DRAINING,
    SPWPN_SPEED,
    SPWPN_VORPAL,
#if TAG_MAJOR_VERSION == 34
    SPWPN_FLAME,   // ranged, only
    SPWPN_FROST,   // ranged, only
#endif
    SPWPN_VAMPIRISM,
    SPWPN_PAIN,
    SPWPN_ANTIMAGIC,
    SPWPN_DISTORTION,
#if TAG_MAJOR_VERSION == 34
    SPWPN_REACHING,
    SPWPN_RETURNING,
#endif
    SPWPN_CHAOS,
#if TAG_MAJOR_VERSION == 34
    SPWPN_EVASION,
#endif

#if TAG_MAJOR_VERSION == 34
    SPWPN_CONFUSE, // XXX not a real weapon brand, only for Confusing Touch
#endif
    SPWPN_PENETRATION,
    SPWPN_REAPING,
    SPWPN_SILVER,
    SPWPN_ACID,
    SPWPN_SPECTRAL,
    MAX_GHOST_BRAND = SPWPN_SPECTRAL,

// From this point on save compat is irrelevant.
    NUM_REAL_SPECIAL_WEAPONS,

#if TAG_MAJOR_VERSION > 34
    SPWPN_CONFUSE, // Confusing Touch only for the moment
#endif
    SPWPN_DEBUG_RANDART,
    NUM_SPECIAL_WEAPONS,
};
COMPILE_CHECK(NUM_SPECIAL_WEAPONS <= SP_UNKNOWN_BRAND);

enum corpse_type
{
    CORPSE_BODY,
    CORPSE_SKELETON,
};

enum hands_reqd_type
{
    HANDS_ONE,
    HANDS_TWO,
};

enum jewellery_type
{
#if TAG_MAJOR_VERSION == 34
    RING_REGENERATION,
#endif
    RING_PROTECTION,
    RING_FIRST_RING = RING_PROTECTION,
#if TAG_MAJOR_VERSION == 34
    RING_CHAOS,
#endif
    RING_POISON_RESISTANCE,
#if TAG_MAJOR_VERSION == 34
    RING_PROTECTION_FROM_COLD,
#endif
    RING_STRENGTH,
    RING_SLAYING,
#if TAG_MAJOR_VERSION == 34
    RING_SEE_INVISIBLE,
#endif
    RING_RESIST_CORROSION,
    RING_ATTENTION,
    RING_TELEPORTATION,
    RING_EVASION,
#if TAG_MAJOR_VERSION == 34
    RING_SUSTAIN_ATTRIBUTES,
#endif
    RING_STEALTH,
    RING_DEXTERITY,
    RING_INTELLIGENCE,
    RING_WIZARDRY,
    RING_MAGICAL_POWER,
#if TAG_MAJOR_VERSION == 34
    RING_FLIGHT,
#endif
    RING_LIFE_PROTECTION,
    RING_PROTECTION_FROM_MAGIC,
    RING_FIRE,
    RING_ICE,
#if TAG_MAJOR_VERSION == 34
    RING_TELEPORT_CONTROL,
#endif
    NUM_RINGS,                         //   keep as last ring; should not overlap
                                       //   with amulets!
    // RINGS after num_rings are for unique types for artefacts
    //   (no non-artefact version).
    // Currently none.
    // XXX: trying to add one doesn't actually work

    AMU_RAGE = 35,
    AMU_FIRST_AMULET = AMU_RAGE,
    AMU_HARM,
    AMU_ACROBAT,
    AMU_MANA_REGENERATION,
    AMU_THE_GOURMAND,
    AMU_CHAOS,
#if TAG_MAJOR_VERSION == 34
    AMU_CONTROLLED_FLIGHT,
#endif
    AMU_INACCURACY,
    AMU_NOTHING,
    AMU_GUARDIAN_SPIRIT,
    AMU_FAITH,
    AMU_REFLECTION,
    AMU_REGENERATION,

    NUM_JEWELLERY
};

enum class launch_retval
{
    BUGGY = -1, // could be 0 maybe? TODO: test
    FUMBLED,
    LAUNCHED,
    THROWN,
};

enum misc_item_type
{
#if TAG_MAJOR_VERSION == 34
    MISC_BOTTLED_EFREET,
#endif
    MISC_FAN_OF_GALES,
    MISC_LAMP_OF_FIRE,
#if TAG_MAJOR_VERSION == 34
    MISC_STONE_OF_TREMORS,
#endif
    MISC_LANTERN_OF_SHADOWS,
    MISC_HORN_OF_GERYON,
    MISC_BOX_OF_BEASTS,
    MISC_CRYSTAL_BALL_OF_ENERGY,
    MISC_EMPTY_BOTTLE,
    MISC_LIGHTNING_ROD,

    MISC_DECK_OF_ESCAPE,
    MISC_FIRST_DECK = MISC_DECK_OF_ESCAPE,
    MISC_DECK_OF_DESTRUCTION,
#if TAG_MAJOR_VERSION == 34
    MISC_DECK_OF_DUNGEONS,
#endif
    MISC_DECK_OF_SUMMONING,
#if TAG_MAJOR_VERSION == 34
    MISC_DECK_OF_WONDERS,
#endif
    MISC_DECK_OF_PUNISHMENT,

#if TAG_MAJOR_VERSION == 34
    MISC_DECK_OF_WAR,
    MISC_DECK_OF_CHANGES,
    MISC_DECK_OF_DEFENCE,
    MISC_LAST_DECK = MISC_DECK_OF_DEFENCE,

    MISC_RUNE_OF_ZOT,
#else
    MISC_LAST_DECK = MISC_DECK_OF_PUNISHMENT,
#endif

    MISC_QUAD_DAMAGE, // Sprint only

    MISC_PHIAL_OF_FLOODS,
    MISC_SACK_OF_SPIDERS,
    MISC_ZIGGURAT,

    MISC_PHANTOM_MIRROR,
#if TAG_MAJOR_VERSION == 34
    MISC_DECK_OF_ODDITIES,
    MISC_XOMS_CHESSBOARD,
#endif
    MISC_CONDENSER_VANE,

    NUM_MISCELLANY,
    MISC_DECK_UNKNOWN = NUM_MISCELLANY,
};

// in no particular order (but we need *a* fixed order for dbg-scan)
const vector<misc_item_type> deck_types =
{
    MISC_DECK_OF_ESCAPE, MISC_DECK_OF_DESTRUCTION,
#if TAG_MAJOR_VERSION == 34
    MISC_DECK_OF_SUMMONING, MISC_DECK_OF_WONDERS, MISC_DECK_OF_ODDITIES,
#endif
    MISC_DECK_OF_PUNISHMENT, MISC_DECK_OF_WAR,
#if TAG_MAJOR_VERSION == 34
    MISC_DECK_OF_CHANGES, MISC_DECK_OF_DEFENCE, MISC_DECK_OF_DUNGEONS,
#endif
};

// in no particular order (but we need *a* fixed order for dbg-scan)
const vector<misc_item_type> misc_types =
{
    MISC_FAN_OF_GALES, MISC_LAMP_OF_FIRE,
#if TAG_MAJOR_VERSION == 34
    MISC_STONE_OF_TREMORS,
#endif
    MISC_LANTERN_OF_SHADOWS, MISC_HORN_OF_GERYON, MISC_BOX_OF_BEASTS,
    MISC_CRYSTAL_BALL_OF_ENERGY, MISC_LIGHTNING_ROD, MISC_PHIAL_OF_FLOODS,
    MISC_QUAD_DAMAGE, MISC_SACK_OF_SPIDERS, MISC_PHANTOM_MIRROR,
#if TAG_MAJOR_VERSION == 34
    MISC_XOMS_CHESSBOARD,
#endif
    MISC_ZIGGURAT,
#if TAG_MAJOR_VERSION == 34
    MISC_BOTTLED_EFREET, 
#endif
    MISC_EMPTY_BOTTLE,
    MISC_CONDENSER_VANE,
};

enum missile_type
{
    MI_NEEDLE,
    MI_ARROW,
    MI_BOLT,
    MI_TRIPLE_BOLT,
    MI_DOUBLE_BOLT,
    MI_JAVELIN,

    MI_STONE,
    MI_BLADE,
    MI_ROOT,
    MI_SNOWBALL,
    MI_SKULL,
    MI_BONE,
    MI_MUD,
    MI_SEASHELL,
    MI_OOZE,
    MI_PANDEMONIUM,
    MI_ABYSS,
    MI_GOLD,
    MI_BANDAGE,

    MI_LARGE_ROCK,
    MI_SLING_BULLET,
    MI_THROWING_NET,
    MI_TOMAHAWK,

    MI_PIE,

    NUM_MISSILES,
    MI_NONE             // was MI_EGGPLANT... used for launch type detection
};

enum rune_type
{
    RUNE_SWAMP,
    RUNE_SNAKE,
    RUNE_SHOALS,
    RUNE_SLIME,
    RUNE_ELF, // only used in sprints
    RUNE_VAULTS,
    RUNE_TOMB,

    RUNE_DIS,
    RUNE_GEHENNA,
    RUNE_COCYTUS,
    RUNE_TARTARUS,

    RUNE_ABYSSAL,

    RUNE_DEMONIC,

    // order must match monsters
    RUNE_MNOLEG,
    RUNE_LOM_LOBON,
    RUNE_CEREBOV,
    RUNE_GLOORX_VLOQ,

    RUNE_SPIDER,
    RUNE_FOREST, // only used in sprints
    RUNE_RUINS,
    NUM_RUNE_TYPES
};

enum scroll_type
{
#if TAG_MAJOR_VERSION == 34
    SCR_IDENTIFY,
#endif
    SCR_TELEPORTATION,
    SCR_FEAR,
    SCR_ATTENTION,
#if TAG_MAJOR_VERSION == 34
    SCR_REMOVE_CURSE,
#endif
    SCR_SUMMONING,
#if TAG_MAJOR_VERSION == 34
    SCR_ENCHANT_WEAPON,
#endif
    SCR_ENCHANT,
    SCR_TORMENT,
    SCR_RANDOM_USELESSNESS,
#if TAG_MAJOR_VERSION == 34
    SCR_CURSE_WEAPON,
    SCR_CURSE_ARMOUR,
#endif
    SCR_IMMOLATION,
    SCR_BLINKING,
    SCR_MAGIC_MAPPING,
    SCR_FOG,
    SCR_ACQUIREMENT,
#if TAG_MAJOR_VERSION == 34
    SCR_ENCHANT_WEAPON_II,
#endif
    SCR_BLESS_ITEM,
#if TAG_MAJOR_VERSION == 34
    SCR_RECHARGING,
    SCR_ENCHANT_WEAPON_III,
#endif
    SCR_HOLY_WORD,
    SCR_VULNERABILITY,
    SCR_SILENCE,
    SCR_AMNESIA,
#if TAG_MAJOR_VERSION == 34
    SCR_CURSE_JEWELLERY,
#endif
    NUM_SCROLLS
};

// Be sure to update _debug_acquirement_stats and str_to_ego to match.
enum special_armour_type
{
    SPARM_FORBID_EGO = -1,
    SPARM_NORMAL,
    SPARM_RUNNING,
    SPARM_FIRE_RESISTANCE,
    SPARM_COLD_RESISTANCE,
    SPARM_POISON_RESISTANCE,
    SPARM_IMPROVED_VISION,
    SPARM_INVISIBILITY,
    SPARM_STRENGTH,
    SPARM_DEXTERITY,
    SPARM_INTELLIGENCE,
    SPARM_PONDEROUSNESS,
    SPARM_INSULATION,
    SPARM_MAGIC_RESISTANCE,
    SPARM_PROTECTION,
    SPARM_STEALTH,
    SPARM_RESISTANCE,
    SPARM_POSITIVE_ENERGY,
    SPARM_ARCHMAGI,
    SPARM_PRESERVATION,
    SPARM_REFLECTION,
    SPARM_SPIRIT_SHIELD,
    SPARM_ARCHERY,
    SPARM_SOFT,
    SPARM_REPULSION,
    SPARM_CLOUD_IMMUNE,
    SPARM_HIGH_PRIEST,
    SPARM_WIELDING,
    SPARM_STURDY,
    SPARM_HARM,
    SPARM_SHADOWS,
    SPARM_RAMPAGING,
    NUM_REAL_SPECIAL_ARMOURS,
    NUM_SPECIAL_ARMOURS,
};
// We have space for 32 brands in the bitfield.
COMPILE_CHECK(NUM_SPECIAL_ARMOURS <= SP_UNKNOWN_EGO);

// Be sure to update _str_to_ego to match.
enum special_missile_type // to separate from weapons in general {dlb}
{
    SPMSL_FORBID_BRAND = -1,
    SPMSL_NORMAL,
    SPMSL_FLAME,
    SPMSL_FROST,
    SPMSL_POISONED,
    SPMSL_CURARE,                      // Needle-only brand
    SPMSL_RETURNING,
    SPMSL_CHAOS,
    SPMSL_PENETRATION,
    SPMSL_DISPERSAL,
    SPMSL_EXPLODING,
    SPMSL_STEEL,
    SPMSL_SILVER,
    SPMSL_PETRIFICATION,               // needle only from here on
#if TAG_MAJOR_VERSION == 34
    SPMSL_SLOW,
#endif
    SPMSL_SLEEP,
    SPMSL_CONFUSION,
#if TAG_MAJOR_VERSION == 34
    SPMSL_SICKNESS,
#endif
    SPMSL_FRENZY,                      
    SPMSL_BLINDING,                    
    NUM_REAL_SPECIAL_MISSILES,
    NUM_SPECIAL_MISSILES,
};

enum special_ring_type // jewellery env.item[].special values
{
    SPRING_RANDART = 200,
    SPRING_UNRANDART = 201,
};

enum stave_type
{
#if TAG_MAJOR_VERSION == 34
    STAFF_WIZARDRY,
#endif
    STAFF_TRANSMUTATION,
    STAFF_FIRE,
    STAFF_COLD,
    STAFF_POISON,
#if TAG_MAJOR_VERSION == 34
    STAFF_ENERGY,
#endif
    STAFF_DEATH,
#if TAG_MAJOR_VERSION == 34
    STAFF_CONJURATION,
#endif
    STAFF_NOTHING,
    STAFF_LIFE,
    STAFF_AIR,
    STAFF_EARTH,
#if TAG_MAJOR_VERSION == 34
    STAFF_CHANNELING,
#endif
    NUM_STAVES,
};

#if TAG_MAJOR_VERSION == 34
enum rod_type
{
    ROD_LIGHTNING,
    ROD_SWARM,
    ROD_IGNITION,
    ROD_CLOUDS,
    ROD_DESTRUCTION,
    ROD_INACCURACY,
    ROD_WARDING,
    ROD_SHADOWS,
    ROD_IRON,
    ROD_VENOM,
    NUM_RODS,
};
#endif

enum weapon_type
{
    WPN_CLUB,
    WPN_WHIP,
    WPN_HAMMER,
    WPN_MACE,
    WPN_FLAIL,
    WPN_MORNINGSTAR,
#if TAG_MAJOR_VERSION == 34
    WPN_SPIKED_FLAIL,
#endif
    WPN_DIRE_FLAIL,
    WPN_EVENINGSTAR,
    WPN_GREAT_MACE,

    WPN_DAGGER,
    WPN_TANTO,
    WPN_SHORT_SWORD,
    WPN_RAPIER,

    WPN_FALCHION,
    WPN_LONG_SWORD,
    WPN_SCIMITAR,
    WPN_GREAT_SWORD,

    WPN_HAND_AXE,
    WPN_WAR_AXE,
    WPN_BROAD_AXE,
    WPN_BATTLEAXE,
    WPN_EXECUTIONERS_AXE,

    WPN_SPEAR,
    WPN_TRIDENT,
    WPN_HALBERD,
    WPN_GLAIVE,
    WPN_BARDICHE,

    WPN_KRIS,

#if TAG_MAJOR_VERSION > 34
    WPN_HAND_CROSSBOW,
#endif
    WPN_ARBALEST,
#if TAG_MAJOR_VERSION > 34
    WPN_TRIPLE_CROSSBOW,
#endif

    WPN_SHORTBOW,
    WPN_LONGBOW,

#if TAG_MAJOR_VERSION > 34
    WPN_HUNTING_SLING,
    WPN_FUSTIBALUS,
#endif

    WPN_DEMON_WHIP,
    WPN_GIANT_CLUB,
    WPN_GIANT_SPIKED_CLUB,

    WPN_DEMON_BLADE,
    WPN_DOUBLE_SWORD,
    WPN_TRIPLE_SWORD,

    WPN_DEMON_TRIDENT,
    WPN_SCYTHE,

    WPN_STAFF,          // Just used for the weapon stats for magical staves.
    WPN_QUARTERSTAFF,
    WPN_LAJATANG,

#if TAG_MAJOR_VERSION == 34
    WPN_HUNTING_SLING,

    WPN_BLESSED_FALCHION,
    WPN_BLESSED_LONG_SWORD,
    WPN_BLESSED_SCIMITAR,
    WPN_BLESSED_GREAT_SWORD,
#endif
    WPN_EUDEMON_BLADE,
#if TAG_MAJOR_VERSION == 34
    WPN_BLESSED_DOUBLE_SWORD,
    WPN_BLESSED_TRIPLE_SWORD,
#endif
    WPN_SACRED_SCOURGE,
    WPN_TRISHULA,

#if TAG_MAJOR_VERSION == 34
    WPN_FUSTIBALUS,
    WPN_HAND_CROSSBOW,
    WPN_TRIPLE_CROSSBOW,
#endif

    WPN_KATAR,

    // These are only used as part of a hacky way of getting semi-customizable start stats
    WPN_AVERAGE,
    WPN_STRONG,
    WPN_INTELLIGENT,
    WPN_DEFT,

    WPN_MANGONEL,

    WPN_CLEAVER,

    WPN_PICKAXE,

    WPN_LEIOMANO,

    NUM_WEAPONS,

// special cases
    WPN_UNARMED,
    WPN_UNKNOWN,
    WPN_RANDOM,
    WPN_VIABLE,

#if TAG_MAJOR_VERSION == 34
// thrown weapons (for hunter weapon selection) - rocks, javelins, tomahawks
    WPN_THROWN,
#endif
};

enum weapon_property_type
{
    PWPN_DAMAGE,
    PWPN_HIT,
    PWPN_SPEED,
    PWPN_ACQ_WEIGHT,
};

enum shield_property_type
{
    PSHD_HYBRID,
    PSHD_SH,
    PSHD_ER,
    PSHD_HIT,
    PSHD_DAMAGE,
    PSHD_SPEED,
    PSHD_SIZE,
};


enum wpn_flags
{
    WPNF_NO_FLAGS       = 0x0000,

    // Made of non-metal materials and cannot recieve "molten" brand.
    WPNF_WOODEN         = 0x0001,

    // Lightweight construction gives an oddly fast mindelay.
    WPNF_LIGHTWEIGHT    = 0x0002,

    // Heavy and awkward weapon doesn't get any faster with skill.
    WPNF_HEAVYWEIGHT    = 0x0004,

    // Chance to hit with a spike directly and crit for double damage.
    WPNF_SPIKY          = 0x0008,
};

enum vorpal_damage_type
{
    // These are the types of damage a weapon can do. You can set more
    // than one of these.
    DAM_BASH            = 0x0000,       // non-melee weapon blugeoning
    DAM_BLUDGEON        = 0x0001,       // crushing
    DAM_SLICE           = 0x0002,       // slicing/chopping
    DAM_PIERCE          = 0x0004,       // stabbing/piercing
    DAM_WHIP            = 0x0008,       // whip slashing
    DAM_FORCE           = 0x0010,       // used by monster attacks that ignore weapon resistances
    DAM_CRIT            = 0x0020,       // used by critical hits
    DAM_MAX_TYPE        = DAM_CRIT,

    // These are used for vorpal weapon descriptions. You shouldn't set
    // more than one of these.
    DVORP_NONE          = 0x0000,       // used for non-melee weapons
    DVORP_CRUSHING      = 0x1000,
    DVORP_SLICING       = 0x2000,
    DVORP_PIERCING      = 0x3000,
    DVORP_CHOPPING      = 0x4000,       // used for axes
    DVORP_SLASHING      = 0x5000,       // used for whips

    DVORP_CLAWING       = 0x6000,       // claw damage
    DVORP_TENTACLE      = 0x7000,       // tentacle damage

    // These are shortcuts to tie vorpal/damage types for easy setting...
    // as above, setting more than one vorpal type is trouble.
    DAMV_CRUSHING       = DVORP_CRUSHING | DAM_BLUDGEON,
    DAMV_SLICING        = DVORP_SLICING  | DAM_SLICE,
    DAMV_PIERCING       = DVORP_PIERCING | DAM_PIERCE,
    DAMV_CHOPPING       = DVORP_CHOPPING | DAM_SLICE,
    DAMV_SLASHING       = DVORP_SLASHING | DAM_WHIP,

    DAM_MASK            = 0x0fff,       // strips vorpal specification
    DAMV_MASK           = 0xf000,       // strips non-vorpal specification
};

enum wand_type
{
    WAND_FLAME,
#if TAG_MAJOR_VERSION == 34
    WAND_FROST_REMOVED,
    WAND_SLOWING_REMOVED,
#endif
    WAND_HASTING,
#if TAG_MAJOR_VERSION == 34
    WAND_MAGIC_DARTS_REMOVED,
#endif
    WAND_HEAL_WOUNDS,
    WAND_ENSNARE,
#if TAG_MAJOR_VERSION == 34
    WAND_FIRE_REMOVED,
    WAND_COLD_REMOVED,
    WAND_CONFUSION_REMOVED,
    WAND_INVISIBILITY_REMOVED,
    WAND_DIGGING_REMOVED,
#endif
    WAND_ICEBLAST,
#if TAG_MAJOR_VERSION == 34
    WAND_TELEPORTATION_REMOVED,
#endif
    WAND_DRAIN,
    WAND_POLYMORPH,
    WAND_ENSLAVEMENT,
    WAND_ACID,
    WAND_RANDOM_EFFECTS,
    WAND_DISINTEGRATION,
    WAND_CLOUDS,
    WAND_SCATTERSHOT,
    NUM_WANDS
};

enum food_type
{
    FOOD_RATION,
#if TAG_MAJOR_VERSION == 34
    FOOD_BREAD_RATION,
    FOOD_PEAR,
    FOOD_APPLE,
    FOOD_CHOKO,
#endif
#if TAG_MAJOR_VERSION == 34
    FOOD_ROYAL_JELLY,   // was: royal jelly
    FOOD_UNUSED, // was: royal jelly and/or pizza
    FOOD_FRUIT,  // was: snozzcumber
    FOOD_PIZZA,
    FOOD_APRICOT,
    FOOD_ORANGE,
    FOOD_BANANA,
    FOOD_STRAWBERRY,
    FOOD_RAMBUTAN,
    FOOD_LEMON,
    FOOD_GRAPE,
    FOOD_SULTANA,
    FOOD_LYCHEE,
    FOOD_BEEF_JERKY,
    FOOD_CHEESE,
    FOOD_SAUSAGE,
#endif
    FOOD_CHUNK,
#if TAG_MAJOR_VERSION == 34
    FOOD_AMBROSIA,
#endif
    NUM_FOODS
};
