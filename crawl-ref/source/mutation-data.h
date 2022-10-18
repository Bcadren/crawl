struct mutation_def
{
    mutation_type mutation;
    short       weight;     ///< Commonality of the mutation; bigger = appears
                            /// more often.
    short       levels;     ///< The number of levels of the mutation.
    mutflags    uses;       ///< Bitfield holding types of effects that grant
                            /// this mutation (mutflag::*)
    bool        form_based; ///< Mutation is suppressed when shapechanged.
    const char* short_desc; ///< What appears on the '%' screen.
    const char* have[3];    ///< What appears on the 'A' screen.
    const char* gain[3];    ///< Message when you gain the mutation.
    const char* lose[3];    ///< Message when you lose the mutation.
};

struct mutation_category_def
{
  mutation_type mutation;
  const char* short_desc;
};

static const mutation_def mut_data[] =
{

// Messaging handled elsewhere.
{ MUT_STATS, 10, 255, mutflag::good | mutflag::bad, false,
  "stats",

  { "", "", "" },
  { "", "", "" },
  { "", "", "" },
},

{ MUT_SUPPRESSION, 45, 255, mutflag::bad, false,
  "suppression",

  { "", "", "" },
  { "", "", "" },
  { "", "", "" },
},

{ MUT_HALF_DEATH, 0, 2, mutflag::good | mutflag::jiyva, false,
  "between life and death",

  {"", "", ""},
  {"", "You begin to exude an aura of decay.", ""},
  {"", "Your aura receeds.", ""},
},

{ MUT_POISON_RESISTANCE, 12, 1, mutflag::good, true,
  "poison resistance",

  {"Your system is resistant to poisons. (rPois)", "", ""},
  {"You feel resistant to poisons.", "",  ""},
  {"You feel less resistant to poisons.", "", ""},
},

{ MUT_CARNIVOROUS, 3, 1, mutflag::bad, false,
  "carnivore",

  {"You are carnivorous and can eat meat at any time.", "", ""},
  {"You hunger for flesh.", "", ""},
  {"You feel able to eat a more balanced diet.", "", ""},
},

{ MUT_HERBIVOROUS, 3, 1, mutflag::bad, false,
  "herbivore",

  {"You are a herbivore.", "", ""},
  {"You hunger for vegetation.", "", ""},
  {"You feel able to eat a more balanced diet."},
},

{ MUT_HEAT_RESISTANCE, 12, 3, mutflag::good, true,
  "fire resistance",

  {"You are heat resistant. (rF+)",
   "You are very heat resistant. (rF++)",
   "You are almost immune to the effects of heat. (rF+++)"},

  {"You feel resistant to heat.",
   "You feel more resistant to heat.",
   "You feel more resistant to heat."},

  {"You no longer feel heat resistant.",
   "You feel less heat resistant.",
   "You feel less heat resistant."},
},

{ MUT_COLD_RESISTANCE, 12, 3, mutflag::good, true,
  "cold resistance",

  {"You are cold resistant. (rC+)",
   "You are very cold resistant. (rC++)",
   "You are almost immune to the effects of cold. (rC+++)"},

  {"You feel resistant to cold.",
   "You feel more resistant to cold.",
   "You feel more resistant to cold."},

  {"You no longer feel cold resistant.",
   "You feel less cold resistant.",
   "You feel less cold resistant."},
},

{ MUT_POISON_SUSCEPTIBILITY, 12, 3, mutflag::bad, true,
  "poison susceptibility",

  {"Your flesh is particularly susceptible to poisons. (rPois-)", "", ""},
  {"You feel more susceptible to poisons.", "", ""},
  {"You feel strangely mythridized.", "", ""},
},

{ MUT_HEAT_VULNERABILITY, 12, 3,
  mutflag::bad | mutflag::qazlal, true,
  "heat vulnerability",

  {"You are vulnerable to heat. (rF-)",
   "You are very vulnerable to heat. (rF--)",
   "You are extremely vulnerable to heat. (rF---)"},

  {"You feel vulnerable to heat.",
   "You feel vulnerable to heat.",
   "You feel vulnerable to heat."},

  {"You no longer feel vulnerable to heat.",
   "You feel less vulnerable to heat.",
   "You feel less vulnerable to heat."},
},

{ MUT_COLD_VULNERABILITY, 12, 3,
  mutflag::bad | mutflag::qazlal, true,

  "cold vulnerability",

  {"You are vulnerable to cold. (rC-)",
   "You are very vulnerable to cold. (rC--)",
   "You are extremely vulnerable to cold. (rC---)"},

  {"You feel vulnerable to cold.",
   "You feel vulnerable to cold.",
   "You feel vulnerable to cold."},

  {"You no longer feel vulnerable to cold.",
   "You feel less vulnerable to cold.",
   "You feel less vulnerable to cold."},
},

{ MUT_DEMONIC_GUARDIAN, 0, 3, mutflag::good, false,
  "demonic guardian",

  {"A weak demonic guardian rushes to your aid.",
   "A demonic guardian rushes to your aid.",
   "A powerful demonic guardian rushes to your aid."},

  {"You feel the presence of a demonic guardian.",
   "Your guardian grows in power.",
   "Your guardian grows in power."},

  {"Your demonic guardian is gone.",
   "Your demonic guardian is weakened.",
   "Your demonic guardian is weakened."},
},

{ MUT_SHOCK_RESISTANCE, 7, 1, mutflag::good, true,
  "electricity resistance",

  {"You are resistant to electric shocks. (rElec)", "", ""},
  {"You feel insulated.", "", ""},
  {"You feel conductive.", "", ""},
},

{ MUT_SHOCK_VULNERABILITY, 7, 1, mutflag::bad | mutflag::qazlal, true,
  "electricity vulnerability",

  {"You are vulnerable to electric shocks.", "", ""},
  {"You feel more conductive.", "", ""},
  {"You feel more insulated.", "", ""},
},

{ MUT_REGENERATION, 15, 2, mutflag::good, false,
  "regeneration",

  {"You heal very quickly.",
   "You regenerate.", ""},

  {"You begin to heal more quickly.",
   "You begin to regenerate.", ""},

  {"Your rate of healing slows.",
   "Your rate of healing slows.", ""},
},

{ MUT_INHIBITED_REGENERATION, 15, 1, mutflag::bad, false,
  "inhibited regeneration",

  {"You do not regenerate when monsters are visible.", "", ""},
  {"Your regeneration stops near monsters.", "", ""},
  {"You begin to regenerate regardless of the presence of monsters.", "", ""},
},

{ MUT_ROTTING_BODY, 0, 1, mutflag::bad, true,
  "rotting body",

  {"Your body is slowly rotting away. You can heal your form by consuming rotten meat.", "", ""},
  {"", "", ""},
  {"", "", ""},
},

{ MUT_GOBLINS_GREED, 0, 1, mutflag::good, false,
  "goblin's greed",

  {"Your goblin ancestry allows you to find more gold. (+20 %)", "", ""},
  {"You feel a greedy glee.", "", ""},
  {"You feel strangely charitable.", "", ""},
},

{ MUT_FAST_METABOLISM, 3, 3, mutflag::bad, false,
  "fast metabolism",

  {"You have a fast metabolism.",
   "You have a very fast metabolism.",
   "Your metabolism is lightning-fast."},

  {"You feel a little hungry.",
   "You feel a little hungry.",
   "You feel a little hungry."},

  {"Your metabolism slows.",
   "Your metabolism slows.",
   "Your metabolism slows."},
},

{ MUT_SLOW_METABOLISM, 3, 2, mutflag::good, false,
  "slow metabolism",

  {"You have a slow metabolism.",
   "You need to consume almost no food.", ""},

  {"Your metabolism slows.",
   "Your metabolism slows.", ""},

  {"You feel a little hungry.",
   "You feel a little hungry.", ""},
},

{ MUT_MERFOLK_TAIL, 0, 1, mutflag::good, true,
  "merfolk tail",

  { "On contact with water, your feet merge into a fishtail, which allows you to swim quickly, evasively and stealthily.", "", "" },
  { "", "", "" },
  { "", "", "" },
},

{ MUT_TELEPORT, 15, 1, mutflag::bad, false,
  "teleportitis",

  {"You are often teleported next to monsters.", "", ""},
  {"You feel weirdly uncertain.", "", ""},
  {"You feel stable.", "", ""},
},

{ MUT_MAGIC_RESISTANCE, 17, 1, mutflag::good, false,
  "magic resistance",

  {"You are highly resistant to hostile enchantments. (MR++)", "", ""},
  {"You feel more resistant to hostile enchantments.", "", ""},
  {"You feel vulnerable to hostile enchantments.", "", ""},
},

{ MUT_FAST, 7, 3, mutflag::good, true,
  "speed",

  {"You cover ground quickly.",
   "You cover ground very quickly.",
   "You cover ground extremely quickly."},

  {"You feel quick.",
   "You feel quick.",
   "You feel quick."},

  {"You feel sluggish.",
   "You feel sluggish.",
   "You feel sluggish."},
},

{ MUT_SLOW, 7, 3, mutflag::bad, true,
  "slowness",

  {"You cover ground slowly.",
   "You cover ground very slowly.",
   "You cover ground extremely slowly."},

  {"You feel sluggish.",
   "You feel sluggish.",
   "You feel sluggish."},

  {"You feel quick.",
   "You feel quick.",
   "You feel quick."},
},

{ MUT_ACUTE_VISION, 7, 1, mutflag::good, false,
  "see invisible",

  {"You have supernaturally acute eyesight. (+Vis)", "", ""},
  {"Your vision sharpens.", "", ""},
  {"Your vision seems duller.", "", ""},
},

{ MUT_DEFORMED, 31, 2, mutflag::bad | mutflag::xom, true,
  "deformed body",

  {"Armour fits poorly on your strangely shaped body.", 
   "You cannot wear any armour on your peculiar form.", ""},

  {"Your body twists and deforms.",
   "Your body twists and deforms.", ""},

  {"Your body's shape seems more normal.", 
   "Your body's shape seems more normal.", ""},
},

{ MUT_SPIT_POISON, 10, 2, mutflag::good, false,
  "spit poison",

  {"You can spit venom.",
   "You can exhale a cloud of poison.", ""},

  {"A venom gland forms on the roof of your mouth.",
   "Your venom gland gets more potent.", ""},

  {"Your venom gland atrophies and becomes useless.",
   "Your venom becomes weaker.", ""},
},

{ MUT_BREATHE_MAGMA, 0, 2, mutflag::good, false,
  "breathe magma",

  {"You can breathe lava.",
   "You can breathe bursts of magma.", ""},

  {"Your throat feels hot.",
   "Your throat feels hot.", ""},

  {"A chill runs up and down your throat.",
   "A chill runs up and down your throat.", ""},
},

{ MUT_BLINK, 15, 1, mutflag::good, false,
  "blink",

  {"You can translocate small distances at will.", "", ""},
  {"You feel jittery.", "", ""},
  {"You no longer feel jittery.", "", ""},
},

{ MUT_SHOUTITUS, 8, 1, mutflag::bad | mutflag::xom, false,
  "shoutitus",

  {"You frequently shout uncontrollably at your foes.", "", ""},
  {"You feel a strong urge to shout.", "", ""},
  {"Your urge to shout disappears.", "", ""},
},

{ MUT_CLARITY, 15, 1, mutflag::good, false,
  "clarity",

  {"You possess an exceptional clarity of mind.", "", ""},
  {"Your thoughts seem clearer.", "", ""},
  {"Your thinking seems confused.", "", ""},
},

{ MUT_BERSERK, 7, 1, mutflag::bad, false,
  "berserkitis",

  {"You have an uncontrollable temper.", "", ""},
  {"You feel a little pissed off.", "", ""},
  {"You feel calm.", "", ""},
},

{ MUT_DETERIORATION, 31, 1, mutflag::bad | mutflag::xom, false,
  "deterioration",

  {"Your body often falls apart upon taking damage.", "", ""},
  {"You feel your body start to fall apart.", "", ""},
  {"Your body feels more substantial.", "", ""},
},

{ MUT_IMPAIRED_VISION, 7, 1, mutflag::bad | mutflag::xom, false,
  "impaired vision",

  {"Your poor vision makes scrolls take longer to read and aiming more difficult.", "", ""},
  {"Your vision dulls.", "", ""},
  {"Your vision sharpens.", "", ""},
},

{ MUT_MUTATION_RESISTANCE, 6, 3, mutflag::good, false,
  "mutation resistance",

  {"You are somewhat resistant to further mutation.",
   "You are somewhat resistant to both further mutation and mutation removal.",
   "You are almost entirely immune to further mutation and mutation removal."},

  {"You feel genetically stable.",
   "You feel genetically stable.",
   "You feel genetically immutable."},

  {"You feel genetically unstable.",
   "You feel genetically unstable.",
   "You feel genetically unstable."},
},

{ MUT_EVOLUTION, 30, 2, mutflag::good, false,
  "evolution",

  {"You evolve.",
   "You rapidly evolve.", ""},

  {"You feel nature experimenting on you. Don't worry, failures die fast.",
   "Your genes go into a fast flux.", ""},

  {"Nature stops experimenting on you.",
   "Your wild genetic ride slows down.", ""},
},

{ MUT_FRAIL, 15, 3, mutflag::bad | mutflag::xom, false,
  "frail",

  {"You are frail. (-10% HP)",
   "You are very frail. (-20% HP)",
   "You are extremely frail. (-30% HP)"},

  {"You feel frail.",
   "You feel frail.",
   "You feel frail."},

  {"You feel robust.",
   "You feel robust.",
   "You feel robust."},
},

{ MUT_ROBUST, 15, 3, mutflag::good, false,
  "robust",

  {"You are robust. (+10% HP)",
   "You are very robust. (+20% HP)",
   "You are extremely robust. (+30% HP)"},

  {"You feel robust.",
   "You feel robust.",
   "You feel robust."},

  {"You feel frail.",
   "You feel frail.",
   "You feel frail."},
},

{ MUT_UNBREATHING, 0, 1, mutflag::good, true,
  "unbreathing",

  {"You can survive without breathing.", "", ""},
  {"You feel breathless.", "", ""},
  {"You gasp for air.", "", ""},
},

{ MUT_TORMENT_RESISTANCE, 0, 1, mutflag::good, false,
  "torment resistance",

  {"You are immune to unholy pain and torment.", "", ""},
  {"You feel a strange anaesthesia.", "", ""},
  {"You feel a sharp pain in your heart.", "", ""},
},

{ MUT_NEGATIVE_ENERGY_RESISTANCE, 8, 3, mutflag::good, false,
  "negative energy resistance",

  {"You resist negative energy. (rN+)",
   "You are quite resistant to negative energy. (rN++)",
   "You are immune to negative energy. (rN+++)"},

  {"You feel resistant to negative energy.",
   "You feel more resistant to negative energy.",
   "You feel immune to negative energy."},

  {"You no longer feel resistant to negative energy.", 
   "You feel less resistant to negative energy.", 
   "You are no longer immune to negative energy."},
},

{ MUT_NEGATIVE_ENERGY_VULNERABILITY, 8, 3, mutflag::bad, false,
  "negative energy vulnerability",

  {"You are vulnerable to negative energy. (rN-)",
   "You are very vulnerable to negative energy. (rN--)",
   "You are extremely vulnerable to negative energy. (rN---)"},

  {"You feel vulnerable to negative energy.",
   "You feel vulnerable to negative energy.",
   "You feel vulnerable to negative energy."},

  {"You no longer feel vulnerable to negative energy.",
   "You feel less vulnerable to negative energy.",
   "You feel less vulnerable to negative energy."},
},

{ MUT_ACID_VULNERABILITY, 7, 1, mutflag::bad, true,
  "acid vulnerability",

  {"You are vulnerable to acid. (rCorr-)", "", ""},
  {"You feel vulnerable to acid.", "", ""},
  {"You no longer feel vulnerable to acid.", "", ""},
},

{ MUT_NECRO_ENHANCER, 0, 2, mutflag::good, false,
  "in touch with death",

  {"You are in touch with the powers of death.",
   "You are strongly in touch with the powers of death.", ""},

  {"You feel more in touch with the powers of death.",
   "You feel more in touch with the powers of death.", ""},

  {"You feel out of touch with the powers of death.", 
   "You feel less in touch with the powers of death.", ""},
},

{ MUT_TENGU_FLIGHT, 0, 1, mutflag::good, true,
  "tengu flight",

  {"You can fly continuously. Your speed and evasion are boosted while flying.", "", ""},
  {"You have gained the ability to fly.", "", ""},
  {"Feathers fall from your wings; you can't fly anymore!", "", ""},
},

{
  MUT_FAIRY_LIGHT, 30, 2, mutflag::good, false,
  "fairy light",

  {"You exude a bright light in a small radius around yourself.", 
   "You exude a bright light in a large radius around yourself.", ""},

  {"Your natural light returns.", "Your natural light brightens.", ""},

  {"Your natural light goes out.", "Your natural light dims.", ""},
},

{
  MUT_DAYSTRIDER, 12, 1, mutflag::bad, false,
  "daystrider",

  {"You can see and be seen from farther away.", "", ""},
  {"You feel more exposed.", "", ""},
  {"You return to the shadows of the dungeon.", "", ""},
},

{ MUT_HURL_HELLFIRE, 0, 1, mutflag::good, false,
  "hellfire blast",

  {"You can hurl blasts of hellfire.", "", ""},
  {"You smell a hint of brimstone.", "", ""},
  {"You can no longer hurl hellfire.", "", ""},
},

// body-slot facets
{ MUT_HORNS, 8, 2, mutflag::good, true,
  "horns",

  {"You have a pair of horns on your head.",
   "You reflexively attack those that strike you in melee with your horns.", ""},

  {"A pair of horns grows on your head!", 
   "Your feel an urge to charge someone with your horns.", ""},

  {"The horns on your head fall off.", 
   "You feel less irritable.", ""},
},

{ MUT_BEAK, 8, 1, mutflag::good, true,
  "beak",

  {"You have a beak for a mouth.", "", ""},
  {"Your mouth lengthens and hardens into a beak!", "", ""},
  {"Your beak shortens and softens into a mouth.", "", ""},
},

{ MUT_CLAWS, 10, 3, mutflag::good, true,
  "claws",

  {"You have sharp fingernails. (+3 Unarmed)",
   "You have claws for hands. (+6 Unarmed)",
   "You have huge, feral claws. (+9 Unarmed)"},

  {"Your fingernails lengthen.",
   "Your hands twist into claws.",
   "Your claws increase in size and ferocity."},

  {"Your fingernails shrink to normal size.",
   "Your hands feel fleshier.",
   "Your claws shrink."},
},

{ MUT_FANGS, 7, 1, mutflag::good, true,
  "fangs",

  {"You have razor-sharp fangs.", "", ""},
  {"Your teeth extend into long, razor-sharp fangs."},
  {"Your teeth shrink to normal size.", "", ""},
},

{ MUT_HOOVES, 7, 1, mutflag::good, true,
  "hooves",

  {"You have hooves in place of feet.", "", ""},
  {"Your feet thicken and reform into hooves.", "", ""},
  {"Your hooves expand and flesh out into feet!", "", ""},
},

{ MUT_ANTENNAE, 9, 1, mutflag::good, true,
  "antennae",

  {"You have a pair of antennae on your head. (+Vis)", "", ""},
  {"A pair of antennae grows on your head!", "", ""},
  {"The antennae on your head shrink away.", "", ""},
},

{ MUT_TALONS, 8, 1, mutflag::good, true,
  "talons",

  {"You have razor-sharp talons for feet.", "", ""},
  {"Your feet stretch into talons.", "", ""},
  {"Your talons dull and shrink into feet.", "", ""},
},

// Versions of roots and branches here is for randomly getting it as a Vine Stalker. 
// Lignifite version is in species-mutation-messaging.h
{ MUT_ROOTS, 0, 1, mutflag::good, true,
  "roots",

  { "You can plant your roots to grant stasis and boost your AC and Regeneration. Uprooting yourself takes awhile, however.", "", "" },
  { "Your symbiote develops a strong root system that you can plant on the fly.", "", "" },
  { "Your symbiote's root system withers and falls away.", "", "" },
},

{ MUT_BRANCHES, 0, 1, mutflag::good, true,
  "branches (SH +",

  { "A tangle of woody branches protects your body from attack. (SH +", "", "" },
  { "Your symbiote develops woody branches upon its vines, shielding you from attack.", "", "" },
  { "Your symbiote's branches wither and fall away.", "", "" },
},

{ MUT_SILENCE_AURA, 30, 3, mutflag::good, false,
  "silence aura",

  { "You are surrounded by a small aura of unnatural quiet.", 
    "You are surrounded by an aura of unnatural quiet.", 
    "You are surrounded by a massive aura of unnatural quiet."},

  { "You develop an aura of silence.",
    "Your aura of unnatural quiet expands.",
    "Your aura of unnatural quiet expands." },

  { "The world fills with sound again. Your silence aura fades.", 
    "Your aura of unnatural quiet shrinks.",
    "Your aura of unnatural quiet shrinks."},
},

// Octopode only
{ MUT_TENTACLE_SPIKE, 30, 1, mutflag::good, true,
  "tentacle spike",

  {"One of your tentacles bears a large vicious spike.", "", ""},
  {"One of your lower tentacles grows a large vicious spike.", "", ""},
  {"Your tentacle spike disappears.", "", ""},
},

{ MUT_CONSTRICTING_TAIL, 0, 1, mutflag::good, true,
  "constrict 1",

  {"You can use your snake-like lower body to constrict enemies.", "", ""},
  {"Your tail grows strong enough to constrict your enemies.", "", ""},
  {"Your tail weakens and can no longer constrict enemies.", "", ""},
},

// Naga and Draconian only
{ MUT_STINGER, 30, 1, mutflag::good, true,
  "stinger",

  {"Your tail ends in a sharp, venomous barb.", "", ""},
  {"A venomous barb forms on the end of your tail.", "", ""},
  {"The barb on your tail recedes.", "", ""},
},

// Draconian only
{ MUT_BIG_WINGS, 15, 1, mutflag::good, true,
  "big wings",

  {"Your large and strong wings let you fly indefinitely.", "", ""},
  {"Your wings grow larger and stronger.", "", ""},
  {"Your wings shrivel and weaken.", "", ""},
},

// species-dependent innate mutations
{ MUT_ROT_IMMUNITY, 0, 1, mutflag::good, false,
  "rot immunity",

  {"You are immune to rotting.", "", ""},
  {"You feel immune to rotting.", "", ""},
  {"You feel vulnerable to rotting.", "", ""},
},

{ MUT_GOURMAND, 0, 1, mutflag::good, false,
  "gourmand",

  {"You like to eat raw meat.", "", ""},
  {"You feel like savouring the dungeon's cuisine.", "", ""},
  {"You feel repulsed by the smell of raw flesh.", "", ""},
},

{ MUT_FROG_LEGS, 7, 2, mutflag::good, true,
  "frog legs",

  {"Your frog legs can swim and hop short distances.",
   "Your frog legs can swim and hop long distances.", ""},

  {"Your toes lengthen and a thick webbing forms between them. You feel like hopping.", "Your legs feel stronger.", ""},
  {"The webbing on your feet shrivels and dries.", "Your legs feel weaker.", ""},
},

{ MUT_HIGH_MAGIC, 15, 2, mutflag::good, false,
  "high mp",

  {"You have an increased reservoir of magic. (+15% MP)",
   "You have a greatly increased reservoir of magic. (+30% MP)", ""},

  {"You feel more energetic.",
   "You feel more energetic.", ""},

  {"You feel less energetic.",
   "You feel less energetic.", ""},
},

{ MUT_LOW_MAGIC, 15, 2, mutflag::bad, false,
  "low mp",

  {"Your magical capacity is low. (-15% MP)",
   "Your magical capacity is very low. (-30% MP)", ""},

  {"You feel less energetic.",
   "You feel less energetic.", ""},

  {"You feel more energetic.",
   "You feel more energetic.", ""},
},

{ MUT_WILD_MAGIC, 8, 1, mutflag::good, false,
  "wild magic",

  {"Your spells are harder to cast, but more powerful.", "", ""},
  {"You feel your magical power running wild!", "", ""},
  {"You regain control of your magic.", "", ""},
},

{ MUT_SUBDUED_MAGIC, 8, 1, mutflag::bad, false,
  "subdued magic",

  {"Your spells are easier to cast, but less powerful.", "", ""},
  {"Your connection to magic feels dormant.", "", ""},
  {"Your magic regains its normal vibrancy.", "", ""},
},

{ MUT_FORLORN, 8, 1, mutflag::bad, false,
  "forlorn",

  {"You have difficulty communicating with the divine.","",""},
  {"You feel forlorn.","",""},
  {"You feel more spiritual.","",""},
},

{ MUT_STOCHASTIC_TORMENT_RESISTANCE, 0, 1, mutflag::good, false,
  "50% torment resistance",

  {"You are somewhat able to resist unholy torments (1 in 2 success).","",""},
  {"You feel a strange anaesthesia.", "", ""},
  {"You feel a strange intensity.", "", ""},
},

{ MUT_PASSIVE_MAPPING, 3, 1, mutflag::good, false,
  "sense surroundings",

  {"You passively map the area around you.", "", ""},
  {"You feel a strange attunement to the structure of the dungeons.", "", ""},
  {"You feel disoriented.", "", ""},
},

{ MUT_ICEMAIL, 0, 1, mutflag::good, false,
  "icemail",

  {"A meltable icy envelope protects you from harm. (AC +", "", ""},
  {"An icy envelope takes form around you.", "", ""},
  {"", "", ""},
},

{ MUT_PASSIVE_FREEZE, 0, 1, mutflag::good, false,
  "passive freeze",

  {"A frigid envelope surrounds you and freezes all who hurt you.", "", ""},
  {"Your skin feels very cold.", "", ""},
  {"", "", ""},
},

{ MUT_NIGHTSTALKER, 12, 3, mutflag::good, false,
  "nightstalker",

  {"You are slightly more attuned to the shadows.",
   "You are significantly more attuned to the shadows.",
   "You are completely attuned to the shadows."},

  {"You slip into the darkness of the dungeon.",
   "You slip further into the darkness.",
   "You are surrounded by darkness."},

  {"Your affinity for the darkness vanishes.",
   "Your affinity for the darkness weakens.",
   "Your affinity for the darkness weakens."},
},

{ MUT_SPINY, 3, 3, mutflag::good, true,
  "spiny",

  {"You are partially covered in sharp spines.",
   "You are mostly covered in sharp spines.",
   "You are completely covered in sharp spines."},

  {"Sharp spines emerge from parts of your body.",
   "Sharp spines emerge from more of your body.",
   "Sharp spines emerge from your entire body."},

  {"Your sharp spines disappear entirely.",
   "Your sharp spines retract somewhat.",
   "Your sharp spines retract somewhat."},
},

{ MUT_POWERED_BY_DEATH, 0, 3, mutflag::good, false,
  "powered by death",

  {"You regenerate a little health from kills.",
   "You regenerate health from kills.",
   "You regenerate a lot of health from kills."},

  {"A wave of death washes over you.",
   "The wave of death grows in power.",
   "The wave of death grows in power."},

  {"Your control of surrounding life forces is gone.",
   "Your control of surrounding life forces weakens.",
   "Your control of surrounding life forces weakens."},
},

{ MUT_POWERED_BY_PAIN, 0, 3, mutflag::good, false,
  "powered by pain",

  {"You sometimes gain a little power by taking damage.",
   "You sometimes gain power by taking damage.",
   "You are powered by pain."},

  {"You feel energised by your suffering.",
   "You feel even more energised by your suffering.",
   "You feel completely energised by your suffering."},

  {"You are no longer energized by suffering.",
   "You feel less energized by suffering.", 
   "You feel less energized by suffering."},
},

{ MUT_AUGMENTATION, 0, 3, mutflag::good, false,
  "augmentation",

  {"Your magical and physical power is slightly enhanced at high health.",
   "Your magical and physical power is enhanced at high health.",
   "Your magical and physical power is greatly enhanced at high health."},

  {"You feel power flowing into your body.",
   "You feel power rushing into your body.",
   "You feel saturated with power."},

  {"Your augmented power disappears completely.",
   "Your power wanes.", 
   "Your power wanes."},
},

{ MUT_MANA_SHIELD, 0, 1, mutflag::good, false,
  "magic shield",

  {"When hurt, damage is shared between your health and your magic reserves.", "", ""},
  {"You feel your magical essence form a protective shroud around your flesh.", "", ""},
  {"Your magical shield disperses.", "", ""},
},

{ MUT_MANA_REGENERATION, 0, 1, mutflag::good, false,
  "magic regeneration",

  {"You regenerate magic rapidly.", "", ""},
  {"You feel your magic shroud grow more resilient.", "", ""},
  {"You regenerate magic more slowly.", "", ""},
},

{ MUT_MANA_LINK, 0, 1, mutflag::good, false,
  "magic link",

  {"When low on magic, you restore magic in place of health.", "", ""},
  {"You feel your life force and your magical essence meld.", "", ""},
  {"You no longer link your magic regeneration to your health regeneration.", "", ""},
},

{ MUT_OOZOMORPH, 0, 3, mutflag::good, true,
  "gelatinous body",

  {"Your gelatinous form absorbs acid and is immune to constriction and sticky flames. (rCorr(absorb), Slime)", "", ""},
  {"", "", ""},
  {"", "", ""},
},

// Jiyva only mutations
{ MUT_SLIME, 0, 3, mutflag::good | mutflag::jiyva, true,
  "slime",

  {"Your slimy body is resistant to acid. (rCorr+)",
   "Your oozing form is immune to acid. (rCorr+++)\n"
   "You regenerate health quickly. (Regen)",
   "Your gelatinous form absorbs acid and is immune to constriction and sticky flames. (rCorr(absorb), Slime)\n"
   "You regenerate health and magic quickly. (Regen, MPRegen)"},

  {"You feel more slimy.",
   "You feel very pliable.",
   "Your body melts completely into viscous ooze."},

  {"You resolidify.",
   "Your body stops oozing.",
   "Your body becomes less slimy."},
},

{ MUT_PROTOPLASM, 0, 3, mutflag::good | mutflag::jiyva, true,
  "watery protoplasm",

  {"Your watery protoplasm protects you from the cold. (rC+)",
   "Your watery protoplasm protects you from extremes of temperature. (rF+, rC+)",
   "Your watery protoplasm protects you from extremes of temperature. (rF+, rC+)\n"
   "When hit by a fiery attack, your protoplasm may erupt scalding steam."},

  {"A vacuole full of water envelops your skin.",
   "Your protoplasm gets thicker and waterier.",
   "Your protoplasm oozes water."},

  {"Your protoplasm dries out and turns into skin.",
   "Your protoplasm feels drier.",
   "Your protoplasm feels drier."},
},

{ MUT_SKIN_BREATHING, 0, 3, mutflag::good | mutflag::jiyva, true,
  "porous membrane",

  {"Breathing through your porous membrane protects you from clouds and drowning. (rCloud, rDrown)",
   "Breathing through your porous membrane protects you from clouds and drowning. (rCloud, rDrown)\n"
   "You engulf enemies in ooze in melee combat.",
   "Breathing through your porous membrane protects you from clouds and drowning. (rCloud, rDrown)\n"
   "You engulf enemies in acidic ooze in melee combat."},

  {"Your skin develops small pores you can breathe through.",
   "Your pores begin to secrete an ooze you can engulf enemies in.",
   "Your ooze becomes acidic."},

  {"Your membrane feels hermetic.",
   "Your membrane quits oozing.",
   "Your ooze feels thinner."},
},

{ MUT_GOLDEN_EYEBALLS, 0, 3, mutflag::good | mutflag::jiyva, true,
  "golden eyeballs",

  {"Your membrane is dotted with golden eyes which may confuse attackers. (+Vis)",
   "Your membrane is coated in golden eyes which increase your resistance against hostile enchantments and may confuse attackers. (+Vis, MR+)",
   "Your membrane is almost completely made up of golden eyes which increase your resistance against hostile enchantments and may confuse attackers. (++Vis, MR++)"},

  {"Golden eyeballs ripple beneath your cytoplasm.",
   "Golden eyeballs peer out through your membranes.",
   "Golden eyeballs cover your extremities fully."},

  {"The eyeballs in your cytoplasm disappear.",
   "The eyeballs in your cytoplasm recede somewhat.",
   "The eyeballs in your cytoplasm recede somewhat."},
},

{ MUT_BUDDING_EYEBALLS, 0, 3, mutflag::good | mutflag::jiyva, true,
  "budding eyeballs",

  {"Your membrane is dotted with shining eyes which may malmutate attackers. (+Vis)",
   "Your membrane is coated in shining eyes which may malmutate attackers. (+Vis)",
   "A variety of eyeballs carouse through your cytoplasm. (++Vis)\n"
   "You may expend your own health to spawn eyeballs from your flesh."},

  {"Eyeballs ripple beneath your cytoplasm.",
   "Eyeballs press against your membranes.",
   "Eyeballs bud out of your cytoplasm, ready to form their own creatures!"},

  {"The eyeballs in your cytoplasm disappear.",
   "The eyeballs in your cytoplasm recede somewhat.",
   "The eyeballs in your cytoplasm recede somewhat."},
},

{ MUT_JIBBERING_MAWS, 0, 3, mutflag::good | mutflag::jiyva, true,
  "jibbering maws",

  {"You are covered in demonic mouths, which bite at your foes in melee.",
   "You are covered in demonic mouths, which bite at your foes in melee; speaking demonic speech through these mouths allows you to cast and invoke divine aid, but not read scrolls during silence.",
   "You are covered in demonic mouths, which bite at your foes in melee; speaking demonic speech through these mouths allows you to cast and invoke divine aid, but not read scrolls during silence.\n"
   "You may expend your own health to let out a foul demonic scream, which shrouds you in unnatural silence and damages those that hear it."},

  {"Foul demonic mouths cover your extremities.",
   "Your demonic maws begin to gibber incessantly in a foul tongue.",
   "You may now let out a demonic scream through your maws."},

  {"The maws dotting your membranes disappear.",
   "Your maws go quiet.",
   "Your maws seem quieter."},
},

{ MUT_FROST_BURST, 0, 3, mutflag::good | mutflag::jiyva, true,
  "frigid spines",

  {"You are partially covered in icy spines of congealed slime, which impale those that strike you. (rC+)",
   "You are completely covered in icy spines of congealed slime, which impale those that strike you. (rC++)",
   "You are completely covered in icy spines of congealed slime, which impale those that strike you. (rC++)\n"
   "You may expend your own health to fire an icy burst of sharp spines in all directions."},

  {"Parts of your protoplasm congeal into tough spines.",
   "More of your protoplasm turns into barbs.",
   "You may now fire spines of icy protoplasm from your flesh."},

  {"Your protoplasm smooths out.",
   "Your membrane is less covered in barbs.",
   "You can no longer fire barbs from your flesh."},
},

{ MUT_ACID_WAVE, 0, 3, mutflag::good | mutflag::jiyva, true,
  "hulking protoplasm",

  {"Your protoplasm is much larger than normal. (HP +10%)",
   "Your protoplasm is much larger and thicker than normal. (HP +20%, rElec+)",
   "Your protoplasm is much larger and thicker than normal. (HP +30%, rElec+)\n"
   "You may expend your own health to fire an acidic wave of ooze from your protoplasm."},

  {"Your protoplasm grows larger.",
   "Your protoplasm grows larger.",
   "You may now fire caustic ooze from your protoplasm."},

  {"Your protoplasm shrinks.",
   "Your protoplasm shrinks.",
   "You can no longer fire caustic ooze from your protoplasm."},
},

{ MUT_MELT, 0, 3, mutflag::good | mutflag::jiyva, true,
  "melting flesh",

  {"Your protoplasm is hot to the touch. (rF+)",
   "Your boiling flesh constantly melts leaving a trail of flaming slime behind you. (rF+)",
   "Your boiling flesh constantly melts leaving a trail of flaming slime behind you. (rF+)\n"
   "You may expend your own health to melt completely and pass through monsters in a line."},

  {"Your protoplasm heats up.",
   "Your protoplasm begins to boil and spill everywhere.",
   "You may now melt completely in order to ooze through your foes."},

  {"Your protoplasm congeals.",
   "Your protoplasm cools down.",
   "You can no longer melt."},
},

{ MUT_TRANSLUCENT_SKIN, 0, 3, mutflag::good | mutflag::jiyva, true,
  "translucent skin",

  {"Your translucent skin slightly reduces your foes' accuracy. (Stealth+)",
   "Your translucent skin reduces your foes' accuracy. (Stealth++)",
   "Your transparent skin significantly reduces your foes' accuracy. (Stealth+++)\n"
   "You may expend some of your HP to become completely invisible, without magical contamination. (+Inv)"},

  {"Your skin becomes partially translucent.",
   "Your skin becomes more translucent.",
   "Your skin becomes completely transparent."},

  {"Your skin returns to its normal opacity.",
   "Your skin's translucency fades.",
   "Your skin's transparency fades."},
},

{ MUT_CYTOPLASM_TRAP, 0, 3, mutflag::good | mutflag::jiyva, true,
  "cytoplasmic trap",

  {"Your cytoplasm is hot to the touch. (rF+)",
   "Your viscous, smoldering cytoplasm coats everything that touches it. (rF+)",
   "Your viscous, smoldering cytoplasm may burn those that touch it or capture the weapons of those that strike it. (rF+)"},
  {"Your cytoplasm heats up.",
   "Your cytoplasm feels particularly viscous and tacky.",
   "Your hot gooey cytoplasm begins to capture things that hit it."},
  {"Your cytoplasm feels colder.",
   "Your cytoplasm feels less resinous.", 
   "Your cytoplasm no longer captures weapons."},
},

{ MUT_PSEUDOPODS, 0, 1, mutflag::good | mutflag::jiyva, true,
  " pseudopods",

  {" pseudopods extend from your body and strike at your foes", "", ""},
  {" pseudopods emerge from your body.", "", ""},
  {"Your pseudopods retract into your body.", "", ""},
},

{ MUT_TENDRILS, 0, 1, mutflag::good | mutflag::jiyva, false,
  "tendrils",

  {"A pair of whip-like tendrils extend from your body. You may wear an extra ring on each.", "", ""},
  {"Tendrils emerge from you body.", "", ""},
  {"Your tendrils retract into your body.", "", ""},
},

{ MUT_AMORPHOUS_BODY, 0, 1, mutflag::good | mutflag::jiyva, false,
  "amorphous body", 

  {"Your body is completely amorphous, allowing you to wear any combination of armour.\n"
      "You have seven amorphous slots; replacing the normal gear slots. Body armours and bardings take two slots, all other armours take one.", "", ""},
  {"Your body becomes completely amorphous.", "", ""},
  {"Your body feels more rigid.", "", ""},
},

{ MUT_CYTOPLASMIC_SUSPENSION, 0, 1, mutflag::good | mutflag::jiyva, false, 
  "cytoplasmic subsumption",

  {"You may subsume any weapon, armour, or jewellery into your cytoplasm to benefit from its magical properties.", "", ""},
  {"A cavity forms in the center of your cytoplasm, in which you can subsume an item.", "", ""},
  {"Your cytoplasm can no longer hold an extra item.", "", ""},
},

{ MUT_ARM_MORPH, 0, 1, mutflag::good | mutflag::jiyva, true, 
  "morphing arms",

  {"Your arms can morph in size from ridiculously small and thin to oversized and top-heavy.\n You may wield any weapon or shield, one-handed; including those normally too large or small for you.", "", ""},
  {"Your arms become more pliable.", "", ""},
  {"Your arms return to their normal size.", "", ""},
},

{ MUT_GELATINOUS_TAIL, 0, 1, mutflag::good | mutflag::jiyva, true, 
  "gelatinous tail, constrict 1",

  {"Your gelatinous tail can morph to fit any barding and constrict a single enemy in combat.", "", ""},
  {"You grow a massive gelatinous, serpentine tail.", "", ""},
  {"Your tail shrinks ", "", ""},
},

{ MUT_MISSILE_GUARD, 0, 1, mutflag::good | mutflag::jiyva, false, 
  "missile-eating ooze (DMsl+)",

  {"You are covered in a sticky ooze that may intercept and consume missiles aimed at you. (DMsl+)", "", ""},
  {"You exude a sticky ooze from your skin.", "", ""},
  {"The ooze on your skin dries up and flakes away.", "", ""},
},

{ MUT_RADIOSYNTHESIS, 0, 3, mutflag::good | mutflag::jiyva, false,
  "radiosynthesis",

  {"You don't mutate from contamination; you become a little more contaminated while casting spells.", 
   "You regenerate faster while glowing and aren't malmutated by magical radiation; you become more contaminated while casting spells.", 
   "You always glow, regenerate faster the more magical radiation you absorb and cannot be malmutated by radiation; you become much more contaminated while casting spells."},
  {"You feel connected to magical radiation.", 
   "You begin to regenerate faster while glowing.", 
   "You feel like bathing in mutagenic glow."},
  {"Your connection to the glow disappears completely.",
   "Your connection to magical radiation recedes.",
   "Your connection to magical radiation recedes."},
},

{ MUT_CORE_MELDING, 0, 2, mutflag::good | mutflag:: jiyva, true,
  "core melding",

   { "The liquid core of your body molds to fit into any body armour near perfectly. (Reduced Encumberance from Armour).", 
     "The molten core of your body molds to fit into any body armour perfectly. (1.5x Base AC and No Encumberance from Armour).", "" },
   { "The core of your form feels very pliable.", "", "" },
   { "Your core feels rigid.", "", "" },
},

{ MUT_ACIDIC_BITE, 3, 1, mutflag::good, true,
  "acidic bite",

  {"You have acidic saliva.", "", ""},
  {"Acid begins to drip from your mouth.", "", ""},
  {"Your mouth feels dry.", "", ""},
},

{ MUT_ANCIENT_WISDOM, 0, 1, mutflag::good, true,
  "ancient wisdom",

  { "You can memorise 50% more levels worth of spells.\nYou gain random stats at level up more slowly, but gain an extra +1 intelligence each time.", "", "" },
  { "", "", "" },
  { "", "", "" },
},

{ MUT_ANTIMAGIC_BITE, 0, 1, mutflag::good, true,
  "antimagic bite",

  {"Your bite disrupts and absorbs the magic of your enemies.", "", ""},
  {"You feel a sudden thirst for magic.", "", ""},
  {"Your magical appetite wanes.", "", ""},
},

{ MUT_NO_POTION_HEAL, 9, 1, mutflag::bad, false,
  "no potion heal",

  {"Potions cannot restore your health.", "", ""},
  {"Your system rejects the healing effects of potions.", "", ""},
  {"Your system accepts the healing effects of potions.", "", ""},
},

// Scale mutations
{ MUT_DISTORTION_FIELD, 5, 1, mutflag::good, false,
  "repulsion field",

  {"You are surrounded by a repulsion field. (RMsl, EV +", "", ""},
  {"You begin to radiate repulsive energy.", "", ""},
  {"You feel less repulsive.", "", ""},
},

{ MUT_ICY_BLUE_SCALES, 2, 1, mutflag::good, true,
  "icy blue scales",

  {"You are covered in icy blue scales. (rC+, AC +", "", ""},
  {"Icy blue scales spread over your body.", "", ""},
  {"Your icy blue scales disappear.", "", ""},
},

{ MUT_IRIDESCENT_SCALES, 2, 1, mutflag::good, true,
  "iridescent scales",

  {"You are covered in iridescent scales. (AC ", "", ""},
  {"Iridescent scales spread over your body.", "", ""},
  {"Your iridescent scales disappear.", "", ""},
},

{ MUT_LARGE_BONE_PLATES, 5, 1, mutflag::good, true,
  "large bone plates",

  {"Your arms are covered in large bone plates. (SH +", "", ""},
  {"Large bone plates spread over your arms.", "", ""},
  {"Your large bone plates disappear.", "", ""},
},

{ MUT_MOLTEN_SCALES, 2, 1, mutflag::good, true,
  "molten scales",

  {"You are covered in molten scales. (rF+, AC +", "", ""},
  {"Molten scales spread over your body.", "", ""},
  {"Your molten scales disappear.", "", ""},
},

{ MUT_ROUGH_BLACK_SCALES, 2, 1, mutflag::good, true,
  "rough black scales",

  {"You are covered in rough black scales. (rN+, AC +"},
  {"Rough black scales spread over your body.", "", ""},
  {"Your rough black scales disappear.", "", ""},
},

{ MUT_RUGGED_BROWN_SCALES, 2, 1, mutflag::good, true,
  "rugged brown scales",

  {"You are covered in rugged brown scales. (+7% HP, AC +", "", ""},
  {"Rugged brown scales spread over your body.", "", ""},
  {"Your rugged brown scales disappear.", "", ""},
},

{ MUT_SLIMY_GREEN_SCALES, 2, 1, mutflag::good, true,
  "slimy green scales",

  {"You are covered in slimy green scales. (rPois, AC +"},
  {"Slimy green scales spread over your body.", "", ""},
  {"Your slimy green scales disappear.", "", ""},
},

{ MUT_THIN_METALLIC_SCALES, 2, 1, mutflag::good, true,
  "thin metallic scales",

  {"You are covered in thin metallic scales. (rElec, AC +", "", ""},
  {"Thin metallic scales spread over your body.", "", ""},
  {"Your thin metallic scales disappear.", "", ""},
},

{ MUT_THIN_SKELETAL_STRUCTURE, 7, 1, mutflag::good, false,
  "thin skeletal structure",

  {"You have an unnaturally thin skeletal structure. (Stealth++, Dex +", "", ""},
  {"Your bones become less dense.", "", ""},
  {"Your skeletal structure redensifies.", "", ""},
},

{ MUT_YELLOW_SCALES, 2, 1, mutflag::good, true,
  "yellow scales",

  {"You are covered in yellow scales. (rCorr, AC +", "", ""},
  {"Yellow scales spread over your body.", "", ""},
  {"Your yellow scales disappear.", "", ""},
},

{ MUT_STURDY_FRAME, 9, 1, mutflag::good, true,
  "sturdy frame",

  {"Your movements are less encumbered by armour. (ER -", "", ""},
  {"You feel less encumbered by your armour.", "",""},
  {"You feel more encumbered by your armour.", "", ""},
},

{ MUT_SANGUINE_ARMOUR, 0, 1, mutflag::good, false,
  "sanguine armour",

  {"When seriously injured, your bleeding wounds create armour. (AC +", "", ""},
  {"You feel your blood ready itself to protect you.", "", ""},
  {"You feel your blood become quiescent.", "", ""},
},

// Draconian rework mutations all the messaging on these is done specially.
{ MUT_MINOR_MARTIAL_APT_BOOST, 0, 1, mutflag::good, false,
   "minor martial skill boost",
    
  {"You are naturally skilled at ", "", ""},
  {"Your draconian ancestry asserts itself.", "", ""},
  {"", "", ""},
},

{ MUT_MAJOR_MARTIAL_APT_BOOST, 0, 2, mutflag::good, false,
   "major martial skill boost",

  { "You are naturally skilled at ", "You are naturally adept at ", "" },
  { "Your draconian ancestry asserts itself.", "Your draconian ancestry asserts itself.", "" },
  {"", "", ""},
},

{ MUT_DRACONIAN_ENHANCER, 0, 2, mutflag::good, true,
  "draconian enhancer",

  { "You are in touch with the powers of ",
    "You are strongly in touch with the powers of ", ""},
  { "You become more in touch with the powers of ", "You become more in touch with the powers of ", ""},
  { "You lose touch with the powers of ", 
    "You feel less in touch with the powers of ", ""},
},

{ MUT_DRACONIAN_DEFENSE, 0, 1, mutflag::good, true,
  "draconian defense",

  { "", "", ""},
  { "", "", ""},
  { "", "", ""},
},

{ MUT_DEFENSIVE_APT_BOOST, 0, 1, mutflag::good, false,
   "defensive skill boost",
    
  {"You are naturally skilled at ", "", ""},
  {"Your draconian ancestry asserts itself.", "", ""},
  {"", "", ""},
},

{ MUT_CAMOUFLAGE, 8, 2, mutflag::good, true,
  "camouflage",

  {"Your skin changes colour to match your surroundings (Stealth++).",
   "Your skin perfectly mimics your surroundings (Stealth++++)).", ""},

  {"Your skin functions as natural camouflage.",
   "Your natural camouflage becomes more effective.", ""},

  {"Your skin no longer functions as natural camouflage.",
   "Your natural camouflage becomes less effective.", ""},
},

{ MUT_IGNITE_BLOOD, 0, 1, mutflag::good, false,
  "ignite blood",

  {"Your demonic aura causes spilled blood to erupt in flames.", "", ""},
  {"Your blood runs red-hot!", "", ""},
  {"You feel a deep chill in your very blood!", "", ""},
},

{ MUT_FOUL_STENCH, 0, 2, mutflag::good, false,
  "foul stench",

  {"You may emit foul miasma when damaged in melee.",
   "You frequently emit foul miasma when damaged in melee.", ""},

  {"You begin to emit a foul stench of rot and decay.",
   "You begin to radiate miasma.", ""},

  {"You faintly smell roses.", 
   "You emit miasma far less often.", ""},
},

{ MUT_PETRIFICATION_RESISTANCE, 0, 1, mutflag::good, false,
  "petrification resistance",

  {"You are immune to petrification.", "", ""},
  {"Your body vibrates.", "", ""},
  {"You briefly stop moving.", "", ""},
},

#if TAG_MAJOR_VERSION == 34
{ MUT_TRAMPLE_RESISTANCE, 0, 1, mutflag::good, false,
  "trample resistance",

  {"You are resistant to trampling.", "", ""},
  {"You feel steady.", "", ""},
  {"You feel unsteady..", "", ""},
},

{ MUT_CLING, 0, 1, mutflag::good, true,
  "cling",

  {"You can cling to walls.", "", ""},
  {"You feel sticky.", "", ""},
  {"You feel slippery.", "", ""},
},
#endif

{ MUT_BLACK_MARK, 0, 1, mutflag::good, false,
  "black mark",

  {"Your melee attacks may debilitate your foes.", "", ""},
  {"An ominous black mark forms on your body.", "", ""},
  {"Your black markings fade.", "", ""},
},

{ MUT_COLD_BLOODED, 0, 1, mutflag::bad, true,
  "cold-blooded",

  {"You are cold-blooded and are slowed more easily by cold attacks.", "", ""},
  {"Your blood runs cold.", "", ""},
  {"Your heart warms up.", "", ""},
},

{ MUT_FLAME_CLOUD_IMMUNITY, 0, 1, mutflag::good, false,
  "flame cloud immunity",

  {"You are immune to clouds of flame.", "", ""},
  {"You feel less concerned about heat.", "", ""},
  {"You feel an odd paranoia about heat.", "", ""},
},

{ MUT_FREEZING_CLOUD_IMMUNITY, 0, 1, mutflag::good, false,
  "freezing cloud immunity",

  {"You are immune to freezing clouds.", "", ""},
  {"You feel less concerned about cold.", "", ""},
  {"You feel an odd paranoia about cold.", "", ""},
},

{ MUT_NO_DRINK, 0, 1, mutflag::bad, false,
  "inability to drink while threatened",

  {"You cannot drink potions while threatened.", "", ""},
  {"You no longer can drink potions while threatened.", "", ""},
  {"You can once more drink potions while threatened.", "", ""},
},

{ MUT_NO_READ, 0, 1, mutflag::bad, false,
  "inability to read while threatened",

  {"You cannot read scrolls while threatened.", "", ""},
  {"You can no longer read scrolls while threatened.", "", ""},
  {"You can once more read scrolls while threatened.", "", ""},
},

{ MUT_MISSING_HAND, 0, 1, mutflag::bad, false,
  "missing a hand",

  {"You are missing a hand.", "", ""},
  {"One of your hands has vanished, leaving only a stump!", "", ""},
  {"Your stump has regrown into a hand!", "", ""},
},

{ MUT_NO_STEALTH, 0, 1, mutflag::bad, false,
  "no stealth",

  {"You cannot be stealthy.", "", ""},
  {"You can no longer be stealthy.", "", ""},
  {"You can once more be stealthy.", "", ""},
},

{ MUT_NO_ARTIFICE, 0, 1, mutflag::bad, false,
  "inability to use devices",

  {"You cannot study or use magical devices.", "", ""},
  {"You can no longer study or use magical devices.", "", ""},
  {"You can once more study and use magical devices.", "", ""},
},

{ MUT_NO_LOVE, 0, 1, mutflag::bad, false,
  "hated by all",

  {"You are hated by all.", "", ""},
  {"You are now hated by all.", "", ""},
  {"You are no longer hated by all.", "", ""},
},

{ MUT_COWARDICE, 0, 1, mutflag::bad, false,
  "cowardly",

  {"Your cowardice makes you less effective in combat with threatening monsters.", "", ""},
  {"You have lost your courage.", "", ""},
  {"You have regained your courage.", "", ""},
},

{ MUT_NO_DODGING, 0, 1, mutflag::bad, false,
  "inability to train dodging",

  {"You cannot train Dodging skill.", "", ""},
  {"You can no longer train Dodging skill.", "", ""},
  {"You can once more train Dodging skill.", "", ""},
},

{ MUT_NO_ARMOUR, 0, 1, mutflag::bad, false,
  "inability to train armour",

  {"You cannot train Armour skill.", "", ""},
  {"You can no longer train Armour skill.", "", ""},
  {"You can once more train Armour skill.", "", ""},
},

{ MUT_NO_AIR_MAGIC, 0, 1, mutflag::bad, false,
  "no air magic",

  {"You cannot study or cast Air magic.", "", ""},
  {"You can no longer study or cast Air magic.", "", ""},
  {"You can once more study and cast Air magic.", "", ""},
},

{ MUT_NO_CHARM_MAGIC, 0, 1, mutflag::bad, false,
  "no charms magic",

  {"You cannot study or cast Charms magic.", "", ""},
  {"You can no longer study or cast Charms magic.", "", ""},
  {"You can once more study and cast Charms magic.", "", ""},
},

{ MUT_NO_EARTH_MAGIC, 0, 1, mutflag::bad, false,
  "no earth magic",

  {"You cannot study or cast Earth magic.", "", ""},
  {"You can no longer study or cast Earth magic.", "", ""},
  {"You can once more study and cast Earth magic.", "", ""},
},

{ MUT_NO_FIRE_MAGIC, 0, 1, mutflag::bad, false,
  "no fire magic",

  {"You cannot study or cast Fire magic.", "", ""},
  {"You can no longer study or cast Fire magic.", "", ""},
  {"You can once more study and cast Fire magic.", "", ""},
},

{ MUT_NO_HEXES_MAGIC, 0, 1, mutflag::bad, false,
  "no hexes magic",

  {"You cannot study or cast Hexes magic.", "", ""},
  {"You can no longer study or cast Hexes magic.", "", ""},
  {"You can once more study and cast Hexes magic.", "", ""},
},

{ MUT_NO_ICE_MAGIC, 0, 1, mutflag::bad, false,
  "no ice magic",

  {"You cannot study or cast Ice magic.", "", ""},
  {"You can no longer study or cast Ice magic.", "", ""},
  {"You can once more study and cast Ice magic.", "", ""},
},

{ MUT_NO_NECROMANCY_MAGIC, 0, 1, mutflag::bad, false,
  "no necromancy magic",

  {"You cannot study or cast Necromancy magic.", "", ""},
  {"You can no longer study or cast Necromancy magic.", "", ""},
  {"You can once more study and cast Necromancy magic.", "", ""},
},

{ MUT_NO_POISON_MAGIC, 0, 1, mutflag::bad, false,
  "no poison magic",

  {"You cannot study or cast Poison magic.", "", ""},
  {"You can no longer study or cast Poison magic.", "", ""},
  {"You can once more study and cast Poison magic.", "", ""},
},

{ MUT_NO_SUMMONING_MAGIC, 0, 1, mutflag::bad, false,
  "no summoning magic",

  {"You cannot study or cast Summoning magic.", "", ""},
  {"You can no longer study or cast Summoning magic.", "", ""},
  {"You can once more study and cast Summoning magic.", "", ""},
},

{ MUT_NO_TRANSLOCATION_MAGIC, 0, 1, mutflag::bad, false,
  "no translocations magic",

  {"You cannot study or cast Translocations magic.", "", ""},
  {"You can no longer study or cast Translocations magic.", "", ""},
  {"You can once more study and cast Translocations magic.", "", ""},
},

{ MUT_NO_TRANSMUTATION_MAGIC, 0, 1, mutflag::bad, false,
  "no transmutations magic",

  {"You cannot study or cast Transmutations magic.", "", ""},
  {"You can no longer study or cast Transmutations magic.", "", ""},
  {"You can once more study and cast Transmutations magic.", "", ""},
},

{ MUT_PHYSICAL_VULNERABILITY, 6, 3, mutflag::bad, false,
  "reduced AC",

  {"You take slightly more damage. (AC -5)",
    "You take more damage. (AC -10)",
    "You take considerably more damage. (AC -15)"},
  {"You feel more vulnerable to harm.",
    "You feel more vulnerable to harm.",
    "You feel more vulnerable to harm."},
  {"You no longer feel extra vulnerable to harm.",
    "You feel less vulnerable to harm.",
    "You feel less vulnerable to harm."},
},

{ MUT_SLOW_REFLEXES, 6, 3, mutflag::bad, false,
  "reduced EV",

  {"You have somewhat slow reflexes. (EV -5)",
    "You have slow reflexes. (EV -10)",
    "You have very slow reflexes. (EV -15)"},
  {"Your reflexes slow.",
    "Your reflexes slow further.",
    "Your reflexes slow further."},
  {"You reflexes return to normal.",
    "You reflexes speed back up.",
    "You reflexes speed back up."},
},

{ MUT_MAGICAL_VULNERABILITY, 17, 2, mutflag::bad, false,
  "magic vulnerability",

  {"You are vulnerable to magic. (MR--)",
    "You are very vulnerable to magic. (MR----)", ""},
  {"You feel vulnerable to magic.",
    "You feel more vulnerable to magic.", ""},
  {"You no longer feel vulnerable to magic.",
    "You feel less vulnerable to magic.", ""},
},

{ MUT_ANTI_WIZARDRY, 0, 3, mutflag::bad, false,
  "disrupted magic",

  {"Your casting is slightly disrupted.",
    "Your casting is disrupted.",
    "Your casting is seriously disrupted."},
  {"Your ability to control magic is disrupted.",
    "Your ability to control magic is more disrupted.",
    "Your ability to control magic is more disrupted."},
  {"Your ability to control magic is no longer disrupted.",
    "Your ability to control magic is less disrupted.",
    "Your ability to control magic is less disrupted."},
},

{ MUT_MP_WANDS, 7, 1, mutflag::good, false,
  "MP-powered wands",

  {"You expend magic power (3 MP) to strengthen your wands.", "", ""},
  {"You feel your magical essence link to the dungeon's wands.", "", ""},
  {"Your magical essence no longer links to wands of the dungeon.", "", ""},
},

{ MUT_UNSKILLED, 4, 3, mutflag::bad, false,
  "unskilled",

  {"You are somewhat unskilled. (-1 Apt)",
    "You are unskilled. (-2 Apt)",
    "You are extremely unskilled. (-3 Apt)"},
  {"You feel less skilled.",
    "You feel less skilled.",
    "You feel less skilled."},
  {"You regain all your skill.",
    "You regain some skill.",
    "You regain some skill."},
},

{ MUT_INEXPERIENCED, 0, 3, mutflag::bad, false,
    "inexperienced",

  {"You are somewhat inexperienced. (-2 XL)",
   "You are inexperienced. (-4 XL)",
   "You are extremely inexperienced. (-6 XL)"},
  {"You feel less experienced.",
   "You feel less experienced.",
   "You feel less experienced."},
  {"You regain all your potential.",
   "You regain some potential.",
   "You regain some potential."},
},

{ MUT_STASIS, 0, 1, mutflag::good, false,
  "permanent stasis",

  {"You cannot be hasted, slowed, berserked, paralysed or teleported.", "", ""},
  {"You feel a comfortable sense of stasis.", "", ""},
  {"You feel weirdly uncertain.", "", ""},
},

{ MUT_BURROWING, 0, 1, mutflag::good, true,
  "dig shafts and tunnels",

  {"You can dig through walls and to a lower floor.", "", ""},
  {"Your mandibles function properly again.", "", ""},
  {"Your mandibles atrophy and become useless.", "", ""},
},

{ MUT_MULTIARM, 0, 1, mutflag::good, true,
  "four arms",

  { "You can wield any weapon or shield with a single pair of your four arms.", "", "" },
  { "", "", "" },
  { "", "", "" },
},

{ MUT_PAWS, 0, 1, mutflag::good, true,
  "sharp and stealthy paws",

  {"Your feline paws and retractable claws are effective at attacking unaware monsters and moving quietly.", "", ""},
  {"", "", ""},
  {"", "", ""},
},

{ MUT_MISSING_EYE, 0, 1, mutflag::bad, false,
  "missing an eye",

  {"You are missing an eye, making it more difficult to aim.", "", ""},
  {"Your right eye vanishes! The world loses its depth.", "", ""},
  {"Your right eye suddenly reappears! The world regains its depth.", "", ""},
},

{ MUT_TEMPERATURE_SENSITIVITY, 0, 1, mutflag::bad, false,
  "temperature sensitive",

  {"You are sensitive to extremes of temperature. (rF-, rC-)", "", ""},
  {"You feel sensitive to extremes of temperature.", "", ""},
  {"You no longer feel sensitive to extremes of temperature", "", ""},
},

{ MUT_NO_REGENERATION, 0, 1, mutflag::bad, false,
  "no regeneration",

  {"You do not regenerate.", "", ""},
  {"You stop regenerating.", "", ""},
  {"You start regenerating.", "", ""},
},

{ MUT_GODS_PITY, 8, 2, mutflag::good, false,
  "maniacal devotion",

  { "Your maniacal devotion increases the power of your invoked divine abilities.",
    "Your maniacal devotion increases the power of your passive and invoked divine abilities.", "" },
  { "You feel a growing devotion to the divine.",
    "You feel a growing devotion to the divine.", "" },
  { "You feel less arduent as a worshipper.",
    "You feel less arduent as a worshipper.", "" },
},

{ MUT_STRONG_NOSE, 7, 1, mutflag::good, false,
  "strong nose",

  {"Your uncanny sense of smell can sniff out nearby items.", "", ""},
  {"Your sense of smell grows stronger.", "", ""},
  {"Your sense of smell gets weaker.", "", ""},
},

{ MUT_SILENT_CAST, 0, 1, mutflag::good, false,
  "silent casting",

  { "You can communicate with gods and cast spells while silenced.", "", "" },
  { "You gain the power to cast without speech.", "", "" },
  { "Silence once again prevents your spellcasting.", "", "" },
},

{ MUT_INSUBSTANTIAL, 0, 1, mutflag::good, false,
  "insubstantial",

  { "Your insubstantial form grants resistance to all physical attack and full immunity to sticky flames, nets and constriction. (rPhysical+)", "", "" },
  { "Your spirit separates from your body becoming a ghostly form.", "", "" },
      // BCADDNOTE: This only makes sense for the only race that currently gains this with level. If more races are given this with level revise/use mutation-messaging.
  { "You become solid.", "", "" },
},

{ MUT_SOFT_FLESH, 0, 1, mutflag::bad, true,
  "soft flesh",

  { "Your soft flesh is weak to deep cuts. Heavy manmade armour will nullify this weakness. (rSlashing-)", "", "" },
  { "Your flesh turns soft and spongy.", "", "" },
  { "Your scales now protect your innards.", "", "" },
},

{ MUT_UNDEAD_FLESH, 0, 1, mutflag::good, true,
  "undead flesh",

  { "Your undead flesh lacks the need for functioning vital organs and takes reduced damage from stabs. (rPiercing+)", "", "" },
  { "You are no longer resistant against piercing.", "", "" },
  { "Your undeath grants you resistance to stabs.", "", "" },
},

{ MUT_BARKSKIN, 0, 1, mutflag::good, false,
  "vine-covered skin",

  { "The vines covering you are difficult to pierce. (rPiercing+)", "", "" },
  { "Your skin becomes covered in vines.", "", "" },
  { "The vines on your skin wither and fall away.", "", "" },
},

{ MUT_ARTIFICIAL_FLESH, 0, 1, mutflag::good, true,
  "artificial flesh",

  { "Your artificial form resists hard blunt hits and is difficult to cut into. (rBludgeoning+ , rSlashing+)", "", "" },
  { "", "", "" },
  { "", "", "" },
},

{ MUT_EXOSKELETON, 0, 1, mutflag::good, true,
  "exoskeleton",

  { "Your insectoid exoskeleton is difficult to cut into, but your joints provide obvious stabbing targets. (rSlashing+, rPiercing-)", "", "" },
  { "", "", "" },
  { "", "", "" },
},

{ MUT_GELATINOUS_FLESH, 0, 1, mutflag::good | mutflag::jiyva, true,
  "gelatinous flesh",

  { "Your formless flesh reforms from cuts and stabs, but takes increased shock from blunt direct hits. Heavy manmade armour will nullify this weakness. (rSlashing+, rPiercing+, rBludgeoning-)", "", "" },
  { "", "", "" },
  { "", "", "" },
},

{ MUT_ACID_RESISTANCE, 7, 1, mutflag::good, true,
  "acid resistance",

  {"You are resistant to acid. (rCorr)", "", ""},
  {"You feel resistant to acid.", "",  ""},
  {"You feel less resistant to acid.", "", ""},
},

{ MUT_CORRUPTED_CHARM, 0, 1, mutflag::bad | mutflag::miscast, false,
  "-Charms",

  {"Corrupted Charms magic prevents successful use of Charms spells. (-Charms)", "", ""},
  {"You are suffused with corrupted Charms magic.", "",  ""},
  {"You are capable of using Charms spells again.", "", ""},
},

{ MUT_BUFF_AURA, 0, 1, mutflag::bad | mutflag::miscast, false,
  "beguiling aura",

  {"Lingering Charms magic may augment those that attack you.", "", ""},
  {"Your flesh pulses with an an unnatural beguiling aura.", "",  ""},
  {"Your beguiling aura wears off.", "", ""},
},

};

static const mutation_category_def category_mut_data[] =
{
  { RANDOM_MUTATION, "any"},
  { RANDOM_XOM_MUTATION, "xom"},
  { RANDOM_GOOD_MUTATION, "good"},
  { RANDOM_BAD_MUTATION, "bad"},
  { RANDOM_SLIME_MUTATION, "jiyva"},
  { RANDOM_NON_SLIME_MUTATION, "nonslime"},
  { RANDOM_CORRUPT_MUTATION, "corrupt"},
  { RANDOM_QAZLAL_MUTATION, "qazlal"},
};
