#ifndef SS_ENEMIES_H
#define SS_ENEMIES_H

#include "ss_base.h"
#include "ss_enemy_data.h"

typedef struct
{
    bool active;
    bool entered;           /* has been fully on screen at least once */
    u8 kind;                /* enemy_kind */
    u8 frame;
    s8 formation;
    s16 flags;
    vec2 pos;
    vec2 vel;
    fx base_y;
    int hp;
    int timer;
    int fire_timer;
    int param;
    int phase;
    int angle;
    int burst;
    int flash;
} enemy;

extern enemy enemies[MAX_ENEMIES];

void enemies_reset(void);
void enemies_spawn(enemy_kind kind, fx x, fx y, int param, int flags, int formation);
void enemies_spawn_formation(int type, int y, int count, int flags);
void enemies_update(void);
void enemies_render(void);

/* Damage from a player projectile; destroys the enemy when its HP runs out. */
void enemies_damage(enemy* e, int amount);

/* Removes an enemy with an explosion. by_player: score, capsule drop roll, mine burst. */
void enemies_destroy(enemy* e, bool by_player);

void enemies_destroy_all(void);     /* boss death: no score */
void enemies_shockwave(void);       /* with score, without drops or mine bursts */

/* Closest entered enemy ahead of `from` (homing weapons). */
bool enemies_nearest_target(vec2 from, vec2* out);

int enemies_count(void);
int enemies_dropped(void);

static inline hitbox enemy_box(const enemy* e)
{
    const enemy_def* d = &enemy_defs[e->kind];
    return make_hitbox(e->pos, d->half_w, d->half_h);
}

#endif
