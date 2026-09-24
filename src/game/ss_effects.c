#include "ss_effects.h"

#include "gen_gfx.h"
#include "ss_sprites.h"
#include "ss_world.h"

typedef enum
{
    EFFECT_EXPLOSION_SMALL,
    EFFECT_EXPLOSION_BIG,
    EFFECT_SPARK
} effect_kind;

typedef struct
{
    u8 sheet;
    u8 frames;
    u8 frame_period;
} effect_info;

static const effect_info infos[3] = {
    { GEN_SPR_EXPLOSION_SMALL, 6, 4 },
    { GEN_SPR_EXPLOSION_BIG, 6, 5 },
    { GEN_SPR_SPARK, 3, 3 },
};

typedef struct
{
    bool active;
    u8 kind;
    vec2 pos;
    vec2 vel;
    int timer;
    int delay;              /* frames before the effect appears (explosion chains) */
} effect;

static effect pool[MAX_EFFECTS];
static int active_count;

void effects_reset(void)
{
    for(int i = 0; i < MAX_EFFECTS; ++i)
    {
        pool[i].active = false;
    }

    active_count = 0;
}

int effects_count(void)
{
    return active_count;
}

static void spawn(effect_kind kind, vec2 pos, vec2 vel, int delay)
{
    for(int i = 0; i < MAX_EFFECTS; ++i)
    {
        effect* fx = &pool[i];

        if(! fx->active)
        {
            fx->active = true;
            fx->kind = (u8) kind;
            fx->pos = pos;
            fx->vel = vel;
            fx->timer = 0;
            fx->delay = delay;
            ++active_count;
            return;
        }
    }
}

void effects_explosion_small(vec2 pos, int delay)
{
    spawn(EFFECT_EXPLOSION_SMALL, pos, v2(-world.scroll_speed / 2, 0), delay);
}

void effects_explosion_big(vec2 pos, int delay)
{
    spawn(EFFECT_EXPLOSION_BIG, pos, v2(-world.scroll_speed / 2, 0), delay);

    /* A couple of debris sparks flying outwards. */
    spawn(EFFECT_SPARK, pos, v2i(-1, -1), delay + 2);
    spawn(EFFECT_SPARK, pos, v2i(1, 1), delay + 4);
}

void effects_spark(vec2 pos)
{
    spawn(EFFECT_SPARK, pos, v2(0, 0), 0);
}

void effects_update(void)
{
    for(int i = 0; i < MAX_EFFECTS; ++i)
    {
        effect* fx = &pool[i];

        if(! fx->active)
        {
            continue;
        }

        if(fx->delay > 0)
        {
            --fx->delay;
            continue;
        }

        const effect_info* info = &infos[fx->kind];

        if(++fx->timer / info->frame_period >= info->frames)
        {
            fx->active = false;
            --active_count;
            continue;
        }

        fx->pos = v2_add(fx->pos, fx->vel);
    }
}

void effects_render(void)
{
    for(int i = 0; i < MAX_EFFECTS; ++i)
    {
        const effect* fx = &pool[i];

        if(fx->active && ! fx->delay)
        {
            const effect_info* info = &infos[fx->kind];
            sprites_draw(info->sheet, fx->timer / info->frame_period, fx->pos, 0);
        }
    }
}
