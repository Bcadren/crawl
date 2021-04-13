#pragma once

struct species_mutation_message
{
    species_type   species;
    mutation_type  mutation;
    const char* short_desc; ///< What appears on the '%' screen.
    const char* have[3];    ///< What appears on the 'A' screen.
    const char* gain[3];    ///< Message when you gain the mutation.
    const char* lose[3];    ///< Message when you lose the mutation.
};

const int spmu_length = 25;

static const species_mutation_message spmu_data[] =
{

{ SP_HUMAN, MUT_NON_MUTATION,
    "null",

    { "","","" },
    { "","","" },
    { "","","" },
},

{ SP_FELID, MUT_COLD_RESISTANCE, 
    "fur coat (rC)",

    {"Your fur coat keeps you warm. (rC+)",
     "Your luxious mane keeps you nice and toasty. (rC++)", ""},
    {"Your fur is now thick enough to protect against the cold.",
     "Your fur grows even thicker and insulates better against the cold.", ""},
    {"Your fur thins out. You feel a chill.",
     "Your fur thins out. You feel a chill.", ""},
},

{ SP_FAIRY, MUT_MP_WANDS,
  "MP-powered wands",

  {"You expend magic power (1 MP) to strengthen your wands.", "", ""},
  {"You feel your magical essence link to the dungeon's wands.", "", ""},
  {"Your magical essence no longer links to wands of the dungeon.", "", ""},
},

{ SP_LIGNIFITE, MUT_UNBREATHING,
  "plant",

  {"You require very little carbon dioxide to survive.","",""},
  {"","",""},
  {"","",""},
},

{ SP_LIGNIFITE, MUT_CLAWS,
  "barbs",

  {"You have sharp barbs on your forebranches. (+2 Unarmed)",
   "You have very sharp barbs on your forebranches. (+4 Unarmed)",
   "You have spiked branches. (+6 Unarmed)" },

  {"Barbs sprout from your front branches.",
   "Your barbs become longer and sharper.", 
   "Your forebranches twist and become covered in long spikes."},

  {"Your barbs shrink away completely.",
   "Your barbs become more like brambles."
   "Your spikes become smaller barbs."},
},

{ SP_OCTOPODE, MUT_CLAWS,
  "toothy tentacles",

  {"Your front tentacles are covered in pointy teeth. (+2 Unarmed)",
   "Your tentacles are covered in sharp spiky teeth. (+4 Unarmed)",
   "You have razor-sharp teeth on your tentacles. (+6 Unarmed)"},

  {"Squid-like teeth form on your front tentacles.",
   "Your teeth become harder and sharper.",
   "Your forearms are completely covered in teeth."},

  {"Your teeth soften and return to being fleshy.",
   "Your teeth shrink."
   "Your sharp teeth become softer and duller."},
},

{ SP_LIGNIFITE, MUT_PASSIVE_FREEZE,
  "passive freeze",

  { "A frigid envelope surrounds you and freezes all who hurt you.", "", "" },
  { "Your bark feels very cold.", "", "" },
  { "", "", "" },
},

{ SP_LIGNIFITE, MUT_SPINY,
  "thorny",

  { "You are partially covered in thorns.",
    "You are mostly covered in sharp thorns.",
    "You are completely covered in razor-sharp thorns." },

  { "You sprout thorny brambles.",
    "More thorny brambles sprout from your body.",
    "Sharp briars grow from your entire body." },

  { "Your thorny briars wither and fall away.",
    "You lose some of your briars.",
    "Your brambles lose some of their sharpness." },
},

{ SP_LIGNIFITE, MUT_TRANSLUCENT_SKIN,
    "translucent bark",

  { "Your translucent bark slightly reduces your foes' accuracy. (Stealth)",
    "Your translucent bark reduces your foes' accuracy. (Stealth)",
    "Your transparent bark significantly reduces your foes' accuracy. (Stealth)" },

  { "Your bark becomes partially translucent.",
    "Your bark becomes more translucent.",
    "Your bark becomes completely transparent." },

  { "Your bark returns to its normal opacity.",
    "Your bark's translucency fades.",
    "Your bark's transparency fades." },
},

{ SP_LIGNIFITE, MUT_ACIDIC_BITE,
    "acidic bite",

    { "You have acidic rosin.", "", "" },
    { "A layer of acidic rosin forms in your mouth.", "", "" },
    { "Your rosin feels more basic.", "", "" },
},

{ SP_LIGNIFITE, MUT_ICY_BLUE_SCALES,
    "icy blue bark",

  { "You are partially covered in icy blue bark. (AC +2)",
    "You are mostly covered in icy blue bark. (AC +3)",
    "You are completely covered in icy blue bark. (AC +4, rC+)" },

  { "Parts of your bark take on an icy blue colour and grow thicker.",
    "More of your bark takes on an icy blue colour.",
    "You are completely covered in icy blue bark." },

  { "Your icy blue bark withers and falls away completely.",
    "Parts of your icy blue bark flake away.",
    "Parts of your icy blue bark flake away." },
},

{ SP_LIGNIFITE, MUT_IRIDESCENT_SCALES, 
    "iridescent bark",

  { "You are partially covered in iridescent bark. (AC +2)",
    "You are mostly covered in iridescent bark. (AC +4)",
    "You are completely covered in iridescent bark. (AC +6)" },

  { "Parts of your bark take on an iridescent colour and grow thicker.",
    "More of your bark takes on an iridescent colour.",
    "You are completely covered in iridescent bark." },

  { "Your iridescent bark withers and falls away completely.",
    "Parts of your iridescent bark flake away.",
    "Parts of your iridescent bark flake away." },
},

{ SP_LIGNIFITE, MUT_LARGE_BONE_PLATES,
    "large wooden plates",

  { "You are partially covered in large wooden plates. (SH +4)",
    "You are mostly covered in large wooden plates. (SH +6)",
    "You are completely covered in large wooden plates. (SH +8)" },

  { "Large wooden plates grow over parts of your branches.",
    "Large wooden plates spread over more of your branches.",
    "Large wooden plates cover your branches completely." },

  { "Your wooden bone plates disappear.",
    "Your wooden bone plates recede somewhat.",
    "Your wooden bone plates recede somewhat." },
},

{ SP_LIGNIFITE, MUT_MOLTEN_SCALES,
    "molten bark",

  { "You are partially covered in molten bark. (AC +2)",
    "You are mostly covered in molten bark. (AC +3)",
    "You are completely covered in molten bark. (AC +4, rF+)" },

  { "Parts of your bark become tough, yet hot and malleable like molten rock.",
    "More of your bark turns into a lava-like coating.",
    "You are completely covered in malleable molten stone." },

  { "Your molten bark withers and falls away completely.",
    "Parts of your molten bark flake away.",
    "Parts of your molten bark flake away." },
},

{ SP_LIGNIFITE, MUT_RUGGED_BROWN_SCALES,
    "rough rugged bark",

  { "You are partially covered in rough rugged bark. (AC +1, +3% HP)",
     "You are mostly covered in rough rugged bark. (AC +2, +5% HP)",
     "You are completely covered in rough rugged bark. (AC +3, +7% HP)" },

  { "Parts of your bark become rougher and more rugged.",
    "More of your bark becomes traditionally manly.",
    "You are completely covered in gruff rugged bark." },

  { "Your rough bark withers and falls away completely.",
    "Parts of your rough rugged bark flake away.",
    "Parts of your rough rugged bark flake away." },
},

{ SP_LIGNIFITE, MUT_SLIMY_GREEN_SCALES, 
    "slimy green bark",

  { "You are partially covered in slimy green bark. (AC +2)",
    "You are mostly covered in slimy green bark. (AC +3)",
    "You are completely covered in slimy green bark. (AC +4, rPois)" },

  { "Parts of your bark take on a slimy green colour and grow thicker.",
    "More of your bark takes on a slimy green colour.",
    "You are completely covered in slimy green bark." },

  { "Your slimy green bark withers and falls away completely.",
    "Parts of your slimy green bark flake away.",
    "Parts of your slimy green bark flake away." },
},

{ SP_LIGNIFITE, MUT_THIN_METALLIC_SCALES, 
    "thin metallic chaff",

  { "You are partially covered in a thin metallic chaff. (AC +2)",
    "You are mostly covered in a thin metallic chaff. (AC +3)",
    "You are completely covered in a thin metallic chaff. (AC +4, rElec)" },

  { "A thin metallic chaff grows over parts of your bark.",
    "The thin metallic chaff spreads over more of your bark.",
    "The thin metallic chaff covers your body completely." },

  { "Your thin metallic chaff withers and falls away completely.",
    "Parts of your thin metallic chaff flake away.",
    "Parts of your thin metallic chaff flake away." },
},

{ SP_LIGNIFITE, MUT_THIN_SKELETAL_STRUCTURE,
    "soft wood",

  { "You're made of somewhat softer, lighter wood. (Dex +2, Stealth)",
    "You're made of softwood. (Dex +4, Stealth+)",
    "You're made of an unnaturally soft and light wood. (Dex +6, Stealth++)" },

  { "Your wood becomes slightly less dense.",
    "Your wood becomes somewhat less dense.",
    "Your wood becomes less dense." },

  { "Your wood returns to normal.",
    "Your wood densifies.",
    "Your wood densifies." },
},

{ SP_OCTOPODE, MUT_THIN_SKELETAL_STRUCTURE,
    "flexibility",

  { "You're somewhat more flexible than a normal octopode. (Dex +2, Stealth)",
    "You're much more flexible than a normal octopode. (Dex +4, Stealth+)",
    "You're an unnatural spineless contortionist. (Dex +6, Stealth++)" },

  { "You feel oddly flexible.",
    "You feel like you could squeeze through a hole one-hundredth your size.",
    "You feel like you could tie yourself in a knot and easily get back loose." },

  { "You feel stiff.",
    "You feel less flexible.",
    "You feel less flexible." },
},

{ SP_LIGNIFITE, MUT_YELLOW_SCALES,
    "yellow bark",

  { "You are partially covered in yellow bark. (AC +2)",
    "You are mostly covered in yellow bark. (AC +3)",
    "You are completely covered in yellow bark. (AC +4, rCorr)" },

  { "Parts of your bark take on a yellow colour and grow thicker.",
    "More of your bark takes on a yellow colour.",
    "You are completely covered in yellow bark." },

  { "Your yellow bark withers and falls away completely.",
    "Parts of your yellow bark flake away.",
    "Parts of your yellow bark flake away." },
},

{ SP_LIGNIFITE, MUT_SANGUINE_ARMOUR,
  "sanguine armour",

  {"When seriously injured, your rosin creates armour. (AC +",
   "When seriously injured, your rosin creates thick armour. (AC +",
   "When seriously injured, your rosin creates very thick armour. (AC +"},

  {"You feel your rosin ready itself to protect you.",
   "You feel your rosin thicken.",
   "You feel your rosin thicken."},

  {"You feel your rosin become entirely quiescent.",
   "You feel your rosin thin.",
   "You feel your rosin thin."},
},

{ SP_LIGNIFITE, MUT_IGNITE_BLOOD,
  "ignite blood",

  {"Your demonic aura causes spilled blood to erupt in flames.", "", ""},
  {"You feel an intense hatred of all warm-blooded creatures!", "", ""},
  {"", "", ""},
},

{ SP_LIGNIFITE, MUT_NO_DRINK, 
  "inability to use potions while threatened",

  {"Your roots cannot absorb potions while threatened.", "", ""},
  {"Your roots no longer can absorb potions while threatened.", "", ""},
  {"Your roots can once more absorb potions while threatened.", "", ""},
},

{ SP_LIGNIFITE, MUT_MISSING_HAND, 
  "missing a core branch",

  {"You are missing a core branch.", "", ""},
  {"One of your branches has vanished, leaving only a stump!", "", ""},
  {"Your stump has regrown into a branch!", "", ""},
},

{ SP_FELID, MUT_MISSING_HAND, 
  "missing a paw",

  {"You are missing a paw.", "", ""},
  {"One of your paws has vanished, leaving only a stump!", "", ""},
  {"Your stump has regrown into a paw!", "", ""},
},


{ SP_OCTOPODE, MUT_MISSING_HAND, 
  "missing a tentacle",

  {"You are missing one of your front tentacles.", "", ""},
  {"One of your tentacles has vanished, leaving only a stump!", "", ""},
  {"Your stump has regrown into a tentacle!", "", ""},
},

{ SP_FORMICID, MUT_MISSING_HAND, 
  "missing a pair of hands",

  {"You are missing a pair of hands.", "", ""},
  {"One of your pairs of hands has vanished, leaving only stumps!", "", ""},
  {"Your stumps have regrown into a hands!", "", ""},
},

};