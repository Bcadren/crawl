#pragma once

enum beam_type                  // bolt::flavour
{
    BEAM_NONE,

    BEAM_MISSILE,
    BEAM_MMISSILE,                //    and similarly irresistible things
    BEAM_FIRE,
    BEAM_COLD,
    BEAM_MAGIC,
    BEAM_ELECTRICITY,
    BEAM_POISON,
    BEAM_NEG,                   // Negative energy w/o drain XP.
    BEAM_DRAIN,                 // Negative energy with drain XP.
    BEAM_ACID,
    BEAM_MIASMA,
    BEAM_WATER,

    BEAM_SPORE,
    BEAM_POISON_ARROW,
    BEAM_HELLFIRE,
    BEAM_STICKY_FLAME,
    BEAM_STEAM,
    BEAM_ENERGY,
    BEAM_HOLY,
    BEAM_FRAG,
    BEAM_SILVER_FRAG,
    BEAM_SILVER,
    BEAM_LAVA,
    BEAM_ICE,
    BEAM_FREEZE,                  // Identical to ice except it causes water to freeze.
    BEAM_ICY_SHARDS,              // Identical to ice except icy mons are immune.
    BEAM_DEVASTATION,
    BEAM_RANDOM,                  // currently translates into FIRE..ACID
    BEAM_CHAOS,
    BEAM_CHAOTIC,
    BEAM_ICY_DEVASTATION,
    BEAM_CHAOTIC_DEVASTATION,
    BEAM_ELDRITCH,                // Majin-Bo.
    BEAM_UNRAVELLED_MAGIC,

    // Enchantments
    BEAM_SLOW,
    BEAM_FIRST_ENCHANTMENT = BEAM_SLOW,
    BEAM_HASTE,
    BEAM_MIGHT,
    BEAM_HEALING,
    BEAM_CONFUSION,
    BEAM_INVISIBILITY,
    BEAM_DIGGING,
    BEAM_TELEPORT,
    BEAM_POLYMORPH,
    BEAM_MALMUTATE,
    BEAM_ENSLAVE,
    BEAM_BANISH,
    BEAM_PAIN,
    BEAM_DISPEL_UNDEAD,
    BEAM_DISINTEGRATION,
    BEAM_BLINK,
    BEAM_BLINK_CLOSE,
    BEAM_PETRIFY,
    BEAM_PORKALATOR,
    BEAM_HIBERNATION,
    BEAM_BERSERK,
    BEAM_SLEEP,
    BEAM_INNER_FLAME,
    BEAM_SENTINEL_MARK,
    BEAM_DIMENSION_ANCHOR,
    BEAM_VULNERABILITY,
    BEAM_MALIGN_OFFERING,
    BEAM_VIRULENCE,
    BEAM_AGILITY,
    BEAM_SAP_MAGIC,
    BEAM_DRAIN_MAGIC,
    BEAM_TUKIMAS_DANCE,
    BEAM_RESISTANCE,
    BEAM_UNRAVELLING,
    BEAM_SHARED_PAIN,
    BEAM_IRRESISTIBLE_CONFUSION,
    BEAM_INFESTATION,
    BEAM_AGONY,
    BEAM_SNAKES_TO_STICKS,
    BEAM_CHAOS_ENCHANTMENT,
    BEAM_ENTROPIC_BURST,
    BEAM_CHAOTIC_INFUSION,
    BEAM_VILE_CLUTCH,
    BEAM_CIGOTUVI,
    BEAM_LAST_ENCHANTMENT = BEAM_CIGOTUVI,

    BEAM_MEPHITIC,
    BEAM_AIR,
    BEAM_PETRIFYING_CLOUD,
    BEAM_ENSNARE,
    BEAM_CRYSTAL,
    BEAM_DEATH_RATTLE, // BCADDO: Deprecate (similar to Air or Neg).
    BEAM_MAGIC_CANDLE,
    BEAM_FOG,
    BEAM_BUTTERFLY,
    BEAM_BLOOD,
    BEAM_IRRADIATE,
    BEAM_PARADOXICAL,
    BEAM_CRYSTAL_SPEAR,
    BEAM_CRYSTAL_FIRE,
    BEAM_CRYSTAL_ICE,
    BEAM_ROT,
    BEAM_WAND_HEALING,
    BEAM_ACID_WAVE,
    BEAM_SLASH,
    BEAM_PIERCE,
    BEAM_BLUDGEON,
    BEAM_LAST_REAL = BEAM_BLUDGEON,
    BEAM_WAND_RANDOM, // Used for Monster Setup only.

    // For getting the visual effect of a beam.
    BEAM_VISUAL,
    BEAM_BOUNCY_TRACER,           // Used for random bolt tracer (bounces as
                                  // crystal bolt, but irresistible).

    BEAM_TORMENT_DAMAGE,          // Pseudo-beam for damage flavour.
    BEAM_FIRST_PSEUDO = BEAM_TORMENT_DAMAGE,

    NUM_BEAMS
};
