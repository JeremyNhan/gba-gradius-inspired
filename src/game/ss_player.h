#ifndef SS_PLAYER_H
#define SS_PLAYER_H

#include "ss_base.h"

void player_init(void);
void player_update(void);
void player_render(void);

/* Something lethal touched the player. Returns true if absorbed (shield / invulnerable), false if
 * the ship was destroyed. */
bool player_hit(void);

bool player_alive(void);
vec2 player_position(void);
hitbox player_core_hitbox(void);        /* small core (classic shmup) */
hitbox player_pickup_hitbox(void);      /* larger box for collecting capsules */

void player_start_outro(void);          /* stage clear: stop shooting, fly out to the right */
bool player_outro_done(void);

/* Syncs shield / shooters after the power level changed; reaching SHOCKWAVE fires one at once. */
void player_refresh_power(int previous_power);
void player_shockwave_invulnerability(int frames);

#endif
