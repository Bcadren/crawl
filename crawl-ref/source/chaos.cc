
#include "AppHdr.h"

#include "chaos.h"

#include "losglobal.h"
#include "mon-cast.h"
#include "mon-clone.h"
#include "mon-ench.h"
#include "mpr.h"
#include "mutation.h"
#include "random.h"
#include "religion.h"
#include "shout.h"
#include "spl-miscast.h"
#include "traps.h"
#include "xom.h"

beam_type eldritch_damage_type()
{
    return random_choose_weighted(
        1, BEAM_FIRE,
        1, BEAM_COLD,
        3, BEAM_NEG,
        3, BEAM_ACID,
        4, BEAM_DAMNATION,
        3, BEAM_DEVASTATION,
        1, BEAM_ELECTRICITY);
}

beam_type chaos_damage_type(bool player)
{
    beam_type retval;
    retval = random_choose_weighted(4, BEAM_FIRE,
        4, BEAM_COLD,
        4, BEAM_NEG,
        4, BEAM_ACID,
        1, BEAM_DAMNATION,
        6, BEAM_DEVASTATION,
        3, BEAM_ELECTRICITY);
    if (player && is_good_god(you.religion) && (retval == BEAM_NEG || retval == BEAM_DAMNATION))
        return BEAM_HOLY;
    return retval;
}

enum chaotic_buff_type
{
    CB_CLONE = 0x0001,
    CB_HASTE = 0x0002,
    CB_POLY = 0x0004,
    CB_MIGHT = 0x0008,
    CB_AGIL = 0x0010,
    CB_BRILL = 0x0020,
    CB_INVIS = 0x0040,
    CB_SHAPESHIFT = 0x0080,
    CB_ICE_ARMOUR = 0x0100,
    CB_SWIFT = 0x0200,
    CB_REGEN = 0x0400,
    CB_BERSERK = 0x0800,
};

enum chaotic_debuff_type
{
    CD_PETRIFY = 0x00001,
    CD_MISCAST = 0x00002,
    CD_POLY = 0x00004,
    CD_STICKY = 0x00008,
    CD_FROZEN = 0x00010,
    CD_SLOW = 0x00020,
    CD_CONFUSE = 0x00040,
    CD_FEAR = 0x00080,
    CD_MUTE = 0x00100,
    CD_BANISH = 0x00200,
    CD_BLINK = 0x00400,
    CD_ENSNARE = 0x00800,
    CD_VULN = 0x01000,
    CD_FLAY = 0x02000,
    CD_STAT_DRAIN = 0x04000,
    CD_WRETCHED = 0x08000,
    CD_BLIND = 0x10000,
    CD_BARBS = 0x20000,
    CD_INNER = 0x40000,
};

void chaotic_buff(actor* act, int dur, actor * attacker)
{
    if (!act->alive())
        return;

    // Total Weight: 118 (Arbitrary)
    chaotic_buff_type buff = random_choose_weighted(
        1, CB_CLONE,
        20, CB_HASTE,
        8, CB_POLY,
        10, CB_MIGHT,
        10, CB_AGIL,
        10, CB_BRILL,
        20, CB_INVIS,
        4, CB_SHAPESHIFT,
        12, CB_ICE_ARMOUR,
        10, CB_SWIFT,
        8, CB_REGEN,
        5, CB_BERSERK);

    bool player = act->is_player();

    switch (buff)
    {
    case CB_CLONE:
        if (!player)
        {
            monster * clone = clone_mons(act->as_monster(), true);
            if (clone)
            {
                if (attacker->is_player())
                    mprf("You duplicate %s.", act->name(DESC_THE).c_str());
                else
                    mprf("%s duplicates %s.", attacker->name(DESC_THE).c_str(), act->name(DESC_THE).c_str());
                xom_is_stimulated(clone->friendly() ? 12 : 25);
                // Monsters being cloned is interesting.
            }
        }
        break;
    case CB_HASTE:
        if (player)
        {
            if (haste_player(dur, false, false))
                mprf(MSGCH_DURATION, "A spark of chaos speeds you up.");
        }
        else
        {
            act->as_monster()->add_ench(mon_enchant(ENCH_HASTE, 0, attacker, dur * BASELINE_DELAY));
            mprf(player ? MSGCH_DURATION : MSGCH_MONSTER_ENCHANT, "A spark of chaos speeds %s up.", player ? "you" : act->name(DESC_THE).c_str());
        }
        break;
    case CB_AGIL:
        mprf(player ? MSGCH_DURATION : MSGCH_MONSTER_ENCHANT, "A spark of chaos increases %s%s.",
            player ? "your agility" : "the reflexes of ", player ? "" : act->name(DESC_THE).c_str());
        if (player)
            you.increase_duration(DUR_AGILITY, dur);
        else
            act->as_monster()->add_ench(mon_enchant(ENCH_AGILE, 0, attacker, dur * BASELINE_DELAY));
        break;
    case CB_BRILL:
        if (player)
        {
            mprf(MSGCH_DURATION, "A spark of chaos makes you feel quite brilliant.");
            you.increase_duration(DUR_BRILLIANCE, dur);
        }
        else
        {
            mprf("The chaos empowers the spells of %s.", act->name(DESC_THE).c_str());
            act->as_monster()->add_ench(mon_enchant(ENCH_EMPOWERED_SPELLS, 0, attacker, dur * BASELINE_DELAY));
        }
        break;
    case CB_ICE_ARMOUR:
        mprf(player ? MSGCH_DURATION : MSGCH_MONSTER_ENCHANT, "A chaotic chill coats %s in an icy armour.",
            player ? "you" : act->name(DESC_THE).c_str());
        if (player)
            you.increase_duration(DUR_ICY_ARMOUR, dur);
        else
            act->as_monster()->add_ench(mon_enchant(ENCH_OZOCUBUS_ARMOUR, dur / 3, attacker, dur * BASELINE_DELAY));
        break;
    case CB_INVIS:
        if (player)
        {
            mprf(MSGCH_DURATION, "A chaotic spark makes you %s.", invis_allowed(true) ? "fade into invisibility" : "translucent, though still outlined in light.");
            you.increase_duration(DUR_INVIS, dur);
        }
        else
        {
            mprf("As it is struck, %s flickers %s.", act->name(DESC_THE).c_str(),
                you.can_see_invisible() ? "slightly." : "and vanishes.");
            act->as_monster()->add_ench(mon_enchant(ENCH_INVIS, 0, attacker, dur * BASELINE_DELAY));
        }
        break;
    case CB_MIGHT:
        if (player)
        {
            mprf(MSGCH_DURATION, "You feel mighty as the magic touches you.");
            you.increase_duration(DUR_MIGHT, dur);
        }
        else
        {
            mprf("%s becomes stronger as the magic touches it.", act->name(DESC_THE).c_str());
            act->as_monster()->add_ench(mon_enchant(ENCH_MIGHT, 0, attacker, dur * BASELINE_DELAY));
        }
        break;
    case CB_POLY:
        act->polymorph(dur * 2, false);
        break;
    case CB_REGEN:
        if (player)
        {
            string useless = spell_uselessness_reason(SPELL_REGENERATION);
            mprf(MSGCH_DURATION, "A lively spark makes you%s.", useless.empty() ? "r skin crawl" : " magically regain health");
            you.increase_duration(DUR_REGENERATION, dur);
        }
        else
        {
            mprf("%s starts to regenerate quickly as life sparks into it.", act->name(DESC_THE).c_str());
            act->as_monster()->add_ench(mon_enchant(ENCH_REGENERATION, 0, attacker, dur * BASELINE_DELAY));
        }
        break;
    case CB_SHAPESHIFT:
        if (player)
            you.malmutate("chaos magic");
        else
        {
            mprf("%s begins to change shapes rapidly.", act->name(DESC_THE).c_str());
            act->as_monster()->add_ench(mon_enchant(dur > 30 ? ENCH_GLOWING_SHAPESHIFTER : ENCH_SHAPESHIFTER, 0, attacker, 1));
        }
        act->polymorph(dur * 3, false);
        break;
    case CB_SWIFT:
        mprf(player ? MSGCH_DURATION : MSGCH_MONSTER_ENCHANT, "Magical energy makes %s cover ground more quickly.",
            player ? "you" : act->name(DESC_THE).c_str());
        if (player)
            you.increase_duration(DUR_SWIFTNESS, dur);
        else
            act->as_monster()->add_ench(mon_enchant(ENCH_SWIFT, 0, attacker, dur * BASELINE_DELAY));
        break;
    case CB_BERSERK:
        if (player)
            you.go_berserk(false, false);
        else
            act->as_monster()->go_berserk(false, false);
        break;
    }
}

void chaotic_debuff(actor* act, int dur, actor * attacker)
{
    if (!act->alive())
        return;

    chaotic_debuff_type debuff = random_choose_weighted(
        10, CD_PETRIFY,
        60, CD_MISCAST,
        4, CD_POLY,
        16, CD_STICKY,
        8, CD_FROZEN,
        16, CD_SLOW,
        16, CD_CONFUSE,
        8, CD_FEAR,
        8, CD_MUTE,
        6, CD_BANISH,
        18, CD_BLINK,
        8, CD_ENSNARE,
        24, CD_VULN,
        18, CD_FLAY,
        12, CD_STAT_DRAIN,
        12, CD_WRETCHED,
        8, CD_BLIND,
        16, CD_BARBS,
        40, CD_INNER);

    bool player = act->is_player();

    switch (debuff)
    {
    case CD_INNER:
        if (player)
            break;
        act->as_monster()->add_ench(mon_enchant(ENCH_ENTROPIC_BURST, 0, attacker, dur * BASELINE_DELAY));
        break;
    case CD_BANISH:
        act->banish(attacker);
        break;
    case CD_BARBS:
        if (player && !you.get_mutation_level(MUT_INSUBSTANTIAL) && !(you.get_mutation_level(MUT_SLIME) >= 3))
        {
            mprf(MSGCH_WARN, "The chaotic magic splinters into barbs that impale your flesh.");
            you.increase_duration(DUR_BARBS, dur);
        }
        else if (!player && !act->as_monster()->is_insubstantial())
        {
            mprf("The chaotic magic splinters into barbs that impale %s.", act->name(DESC_THE).c_str());
            act->as_monster()->add_ench(mon_enchant(ENCH_BARBS, 0, attacker, dur * BASELINE_DELAY));
        }
        break;
    case CD_BLIND:
        if (!player)
        {
            mprf("The scintillating magical residue splatters into the eyes of %s.", act->name(DESC_THE).c_str());
            act->as_monster()->add_ench(mon_enchant(ENCH_BLIND, 0, attacker, dur * BASELINE_DELAY));
        }
        // BCADDO: If player blind is added put here.
        break;
    case CD_BLINK:
        act->blink();
        break;
    case CD_CONFUSE:
        if (act->is_player())
        {
            mprf(MSGCH_WARN, "Bemusing magic seeps into your mind...");
            confuse_player(dur);
        }
        else
        {
            mprf("Chaotic magical radiation drives %s to madness.", act->name(DESC_THE).c_str());
            act->as_monster()->add_ench(mon_enchant(ENCH_CONFUSION, 0, attacker, dur * BASELINE_DELAY));
        }
        break;
    case CD_ENSNARE:
        ensnare(act);
        break;
    case CD_FEAR:
        if (attacker == act)
            break; // Afraid of yourself? O_O;
        if (!attacker->is_player())
            attacker->as_monster()->add_ench(ENCH_FEAR_INSPIRING);
        if (act->is_player())
        {
            mprf(MSGCH_WARN, "A magical surge fills you with panic, you fear the %s.", attacker->name(DESC_THE).c_str());
            you.add_fearmonger(attacker->as_monster());
            you.increase_duration(DUR_AFRAID, dur);
        }
        else
        {
            mprf("%s cries out in fear of your magic.", act->name(DESC_THE).c_str());
            noisy(20, act->pos(), act->mid);
            act->as_monster()->add_ench(mon_enchant(ENCH_FEAR, 0, attacker, dur * BASELINE_DELAY));
        }
        break;
    case CD_FLAY:
        dur = div_rand_round(dur, 2);
        flay(*attacker->as_monster(), *act, dur);
        break;
    case CD_FROZEN:
        mprf(player ? MSGCH_WARN : MSGCH_MONSTER_ENCHANT, "A chaotic chill freezes %s in place, making %s cover ground more slowly.",
            player ? "you" : act->name(DESC_THE).c_str(), player ? "you" : act->pronoun(PRONOUN_OBJECTIVE).c_str());
        if (act->is_player())
            you.increase_duration(DUR_FROZEN, dur);
        else
            act->as_monster()->add_ench(mon_enchant(ENCH_FROZEN, 0, attacker, dur * BASELINE_DELAY));
        break;
    case CD_MISCAST:
        mprf(player ? MSGCH_WARN : MSGCH_MONSTER_DAMAGE, "Chaotic magic lashes out at %s.",
            player ? "you" : act->name(DESC_THE).c_str());
        MiscastEffect(act, attacker, { miscast_source::spell },
            spschool::random, max(1, min(div_rand_round(dur, 10), 3)), "chaotic magic",
            nothing_happens::NEVER, 0, "", false);
        break;
    case CD_MUTE:
        if (player)
        {
            mpr("You are engulfed in a profound silence.");
            you.increase_duration(DUR_SILENCE, dur);
        }
        else
        {
            if (one_chance_in(3))
            {
                mprf("Glittering chaos makes %s begin to radiant silence.", act->name(DESC_THE).c_str());
                act->as_monster()->add_ench(mon_enchant(ENCH_SILENCE, 0, attacker, dur * BASELINE_DELAY));
            }
            else
            {
                mprf("Chaos steals away with %s voice.", act->name(DESC_ITS).c_str());
                act->as_monster()->add_ench(mon_enchant(ENCH_MUTE, 0, attacker, dur * BASELINE_DELAY));
            }
        }
        invalidate_los();
        break;
    case CD_PETRIFY:
        mprf(player ? MSGCH_DANGER : MSGCH_MONSTER_DAMAGE, "Earth magic residue begins to slowly turns %s to stone.",
            player ? "you" : act->name(DESC_THE).c_str());
        act->petrify(attacker);
        break;
    case CD_POLY:
        act->polymorph(dur * 2, false);
        break;
    case CD_SLOW:
        if (player && slow_player(dur, false))
            mprf(MSGCH_WARN, "You feel yourself slow down as chaos touches upon you.");
        else
        {
            act->slow_down(attacker, dur);
            mprf("Chaos touches upon %s, slowing it down.", act->name(DESC_THE).c_str());
        }
        break;
    case CD_STAT_DRAIN:
        if (player)
            you.drain_stat(STAT_RANDOM, 1 + random2(2));
        else
            act->as_monster()->drain_exp(attacker, false, div_rand_round(dur, 8));
        break;
    case CD_STICKY:
        if (player && !you.res_sticky_flame())
        {
            mprf(MSGCH_WARN, "The chaos reforms as liquid flames that stick to you!");
            you.increase_duration(DUR_LIQUID_FLAMES, dur);
        }
        else if (!player && !act->as_monster()->res_sticky_flame())
        {
            mprf(MSGCH_MONSTER_DAMAGE, "The chaos reforms as liquid flames that stick to %s!", act->name(DESC_THE).c_str());
            act->as_monster()->add_ench(mon_enchant(ENCH_STICKY_FLAME, 0, attacker, dur * BASELINE_DELAY));
        }
        break;
    case CD_VULN:
        if (act->is_player())
        {
            duration_type vuln_type = DUR_FIRE_VULN; // Set on initialization to avoid compiler warnings.
            switch (random2(3))
            {
            case 0:                            mprf(MSGCH_WARN, "You feel more vulnerable to fire."); break;
            case 1: vuln_type = DUR_COLD_VULN; mprf(MSGCH_WARN, "You feel more vulnerable to cold."); break;
            case 2: vuln_type = DUR_ELEC_VULN; mprf(MSGCH_WARN, "You feel more vulnerable to electric shocks."); break;
            case 3: if (you.get_mutation_level(MUT_POISON_RESISTANCE) >= 3)
            {
                mprf(MSGCH_WARN, "You feel more vulnerable to fire.");
            }
                    else { vuln_type = DUR_POISON_VULN; mprf(MSGCH_WARN, "You feel more vulnerable to poison."); } break;
            case 4: vuln_type = DUR_PHYS_VULN; mprf(MSGCH_WARN, "You feel more vulnerable to physical attacks."); break;
            }
            you.increase_duration(vuln_type, dur);
        }
        else
        {
            enchant_type vuln_type = ENCH_FIRE_VULN; // Set on initialization to avoid compiler warnings.
            switch (random2(3))
            {
            case 0:                             mprf(MSGCH_MONSTER_ENCHANT, "%s appears more vulnerable to fire.", act->name(DESC_THE).c_str()); break;
            case 1: vuln_type = ENCH_COLD_VULN; mprf(MSGCH_MONSTER_ENCHANT, "%s appears more vulnerable to cold.", act->name(DESC_THE).c_str()); break;
            case 2: vuln_type = ENCH_ELEC_VULN; mprf(MSGCH_MONSTER_ENCHANT, "%s appears more vulnerable to electrical shocks.", act->name(DESC_THE).c_str()); break;
            case 3: if (act->as_monster()->holiness() & (MH_UNDEAD | MH_NONLIVING))
            {
                mprf(MSGCH_MONSTER_ENCHANT, "%s appears more vulnerable to fire.", act->name(DESC_THE).c_str());
            }
                    else { vuln_type = ENCH_POISON_VULN; mprf(MSGCH_MONSTER_ENCHANT, "%s appears more vulnerable to poison.", act->name(DESC_THE).c_str()); } break;
            case 4: vuln_type = ENCH_PHYS_VULN; mprf(MSGCH_MONSTER_ENCHANT, "%s appears more vulnerable to physical attacks.", act->name(DESC_THE).c_str()); break;
            }
            act->as_monster()->add_ench(mon_enchant(vuln_type, 0, attacker, dur * BASELINE_DELAY));
        }
        break;
    case CD_WRETCHED:
        if (player)
        {
            mprf(MSGCH_MUTATION, "The chaos corrupts your form!");
            int num_mutations = 1 + random2(3);
            for (int i = 0; i < num_mutations; ++i)
                temp_mutate(RANDOM_CORRUPT_MUTATION, "chaos magic");
        }
        else
        {
            mprf(MSGCH_MONSTER_ENCHANT, "The chaos corrupts %s form.", act->name(DESC_ITS).c_str());
            act->as_monster()->add_ench(mon_enchant(ENCH_WRETCHED, 0, attacker, dur * BASELINE_DELAY));
        }
        break;
    }
}

void chaotic_status(actor * victim, int dur, actor * source)
{
    if (!victim->alive() || one_chance_in(3))
        return;
    if (coinflip())
        chaotic_buff(victim, dur, source);
    else
        chaotic_debuff(victim, dur, source);
}