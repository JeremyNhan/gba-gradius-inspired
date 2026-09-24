#include "ss_shots.h"

#include "gen_gfx.h"
#include "ss_boss.h"
#include "ss_effects.h"
#include "ss_enemies.h"
#include "ss_level.h"
#include "ss_sprites.h"
#include "ss_world.h"

player_shot shots[MAX_PLAYER_SHOTS];
static int active_count;
static int dropped;

static int sheet_of(shot_kind kind)
{
    switch(kind)
    {
    case SHOT_LASER:
        return GEN_SPR_SHOT_LASER;

    case SHOT_DOT:
        return GEN_SPR_SHOT_SPREAD;

    case SHOT_MISSILE:
        return GEN_SPR_SHOT_MISSILE;

    case SHOT_BEAM:
        return GEN_SPR_SHOT_CHARGE;

    default:
        return GEN_SPR_SHOT_NORMAL;
    }
}

/* Missile frames are drawn every 45 degrees counter-clockwise (visually) from "right". */
static int missile_frame(int angle)
{
    return (((ANGLE_TURN - angle) + 4096) >> 13) & 7;
}

hitbox shot_box(const player_shot* s)
{
    switch(s->kind)
    {
    case SHOT_BEAM:
        return make_hitbox(s->pos, 13, 7);

    case SHOT_LASER:
        return make_hitbox(s->pos, 12, 2);

    case SHOT_MISSILE:
    case SHOT_DOT:
        return make_hitbox(s->pos, 3, 3);

    default:
        return make_hitbox(s->pos, 4, 2);
    }
}

void shots_reset(void)
{
    for(int i = 0; i < MAX_PLAYER_SHOTS; ++i)
    {
        shots[i].active = false;
    }

    active_count = 0;
}

static player_shot* spawn(shot_kind kind, vec2 pos, vec2 vel, int damage)
{
    for(int i = 0; i < MAX_PLAYER_SHOTS; ++i)
    {
        player_shot* s = &shots[i];

        if(! s->active)
        {
            s->active = true;
            s->kind = (u8) kind;
            s->pos = pos;
            s->vel = vel;
            s->damage = damage;
            s->homing = false;
            s->angle = 0;
            s->life = 0;
            s->hit_mask = 0;
            s->boss_cooldown = 0;
            s->frame = 0;
            ++active_count;
            return s;
        }
    }

    ++dropped;          /* pool full: the projectile is simply not fired */
    return NULL;
}

void shots_fire(shot_kind kind, vec2 pos, vec2 vel, int damage)
{
    player_shot* s = spawn(kind, pos, vel, damage);

    if(s && kind == SHOT_LASER)
    {
        s->frame = vel.y < 0 ? 1 : vel.y > 0 ? 2 : 0;     /* level, rising, falling */
    }
}

void shots_fire_missile(vec2 pos, int angle, bool homing)
{
    player_shot* s = spawn(SHOT_MISSILE, pos, direction(angle, MISSILE_SPEED), MISSILE_DAMAGE);

    if(s)
    {
        s->angle = angle & 0xFFFF;
        s->homing = homing;
        s->frame = (u8) missile_frame(s->angle);
    }
}

void shots_fire_dot(vec2 pos)
{
    player_shot* s = spawn(SHOT_DOT, pos, v2(DOT_SPEED, 0), DOT_DAMAGE);

    if(s)
    {
        s->homing = true;
    }
}

void shots_release(player_shot* s)
{
    if(s->active)
    {
        s->active = false;
        --active_count;
    }
}

int shots_count(void)
{
    return active_count;
}

int shots_dropped(void)
{
    return dropped;
}

int shots_count_of(shot_kind kind)
{
    int n = 0;

    for(int i = 0; i < MAX_PLAYER_SHOTS; ++i)
    {
        n += shots[i].active && shots[i].kind == kind;
    }

    return n;
}

static void steer(player_shot* s, int turn_rate, fx speed)
{
    vec2 target;
    bool found = enemies_nearest_target(s->pos, &target) || boss_target_point(&target);

    if(found && s->life > 6)
    {
        int delta = angle_delta(s->angle, angle_to(s->pos, target));
        delta = SS_CLAMP(delta, -turn_rate, turn_rate);
        s->angle = (s->angle + delta) & 0xFFFF;
    }

    s->vel = direction(s->angle, speed);
}

void shots_update(void)
{
    for(int i = 0; i < MAX_PLAYER_SHOTS; ++i)
    {
        player_shot* s = &shots[i];

        if(! s->active)
        {
            continue;
        }

        ++s->life;

        if(s->boss_cooldown)
        {
            --s->boss_cooldown;
        }

        if(s->kind == SHOT_MISSILE && s->homing)
        {
            steer(s, MISSILE_TURN_RATE, MISSILE_SPEED);
            s->frame = (u8) missile_frame(s->angle);
        }
        else if(s->kind == SHOT_DOT)
        {
            steer(s, DOT_TURN_RATE, DOT_SPEED);
        }
        else if(s->kind == SHOT_BEAM)
        {
            s->frame = (u8) ((s->life >> 2) & 1);
        }

        s->pos = v2_add(s->pos, s->vel);
        bool dead = off_screen(s->pos, 24) || s->life > 150;

        /* Terrain stops everything except the beam. */
        if(! dead && s->kind != SHOT_BEAM && terrain_blocks(shot_box(s), world.scroll_x))
        {
            effects_spark(s->pos);
            dead = true;
        }

        if(dead)
        {
            shots_release(s);
        }
    }
}

void shots_render(void)
{
    for(int i = 0; i < MAX_PLAYER_SHOTS; ++i)
    {
        const player_shot* s = &shots[i];

        if(s->active)
        {
            sprites_draw(sheet_of((shot_kind) s->kind), s->frame, s->pos, 0);
        }
    }
}
