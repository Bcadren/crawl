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

{ SP_LIGNIFITE, MUT_BARKSKIN,
  "barkskin",

  {"Your bark is difficult to pierce. (rPiercing+)", "", ""},
  {"", "", ""},
  {"", "", ""},
},

{ SP_TENGU, MUT_HOOVES,
  "hippogriff form",

  {"You have a hippogriff-like bottom half; talons in the front, hooves in the back.", "", ""},
  {"", "", ""},
  {"", "", ""},
},

{ SP_FAIRY, MUT_BIG_WINGS,
  "flight",

  {"You are able to fly continuously.", "", ""},
  {"You regain your ability to fly.", "", ""},
  {"You can't fly anymore!", "", ""},
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

{ SP_GOBLIN, MUT_GODS_PITY,
  "god's pity",

  {"The pity of the gods increases the power of your invoked divine abilities.",
   "The pity of the gods increases the power of your passive and invoked divine abilities.", "" },
  {"The dieties of the dungeon take pity on you.",
   "The dieties of the dungeon take greater pity upon you.", "" },
  {"The dieties treat you like any other subject.",
   "The pity of the gods is waning.", "" },
},

{ SP_DRACONIAN, MUT_EXOSKELETON,
 "draconic flesh",
 
  {"Your scales resist being cut, but it's oddly easy to stick a small blade between them. (rSlashing+, rPiercing-)", "", ""},
  {"", "", ""},
  {"", "", ""},
},

{ SP_LIGNIFITE, MUT_HEAT_VULNERABILITY,
  "woody flesh (rF-)",

  {"Your woody flesh burns easily. (rF-)", 
   "Your woody flesh burns very easily. (rF--)",
   "Your dry woody flesh could combust at any time. (rF---)"},

  {"Your wood feels drier.",
   "Your wood feels arid.",
   "You feel like a matchstick!"},

  {"Your wood feels wetter. You feel less combustible.",
   "Your wood feels wetter. You feel less combustible.",
   "Your wood feels wetter. You feel less combustible."},
},

{ SP_LIGNIFITE, MUT_ROOTS,
  "roots",

  { "You can plant your roots to grant stasis and boost your AC and Regeneration. Uprooting yourself takes awhile, however.", "", "" },
  { "Your roots grow strong enough to plant themselves on the fly.", "", "" },
  { "Your root system weakens and can no longer be planted.", "", "" },
},

{ SP_LIGNIFITE, MUT_BRANCHES,
  "branches (SH +",

  { "A tangle of shorter branches protects your body from attack. (SH +", "", "" },
  { "Your branches feel lush and full again.", "", "" },
  { "Your branches thin out.", "", "" },
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

  {"You have sharp barbs on your forebranches. (+3 Unarmed)",
   "You have very sharp barbs on your forebranches. (+6 Unarmed)",
   "You have spiked branches. (+9 Unarmed)" },

  {"Barbs sprout from your front branches.",
   "Your barbs become longer and sharper.", 
   "Your forebranches twist and become covered in long spikes."},

  {"Your barbs shrink away completely.",
   "Your barbs become more like brambles."
   "Your spikes become smaller barbs."},
},

{ SP_OCTOPODE, MUT_CLAWS,
  "toothy tentacles",

  {"Your front tentacles are covered in pointy teeth. (+3 Unarmed)",
   "Your tentacles are covered in sharp spiky teeth. (+6 Unarmed)",
   "You have razor-sharp teeth on your tentacles. (+9 Unarmed)"},

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

  { "You are covered in icy blue bark. (rC+, AC +", "", "" },
  { "Your bark takes on an icy blue colour and grows thicker.", "", ""},
  { "Your icy blue bark withers and flakes away.", "", ""},
},

{ SP_LIGNIFITE, MUT_IRIDESCENT_SCALES, 
    "iridescent bark",

  { "You are covered in iridescent bark. (AC +", "", ""},
  { "Your bark takes on an iridescent colour and grows thicker.", "", ""},
  { "Your iridescent bark withers and flakes away.", "", ""},
},

{ SP_LIGNIFITE, MUT_LARGE_BONE_PLATES,
    "large wooden plates",

  { "Your branches are covered in large wooden plates. (SH +", "", "" },
  { "Large wooden plates grow over your branches.", "", "" },
  { "Your wooden plates disappear.", "", "" },
},

{ SP_LIGNIFITE, MUT_MOLTEN_SCALES,
    "molten bark",

  { "You are completely covered in molten bark. (rF+, AC+", "", "" },
  { "Your bark becomes tough, yet hot and malleable like molten rock.", "", ""},
  { "Your molten bark withers and flakes away.", "", ""},
},

{ SP_LIGNIFITE, MUT_RUGGED_BROWN_SCALES,
    "rough rugged bark",

  { "You are covered in rough rugged bark. (+7% HP, AC +" , "", ""},
  { "You are covered in gruff rugged bark.", "", "" },
  { "Your rough rugged bark withers and flakes away.", "", ""},
},

{ SP_LIGNIFITE, MUT_SLIMY_GREEN_SCALES, 
    "slimy green bark",

  { "You are completely covered in slimy green bark. (rPois, AC +", "", "" },
  { "Your bark takes on a slimy green colour and grows thicker.", "", ""},
  { "Your slimy green bark withers and flakes away.", "", ""},
},

{ SP_LIGNIFITE, MUT_THIN_METALLIC_SCALES, 
    "thin metallic chaff",

  { "You are completely covered in a thin metallic chaff. (rElec, AC +", "", ""},
  { "A thin metallic chaff grows over your bark.", "", "" },
  { "Your thin metallic chaff withers and flakes away.", "", ""},
},

{ SP_LIGNIFITE, MUT_THIN_SKELETAL_STRUCTURE,
    "soft wood",

  { "You're made of an unnaturally soft and light wood. (Stealth++, Dex +", "", "" },
  { "Your wood becomes less dense.", "", "" },
  { "Your wood redensifies.", "", ""},
},

{ SP_OCTOPODE, MUT_THIN_SKELETAL_STRUCTURE,
    "flexibility",

  { "You're an unnatural spineless contortionist. (Stealth++, Dex +", "", "" },
  { "You feel like you could squeeze through a hole one-hundredth your size.", "", "" },
  { "You feel stiff.", "", ""},
},

{ SP_LIGNIFITE, MUT_YELLOW_SCALES,
    "yellow bark",

  { "You are completely covered in yellow bark. (rCorr, AC +", "", ""},
  { "Your bark takes on a yellow colour and grows thicker.", "", ""},
  { "Your yellow bark withers and flakes away.", "", "" },
},

{ SP_LIGNIFITE, MUT_CRAGGY_SKIN,
    "craggy bark",

  { "Your rough stony bark repels attacks and injures those that touch it. (AC +", "", "" },
  { "Your bark hardens and calcifies into a rougher craggier texture.", "", "" },
  { "Your bark softens.", "", "" },
},

{ SP_LIGNIFITE, MUT_SHARP_SCALES,
    "thorny bark",

  { "You are covered in thorns. (AC +", "", ""},
  { "Your bark grows thicker and sprouts sharp thorns.", "", ""},
  { "Your thorns wither and flake away.", "", "" },
},

{ SP_LIGNIFITE, MUT_SANGUINE_ARMOUR,
  "sanguine armour",

  {"When seriously injured, your rosin creates armour. (AC +", "", ""},
  {"You feel your rosin ready itself to protect you.", "", ""},
  {"You feel your rosin become entirely quiescent.", "", ""},
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