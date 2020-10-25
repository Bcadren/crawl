#pragma once

#include <vector>

#include "mon-info.h"
#include "tag-version.h"

using std::vector;

struct monster_info_flag_name
{
    monster_info_flags flag;
    string short_singular; // HUD display (empty to not show)
    string long_singular; // Targeter display
    string plural; // HUD display in groups (empty to not show)
};

// This vector is in display order. Roughly:
// - Attitudes and summon status
// - Bad things for the player (most buffs)
// - Vulnerabilities
// - Debuffs
// - Ally-only buffs
// - Misc. Low Priority
static const vector<monster_info_flag_name> monster_info_flag_names = {
    // Attitudes
    { MB_CHARMED, "charmed", "charmed", "charmed"},
    { MB_HEXED, "hexed", "control wrested from you", "hexed"},
    { MB_SUMMONED, "summoned", "summoned", "summoned"},
    { MB_WANDERING, "wandering", "wandering", "wandering"},
    { MB_PERM_SUMMON, "", "durably summoned", ""},
    { MB_SUMMONED_CAPPED, "expiring", "expiring", "expiring"},
    // Bad things for the player
    { MB_MESMERIZING, "mesmerising", "mesmerising", "mesmerising"},
    { MB_BERSERK, "berserk", "berserk", "berserk"},
    { MB_HASTED, "fast", "fast", "fast"},
    { MB_INNER_FLAME, "inner flame", "inner flame", "inner flame"},
    { MB_ENTROPIC_BURST, "entropic burst", "entropic burst", "entropic burst" },
    { MB_CHAOTIC_INFUSION, "chaotic infusion", "infusion of pure chaos", "chaotic infusions" },
    { MB_MIRROR_DAMAGE, "reflects damage", "reflecting injuries", "reflect damage"},
    { MB_BOUND_SOUL, "soul bound", "soul bound", "souls bound"},
    { MB_STRONG, "strong", "strong", "strong"},
    { MB_EMPOWERED_SPELLS, "empowered", "spells empowered", "empowered"},
    { MB_FULLY_CHARGED, "charged", "fully charged", "charged"},
    { MB_PARTIALLY_CHARGED, "charging", "partially charged", "charging"},
    { MB_AGILE, "agile", "unusually agile", "agile"},
    { MB_SWIFT, "swift", "covering ground quickly", "swift"},
    { MB_STILL_WINDS, "stilling wind", "stilling the winds", "stilling wind"},
    { MB_SILENCING, "silencing", "radiating silence", "silencing"},
    { MB_READY_TO_HOWL, "can howl", "ready to howl", "can howl"},
    { MB_BRILLIANCE_AURA, "brilliance aura", "brilliance aura", "brilliance auras"},
    { MB_TORNADO, "tornado", "surrounded by raging winds", "tornadoes"},
    { MB_CHAOSNADO, "cyclone", "surrounded by cyclonic chaos", "cyclones"},
    { MB_TORNADO_COOLDOWN, "gusty", "surrounded by restless winds", "gusty"},
    { MB_INSANE, "insane", "frenzied and insane", "insane"},
    { MB_FEAR_INSPIRING, "scary", "inspiring fear", "scary"},
    { MB_WORD_OF_RECALL, "chanting recall", "chanting recall", "chanting recall"},
    { MB_DEFLECT_MSL, "deflects missiles", "deflecting missiles", "deflect missiles" },
    { MB_REPEL_MSL, "repels missiles", "repelling missiles", "repel missiles"},
    { MB_TOXIC_RADIANCE, "toxic aura", "radiating toxic energy", "toxic auras"},
    { MB_BLACK_MARK, "black mark", "absorbing vital energies", "black marks"},
    { MB_OZOCUBUS_ARMOUR, "icy armour", "covered in an icy film", "icy armour"},
    { MB_CONDENSATION_SHIELD, "condensation shield", "protected by a floating shield of ice", "condensation shield"},
    { MB_ICEMAIL, "icemail", "surrounded by an icy envelope", "icemail"},
    { MB_SHROUD, "shrouded", "shrouded", "shrouded"},
    { MB_RESISTANCE, "resistant", "unusually resistant", "resistant"},
    { MB_INVISIBLE, "invisible", "slightly transparent", "invisible"},
    { MB_REGENERATION, "regenerating", "regenerating", "regenerating"},
    { MB_RAISED_MR, "resisting magic", "resistant to hostile enchantments", "resisting magic"},
    { MB_INJURY_BOND, "sheltered", "sheltered from injuries", "sheltered"},
    { MB_GOZAG_INCITED, "incited", "incited by Gozag", "incited"},
    { MB_CLOUD_RING_THUNDER, "clouds", "surrounded by thunder", "clouds" },
    { MB_CLOUD_RING_FLAMES, "clouds", "surrounded by flames", "clouds" },
    { MB_CLOUD_RING_CHAOS, "clouds", "surrounded by chaotic energy", "clouds" },
    { MB_CLOUD_RING_MUTATION, "clouds", "surrounded by mutagenic energy", "clouds" },
    { MB_CLOUD_RING_FOG, "clouds", "surrounded by fog", "clouds" },
    { MB_CLOUD_RING_ICE, "clouds", "surrounded by freezing clouds", "clouds" },
    { MB_CLOUD_RING_DRAINING, "clouds", "surrounded by negative energy", "clouds" },
    { MB_CLOUD_RING_ACID, "clouds", "surrounded by acidic fog", "clouds" },
    { MB_CLOUD_RING_MIASMA, "clouds", "surrounded by foul miasma", "clouds" },
    // Vulnerabilities
    { MB_POSSESSABLE, "possessable", "possessable", "possessable"},
    { MB_CAUGHT, "caught", "entangled in a net", "caught"},
    { MB_WEBBED, "webbed", "entangled in a web", "webbed"},
    { MB_PARALYSED, "paralysed", "paralysed", "paralysed"},
    { MB_PETRIFIED, "petrified", "petrified", "petrified"},
    { MB_CONFUSED, "confused", "confused", "confused"},
    { MB_DORMANT, "dormant", "dormant", "dormant"},
    { MB_SLEEPING, "asleep", "asleep", "asleep"},
    { MB_UNAWARE, "unaware", "unaware", "unaware"},
    { MB_BLIND, "blind", "blind", "blind"},
    { MB_INFESTATION, "infested", "infested", "infested"},
    // Debuffs
    { MB_DUMB, "stupefied", "stupefied", "stupefied"},
    { MB_PINNED, "pinned", "pinned", "pinned"},
    { MB_PETRIFYING, "petrifying", "petrifying slowly", "petrifying"},
    { MB_CIGOTUVI, "degenerating", "infected with magical cancer", "degenerating" },
    { MB_MAD, "mad", "lost in madness", "mad"},
    { MB_FLEEING, "fleeing", "fleeing", "fleeing"},
    { MB_DAZED, "dazed", "dazed", "dazed"},
    { MB_MUTE, "mute", "mute", "mute"},
    { MB_NO_CHARMS, "disenchanted", "unable to use charms magic", "disenchanted" },
    { MB_FROZEN, "frozen", "encased in ice", "frozen"},
    { MB_LAVA_CAKE, "stuck", "encased in volcanic glass", "stuck"},
    { MB_WATER_HOLD, "engulfed", "engulfed", "engulfed"},
    { MB_WATER_HOLD_DROWN, "drowning", "unable to breathe", "drowning"},
    // BCADDO: Check on Air Hold Drown and add a frog-specific case here.
    { MB_AIR_HOLD, "enveloped", "enveloped in fumes", "enveloped"},
    { MB_AIR_HOLD_DROWN, "asphyxiating", "unable to breathe", "asphyxiating"},
    { MB_BURNING, "burning", "covered in liquid flames", "burning"},
    { MB_POISONED, "poisoned", "poisoned", "poisoned"},
    { MB_MORE_POISONED, "very poisoned", "very poisoned", "very poisoned"},
    { MB_MAX_POISONED, "extremely poisoned", "extremely poisoned", "extremely poisoned"},
    { MB_SLOWED, "slow", "slow", "slow"},
    { MB_BREATH_WEAPON, "catching breath", "catching @possessive@ breath", "catching breath"},
    { MB_VULN_MAGIC, "hex vulnerable", "susceptible to hostile enchantments", "hex vulnerable"},
    { MB_FIRE_VULN, "fire vulnerable", "more vulnerable against fire", "fire vulnerable"},
    { MB_COLD_VULN, "cold vulnerable", "more vulnerable against cold", "cold vulnerable"},
    { MB_ELEC_VULN, "electrically vulnerable", "more vulnerable against electrical shocks", "electrically vulnerable"},
    { MB_PHYS_VULN, "physically vulnerable", "more vulnerable against physical attacks", "physically vulnerable"},
    { MB_POISON_VULN, "easily poisoned", "more vulnerable to poison", "easily poisoned"},
    { MB_WRETCHED, "misshapen", "misshapen and mutated", "misshapen"},
    { MB_CORROSION, "corroded", "covered in acid", "corroded"},
    { MB_FLAYED, "flayed", "covered in terrible wounds", "flayed"},
    { MB_GRASPING_ROOTS, "rooted", "constricted by roots", "rooted"},
    { MB_VILE_CLUTCH, "clutched", "constricted by zombie hands", "clutched"},
    { MB_BARBS, "skewered", "skewered by barbs", "skewered"},
    { MB_SICK, "sick", "sick", "sick"},
    { MB_WEAK, "weak", "weak", "weak"},
    { MB_LIGHTLY_DRAINED, "drained", "lightly drained", "drained"},
    { MB_HEAVILY_DRAINED, "drained", "heavily drained", "drained"},
    { MB_SAP_MAGIC, "magic-sapped", "magic-sapped", "magic-sapped"},
    { MB_GLOWING, "lit", "candlelit", "lit"},
    { MB_DIMENSION_ANCHOR, "anchored", "unable to translocate", "anchored"},
    { MB_SLOW_MOVEMENT, "struggling", "covering ground slowly", "struggling"},
    // Misc/low priority
    { MB_PAIN_BOND, "pain bonded", "sharing @possessive@ pain", "pain bonded"},
    { MB_IDEALISED, "idealised", "idealised", "idealised"},
    { MB_ENSLAVED, "", "disembodied soul", "" },
    { MB_CLINGING, "clinging", "wall clinging", "clinging" },
    { MB_INSECT_EGGS, "eggs implanted", "eggs implanted", "eggs implanted" },
};
