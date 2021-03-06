#pragma once

enum equipment_type
{
    EQ_NONE = -1,

    EQ_WEAPON0,
    EQ_FIRST_EQUIP = EQ_WEAPON0,
    EQ_WEAPON1,
    EQ_CLOAK,
    EQ_HELMET,
    EQ_GLOVES,
    EQ_BOOTS,
    EQ_BARDING,
    EQ_BODY_ARMOUR,
    EQ_FIRST_JEWELLERY,
    EQ_LEFT_RING = EQ_FIRST_JEWELLERY,
    EQ_RIGHT_RING,
    EQ_AMULET,
    // Octopodes don't have left and right rings. They have eight rings, instead.
    EQ_RING_ONE,
    EQ_RING_TWO,
    EQ_RING_THREE,
    EQ_RING_FOUR,
    EQ_RING_FIVE,
    EQ_RING_SIX,
    EQ_RING_SEVEN,
    EQ_RING_EIGHT,
    // Fairy specific slot.
    EQ_FAIRY_JEWEL,
    // Finger amulet provides an extra ring slot
    EQ_RING_AMULET,
    // Tendrils
    EQ_RING_LEFT_TENDRIL,
    EQ_RING_RIGHT_TENDRIL,
    EQ_LAST_JEWELLERY = EQ_RING_RIGHT_TENDRIL,
    EQ_CYTOPLASM, // Weird case.
    EQ_JIYVA0, // Morphing Slots
    EQ_JIYVA1,
    EQ_JIYVA2,
    EQ_JIYVA3,
    EQ_JIYVA4,
    EQ_JIYVA5,
    EQ_JIYVA6,
    NUM_EQUIP,

    EQ_FIRST_MORPH = EQ_JIYVA0,
    EQ_LAST_MORPH = EQ_JIYVA6,
    EQ_MIN_ARMOUR = EQ_CLOAK,
    EQ_MAX_ARMOUR = EQ_BODY_ARMOUR,
    EQ_MAX_WORN   = EQ_RING_AMULET,
    // these aren't actual equipment slots, they're categories for functions
    EQ_STAFF            = 100,         // weapon with base_type OBJ_STAVES
    EQ_RINGS,                          // check both rings
    EQ_RINGS_PLUS,                     // check both rings and sum plus
#if TAG_MAJOR_VERSION == 34
    EQ_RINGS_PLUS2,                    // check both rings and sum plus2
#endif
    EQ_ALL_ARMOUR,                     // check all armour types
    EQ_AMULET_PLUS,                    // check amulet for pluses.
};
