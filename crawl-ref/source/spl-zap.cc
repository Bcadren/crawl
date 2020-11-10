#include "AppHdr.h"

#include "spl-zap.h"

#include "beam.h"
#include "stepdown.h"
#include "zap-type.h"

static pair<spell_type, zap_type> _spl_zaps[] =
{
    { SPELL_FORCE_LANCE,            ZAP_FORCE_LANCE },
    { SPELL_THROW_FLAME,            ZAP_THROW_FLAME },
    { SPELL_THROW_FROST,            ZAP_THROW_FROST },
    { SPELL_PAIN,                   ZAP_PAIN },
    { SPELL_FLAME_TONGUE,           ZAP_FLAME_TONGUE },
    { SPELL_SHOCK,                  ZAP_SHOCK },
    { SPELL_STING,                  ZAP_STING },
    { SPELL_BOLT_OF_FIRE,           ZAP_BOLT_OF_FIRE },
    { SPELL_BOLT_OF_COLD,           ZAP_BOLT_OF_COLD },
    { SPELL_SANDBLAST,              ZAP_SANDBLAST },
    { SPELL_STONE_ARROW,            ZAP_STONE_ARROW },
    { SPELL_POISON_ARROW,           ZAP_POISON_ARROW },
    { SPELL_IRON_SHOT,              ZAP_IRON_SHOT },
    { SPELL_ARROW_OF_SPITE,         ZAP_ARROW_OF_SPITE },
    { SPELL_SILVER_SHOT,            ZAP_SILVER_SHOT },
    { SPELL_LIGHTNING_BOLT,         ZAP_LIGHTNING_BOLT },
    { SPELL_BOLT_OF_MAGMA,          ZAP_BOLT_OF_MAGMA },
    { SPELL_VENOM_BOLT,             ZAP_VENOM_BOLT },
    { SPELL_BOLT_OF_DRAINING,       ZAP_BOLT_OF_DRAINING },
    { SPELL_LEHUDIBS_CRYSTAL_SPEAR, ZAP_LEHUDIBS_CRYSTAL_SPEAR },
    { SPELL_FIREBALL,               ZAP_FIREBALL },
    { SPELL_GHOSTLY_FIREBALL,       ZAP_GHOSTLY_FIREBALL },
    { SPELL_MEPHITIC_CLOUD,         ZAP_MEPHITIC },
    { SPELL_VIOLENT_UNRAVELLING,    ZAP_UNRAVELLING },
    { SPELL_DISPEL_UNDEAD,          ZAP_DISPEL_UNDEAD },
    { SPELL_AGONY,                  ZAP_AGONY },
    { SPELL_WAND_DISINTEGRATE,           ZAP_DISINTEGRATE },
    { SPELL_THROW_ICICLE,           ZAP_THROW_ICICLE },
    // Wizard mode only.
    { SPELL_PORKALATOR,             ZAP_PORKALATOR },
    { SPELL_HURL_HELLFIRE,          ZAP_HURL_HELLFIRE },
    { SPELL_MAGIC_CANDLE,           ZAP_MAGIC_CANDLE },
    { SPELL_ENSLAVEMENT,            ZAP_ENSLAVEMENT },
    { SPELL_BANISHMENT,             ZAP_BANISHMENT },
    { SPELL_SLOW,                   ZAP_SLOW },
    { SPELL_CONFUSE,                ZAP_CONFUSE },
    { SPELL_HIBERNATION,            ZAP_HIBERNATION },
    { SPELL_SLEEP,                  ZAP_SLEEP },
    { SPELL_PETRIFY,                ZAP_PETRIFY },
    { SPELL_CIGOTUVIS_DEGENERATION, ZAP_CIGOTUVI },
    { SPELL_POLYMORPH,              ZAP_POLYMORPH },
    { SPELL_TELEPORT_OTHER,         ZAP_TELEPORT_OTHER },
    { SPELL_INNER_FLAME,            ZAP_INNER_FLAME },
    { SPELL_HASTE,                  ZAP_HASTE },
    { SPELL_INVISIBILITY,           ZAP_INVISIBILITY },
    { SPELL_DIG,                    ZAP_DIG },
    { SPELL_CRYSTAL_BOLT,           ZAP_CRYSTAL_BOLT },
    { SPELL_TUKIMAS_DANCE,          ZAP_TUKIMAS_DANCE },
    { SPELL_SNAKES_TO_STICKS,       ZAP_SNAKES_TO_STICKS },
    { SPELL_CORROSIVE_BOLT,         ZAP_CORROSIVE_BOLT },
    { SPELL_FORCE_LASSO,            ZAP_FORCE_LASSO },
    { SPELL_TONGUE_LASH,            ZAP_TONGUE_LASH },
    { SPELL_DEBUGGING_RAY,          ZAP_DEBUGGING_RAY },
    { SPELL_DRAIN_LIFE,             ZAP_DRAIN_LIFE},
    { SPELL_SONIC_WAVE,             ZAP_SONIC_WAVE},
    { SPELL_EMPOWERED_BREATH,       ZAP_EMPOWERED_BREATH},
    { SPELL_MALIGN_OFFERING,        ZAP_MALIGN_OFFERING},

    // wands
    { SPELL_WAND_FLAME,             ZAP_WAND_FLAME },
    { SPELL_WAND_ACID,              ZAP_WAND_ACID },
    { SPELL_WAND_DRAIN,             ZAP_WAND_DRAIN },
    { SPELL_WAND_POLYMORPH,         ZAP_WAND_POLYMORPH },
    { SPELL_WAND_HEALING,           ZAP_WAND_HEALING },
    { SPELL_WAND_ENSNARE,           ZAP_WAND_ENSNARE },
    { SPELL_WAND_HASTING,           ZAP_WAND_HASTING },

    // monster-specific
    { SPELL_BREATHE_DART,           ZAP_BREATHE_DART },
    { SPELL_PRIMAL_WAVE,            ZAP_PRIMAL_WAVE },
    { SPELL_BLINKBOLT,              ZAP_BLINKBOLT },
    { SPELL_STEAM_BALL,             ZAP_BREATHE_STEAM },
    { SPELL_DIMENSION_ANCHOR,       ZAP_DIMENSION_ANCHOR },
    { SPELL_STRIP_RESISTANCE,       ZAP_VULNERABILITY },
    { SPELL_SENTINEL_MARK,          ZAP_SENTINEL_MARK },
    { SPELL_VIRULENCE,              ZAP_VIRULENCE },
    { SPELL_SAP_MAGIC,              ZAP_SAP_MAGIC },
    { SPELL_DRAIN_MAGIC,            ZAP_DRAIN_MAGIC },
    { SPELL_HARPOON_SHOT,           ZAP_HARPOON_SHOT },
    { SPELL_THROW_PIE,              ZAP_THROW_PIE },
    { SPELL_SILVER_SPLINTERS,       ZAP_BREATHE_SILVER },
    { SPELL_BONE_SHARDS,            ZAP_BREATHE_BONE },
    { SPELL_NOXIOUS_CLOUD,          ZAP_NOXIOUS_CLOUD },
    { SPELL_POISONOUS_CLOUD,        ZAP_POISONOUS_CLOUD },
    { SPELL_ENERGY_BOLT,            ZAP_ENERGY_BOLT },
    { SPELL_SPLINTERSPRAY,          ZAP_SPLINTERSPRAY },
    { SPELL_THORN_VOLLEY,           ZAP_THORN_VOLLEY },
    { SPELL_HELLFIRE_BLAST,         ZAP_HELLFIRE },
    { SPELL_SPIT_POISON,            ZAP_SPIT_POISON },
    { SPELL_MIASMA_BREATH,          ZAP_MIASMA_BREATH },
    { SPELL_PETRIFYING_CLOUD,       ZAP_PETRIFYING_CLOUD },

    // These are all for zap -> spell lookup.
    { SPELL_QUICKSILVER_BOLT,       ZAP_QUICKSILVER_BOLT },
    { SPELL_QUICKSILVER_BOLT,       ZAP_BREATHE_POWER },
    { SPELL_STICKY_FLAME,           ZAP_STICKY_FLAME },
    { SPELL_STICKY_FLAME_RANGE,     ZAP_STICKY_FLAME_RANGE },
    { SPELL_BLINDING_SPRAY,         ZAP_BLINDING_SPRAY },
    { SPELL_STEAM_BALL,             ZAP_BREATHE_STEAM },
    { SPELL_ORB_OF_ELECTRICITY,     ZAP_ORB_OF_ELECTRICITY },
    { SPELL_CHILLING_BREATH,        ZAP_BREATHE_FROST },
    { SPELL_ICEBLAST,               ZAP_ICEBLAST },
    { SPELL_ACID_SPLASH,            ZAP_BREATHE_ACID },
    { SPELL_BORGNJORS_VILE_CLUTCH,  ZAP_VILE_CLUTCH},
    { SPELL_OZOCUBUS_REFRIGERATION, ZAP_REFRIGERATE},
    { SPELL_FOXFIRE,                ZAP_FOXFIRE },
};

zap_type spell_to_zap(spell_type spell)
{
    // This is to make sure that spl-cast.cc doesn't just zap dazzling
    // spray right away.
    if (spell == SPELL_BLINDING_SPRAY)
        return NUM_ZAPS;

    for (const auto &spzap : _spl_zaps)
        if (spzap.first == spell)
            return spzap.second;

    return NUM_ZAPS;
}

spell_type zap_to_spell(zap_type zap)
{
    for (const auto &spzap : _spl_zaps)
        if (spzap.second == zap)
            return spzap.first;

    return SPELL_NO_SPELL;
}

int spell_zap_power(spell_type spell, int pow)
{
    switch (spell)
    {
    case SPELL_MAGIC_CANDLE:
        return pow + 10;
    case SPELL_HIBERNATION:
        return stepdown_value(pow * 9 / 10, 5, 35, 45, 50);
    default:
        return pow;
    }
}

int spell_zap_power_cap(spell_type spell)
{
    const zap_type zap = spell_to_zap(spell);

    if (zap == NUM_ZAPS)
        return 0;

    const int cap = zap_power_cap(zap);

    switch (spell)
    {
    case SPELL_MAGIC_CANDLE:
        return max<int>(cap - 10, 0);
    case SPELL_HIBERNATION:
        return 50;
    default:
        return cap;
    }
}
