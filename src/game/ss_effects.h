/* Purely visual, short-lived sprites (explosions, sparks). When the pool is full, effects are skipped. */
#ifndef SS_EFFECTS_H
#define SS_EFFECTS_H

#include "ss_base.h"

void effects_reset(void);
void effects_explosion_small(vec2 pos, int delay);
void effects_explosion_big(vec2 pos, int delay);
void effects_spark(vec2 pos);
void effects_update(void);
void effects_render(void);
int effects_count(void);

#endif
