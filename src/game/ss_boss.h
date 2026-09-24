/*
 * Stage bosses. Behaviour is deterministic: attack timing depends only on the boss timer, its
 * HP-based phase (3 phases each) and the player position.
 */
#ifndef SS_BOSS_H
#define SS_BOSS_H

#include "ss_base.h"
#include "ss_stage_data.h"

void boss_reset(void);
void boss_start(boss_id id);
void boss_update(void);
void boss_render(void);

bool boss_active(void);
bool boss_fighting(void);

/* Applies a projectile hit if the box touches a vulnerable part. Returns true if it hit. */
bool boss_take_hit(hitbox box, int damage);

/* Shockwave: hp_max / SHOCKWAVE_BOSS_DAMAGE_DIVISOR damage (no hitbox test). */
void boss_shockwave_hit(void);

/* True if the box touches the boss body (lethal to the player). */
bool boss_touches(hitbox box);

/* Aim point for homing weapons (only while fighting). */
bool boss_target_point(vec2* out);

int boss_hp(void);
int boss_hp_max(void);
vec2 boss_position(void);

#endif
