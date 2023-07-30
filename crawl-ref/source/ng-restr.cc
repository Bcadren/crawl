/**
 * @file
 * @brief Character choice restrictions.
 *
 * The functions in this file are "pure": They don't
 * access any global data.
**/

#include "AppHdr.h"

#include "ng-restr.h"

#include "item-prop.h"
#include "jobs.h"
#include "newgame.h"
#include "newgame-def.h"
#include "season.h"
#include "size-type.h"
#include "species.h"

static bool _banned_combination(job_type job, species_type species)
{
    switch (job)
    {
    case JOB_BERSERKER:
        if (species_undead_type(species) != US_ALIVE
            || species == SP_FORMICID
            || species == SP_FAIRY)
        {
            return true;
        }
        break;
    case JOB_MERFOLK:
        if (species == SP_OCTOPODE)
            return true;
        // Fallthrough
    case JOB_CENTAUR:
    case JOB_NAGA:
    case JOB_GLADIATOR:
    case JOB_HUNTER:
    case JOB_ARCANE_MARKSMAN:
        if (species == SP_FELID)
            return true;
        // Fallthrough
    case JOB_FIGHTER:
    case JOB_MONK:
    case JOB_ENCHANTER:
    case JOB_SKALD:
        if (species == SP_FAIRY)
            return true;
        break;
    case JOB_MUMMY:
        if (species == SP_GARGOYLE || species == SP_MOLTEN_GARGOYLE || species == SP_OOZOMORPH || species == SP_LIGNIFITE)
            return true;
    case JOB_VINE_STALKER:
        if (species == SP_LIGNIFITE)
            return true;
        // fallthrough
    case JOB_DEMONSPAWN:
        if (species_undead_type(species) == US_GHOST)
            return true;
        // fallthrough
    case JOB_TRANSMUTER:
        if (species_undead_type(species) == US_UNDEAD
            || species_undead_type(species) == US_HUNGRY_DEAD
            || species == SP_FAIRY)
        {
            return true;
        }
        break;
    case JOB_JESTER:
        if (!is_april_fools())
            return true;
        if (species == SP_FAIRY || species == SP_FELID || species == SP_DRACONIAN)
            return true;
    default:
        break;
    }

    return false;
}

char_choice_restriction species_allowed(job_type job, species_type speci)
{
    if (!is_starting_species(speci) || !is_starting_job(job))
        return CC_BANNED;

    if (_banned_combination(job, speci))
        return CC_BANNED;

    if (job_recommends_species(job, speci))
        return CC_UNRESTRICTED;

    return CC_RESTRICTED;
}

bool character_is_allowed(species_type species, job_type job)
{
    return !_banned_combination(job, species);
}

char_choice_restriction job_allowed(species_type speci, job_type job)
{
    if (!is_april_fools() && job == JOB_JESTER)
        return CC_BANNED;

    if ((!is_starting_species(speci) && speci != SP_UNKNOWN) || !is_starting_job(job))
        return CC_BANNED;

    if (_banned_combination(job, speci))
        return CC_BANNED;

    if (species_recommends_job(speci, job))
        return CC_UNRESTRICTED;

    return CC_RESTRICTED;
}

// Is the given god restricted for the character defined by ng?
// Only uses ng.species and ng.job.
char_choice_restriction weapon_restriction(weapon_type wpn,
                                           const newgame_def &ng)
{
    ASSERT(is_starting_species(ng.species));
    ASSERT(is_starting_job(ng.job));

    // Some special cases:

    if (ng.species == SP_FELID && wpn != WPN_UNARMED)
        return CC_BANNED;

    if (species_recommends_weapon(ng.species, wpn))
        return CC_UNRESTRICTED;

    if (wpn != WPN_UNARMED && wpn != WPN_UNKNOWN &&
        (ng.job == JOB_MERFOLK) && (wpn_skill(wpn) == SK_SHORT_BLADES))
    {
        return CC_UNRESTRICTED;
    }

    return CC_RESTRICTED;
}
