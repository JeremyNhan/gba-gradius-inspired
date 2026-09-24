#include "ss_enemies.h"

#include "gen_gfx.h"
#include "ss_audio.h"
#include "ss_bullets.h"
#include "ss_effects.h"
#include "ss_player.h"
#include "ss_powerups.h"
#include "ss_sprites.h"
#include "ss_stage_data.h"
#include "ss_world.h"

#define SPAWN_X 136
#define MAX_FORMATIONS 6

enemy enemies[MAX_ENEMIES];
static int active_count;
static int dropped;

typedef struct
{
    bool used;
    bool bonus;
    int alive;
    int killed;
    int spawned;
} formation_info;

static formation_info formations[MAX_FORMATIONS];

void enemies_reset(void)
{
    for(int i = 0; i < MAX_ENEMIES; ++i)
    {
        enemies[i].active = false;
    }

    for(int i = 0; i < MAX_FORMATIONS; ++i)
    {
        formations[i].used = false;
    }

    active_count = 0;
}

int enemies_count(void)
{
    return active_count;
}

int enemies_dropped(void)
{
    return dropped;
}

static int alloc_formation(bool bonus)
{
    for(int i = 0; i < MAX_FORMATIONS; ++i)
    {
        formation_info* f = &formations[i];

        if(! f->used)
        {
            f->used = true;
            f->bonus = bonus;
            f->alive = 0;
            f->killed = 0;
            f->spawned = 0;
            return i;
        }
    }

    return -1;
}

static void release(enemy* e)
{
    e->active = false;
    --active_count;
}

static void formation_member_gone(enemy* e, bool killed)
{
    if(e->formation < 0)
    {
        return;
    }

    formation_info* f = &formations[(int) e->formation];
    --f->alive;

    if(killed)
    {
        ++f->killed;
    }

    if(f->alive <= 0)
    {
        /* Bonus for wiping out a whole bonus formation. */
        if(f->bonus && f->killed == f->spawned)
        {
            game_add_score(500);
        }

        f->used = false;
    }
}

void enemies_spawn(enemy_kind kind, fx x, fx y, int param, int flags, int formation)
{
    enemy* e = NULL;

    for(int i = 0; i < MAX_ENEMIES && ! e; ++i)
    {
        if(! enemies[i].active)
        {
            e = &enemies[i];
        }
    }

    if(! e)
    {
        ++dropped;
        return;
    }

    const enemy_def* def = &enemy_defs[kind];
    e->active = true;
    e->entered = false;
    e->kind = (u8) kind;
    e->frame = 0;
    e->formation = (s8) formation;
    e->flags = (s16) flags;
    e->pos = v2(x, y);
    e->vel = v2(-def->speed, 0);
    e->base_y = y;
    e->hp = def->hp;
    e->timer = 0;
    e->fire_timer = def->first_fire_delay + rng_int(&world.random, 30);
    e->param = param;
    e->phase = 0;
    e->angle = 0;
    e->burst = 0;
    e->flash = 0;
    ++active_count;

    if(def->move == MOVE_STRAIGHT || def->move == MOVE_TUMBLE)
    {
        e->vel.y = FX(param) / 100;
    }

    if(flags & FLAG_FROM_LEFT)
    {
        e->vel.x = -e->vel.x;
    }

    if(formation >= 0)
    {
        ++formations[formation].alive;
        ++formations[formation].spawned;
    }
}

void enemies_spawn_formation(int type, int y, int count, int flags)
{
    int formation = alloc_formation(flags & FLAG_BONUS);
    int member_flags = flags & ~FLAG_BONUS;
    int mid = count / 2;

    for(int i = 0; i < count; ++i)
    {
        switch(type)
        {
        case FORMATION_LINE:
            enemies_spawn(ENEMY_DART, FX(SPAWN_X + i * 20), FX(y), 0, member_flags, formation);
            break;

        case FORMATION_V:
            enemies_spawn(ENEMY_DART, FX(SPAWN_X + ABS(i - mid) * 16), FX(y + (i - mid) * 14), 0, member_flags,
                          formation);
            break;

        case FORMATION_COLUMN:
            enemies_spawn(ENEMY_DART, FX(SPAWN_X), FX(y + (i - mid) * 18), 0, member_flags, formation);
            break;

        case FORMATION_WAVE:
            enemies_spawn(ENEMY_WAVER, FX(SPAWN_X + i * 22), FX(y), 28, member_flags, formation);
            break;

        case FORMATION_SWARM_LOOP:
            enemies_spawn(ENEMY_SWARM, FX(SPAWN_X + i * 16), FX(y), 30, member_flags, formation);
            break;

        case FORMATION_MINE_FIELD:
            enemies_spawn(ENEMY_MINE, FX(SPAWN_X + i * 34), FX(y + ((i * 37) % 90) - 45), 0, member_flags, formation);
            break;

        default:
            break;
        }
    }
}

void enemies_damage(enemy* e, int amount)
{
    e->hp -= amount;

    if(e->hp <= 0)
    {
        enemies_destroy(e, true);
        return;
    }

    e->flash = 3;
    audio_play(SFX_HIT_ID);
}

void enemies_destroy(enemy* e, bool by_player)
{
    const enemy_def* def = &enemy_defs[e->kind];
    vec2 pos = e->pos;

    if(def->big_explosion)
    {
        effects_explosion_big(pos, 0);
        world_shake(12, 2);
        audio_play(SFX_EXPLODE_BIG_ID);
    }
    else
    {
        effects_explosion_small(pos, 0);
        audio_play(SFX_EXPLODE_ID);
    }

    if(by_player)
    {
        game_add_score(def->score);

        /* Power capsules are a random reward for kills (deterministic: the world RNG is seeded). */
        if(def->drop_chance && rng_int(&world.random, 100) < def->drop_chance)
        {
            powerups_drop(pos);
        }

        if(e->kind == ENEMY_MINE)
        {
            bullets_fire_ring(BULLET_SMALL, pos, 8, FX_F(1.3), world.stage_frame * 512);
        }
    }

    bool split = e->kind == ENEMY_ASTEROID_BIG && by_player;
    formation_member_gone(e, by_player);
    release(e);

    if(split)
    {
        enemies_spawn(ENEMY_ASTEROID_SMALL, pos.x, pos.y - FX(6), -70, 0, -1);
        enemies_spawn(ENEMY_ASTEROID_SMALL, pos.x + FX(4), pos.y, 0, 0, -1);
        enemies_spawn(ENEMY_ASTEROID_SMALL, pos.x, pos.y + FX(6), 70, 0, -1);
    }
}

static void remove_all(bool with_score)
{
    int delay = 1;

    for(int i = 0; i < MAX_ENEMIES; ++i)
    {
        enemy* e = &enemies[i];

        if(e->active)
        {
            if(with_score)
            {
                game_add_score(enemy_defs[e->kind].score);
            }

            effects_explosion_small(e->pos, delay);
            delay += 2;
            formation_member_gone(e, false);
            release(e);
        }
    }
}

void enemies_destroy_all(void)
{
    remove_all(false);
}

void enemies_shockwave(void)
{
    remove_all(true);
}

bool enemies_nearest_target(vec2 from, vec2* out)
{
    int best = 0x7FFFFFFF;
    bool found = false;

    for(int i = 0; i < MAX_ENEMIES; ++i)
    {
        const enemy* e = &enemies[i];

        if(e->active && e->entered && e->pos.x > from.x - FX(16))
        {
            int dx = fx_trunc(e->pos.x - from.x);
            int dy = fx_trunc(e->pos.y - from.y);
            int distance = dx * dx + dy * dy;

            if(distance < best)
            {
                best = distance;
                *out = e->pos;
                found = true;
            }
        }
    }

    return found;
}

/* ----- behaviour ---------------------------------------------------------------------------------- */

static void move(enemy* e)
{
    const enemy_def* def = &enemy_defs[e->kind];
    vec2 target = player_position();

    switch(def->move)
    {
    case MOVE_STRAIGHT:
    case MOVE_TUMBLE:
        e->pos = v2_add(e->pos, e->vel);
        break;

    case MOVE_SINE:
        {
            int amplitude = e->param ? e->param : 24;
            e->pos.x -= def->speed;
            e->pos.y = e->base_y + direction(e->timer * 512 + e->angle, FX(amplitude)).y;
        }
        break;

    case MOVE_INTERCEPT:
        if(e->phase == 0)
        {
            fx stop_x = FX(e->param ? e->param : 70);
            fx step = (e->pos.x - stop_x) / 12;

            if(step < FX_F(0.4))
            {
                e->phase = 1;
                e->timer = 0;
            }
            else
            {
                e->pos.x -= step;
            }
        }
        else if(e->phase == 1)
        {
            if(e->timer == 10 && player_alive())
            {
                e->burst = 3;
                e->fire_timer = 0;
            }

            if(e->timer >= 45)
            {
                e->phase = 2;
                int dash = player_alive() ? angle_to(e->pos, target) : ANGLE_LEFT;
                int delta = angle_delta(ANGLE_LEFT, dash);

                /* never dash backwards: clamp to the left half-plane */
                if(ABS(delta) > 12000)
                {
                    dash = ANGLE_LEFT + (delta > 0 ? 12000 : -12000);
                }

                e->vel = direction(dash, def->speed);
            }
        }
        else
        {
            e->pos = v2_add(e->pos, e->vel);
        }
        break;

    case MOVE_GROUND:
        e->pos.x -= world.scroll_speed;
        break;

    case MOVE_HOVER:
        if(e->phase == 0)
        {
            e->pos.x -= def->speed;

            if(e->pos.x <= FX(70))
            {
                e->phase = 1;
                e->timer = 0;
            }
        }
        else if(e->phase == 1)
        {
            e->pos.y = e->base_y + direction(e->timer * 300, FX(28)).y;

            if(e->timer > 480)
            {
                e->phase = 2;
            }
        }
        else
        {
            e->pos.x -= def->speed * 2;
        }
        break;

    case MOVE_LOOP:
        if(e->phase == 0)
        {
            e->pos.x -= def->speed;

            if(e->pos.x <= FX(e->param))
            {
                e->phase = 1;
                e->timer = 0;
                e->angle = ANGLE_LEFT;
            }
        }
        else if(e->phase == 1)
        {
            /* Full circle in 64 frames (upwards in the bottom half of the screen, downwards in the top). */
            e->angle = (e->angle + (e->base_y > 0 ? 1024 : -1024)) & 0xFFFF;
            e->pos = v2_add(e->pos, direction(e->angle, def->speed));

            if(e->timer >= 64)
            {
                e->phase = 2;
            }
        }
        else
        {
            e->pos.x -= fx_mul(def->speed, FX_F(1.3));
        }
        break;

    case MOVE_DRIFT:
        {
            fx vy = e->vel.y;

            if(player_alive())
            {
                vy += target.y > e->pos.y ? FX_F(0.01) : -FX_F(0.01);
            }

            e->vel.y = SS_CLAMP(vy, -FX_F(0.45), FX_F(0.45));
            e->pos = v2_add(e->pos, e->vel);
        }
        break;

    default:
        break;
    }
}

static void fire(enemy* e)
{
    const enemy_def* def = &enemy_defs[e->kind];
    int difficulty = world_difficulty();

    if(! player_alive() || ! e->entered || e->pos.x < FX(-100) || e->pos.x > FX(112))
    {
        return;
    }

    /* Burst in progress (interceptors, BURST3). */
    if(e->burst > 0)
    {
        if(--e->fire_timer <= 0)
        {
            bullets_fire_aimed(BULLET_SMALL, e->pos, FX_F(2.2) + FX_F(0.2) * difficulty, 0);
            --e->burst;
            e->fire_timer = 6;
        }

        return;
    }

    if(def->fire == FIRE_NONE || def->fire == FIRE_RING8 || def->move == MOVE_INTERCEPT ||
       difficulty < def->fire_from_stage)
    {
        return;
    }

    if(--e->fire_timer > 0)
    {
        return;
    }

    /* Harder stages shorten the interval: x1, x0.67, x0.5. */
    e->fire_timer = (def->fire_interval * 4) / (4 + difficulty * 2);

    /* Don't shoot point-blank. */
    vec2 target = player_position();

    if(fx_abs(target.x - e->pos.x) < FX(24) && fx_abs(target.y - e->pos.y) < FX(24))
    {
        return;
    }

    fx speed = FX_F(1.5) + FX_F(0.2) * difficulty;
    vec2 muzzle = e->pos;

    if(def->move == MOVE_GROUND)
    {
        muzzle = v2_add(muzzle, direction(angle_to(e->pos, target), FX(7)));
    }

    switch(def->fire)
    {
    case FIRE_AIMED:
        bullets_fire_aimed(BULLET_SMALL, muzzle, speed, 0);
        break;

    case FIRE_AIMED_SPREAD3:
        bullets_fire_aimed(BULLET_BIG, muzzle, speed, 0);
        bullets_fire_aimed(BULLET_BIG, muzzle, speed, DEGREES(18));
        bullets_fire_aimed(BULLET_BIG, muzzle, speed, DEGREES(-18));
        break;

    case FIRE_BURST3:
        e->burst = 3;
        e->fire_timer = 0;
        break;

    case FIRE_STRAIGHT:
        bullets_fire(BULLET_NEEDLE, muzzle, v2(-speed - FX(1), 0));
        break;

    default:
        break;
    }
}

static void animate(enemy* e)
{
    const enemy_def* def = &enemy_defs[e->kind];

    if(def->move == MOVE_GROUND)
    {
        /* Barrel frames: 0 = left, 2 = up, 4 = right (visual angles 180..0 degrees). */
        int visual = (ANGLE_TURN - angle_to(e->pos, player_position())) & 0xFFFF;

        if(e->flags & FLAG_CEILING)
        {
            visual = (ANGLE_TURN - visual) & 0xFFFF;
        }

        if(visual > 32768)
        {
            e->frame = visual > 49152 ? 4 : 0;
        }
        else
        {
            e->frame = (u8) SS_CLAMP(4 - ((visual + 4096) >> 13), 0, 4);
        }
    }
    else if(def->anim_period)
    {
        e->frame = (u8) ((e->timer / def->anim_period) % def->frames);
    }

    if(e->flash)
    {
        --e->flash;
    }
}

void enemies_update(void)
{
    for(int i = 0; i < MAX_ENEMIES; ++i)
    {
        enemy* e = &enemies[i];

        if(! e->active)
        {
            continue;
        }

        ++e->timer;
        move(e);

        fx x = e->pos.x;
        fx y = e->pos.y;

        if(! e->entered && x < FX(112) && x > FX(-112) && y > FX(-76) && y < FX(76))
        {
            e->entered = true;
        }

        if(x < FX(-150) || x > FX(260) || y < FX(-130) || y > FX(130) || (e->entered && off_screen(e->pos, 40)))
        {
            formation_member_gone(e, false);
            release(e);
            continue;
        }

        fire(e);
        animate(e);
    }
}

void enemies_render(void)
{
    for(int i = 0; i < MAX_ENEMIES; ++i)
    {
        const enemy* e = &enemies[i];

        if(e->active)
        {
            int flags = ((e->flags & FLAG_FROM_LEFT) ? SPR_HFLIP : 0) | ((e->flags & FLAG_CEILING) ? SPR_VFLIP : 0) |
                    ((e->flash & 2) ? SPR_FLASH : 0);
            sprites_draw(enemy_defs[e->kind].sheet, e->frame, e->pos, flags);
        }
    }
}
