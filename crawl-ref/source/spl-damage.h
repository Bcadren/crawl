#pragma once

#include <vector>

#include "beam-type.h"
#include "enum.h"
#include "mpr.h"
#include "spl-cast.h"
#include "zap-type.h"

using std::vector;

struct bolt;
struct dice_def;
class dist;

void setup_fire_storm(const actor *source, int pow, bolt &beam);
spret cast_fire_storm(int pow, bolt &beam, bool fail);
bool cast_smitey_damnation(int pow, bolt &beam);
spret cast_chain_spell(spell_type spell_cast, int pow,
                            const actor *caster, bool fail = false);
spret cast_torment(bool fail);
string desc_chain_lightning_dam(int pow);

spret trace_los_attack_spell(spell_type spell, int pow,
                                  const actor* agent);
spret fire_los_attack_spell(spell_type spell, int pow, actor* agent,
                                 bool fail = false,
                                 int* damage_done = nullptr);
void sonic_damage(bool scream);
bool mons_shatter(monster* caster, bool actual = true);
void shillelagh(actor *wielder, coord_def where, int pow);
spret vampiric_drain(int pow, monster* mons, bool fail);
spret cast_freeze(int pow, monster* mons, bool fail);
spret cast_malign_offering(int pow, const dist &beam, bool fail);
spret cast_airstrike(int pow, const dist &beam, bool fail);
spret cast_shatter(int pow, bool fail);
spret cast_irradiate(int powc, actor* who, bool fail);
spret cast_heal_blast(int powc, actor* who, bool fail);
dice_def irradiate_damage(int powc, actor* who, bool chaos = false);
bool ignite_poison_affects(const actor* act);
spret cast_ignite_poison(actor *agent, int pow, bool fail,
                              bool tracer = false, bool olgreb = false);
bool safe_discharge(coord_def where, vector<const actor *> &exclude);
spret cast_discharge(int pow, const actor &agent, bool fail = false,
                          bool prompt = true);
bool setup_fragmentation_beam(bolt &beam, int pow, const actor *caster,
                              const coord_def target, bool quiet,
                              const char **what, bool &hole, bool &destroy);
spret cast_fragmentation(int powc, const actor *caster,
                              const coord_def target, bool fail);
spret cast_tornado(int powc, bool fail);
spret warped_cast(zap_type zap, int pow, bolt target, actor * caster);
void tornado_damage(actor *caster, int dur, bool is_vortex = false);
void cancel_tornado(bool tloc = false);
void tornado_move(const coord_def &pos);
void cloud_strike(actor *caster, actor *foe, int damage);
spret cast_thunderbolt(actor *caster, int pow, coord_def aim,
                            bool fail);
void additional_flavoured_damage(actor *caster, actor *foe, int preres, 
                            beam_type damtype, string verb, string noun);
bool allied_chameleon(const actor * caster, const actor * act);

actor* forest_near_enemy(const actor *mon);
void forest_message(const coord_def pos, const string &msg,
                    msg_channel_type ch = MSGCH_PLAIN);
void forest_damage(actor *mon);

vector<bolt> get_spray_rays(const actor *caster, coord_def aim, int range,
                            int max_rays, int max_spacing = 3);
spret cast_dazzling_spray(int pow, coord_def aim, bool fail);

spret cast_toxic_radiance(actor *caster, int pow, bool fail = false,
                               bool mon_tracer = false);
void toxic_radiance_effect(actor* agent, int mult, bool on_cast = false, bool chaos = false);
void sickening_aura(const int delay);

spret cast_searing_ray(int pow, bolt &beam, bool fail);
void handle_searing_ray();
void end_searing_ray();

spret cast_glaciate(actor *caster, int pow, coord_def aim,
                         bool fail = false);

spret cast_random_bolt(int pow, bolt& beam, bool fail = false);

size_t shotgun_beam_count(int pow);
spret cast_scattershot(const actor *caster, int pow, const coord_def &pos,
                            bool fail = false, zap_type zap = ZAP_SCATTERSHOT,
                            bool empowered = false);
vector<coord_def> get_ignition_blast_sources(const actor *agent);

spret cast_cascade(const actor *caster, int pow, bool fail);
spret cast_dash(int pow, bool fail);

spret cast_borgnjors_vile_clutch(int pow, bolt &beam, bool fail);

spret cast_starburst(int pow, bool fail, bool tracer = false, bool frostburst = false);
void foxfire_attack(const monster *foxfire, const actor *target);

spret cast_hailstorm(int pow, bool fail, bool tracer = false);

spret cast_imb(int pow, bool fail);

void actor_apply_toxic_bog(actor *act);
