/* Player projectiles: fixed pool; when it is full, new shots are simply not fired. */
#ifndef SS_SHOTS_H
#define SS_SHOTS_H

#include "ss_base.h"
#include "ss_weapon_data.h"

typedef struct
{
    bool active;
    bool homing;            /* missiles and dots: steer toward the nearest target */
    u8 kind;                /* shot_kind */
    u8 frame;
    vec2 pos;
    vec2 vel;
    int damage;
    int angle;              /* missiles and dots: binary angle */
    int life;
    u32 hit_mask;           /* piercing shots: enemies already hit (bit per enemy slot) */
    int boss_cooldown;      /* charged beam: frames until the boss can be hit again */
} player_shot;

extern player_shot shots[MAX_PLAYER_SHOTS];

void shots_reset(void);
void shots_fire(shot_kind kind, vec2 pos, vec2 vel, int damage);
void shots_fire_missile(vec2 pos, int angle, bool homing);
void shots_fire_dot(vec2 pos);
void shots_release(player_shot* s);
void shots_update(void);
void shots_render(void);

int shots_count(void);
int shots_count_of(shot_kind kind);
int shots_dropped(void);

hitbox shot_box(const player_shot* s);

static inline bool shot_pierces(const player_shot* s)
{
    return s->kind == SHOT_BEAM || s->kind == SHOT_LASER;
}

#endif
