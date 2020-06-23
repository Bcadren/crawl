#pragma once

#include "spl-cast.h"

spret cast_disjunction(int pow, bool fail);
void disjunction_spell();

spret cast_blink(bool fail = false, bool force = false);
spret cast_controlled_blink(bool fail = false, bool safe = true, int power = 200);
spret blink_bolt(bool fail = false, int power = 200);
void uncontrolled_blink(bool override_stasis = false, coord_def disp_center = coord_def (0,0));
spret controlled_blink(bool fail, bool safe_cancel = true, int power = 200);
spret frog_hop(bool fail, bool spider = false);
spret rolling_charge(bool fail);
void wizard_blink();

bool dangerous_terrain_seen();

void you_teleport();
void you_teleport_now(bool wizard_tele = false, bool teleportitis = false,
                      string reason = "");
bool you_teleport_to(const coord_def where,
                     bool move_monsters = false);

spret cast_portal_projectile(int pow, bool fail);

struct bolt;
spret cast_apportation(int pow, bool fail);
spret cast_golubrias_passage(const coord_def& where, bool fail);

spret cast_dispersal(int pow, bool fail);

int gravitas_range(int pow);
bool fatal_attraction(const coord_def& pos, const actor *agent, int pow);
spret cast_gravitas(int pow, const coord_def& where, bool fail);

bool beckon(coord_def &origin, actor &beckoned, const bolt &path, int pow, actor &agent);
