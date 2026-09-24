/* Enemy projectiles: fixed pool; when it is full, new bullets are not created (a natural density cap). */
#ifndef SS_BULLETS_H
#define SS_BULLETS_H

#include "ss_base.h"

typedef enum
{
    BULLET_SMALL,           /* pink orb, standard */
    BULLET_BIG,             /* orange orb, slightly larger hitbox */
    BULLET_NEEDLE           /* fast horizontal dart */
} bullet_kind;

typedef struct
{
    bool active;
    u8 kind;
    vec2 pos;
    vec2 vel;
    int timer;
} enemy_bullet;

extern enemy_bullet bullets[MAX_ENEMY_BULLETS];

void bullets_reset(void);
void bullets_fire(bullet_kind kind, vec2 pos, vec2 vel);
void bullets_fire_aimed(bullet_kind kind, vec2 pos, fx speed, int angle_offset);
void bullets_fire_ring(bullet_kind kind, vec2 pos, int count, fx speed, int phase);
void bullets_fire_fan(bullet_kind kind, vec2 pos, int count, fx speed, int spread_step);
void bullets_release(enemy_bullet* b);
void bullets_cancel_all(bool with_sparks);
void bullets_update(void);
void bullets_render(void);

int bullets_count(void);
int bullets_dropped(void);

static inline hitbox bullet_box(const enemy_bullet* b)
{
    return make_hitbox(b->pos, b->kind == BULLET_BIG ? 3 : 2, b->kind == BULLET_NEEDLE ? 1 : 2);
}

#endif
