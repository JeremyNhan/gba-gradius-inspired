/* Power capsules: dropped by destroyed enemies, each one advances the power ladder by one step. */
#ifndef SS_POWERUPS_H
#define SS_POWERUPS_H

#include "ss_base.h"

typedef struct
{
    bool active;
    vec2 pos;
    int timer;
} powerup;

extern powerup powerups[MAX_POWERUPS];

void powerups_reset(void);
void powerups_drop(vec2 pos);
void powerups_collect(powerup* p);
void powerups_update(void);
void powerups_render(void);
int powerups_count(void);

static inline hitbox powerup_box(const powerup* p)
{
    return make_hitbox(p->pos, 6, 6);
}

#endif
