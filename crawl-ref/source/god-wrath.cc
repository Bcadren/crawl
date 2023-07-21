/**
 * @file
 * @brief Divine retribution.
**/

#include "AppHdr.h"

#include "god-wrath.h"

#include <sstream>

#include "abyss.h" // lugonu_corrupt_level
#include "act-iter.h"
#include "areas.h"
#include "artefact.h"
#include "attack.h" // attack_strength_punctuation
#include "attitude-change.h"
#include "cleansing-flame-source-type.h"
#include "coordit.h"
#include "database.h"
#include "death-curse.h"
#include "decks.h"
#include "env.h"
#include "food.h"
#include "ghost.h"
#include "god-abil.h"
#include "god-passive.h" // shadow_monster
#include "item-prop.h"
#include "item-status-flag-type.h"
#include "items.h"
#include "makeitem.h"
#include "message.h"
#include "misc.h"
#include "mon-behv.h"
#include "mon-cast.h"
#include "mon-pick.h"
#include "mon-place.h"
#include "mon-tentacle.h"
#include "mutation.h"
#include "notes.h"
#include "player-stats.h"
#include "random.h"
#include "religion.h"
#include "shopping.h"
#include "shout.h"
#include "spl-clouds.h"
#include "spl-goditem.h"
#include "spl-selfench.h"
#include "spl-summoning.h"
#include "spl-transloc.h"
#include "spl-util.h"
#include "state.h"
#include "stringutil.h"
#include "tag-version.h"
#include "terrain.h"
#include "transform.h"
#include "unicode.h"
#include "view.h"
#include "xom.h"

static void _god_smites_you(god_type god, const char *message = nullptr,
                            kill_method_type death_type = NUM_KILLBY);
static void _tso_blasts_cleansing_flame(const char *message = nullptr);

static const char *_god_wrath_adjectives[] =
{
    "bugginess",        // NO_GOD
    "wrath",            // Zin
    "wrath",            // the Shining One (unused)
    "malice",           // Kikubaaqudgha
    "anger",            // Yredelemnul
    "capriciousness",   // Xom
    "wrath",            // Vehumet
    "fury",             // Okawaru
    "fury",             // Makhleb
    "will",             // Sif Muna
    "fiery rage",       // Trog
    "wrath",            // Nemelex
    "displeasure",      // Elyvilon
    "touch",            // Lugonu
    "wrath",            // Beogh
    "vengeance",        // Jiyva
    "enmity",           // Fedhas Madhash
    "meddling",         // Cheibriados
    "doom",             // Ashenzari (unused)
    "darkness",         // Dithmenos
    "greed",            // Gozag (unused)
    "adversity",        // Qazlal
    "disappointment",   // Ru
#if TAG_MAJOR_VERSION == 34
    "progress",         // Pakellas
#endif
    "fury",             // Uskayaw
    "memory",           // Hepliaklqana (unused)
    "rancor",           // Wu Jian
    "outrage",          // Bahamut & Tiamat
};
COMPILE_CHECK(ARRAYSZ(_god_wrath_adjectives) == NUM_GODS);

/**
 * Return a name associated with the given god's wrath.
 *
 * E.g., "the darkness of Dithmenos".
 * Used for cause of death messages (e.g. 'killed by a titan (summoned by the
 * wrath of Okawaru)')
 *
 * @param god   The god in question.
 * @return      A string name for the god's wrath.
 */
static string _god_wrath_name(god_type god)
{
    const bool use_full_name = god == GOD_FEDHAS      // fedhas is very formal.
                               || god == GOD_WU_JIAN; // apparently.

    // You like exceptions right? Twin gods need special rules.
    if (god == GOD_BAHAMUT_TIAMAT)
    {
        return coinflip ()  ? "the outrage of Bahamut"
                            : "the brutality of Tiamat";
    }

    return make_stringf("the %s of %s",
                        _god_wrath_adjectives[god],
                        god_name(god, use_full_name).c_str());
}

static mgen_data _wrath_mon_data(monster_type mtyp, god_type god)
{
    mgen_data mg = mgen_data::hostile_at(mtyp, true, you.pos())
                    .set_summoned(nullptr, 0, 0, god)
                    .set_non_actor_summoner(_god_wrath_name(god));
    mg.extra_flags |= (MF_NO_REWARD | MF_HARD_RESET);
    return mg;
}

static const pop_entry _okawaru_servants[] =
{ // warriors
  {  1,  3,   3, FALL, MONS_ORC },
  {  1,  3,   3, FALL, MONS_GNOLL },
  {  2,  6,   3, PEAK, MONS_OGRE },
  {  2,  6,   2, PEAK, MONS_GNOLL_SERGEANT },
  {  3,  7,   1, FLAT, MONS_TWO_HEADED_OGRE },
  {  3, 13,   3, PEAK, MONS_ORC_WARRIOR },
  {  5, 15,   3, PEAK, MONS_ORC_KNIGHT },
  {  5, 15,   2, PEAK, MONS_CYCLOPS },
  {  7, 21,   2, PEAK, MONS_CENTAUR_WARRIOR },
  {  7, 21,   2, PEAK, MONS_NAGA_WARRIOR },
  {  7, 21,   2, PEAK, MONS_TENGU_WARRIOR },
  {  7, 21,   1, FLAT, MONS_MERFOLK_IMPALER },
  {  7, 21,   1, FLAT, MONS_MERFOLK_JAVELINEER },
  {  7, 21,   1, FLAT, MONS_MINOTAUR },
  {  9, 27,   2, FLAT, MONS_STONE_GIANT },
  {  9, 27,   1, FLAT, MONS_DEEP_ELF_KNIGHT },
  {  9, 27,   1, FLAT, MONS_DEEP_ELF_ARCHER },
  { 11, 21,   1, FLAT, MONS_ORC_WARLORD },
  { 11, 27,   2, FLAT, MONS_FIRE_GIANT },
  { 11, 27,   2, FLAT, MONS_FROST_GIANT },
  { 13, 27,   1, FLAT, MONS_DEEP_ELF_BLADEMASTER },
  { 13, 27,   1, FLAT, MONS_DEEP_ELF_MASTER_ARCHER },
  { 13, 27,   1, FLAT, RANDOM_BASE_DRACONIAN },
  { 15, 27,   2, FLAT, MONS_TITAN },
  { 18, 27,   1, RISE, MONS_DRACONIAN_KNIGHT },
  { 0,0,0,FLAT,MONS_0 }
};

static bool _okawaru_random_servant()
{
    monster_type mon_type = pick_monster_from(_okawaru_servants,
                                              you.experience_level);

    mgen_data temp = _wrath_mon_data(mon_type, GOD_OKAWARU);

    // Don't send dream sheep into battle, but otherwise let bands in.
    // This makes sure you get multiple orcs/gnolls early on.
    if (mon_type != MONS_CYCLOPS)
        temp.flags |= MG_PERMIT_BANDS;

    return create_monster(temp, false);
}

static bool _dithmenos_random_shadow(const int count, const int tier)
{
    monster_type mon_type = MONS_SHADOW;
    if (tier >= 2 && count == 0 && coinflip())
        mon_type = MONS_TZITZIMITL;
    else if (tier >= 1 && count < 3 && coinflip())
        mon_type = MONS_CYSGODOL;

    return create_monster(_wrath_mon_data(mon_type, GOD_DITHMENOS), false);
}

/**
 * Summon divine warriors of the Shining One to punish the player.
 */
static void _tso_summon_warriors()
{
    bool success = false;
    int how_many = 1 + random2(you.experience_level / 5) + random2(3);

    for (; how_many > 0; --how_many)
    {
        if (summon_holy_warrior(100, true))
            success = true;
    }

    simple_god_message(success ? " sends the divine host to punish "
                       "you for your evil ways!"
                       : "'s divine host fails to appear.", GOD_SHINING_ONE);

}

/**
 * The Shining One shouts angrily to alert the player's foes!
 */
static void _tso_shouts()
{
    simple_god_message(" booms out: "
                       "\"Take the path of righteousness! REPENT!\"",
                       GOD_SHINING_ONE);
    noisy(25, you.pos()); // same as scroll of noise
}

/**
 * The Shining One silences the player!!
 */
static void _tso_squelches()
{
    god_speaks(GOD_SHINING_ONE,
               "You feel the Shining One's silent rage upon you!");
    cast_silence(25);
}

/**
 * Call down the wrath of the Shining One upon the player!
 *
 * Holy warriors/cleansing theme.
 *
 * @return Whether to take further divine wrath actions afterward.
 */
static bool _tso_retribution()
{
    switch (random2(7))
    {
    case 0:
    case 1:
    case 2:
        _tso_summon_warriors();
        break;
    case 3:
    case 4:
        _tso_blasts_cleansing_flame();
        break;
    case 5:
        _tso_shouts();
        break;
    case 6:
        _tso_squelches();
        break;
    }
    return true;
}

static void _zin_remove_good_mutations()
{
    if (!you.how_mutated())
        return;

    const god_type god = GOD_ZIN;
    bool success = false;

    simple_god_message(" draws some chaos from your body!", god);

    bool failMsg = true;

    for (int i = 7; i >= 0; --i)
    {
        // Ensure that only good mutations are removed.
        if (i <= random2(10)
            && delete_mutation(RANDOM_GOOD_MUTATION, _god_wrath_name(god),
                               failMsg, false, true, true))
        {
            success = true;
        }
        else
            failMsg = false;
    }

    if (success && !you.how_mutated())
        simple_god_message(" rids your body of chaos!", god);
}

static bool _zin_retribution()
{
    // preaching/creeping doom theme
    const god_type god = GOD_ZIN;

    int punishment = random2(8);

    // If not mutated, do something else instead.
    if (punishment > 7 && !you.how_mutated())
        punishment = random2(6);

    switch (punishment)
    {
    case 0:
    case 1:
    case 2: // recital
        simple_god_message(" recites the Axioms of Law to you!", god);
        switch (random2(3))
        {
        case 0:
            confuse_player(5 + random2(3));
            break;
        case 1:
            you.put_to_sleep(nullptr, 30 + random2(20));
            break;
        case 2:
            slow_player(20 + random2(15));
            return false;
        }
        break;
    case 3:
    case 4: // famine
        simple_god_message(" sends a famine down upon you!", god);
        make_hungry(you.hunger / 2, false);
        break;
    case 5: // noisiness
        simple_god_message(" booms out: \"Turn to the light! REPENT!\"", god);
        noisy(25, you.pos()); // same as scroll of noise
        break;
    case 6:
    case 7: // remove good mutations
        _zin_remove_good_mutations();
        break;
    }
    return true;
}

static bool _cheibriados_retribution()
{
    // time god/slowness theme
    const god_type god = GOD_CHEIBRIADOS;

    // Chei retribution might only make sense in combat.
    // We can crib some Xom code for this. {bh}
    int tension = get_tension(GOD_CHEIBRIADOS);
    int wrath_value = random2(tension);

    // Determine the level of wrath
    int wrath_type = 0;

    if (wrath_value < 4)       { wrath_type = 1; }
    else
    {
        if (x_chance_in_y(you.experience_level, 90))
            wrath_type = 4;
        else if (coinflip())
            wrath_type = 3;
        else
            wrath_type = 2;
    }

    // Strip away extra speed
    dec_haste_player(10000);

    switch (wrath_type)
    {
    // Very high tension wrath.
    // Summon torpor snails or direct (slouch) damage then start sleeping.
    case 4:
        if (coinflip())
        {
            simple_god_message(" sends glorious snails to punish you.", god);
            for (int i = 2 + you.experience_level / 8; i > 0; i++)
                create_monster(_wrath_mon_data(MONS_TORPOR_SNAIL, GOD_CHEIBRIADOS), false);
            dec_penance(god, 1); // and fall-through.
        }
        else
        {
            int dam = 3 + roll_dice(3, you.experience_level);
            dam *= max(20 - player_movement_speed(), 4);
            dam /= 10;
            mprf(MSGCH_WARN, "You feel time thicken around you%s", attack_strength_punctuation(dam).c_str());
            ouch(dam, KILLED_BY_DIVINE_WRATH, MID_NOBODY, "displeasure of Cheibriados");

        }
    // High tension wrath
    // Sleep the player and slow the player with 50% chance.
    case 3:
        mpr("You lose track of time.");
        you.put_to_sleep(nullptr, 30 + random2(20));
        if (wrath_type == 4 || coinflip())
            break;
        else
            dec_penance(god, 1); // and fall-through.
    // Medium tension
    case 2:
        if (you.duration[DUR_SLOW] < 180 * BASELINE_DELAY)
        {
            mprf(MSGCH_WARN, "You feel the world leave you behind!");
            slow_player(100);
        }
        break;
    // Low/no tension; lose stats.
    case 1:
        simple_god_message(" obstructs your stats as your blasphemy continues.");
        lose_stat(STAT_RANDOM, 1 + random2avg(8, 2));
        break;

    default:
        break;
    }

    return true;
}

/**
 * Choose a type of destruction with which to punish the player.
 *
 * @return A spell type to hurl at the player.
 */
static spell_type _makhleb_destruction_type()
{
    const int severity = min(random_range(you.experience_level / 14,
                                          you.experience_level / 9),
                             2);
    switch (severity)
    {
        case 0:
        default:
            // minor destruction
            return random_choose(SPELL_THROW_FLAME,
                                 SPELL_PAIN,
                                 SPELL_STONE_ARROW,
                                 SPELL_SHOCK,
                                 SPELL_SPIT_ACID);
        case 1:
            // major destruction
            return random_choose(SPELL_BOLT_OF_FIRE,
                                 SPELL_FIREBALL,
                                 SPELL_LIGHTNING_BOLT,
                                 SPELL_STICKY_FLAME,
                                 SPELL_IRON_SHOT,
                                 SPELL_BOLT_OF_DRAINING,
                                 SPELL_ORB_OF_ELECTRICITY);
        case 2:
            // legendary destruction (no IOOD because it doesn't really
            // work here)
            return random_choose(SPELL_FIREBALL,
                                 SPELL_LEHUDIBS_CRYSTAL_SPEAR,
                                 SPELL_ORB_OF_ELECTRICITY,
                                 SPELL_FLASH_FREEZE,
                                 SPELL_GHOSTLY_FIREBALL);
    }
}

/**
* Create a fake 'avatar' monster representing a god, with which to hurl
* destructive magic at foolish players.
*
* @param god           The god doing the wrath-hurling.
* @return              An avatar monster, or nullptr if none could be set up.
*/
static monster* _get_avatar(god_type god)
{
    monster* avatar = shadow_monster(false);
    if (!avatar)
        return nullptr;

    // shadow_monster() has the player's mid, which is no good here.
    avatar->set_new_monster_id();

    avatar->mname = _god_wrath_name(god);
    avatar->flags |= MF_NAME_REPLACE;
    avatar->attitude = ATT_HOSTILE;
    avatar->set_hit_dice(you.experience_level);

    return avatar;
}

/// Cleanup a temporary 'avatar' monster.
static void _reset_avatar(monster &avatar)
{
    env.mid_cache.erase(avatar.mid);
    shadow_monster_reset(&avatar);
}

bool spell_retribution(spell_type spell, god_type god, const char* message)
{
    monster* avatar = _get_avatar(god);
    // can't be const because mons_cast() doesn't accept const monster*

    if (avatar == nullptr || spell == SPELL_NO_SPELL)
    {
        simple_god_message(" has no time to deal with you just now.", god);
        return false; // not a very dazzling divine experience...
    }

    simple_god_message(message ? message : " rains destruction down upon you!",
        god);
    bolt beam;
    beam.source = you.pos();
    beam.target = you.pos();
    beam.aimed_at_feet = true;
    beam.source_name = avatar->mname;
    mons_cast(avatar, beam, spell, MON_SPELL_PRIEST, false);

    _reset_avatar(*avatar);
    return true;
}

/**
 * Figure out how many greater servants (2s) an instance of Makhleb's wrath
 * should summon.
 *
 * @return The number of greater servants to be summoned by Makhleb's wrath.
 */
static int _makhleb_num_greater_servants()
{
    const int severity = 1 + you.experience_level / 2
                           + random2(you.experience_level / 2);

    if (severity > 13)
        return 2 + random2(you.experience_level / 5 - 2); // up to 6 at XL27
    else if (severity > 7 && !one_chance_in(5))
        return 1;
    return 0;
}

/**
 * Attempt to summon one of Makhleb's diabolical servants to punish the player.
 *
 * @param servant   The type of servant to be summoned.
 * @return          Whether the summoning was successful.
 */
static bool _makhleb_summon_servant(monster_type servant)
{
    return create_monster(_wrath_mon_data(servant, GOD_MAKHLEB), false);
}

/**
 * Unleash Makhleb's fiendish minions on the player!
 *
 * @return Whether to take further divine wrath actions afterward. (true.)
 */
static bool _makhleb_summon_servants()
{
    const int greater_servants = _makhleb_num_greater_servants();

    // up to 6 at XL25+
    const int total_servants = max(greater_servants,
                               1 + (random2(you.experience_level)
                                 + random2(you.experience_level)) / 10);
    const int lesser_servants = total_servants - greater_servants;

    int summoned = 0;

    for (int i = 0; i < greater_servants; i++)
    {
        const monster_type servant = random_choose(MONS_OXARMORDTH,
                                                   MONS_GWYRDD,
                                                   MONS_CENYSUS,
                                                   MONS_BALRUG,
                                                   MONS_TITIVILUS);
        if (_makhleb_summon_servant(servant))
            summoned++;
    }

    for (int i = 0; i < lesser_servants; i++)
    {
        const monster_type servant = random_choose(MONS_VLERK,
                                                   MONS_NEQOXEC,
                                                   MONS_NYCHDOD,
                                                   MONS_MYGDARTH,
                                                   MONS_YNOXINUL);
        if (_makhleb_summon_servant(servant))
            summoned++;
    }

    simple_god_message(summoned > 1 ? " sends minions to punish you." :
                       summoned > 0 ? " sends a minion to punish you."
                       : "'s minions fail to arrive.", GOD_MAKHLEB);

    return true;

}

/**
 * Call down the wrath of Makhleb upon the player!
 *
 * Demonic servant theme.
 *
 * @return Whether to take further divine wrath actions afterward.
 */
static bool _makhleb_retribution()
{
    if (coinflip())
        return spell_retribution(_makhleb_destruction_type(), GOD_MAKHLEB);
    else
        return _makhleb_summon_servants();
}

static bool _kikubaaqudgha_retribution()
{
    // death/necromancy theme
    const god_type god = GOD_KIKUBAAQUDGHA;

    god_speaks(god, coinflip() ? "You hear Kikubaaqudgha cackling."
                               : "Kikubaaqudgha's malice focuses upon you.");

    if (!count_corpses_in_los(nullptr) || random2(you.experience_level) > 4)
    {
        // Either zombies, or corpse rot + skeletons.
        kiku_receive_corpses(you.experience_level * 4);

        if (coinflip())
            corpse_rot(nullptr);
    }

    if (x_chance_in_y(you.experience_level, 27))
    {
        // torment, or 3 death curses of maximum power
        if (!player_res_torment(false))
            torment(nullptr, TORMENT_KIKUBAAQUDGHA, you.pos());
        else
        {
            for (int i = 0; i < 3; ++i)
            {
                death_curse(you, nullptr,
                            _god_wrath_name(god), you.experience_level);
            }
        }
    }
    else if (random2(you.experience_level) >= 4)
    {
        // death curse, 25% chance of additional curse
        const int num_curses = one_chance_in(4) ? 2 : 1;
        for (int i = 0; i < num_curses; i++)
        {
                death_curse(you, nullptr,
                            _god_wrath_name(god), you.experience_level);
        }
    }

    // Every act of retribution causes corpses in view to rise against
    // you.
    animate_dead(&you, 1 + random2(3), BEH_HOSTILE, MHITYOU, 0,
                 _god_wrath_name(god), god);

    return true;
}

static bool _yredelemnul_retribution()
{
    // undead theme
    const god_type god = GOD_YREDELEMNUL;

    if (coinflip())
    {
        if (you_worship(god) && coinflip() && yred_slaves_abandon_you())
            ;
        else
        {
            int how_many = 1 + random2avg(1 + (you.experience_level / 5), 2);
            int count = 0;

            for (; how_many > 0; --how_many)
            {
                monster_type mon_type = MONS_ABOMINATION_SMALL;
                if (x_chance_in_y(you.experience_level, 40))
                    mon_type = MONS_ABOMINATION_LARGE;

                mgen_data mg(mon_type, BEH_HOSTILE, you.pos(), MHITYOU);
                mg.set_summoned(0, 0, 0, GOD_YREDELEMNUL);
                mg.non_actor_summoner = "the anger of Yredelemnul";
                mg.extra_flags |= (MF_NO_REWARD | MF_HARD_RESET);

                create_monster(mg);
            }

            simple_god_message(count > 1 ? " sends abominations to punish you." :
                               count > 0 ? " sends an abomination to punish you."
                                         : "'s abominations fail to arrive.", god);
        }
    }
    else
        spell_retribution(SPELL_BOLT_OF_DRAINING, god, "'s anger turns toward you for a moment.");

    return true;
}

static bool _trog_retribution()
{
    // berserk theme
    const god_type god = GOD_TROG;

    if (you.berserk())
    {
        simple_god_message(" shortens your rage.");
        you.duration[DUR_BERSERK] = 20;
    }
    else if (you.can_go_berserk(true) && coinflip())
    {
        simple_god_message(" takes away your ability to berserk.");
        const int dur = 12 + roll_dice(2, 12);
        you.increase_duration(DUR_BERSERK_COOLDOWN, dur * 2);
    }

    if (!one_chance_in(3))
    {
        int count = 0;
        int points = 3 + you.experience_level * 3;

        {
            msg::suppress msg;

            while (points > 0)
            {
                int cost =
                    min(min(random2avg((1 + you.experience_level / 3), 2) + 3,
                            10),
                        points);

                // quick reduction for large values
                if (points > 20 && coinflip())
                {
                    points -= 10;
                    cost = min(1 + div_rand_round(you.experience_level, 2), 10);
                }

                points -= cost;

                if (summon_berserker(cost * 20, 0))
                    count++;
            }
        }

        if (count > 0)
        {
            if (x_chance_in_y(you.experience_level, 30) && coinflip())
            {
                if (coinflip() && !you.duration[DUR_WEAK])
                {
                    simple_god_message(" saps your strength.", god);
                    you.weaken(nullptr, 30);
                }
                else if (you.duration[DUR_SLOW] < 180 * BASELINE_DELAY)
                {
                    mprf(MSGCH_WARN, "You suddenly feel lethargic!");
                    slow_player(100);
                }
            }
        }

        simple_god_message(count > 1 ? " sends monsters to punish you." :
                           count > 0 ? " sends a monster to punish you."
                                     : " has no time to punish you... now.",
                                       god);

        if (count > 0)
            return true;
        return false;
    }

    simple_god_message("'s voice booms out, \"Feel my wrath!\"", god);
    noisy(40, you.pos());

    switch (random2(4))
    {
    case 0:
    case 1:
    case 2:
        if (you.strength() > 0)
        {
            lose_stat(STAT_STR, 1 + random2(you.max_strength() / 3));
            return true;
        }
        break;

    case 3:
        if (you.can_sleep())
        {
            mprf(MSGCH_WARN, "You pass out from exhaustion!");
            you.put_to_sleep(nullptr, 30 + random2(20));
            return true;
        }
        break;
    }
    return false;
}

static bool _beogh_retribution()
{
    // orcish theme
    const god_type god = GOD_BEOGH;

    switch (random2(8))
    {
    case 0: // smiting (25%)
    case 1:
        _god_smites_you(god);
        break;

    case 2: // send out one or two dancing weapons (12.5%)
    {
        int num_created = 0;
        int num_to_create = random_range(1, 2);

        for (int i = 0; i < num_to_create; ++i)
        {
            const int wpn_type =
                random_choose(WPN_CLUB,        WPN_MACE,      WPN_FLAIL,
                              WPN_MORNINGSTAR, WPN_DAGGER,    WPN_SHORT_SWORD,
                              WPN_LONG_SWORD,  WPN_SCIMITAR,  WPN_GREAT_SWORD,
                              WPN_HAND_AXE,    WPN_BATTLEAXE, WPN_SPEAR,
                              WPN_HALBERD);

            // Now create monster.
            if (monster *mon =
                create_monster(_wrath_mon_data(MONS_DANCING_WEAPON, god)))
            {
                ASSERT(mon->weapon() != nullptr);
                item_def& wpn(*mon->weapon());

                set_item_ego_type(wpn, SPWPN_ORC_SLAYING);

                wpn.plus  = random2(3);
                wpn.sub_type = wpn_type;

                set_ident_flags(wpn, ISFLAG_KNOW_TYPE);

                item_colour(wpn);

                ghost_demon newstats;
                newstats.init_dancing_weapon(wpn,
                                             you.experience_level * 50 / 9);

                mon->set_ghost(newstats);
                mon->ghost_demon_init();

                num_created++;
            }
        }

        if (num_created > 0)
        {
            ostringstream msg;
            msg << " throws "
                << (num_created == 1 ? "an implement" : "implements")
                << " of electrocution at you.";
            simple_god_message(msg.str().c_str(), god);
            break;
        } // else fall through
    }
    case 3: // 25%, relatively harmless
    case 4: // in effect, only for penance
        if (you_worship(god) && beogh_followers_abandon_you())
            break;
        // else fall through
    default: // send orcs after you (3/8 to 5/8)
    {
        const int points = you.experience_level + 3
                           + random2(you.experience_level * 3);

        monster_type punisher;
        // "natural" bands
        if (points >= 30) // min: lvl 7, always: lvl 27
            punisher = MONS_ORC_WARLORD;
        else if (points >= 24) // min: lvl 6, always: lvl 21
            punisher = MONS_ORC_HIGH_PRIEST;
        else if (points >= 18) // min: lvl 4, always: lvl 15
            punisher = MONS_ORC_KNIGHT;
        else if (points > 10) // min: lvl 3, always: lvl 8
            punisher = MONS_ORC_WARRIOR;
        else
            punisher = MONS_ORC;

        mgen_data temp = _wrath_mon_data(punisher, god);
        temp.flags |=  MG_PERMIT_BANDS;

        monster *mons = create_monster(temp, false);

        // sometimes name band leader
        if (mons && one_chance_in(3))
            give_monster_proper_name(*mons);

        simple_god_message(
            mons ? " sends forth an army of orcs."
                 : " is still gathering forces against you.", god);
    }
    }

    return true;
}

static bool _okawaru_retribution()
{
    // warrior theme
    const god_type god = GOD_OKAWARU;

    int how_many = 1 + (you.experience_level / 5);
    int count = 0;

    for (; how_many > 0; --how_many)
        count += _okawaru_random_servant();

    simple_god_message(count > 0 ? " sends forces against you!"
                                 : "'s forces are busy with other wars.", god);

    return true;
}

static bool _sif_muna_retribution()
{
    // magic/intelligence theme
    const god_type god = GOD_SIF_MUNA;

    simple_god_message("'s wrath finds you.", god);

    switch (random2(10))
    {
    case 0:
    case 1:
    case 2:
        lose_stat(STAT_INT, 1 + random2(you.intel() / 5));
        break;

    case 3:
    case 4:
    case 5:
        confuse_player(5 + random2(3));
        break;

    case 6:
    case 7:
    case 8:
        if (you.magic_points > 0)
        {
            dec_mp(you.magic_points);
            canned_msg(MSG_MAGIC_DRAIN);
        }
        break;

    case 9:
        // This will set all the extendable duration spells to
        // a duration of one round, thus potentially exposing
        // the player to real danger.
        you.debuff();
        break;
    }

    return true;
}

/**
 * Perform translocation-flavored Lugonu retribution.
 *
 * 50% banishment/corruption; 50% teleport near monsters.
 */
static void _lugonu_transloc_retribution()
{
    const god_type god = GOD_LUGONU;

    bool acted = false;

    if (coinflip())
    {
        if (x_chance_in_y(you.experience_level, 30))
            lugonu_corrupt_level(150 + you.experience_level * 30, true);
        else
        {
            simple_god_message(" draws you home!", god);
            you.banish(nullptr, "Lugonu's touch", you.get_experience_level(), true);
        }
        acted = true;
    }
    else if (coinflip())
    {
        you_teleport_now(false, true, "Space warps around you!");
        acted = true;
    }

    if (one_chance_in(3) && acted)
    {
        switch (random2(3))
        {
        case 0:
            mprf(MSGCH_GOD, "You're contaminated by residual distortion.");
            contaminate_player(random2avg(18000, 3));
            break;
        case 1:
            simple_god_message(" thickens space around you!", god);
            you.increase_duration(DUR_DIMENSION_ANCHOR, 20 + random2(11), 50);
            break;
        case 2:
            mprf(MSGCH_GOD, "Reality tears apart around you.");
            ouch(9 + random2avg(17, 2), KILLED_BY_DIVINE_WRATH, MID_NOBODY, "Lugonu's touch");
            for (int i = 1 + random2(4); i > 0; i++)
            {
                create_monster(_wrath_mon_data(x_chance_in_y(you.experience_level, 50) ? 
                    MONS_SPATIAL_MAELSTROM : MONS_SPATIAL_VORTEX, GOD_LUGONU), false);
            }
            break;
        }
    }
}

/**
 * Summon Lugonu's minions to punish the player.
 *
 * Possibly a major minion with pals, possibly just some riff-raff, depending
 * on level & chance.
 */
static void _lugonu_minion_retribution()
{
    // abyssal servant theme
    const god_type god = GOD_LUGONU;

    // should we summon more & higher-tier lugonu minions?
    // linear chance, from 0% at xl 4 to 80% at xl 16
    const bool major = (you.experience_level > (4 + random2(12))
                        && !one_chance_in(5));

    // how many lesser minions should we try to summon?
    // if this is major wrath, summon a few minions; 0 below xl9, 0-3 at xl 27.
    // otherwise, summon exactly (!) 1 + xl/7 minions, maxing at 4 at xl 21.
    const int how_many = (major ? random2(you.experience_level / 9 + 1)
                                : 1 + you.experience_level / 7);

    // did we successfully summon any minions? (potentially set true below)
    bool success = false;

    for (int i = 0; i < how_many; ++i)
    {
        // try to summon a few minor minions...
        // weight toward large abominations, and away from small ones, at
        // higher levels
        const monster_type to_summon =
            random_choose_weighted(
                7,                              MONS_RAIJU,
                6,                              MONS_THRASHING_HORROR,
                3,                              MONS_ANCIENT_ZYME,
                3,                              MONS_APOCALYPSE_CRAB
            );

        if (create_monster(_wrath_mon_data(to_summon, god), false))
            success = true;
    }

    if (major)
    {
        // try to summon one nasty monster.
        const monster_type to_summon = random_choose(MONS_TENTACLED_STARSPAWN,
                                                     MONS_WRETCHED_STAR,
                                                     MONS_STARCURSED_MASS);

        if (create_monster(_wrath_mon_data(to_summon, god), false))
            success = true;
    }

    simple_god_message(success ? " sends minions to punish you."
                               : "'s minions fail to arrive.", god);
}

/**
 * Call down the wrath of Lugonu upon the player!
 *
 * @return Whether to take further divine wrath actions afterward.
 */
static bool _lugonu_retribution()
{
    _lugonu_transloc_retribution();
    _lugonu_minion_retribution();

    return true;
}



/**
 * Choose a type of destruction with which to punish the player.
 *
 * @return A spell type to hurl at the player.
 */
static spell_type _vehumet_wrath_type()
{
    const int severity = min(random_range(1 + you.experience_level / 5,
                                          1 + you.experience_level / 3),
                             9);
    // Mostly player-castable conjurations with a couple of additions.
    switch (severity)
    {
        case 1:
            return random_choose(SPELL_STING,
                                 SPELL_SHOCK,
                                 SPELL_FLAME_TONGUE);
        case 2:
            return random_choose(SPELL_THROW_FLAME,
                                 SPELL_THROW_FROST);
        case 3:
            return random_choose(SPELL_MEPHITIC_CLOUD,
                                 SPELL_STONE_ARROW);
        case 4:
            return random_choose(SPELL_STICKY_FLAME,
                                 SPELL_THROW_ICICLE,
                                 SPELL_ENERGY_BOLT);
        case 5:
            return random_choose(SPELL_FIREBALL,
                                 SPELL_LIGHTNING_BOLT,
                                 SPELL_BOLT_OF_MAGMA,
                                 SPELL_VENOM_BOLT,
                                 SPELL_BOLT_OF_DRAINING,
                                 SPELL_QUICKSILVER_BOLT,
                                 SPELL_METAL_SPLINTERS);
        case 6:
            return random_choose(SPELL_BOLT_OF_FIRE,
                                 SPELL_BOLT_OF_COLD,
                                 SPELL_CORROSIVE_BOLT,
                                 SPELL_FREEZING_CLOUD,
                                 SPELL_POISONOUS_CLOUD,
                                 SPELL_POISON_ARROW,
                                 SPELL_IRON_SHOT,
                                 SPELL_CONJURE_BALL_LIGHTNING);
        case 7:
            return random_choose(SPELL_ORB_OF_ELECTRICITY,
                                 SPELL_FLASH_FREEZE);
        case 8:
            return SPELL_LEHUDIBS_CRYSTAL_SPEAR;
        case 9:
            return SPELL_FIRE_STORM;
        default:
            return SPELL_NO_SPELL;
    }
}

/**
 * Call down the wrath of Vehumpet upon the player!
 *
 * Conjuration theme.
 *
 * @return Whether to take further divine wrath actions afterward.
 */
static bool _vehumet_retribution()
{
    return spell_retribution(_vehumet_wrath_type(), GOD_VEHUMET);
}

static bool _nemelex_retribution()
{
    // card theme
    const god_type god = GOD_NEMELEX_XOBEH;

    simple_god_message(" makes you draw from the deck of Punishment.", god);
    draw_from_deck_of_punishment();
    return true;
}

/**
 * Let Jiyva throw a few malmutations the player's way.
 */
static void _jiyva_mutate_player()
{
    const god_type god = GOD_JIYVA;
    god_speaks(god, "You feel Jiyva alter your body.");

    const int mutations = 1 + random2(3);
    for (int i = 0; i < mutations; ++i)
        mutate(RANDOM_BAD_MUTATION, _god_wrath_name(god), true, false, true);
    flash_view_delay(UA_PLAYER, MAGENTA, 300);
}

static void _jiyva_remove_slime_mutation()
{
    bool slimy = false;
    for (int i = 0; i < NUM_MUTATIONS; ++i)
    {
        if (is_slime_mutation(static_cast<mutation_type>(i))
            && you.has_mutation(static_cast<mutation_type>(i)))
        {
            slimy = true;
        }
    }

    if (!slimy)
        return;

    const god_type god = GOD_JIYVA;
    simple_god_message("'s gift of slime is revoked.", god);
    delete_mutation(RANDOM_SLIME_MUTATION, _god_wrath_name(god),
                    true, false, true);
}

/**
 * Make Jiyva polymorph the player into a bad form.
 */
static void _jiyva_transform()
{
    const god_type god = GOD_JIYVA;
    god_speaks(god, "Mutagenic energy floods into your body!");

    const transformation form = random_choose(transformation::bat,
                                              transformation::fungus,
                                              transformation::pig,
                                              transformation::tree,
                                              transformation::wisp);

    if (transform(random2(you.penance[god]) * 2, form, true))
        you.transform_uncancellable = true;
}
/**
 * Make Jiyva contaminate tha player.
 */
static void _jiyva_contaminate()
{
    const god_type god = GOD_JIYVA;
    god_speaks(god, "Mutagenic energy floods into your body!");
    contaminate_player(random2(you.penance[god] * 500));
}

static void _jiyva_summon_slimes()
{
    const god_type god = GOD_JIYVA;

    const monster_type slimes[] =
    {
        MONS_FLOATING_EYE,
        MONS_EYE_OF_DRAINING,
        MONS_EYE_OF_DEVASTATION,
        MONS_GREAT_ORB_OF_EYES,
        MONS_SHINING_EYE,
        MONS_GLOWING_ORANGE_BRAIN,
        MONS_JELLY,
        MONS_ACID_BLOB,
        MONS_AZURE_JELLY,
        MONS_DEATH_OOZE,
        MONS_SLIME_CREATURE,
    };

    const int how_many = 1 + (you.experience_level / 10) + random2(3);
    bool success = false;

    for (int i = 0; i < how_many; i++)
    {
        const monster_type slime = RANDOM_ELEMENT(slimes);

        if (create_monster(_wrath_mon_data(slime, god), false))
            success = true;
    }

    god_speaks(god, success ? "Some slimes ooze up out of the ground!"
                            : "The ground quivers slightly.");
}

/**
 * Call down the wrath of Jiyva upon the player!
 *
 * Mutations and slime theme.
 *
 * @return Whether to take further divine wrath actions afterward. (true.)
 */
static bool _jiyva_retribution()
{
    const god_type god = GOD_JIYVA;

    if (you.can_safely_mutate() && one_chance_in(7))
        _jiyva_mutate_player();
    else if (one_chance_in(3) && !you.transform_uncancellable)
        _jiyva_transform();
    else if (!one_chance_in(3) || you_worship(god))
        _jiyva_contaminate();
    else
        _jiyva_summon_slimes();

    if (coinflip())
        _jiyva_remove_slime_mutation();

    return true;
}

/**
 * Let Fedhas call down the enmity of nature upon the player!
 * Equal chance corrosive bolt, primal wave (a throwback to rain),
 * or thorn volley
 */
static void _fedhas_nature_retribution()
{
    spell_type spell = random_choose(SPELL_CORROSIVE_BOLT,
                                     SPELL_PRIMAL_WAVE,
                                     SPELL_THORN_VOLLEY);

    spell_retribution(spell, GOD_FEDHAS, " invokes nature against you.");
}

static bool _fedhas_summon_elemental()
{
    bool success = false;

    mgen_data rot = _wrath_mon_data(MONS_ROT_ELEMENTAL, GOD_FEDHAS);

    for (int x = 1 + random2(you.experience_level/9); x > 0; x--)
    {
        if (create_monster(rot, false))
            success = true;
    }

    if (!success)
        return true;

    mgen_data cap = _wrath_mon_data(MONS_DEATHCAP, GOD_FEDHAS);

    for (int x = 1 + random2(you.experience_level/3); x > 0; x--)
        create_monster(cap, false);

    simple_god_message(" wants you to rot away.", GOD_FEDHAS);

    return false;
}

/**
 * Summon Fedhas's oklobs & mushrooms around the player.
 *
 * @return Whether to take further divine wrath actions afterward.
 */
static bool _fedhas_summon_plants()
{
    const god_type god = GOD_FEDHAS;
    bool success = false;

    // We are going to spawn some oklobs but first we need to find
    // out a little about the situation.
    vector<vector<coord_def> > radius_points;
    collect_radius_points(radius_points, you.pos(), LOS_NO_TRANS);

    int max_idx = 3;
    unsigned max_points = radius_points[max_idx].size();

    for (unsigned i = max_idx + 1; i < radius_points.size(); i++)
    {
        if (radius_points[i].size() > max_points)
        {
            max_points = radius_points[i].size();
            max_idx = i;
        }
    }

    mgen_data temp = _wrath_mon_data(MONS_OKLOB_PLANT, god);

    // If we have a lot of space to work with we can do something
    // flashy.
    if (radius_points[max_idx].size() > 24)
    {
        int seen_count;

        temp.cls = MONS_PLANT;

        place_ring(radius_points[0],
                   you.pos(),
                   temp,
                   1, radius_points[0].size(),
                   seen_count);

        if (seen_count > 0)
            success = true;

        temp.cls = MONS_OKLOB_PLANT;

        place_ring(radius_points[max_idx],
                   you.pos(),
                   temp,
                   random_range(3, 8), 1,
                   seen_count);

        if (seen_count > 0)
            success = true;
    }
    // Otherwise we do something with the nearest neighbors
    // (assuming the player isn't already surrounded).
    else if (!radius_points[0].empty())
    {
        unsigned target_count = random_range(2, 8);
        if (target_count < radius_points[0].size())
            prioritise_adjacent(you.pos(), radius_points[0]);
        else
            target_count = radius_points[0].size();

        for (unsigned i = radius_points[0].size() - target_count;
             i < radius_points[0].size(); ++i)
        {
            temp.pos = radius_points[0].at(i);
            temp.cls = coinflip() ? MONS_WANDERING_MUSHROOM
                                  : MONS_OKLOB_PLANT;

            if (create_monster(temp, false))
                success = true;
        }
    }

    if (success)
    {
        god_speaks(god, "Plants grow around you in an ominous manner.");
        return false;
    }

    return true;
}

/**
 * Call down the wrath of Fedhas upon the player!
 *
 * Plants and plant/nature themed attacks.
 *
 * @return Whether to take further divine wrath actions afterward.
 */
static bool _fedhas_retribution()
{
    const god_type god = GOD_FEDHAS;

    // We have 3 forms of retribution, but players under penance will be
    // spared the 'you are now surrounded by oklob plants, please die' one.
    const int retribution_options = you_worship(god) ? 2 : 4;

    switch (random2(retribution_options))
    {
    case 0:
        // Try and spawn some hostile ballistomycete spores, if none are created
        // fall through to the elemental miscast effects.
        if (fedhas_corpse_spores(BEH_HOSTILE))
        {
            simple_god_message(" produces spores.", god);
            return true;
        }

    case 1:
    default:
        _fedhas_nature_retribution();
        return true;

    case 2:
        return _fedhas_summon_elemental();

    case 3:
        return _fedhas_summon_plants();
    }
}

static bool _dithmenos_retribution()
{
    // shadow theme
    const god_type god = GOD_DITHMENOS;

    switch (random2(4))
    {
    case 0:
    {
        int count = 0;
        int how_many = 3 + random2avg(div_rand_round(you.experience_level, 3),
                                      2);
        const int tier = div_rand_round(you.experience_level, 9);
        while (how_many-- > 0)
        {
            if (_dithmenos_random_shadow(count, tier))
                count++;
        }
        simple_god_message(count ? " calls forth shadows to punish you."
                                 : " fails to incite the shadows against you.",
                           god);
        break;
    }
    case 1:
    {
        int count = 0;
        int how_many = 2 + random2avg(div_rand_round(you.experience_level, 4),
                                      4);
        for (int i = 0; i < how_many; ++i)
        {
            if (create_monster(
                    mgen_data(
                        RANDOM_MOBILE_MONSTER, BEH_HOSTILE, you.pos(), MHITYOU,
                              MG_NONE, god)
                            .set_place(level_id(BRANCH_DUNGEON,
                                                min(27,
                                                    you.experience_level + 5)))
                            .set_summoned(nullptr, 4, MON_SUMM_WRATH)
                            .set_non_actor_summoner(_god_wrath_name(god))))
            {
                count++;
            }
        }
        simple_god_message(count ? " weaves monsters from the shadows."
                                 : " fails to weave monsters from the shadows.",
                           god);
        break;
    }
    case 2:
    {
        // This is possibly kind of underwhelming?
        god_speaks(god, "You feel overwhelmed by the shadows around you.");
        you.put_to_sleep(nullptr, 30 + random2(20));
        break;
    }
    case 3:
        simple_god_message(" tears the shadows away from you.", god);
        you.sentinel_mark();
        break;
    }
    return true;
}

static const pop_entry pop_qazlal_wrath[] =
{
  {  0, 12, 25, SEMI, MONS_AIR_ELEMENTAL },
  {  4, 12, 50, FLAT, MONS_WIND_DRAKE },
  { 10, 22, 50, SEMI, MONS_SPARK_WASP },
  { 18, 27, 50, RISE, MONS_STORM_DRAGON },

  {  0, 12, 25, SEMI, MONS_FIRE_ELEMENTAL },
  {  4, 12, 50, FLAT, MONS_FIRE_CRAB },
  {  8, 16, 30, FLAT, MONS_LINDWURM },
  { 12, 27, 50, SEMI, MONS_FIRE_DRAGON },

  {  0, 12, 25, SEMI, MONS_WATER_ELEMENTAL },
  {  2, 10, 50, FLAT, MONS_RIME_DRAKE },
  { 12, 27, 50, SEMI, MONS_ICE_DRAGON },
  { 20, 27, 30, RISE, MONS_SHARD_SHRIKE },

  {  0, 12, 25, SEMI, MONS_EARTH_ELEMENTAL },
  {  2, 10, 50, FLAT, MONS_BASILISK },
  {  4, 14, 30, FLAT, MONS_CATOBLEPAS },
  { 18, 27, 50, RISE, MONS_IRON_DRAGON },
  
  {  0,  6, 12, SEMI, MONS_EARTH_ELEMENTAL },

  { 0,0,0,FLAT,MONS_0 }
};

/**
 * Summon elemental creatures to destroy the player!
 */
static void _qazlal_summon_elementals()
{
    const god_type god = GOD_QAZLAL;

    const int how_many = 1 + div_rand_round(you.experience_level, 7);
    bool success = false;

    for (int i = 0; i < how_many; i++)
    {
        monster_type mon = pick_monster_from(pop_qazlal_wrath,
                                             you.experience_level);

        if (create_monster(_wrath_mon_data(mon, god), false))
            success = true;
    }

    if (success)
        simple_god_message(" incites the elements against you!", god);
    else
        simple_god_message(" fails to incite the elements against you.", god);
}

/**
 * Give the player temporary elemental-vulnerability mutations.
 */
static void _qazlal_elemental_vulnerability()
{
    const god_type god = GOD_QAZLAL;

    if (mutate(RANDOM_QAZLAL_MUTATION, _god_wrath_name(god), false,
               false, true, false, MUTCLASS_TEMPORARY))
    {
        simple_god_message(" strips away your elemental protection.",
                           god);
    }
    else
    {
        simple_god_message(" fails to strip away your elemental protection.",
                           god);
    }
}

/**
 * Call down the wrath of Qazlal upon the player!
 *
 * Disaster/elemental theme.
 *
 * @return Whether to take further divine wrath actions afterward.
 */
static bool _qazlal_retribution()
{
    if (coinflip())
    {
        simple_god_message(" causes a mighty clap of thunder!",
                           GOD_QAZLAL);
        noisy(25, you.pos());
    }

    if (coinflip())
        _qazlal_summon_elementals();
    else
        _qazlal_elemental_vulnerability();

    return true;
}

static int _wu_jian_summon_weapons()
{
    god_type god = GOD_WU_JIAN;
    const int num = 3 + random2(3);
    int created = 0;

    for (int i = 0; i < num; ++i)
    {
        const int subtype = random_choose(WPN_DIRE_FLAIL, WPN_QUARTERSTAFF,
                                          WPN_BROAD_AXE, WPN_GREAT_SWORD,
                                          WPN_TANTO, WPN_GLAIVE);
        int ego = SPWPN_NORMAL;
        while (ego == SPWPN_NORMAL || !is_weapon_brand_ok(subtype, ego, true))
        {
            ego = random_choose(SPWPN_VORPAL, SPWPN_MOLTEN,
                                SPWPN_FREEZING, SPWPN_ELECTROCUTION,
                                SPWPN_SPEED);
        }

        if (monster *mon =
            create_monster(_wrath_mon_data(MONS_DANCING_WEAPON, god)))
        {
            ASSERT(mon->weapon() != nullptr);
            item_def& wpn(*mon->weapon());

            set_item_ego_type(wpn, ego);

            wpn.plus = random2(5);
            wpn.sub_type = subtype;

            set_ident_flags(wpn, ISFLAG_KNOW_TYPE);

            item_colour(wpn);

            ghost_demon newstats;
            newstats.init_dancing_weapon(wpn, you.experience_level * 50 / 9);

            mon->set_ghost(newstats);
            mon->ghost_demon_init();

            created++;
        }
    }

    return created;
}

static bool _wu_jian_retribution()
{
    god_type god = GOD_WU_JIAN;

    if (_wu_jian_summon_weapons())
    {
        switch (random2(4))
        {
        case 0:
            wu_jian_sifu_message(" says: Die by a thousand cuts!");
            you.set_duration(DUR_BARBS, random_range(5, 10));
            break;
        case 1:
            wu_jian_sifu_message(" whispers: Nowhere to run...");
            you.set_duration(DUR_SLOW, random_range(5, 10));
            break;
        case 2:
            wu_jian_sifu_message(" whispers: These will loosen your tongue!");
            you.increase_duration(DUR_SILENCE, 5 + random2(11), 50);
            invalidate_agrid(true);
            break;
        case 3:
            wu_jian_sifu_message(" says: Suffer, mortal!");
            you.corrode_equipment(_god_wrath_name(god).c_str(), 2);
            break;
        }
    }
    else
        simple_god_message("'s divine weapons fail to arrive.", god);

    return true;
}

static bool _uskayaw_retribution()
{
    const god_type god = GOD_USKAYAW;

    // check if we have monsters around
    monster* mon = nullptr;
    mon = choose_random_nearby_monster(0, choose_hostile_monster);

    switch (random2(5))
    {
    case 0:
    case 1:
        if (mon && mon->can_go_berserk())
        {
            simple_god_message(make_stringf(" drives %s into a dance frenzy!",
                                     mon->name(DESC_THE).c_str()).c_str(), god);
            mon->go_berserk(true);
            return true;
        }
        // else we intentionally fall through

    case 2:
    case 3:
        if (mon)
        {
            simple_god_message(" booms out, \"Time for someone else to take a solo\"",
                                    god);
            you.petrify(nullptr);
            dec_penance(god, 1);
            return false;
        }
        // else we intentionally fall through

    case 4:
        simple_god_message(" booms out: \"Revellers, it's time to dance!\"", god);
        noisy(35, you.pos());
        break;
    }
    return true;
}

static const pop_entry _bahamut_draconians[] =
{
  {  0, 12,   5, FLAT, MONS_DRACONIAN },
  {  8, 27,  10, FALL, RANDOM_BASE_DRACONIAN },
  { 15, 27,  10, RISE, RANDOM_NONBASE_DRACONIAN },
  { 0,0,0,FLAT,MONS_0 }
};

static const pop_entry _bahamut_dragons[] =
{
  {  0, 12,  10, FLAT, MONS_STEAM_DRAGON },
  {  6, 18,  10, PEAK, MONS_ACID_DRAGON },
  {  8, 24,  10, PEAK, MONS_FIRE_DRAGON },
  {  8, 24,  10, PEAK, MONS_ICE_DRAGON },
  { 10, 18,  10, RISE, MONS_HYDRA },
  { 12, 27,  20, RISE, MONS_SHADOW_DRAGON },
  { 12, 27,  20, PEAK, MONS_STORM_DRAGON },
  { 12, 27,  20, PEAK, MONS_GOLDEN_DRAGON },
  { 18, 27,  20, RISE, MONS_QUICKSILVER_DRAGON },
  { 18, 27,  10, RISE, MONS_IRON_DRAGON },
  { 21, 27,  10, RISE, MONS_BONE_DRAGON },
  { 21, 27,   5, RISE, MONS_PEARL_DRAGON },
  { 0,0,0,FLAT,MONS_0 }
};

static bool _bahamut_retribution()
{
    monster* mon = nullptr;
    mon = choose_random_nearby_monster(0, choose_hostile_monster);
    int x = random2(5);

    if (mon && coinflip())
        x = 3;

    switch (x)
    {
    case 0:
    {
        int count = 1 + you.experience_level / 9;
        bool success = false;
        for (int i = 0; i < count; i++)
        {
            if (summon_drakes(you.get_experience_level(), true))
            {
                success = true;
            }
        }
        if (success)
        {
            mprf(MSGCH_GOD, "Tiamat sends hordes of drakes to punish you.");
            return true;
        }
    }   // If fail, fallthrough to next.
    case 1:
    {
        int count = 1 + you.experience_level / 8 + random2(1 + you.experience_level / 3);
        bool success = false;

        for (int i = 0; i < count; i++)
        {
            monster_type mon_type = pick_monster_from(_bahamut_dragons,
                you.experience_level);

            mgen_data temp = _wrath_mon_data(mon_type, GOD_BAHAMUT_TIAMAT);

            if (create_monster(temp, false))
                success = true;
        }
        count = 1 + you.experience_level / 9;
        for (int i = 0; i < count; i++)
        {
            monster_type mon_type = pick_monster_from(_bahamut_draconians,
                you.experience_level);

            mgen_data temp = _wrath_mon_data(mon_type, GOD_BAHAMUT_TIAMAT);

            if (you.experience_level > (10 + random2(5)))
                temp.flags |= MG_PERMIT_BANDS;

            if (create_monster(temp, false))
                success = true;
        }
        if (success)
        {
            mprf(MSGCH_GOD, "Bahamut calls forth your kin to admonish your indiscretion.");
            return true;
        }
    }   // If fail, fallthrough to next.
    case 2:
    {
        if (mon)
        {
            if (you.props.exists(BAHAMUT_TIAMAT_CHOICE0_KEY) && you.props[BAHAMUT_TIAMAT_CHOICE0_KEY].get_bool() && !you.stasis() && !you.res_petrify())
            {
                you.petrify(nullptr);
                mprf(MSGCH_GOD, "Bahamut Booms: \"Repent and I shall protect you once more.\"");
            }
            else
            {
                monster * target = nullptr;
                for (monster_iterator mi; mi; ++mi)
                {
                    if (you.see_cell_no_trans(mi->pos()) &&
                        (!target || (mi->get_experience_level() >= target->get_experience_level()))
                        && !bool(mi->holiness() & (MH_ELEMENTAL | MH_UNDEAD | MH_CONSTRUCT)))
                    {
                        target = *mi;
                    }
                }
                if (target)
                {
                    mprf(MSGCH_GOD, "%s blood boils as Tiamat raises it to a haste.", target->name(DESC_ITS).c_str());
                    target->add_ench(ENCH_HASTE);
                }
            }
        }
    }   // If fail, fallthrough to next.
    case 3:
    {
        mprf(MSGCH_GOD, "Tiamat compels you to roar at the top of your lungs!");

        if (silenced(you.pos()))
            mpr("...but you couldn't make a sound.");

        noisy(35, you.pos(), you.mid);
        you.increase_duration(DUR_BREATH_WEAPON, you.experience_level + random2(you.experience_level), 100, "You are out of breath.");
        return true;
    }
    default:
    case 4:
    {
        if (you.drac_colour != DR_BROWN && you.penance[GOD_BAHAMUT_TIAMAT] > 6) // Don't do this at low penance since it would just revert near immediately.
        {
            mprf(MSGCH_GOD, "Bahamut sneers: \"You need to grow up!\"");
            you.attribute[ATTR_STRIPPED_COLOUR] = you.drac_colour;
            change_drac_colour(DR_BROWN);
            return true;
        }
    }
    }

    return false;
}

bool divine_retribution(god_type god, bool no_bonus, bool force)
{
    ASSERT(god != GOD_NO_GOD);

    if (is_unavailable_god(god))
        return false;

    // Good gods don't use divine retribution on their followers, and
    // gods don't use divine retribution on followers of gods they don't
    // hate.
    if (!force && ((god == you.religion && is_good_god(god))
        || (god != you.religion && !god_hates_your_god(god))))
    {
        return false;
    }

    god_acting gdact(god, true);

    bool restore_colour = ((you.attribute[ATTR_STRIPPED_COLOUR] > 0) && one_chance_in(3));

    bool do_more        = true;
    switch (god)
    {
    // One in ten chance that Xom might do something good...
    case GOD_XOM:
        xom_acts(abs(you.piety - HALF_MAX_PIETY),
                 frombool(one_chance_in(10)));
        break;
    case GOD_SHINING_ONE:       do_more = _tso_retribution(); break;
    case GOD_ZIN:               do_more = _zin_retribution(); break;
    case GOD_MAKHLEB:           do_more = _makhleb_retribution(); break;
    case GOD_KIKUBAAQUDGHA:     do_more = _kikubaaqudgha_retribution(); break;
    case GOD_YREDELEMNUL:       do_more = _yredelemnul_retribution(); break;
    case GOD_TROG:              do_more = _trog_retribution(); break;
    case GOD_BEOGH:             do_more = _beogh_retribution(); break;
    case GOD_OKAWARU:           do_more = _okawaru_retribution(); break;
    case GOD_LUGONU:            do_more = _lugonu_retribution(); break;
    case GOD_VEHUMET:           do_more = _vehumet_retribution(); break;
    case GOD_NEMELEX_XOBEH:     do_more = _nemelex_retribution(); break;
    case GOD_SIF_MUNA:          do_more = _sif_muna_retribution(); break;
    case GOD_JIYVA:             do_more = _jiyva_retribution(); break;
    case GOD_FEDHAS:            do_more = _fedhas_retribution(); break;
    case GOD_CHEIBRIADOS:       do_more = _cheibriados_retribution(); break;
    case GOD_DITHMENOS:         do_more = _dithmenos_retribution(); break;
    case GOD_QAZLAL:            do_more = _qazlal_retribution(); break;
    case GOD_USKAYAW:           do_more = _uskayaw_retribution(); break;
    case GOD_WU_JIAN:           do_more = _wu_jian_retribution(); break;
    case GOD_BAHAMUT_TIAMAT:    do_more = _bahamut_retribution(); break;

    case GOD_ASHENZARI:
    case GOD_ELYVILON:
    case GOD_GOZAG:
    case GOD_RU:
    case GOD_HEPLIAKLQANA:
#if TAG_MAJOR_VERSION == 34
    case GOD_PAKELLAS:
#endif
        // No reduction with time.
        return false;

    default:
#if defined(DEBUG_DIAGNOSTICS) || defined(DEBUG_RELIGION)
        mprf(MSGCH_DIAGNOSTICS, "No retribution defined for %s.",
             god_name(god).c_str());
#endif
        return false;
    }

    if (restore_colour)
    {
        mprf(MSGCH_INTRINSIC_GAIN, "You feel yourself maturing once more...");
        change_drac_colour((draconian_colour)you.attribute[ATTR_STRIPPED_COLOUR]);
        you.attribute[ATTR_STRIPPED_COLOUR] = 0;
    }

    if (no_bonus)
        return true;

    // Sometimes divine experiences are overwhelming...
    if (do_more && one_chance_in(5) && you.experience_level < random2(37))
    {
        if (coinflip())
        {
            if (!you.confused())
            {
                mprf(MSGCH_WARN, "The divine experience confuses you!");
                confuse_player(5 + random2(3));
            }
        }
        else
        {
            if (you.duration[DUR_SLOW] < 180 * BASELINE_DELAY)
            {
                mprf(MSGCH_WARN, "The divine experience drains your vigour!");

                slow_player(random2(20));
            }
        }
    }

    // Just the thought of retribution mollifies the god by at least a
    // point...the punishment might have reduced penance further.
    dec_penance(god, 1 + random2(3));

    return true;
}

// Currently only used when orcish idols have been destroyed.
static string _get_beogh_speech(const string &key)
{
    string result = getSpeakString("Beogh " + key);

    if (result.empty())
        return "Beogh is angry!";

    return result;
}

// Destroying orcish idols (a.k.a. idols of Beogh) may anger Beogh.
void beogh_idol_revenge()
{
    god_acting gdact(GOD_BEOGH, true);

    // Beogh watches his charges closely, but for others doesn't always
    // notice.
    if (!you_worship(GOD_BEOGH)
        && (!species_is_orcish(you.species) || coinflip())
        && x_chance_in_y(2, 3))
    {
        return;
    }

    string revenge;

    if (you_worship(GOD_BEOGH))
        revenge = _get_beogh_speech("idol follower");
    else if (species_is_orcish(you.species))
        revenge = _get_beogh_speech("idol orc");
    else
        revenge = _get_beogh_speech("idol other");

    _god_smites_you(GOD_BEOGH, revenge.c_str());
}

static void _tso_blasts_cleansing_flame(const char *message)
{
    // If there's a message, display it before firing.
    if (message)
        god_speaks(GOD_SHINING_ONE, message);

    simple_god_message(" blasts you with cleansing flame!",
                       GOD_SHINING_ONE);

    // damage is 2d(pow), *3/2 for undead and demonspawn
    cleansing_flame(5 + (you.experience_level * 7) / 12,
                    cleansing_flame_source::tso, you.pos());
}

static void _god_smites_you(god_type god, const char *message,
                            kill_method_type death_type)
{
    ASSERT(god != GOD_NO_GOD);

    if (death_type == NUM_KILLBY)
    {
        switch (god)
        {
            case GOD_BEOGH:     death_type = KILLED_BY_BEOGH_SMITING; break;
            case GOD_SHINING_ONE: death_type = KILLED_BY_TSO_SMITING; break;
            default:            death_type = KILLED_BY_DIVINE_WRATH;  break;
        }
    }

    string aux;

    if (death_type != KILLED_BY_BEOGH_SMITING
        && death_type != KILLED_BY_TSO_SMITING)
    {
        aux = "smitten by " + god_name(god);
    }

    // If there's a message, display it before smiting.
    if (message)
        god_speaks(god, message);

    int divine_hurt = 10 + random2(10);

    for (int i = 0; i < 5; ++i)
        divine_hurt += random2(you.experience_level);

    simple_god_message(" smites you!", god);
    ouch(divine_hurt, death_type, MID_NOBODY, aux.c_str());
}

void reduce_xp_penance(god_type god, int amount)
{
    if (!you.penance[god] || !you.exp_docked_total[god])
        return;

    int lost = min(amount / 2, you.exp_docked[god]);
    you.exp_docked[god] -= lost;

    int new_pen = (((int64_t)you.exp_docked[god] * 50)
                   + you.exp_docked_total[god] - 1)
                / you.exp_docked_total[god];
    if (new_pen < you.penance[god])
        dec_penance(god, you.penance[god] - new_pen);
}

/// Let Gozag's wrath buff newly-seen hostile monsters, maybe.
void maybe_gozag_incite(vector<monster*> monsters)
{
    if (!player_under_penance(GOD_GOZAG))
        return;

    counted_monster_list mon_count;
    vector<monster *> incited;
    for (monster* mon : monsters)
    {
        // XXX: some of this is probably redundant with interrupt_activity
        if (!mon->see_cell(you.pos()) // xray_vision
            || mon->wont_attack()
            || mon->is_stationary()
            || mons_is_object(mon->type)
            || mons_is_tentacle_or_tentacle_segment(mon->type))
        {
            continue;
        }

        if (coinflip()
            && mon->get_experience_level() >= random2(you.experience_level))
        {
            mon_count.add(mon);
            incited.push_back(mon);
        }
    }

    if (incited.empty())
        return;

    string msg = make_stringf("%s incites %s against you.",
        god_name(GOD_GOZAG).c_str(),
        mon_count.describe().c_str());
    if (strwidth(msg) >= get_number_of_cols() - 2)
    {
        msg = make_stringf("%s incites your enemies against you.",
            god_name(GOD_GOZAG).c_str());
    }
    mprf(MSGCH_GOD, GOD_GOZAG, "%s", msg.c_str());

    for (monster *mon : incited)
        gozag_incite(mon);
}

void gozag_incite(monster *mon)
{
    ASSERT(!mon->wont_attack());

    behaviour_event(mon, ME_ALERT, &you);

    bool success = false;

    if (mon->needs_berserk(true, true))
    {
        mon->go_berserk(true);
        success = true;
    }
    else if (!mon->has_ench(ENCH_HASTE))
    {
        enchant_actor_with_flavour(mon, mon, BEAM_HASTE);
        success = true;
    }

    if (success)
    {
        mon->add_ench(ENCH_GOZAG_INCITE);
        view_update_at(mon->pos());
    }
}
