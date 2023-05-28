/**
 * @file
 * @brief Functions for imposing mummy death curses.
 *
 * This is made very complicated because one source of death curses is the
 * scythe (so we have to keep a source around for kill type tracking and
 * messaging) and I (ebering) like the item too much to remove it in the name
 * of code simplification.
 *
 * Curses are currently used by:
 * - Dying mummies
 * - Kiku wrath
 * - Scythe of curses
 */
#include "AppHdr.h"

#include "death-curse.h"

#include "actor.h"
#include "areas.h"
#include "attack.h"
#include "beam.h"
#include "beam-type.h"
#include "description-level-type.h"
#include "externs.h"
#include "fight.h"
#include "god-passive.h"
#include "killer-type.h"
#include "message.h"
#include "mgen-data.h"
#include "mon-place.h"
#include "monster.h"
#include "mon-death.h"
#include "mon-enum.h"
#include "mon-util.h"
#include "mount.h"
#include "mpr.h"
#include "ouch.h"
#include "player.h"
#include "player-stats.h"
#include "random.h"
#include "shout.h"
#include "species-type.h"
#include "spl-goditem.h"
#include "stat-type.h"
#include "stringutil.h"

static void _do_msg(actor& target, bool mount, string player_msg, string mon_seen_msg,
                    string mon_unseen_msg)
{
    if (mount && !mon_seen_msg.empty())
    {
        string cap_str = make_stringf("Your %s", you.mount_name(true).c_str());
        string low_str = make_stringf("your %s", you.mount_name(true).c_str());
        string msg = replace_all(mon_seen_msg, "@the_monster@", low_str);
        msg = replace_all(msg, "@The_monster@", cap_str);
        mpr(msg);
    }
    else if (target.is_player() && !player_msg.empty())
        mpr(player_msg);
    else if (you.can_see(target) && !mon_seen_msg.empty())
    {
        mpr(do_mon_str_replacements(mon_seen_msg,
            *target.as_monster(), S_SILENT));
    }
    else if (!mon_unseen_msg.empty())
    {
        mpr(do_mon_str_replacements(mon_unseen_msg,
            *target.as_monster(), S_SILENT));
    }
}

// Handle applying damage for death-curse effects
static void _ouch(actor& target, const actor * source, int dam,
                  const string cause, bool mt)
{
    if (mt)
    {
        damage_mount(dam);
        return;
    }

    killer_type kt;

    if (source && source->is_player())
        kt = KILL_YOU_MISSILE;
    else if (source && source->is_monster())
        if (source->as_monster()->confused_by_you()
            && !source->as_monster()->friendly())
        {
            kt = KILL_YOU_CONF;
        }
        else
            kt = KILL_MON_MISSILE;
    else
        kt = KILL_MISCAST;

    if (target.is_monster())
    {
        monster* mon_target = target.as_monster();

        // curse damage is unresistable (torment flavoured, rtorm is
        // checked earlier for messaging reasons)
        mon_target->hurt(source, dam, BEAM_TORMENT_DAMAGE, KILLED_BY_BEAM,
                         "", "", false);

        if (!mon_target->alive())
            monster_die(*mon_target, kt, actor_to_death_source(source));
    }
    else
    {
        bool see_source = source && you.can_see(*source);
        ouch(dam, KILLED_BY_SOMETHING, source ? source->mid : MID_NOBODY,
             cause.c_str(), see_source,
             source ? source->name(DESC_A, true).c_str() : nullptr);
    }
}

struct  curse_effect
{
    string name;
    function<void (actor& target, actor* source,
             string cause, int severity, bool mount)> effect;
    int trivial_weight; // Weight at severity 0
    int severe_weight;  // Weight at severity 15. Linearly interpolated
};

static void _curse_message(actor& target, actor* /*source*/,
                           string /*cause*/, int /*severity*/, bool /*mount*/)
{
    // Avoid message spam
    if (!target.is_player())
        return;

    vector<string> messages = {
        "You feel homesick.",
        "The world around you seems to dim momentarily.",
        "You feel numb.",
        "Strange energies run through your body.",
        "You shiver with cold.",
        "You sense a malignant aura.",
        "You feel very uncomfortable.",
        "Something just walked over your grave. No, really!",
    };

    if (you.can_smell())
        messages.push_back("You smell decay.");

    if (you.char_class == JOB_MUMMY && you.species != SP_DRACONIAN)
        messages.push_back("Your bandages flutter.");

    if (!silenced(you.pos()))
        messages.push_back("You hear strange and distant voices.");

    if (!(you.species == SP_OCTOPODE || you.species == SP_FORMICID
        || you.species == SP_FAIRY || you.species == SP_GARGOYLE
        || you.species == SP_LIGNIFITE || you.species == SP_SILENT_SPECTRE
        || you.species == SP_OOZOMORPH || you.species == SP_MOLTEN_GARGOYLE))
    {
        messages.push_back("Your bones ache.");
    }

    mpr(*random_iterator(messages));
}

/** Table of possible curse effects
 * Severity scales from 0 up to 27. For mummies this uses their HD:
 * Guardian 7, Priest 10, Greater 15, and Khufu 18.
 * For Kiku wrath this uses player XL, and for the scythe of curses this uses
 * damage done (capped at 27).
 *
 * The severity function in each is linearly interpolating between the
 * following two sets of weights, with the weight of "message" kept at 0 for
 * severities higher than 15.
 *
 * | severity | message | pain | elementals | rot |slow | drain | torment |
 * | -------- | ------- | ---- | ---------- | --- |---- | ----- | ------- |
 * | 0        | 80      | 20   | 15         | 10  | 0   | 0     | 0       |
 * | 15       | 0       | 40   | 45         | 20  | 20  | 20    | 20      |
 *
 * Pain damage, slow duration, and drain effect all scale with severity.
 *
 * Kiku curse protection halves the severity when partially averting the curse,
 * making Khufu's curse a bit weaker than a mummy priest's.
 */
static const vector<curse_effect> curse_effects = {
    {
        "message",
        _curse_message,
        80, 0,
    },
    {
        "pain",
        [](actor& target, actor* source, string cause, int severity, bool mt) {
            if (target.res_torment(mt) || target.res_negative_energy(mt) >= 3)
            {
                _do_msg(target, mt, "You feel weird for a moment.",
                        "@The_monster@ has a weird expression for a moment.",
                        "Something is bathed in an unholy light.");
                return;
            }
            else
            {
                int dmg = 8 + random2avg(3*severity,2);
                dmg = resist_adjust_damage(&target, BEAM_NEG, dmg, mt);
                string punct = attack_strength_punctuation(dmg);
                _do_msg(target, mt, "Pain shoots through your body" + punct,
                        "@The_monster@ convulses with pain" + punct,
                        "Something is bathed in an unholy light" + punct);
                _ouch(target, source, dmg, cause, mt);
            }
        },
        20, 40,
    },
    {
        "rot",
        [](actor& target, actor* source, string /*cause*/, int severity, bool mount) {
            if (mount)
                rot_mount(severity / 3 + random2(severity));
            else
                target.rot(source, severity / 3 + random2(severity));
        },
        10, 20,
    },
    {
        "elementals",
        [](actor& target, actor* source, string cause, int severity, bool mount) {

            monster_type type = MONS_REAPER;

            if (target.is_player() && you.experience_level < 21
                || x_chance_in_y(30 - severity, 30))
            {
                if (coinflip() || bool(target.holiness() & MH_UNDEAD))
                    type = MONS_ROT_ELEMENTAL;
                else
                    type = MONS_PAIN_ELEMENTAL;
            }

            if (target.is_player() && you.experience_level < 12
                || x_chance_in_y(15 - severity, 15))
            {
                type = MONS_SHADOW;
            }

            mgen_data data = mgen_data::hostile_at(type, true, target.pos());
            data.set_summoned(source ? source : nullptr, 4, SPELL_NO_SPELL);
            data.set_non_actor_summoner(cause);
            data.extra_flags |= (MF_NO_REWARD | MF_HARD_RESET);

            if (target.is_monster())
            {
                monster* mon_target = target.as_monster();

                switch (mon_target->temp_attitude())
                {
                case ATT_FRIENDLY:     data.behaviour = BEH_HOSTILE; break;
                case ATT_HOSTILE:      data.behaviour = BEH_FRIENDLY; break;
                case ATT_GOOD_NEUTRAL:
                case ATT_NEUTRAL:
                case ATT_STRICT_NEUTRAL:
                case ATT_PASSIVE:
                    data.behaviour = BEH_NEUTRAL;
                    break;
                }
            }

            monster * x = create_monster(data, false);

            if (x && x->defined())
            {
                switch (type)
                {
                default:
                case MONS_REAPER:
                    _do_msg(target, mount, "Death has come for you...", "Death has come for @the_monster@...", "An agent of death appears from thin air...");
                    break;
                case MONS_PAIN_ELEMENTAL:
                    noisy(30, target.pos());
                    mpr("Pain screams into solid form...");
                    break;
                case MONS_ROT_ELEMENTAL:
                    _do_msg(target, mount, "Decay reaches out for you...", "Decay reaches out for @the_monster@...", "Decay forms out of thin air...");
                    break;
                case MONS_SHADOW:
                    _do_msg(target, mount, "A shadow passes over your grave...", "A shadow passes over @the_monster@...", "A shadow flickers into being...");
                    break;
                }
            }
        },
        15, 45,
    },
    {
        "slow",
        [](actor& target, actor* source, string /*cause*/, int severity, bool mount) {
            _do_msg(target, mount,
                    "You feel horribly lethargic.",
                    "@The_monster@ looks incredibly listless.",
                    "");
            if (mount)
                slow_mount(severity);
            else
                target.slow_down(source, severity);
        },
        0, 40,
    },
    {
        "drain",
        [](actor& target, actor* source, string /*cause*/, int severity, bool mount) {
            _do_msg(target, mount,
                    "You are engulfed in negative energy!",
                    "@The_monster@ is engulfed in negative energy!",
                    "Something is engulfed in negative energy!");
            if (mount)
                drain_mount(severity);
            else if (target.is_player() && x_chance_in_y(severity, 27))
                lose_stat(STAT_RANDOM, 1 + random2avg(severity / 3, 2));
            else
                target.drain_exp(source, false, ( severity * 100 ) / 27);
        },
        0, 40,
    },
    {
        "torment",
        [](actor& target, actor* source,
           string /*cause*/, int /*severity*/, bool /*mount*/) {
            torment_cell(target.pos(), source, TORMENT_MISCAST);
        },
        0, 40,
    },
};

void death_curse(actor& target, actor* source, string cause, int severity, bool mount_hit)
{
    if (target.is_player()
        && have_passive(passive_t::miscast_protection_necromancy))
    {
        if (coinflip())
        {
            simple_god_message(" averts the curse.");
            return;
        }
        else
        {
            simple_god_message(" partially averts the curse.");
            severity = severity / 2;
        }
    }

    vector<pair<const curse_effect&, int>> weights;
    for (const curse_effect& curse : curse_effects)
    {
        const int w = ((15 - severity) * curse.trivial_weight
                       + severity * curse.severe_weight) / 15;
        weights.push_back({curse, w});
    }

    random_choose_weighted(weights)->effect(target, source, cause, severity, mount_hit);
}
