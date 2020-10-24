#pragma once

#include "enum.h"

enum mon_holy_type_flags
{
    MH_NONE              = 0,
    MH_HOLY              = 1<<0,
    MH_NATURAL           = 1<<1,
    MH_UNDEAD            = 1<<2,
    MH_DEMONIC           = 1<<3,
    MH_ELEMENTAL         = 1<<4, // Abyssal weirdness and elementals 
    MH_PLANT             = 1<<5,
    MH_EVIL              = 1<<6, // priests/wizards with evil spells
    MH_CONSTRUCT         = 1<<7, // golems and other constructs
};
DEF_BITFIELD(mon_holy_type, mon_holy_type_flags, 8);
