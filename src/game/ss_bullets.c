#include "ss_bullets.h"

#include "gen_gfx.h"
#include "ss_effects.h"
#include "ss_level.h"
#include "ss_player.h"
#include "ss_sprites.h"
#include "ss_world.h"

enemy_bullet bullets[MAX_ENEMY_BULLETS];
static int active_count;
static int dropped;

void bullets_reset(void)
{
    for(int i = 0; i < MAX_ENEMY_BULLETS; ++i)
    {
        bullets[i].active = false;
    }

    active_count = 0;
}

int bullets_count(void)
{
    return active_count;
}

int bullets_dropped(void)
{
    return dropped;
}

void bullets_fire(bullet_kind kind, vec2 pos, vec2 vel)
{
    for(int i = 0; i < MAX_ENEMY_BULLETS; ++i)
    {
        enemy_bullet* b = &bullets[i];

        if(! b->active)
        {
            b->active = true;
            b->kind = (u8) kind;
            b->pos = pos;
            b->vel = vel;
            b->timer = 0;
            ++active_count;
            return;
        }
    }

    ++dropped;
}

void bullets_fire_aimed(bullet_kind kind, vec2 pos, fx speed, int angle_offset)
{
    int angle = angle_to(pos, player_position()) + angle_offset;
    bullets_fire(kind, pos, direction(angle, speed));
}

void bullets_fire_ring(bullet_kind kind, vec2 pos, int count, fx speed, int phase)
{
    int step = ANGLE_TURN / count;

    for(int i = 0; i < count; ++i)
    {
        bullets_fire(kind, pos, direction(phase + i * step, speed));
    }
}

void bullets_fire_fan(bullet_kind kind, vec2 pos, int count, fx speed, int spread_step)
{
    int first = -(count - 1) * spread_step / 2;

    for(int i = 0; i < count; ++i)
    {
        bullets_fire_aimed(kind, pos, speed, first + i * spread_step);
    }
}

void bullets_release(enemy_bullet* b)
{
    if(b->active)
    {
        b->active = false;
        --active_count;
    }
}

void bullets_cancel_all(bool with_sparks)
{
    int sparks = with_sparks ? 0 : 5;

    for(int i = 0; i < MAX_ENEMY_BULLETS; ++i)
    {
        enemy_bullet* b = &bullets[i];

        if(b->active)
        {
            if(sparks < 5 && (i & 3) == 0)
            {
                effects_spark(b->pos);
                ++sparks;
            }

            bullets_release(b);
        }
    }
}

void bullets_update(void)
{
    for(int i = 0; i < MAX_ENEMY_BULLETS; ++i)
    {
        enemy_bullet* b = &bullets[i];

        if(! b->active)
        {
            continue;
        }

        b->pos = v2_add(b->pos, b->vel);
        ++b->timer;

        if(off_screen(b->pos, 8) || terrain_blocks(bullet_box(b), world.scroll_x))
        {
            bullets_release(b);
        }
    }
}

void bullets_render(void)
{
    static const u8 sheets[3] = { GEN_SPR_BULLET_SMALL, GEN_SPR_BULLET_BIG, GEN_SPR_BULLET_NEEDLE };

    for(int i = 0; i < MAX_ENEMY_BULLETS; ++i)
    {
        const enemy_bullet* b = &bullets[i];

        if(b->active)
        {
            int frame = b->kind == BULLET_NEEDLE ? 0 : (b->timer >> 3) & 1;
            int flags = (b->kind == BULLET_NEEDLE && b->vel.x > 0) ? SPR_HFLIP : 0;
            sprites_draw(sheets[b->kind], frame, b->pos, flags);
        }
    }
}
