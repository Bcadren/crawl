/**
 * @file
 * @brief Summoning spells and other effects creating monsters.
**/

#include "AppHdr.h"

#include "spl-summoning.h"

#include <algorithm>
#include <cmath>

#include "act-iter.h"
#include "areas.h"
#include "artefact.h"
#include "butcher.h"
#include "cloud.h"
#include "colour.h"
#include "coordit.h"
#include "database.h"
#include "delay.h"
#include "directn.h"
#include "dungeon.h"
#include "english.h"
#include "env.h"
#include "fight.h"
#include "fprop.h"
#include "food.h"
#include "god-abil.h"
#include "god-companions.h"
#include "god-conduct.h"
#include "god-item.h"
#include "god-passive.h"
#include "invent.h"
#include "item-prop.h"
#include "item-status-flag-type.h"
#include "items.h"
#include "libutil.h"
#include "mapmark.h"
#include "message.h"
#include "mgen-data.h"
#include "mon-abil.h"
#include "mon-act.h"
#include "mon-behv.h"
#include "mon-book.h" // MON_SPELL_WIZARD
#include "mon-cast.h"
#include "mon-death.h"
#include "mon-movetarget.h"
#include "mon-place.h"
#include "mon-speak.h"
#include "player-equip.h"
#include "player-stats.h"
#include "prompt.h"
#include "religion.h"
#include "shout.h"
#include "spl-util.h"
#include "spl-wpnench.h"
#include "spl-zap.h"
#include "state.h"
#include "stepdown.h"
#include "stringutil.h"
#include "target.h"
#include "teleport.h"
#include "terrain.h"
#include "timed-effects.h"
#include "unwind.h"
#include "viewchar.h"
#include "xom.h"

static void _monster_greeting(monster *mons, const string &key)
{
    string msg = getSpeakString(key);
    if (msg == "__NONE")
        msg.clear();
    mons_speaks_msg(mons, msg, MSGCH_TALK, silenced(mons->pos()));
}

static mgen_data _summon_data(const actor &caster, monster_type mtyp,
                              int dur, god_type god, spell_type spell)
{
    return mgen_data(mtyp, BEH_COPY, caster.pos(),
                     caster.is_player() ? int{MHITYOU}
                                        : caster.as_monster()->foe,
                     MG_AUTOFOE)
                     .set_summoned(&caster, dur, spell, god);
}

static mgen_data _pal_data(monster_type pal, int dur, god_type god,
                           spell_type spell)
{
    return _summon_data(you, pal, dur, god, spell);
}

static void _scale_summon_duration(spell_type spell, monster * mon, int power, bool summoned)
{
    const int min_pow = spell_difficulty(spell) * 10;

    mon_enchant abj = mon->get_ench(summoned ? ENCH_ABJ : ENCH_FAKE_ABJURATION);

    if (!abj.ench)
    {
        if (summoned)
            mprf(MSGCH_ERROR, "Error. Wrong summon type scale.");
        if (!summoned && !have_passive(passive_t::extend_undead))
            mprf(MSGCH_ERROR, "Error. Indefinite undead without Kiku Boost.");
        return;
    }

    int dur = abj.duration;

    if (power < min_pow)
        dur /= 3;

    dur *= power + 10;
    dur /= min_pow + 10;

    dur = min(dur, abj.duration * 3); // Lets not go nuts with low level spells.

    abj.duration = dur;
    mon->update_ench(abj);
}

void player_post_summon_adjustments(spell_type spell, monster * mon, int power, int used)
{
    apply_summon_xp_bonus(spell, mon, used);

    if (power == 0)
        power = calc_spell_power(spell, true);

    bool summoned = true;
    switch (spell)
    {
    case SPELL_INFESTATION:
    case SPELL_ANIMATE_DEAD:
    case SPELL_SKELETAL_UPRISING:
    case SPELL_SIMULACRUM:
        summoned = false;
    default:
        break;
    }

    _scale_summon_duration(spell, mon, power, summoned);
    chaos_summon(spell, mon, &you, summoned);
}

void apply_summon_xp_bonus(spell_type spell, monster * mon, int used)
{
    int bonus = 0;
    for (player::summon_xp xp : you.summon_xp_data)
    {
        if (xp.summon == spell)
        {
            bonus = xp.bonus;
            break;
        }
    }

    bonus -= used;

    if (bonus > 0)
    {
        mon->set_hit_dice(mon->get_hit_dice() + bonus);
        mon->scale_hp(10 + bonus, 10);
    }
}

void chaos_summon(spell_type spell, monster * mon, actor * caster, bool summoned)
{
    if (!mon || !caster || !caster->alive()) // Sanity.
        return;

    if (determine_chaos(caster, spell))
    {
        if (one_chance_in(8) && !(mon->holiness() & (MH_UNDEAD | MH_ELEMENTAL | MH_CONSTRUCT)))
            mon->add_ench(mon_enchant(coinflip() ? ENCH_GLOWING_SHAPESHIFTER 
                                                 : ENCH_SHAPESHIFTER, 0, caster, INFINITE_DURATION));

        else if (!summoned && one_chance_in(3))
            mon->add_ench(mon_enchant(ENCH_ENTROPIC_BURST, 0, caster, INFINITE_DURATION));

        else
            mon->add_ench(mon_enchant(ENCH_CHAOTIC_INFUSION, 0, caster, INFINITE_DURATION));
    }
}

spret cast_summon_butterflies(int pow, god_type god, bool fail)
{
    if (otr_stop_summoning_prompt())
        return spret::abort;

    fail_check();
    bool success = false;

    const int how_many = min(8, 3 + random2(3) + random2(pow) / 10);

    for (int i = 0; i < how_many; ++i)
    {
        if (create_monster(_pal_data(MONS_BUTTERFLY, 3, god,
                                     SPELL_SUMMON_BUTTERFLIES)))
        {
            success = true;
        }
    }

    if (!success)
        canned_msg(MSG_NOTHING_HAPPENS);

    return spret::success;
}

static bool _change_type(monster_type & type, monster_type changed, int bonus, int min, int max, int & used)
{
    if (bonus >= min && x_chance_in_y(bonus - min + 1, max - min + 1) && !god_hates_monster(changed))
    {
        used = min - coinflip();
        type = changed;
        return true;
    }
    return false;
}

spret cast_summon_small_mammal(int pow, god_type god, bool fail)
{
    if (otr_stop_summoning_prompt())
        return spret::abort;

    fail_check();

    monster_type mon = MONS_PROGRAM_BUG;
    int used = 0;

    const int bonus = you.lookup_spell_xp_bonus(SPELL_SUMMON_SMALL_MAMMAL);

    switch (random2(4))
    {
    default:
    case 0:
        mon = MONS_QUOKKA;
        _change_type(mon, MONS_PORCUPINE, bonus, 5, 7, used);
        break;
    
    case 1:
    case 2:
        mon = MONS_RAT;
        if (!_change_type(mon, MONS_HELL_RAT, bonus, 6, 9, used))
            _change_type(mon, MONS_SEWER_RAT, bonus, 3, 5, used);
        break;

    case 3:
        mon = MONS_BAT;
        if (!_change_type(mon, MONS_FIRE_BAT, bonus, 4, 7, used))
            _change_type(mon, MONS_VAMPIRE_BAT, bonus, 2, 4, used);
        break;
    }

    if (monster * m = create_monster(_pal_data(mon, 3, god, SPELL_SUMMON_SMALL_MAMMAL)))
        player_post_summon_adjustments(SPELL_SUMMON_SMALL_MAMMAL, m, pow, used);
    else
        canned_msg(MSG_NOTHING_HAPPENS);

    return spret::success;
}

spret cast_call_lost_souls(int pow, god_type god, bool fail)
{
    fail_check();

    int count = 1 + div_rand_round(pow, 50);
    count += random2(div_round_up(pow, 25));
    bool any_summoned = false;

    for (int i = 0; i < count; ++i)
    {
        monster * soul = create_monster(_pal_data(MONS_LOST_SOUL, 3, god, SPELL_CALL_LOST_SOULS));
        if (soul)
        {
            any_summoned = true;
            soul->max_hit_points = soul->hit_points = div_rand_round(soul->max_hit_points * pow, 80);
        }
    }

    if (!any_summoned)
    {
        mprf(MSGCH_WARN, "There was no room to summon the spirits!");
        return spret::fail;
    }

    return spret::success;
}

spret cast_sticks_to_snakes(int pow, god_type god, bool fail)
{
    if (otr_stop_summoning_prompt("create snakes"))
        return spret::abort;

    int how_many_max = 1;
    int fence = random2(100); // Breakpoint for Random2.

    if (random2(pow) > fence)
        how_many_max++;

    int count = 0;

    fail_check();
    for (int i = 0; i < how_many_max; i++)
    {
        monster_type mon = MONS_BALL_PYTHON;
        const int bonus = you.lookup_spell_xp_bonus(SPELL_STICKS_TO_SNAKES);
        int used = 0;

        if (coinflip()) // Constrictors
        {
            if (!_change_type(mon, MONS_BLOOD_BOA, bonus, 8, 11, used))
                _change_type(mon, MONS_ANACONDA, bonus, 4, 9, used);
        }
        else // Venomous
        {
            mon = MONS_ADDER;

            if (!_change_type(mon, MONS_SEA_SNAKE, bonus, 7, 9, used)
                && !_change_type(mon, MONS_BLACK_MAMBA, bonus, 5, 8, used))
            {
                _change_type(mon, MONS_WATER_MOCCASIN, bonus, 3, 5, used);
            }
        }


        if (monster * m = create_monster(_pal_data(mon, 3, god, SPELL_STICKS_TO_SNAKES), false))
        {
            player_post_summon_adjustments(SPELL_STICKS_TO_SNAKES, m, pow, used);
            count++;
        }
    }
    if (count)
    {
        if (count > 1)
            mpr("You create a pair of snakes!");
        else
            mpr("You create a snake!");
    }
    else
        mpr("You fail to create any snakes.");

    return spret::success;
}

#define DOG_FLIP "canine_familiar_type"

spret cast_call_canine_familiar(int pow, god_type god, bool fail)
{
    if (otr_stop_summoning_prompt())
        return spret::abort;

    fail_check();
    monster_type mon = MONS_JACKAL;
    int used = 0;
    
    if (!you.props.exists(DOG_FLIP))
        you.props[DOG_FLIP] = true;

    const int bonus = you.lookup_spell_xp_bonus(SPELL_CALL_CANINE_FAMILIAR);
    const bool flip = you.props[DOG_FLIP].get_bool();

    switch (bonus)
    {
    case 9:
    case 8:
    case 7:
        if (flip || god_hates_monster(MONS_HELL_HOUND))
        {
            used = 6;
            mon = MONS_RAIJU;
            break;
        }
        used = 5;
        mon = MONS_HELL_HOUND;
        break;
    case 6:
    case 5:
        if (flip)
            mon = MONS_WAR_DOG;
        else
            mon = MONS_WARG;
        used = 4;
        break;
    case 4:
    case 3:
        mon = MONS_WOLF;
        used = 2;
        break;
    case 2:
    case 1:
        mon = MONS_HOUND;
    default:
        break;
    }
    
    you.props[DOG_FLIP] = !you.props[DOG_FLIP];

    const int dur = min(2 + (random2(pow) / 4), 6);

    if (monster * m = create_monster(_pal_data(mon, dur, god, SPELL_CALL_CANINE_FAMILIAR)))
        player_post_summon_adjustments(SPELL_CALL_CANINE_FAMILIAR, m, pow, used); 
    else
        canned_msg(MSG_NOTHING_HAPPENS);

    return spret::success;
}

spret cast_summon_ice_beast(int pow, god_type god, bool fail)
{
    fail_check();
    const int dur = min(2 + (random2(pow) / 4), 4);

    mgen_data ice_beast = _pal_data(MONS_ICE_BEAST, dur, god,
                                    SPELL_SUMMON_ICE_BEAST);
    ice_beast.hd = (3 + div_rand_round(pow, 13));

    if (monster * mon = create_monster(ice_beast))
    {
        player_post_summon_adjustments(SPELL_SUMMON_ICE_BEAST, mon, pow);
        mpr("A chill wind blows around you.");
    }
    else
        canned_msg(MSG_NOTHING_HAPPENS);

    return spret::success;
}

// BCADNOTE: Not sure what to do with this one.
spret cast_monstrous_menagerie(actor* caster, int pow, god_type god, bool fail)
{
    if (caster->is_player() && otr_stop_summoning_prompt())
        return spret::abort;

    fail_check();
    monster_type type = MONS_PROGRAM_BUG;

    if (random2(pow) > 60 && coinflip())
        type = MONS_SPHINX;
    else
        type = random_choose(MONS_HARPY, MONS_MANTICORE, MONS_LINDWURM);

    int num = (type == MONS_HARPY ? 1 + x_chance_in_y(pow, 80)
                                      + x_chance_in_y(pow - 75, 100)
                                  : 1);
    const bool plural = (num > 1);

    mgen_data mdata = _summon_data(*caster, type, 4, god,
                                   SPELL_MONSTROUS_MENAGERIE);
    mdata.flags |= MG_DONT_CAP;
    if (caster->is_player())
        mdata.hd = get_monster_data(type)->HD + div_rand_round(pow - 50, 25);

    bool seen = false;
    bool first = true;
    int mid = -1;
    while (num-- > 0)
    {
        if (monster* beast = create_monster(mdata))
        {
            if (you.can_see(*beast))
                seen = true;

            if (caster->is_player())
                apply_summon_xp_bonus(SPELL_MONSTROUS_MENAGERIE, beast);
            chaos_summon(SPELL_MONSTROUS_MENAGERIE, beast, caster);

            // Link the harpies together as one entity as far as the summon
            // cap is concerned.
            if (type == MONS_HARPY)
            {
                if (mid == -1)
                    mid = beast->mid;

                beast->props["summon_id"].get_int() = mid;
            }

            // Handle cap only for the first of the batch being summoned
            if (first)
                summoned_monster(beast, &you, SPELL_MONSTROUS_MENAGERIE);

            first = false;
        }
    }

    if (seen)
    {
        mprf("%s %s %s %s!", caster->name(DESC_THE).c_str(),
                             caster->conj_verb("summon").c_str(),
                             plural ? "some" : "a",
                             plural ? pluralise_monster(mons_type_name(type, DESC_PLAIN)).c_str()
                                    : mons_type_name(type, DESC_PLAIN).c_str());
    }
    else
        canned_msg(MSG_NOTHING_HAPPENS);

    return spret::success;
}

// Monster only spell these days.
spret cast_summon_hydra(actor *caster, int pow, god_type god, bool fail)
{
    fail_check();
    // Power determines number of heads. Minimum 4 heads, maximum 12.
    // Rare to get more than 8.
    const int maxheads = one_chance_in(6) ? 12 : 8;
    const int heads = max(4, min(random2(pow) / 6, maxheads));

    // Duration is always very short - just 1.
    mgen_data mg = _summon_data(*caster, MONS_HYDRA, 1, god,
                                SPELL_SUMMON_HYDRA);
    mg.props[MGEN_NUM_HEADS] = heads;
    if (monster *hydra = create_monster(mg))
    {
        chaos_summon(SPELL_SUMMON_HYDRA, hydra, caster);
        if (you.see_cell(hydra->pos()))
            mprf("%s appears.", hydra->name(DESC_A).c_str());
    }
    else if (caster->is_player())
        canned_msg(MSG_NOTHING_HAPPENS);

    return spret::success;
}

static monster_type _choose_dragon_type(int pow, god_type god)
{
    monster_type mon = MONS_NO_MONSTER;

    int chance = random2(pow);

    if (chance >= 80 || one_chance_in(6))
        mon = random_choose(MONS_BONE_DRAGON, MONS_GOLDEN_DRAGON, MONS_QUICKSILVER_DRAGON);
    else if (chance >= 40 || one_chance_in(6))
        mon = random_choose(MONS_IRON_DRAGON, MONS_SHADOW_DRAGON, MONS_STORM_DRAGON);
    else if (chance >= 20 || one_chance_in(6) || pow > 100)
        mon = random_choose(MONS_FIRE_DRAGON, MONS_ICE_DRAGON);
    else
        mon = random_choose(MONS_ACID_DRAGON, MONS_STEAM_DRAGON);

    if (god_hates_monster(mon, god))
    {
        if (is_good_god(god) && coinflip())
            mon = MONS_PEARL_DRAGON;
        else
            mon = random_choose(MONS_STORM_DRAGON, MONS_IRON_DRAGON);
    }

    return mon;
}

spret cast_dragon_call(int pow, bool fail)
{
    if (otr_stop_summoning_prompt("call dragons"))
        return spret::abort;

    fail_check();

    mpr("You call out to the draconic realm, and the dragon horde roars back!");
    noisy(spell_effect_noise(SPELL_DRAGON_CALL), you.pos());

    you.duration[DUR_DRAGON_CALL] = (15 + pow / 5 + random2(15)) * BASELINE_DELAY;

    return spret::success;
}

static void _place_dragon()
{
    const int pow = calc_spell_power(SPELL_DRAGON_CALL, true);
    const int bonus = you.lookup_spell_xp_bonus(SPELL_DRAGON_CALL);
    const int mp_cost = you.species == SP_FAIRY ? 1 : random_range(2, 3);
    
    int used = 0;
    monster_type mon = MONS_ACID_DRAGON;

    monster_type tier1 = random_choose(MONS_BONE_DRAGON, MONS_GOLDEN_DRAGON, MONS_QUICKSILVER_DRAGON);
    monster_type tier2 = random_choose(MONS_IRON_DRAGON, MONS_SHADOW_DRAGON, MONS_STORM_DRAGON);
    monster_type tier3 = random_choose(MONS_FIRE_DRAGON, MONS_ICE_DRAGON);

    if (is_good_god(you.religion))
    {
        if (tier1 == MONS_BONE_DRAGON)
            tier1 = MONS_PEARL_DRAGON;
        if (tier2 == MONS_SHADOW_DRAGON)
            tier2 = MONS_PEARL_DRAGON;
    }

    if (you.religion == GOD_DITHMENOS)
        tier3 = MONS_ICE_DRAGON;

    if (one_chance_in(5))
    {
        switch (random2(4))
        {
        case 0:
            mon = tier1;
            used = min(bonus, 6);
            break;
        case 1:
            mon = tier2;
            used = min(bonus, 3);
            break;
        case 2:
            mon = tier3;
            break;
        case 3:
            break;
        }
    }
    else if (!_change_type(mon, tier1, bonus, 6, 11, used)
        && !_change_type(mon, tier2, bonus, 3, 8, used))
    {
        _change_type(mon, tier3, bonus, 0, 5, used);
    }

    vector<monster*> targets;

    // Pick a random hostile in sight
    for (monster_near_iterator mi(&you, LOS_NO_TRANS); mi; ++mi)
        if (!mons_aligned(&you, *mi) && mons_is_threatening(**mi))
            targets.push_back(*mi);

    shuffle_array(targets);

    // Attempt to place adjacent to the first chosen hostile. If there is no
    // valid spot, move on to the next one.
    for (monster *target : targets)
    {
        // Chose a random viable adjacent spot to the select target
        vector<coord_def> spots;
        for (adjacent_iterator ai(target->pos()); ai; ++ai)
        {
            if (monster_habitable_grid(MONS_FIRE_DRAGON, env.grid(*ai))
                && !actor_at(*ai))
            {
                spots.push_back(*ai);
            }
        }

        // Now try to create the actual dragon
        if (spots.size() <= 0)
            continue;

        // Abort if we lack sufficient MP, but the dragon call duration
        // remains, as the player might soon have enough again.
        if (!enough_mp(mp_cost, true))
        {
            mpr("A dragon tries to answer your call, but you don't have enough "
                "magical power!");
            return;
        }

        const coord_def pos = spots[random2(spots.size())];
        monster *dragon = create_monster(
            mgen_data(mon, BEH_COPY, pos, MHITYOU, MG_FORCE_PLACE | MG_AUTOFOE)
            .set_summoned(&you, 2, SPELL_DRAGON_CALL));
        if (!dragon)
            continue;

        player_post_summon_adjustments(SPELL_DRAGON_CALL, dragon, pow, used);

        dec_mp(mp_cost);
        if (you.see_cell(dragon->pos()))
            mpr("A dragon arrives to answer your call!");

        // The dragon is allowed to act immediately here
        dragon->flags &= ~MF_JUST_SUMMONED;
        return;
    }

    return;
}

void do_dragon_call(int time)
{
    noisy(spell_effect_noise(SPELL_DRAGON_CALL), you.pos());

    while (time > you.attribute[ATTR_NEXT_DRAGON_TIME]
           && you.duration[DUR_DRAGON_CALL])
    {
        time -= you.attribute[ATTR_NEXT_DRAGON_TIME];
        _place_dragon();
        you.attribute[ATTR_NEXT_DRAGON_TIME] = 3 + random2(5)
                                               + count_summons(&you, SPELL_DRAGON_CALL) * 5;
    }
    you.attribute[ATTR_NEXT_DRAGON_TIME] -= time;
}

/**
 * Handle the Doom Howl status effect, possibly summoning hostile nasties
 * around the player.
 *
 * @param time      The number of aut that the howling has been going on for
 *                  since the last doom_howl call.
 */
void doom_howl(int time)
{
    // TODO: pull hound-count generation into a helper function
    int howlcalled_count = 0;
    if (!you.props.exists(NEXT_DOOM_HOUND_KEY))
        you.props[NEXT_DOOM_HOUND_KEY] = random_range(20, 40);
    // 1 nasty beast every 2-4 turns
    while (time > 0)
    {
        const int time_to_call = you.props[NEXT_DOOM_HOUND_KEY].get_int();
        if (time_to_call <= time)
        {
            you.props[NEXT_DOOM_HOUND_KEY] = random_range(20, 40);
            ++howlcalled_count;
        }
        else
            you.props[NEXT_DOOM_HOUND_KEY].get_int() -= time;
        time -= time_to_call;
    }

    if (!howlcalled_count)
        return;

    const actor *target = &you;

    for (int i = 0; i < howlcalled_count; ++i)
    {
        const monster_type howlcalled = random_choose(
                MONS_BONE_DRAGON, MONS_SHADOW_DRAGON, MONS_CYSGODOL,
                MONS_REAPER, MONS_POENYDIO, MONS_TZITZIMITL
        );
        vector<coord_def> spots;
        for (adjacent_iterator ai(target->pos()); ai; ++ai)
        {
            if (monster_habitable_grid(howlcalled, env.grid(*ai))
                && !actor_at(*ai))
            {
                spots.push_back(*ai);
            }
        }
        if (spots.size() <= 0)
            continue;

        const coord_def pos = spots[random2(spots.size())];

        monster *mons = create_monster(mgen_data(howlcalled, BEH_HOSTILE,
                                                 pos, target->mindex(),
                                                 MG_FORCE_BEH));
        if (mons)
        {
            mons->add_ench(mon_enchant(ENCH_HAUNTING, 1, target,
                                       INFINITE_DURATION));
            mons->behaviour = BEH_SEEK;
            mons_add_blame(mons, "called by a doom hound"); // assumption!
            check_place_cloud(CLOUD_BLACK_SMOKE, mons->pos(),
                              random_range(1,2), mons);
        }
    }
}

void create_vortices(actor * caster)
{
    const int num = 3 + random2(5);

    int count = 0;

    for (int i = 0; i < num; i++)
    {
        coord_def pos = find_gateway_location(caster);

        if (pos != coord_def(0, 0))
        {
            monster * x = create_monster(mgen_data(MONS_CHAOS_VORTEX, caster->is_player() ? BEH_FRIENDLY 
                                            : SAME_ATTITUDE(caster->as_monster()), pos, MHITYOU));
            if (x)
            {
                x->add_ench(mon_enchant(ENCH_FAKE_ABJURATION, 0, 0, (5 + random2(10)) * BASELINE_DELAY));
                count++;
            }
        }
    }

    if (count)
        mprf("The chaotic winds form %schaos vort%s.", count > 1 ? "" : "a ", count > 1 ? "ices" : "ex");
}

spret cast_summon_dragon(actor *caster, int pow, god_type god, bool fail)
{
    // Dragons are always friendly. Dragon type depends on power and
    // random chance, with two low-tier dragons possible at high power.
    // Duration fixed at 6.

    fail_check();
    bool success = false;

    if (god == GOD_NO_GOD)
        god = caster->deity();

    int how_many = 1;
    monster_type mon = _choose_dragon_type(pow, god);

    if (pow >= 100 && (mon == MONS_FIRE_DRAGON || mon == MONS_ICE_DRAGON))
        how_many = 2;

    for (int i = 0; i < how_many; ++i)
    {
        if (monster *dragon = create_monster(
                _summon_data(*caster, mon, 6, god, SPELL_SUMMON_DRAGON)))
        {
            if (you.see_cell(dragon->pos()))
                mpr("A dragon appears.");
            chaos_summon(SPELL_SUMMON_DRAGON, dragon, caster);
            success = true;
        }
    }

    if (!success && caster->is_player())
        canned_msg(MSG_NOTHING_HAPPENS);

    return spret::success;
}

spret cast_summon_mana_viper(int pow, god_type god, bool fail)
{
    fail_check();

    const int bonus = you.lookup_spell_xp_bonus(SPELL_SUMMON_MANA_VIPER);
    monster_type mon = MONS_MANA_VIPER;
    int used = 0;

    if (one_chance_in(3))
        mon = MONS_MANA_VIPER;
    else if (!_change_type(mon, MONS_MANA_COBRA, bonus, 3, 7, used))
        mon = MONS_MANA_VIPER;
    
    if (one_chance_in(5))
    {
        mon = MONS_MANA_COBRA;
        used = 0;
    }

    mgen_data viper = _pal_data(mon, 2, god, SPELL_SUMMON_MANA_VIPER);
    viper.hd = (5 + div_rand_round(pow, 12));

    // Don't scale hp at the same time as their antimagic power
    viper.hp = hit_points(495); // avg 50

    if (monster * vip = create_monster(viper))
    {
        player_post_summon_adjustments(SPELL_SUMMON_MANA_VIPER, vip, pow, used);
        if (mon == MONS_MANA_VIPER)
            mpr("A mana viper appears with a sibilant hiss.");
        else
            mpr("A mana cobra appears with a sinister snarl.");
    }
    else
        canned_msg(MSG_NOTHING_HAPPENS);

    return spret::success;
}

// This assumes that the specified monster can go berserk.
static void _make_mons_berserk_summon(monster* mon)
{
    mon->go_berserk(false);
    mon_enchant berserk = mon->get_ench(ENCH_BERSERK);
    mon_enchant abj = mon->get_ench(ENCH_ABJ);

    // Let Trog's gifts berserk longer, and set the abjuration timeout
    // to the berserk timeout.
    berserk.duration = berserk.duration * 3 / 2;
    berserk.maxduration = berserk.duration;
    abj.duration = abj.maxduration = berserk.duration;
    mon->update_ench(berserk);
    mon->update_ench(abj);
}

// This is actually one of Trog's wrath effects.
bool summon_berserker(int pow, actor *caster, monster_type override_mons)
{
    monster_type mon = MONS_PROGRAM_BUG;

    const int dur = min(2 + (random2(pow) / 4), 6);

    if (override_mons != MONS_PROGRAM_BUG)
        mon = override_mons;
    else
    {
        if (pow <= 100)
        {
            // bears
            mon = random_choose(MONS_BLACK_BEAR, MONS_POLAR_BEAR);
        }
        else if (pow <= 140)
        {
            // ogres
            mon = random_choose_weighted(1, MONS_TWO_HEADED_OGRE, 2, MONS_OGRE);
        }
        else if (pow <= 180)
        {
            // trolls
            mon = random_choose_weighted(3, MONS_TROLL,
                                         3, MONS_DEEP_TROLL,
                                         2, MONS_IRON_TROLL);
        }
        else
        {
            // giants
            mon = random_choose(MONS_CYCLOPS, MONS_STONE_GIANT);
        }
    }

    mgen_data mg(mon, caster ? BEH_COPY : BEH_HOSTILE,
                 caster ? caster->pos() : you.pos(),
                 (caster && caster->is_monster()) ? ((monster*)caster)->foe
                                                  : int{MHITYOU},
                 MG_AUTOFOE);
    mg.set_summoned(caster, caster ? dur : 0, SPELL_NO_SPELL, GOD_TROG);

    if (!caster)
    {
        mg.non_actor_summoner = "the rage of " + god_name(GOD_TROG);
        mg.extra_flags |= (MF_NO_REWARD | MF_HARD_RESET);
    }

    monster *mons = create_monster(mg);

    if (!mons)
        return false;

    _make_mons_berserk_summon(mons);
    return true;
}

// Not a spell. Rather, this is TSO's doing.
bool summon_holy_warrior(int pow, bool punish)
{
    mgen_data mg(random_choose(MONS_ANGEL, MONS_DAEVA),
                 punish ? BEH_HOSTILE : BEH_FRIENDLY,
                 you.pos(), MHITYOU, MG_FORCE_BEH | MG_AUTOFOE);
    mg.set_summoned(punish ? 0 : &you,
                    punish ? 0 : min(2 + (random2(pow) / 4), 6),
                    SPELL_NO_SPELL, GOD_SHINING_ONE);

    if (punish)
    {
        mg.extra_flags |= (MF_NO_REWARD | MF_HARD_RESET);
        mg.non_actor_summoner = god_name(GOD_SHINING_ONE);
    }

    monster *summon = create_monster(mg);

    if (!summon)
        return false;

    summon->flags |= MF_ATT_CHANGE_ATTEMPT;

    if (!punish)
        mpr("You are momentarily dazzled by a brilliant light.");

    return true;
}

static monster_type _pick_drake(int pow)
{
    int chance = random2(3 + pow);

    if (chance > 24 || one_chance_in(6))
        return MONS_ACID_DRAGON;
    if (chance > 16 || one_chance_in(4))
        return random_choose(MONS_DEATH_DRAKE, MONS_LINDWURM);
    if (chance > 8 || one_chance_in(3))
        return random_choose(MONS_WIND_DRAKE, MONS_RIME_DRAKE, MONS_SWAMP_DRAKE);
    else
        return MONS_WYVERN;
}

// Not a spell. Rather, this is Tiamat's doing.
bool summon_drakes(int pow, bool punish)
{
    int amount = 2 + (random2(pow) / 9) + coinflip();
    int summoned = 0;

    for (int i = 0; i < amount; i++)
    {
        mgen_data mg(_pick_drake(pow),
            punish ? BEH_HOSTILE : BEH_FRIENDLY,
            you.pos(), MHITYOU, MG_FORCE_BEH | MG_AUTOFOE);
        mg.set_summoned(punish ? 0 : &you,
            punish ? 0 : min(2 + (random2(pow) / 4), 6),
            SPELL_NO_SPELL, GOD_BAHAMUT_TIAMAT);

        if (punish)
        {
            mg.extra_flags |= (MF_NO_REWARD | MF_HARD_RESET);
            mg.non_actor_summoner = god_name(GOD_BAHAMUT_TIAMAT);
        }

        monster *summon = create_monster(mg);

        if (!summon)
            continue;
        else
            summoned++;

        summon->flags |= MF_ATT_CHANGE_ATTEMPT;
    }

    if (!punish && summoned)
        mpr("You call forth Tiamat's armies to fight for you.");

    if (summoned)
        return true;
    return false;
}

/**
 * Essentially a macro to allow for a generic fail pattern to avoid leaking
 * information about invisible enemies. (Not implemented as a macro because I
 * find they create unreadable code.)
 *
 * @return spret::success
 **/
static bool _fail_tukimas()
{
    mprf("You can't see a target there!");
    return false; // Waste the turn - no anti-invis tech
}

/**
 * Gets an item description for use in Tukima's Dance messages.
 **/
static string _get_item_desc(const item_def* wpn, bool target_is_player)
{
    return wpn->name(target_is_player ? DESC_YOUR : DESC_THE);
}

/**
 * Checks if Tukima's Dance can actually affect the target (and anger them)
 *
 * @param target  The targeted monster (or player).
 * @return        Whether the target can be affected by Tukima's Dance.
 **/
bool tukima_affects(const actor &target)
{
    const item_def* wpn = target.weapon();
    return wpn
           && is_weapon(*wpn)
           && !is_range_weapon(*wpn)
           && !is_special_unrandom_artefact(*wpn)
           && wpn->base_type != OBJ_SHIELDS // BCADNOTE: Again, enemies can't properly use the new shields.
           && !mons_class_is_animated_weapon(target.type)
           && !mons_is_hepliaklqana_ancestor(target.type);
}

bool is_snake(const monster &mon)
{
    if (mons_is_zombified(mon))
        return (mons_genus(mon.base_monster) == MONS_SNAKE 
                || mons_genus(mon.base_monster) == MONS_GUARDIAN_SERPENT);
    return (mons_genus(mon.type) == MONS_SNAKE || mons_genus(mon.type) == MONS_GUARDIAN_SERPENT);
}

/**
 * Checks if Tukima's Dance is being cast on a valid target.
 *
 * @param target     The spell's target.
 * @return           Whether the target is valid.
 **/
static bool _check_tukima_validity(const actor *target, int weap_slot)
{
    bool target_is_player = target == &you;
    const item_def* wpn = target->weapon(weap_slot);
    bool can_see_target = target_is_player || target->visible_to(&you);

    // See if the wielded item is appropriate.
    if (!wpn)
    {
        if (!can_see_target)
            return _fail_tukimas();

        if (target_is_player)
            mpr(you.hands_act("twitch", "."));
        else
        {
            // FIXME: maybe move hands_act to class actor?
            bool plural = true;
            const string hand = target->hand_name(true, &plural);

            mprf("%s %s %s.",
                 apostrophise(target->name(DESC_THE)).c_str(),
                 hand.c_str(), conjugate_verb("twitch", plural).c_str());
        }
        return false;
    }

    if (!tukima_affects(*target))
    {
        if (!can_see_target)
            return _fail_tukimas();

        if (mons_class_is_animated_weapon(target->type))
        {
            simple_monster_message(*(monster*)target,
                                   " is already dancing.");
        }
        else
        {
            mprf("%s vibrate%s crazily for a second.",
                 _get_item_desc(wpn, target_is_player).c_str(),
                 wpn->quantity > 1 ? "" : "s");
        }
        return false;
    }

    return true;
}


/**
 * Actually animates the weapon of the target creature (no checks).
 *
 * @param pow               Spellpower.
 * @param target            The spell's target (monster or player)
 **/
static void _animate_weapon(int pow, actor* target, int weap_slot)
{
    bool target_is_player = target == &you;
    item_def * const wpn = target->weapon(weap_slot);
    ASSERT(wpn);
    if (target_is_player)
    {
        // Clear temp branding so we don't change the brand permanently.
        if (you.duration[DUR_EXCRUCIATING_WOUNDS] && wpn->link == you.props[PAINED_WEAPON_KEY].get_short())
            end_weapon_brand();

        // Mark weapon as "thrown", so we'll autopickup it later.
        wpn->flags |= ISFLAG_THROWN;
    }
    // If sac love, the weapon will go after you, not the target.
    const bool sac_love = you.get_mutation_level(MUT_NO_LOVE);
    // Self-casting haunts yourself! MUT_NO_LOVE overrides force friendly.
    const bool friendly = !target_is_player && !sac_love;
    const int dur = min(2 + (random2(pow) / 5), 6);

    mgen_data mg(MONS_DANCING_WEAPON,
                 friendly ? BEH_FRIENDLY : BEH_HOSTILE,
                 target->pos(),
                 (target_is_player || sac_love) ? MHITYOU : target->mindex(),
                 sac_love ? MG_NONE : MG_FORCE_BEH);
    mg.set_summoned(&you, dur, SPELL_TUKIMAS_DANCE);
    mg.props[TUKIMA_WEAPON] = *wpn;
    mg.props[TUKIMA_POWER] = pow;

    monster * const mons = create_monster(mg);

    if (!mons)
    {
        mprf("%s twitches for a moment.",
             _get_item_desc(wpn, target_is_player).c_str());
        return;
    }

    // Don't haunt yourself under sac love.
    if (!sac_love)
    {
        mons->add_ench(mon_enchant(ENCH_HAUNTING, 1, target,
                                   INFINITE_DURATION));
        mons->foe = target->mindex();
    }

    // We are successful. Unwield the weapon, removing any wield effects.
    mprf("%s dances into the air!",
         _get_item_desc(wpn, target_is_player).c_str());
    if (target_is_player)
        unwield_item();
    else
    {
        monster * const montarget = target->as_monster();
        const int primary_weap = montarget->inv[MSLOT_WEAPON];
        const mon_inv_type wp_slot = (primary_weap != NON_ITEM
                                      && &env.item[primary_weap] == wpn) ?
                                         MSLOT_WEAPON : MSLOT_ALT_WEAPON;
        ASSERT(montarget->inv[wp_slot] != NON_ITEM);
        ASSERT(&env.item[montarget->inv[wp_slot]] == wpn);

        montarget->unequip(*(montarget->mslot_item(wp_slot)), false, true);
        montarget->inv[wp_slot] = NON_ITEM;
    }

    // Find out what our god thinks before killing the item.
    conduct_type why = god_hates_item_handling(*wpn);

    wpn->clear();

    if (why)
    {
        simple_god_message(" booms: How dare you animate that foul thing!");
        did_god_conduct(why, 10, true, mons);
    }
}

void stickify(actor * caster, actor * target)
{
    if (target->is_player() || !is_snake(*target->as_monster()))
        return;

    simple_monster_message(*target->as_monster(), " straightens out and turns into a wooden club.");
    target->as_monster()->add_ench(ENCH_STICK);
    target->hurt(caster, INSTANT_DEATH, BEAM_SNAKES_TO_STICKS, KILLED_BY_BEAM);
}

/**
 * Casts Tukima's Dance, animating the weapon of the target creature (if valid)
 *
 * @param pow               Spellpower.
 * @param where             The target grid.
 **/
void cast_tukimas_dance(int pow, actor* target)
{
    ASSERT(target);

    int weap_slot = 0;

    if (_check_tukima_validity(target, 0))
    {
        if (_check_tukima_validity(target, 1))
            weap_slot = coinflip() ? 0 : 1;
        else
            weap_slot = 0;
    }
    else if (_check_tukima_validity(target, 1))
        weap_slot = 1;
    else
        return;

    _animate_weapon(pow, target, weap_slot);
}

int ball_lightning_hd(int pow, bool random)
{
    return max(1, maybe_random_div(pow, 6, random) - 6);
}

spret cast_conjure_ball_lightning(int pow, god_type god, bool fail)
{
    fail_check();
    bool success = false;
    bool chaos = determine_chaos(&you, SPELL_CONJURE_BALL_LIGHTNING);

    const int how_many = min(5, 2 + pow / 100 + random2(pow / 50 + 1));

    mgen_data cbl(chaos ? MONS_ENTROPIC_SPHERE : MONS_BALL_LIGHTNING, BEH_FRIENDLY, you.pos());
    cbl.set_summoned(&you, 0, SPELL_CONJURE_BALL_LIGHTNING, god);
    cbl.hd = ball_lightning_hd(pow);

    for (int i = 0; i < how_many; ++i)
    {
        if (monster *ball = create_monster(cbl))
        {
            success = true;
            ball->add_ench(ENCH_SHORT_LIVED);

            // Avoid ball lightnings without targets always moving towards (0,0)
            set_random_target(ball);
        }
    }

    if (success)
    {
        if (chaos)
            mpr("You conjure spheres of raw chaos!");
        else
            mpr("You create some ball lightning!");
    }
    else
        canned_msg(MSG_NOTHING_HAPPENS);

    return spret::success;
}

spret cast_summon_lightning_spire(int pow, const coord_def& where, god_type god, bool fail)
{
    const int dur = 2;

    if (grid_distance(where, you.pos()) > spell_range(SPELL_SUMMON_LIGHTNING_SPIRE,
                                                      pow)
        || !in_bounds(where))
    {
        mpr("That's too far away.");
        return spret::abort;
    }

    if (!monster_habitable_grid(MONS_HUMAN, env.grid(where)))
    {
        mpr("You can't construct there.");
        return spret::abort;
    }

    monster* mons = monster_at(where);
    if (mons)
    {
        if (you.can_see(*mons))
        {
            mpr("That space is already occupied.");
            return spret::abort;
        }

        fail_check();

        // invisible monster
        mpr("Something you can't see is blocking your construction!");
        return spret::success;
    }

    fail_check();

    mgen_data spire(MONS_LIGHTNING_SPIRE, BEH_FRIENDLY, where, MHITYOU,
                    MG_FORCE_BEH | MG_FORCE_PLACE | MG_AUTOFOE);
    spire.set_summoned(&you, dur, SPELL_SUMMON_LIGHTNING_SPIRE,  god);
    spire.hd = max(1, div_rand_round(pow, 10));

    if (monster * m = create_monster(spire))
    {
        player_post_summon_adjustments(SPELL_SUMMON_LIGHTNING_SPIRE, m, pow);
        if (!silenced(where))
            mpr("An electric hum fills the air.");
    }
    else
        canned_msg(MSG_NOTHING_HAPPENS);

    return spret::success;

}

/**
 * Choose a type of imp to summon with Call Imp.
 *
 * @param pow   The power with which the spell is being cast.
 * @return      An appropriate imp type.
 */
static monster_type _get_imp_type(int pow)
{
    // Proportion of gwybwcas is independent of spellpower.
    if (x_chance_in_y(5, 18))
        return MONS_GWYBWCA;

    // 3/13 * 13/18 = 1/6 chance of one of these two at 0-46 spellpower,
    // increasing up to about 4/9 at max spellpower.
    if (random2(pow) >= 46 || x_chance_in_y(3, 13))
        return one_chance_in(3) ? MONS_HAEARBWCA : MONS_CYSGOBWCA;

    // 5/9 crimson at 0-46 spellpower, about half that at max power.
    return MONS_LLAIBWCA;
}

static map<monster_type, const char*> _imp_summon_messages = {
    { MONS_GWYBWCA,
        "A beastly little devil appears in a puff of frigid air." },
    { MONS_HAEARBWCA, "A metallic apparition takes form in the air." },
    { MONS_CYSGOBWCA, "A shadowy apparition takes form in the air." },
    { MONS_LLAIBWCA, "A beastly little devil appears in a puff of flame." },
};

/**
 * Cast the spell Call Imp, summoning a friendly imp nearby.
 *
 * @param pow   The spellpower at which the spell is being cast.
 * @param god   The god of the caster.
 * @param fail  Whether the caster (you) failed to cast the spell.
 * @return      spret::fail if fail is true; spret::success otherwise.
 */
spret cast_call_imp(int pow, god_type god, bool fail)
{
    fail_check();

    const monster_type imp_type = _get_imp_type(pow);

    const int dur = min(2 + (random2(pow) / 4), 6);

    mgen_data imp_data = _pal_data(imp_type, dur, god, SPELL_CALL_IMP);
    if (monster *imp = create_monster(imp_data))
    {
        mpr(_imp_summon_messages[imp_type]);

        player_post_summon_adjustments(SPELL_CALL_IMP, imp, pow);

        _monster_greeting(imp, "_friendly_imp_greeting");
    }
    else
        canned_msg(MSG_NOTHING_HAPPENS);

    return spret::success;
}

spret cast_call_chameleon(int pow, god_type god, bool fail)
{
    fail_check();

    const int dur = min(2 + (random2(pow) / 3), 6);

    mgen_data c_data = _pal_data(MONS_CHAMELEON, dur, god, SPELL_CALL_CHAMELEON);
    if (monster *cham = create_monster(c_data))
    {
        cham->chameleon_mutate(chameleon_random_colour(true));
        player_post_summon_adjustments(SPELL_CALL_CHAMELEON, cham, pow);
    }
    else
        canned_msg(MSG_NOTHING_HAPPENS);

    return spret::success;
}

static bool _summon_demon_wrapper(int pow, god_type god, int spell,
                                  monster_type mon, int dur, bool friendly,
                                  bool charmed)
{
    bool success = false;

    if (monster *demon = create_monster(
            mgen_data(mon,
                      friendly ? BEH_FRIENDLY :
                       charmed ? BEH_CHARMED
                               : BEH_HOSTILE,
                      you.pos(), MHITYOU, MG_FORCE_BEH | MG_AUTOFOE)
            .set_summoned(&you, dur, spell, god)))
    {
        success = true;

        mpr("A demon appears!");

        if (!friendly)
        {
            mpr(charmed ? "You don't feel so good about this..."
                        : "It doesn't seem very happy.");
        }
        else if (mon == MONS_LLAIBWCA || mon == MONS_GWYBWCA
                || mon == MONS_HAEARBWCA || mon == MONS_CYSGOBWCA)
        {
            _monster_greeting(demon, "_friendly_imp_greeting");
        }

        if (charmed && !friendly)
        {
            int charm_dur = random_range(15 + pow / 14, 27 + pow / 11)
                            * BASELINE_DELAY;

            mon_enchant charm = demon->get_ench(ENCH_CHARM);
            charm.duration = charm_dur;
            demon->update_ench(charm);

            player_post_summon_adjustments(SPELL_SUMMON_DEMON, demon, pow);

            // Ensure that temporarily-charmed demons will outlast their charm
            mon_enchant abj = demon->get_ench(ENCH_ABJ);
            if (charm.duration + 100 > abj.duration)
            {
                abj.duration = charm.duration + 100;
                demon->update_ench(abj);
            }

            // Affects messaging, and stuns demon a turn upon charm wearing off
            demon->props["charmed_demon"].get_bool() = true;
        }
    }

    return success;
}

static bool _summon_common_demon(int pow, god_type god, int spell)
{
    const int chance = 70 - (pow / 3);
    monster_type type = MONS_PROGRAM_BUG;

    if (x_chance_in_y(chance, 100))
        type = random_demon_by_tier(4);
    else
        type = random_demon_by_tier(3);

    return _summon_demon_wrapper(pow, god, spell, type,
                                 min(2 + (random2(pow) / 4), 6),
                                 random2(pow) > 3, false);
}

static bool _summon_greater_demon(int pow, god_type god, int spell)
{
    monster_type mon = summon_any_demon(RANDOM_DEMON_GREATER);

    const bool charmed = (random2(pow) > 5);
    const bool friendly = (charmed && mons_demon_tier(mon) == 2);

    return _summon_demon_wrapper(pow, god, spell, mon,
                                 4, friendly, charmed);
}

bool summon_demon_type(monster_type mon, int pow, god_type god,
                       int spell, bool friendly)
{
    return _summon_demon_wrapper(pow, god, spell, mon,
                                 min(2 + (random2(pow) / 4), 6),
                                 friendly, false);
}

spret cast_summon_demon(int pow, god_type god, bool fail)
{
    // Chaos spawn, nychdods and sixfirhies are not rPois
    if (otr_stop_summoning_prompt())
        return spret::abort;

    fail_check();
    mpr("You open a gate to Pandemonium!");

    if (!_summon_common_demon(pow, god, SPELL_SUMMON_DEMON))
        canned_msg(MSG_NOTHING_HAPPENS);

    return spret::success;
}

spret cast_summon_greater_demon(int pow, god_type god, bool fail)
{
    if (otr_stop_summoning_prompt())
        return spret::abort;

    fail_check();
    mpr("You open a gate to Pandemonium!");

    if (!_summon_greater_demon(pow, god, SPELL_SUMMON_GREATER_DEMON))
        canned_msg(MSG_NOTHING_HAPPENS);

    return spret::success;
}

spret cast_shadow_creatures(int st, god_type god, level_id place,
                                 bool fail)
{
    if (otr_stop_summoning_prompt("summon"))
        return spret::abort;

    fail_check();
    const bool scroll = (st == MON_SUMM_SCROLL);
    mpr("Wisps of shadow whirl around you...");

    int num = (scroll ? roll_dice(2, 2) : 1);
    int num_created = 0;

    for (int i = 0; i < num; ++i)
    {
        if (monster *mons = create_monster(
            mgen_data(RANDOM_COMPATIBLE_MONSTER, BEH_FRIENDLY, you.pos(),
                      MHITYOU, MG_FORCE_BEH | MG_AUTOFOE | MG_NO_OOD)
                      // This duration is only used for band members.
                      .set_summoned(&you, scroll ? 2 : 1, st, god)
                      .set_place(place),
            false))
        {
            // In the rare cases that a specific spell set of a monster will
            // cause anger, even if others do not, try rerolling
            int tries = 0;
            while (god_hates_monster(*mons) && ++tries <= 20)
            {
                // Save the enchantments, particularly ENCH_SUMMON etc.
                mon_enchant_list ench = mons->enchantments;
                FixedBitVector<NUM_ENCHANTMENTS> cache = mons->ench_cache;
                if (mons_class_is_zombified(mons->type))
                    define_zombie(mons, mons->base_monster, mons->type);
                else
                    define_monster(*mons);
                mons->enchantments = ench;
                mons->ench_cache = cache;
            }

            if (!scroll)
                player_post_summon_adjustments(SPELL_SHADOW_CREATURES, mons);

            // If we didn't find a valid spell set yet, just give up
            if (tries > 20)
                monster_die(*mons, KILL_RESET, NON_MONSTER);
            else
            {
                // Choose a new duration based on HD.
                int x = max(mons->get_experience_level() - 3, 1);
                int d = div_rand_round(17,x);
                if (scroll)
                    d++;
                if (d < 1)
                    d = 1;
                if (d > 4)
                    d = 4;
                mon_enchant me = mon_enchant(ENCH_ABJ, d);
                me.set_duration(mons, &me);
                mons->update_ench(me);

                // Set summon ID, to share summon cap with its band members
                mons->props["summon_id"].get_int() = mons->mid;
            }

            // Remove any band members that would turn hostile, and link their
            // summon IDs
            for (monster_iterator mi; mi; ++mi)
            {
                if (testbits(mi->flags, MF_BAND_MEMBER)
                    && (mid_t) mi->props["band_leader"].get_int() == mons->mid)
                {
                    if (god_hates_monster(**mi))
                        monster_die(**mi, KILL_RESET, NON_MONSTER);

                    mi->props["summon_id"].get_int() = mons->mid;
                }
            }

            num_created++;
        }
    }

    if (!num_created)
        mpr("The shadows disperse without effect.");

    return spret::success;
}

bool can_cast_malign_gateway()
{
    timeout_malign_gateways(0);

    return count_malign_gateways() < 1;
}

coord_def find_gateway_location(actor* caster, coord_def target)
{
    vector<coord_def> points;

    bool xray = you.xray_vision;
    you.xray_vision = false;

    if (target != coord_def(0, 0))
    {
        targeter_cone hitfunc(caster, LOS_RADIUS);
        hitfunc.set_aim(target);

        for (int i = 1; i <= LOS_RADIUS; i++)
        {
            for (const auto &entry : hitfunc.sweep[i])
            {
                if (entry.second <= 0)
                    continue;

                coord_def test = entry.first;

                if (!in_bounds(test) || !feat_is_malign_gateway_suitable(env.grid(test))
                    || actor_at(test)
                    || count_neighbours_with_func(test, &feat_is_solid) != 0
                    || !caster->see_cell_no_trans(test))
                {
                    continue;
                }

                points.push_back(test);
            }
        }
    }

    if (!points.empty())
        return points[random2(points.size())];

    for (coord_def delta : Compass)
    {
        coord_def test = coord_def(-1, -1);

        for (int t = 0; t < 11; t++)
        {
            test = caster->pos() + (delta * (2+t));
            if (!in_bounds(test) || !feat_is_malign_gateway_suitable(env.grid(test))
                || actor_at(test)
                || count_neighbours_with_func(test, &feat_is_solid) != 0
                || !caster->see_cell_no_trans(test))
            {
                continue;
            }

            points.push_back(test);
        }
    }

    you.xray_vision = xray;

    if (points.empty())
        return coord_def(0, 0);

    return points[random2(points.size())];
}

void create_malign_gateway(coord_def point, beh_type beh, string cause,
                           int pow, god_type god, bool is_player)
{
    const int malign_gateway_duration = BASELINE_DELAY * (random2(3) + 2);
    env.markers.add(new map_malign_gateway_marker(point,
                            malign_gateway_duration,
                            is_player,
                            is_player ? "" : cause,
                            beh,
                            god,
                            pow));
    env.markers.clear_need_activate();
    env.grid(point) = DNGN_MALIGN_GATEWAY;
    set_terrain_changed(point);

    noisy(spell_effect_noise(SPELL_MALIGN_GATEWAY), point);
    mprf(MSGCH_WARN, "The dungeon shakes, a horrible noise fills the air, "
                     "and a portal to some otherworldly place is opened!");
}

spret cast_malign_gateway(actor * caster, int pow, god_type god, 
                                 coord_def target, bool fail)
{
    coord_def point = find_gateway_location(caster, target);
    bool success = point != coord_def(0, 0);

    bool is_player = caster->is_player();

    if (success)
    {
        fail_check();

        create_malign_gateway(
            point,
            is_player ? BEH_FRIENDLY
                      : attitude_creation_behavior(
                          caster->as_monster()->attitude),
            is_player ? ""
                      : caster->as_monster()->full_name(DESC_A),
            pow,
            god,
            is_player);

        return spret::success;
    }

    // We don't care if monsters fail to cast it.
    if (is_player)
        mpr("A gateway cannot be opened in this cramped space!");

    return spret::abort;
}

spret cast_summon_horrible_things(int pow, god_type god, bool fail)
{
    fail_check();
    if (god == GOD_NO_GOD && one_chance_in(5))
    {
        // if someone deletes the db, no message is ok
        mpr(getMiscString("SHT_int_loss"));
        lose_stat(STAT_INT, 1);
    }

    int num_abominations = random_range(2, 4) + x_chance_in_y(pow, 200);
    int num_tmons = random2(pow) > 120 ? 2 : random2(pow) > 50 ? 1 : 0;

    if (num_tmons == 0)
        num_abominations++;

    int count = 0;

    while (num_abominations-- > 0)
    {
        const mgen_data abom = _pal_data(MONS_ABOMINATION_LARGE, 3, god,
                                         SPELL_SUMMON_HORRIBLE_THINGS);
        if (monster * ab = create_monster(abom))
        {
            player_post_summon_adjustments(SPELL_SUMMON_HORRIBLE_THINGS, ab, pow);
            ++count;
        }
    }

    while (num_tmons-- > 0)
    {
        const mgen_data tmons = _pal_data(MONS_TENTACLED_MONSTROSITY, 3, god,
                                          SPELL_SUMMON_HORRIBLE_THINGS);
        if (monster * tm = create_monster(tmons))
        {
            player_post_summon_adjustments(SPELL_SUMMON_HORRIBLE_THINGS, tm, pow);
            ++count;
        }
    }

    if (!count)
        canned_msg(MSG_NOTHING_HAPPENS);

    return spret::success;
}

static bool _water_adjacent(coord_def p)
{
    for (orth_adjacent_iterator ai(p); ai; ++ai)
    {
        if (feat_is_water(env.grid(*ai)))
            return true;
    }

    return false;
}

spret cast_summon_jungle(int pow, god_type god, bool fail)
{
    fail_check();

    int num_hunters = coinflip() + x_chance_in_y(pow, 150);
    int num_animals = div_rand_round(pow, 40) + random2(3);

    int count = 0;

    while (num_hunters-- > 0)
    {
        const mgen_data hunter = _pal_data(MONS_THORN_HUNTER, 3, god,
            SPELL_SUMMON_JUNGLE);
        if (monster * hu = create_monster(hunter))
        {
            player_post_summon_adjustments(SPELL_SUMMON_JUNGLE, hu, pow);
            ++count;
        }
    }

    while (num_animals-- > 0)
    {
        if (coinflip())
        {
            const mgen_data elephant = _pal_data(MONS_DIRE_ELEPHANT, 3, god,
                SPELL_SUMMON_JUNGLE);
            if (monster * el = create_monster(elephant))
            {
                player_post_summon_adjustments(SPELL_SUMMON_JUNGLE, el, pow);
                ++count;
            }
        }

        else
        {
            const mgen_data anaconda = _pal_data(MONS_ANACONDA, 3, god,
                SPELL_SUMMON_JUNGLE);
            if (monster * an = create_monster(anaconda))
            {
                player_post_summon_adjustments(SPELL_SUMMON_JUNGLE, an, pow);
                ++count;
            }
        }
    }

    if (!count)
        canned_msg(MSG_NOTHING_HAPPENS);
    else
    {
        if (you.duration[DUR_JUNGLE])
            mpr("The dungeon cannot take anymore flooding right now.");
        else
        {
            const int duration = random_range(120 + pow, 200 + pow * 3 / 2);

            // Make a lake
            coord_def pond = you.pos();

            for (distance_iterator di(you.pos(), false, true,
                6 + div_rand_round(pow, 50)); di; ++di)
            {
                if ((feat_is_wall(env.grid(*di)) && !feat_is_permarock(env.grid(*di))))
                {
                    if (one_chance_in(3))
                    {
                        temp_change_terrain(*di, DNGN_SHALLOW_WATER, duration,
                            TERRAIN_CHANGE_FORESTED);
                        env.grid_colours(*di) = GREEN;
                    }
                    else
                        temp_change_terrain(*di, DNGN_MANGROVE, duration,
                            TERRAIN_CHANGE_FORESTED);
                }
                else if (env.grid(*di) == DNGN_FLOOR && x_chance_in_y(pow, 750)
                    && !actor_at(*di) && !plant_forbidden_at(*di, true))
                {
                    temp_change_terrain(*di, DNGN_MANGROVE, duration,
                        TERRAIN_CHANGE_FORESTED);
                }
            }

            you.duration[DUR_JUNGLE] = duration;
            int num = random_range(30, 66);
            int deep = div_rand_round(num, 3);

            for (distance_iterator di(pond, true, false, 7); di && num > 0; ++di)
            {
                if (env.grid(*di) == DNGN_FLOOR
                    && (di.radius() == 0 || _water_adjacent(*di))
                    && x_chance_in_y(4, di.radius() + 3))
                {
                    num--;
                    deep--;

                    dungeon_feature_type feat = DNGN_SHALLOW_WATER;
                    if (deep > 0 && *di != you.pos())
                    {
                        monster* mon = monster_at(*di);
                        if (!mon || mon->is_habitable_feat(DNGN_DEEP_WATER))
                            feat = DNGN_DEEP_WATER;
                    }

                    if (player_in_branch(BRANCH_SLIME))
                    {
                        if (jiyva_is_dead())
                            feat = DNGN_FLOOR;
                        else
                            feat = DNGN_SLIMY_WATER;
                    }

                    temp_change_terrain(*di, feat, duration, TERRAIN_CHANGE_FORESTED);
                    env.grid_colours(*di) = GREEN;
                }
            }
        }
    }

    return spret::success;
}

/**
 * Cast summon forest.
 *
 * @param caster The caster.
 * @param pow    The spell power.
 * @param god    The god of the summoned dryad (usually the caster's).
 * @param fail   Did this spell miscast? If true, abort the cast.
 * @return       spret::abort if a summoning area couldn't be found,
 *               spret::fail if one could be found but we miscast, and
 *               spret::success if the spell was successfully cast.
*/
spret cast_summon_forest(actor* caster, int pow, coord_def &where, god_type god, bool fail)
{
    const int duration = random_range(120 + pow, 200 + pow * 3 / 2);

    // Is this area open enough to summon a forest?
    bool success = false;
    for (adjacent_iterator ai(where, false); ai; ++ai)
    {
        if (count_neighbours_with_func(*ai, &feat_is_solid) == 0)
        {
            success = true;
            break;
        }
    }

    if (grid_distance(where, you.pos()) > spell_range(SPELL_SUMMON_LIGHTNING_SPIRE, pow)
        || !in_bounds(where))
    {
        mpr("That's too far away.");
        return spret::abort;
    }

    if (success)
    {
        if (otr_stop_summoning_prompt("summon a forest"))
            return spret::abort;

        fail_check();
        // Replace some rock walls with trees, then scatter a smaller number
        // of trees on unoccupied floor (such that they do not break connectivity)
        for (distance_iterator di(caster->pos(), false, true,
                                  LOS_DEFAULT_RANGE); di; ++di)
        {
            if ((feat_is_wall(env.grid(*di)) && !feat_is_permarock(env.grid(*di))
                 && x_chance_in_y(pow, 150))
                || (env.grid(*di) == DNGN_FLOOR && x_chance_in_y(pow, 1250)
                    && !actor_at(*di) && !plant_forbidden_at(*di, true)))
            {
                if (player_in_branch(BRANCH_SLIME))
                    temp_change_terrain(*di, DNGN_TREE, duration,
                            TERRAIN_CHANGE_FORESTED);
                else if (player_in_branch(BRANCH_SWAMP))
                    temp_change_terrain(*di, DNGN_MANGROVE, duration,
                        TERRAIN_CHANGE_FORESTED);
                else if (player_in_branch(BRANCH_SLIME) && !jiyva_is_dead())
                    temp_change_terrain(*di, DNGN_SLIMESHROOM, duration,
                        TERRAIN_CHANGE_FORESTED);
                else
                    temp_change_terrain(*di, DNGN_TREE, duration,
                        TERRAIN_CHANGE_FORESTED);
            }
        }

        // Maybe make a pond
        if (coinflip())
        {
            coord_def pond = find_gateway_location(caster);
            int num = random_range(10, 22);
            int deep = (!one_chance_in(3) ? div_rand_round(num, 3) : 0);

            for (distance_iterator di(pond, true, false, 4); di && num > 0; ++di)
            {
                if (env.grid(*di) == DNGN_FLOOR
                    && (di.radius() == 0 || _water_adjacent(*di))
                    && x_chance_in_y(4, di.radius() + 3))
                {
                    num--;
                    deep--;

                    dungeon_feature_type feat = DNGN_SHALLOW_WATER;
                    if (deep > 0 && *di != you.pos())
                    {
                        monster* mon = monster_at(*di);
                        if (!mon || mon->is_habitable_feat(DNGN_DEEP_WATER))
                            feat = DNGN_DEEP_WATER;
                    }

                    if (player_in_branch(BRANCH_SLIME))
                    {
                        if (jiyva_is_dead())
                            feat = DNGN_FLOOR;
                        else
                            feat = DNGN_SLIMY_WATER;
                    }

                    temp_change_terrain(*di, feat, duration, TERRAIN_CHANGE_FORESTED);
                }
            }
        }

        if (player_in_branch(BRANCH_SLIME))
        {
            if (jiyva_is_dead())
                mpr("A sinister howling wind blows as part of Jiyva's old plane tries to return here!");
            else
                mpr("One of Jivya's private planes collides here with a gurgle!");
        }
        if (player_in_branch(BRANCH_SWAMP))
            mpr("An enchanted swamp collides here with a loud woosh!");
        else
            mpr("A forested plane collides here with a resounding crunch!");
        noisy(spell_effect_noise(SPELL_SUMMON_FOREST), caster->pos());

        mgen_data kael_data = _pal_data(MONS_KAEL, 1, god,
                                         SPELL_SUMMON_FOREST);
        kael_data.hd = 5 + div_rand_round(pow, 18);

        if (monster *kael = create_monster(kael_data))
        {
            mon_enchant abj = kael->get_ench(ENCH_ABJ);
            abj.duration = duration - 10;
            kael->update_ench(abj);
            kael->move_to_pos(where);
            kael->max_hit_points = kael->hit_points = div_rand_round(kael->max_hit_points * kael->get_experience_level(), 5);

            player_post_summon_adjustments(SPELL_SUMMON_FOREST, kael, pow);

            // Pre-awaken the forest just summoned.
            bolt dummy;
            mons_cast(kael, dummy, SPELL_AWAKEN_FOREST,
                kael->spell_slot_flags(SPELL_AWAKEN_FOREST));
        }

        you.duration[DUR_FORESTED] = duration;

        return spret::success;
    }

    mpr("You need more open space to cast this spell.");
    return spret::abort;
}

static bool _animatable_remains(const item_def& item)
{
    return item.base_type == OBJ_CORPSES
        && mons_class_can_be_zombified(item.mon_type)
        // the above allows spectrals/etc
        && (mons_zombifiable(item.mon_type)
            || mons_skeleton(item.mon_type));
}

/**
 * Equip the dearly departed with its ex-possessions.
 *
 * This excludes holy items (not wieldable by the undead, and items which were
 * dropped onto the square, so the player can't equip their undead slaves with
 * items of their choice.
 *
 * @param a      where to pull items from
 * @param corpse the corpse of the dead monster.
 * @param mon    the zombie/skeleton/etc. being reequipped.
 */
static void _equip_undead(const coord_def &a, const item_def& corpse, monster *mon)
{
    if (!corpse.props.exists(MONSTER_MID))
        return;
    for (stack_iterator si(a); si; ++si)
    {
        if (!si->props.exists(DROPPER_MID_KEY)
            || si->props[DROPPER_MID_KEY].get_int()
               != corpse.props[MONSTER_MID].get_int())
        {
            continue;
        }

        // Don't equip the undead with holy items.
        if (is_holy_item(*si))
            continue;

        // Stupid undead can't use most items.
        if (si->base_type != OBJ_WEAPONS
            && si->base_type != OBJ_STAVES
            && si->base_type != OBJ_ARMOURS
            && si->base_type != OBJ_SHIELDS
            && si->base_type != OBJ_JEWELLERY)
        {
            continue;
        }

        unwind_var<int> save_speedinc(mon->speed_increment);
        mon->pickup_item(*si, false, true);
    }
}

// Displays message when raising dead with Animate Skeleton or Animate Dead.
static void _display_undead_motions(int motions)
{
    vector<const char *> motions_list;

    // Check bitfield from _raise_remains for types of corpse(s) being animated.
    if (motions & DEAD_ARE_WALKING)
        motions_list.push_back("walking");
    if (motions & DEAD_ARE_HOPPING)
        motions_list.push_back("hopping");
    if (motions & DEAD_ARE_SWIMMING)
        motions_list.push_back("swimming");
    if (motions & DEAD_ARE_FLYING)
        motions_list.push_back("flying");
    if (motions & DEAD_ARE_SLITHERING)
        motions_list.push_back("slithering");
    if (motions & DEAD_ARE_CRAWLING)
        motions_list.push_back("crawling");

    // Prevents the message from getting too long and spammy.
    if (motions_list.size() > 3)
        mpr("The dead have arisen!");
    else
    {
        mprf("The dead are %s!", comma_separated_line(motions_list.begin(),
             motions_list.end()).c_str());
    }
}

static bool _raise_remains(const coord_def &pos, int corps, beh_type beha,
                           unsigned short hitting, actor *as, string nas,
                           god_type god, bool actual, bool apply_lovelessness,
                           monster **raised, int* motions_r, spell_type source_spell)
{
    if (raised)
        *raised = 0;

    const item_def& item = env.item[corps];

    if (!_animatable_remains(item))
        return false;

    if (!actual)
        return true;

    monster_type zombie_type = item.mon_type;
    // hack: don't re-froggify poor prince ribbit after death
    if (zombie_type == MONS_PRINCE_RIBBIT)
        zombie_type = MONS_HUMAN;

    const int hd = (item.props.exists(MONSTER_HIT_DICE)) ?
                    item.props[MONSTER_HIT_DICE].get_short() : 0;

    // Save the corpse name before because it can get destroyed if it is
    // being drained and the raising interrupts it.
    monster_flags_t name_type;
    string name;
    if (is_named_corpse(item))
        name = get_corpse_name(item, &name_type);

    monster_type mon = item.sub_type == CORPSE_BODY ? MONS_ZOMBIE
                                                    : MONS_SKELETON;
    monster_type monnum = static_cast<monster_type>(item.orig_monnum);
    // hack: don't re-froggify poor prince ribbit after death
    if (monnum == MONS_PRINCE_RIBBIT)
        monnum = MONS_HUMAN;

    if (mon == MONS_ZOMBIE && !mons_zombifiable(zombie_type))
    {
        ASSERT(mons_skeleton(zombie_type));
        if (as == &you)
        {
            mpr("The flesh is too rotten for a proper zombie; "
                "only a skeleton remains.");
        }
        mon = MONS_SKELETON;
    }

    // Use the original monster type as the zombified type here, to get
    // the proper stats from it.
    mgen_data mg(mon, beha, pos, hitting,
                 MG_FORCE_BEH | MG_FORCE_PLACE | MG_AUTOFOE);
    mg.set_summoned(as, 0, 0, god);
    mg.set_base(monnum);

    if (item.props.exists(CORPSE_HEADS))
    {
        // Headless hydras cannot be raised, sorry.
        if (item.props[CORPSE_HEADS].get_short() == 0)
        {
            if (you.see_cell(pos))
            {
                mprf("The headless hydra %s sways and immediately collapses!",
                     item.sub_type == CORPSE_BODY ? "corpse" : "skeleton");
            }
            return false;
        }
        mg.props[MGEN_NUM_HEADS] = item.props[CORPSE_HEADS].get_short();
    }

    // No experience for monsters animated by god wrath or the Sword of
    // Zonguldrok.
    if (!nas.empty())
        mg.extra_flags |= MF_NO_REWARD;

    mg.non_actor_summoner = nas;

    monster *mons = create_monster(mg);

    if (raised)
        *raised = mons;

    if (!mons)
        return false;

    if (source_spell)
    {
        if (as->is_player())
            apply_summon_xp_bonus(source_spell, mons);
        chaos_summon(source_spell, mons, as, false);
    }

    if (as->is_monster() || !have_passive(passive_t::extend_undead)) // only Kiku dead-raising lasts forever.
        mons->add_ench(mon_enchant(ENCH_FAKE_ABJURATION, 5));

    // If the original monster has been levelled up, its HD might be different
    // from its class HD, in which case its HP should be rerolled to match.
    if (mons->get_experience_level() != hd)
    {
        mons->set_hit_dice(max(hd, 1));
        roll_zombie_hp(mons);
    }

    if (!name.empty()
        && (!name_type || (name_type & MF_NAME_MASK) == MF_NAME_REPLACE))
    {
        name_zombie(*mons, monnum, name);
    }

    if (!actor_at(pos))
        mons->move_to_pos(pos, true, true);

    // Re-equip the zombie.
    if (mons_class_itemuse(monnum) & MU_WIELD_MASK)
        _equip_undead(pos, item, mons);

    // Destroy the monster's corpse, as it's no longer needed.
    item_was_destroyed(item);
    destroy_item(corps);

    if (apply_lovelessness)
        check_lovelessness(*mons);

    // Bitfield for motions - determines text displayed when animating dead.
    // XXX: could this use monster shape in some way?
    if (mons_class_primary_habitat(zombie_type)    == HT_WATER
        || mons_class_primary_habitat(zombie_type) == HT_LAVA)
    {
        *motions_r |= DEAD_ARE_SWIMMING;
    }
    else if (mons_class_flag(zombie_type, M_FLIES))
        *motions_r |= DEAD_ARE_FLYING;
    else if (mons_genus(zombie_type)    == MONS_SNAKE
             || mons_genus(zombie_type) == MONS_NAGA
             || mons_genus(zombie_type) == MONS_SALAMANDER
             || mons_genus(zombie_type) == MONS_GUARDIAN_SERPENT
             || mons_genus(zombie_type) == MONS_ELEPHANT_SLUG
             || mons_genus(zombie_type) == MONS_TYRANT_LEECH
             || mons_genus(zombie_type) == MONS_WORM)
    {
        *motions_r |= DEAD_ARE_SLITHERING;
    }
    else if (mons_genus(zombie_type)    == MONS_FROG
             || mons_genus(zombie_type) == MONS_QUOKKA)
    {
        *motions_r |= DEAD_ARE_HOPPING;
    }
    else if (mons_genus(zombie_type)    == MONS_WORKER_ANT
             || mons_genus(zombie_type) == MONS_BEETLE
             || mons_base_char(zombie_type) == 's') // many genera
    {
        *motions_r |= DEAD_ARE_CRAWLING;
    }
    else
        *motions_r |= DEAD_ARE_WALKING;

    return true;
}

// Note that quiet will *not* suppress the message about a corpse
// you are butchering being animated.
// This is called for Animate Skeleton and from animate_dead.
int animate_remains(const coord_def &a, corpse_type class_allowed,
                    beh_type beha, unsigned short hitting,
                    actor *as, string nas,
                    god_type god, bool actual,
                    bool quiet, bool apply_lovelessness,
                    monster** mon, int* motions_r,
                    spell_type source_spell)
{
    if (is_sanctuary(a))
        return 0;

    int number_found = 0;
    bool any_success = false;
    int motions = 0;

    // Search all the items on the ground for a corpse.
    for (stack_iterator si(a); si; ++si)
    {
        if (si->base_type != OBJ_CORPSES)
            continue;

        if (class_allowed != CORPSE_BODY && si->sub_type != CORPSE_SKELETON)
            continue;

        if (is_forbidden_food(*si) && class_allowed == CORPSE_SKELETON)
            continue;

        number_found++;

        if (!_animatable_remains(*si))
            continue;

        const bool was_draining = is_being_drained(*si);
        const bool was_butchering = is_being_butchered(*si);

        const bool success = _raise_remains(a, si.index(), beha,
                                            hitting, as, nas, god, actual,
                                            apply_lovelessness, mon, &motions, source_spell);

        if (actual && success)
        {
            // Ignore quiet.
            if (was_butchering || was_draining)
            {
                mprf("The corpse you are %s rises to %s!",
                     was_draining ? "drinking from"
                                  : "butchering",
                     beha == BEH_FRIENDLY ? "join your ranks"
                                          : "attack");
            }

            if (!quiet && you.see_cell(a))
                _display_undead_motions(motions);

            if (was_butchering)
                xom_is_stimulated(200);
        }

        any_success |= success;

        break;
    }

    if (motions_r && you.see_cell(a))
        *motions_r |= motions;

    if (number_found == 0)
        return -1;

    if (!any_success)
        return 0;

    return 1;
}

int animate_dead(actor *caster, int /*pow*/, beh_type beha,
                 unsigned short hitting, actor *as, string nas, god_type god,
                 bool actual, bool spell)
{
    int number_raised = 0;
    int number_seen   = 0;
    int motions       = 0;

    for (radius_iterator ri(caster->pos(), LOS_NO_TRANS); ri; ++ri)
    {
        // There may be many corpses on the same spot.
        while (animate_remains(*ri, CORPSE_BODY, beha, hitting,
                               as, nas, god,
                               actual, true, true, 0, &motions, 
                               spell ? SPELL_ANIMATE_DEAD : SPELL_NO_SPELL) > 0)
        {
            number_raised++;
            if (you.see_cell(*ri))
                number_seen++;

            // For the tracer, we don't care about exact count (and the
            // corpse is not gone).
            if (!actual)
                break;
        }
    }

    if (actual && number_seen > 0)
        _display_undead_motions(motions);

    return number_raised;
}

bool cast_animate_skeleton(god_type god, bool fail, coord_def pos)
{
    bool found = false;
    bool forbidden_found = false;
    string name = "";

    for (stack_iterator si(pos, false); si; ++si)
    {
        if (si->base_type == OBJ_CORPSES
            && mons_class_can_be_zombified(si->mon_type)
            && mons_skeleton(si->mon_type))
        {
            if (!is_forbidden_food(*si))
                found = true;
            else
            {
                forbidden_found = true;
                name = si->name(DESC_THE);
            }
        }
    }

    if (!found)
    {
        if (forbidden_found)
            mprf(MSGCH_GOD, "It would be sinful to animate %s.", name.c_str());
        return false;
    }

    if (fail)
    {
        mpr("You fail to channel necromantic magic through your feet.");
        return false;
    }

    canned_msg(MSG_ANIMATE_REMAINS);

    const char* no_space = "...but the skeleton had no space to rise!";

    // First, we try to animate a skeleton if there is one.
    const int animate_skel_result = animate_remains(pos, CORPSE_SKELETON,
                                                    BEH_FRIENDLY, MHITYOU,
                                                    &you, "", god);
    if (animate_skel_result != -1)
    {
        if (animate_skel_result == 0)
            mpr(no_space);
        return false;
    }

    // If not, look for a corpse and butcher it.
    for (stack_iterator si(pos, false); si; ++si)
    {
        if (si->is_type(OBJ_CORPSES, CORPSE_BODY)
            && mons_skeleton(si->mon_type)
            && mons_class_can_be_zombified(si->mon_type))
        {
            butcher_corpse(*si, true);
            if (you.visible_igrd(pos))
            {
                mpr("Before your eyes, flesh is ripped from the corpse!");
                request_autopickup();
            }
            // Only convert the top one.
            break;
        }
    }

    // Now we try again to animate a skeleton.
    // this return type is insanely stupid
    const int animate_result = animate_remains(pos, CORPSE_SKELETON,
                                               BEH_FRIENDLY, MHITYOU, &you, "",
                                               god);
    dprf("result: %d", animate_result);
    switch (animate_result)
    {
        case -1:
            // No need for failure message.
            break;
        case 0:
            mpr(no_space);
            break;
        default:
            // success, messages already printed
            break;
    }

    return true;
}

spret cast_animate_dead(int pow, god_type god, bool fail)
{
    fail_check();

    const int corpses = animate_dead(&you, pow, BEH_FRIENDLY, MHITYOU, &you, "", god, false);

    if (!corpses && !you.attribute[ATTR_KIKU_CORPSE])
    {
        mpr("There is nothing nearby to animate!");
        return spret::abort;
    }

    if (you.attribute[ATTR_KIKU_CORPSE])
    {
        const int piety = div_rand_round(you.piety, 20);
        const int threshold = 11 - random2(piety);
        if (corpses < threshold)
        {
            if (kiku_receive_corpses(apply_invo_enhancer(you.skill(SK_NECROMANCY), true)))
                lose_piety(3);
            else if (!corpses)
                return spret::abort;
        }
    }
    
    canned_msg(MSG_CALL_DEAD);

    if (!animate_dead(&you, pow + 1, BEH_FRIENDLY, MHITYOU, &you, "", god))
        canned_msg(MSG_NOTHING_HAPPENS);

    return spret::success;
}

// returns an item index, or -1 on failure
int find_simulacrable_corpse(coord_def c)
{
    int co = -1;
    for (stack_iterator si(c, true); si; ++si)
    {
        if (si->is_type(OBJ_CORPSES, CORPSE_BODY)
            && mons_class_can_be_zombified(si->mon_type))
        {
            co = si->index();
        }
    }
    return co;
}

/**
 * Have the player cast simulacrum.
 *
 * @param pow The spell power.
 * @param god The god casting the spell.
 * @param fail If true, return spret::fail unless the spell is aborted.
 * @return spret::abort if no viable corpse was at the player's location,
 *         otherwise spret::success or spret::fail based on fail.
 */
spret cast_simulacrum(int pow, god_type god, bool fail)
{
    int co = find_simulacrable_corpse(you.pos());

    fail_check();

    if (!co)
    {
        if (!you.attribute[ATTR_KIKU_CORPSE])
        {
            mpr("There is nothing here that can be animated!");
            return spret::abort;
        }

        if (feat_destroys_items(env.grid(you.pos())))
        {
            mpr("Any corpse summoned here would be destroyed before you could use it!");
            return spret::abort;
        }

        const int kiku_pow = apply_invo_enhancer(you.skill(SK_NECROMANCY), true);
        const int tries = div_rand_round(kiku_pow, 3);
        monster_type mons = MONS_RAT; // Arbitrary very low value monster if for some reason player uses at almost no Necro.
        int value = 0;

        for (int i = 0; i < tries; i++)
        {
            const monster_type test_mons = random_kiku_monster(kiku_pow);
            const monster_info test_info = monster_info(MONS_SIMULACRUM, test_mons);
            const int test_value = max_corpse_chunks(test_mons) * (test_info.hd + test_info.mbase_speed);
            if (test_value > value)
            {
                mons  = test_mons;
                value = test_value;
            }
        }

        item_def * corpse = place_kiku_corpse(mons, you.pos());

        if (!corpse)
        {
            // Shouldn't happen?
            mprf(MSGCH_GOD, "Kikubaaqudha failed to find a corpse for you!");
            return spret::abort;
        }
        else
        {
            const monster_info name_info = monster_info(mons_genus(mons));
            mprf(MSGCH_GOD, "Kikubaaqudha delivers you %s corpse!", name_info.common_name(DESC_A).c_str());
            lose_piety(3);
        }
        
        co = corpse->index();
    }

    canned_msg(MSG_ANIMATE_REMAINS);

    item_def& corpse = env.item[co];
    // How many simulacra can this particular monster give at maximum.
    int num_sim  = 1 + random2(max_corpse_chunks(corpse.mon_type));
    num_sim  = stepdown_value(num_sim, 4, 4, 12, 12);

    mgen_data mg = _pal_data(MONS_SIMULACRUM, 0, god, SPELL_SIMULACRUM);
    mg.set_base(corpse.mon_type);

    // Can't create more than the max for the monster.
    int how_many = min(8, 4 + random2(pow) / 20);
    how_many = min<int>(how_many, num_sim);

    if (corpse.props.exists(CORPSE_HEADS))
    {
        // Avoid headless hydras. Unlike Animate Dead, still consume the flesh.
        if (corpse.props[CORPSE_HEADS].get_short() == 0)
        {
            // No monster to conj_verb with :(
            mprf("The headless hydra simulacr%s immediately collapse%s into snow!",
                 how_many == 1 ? "um" : "a", how_many == 1 ? "s" : "");
            if (!turn_corpse_into_skeleton(corpse))
                butcher_corpse(corpse, false, false);
            return spret::success;
        }
        mg.props[MGEN_NUM_HEADS] = corpse.props[CORPSE_HEADS].get_short();
    }

    int count = 0;
    for (int i = 0; i < how_many; ++i)
    {
        if (monster *sim = create_monster(mg))
        {
            count++;
            player_post_summon_adjustments(SPELL_SIMULACRUM, sim, pow);
            sim->add_ench(mon_enchant(ENCH_FAKE_ABJURATION, have_passive(passive_t::extend_undead) ? 6 : 3));
        }
    }

    if (!count)
        canned_msg(MSG_NOTHING_HAPPENS);
    else if (!turn_corpse_into_skeleton(corpse))
        butcher_corpse(corpse, false, false);

    return spret::success;
}

// Return a definite/indefinite article for (number) things.
static const char *_count_article(int number, bool definite)
{
    if (number == 0)
        return "No";
    else if (definite)
        return "The";
    else if (number == 1)
        return "A";
    else
        return "Some";
}

bool twisted_resurrection(actor *caster, int pow, beh_type beha,
                          unsigned short foe, god_type god, bool actual)
{
    int num_orcs = 0;
    int num_holy = 0;

    // In a tracer (actual == false), num_crawlies counts the number of
    // affected corpses. When actual == true, these count the number of
    // crawling corpses, macabre masses, and lost corpses, respectively.
    int num_crawlies = 0;
    int num_masses = 0;
    int num_lost = 0;

    // ...and the number of each that were seen by the player.
    int seen_crawlies = 0;
    int seen_masses = 0;
    int seen_lost = 0;
    int seen_lost_piles = 0;

    for (radius_iterator ri(caster->pos(), LOS_NO_TRANS); ri; ++ri)
    {
        int num_corpses = 0;
        int total_max_chunks = 0;
        const bool visible = you.see_cell(*ri);

        // Count up number/size of corpses at this location.
        for (stack_iterator si(*ri); si; ++si)
        {
            if (si->is_type(OBJ_CORPSES, CORPSE_BODY))
            {
                if (!actual)
                {
                    ++num_crawlies;
                    continue;
                }

                if (monster_class_is_orcish(si->mon_type))
                    num_orcs++;
                if (mons_class_holiness(si->mon_type) & MH_HOLY)
                    num_holy++;

                total_max_chunks += max_corpse_chunks(si->mon_type);

                ++num_corpses;
                item_was_destroyed(*si);
                destroy_item(si->index());
            }
        }

        if (!actual || num_corpses == 0)
            continue;

        // 3 HD per 2 max chunks at 500 power.
        int hd = div_rand_round((pow + 100) * total_max_chunks, 400);

        if (hd <= 0)
        {
            num_lost += num_corpses;
            if (visible)
            {
                seen_lost += num_corpses;
                seen_lost_piles++;
            }
            continue;
        }

        // Getting a huge abomination shouldn't be too easy.
        if (hd > 15)
            hd = 15 + (hd - 15) / 2;

        hd = min(hd, 30);

        monster_type montype;

        if (hd >= 11 && num_corpses > 2)
            montype = MONS_ABOMINATION_LARGE;
        else if (hd >= 6 && num_corpses > 1)
            montype = MONS_ABOMINATION_SMALL;
        else if (num_corpses > 1)
            montype = MONS_MACABRE_MASS;
        else
            montype = MONS_CRAWLING_CORPSE;

        if (caster->is_player())
        {
            if (montype == MONS_ABOMINATION_LARGE && !player_allowed_abom(true))
                montype = MONS_ABOMINATION_SMALL;
            if (montype == MONS_ABOMINATION_SMALL && !player_allowed_abom())
                montype = MONS_MACABRE_MASS;
        }

        mgen_data mg(montype, beha, *ri, foe, MG_FORCE_BEH | MG_AUTOFOE);
        mg.set_summoned(caster, 0, 0, god);
        if (monster *mons = create_monster(mg))
        {
            // Make it position on top of where the corpse was, if possible.
            mons->move_to_pos(*ri);

            // Set hit dice, AC, and HP.
            init_abomination(*mons, hd, caster->is_player());

            if (caster->is_player())
            {
                mons_make_god_gift(*mons, GOD_YREDELEMNUL);
                add_companion(mons);
            }

            if (num_corpses > 1)
            {
                ++num_masses;
                if (visible)
                    ++seen_masses;
            }
            else
            {
                ++num_crawlies;
                if (visible)
                    ++seen_crawlies;
            }
        }
        else
        {
            num_lost += num_corpses;
            if (visible)
            {
                seen_lost += num_corpses;
                seen_lost_piles++;
            }
        }
    }

    // Monsters shouldn't bother casting Twisted Res for just a single corpse.
    if (!actual)
        return num_crawlies >= (caster->is_player() ? 1 : 2);

    if (num_lost + num_crawlies + num_masses == 0)
        return false;

    if (seen_lost)
    {
        mprf("%s %s into %s!",
             _count_article(seen_lost, seen_crawlies + seen_masses == 0),
             seen_lost == 1 ? "corpse collapses" : "corpses collapse",
             seen_lost_piles == 1 ? "a pulpy mess" : "pulpy messes");
    }

    if (seen_crawlies > 0)
    {
        mprf("%s %s to drag %s along the ground!",
             _count_article(seen_crawlies, seen_lost + seen_masses == 0),
             seen_crawlies == 1 ? "corpse begins" : "corpses begin",
             seen_crawlies == 1 ? "itself" : "themselves");
    }

    if (seen_masses > 0)
    {
        mprf("%s corpses meld into %s of writhing flesh!",
             _count_article(2, seen_crawlies + seen_lost == 0),
             seen_masses == 1 ? "an agglomeration" : "agglomerations");
    }

    if (num_orcs > 0 && caster->is_player())
        did_god_conduct(DID_DESECRATE_ORCISH_REMAINS, 2 * num_orcs);
    if (num_holy > 0 && caster->is_player())
        did_god_conduct(DID_DESECRATE_HOLY_REMAINS, 2 * num_holy);

    return true;
}

monster_type pick_random_wraith()
{
    return random_choose_weighted(1, MONS_PHANTOM,
                                  1, MONS_HUNGRY_GHOST,
                                  1, MONS_SHADOW_WRAITH,
                                  5, MONS_WRAITH,
                                  2, MONS_FREEZING_WRAITH,
                                  2, MONS_PHANTASMAL_WARRIOR);
}

spret cast_haunt(int pow, const coord_def& where, god_type god, bool fail)
{
    monster* m = monster_at(where);

    if (m == nullptr)
    {
        fail_check();
        mpr("An evil force gathers, but it quickly dissipates.");
        return spret::success; // still losing a turn
    }
    else if (m->wont_attack())
    {
        mpr("You cannot haunt those who bear you no hostility.");
        return spret::abort;
    }

    int mi = m->mindex();
    ASSERT(!invalid_monster_index(mi));

    if (stop_attack_prompt(m, false, you.pos()))
        return spret::abort;

    fail_check();

    bool friendly = true;
    int success = 0;
    int to_summon = stepdown_value(2 + (random2(pow) / 10) + (random2(pow) / 10),
                                   2, 2, 6, -1);

    while (to_summon--)
    {
        const monster_type mon = pick_random_wraith();

        if (monster *mons = create_monster(
                mgen_data(mon, BEH_FRIENDLY, where, mi, MG_FORCE_BEH)
                .set_summoned(&you, 3, SPELL_HAUNT, god)))
        {
            success++;

            player_post_summon_adjustments(SPELL_HAUNT, mons, pow);
            mons->add_ench(mon_enchant(ENCH_HAUNTING, 1, m, INFINITE_DURATION));
            mons->foe = mi;
        }
    }

    if (success > 1)
    {
        mpr(friendly ? "Insubstantial figures form in the air."
                     : "You sense hostile presences.");
    }
    else if (success)
    {
        mpr(friendly ? "An insubstantial figure forms in the air."
                     : "You sense a hostile presence.");
    }
    else
    {
        canned_msg(MSG_NOTHING_HAPPENS);
        return spret::success;
    }

    return spret::success;
}



static spell_type servitor_spells[] =
{
    // primary spells
    SPELL_LEHUDIBS_CRYSTAL_SPEAR,
    SPELL_IOOD,
    SPELL_IRON_SHOT,
    SPELL_BOLT_OF_FIRE,
    SPELL_BOLT_OF_COLD,
    SPELL_POISON_ARROW,
    SPELL_LIGHTNING_BOLT,
    SPELL_BOLT_OF_MAGMA,
    SPELL_GHOSTLY_FIREBALL,
    SPELL_BOLT_OF_DRAINING,
    SPELL_VENOM_BOLT,
    SPELL_THROW_ICICLE,
    SPELL_STONE_ARROW,
    // secondary spells
    SPELL_CONJURE_BALL_LIGHTNING,
    SPELL_FIREBALL,
    SPELL_AIRSTRIKE,
    SPELL_LRD,
    SPELL_FREEZING_CLOUD,
    SPELL_POISONOUS_CLOUD,
    SPELL_FORCE_LANCE,
    SPELL_FORCE_LASSO,
    SPELL_BLINDING_SPRAY,
    SPELL_MEPHITIC_CLOUD,
    // fallback spells
    SPELL_STICKY_FLAME,
    SPELL_THROW_FROST,
    SPELL_FREEZE,
    SPELL_FLAME_TONGUE,
    SPELL_STING,
    SPELL_SANDBLAST,
};

/**
 * Initialize the given spellforged servitor's HD and spellset, based on the
 * caster's spellpower and castable attack spells.
 *
 * @param mon       The spellforged servitor to be initialized.
 * @param caster    The entity summoning the servitor; may be the player.
 */
static void _init_servitor_monster(monster &mon, const actor& caster)
{
    const monster* caster_mon = caster.as_monster();
    const int pow = caster_mon ?
                        6 * caster_mon->spell_hd(SPELL_SPELLFORGED_SERVITOR) :
                        calc_spell_power(SPELL_SPELLFORGED_SERVITOR, true);

    mon.set_hit_dice(9 + div_rand_round(pow, 14));
    mon.max_hit_points = mon.hit_points = 60 + roll_dice(7, 5); // 67-95
                                            // mhp doesn't vary with HD
    int spell_levels = 0;

    for (const spell_type spell : servitor_spells)
    {
        if (caster.has_spell(spell)
            && (caster_mon || raw_spell_fail(spell) < 50))
        {
            mon.spells.emplace_back(spell, 0, MON_SPELL_MAGICAL);
            spell_levels += spell_difficulty(spell);
        }
    }

    // Fix up frequencies now that we know the total number of spell levels.
    const int base_freq = caster_mon ? 67 : 200;
    for (auto& slot : mon.spells)
    {
        slot.freq = max(1, div_rand_round(spell_difficulty(slot.spell)
                                          * base_freq,
                                          spell_levels));
    }
    mon.props[CUSTOM_SPELLS_KEY].get_bool() = true;
}

void init_servitor(monster* servitor, actor* caster)
{
    ASSERT(servitor); // XXX: change to monster &servitor
    ASSERT(caster); // XXX: change to actor &caster
    _init_servitor_monster(*servitor, *caster);

    if (you.can_see(*caster))
    {
        mprf("%s %s a servant imbued with %s destructive magic!",
             caster->name(DESC_THE).c_str(),
             caster->conj_verb("summon").c_str(),
             caster->pronoun(PRONOUN_POSSESSIVE).c_str());
    }
    else
        simple_monster_message(*servitor, " appears!");

    int shortest_range = LOS_RADIUS + 1;
    for (const mon_spell_slot &slot : servitor->spells)
    {
        if (slot.spell == SPELL_NO_SPELL)
            continue;

        int range = spell_range(slot.spell, 100, false);
        if (range < shortest_range)
            shortest_range = range;
    }
    servitor->props["ideal_range"].get_int() = shortest_range;
}

spret cast_spellforged_servitor(int pow, god_type god, bool fail)
{
    fail_check();

    mgen_data mdata = _pal_data(MONS_SPELLFORGED_SERVITOR, 4, god,
                                SPELL_SPELLFORGED_SERVITOR);

    if (monster* mon = create_monster(mdata))
    {
        init_servitor(mon, &you);
        player_post_summon_adjustments(SPELL_SPELLFORGED_SERVITOR, mon, pow);
    }
    else
        canned_msg(MSG_NOTHING_HAPPENS);

    return spret::success;
}

static int _abjuration(int pow, monster *mon)
{
    // Scale power into something comparable to summon lifetime.
    const int abjdur = pow * 12;

    // XXX: make this a prompt
    if (mon->wont_attack())
        return false;

    int duration;
    int summ_type;
    if (mon->is_summoned(&duration, &summ_type))
    {
        int sockage = max(fuzz_value(abjdur, 60, 30), 40);
        dprf("%s abj: dur: %d, abj: %d",
             mon->name(DESC_PLAIN).c_str(), duration, sockage);

        bool shielded = false;
        // Vestiges of "Pseudosummon" status.
        if (summ_type == SPELL_STICKS_TO_SNAKES)
        {
            sockage = 0;
            shielded = true;
        }
        // TSO and Trog's abjuration protection.
        if (mons_is_god_gift(*mon, GOD_SHINING_ONE))
        {
            sockage = sockage * (30 - mon->get_hit_dice()) / 45;
            if (sockage < duration)
            {
                simple_god_message(" protects a fellow warrior from your evil magic!",
                                   GOD_SHINING_ONE);
                shielded = true;
            }
        }
        else if (mons_is_god_gift(*mon, GOD_TROG))
        {
            sockage = sockage * 8 / 15;
            if (sockage < duration)
            {
                simple_god_message(" shields an ally from your puny magic!",
                                   GOD_TROG);
                shielded = true;
            }
        }

        mon_enchant abj = mon->get_ench(ENCH_ABJ);
        if (!mon->lose_ench_duration(abj, sockage) && !shielded)
            simple_monster_message(*mon, " shudders.");
    }

    return true;
}

spret cast_aura_of_abjuration(int pow, bool fail)
{
    fail_check();

    if (!you.duration[DUR_ABJURATION_AURA])
        mpr("You begin to abjure the creatures around you!");
    else
        mpr("You extend your aura of abjuration.");

    you.increase_duration(DUR_ABJURATION_AURA,  6 + roll_dice(2, pow / 12), 50);
    you.props["abj_aura_pow"].get_int() = pow;

    return spret::success;
}

void do_aura_of_abjuration(int delay)
{
    const int pow = you.props["abj_aura_pow"].get_int() * delay / 10;
    for (monster_near_iterator mi(you.pos(), LOS_NO_TRANS); mi; ++mi)
        _abjuration(pow / 2, *mi);
}

monster* find_battlesphere(const actor* agent)
{
    if (agent->props.exists("battlesphere"))
        return monster_by_mid(agent->props["battlesphere"].get_int());
    else
        return nullptr;
}

static int _battlesphere_hd(int pow, bool random = true)
{
    return 1 + maybe_random_div(pow, 11, random);
}

static dice_def _battlesphere_damage(int hd)
{
    return dice_def(2, 5 + hd);
}

dice_def battlesphere_damage(int pow)
{
    return _battlesphere_damage(_battlesphere_hd(pow, false));
}

spret cast_battlesphere(actor* agent, int pow, god_type god, bool fail)
{
    fail_check();

    monster* battlesphere;
    if (agent->is_player() && (battlesphere = find_battlesphere(&you)))
    {
        bool recalled = false;
        if (!you.can_see(*battlesphere))
        {
            coord_def empty;
            if (find_habitable_spot_near(agent->pos(), MONS_BATTLESPHERE, 3, false, empty)
                && battlesphere->move_to_pos(empty))
            {
                recalled = true;
            }
        }

        if (recalled)
        {
            mpr("You recall your battlesphere and imbue it with additional"
                " charge.");
        }
        else
            mpr("You imbue your battlesphere with additional charge.");

        battlesphere->battlecharge = min(20, (int) battlesphere->battlecharge
                                              + 4 + random2(pow + 10) / 10);

        // Increase duration
        mon_enchant abj = battlesphere->get_ench(ENCH_FAKE_ABJURATION);
        abj.duration = min(abj.duration + (7 + roll_dice(2, pow)) * 10, 500);
        battlesphere->update_ench(abj);
    }
    else
    {
        ASSERT(!find_battlesphere(agent));
        mgen_data mg (MONS_BATTLESPHERE,
                      agent->is_player() ? BEH_FRIENDLY
                                         : SAME_ATTITUDE(agent->as_monster()),
                      agent->pos(), agent->mindex());
        mg.set_summoned(agent, 0, SPELL_BATTLESPHERE, god);
        mg.hd = _battlesphere_hd(pow);
        battlesphere = create_monster(mg);

        if (battlesphere)
        {
            int dur = min((7 + roll_dice(2, pow)) * 10, 500);
            battlesphere->add_ench(mon_enchant(ENCH_FAKE_ABJURATION, 1, 0, dur));
            battlesphere->summoner = agent->mid;
            if (agent->is_player())
                player_post_summon_adjustments(SPELL_BATTLESPHERE, battlesphere, pow);
            else
                chaos_summon(SPELL_BATTLESPHERE, battlesphere, agent);
            agent->props["battlesphere"].get_int() = battlesphere->mid;

            if (agent->is_player())
                mpr("You conjure a crystaline sphere.");
            else
            {
                if (you.can_see(*agent) && you.can_see(*battlesphere))
                {
                    simple_monster_message(*agent->as_monster(),
                                           " conjures a crystaline sphere!");
                }
                else if (you.can_see(*battlesphere))
                    simple_monster_message(*battlesphere, " appears!");
                battlesphere->props["band_leader"].get_int() = agent->mid;
            }
            battlesphere->battlecharge = 4 + random2(pow + 10) / 10;
            battlesphere->foe = agent->mindex();
            battlesphere->target = agent->pos();
        }
        else if (agent->is_player() || you.can_see(*agent))
            canned_msg(MSG_NOTHING_HAPPENS);
    }

    return spret::success;
}

void end_battlesphere(monster* mons, bool killed)
{
    // Should only happen if you dismiss it in wizard mode, I think
    if (!mons)
        return;

    actor* agent = actor_by_mid(mons->summoner);
    if (agent)
        agent->props.erase("battlesphere");

    if (!killed)
    {
        if (agent && agent->is_player())
        {
            if (you.can_see(*mons))
            {
                if (mons->battlecharge == 0)
                {
                    mpr("Your battlesphere expends the last of its energy"
                        " and dissipates.");
                }
                else
                    mpr("Your battlesphere wavers and loses cohesion.");
            }
            else
                mpr("You feel your bond with your battlesphere wane.");
        }
        else if (you.can_see(*mons))
            simple_monster_message(*mons, " dissipates.");

        if (!cell_is_solid(mons->pos()))
            place_cloud(CLOUD_MAGIC_TRAIL, mons->pos(), 3 + random2(3), mons);

        monster_die(*mons, KILL_RESET, NON_MONSTER);
    }
}

bool battlesphere_can_mirror(spell_type spell)
{
    return (spell == SPELL_FREEZE
        || spell == SPELL_STICKY_FLAME
        || spell == SPELL_SANDBLAST
        || spell == SPELL_AIRSTRIKE
        || spell == SPELL_BLINDING_SPRAY
        || spell == SPELL_STING
        || spell == SPELL_FIREBALL
        || spell == SPELL_BOLT_OF_FIRE
        || spell == SPELL_BOLT_OF_COLD
        || spell == SPELL_LIGHTNING_BOLT
        || spell == SPELL_BOLT_OF_MAGMA
        || spell == SPELL_THROW_FLAME
        || spell == SPELL_THROW_FROST
        || spell == SPELL_FREEZING_CLOUD
        || spell == SPELL_MEPHITIC_CLOUD
        || spell == SPELL_VENOM_BOLT
        || spell == SPELL_BOLT_OF_DRAINING
        || spell == SPELL_LEHUDIBS_CRYSTAL_SPEAR
        || spell == SPELL_POISONOUS_CLOUD
        || spell == SPELL_POISONOUS_VAPOURS
        || spell == SPELL_FIRE_STORM
        || spell == SPELL_VAMPIRIC_DRAINING
        || spell == SPELL_POISON_ARROW
        || spell == SPELL_HURL_HELLFIRE
        || spell == SPELL_IRON_SHOT
        || spell == SPELL_STONE_ARROW
        || spell == SPELL_SHOCK
        || spell == SPELL_THROW_ICICLE
        || spell == SPELL_GLACIATE
        || spell == SPELL_FLAME_TONGUE
        || spell == SPELL_STEAM_BALL
        || spell == SPELL_ENERGY_BOLT
        || spell == SPELL_PRIMAL_WAVE
        || spell == SPELL_STICKY_FLAME_RANGE
        || spell == SPELL_THUNDERBOLT
        || spell == SPELL_BLINDING_SPRAY
        || spell == SPELL_FORCE_LANCE
        || spell == SPELL_GHOSTLY_FIREBALL
        || spell == SPELL_WATERSTRIKE
        || spell == SPELL_WIND_BLAST
        || spell == SPELL_FLASH_FREEZE
        || spell == SPELL_CRYSTAL_BOLT
        || spell == SPELL_CORROSIVE_BOLT
        || spell == SPELL_ELECTRICAL_BOLT
        || spell == SPELL_ICEBLAST
        || spell == SPELL_SEARING_RAY);
}

bool aim_battlesphere(actor* agent, spell_type spell, int powc, bolt& beam)
{
    //Is this spell something that will trigger the battlesphere?
    if (battlesphere_can_mirror(spell))
    {
        monster* battlesphere = find_battlesphere(agent);

        // If we've somehow gotten separated from the battlesphere (ie:
        // abyss level teleport), bail out and cancel the battlesphere bond
        if (!battlesphere)
        {
            agent->props.erase("battlesphere");
            return false;
        }

        // In case the battlesphere was in the middle of a (failed)
        // target-seeking action, cancel it so that it can focus on a new
        // target
        reset_battlesphere(battlesphere);

        // Don't try to fire at ourselves
        if (beam.target == battlesphere->pos())
            return false;

        // If the player beam is targeted at a creature, aim at this creature.
        // Otherwise, aim at the furthest creature in the player beam path
        bolt testbeam = beam;

        if (agent->is_player())
            testbeam.thrower = KILL_YOU_MISSILE;
        else
        {
            testbeam.thrower = KILL_MON_MISSILE;
            testbeam.source_id = agent->mid;
        }

        testbeam.is_tracer = true;
        zap_type ztype = spell_to_zap(spell);

        // Fallback for non-standard spell zaps
        if (ztype == NUM_ZAPS)
            ztype = ZAP_MAGIC_DART;

        // This is so that reflection and pathing rules for the parent beam
        // will be obeyed when figuring out what is being aimed at
        zappy(ztype, powc, agent->is_monster(), testbeam);

        battlesphere->props["firing_target"] = beam.target;
        battlesphere->props.erase("foe");
        if (!actor_at(beam.target))
        {
            testbeam.fire();

            for (const coord_def c : testbeam.path_taken)
            {
                if (c != battlesphere->pos() && monster_at(c))
                {
                    battlesphere->props["firing_target"] = c;
                    battlesphere->foe = actor_at(c)->mindex();
                    battlesphere->props["foe"] = battlesphere->foe;
                    break;
                }
            }

            // If we're firing at empty air, lose any prior target lock
            if (!battlesphere->props.exists("foe"))
                battlesphere->foe = agent->mindex();
        }
        else
        {
            battlesphere->foe = actor_at(beam.target)->mindex();
            battlesphere->props["foe"] = battlesphere->foe;
        }

        battlesphere->props["ready"] = true;

        return true;
    }

    return false;
}

bool trigger_battlesphere(actor* agent, bolt& beam)
{
    monster* battlesphere = find_battlesphere(agent);
    if (!battlesphere)
        return false;

    if (battlesphere->props.exists("ready"))
    {
        // If the battlesphere is aiming at empty air but the triggering
        // conjuration is an explosion, try to find something to shoot within
        // the blast
        if (!battlesphere->props.exists("foe") && beam.is_explosion)
        {
            explosion_map exp_map;
            exp_map.init(INT_MAX);
            beam.determine_affected_cells(exp_map, coord_def(), 0,
                                          beam.ex_size, true, true);

            for (radius_iterator ri(beam.target, beam.ex_size, C_SQUARE);
                 ri; ++ri)
            {
                if (exp_map(*ri - beam.target + coord_def(9,9)) < INT_MAX)
                {
                    const actor *targ = actor_at(*ri);
                    if (targ && targ != battlesphere)
                    {
                        battlesphere->props["firing_target"] = *ri;
                        battlesphere->foe = targ->mindex();
                        battlesphere->props["foe"] = battlesphere->foe;
                        continue;
                    }
                }
            }
        }

        battlesphere->props.erase("ready");
        battlesphere->props["firing"] = true;

        // Since monsters may be acting out of sequence, give the battlesphere
        // enough energy to attempt to fire this round, and requeue if it's
        // already passed its turn
        if (agent->is_monster())
        {
            battlesphere->speed_increment = 100;
            queue_monster_for_action(battlesphere);
        }

        return true;
    }

    return false;
}

// Called at the start of each round. Cancels firing orders given in the
// previous round, if the battlesphere was not able to execute them fully
// before the next player action
void reset_battlesphere(monster* mons)
{
    if (!mons || mons->type != MONS_BATTLESPHERE)
        return;

    mons->props.erase("ready");

    if (mons->props.exists("tracking"))
    {
        mons->props.erase("tracking");
        mons->props.erase("firing");
        if (mons->props.exists("foe"))
            mons->foe = mons->props["foe"].get_int();
        mons->behaviour = BEH_SEEK;
    }
}

bool fire_battlesphere(monster* mons)
{
    if (!mons || mons->type != MONS_BATTLESPHERE)
        return false;

    actor* agent = actor_by_mid(mons->summoner);

    if (!agent || !agent->alive())
    {
        end_battlesphere(mons, false);
        return false;
    }

    bool used = false;

    if (mons->props.exists("firing") && mons->battlecharge > 0)
    {
        if (mons->props.exists("tracking"))
        {
            if (mons->pos() == mons->props["tracking_target"].get_coord())
            {
                mons->props.erase("tracking");
                if (mons->props.exists("foe"))
                    mons->foe = mons->props["foe"].get_int();
                mons->behaviour = BEH_SEEK;
            }
            else // Currently tracking, but have not reached target pos
            {
                mons->target = mons->props["tracking_target"].get_coord();
                return false;
            }
        }
        else
        {
            // If the battlesphere forgot its foe (due to being out of los),
            // remind it
            if (mons->props.exists("foe"))
                mons->foe = mons->props["foe"].get_int();
        }

        // Set up the beam.
        bolt beam;
        beam.source_name = "battlesphere";

        // If we are locked onto a foe, use its current position
        if (!invalid_monster_index(mons->foe) && env.mons[mons->foe].alive())
            beam.target = env.mons[mons->foe].pos();
        else
            beam.target = mons->props["firing_target"].get_coord();

        // Sanity check: if we have somehow ended up targeting ourselves, bail
        if (beam.target == mons->pos())
        {
            mprf(MSGCH_ERROR, "Battlesphere targeting itself? Fixing.");
            mons->props.erase("firing");
            mons->props.erase("firing_target");
            mons->props.erase("foe");
            return false;
        }

        beam.name       = "crystal beam";
        beam.range      = LOS_RADIUS;
        beam.hit        = AUTOMATIC_HIT;
        beam.damage     = _battlesphere_damage(mons->get_hit_dice());
        beam.glyph      = dchar_glyph(DCHAR_FIRED_ZAP);
        beam.colour     = BROWN;
        beam.flavour    = BEAM_CRYSTAL;
        beam.pierce     = false;

        bool chaos = mons->has_ench(ENCH_CHAOTIC_INFUSION);

        if (chaos)
        {
            beam.name         = "chaos beam";
            beam.colour       = ETC_JEWEL;
            beam.flavour      = BEAM_CHAOTIC;
            beam.real_flavour = BEAM_CHAOTIC;
        }

        // Fire tracer.
        fire_tracer(mons, beam);

        // Never fire if we would hurt the caster, and ensure that the beam
        // would hit at least SOMETHING, unless it was targeted at empty space
        // in the first place
        if (beam.friend_info.count == 0
            && (monster_at(beam.target) ? beam.foe_info.count > 0 :
                find(beam.path_taken.begin(), beam.path_taken.end(),
                     beam.target)
                    != beam.path_taken.end()))
        {
            beam.thrower = (agent->is_player()) ? KILL_YOU : KILL_MON;
            simple_monster_message(*mons, chaos? " chaotically discharges!" : " fires!");
            beam.fire();

            used = true;
            // Decrement # of volleys left and possibly expire the battlesphere.
            if (--mons->battlecharge == 0)
                end_battlesphere(mons, false);

            mons->props.erase("firing");
        }
        // If we are firing at something, try to find a nearby position
        // from which we could safely fire at it
        else
        {
            const bool empty_beam = (beam.foe_info.count == 0);
            for (distance_iterator di(mons->pos(), true, true, 2); di; ++di)
            {
                if (*di == beam.target || actor_at(*di)
                    || cell_is_solid(*di)
                    || !agent->see_cell(*di))
                {
                    continue;
                }

                beam.source = *di;
                beam.is_tracer = true;
                beam.friend_info.reset();
                beam.foe_info.reset();
                beam.fire();
                if (beam.friend_info.count == 0
                    && (beam.foe_info.count > 0 || empty_beam))
                {
                    if (empty_beam
                        && find(beam.path_taken.begin(), beam.path_taken.end(),
                                beam.target) == beam.path_taken.end())
                    {
                        continue;
                    }

                    mons->firing_pos = coord_def(0, 0);
                    mons->target = *di;
                    mons->behaviour = BEH_WANDER;
                    mons->props["foe"] = mons->foe;
                    mons->props["tracking"] = true;
                    mons->foe = MHITNOT;
                    mons->props["tracking_target"] = *di;
                    break;
                }
            }

            // If we didn't find a better firing position nearby, cancel firing
            if (!mons->props.exists("tracking"))
                mons->props.erase("firing");
        }
    }

    // If our last target is dead, or the player wandered off, resume
    // following the player
    if ((mons->foe == MHITNOT || !mons->can_see(*agent)
         || (!invalid_monster_index(mons->foe)
             && !agent->can_see(env.mons[mons->foe])))
        && !mons->props.exists("tracking"))
    {
        mons->foe = agent->mindex();
    }

    return used;
}

int prism_hd(int pow, bool random)
{
    return maybe_random_div(pow, 10, random);
}

spret cast_fulminating_prism(actor* caster, int pow,
                                  const coord_def& where, bool fail)
{
    const int range = (!caster || caster->is_player()) ? spell_range(SPELL_FULMINANT_PRISM, pow)
                                                       : mon_spell_range(SPELL_FULMINANT_PRISM, caster->as_monster());

    if (grid_distance(where, caster->pos())
        > range)
    {
        if (caster->is_player())
            mpr("That's too far away.");
        return spret::abort;
    }

    if (cell_is_solid(where))
    {
        if (caster->is_player())
            mpr("You can't conjure that within a solid object!");
        return spret::abort;
    }

    actor* victim = monster_at(where);
    if (victim)
    {
        if (caster->can_see(*victim))
        {
            if (caster->is_player())
                mpr("You can't place the prism on a creature.");
            return spret::abort;
        }

        fail_check();

        // FIXME: maybe should do _paranoid_option_disable() here?
        if (caster->is_player()
            || (you.can_see(*caster) && you.see_cell(where)))
        {
            if (you.can_see(*victim))
            {
                mprf("%s %s.", victim->name(DESC_THE).c_str(),
                               victim->conj_verb("twitch").c_str());
            }
            else
                canned_msg(MSG_GHOSTLY_OUTLINE);
        }
        return spret::success;      // Don't give free detection!
    }

    fail_check();

    const int hd = prism_hd(pow);

    mgen_data prism_data = mgen_data(MONS_FULMINANT_PRISM,
                                     caster->is_player()
                                     ? BEH_FRIENDLY
                                     : SAME_ATTITUDE(caster->as_monster()),
                                     where, MHITNOT, MG_FORCE_PLACE);
    prism_data.set_summoned(caster, 0, SPELL_FULMINANT_PRISM);
    prism_data.hd = hd;
    monster *prism = create_monster(prism_data);
    if (caster->is_player())
        apply_summon_xp_bonus(SPELL_FULMINANT_PRISM, prism);
    chaos_summon(SPELL_FULMINANT_PRISM, prism, caster, false);

    if (prism)
    {
        if (caster->observable())
        {
            mprf("%s %s a prism of explosive energy!",
                 caster->name(DESC_THE).c_str(),
                 caster->conj_verb("conjure").c_str());
        }
        else if (you.can_see(*prism))
            mprf("A prism of explosive energy appears from nowhere!");
    }
    else if (you.can_see(*caster))
        canned_msg(MSG_NOTHING_HAPPENS);

    return spret::success;
}

monster* find_spectral_weapon(const actor* agent)
{
    if (agent->props.exists("spectral_weapon"))
        return monster_by_mid(agent->props["spectral_weapon"].get_int());
    else
        return nullptr;
}

// BCADNOTE: Since Mons can't use hybrid shield/weapons properly atm; they can't
// be spectral weapons. Consider changing this in the future.

bool weapon_can_be_spectral(const item_def *wpn, bool spell)
{
    if (!spell && bool(wpn->brand != SPWPN_SPECTRAL))
        return false;

    return wpn && is_weapon(*wpn) && !is_range_weapon(*wpn)
        && !is_special_unrandom_artefact(*wpn)
        && wpn->base_type != OBJ_SHIELDS;
}

static bool _is_spectral(const item_def *wpn)
{
    return wpn && is_weapon(*wpn) && (wpn->brand == SPWPN_SPECTRAL);
}

spret cast_spectral_weapon(actor *agent, int pow, god_type god, bool fail, bool spell)
{
    ASSERT(agent);

    const int dur = min(2 + random2(1 + div_rand_round(pow, 25)), 4);
    item_def* wpn = agent->weapon();

    if (agent->is_player())
    {
        // Can't use the spell if either of your weapons is naturally spectral.
        // BCADDO: Dual spectral weapons?
        if (spell)
        {
            item_def * wpn0 = you.weapon(0);
            item_def * wpn1 = you.weapon(1);

            if (wpn0 && _is_spectral(wpn0))
            {
                mprf("%s resists your attempt to coax out additional spirits.", wpn0->name(DESC_YOUR).c_str());
                return spret::abort;
            }

            if (wpn1 && _is_spectral(wpn1))
            {
                mprf("%s resists your attempt to coax out additional spirits.", wpn1->name(DESC_YOUR).c_str());
                return spret::abort;
            }
        }

        if (you.weapon(0) && weapon_can_be_spectral(you.weapon(0), spell))
            wpn = you.weapon(0);
        else if (you.weapon(1) && weapon_can_be_spectral(you.weapon(1), spell))
            wpn = you.weapon(1);
        else if (you.weapon(0))
            wpn = you.weapon(0);
        else if (you.weapon(1))
            wpn = you.weapon(1);

        if (!wpn)
        {
            mpr(you.hands_act("twitch", "."));
            return spret::abort;
        }
        else if (!weapon_can_be_spectral(wpn, spell))
        {
            mprf("%s vibrate%s crazily for a second.",
                wpn->name(DESC_YOUR).c_str(),
                wpn->quantity > 1 ? "" : "s");
            return spret::abort;
        }
    }

    // If the wielded weapon should not be cloned, abort
    if (!wpn || !weapon_can_be_spectral(wpn, spell))
        return spret::abort;

    fail_check();

    // Remove any existing spectral weapons. Only one should be alive at any
    // given time.
    monster *old_mons = find_spectral_weapon(agent);
    if (old_mons)
        end_spectral_weapon(old_mons, false);

    mgen_data mg(MONS_SPECTRAL_WEAPON,
                 agent->is_player() ? BEH_FRIENDLY
                                    : SAME_ATTITUDE(agent->as_monster()),
                 agent->pos(),
                 agent->mindex());
    mg.set_summoned(agent, dur, SPELL_SPECTRAL_WEAPON, god);
    mg.props[TUKIMA_WEAPON] = *wpn;
    mg.props[TUKIMA_POWER] = pow;

    monster *mons = create_monster(mg);
    if (!mons)
    {
        //if (agent->is_player())
            canned_msg(MSG_NOTHING_HAPPENS);

        return spret::success;
    }

    if (agent->is_player())
        mpr("You draw out your weapon's spirit!");
    else
    {
        if (you.can_see(*agent) && you.can_see(*mons))
        {
            string buf = " draws out ";
            buf += agent->pronoun(PRONOUN_POSSESSIVE);
            buf += " weapon's spirit!";
            simple_monster_message(*agent->as_monster(), buf.c_str());
        }
        else if (you.can_see(*mons))
            simple_monster_message(*mons, " appears!");

        mons->props["band_leader"].get_int() = agent->mid;
        mons->foe = agent->mindex();
        mons->target = agent->pos();
    }

    mons->summoner = agent->mid;
    agent->props["spectral_weapon"].get_int() = mons->mid;

    return spret::success;
}

void end_spectral_weapon(monster* mons, bool killed, bool quiet)
{
    // Should only happen if you dismiss it in wizard mode, I think
    if (!mons)
        return;

    actor *owner = actor_by_mid(mons->summoner);

    if (owner)
        owner->props.erase("spectral_weapon");

    if (!quiet)
    {
        if (you.can_see(*mons))
        {
            simple_monster_message(*mons, " fades away.",
                                   MSGCH_MONSTER_DAMAGE, MDAM_DEAD);
        }
        else if (owner && owner->is_player())
            mpr("You feel your bond with your spectral weapon wane.");
    }

    if (!killed)
        monster_die(*mons, KILL_RESET, NON_MONSTER);
}

bool trigger_spectral_weapon(actor* agent, const actor* target)
{
    monster *spectral_weapon = find_spectral_weapon(agent);

    // Don't try to attack with a nonexistent spectral weapon
    if (!spectral_weapon || !spectral_weapon->alive())
    {
        agent->props.erase("spectral_weapon");
        return false;
    }

    // Likewise if the target is the spectral weapon itself
    if (target->as_monster() == spectral_weapon)
        return false;

    // Clear out any old orders.
    reset_spectral_weapon(spectral_weapon);

    spectral_weapon->props[SW_TARGET_MID].get_int() = target->mid;
    spectral_weapon->props[SW_READIED] = true;

    return true;
}

// Called at the start of each round. Cancels attack order given in the
// previous round, if the weapon was not able to execute them fully
// before the next player action
void reset_spectral_weapon(monster* mons)
{
    if (!mons || mons->type != MONS_SPECTRAL_WEAPON)
        return;

    if (mons->props.exists(SW_TRACKING))
    {
        mons->props.erase(SW_TRACKING);
        mons->props.erase(SW_READIED);
        mons->props.erase(SW_TARGET_MID);

        return;
    }

    // If an attack has been readied, begin tracking.
    if (mons->props.exists(SW_READIED))
        mons->props[SW_TRACKING] = true;
    else
        mons->props.erase(SW_TARGET_MID);
}

/* Confirms the spectral weapon can and will attack the given defender.
 *
 * Checks the target, and that we haven't attacked yet.
 * Then consumes our ready state, preventing further attacks.
 */
bool confirm_attack_spectral_weapon(monster* mons, const actor *defender)
{
    // No longer tracking towards the target.
    mons->props.erase(SW_TRACKING);

    // Is the defender our target?
    if (mons->props.exists(SW_TARGET_MID)
        && (mid_t)mons->props[SW_TARGET_MID].get_int() == defender->mid
        && mons->props.exists(SW_READIED))
    {
        // Consume our ready state and attack
        mons->props.erase(SW_READIED);
        return true;
    }

    // Expend the weapon's energy, as it can't attack
    int energy = mons->action_energy(EUT_ATTACK);
    ASSERT(energy > 0);

    mons->speed_increment -= energy;

    return false;
}

static void _setup_infestation(bolt &beam, int pow)
{
    beam.name         = "infestation";
    beam.aux_source   = "infestation";
    beam.flavour      = BEAM_INFESTATION;
    beam.glyph        = dchar_glyph(DCHAR_FIRED_BURST);
    beam.colour       = GREEN;
    beam.source_id    = MID_PLAYER;
    beam.thrower      = KILL_YOU;
    beam.is_explosion = true;
    beam.ex_size      = 2;
    beam.ench_power   = pow;
    beam.origin_spell = SPELL_INFESTATION;
}

spret cast_infestation(int pow, bolt &beam, bool fail)
{
    if (cell_is_solid(beam.target))
    {
        canned_msg(MSG_SOMETHING_IN_WAY);
        return spret::abort;
    }

    fail_check();

    _setup_infestation(beam, pow);
    mpr("You call forth a plague of scarabs!");
    beam.explode();

    return spret::success;
}

struct summon_cap
{
    int type_cap;
    int timeout;
};

// BCADDO: Summon XP Spells:
/*SPELL_SUMMON_JUNGLE,
SPELL_MALIGN_GATEWAY,
SPELL_HAUNT,
SPELL_SUMMON_ICE_BEAST,
SPELL_CALL_CHAMELEON,
SPELL_SHADOW_CREATURES,
SPELL_CALL_CANINE_FAMILIAR,
SPELL_SUMMON_BUTTERFLIES,
SPELL_MONSTROUS_MENAGERIE,
SPELL_CALL_LOST_SOULS,
SPELL_SUMMON_FOREST,
SPELL_SUMMON_LIGHTNING_SPIRE,
SPELL_SPELLFORGED_SERVITOR,
SPELL_SUMMON_MANA_VIPER, */

// spell type, cap, timeout
static const map<spell_type, summon_cap> summonsdata =
{
    // Beasts
    { SPELL_SUMMON_BUTTERFLIES,         { 8, 5 } },
    { SPELL_SUMMON_SMALL_MAMMAL,        { 4, 2 } },
    { SPELL_STICKS_TO_SNAKES,           { 4, 3 } },
    { SPELL_CALL_CANINE_FAMILIAR,       { 1, 2 } },
    { SPELL_CALL_CHAMELEON,             { 4, 3 } },
    { SPELL_SUMMON_ICE_BEAST,           { 3, 3 } },
    { SPELL_SUMMON_HYDRA,               { 3, 2 } },
    { SPELL_SUMMON_MANA_VIPER,          { 2, 2 } },
    { SPELL_SUMMON_JUNGLE,              { 8, 8 } },
    // Demons
    { SPELL_CALL_IMP,                   { 3, 3 } },
    { SPELL_SUMMON_DEMON,               { 3, 2 } },
    { SPELL_SUMMON_GREATER_DEMON,       { 3, 2 } },
    // General monsters
    { SPELL_MONSTROUS_MENAGERIE,        { 3, 2 } },
    { SPELL_SUMMON_HORRIBLE_THINGS,     { 8, 8 } },
    { SPELL_SHADOW_CREATURES,           { 4, 2 } },
    { SPELL_SUMMON_LIGHTNING_SPIRE,     { 1, 2 } },
    { SPELL_SPELLFORGED_SERVITOR,       { 1, 2 } },
    // Monster spells
    { SPELL_SUMMON_UFETUBUS,            { 8, 2 } },
    { SPELL_SUMMON_HELL_BEAST,          { 8, 2 } },
    { SPELL_SUMMON_UNDEAD,              { 8, 2 } },
    { SPELL_SUMMON_GREATER_UNDEAD,      { 3, 2 } },
    { SPELL_SUMMON_DRAKES,              { 4, 2 } },
    { SPELL_SUMMON_MUSHROOMS,           { 8, 2 } },
    { SPELL_SUMMON_EYEBALLS,            { 4, 2 } },
    { SPELL_WATER_ELEMENTALS,           { 3, 2 } },
    { SPELL_FIRE_ELEMENTALS,            { 3, 2 } },
    { SPELL_EARTH_ELEMENTALS,           { 3, 2 } },
    { SPELL_AIR_ELEMENTALS,             { 3, 2 } },
    { SPELL_SUMMON_SPECTRAL_ORCS,       { 3, 2 } },
    { SPELL_FIRE_SUMMON,                { 4, 2 } },
    { SPELL_SUMMON_MINOR_DEMON,         { 3, 3 } },
    { SPELL_CALL_LOST_SOULS,            { 8, 2 } },
    { SPELL_SUMMON_VERMIN,              { 5, 2 } },
    { SPELL_FORCEFUL_INVITATION,        { 3, 1 } },
    { SPELL_PLANEREND,                  { 6, 1 } },
    { SPELL_SUMMON_DRAGON,              { 4, 8 } },
    { SPELL_PHANTOM_MIRROR,             { 4, 1 } },
    { SPELL_FAKE_MARA_SUMMON,           { 2, 1 } },
    { SPELL_SUMMON_EMPEROR_SCORPIONS,   { 6, 2 } },
    { SPELL_SUMMON_SCARABS,             { 8, 1 } },
    { SPELL_SUMMON_HOLIES,              { 4, 2 } },
    { SPELL_SUMMON_OXARMORDTHAR,        { 3, 1 } },
    { SPELL_AWAKEN_EARTH,               { 9, 2 } },
    { SPELL_GREATER_SERVANT_MAKHLEB,    { 1, 2 } },
};

bool summons_are_capped(spell_type spell)
{
    ASSERT_RANGE(spell, 0, NUM_SPELLS);
    return summonsdata.count(spell);
}

int summons_limit(spell_type spell)
{
    const summon_cap *cap = map_find(summonsdata, spell);
    return cap ? cap->type_cap : 0;
}

static bool _spell_has_variable_cap(spell_type spell)
{
    return spell == SPELL_SHADOW_CREATURES
           || spell == SPELL_MONSTROUS_MENAGERIE;
}

static void _expire_capped_summon(monster* mon, int delay, bool recurse)
{
    // Timeout the summon
    mon_enchant abj = mon->get_ench(ENCH_ABJ);
    abj.duration = delay;
    mon->update_ench(abj);
    // Mark our cap abjuration so we don't keep abjuring the same
    // one if creating multiple summons (also, should show a status light).
    mon->add_ench(ENCH_SUMMON_CAPPED);

    if (recurse && mon->props.exists("summon_id"))
    {
        const int summon_id = mon->props["summon_id"].get_int();
        for (monster_iterator mi; mi; ++mi)
        {
            // Summoner check should be technically unnecessary, but saves
            // scanning props for all monsters on the level.
            if (mi->summoner == mon->summoner
                && mi->props.exists("summon_id")
                && mi->props["summon_id"].get_int() == summon_id
                && !mi->has_ench(ENCH_SUMMON_CAPPED))
            {
                _expire_capped_summon(*mi, delay, false);
            }
        }
    }
}

// Call when a monster has been summoned, to manage this summoner's caps.
void summoned_monster(const monster *mons, const actor *caster,
                      spell_type spell)
{
    const summon_cap *cap = map_find(summonsdata, spell);
    if (!cap) // summons aren't capped
        return;

    int max_this_time = cap->type_cap;

    // Cap large abominations and tentacled monstrosities separately
    if (spell == SPELL_SUMMON_HORRIBLE_THINGS)
    {
        max_this_time = (mons->type == MONS_ABOMINATION_LARGE ? max_this_time * 3 / 4
                                                              : max_this_time * 1 / 4);
    }

    if (spell == SPELL_SUMMON_JUNGLE && mons->type == MONS_THORN_HUNTER)
        max_this_time = 2;

    monster* oldest_summon = 0;
    int oldest_duration = 0;

    // Linked summons that have already been counted once
    set<int> seen_ids;

    int count = 1;
    for (monster_iterator mi; mi; ++mi)
    {
        if (mons == *mi)
            continue;

        int duration = 0;
        int stype    = 0;
        const bool summoned = mi->is_summoned(&duration, &stype);
        if (summoned && stype == spell && caster->mid == mi->summoner
            && mons_aligned(caster, *mi))
        {
            // Count large abominations and tentacled monstrosities separately
            if (spell == SPELL_SUMMON_HORRIBLE_THINGS && mi->type != mons->type)
                continue;

            // Summon jungle has two caps 8 for Elephants and Anacondas; 2 for Thorn Hunters.
            if (spell == SPELL_SUMMON_JUNGLE && mons->type == MONS_THORN_HUNTER && mi->type != MONS_THORN_HUNTER)
                continue;

            if (spell == SPELL_SUMMON_JUNGLE && mons->type != MONS_THORN_HUNTER && mi->type == MONS_THORN_HUNTER)
                continue;

            if (_spell_has_variable_cap(spell) && mi->props.exists("summon_id"))
            {
                const int id = mi->props["summon_id"].get_int();

                // Skip any linked summon whose set we have seen already,
                // otherwise add it to the list of seen summon IDs
                if (seen_ids.find(id) == seen_ids.end())
                    seen_ids.insert(id);
                else
                    continue;
            }

            count++;

            // If this summon is the oldest (well, the closest to expiry)
            // remember it (unless already expiring due to a cap)
            if (!mi->has_ench(ENCH_SUMMON_CAPPED)
                && (!oldest_summon || duration < oldest_duration))
            {
                oldest_summon = *mi;
                oldest_duration = duration;
            }
        }
    }

    if (oldest_summon && count > max_this_time)
        _expire_capped_summon(oldest_summon, cap->timeout * 5, true);
}

int count_summons(const actor *summoner, spell_type spell)
{
    int count = 0;
    for (monster_iterator mi; mi; ++mi)
    {
        if (summoner == *mi)
            continue;

        int stype    = 0;
        const bool summoned = mi->is_summoned(nullptr, &stype);
        if (summoned && stype == spell && summoner->mid == mi->summoner
            && mons_aligned(summoner, *mi))
        {
            count++;
        }
    }

    return count;
}

spret cast_foxfire(actor &agent, int pow, god_type god, bool fail)
{
    fail_check();
    int created = 0;

    bool chaos = determine_chaos(&you, SPELL_FOXFIRE);

    for (fair_adjacent_iterator ai(agent.pos()); ai; ++ai)
    {
        const auto att = agent.is_player() ? BEH_FRIENDLY
                                           : SAME_ATTITUDE(agent.as_monster());
        mgen_data fox(chaos ? MONS_EPHEMERAL_SPIRIT : MONS_FOXFIRE, att,
                      *ai, MHITNOT, MG_FORCE_PLACE | MG_AUTOFOE);
        fox.set_summoned(&agent, 0, SPELL_FOXFIRE, god);
        fox.hd = pow;
        monster *foxfire;

        if (!cell_is_solid(*ai) && !monster_at(*ai)
            && (foxfire = create_monster(fox)))
        {
            ++created;
            foxfire->add_ench(ENCH_SHORT_LIVED);
            foxfire->steps_remaining = you.current_vision + 2;

            // Avoid foxfire without targets always moving towards (0,0)
            if (!foxfire->get_foe()
                || !foxfire->get_foe()->is_monster() && !agent.is_monster())
            {
                set_random_target(foxfire);
            }
        }

        if (created == 2)
            break;
    }
    
    if (created)
    {
        if (chaos)
        {
            mprf("%s pull%s some ephemeral spirits from the void!",
                agent.name(DESC_THE).c_str(),
                agent.is_monster() ? "s" : "");
        }
        else
        {
            mprf("%s conjure%s some foxfire!",
                agent.name(DESC_THE).c_str(),
                agent.is_monster() ? "s" : "");
        }
    }
    else if (agent.is_player())
        canned_msg(MSG_NOTHING_HAPPENS);

    return spret::success;
}