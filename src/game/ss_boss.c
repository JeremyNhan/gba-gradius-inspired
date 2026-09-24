#include "ss_boss.h"

#include "gen_gfx.h"
#include "ss_audio.h"
#include "ss_bullets.h"
#include "ss_effects.h"
#include "ss_enemies.h"
#include "ss_player.h"
#include "ss_sprites.h"
#include "ss_weapon_data.h"
#include "ss_world.h"

typedef struct
{
    u8 sheet;
    s16 hp;
    s32 score;
    s16 home_x;
    s16 dying_frames;
} boss_info;

static const boss_info infos[3] = {
    [BOSS_WARDEN] = { GEN_SPR_BOSS_WARDEN, 300, 10000, 64, 150 },
    [BOSS_HIVE] = { GEN_SPR_BOSS_HIVE, 420, 20000, 60, 150 },
    [BOSS_OVERMIND] = { GEN_SPR_BOSS_OVERMIND_FRONT, 640, 50000, 24, 240 },
};

#define POD_HP 30
#define REAR_OFFSET 64

typedef enum
{
    BOSS_NONE,
    BOSS_ENTER,
    BOSS_FIGHT,
    BOSS_DYING
} boss_state;

typedef struct
{
    bool alive;
    int hp;
    int flash;
    vec2 pos;
} pod;

static struct
{
    boss_state state;
    boss_id id;
    int hp;
    int hp_max;
    int phase;
    int timer;
    int attack_timer;
    int flash;
    int open_frames;
    int cycle;
    fx home_x;
    vec2 pos;
    pod pods[2];
} b;

void boss_reset(void)
{
    b.state = BOSS_NONE;
    b.pods[0].alive = false;
    b.pods[1].alive = false;
}

void boss_start(boss_id id)
{
    const boss_info* info = &infos[id];
    b.id = id;
    b.state = BOSS_ENTER;
    b.hp = info->hp;
    b.hp_max = info->hp;
    b.phase = 0;
    b.timer = 0;
    b.attack_timer = 60;
    b.flash = 0;
    b.open_frames = 0;
    b.cycle = 0;
    b.home_x = FX(info->home_x);
    b.pos = v2i(180, 0);

    for(int i = 0; i < 2; ++i)
    {
        b.pods[i].alive = id == BOSS_OVERMIND;
        b.pods[i].hp = POD_HP;
        b.pods[i].flash = 0;
        b.pods[i].pos = b.pos;
    }
}

bool boss_active(void)
{
    return b.state != BOSS_NONE;
}

bool boss_fighting(void)
{
    return b.state == BOSS_FIGHT;
}

int boss_hp(void)
{
    return b.hp;
}

int boss_hp_max(void)
{
    return b.state == BOSS_NONE ? 0 : b.hp_max;
}

vec2 boss_position(void)
{
    return b.pos;
}

static hitbox core_box(void)
{
    switch(b.id)
    {
    case BOSS_WARDEN:
        return make_hitbox(v2(b.pos.x + FX(4), b.pos.y), 27, 20);

    case BOSS_HIVE:
        return make_hitbox(b.pos, 15, 15);

    default:
        return make_hitbox(v2(b.pos.x + FX(4), b.pos.y), 24, 22);
    }
}

static vec2 muzzle(void)
{
    switch(b.id)
    {
    case BOSS_WARDEN:
        return v2(b.pos.x - FX(18), b.pos.y);

    case BOSS_HIVE:
        return b.pos;

    default:
        return v2(b.pos.x - FX(8), b.pos.y);
    }
}

bool boss_target_point(vec2* out)
{
    if(b.state == BOSS_FIGHT)
    {
        *out = b.pos;
        return true;
    }

    return false;
}

bool boss_touches(hitbox box)
{
    if(b.state != BOSS_FIGHT && b.state != BOSS_ENTER)
    {
        return false;
    }

    if(hit_test(core_box(), box))
    {
        return true;
    }

    if(b.id == BOSS_OVERMIND)
    {
        if(hit_test(make_hitbox(v2(b.pos.x + FX(REAR_OFFSET), b.pos.y), 28, 26), box))
        {
            return true;
        }

        for(int i = 0; i < 2; ++i)
        {
            if(b.pods[i].alive && hit_test(make_hitbox(b.pods[i].pos, 6, 5), box))
            {
                return true;
            }
        }
    }

    return false;
}

static void apply_damage(int damage)
{
    b.hp -= damage;
    b.flash = 2;
    audio_play(SFX_HIT_ID);

    int new_phase = b.hp * 3 > b.hp_max * 2 ? 0 : b.hp * 3 > b.hp_max ? 1 : 2;

    if(new_phase != b.phase && b.hp > 0)
    {
        b.phase = new_phase;
        b.attack_timer = 50;
        b.cycle = 0;
        world_shake(20, 2);
        bullets_cancel_all(true);
        audio_play(SFX_EXPLODE_BIG_ID);
        effects_explosion_big(v2(b.pos.x + FX(rng_range(&world.random, -16, 16)),
                                 b.pos.y + FX(rng_range(&world.random, -16, 16))), 0);
    }

    if(b.hp <= 0)
    {
        b.hp = 0;
        b.state = BOSS_DYING;
        b.timer = 0;
        audio_stop_music();
    }
}

bool boss_take_hit(hitbox box, int damage)
{
    if(b.state != BOSS_FIGHT)
    {
        return false;
    }

    /* Pods first: they sit in front of the final boss. */
    if(b.id == BOSS_OVERMIND)
    {
        for(int i = 0; i < 2; ++i)
        {
            pod* p = &b.pods[i];

            if(p->alive && hit_test(make_hitbox(p->pos, 7, 6), box))
            {
                p->hp -= damage;
                p->flash = 3;

                if(p->hp <= 0)
                {
                    p->alive = false;
                    effects_explosion_big(p->pos, 0);
                    game_add_score(3000);
                    audio_play(SFX_EXPLODE_BIG_ID);
                }
                else
                {
                    audio_play(SFX_HIT_ID);
                }

                return true;
            }
        }
    }

    if(! hit_test(core_box(), box))
    {
        return false;
    }

    /* The final boss is armoured while its eye is closed (phase 0): half damage. */
    if(b.id == BOSS_OVERMIND && b.phase == 0)
    {
        damage = (damage + 1) / 2;
    }

    apply_damage(damage);
    return true;
}

void boss_shockwave_hit(void)
{
    if(b.state == BOSS_FIGHT)
    {
        apply_damage(MAX(1, b.hp_max / SHOCKWAVE_BOSS_DAMAGE_DIVISOR));
    }
}

/* ----- attack patterns ----------------------------------------------------------------------------- */

static void update_warden(void)
{
    fx speed = FX(1 + b.phase);
    b.pos.y = direction(b.timer * (180 + b.phase * 60), FX(34 + b.phase * 6)).y;

    if(b.phase == 2)
    {
        b.pos.x = b.home_x + direction(b.timer * 256, FX(14)).x;
    }

    if(b.open_frames)
    {
        --b.open_frames;
    }

    if(--b.attack_timer > 0)
    {
        /* Phase 3 keeps a rotating stream going between volleys. */
        if(b.phase == 2 && (b.timer % 6) == 0)
        {
            bullets_fire(BULLET_SMALL, muzzle(), direction(b.timer * 1400, FX_F(1.4)));
        }

        return;
    }

    ++b.cycle;
    b.open_frames = 24;

    switch(b.phase)
    {
    case 0:
        bullets_fire_fan(BULLET_BIG, muzzle(), 3, FX_F(1.6), DEGREES(20));
        b.attack_timer = 70;

        if((b.cycle % 3) == 0)
        {
            enemies_spawn(ENEMY_DART, b.pos.x + FX(8), b.pos.y - FX(12), -40, 0, -1);
            enemies_spawn(ENEMY_DART, b.pos.x + FX(8), b.pos.y + FX(12), 40, 0, -1);
        }
        break;

    case 1:
        if(b.cycle & 1)
        {
            bullets_fire_ring(BULLET_SMALL, muzzle(), 10, FX_F(1.3), b.cycle * 3000);
        }
        else
        {
            vec2 m = muzzle();
            bullets_fire_aimed(BULLET_NEEDLE, m, speed + FX(1), 0);
            bullets_fire_aimed(BULLET_NEEDLE, v2(m.x, m.y - FX(10)), speed + FX(1), 0);
            bullets_fire_aimed(BULLET_NEEDLE, v2(m.x, m.y + FX(10)), speed + FX(1), 0);
        }

        b.attack_timer = 55;
        break;

    default:
        bullets_fire_fan(BULLET_BIG, muzzle(), 5, FX_F(1.8), DEGREES(16));
        b.attack_timer = 75;

        if((b.cycle % 3) == 0)
        {
            enemies_spawn(ENEMY_DART, b.pos.x + FX(8), b.pos.y - FX(12), -60, 0, -1);
            enemies_spawn(ENEMY_DART, b.pos.x + FX(8), b.pos.y + FX(12), 60, 0, -1);
        }
        break;
    }
}

static void update_hive(void)
{
    vec2 target = player_position();

    if(b.phase == 1)
    {
        /* Ram attack cycle: align with the player, telegraph, dash left, return, ring burst. */
        int t = b.timer % 220;

        if(t < 60)
        {
            b.pos.y += (target.y - b.pos.y) / 16;
        }
        else if(t < 80)
        {
            b.open_frames = 2;
            b.pos.x = b.home_x + ((t & 2) ? FX(2) : FX(-2));
        }
        else if(t < 120)
        {
            b.pos.x = MAX(b.pos.x - FX(5), FX(-70));
        }
        else if(t < 170)
        {
            b.pos.x += (b.home_x - b.pos.x) / 10;

            if(t == 169)
            {
                bullets_fire_ring(BULLET_BIG, b.pos, 12, FX_F(1.4), b.timer * 97);
            }
        }
        else
        {
            b.pos.x = b.home_x;
        }

        if(t < 60 && (b.timer % 10) == 0)
        {
            bullets_fire(BULLET_SMALL, b.pos, direction(b.timer * 900, FX_F(1.2)));
        }

        return;
    }

    /* Phases 0 and 2: hover and fire rotating spiral arms. */
    b.pos.x += (b.home_x - b.pos.x) / 8;
    b.pos.y = direction(b.timer * 150, FX(40)).y;

    int arms = b.phase == 0 ? 2 : 3;
    int period = b.phase == 0 ? 7 : 5;

    if((b.timer % period) == 0)
    {
        int base = b.timer * (b.phase == 0 ? 700 : -800);

        for(int arm = 0; arm < arms; ++arm)
        {
            bullets_fire(BULLET_SMALL, b.pos, direction(base + arm * (ANGLE_TURN / arms), FX_F(1.3) + FX_F(0.2) * b.phase));
        }
    }

    if(b.phase == 2 && (b.timer % 150) == 0)
    {
        enemies_spawn(ENEMY_MINE, b.pos.x - FX(10), b.pos.y - FX(20), 0, 0, -1);
        enemies_spawn(ENEMY_MINE, b.pos.x - FX(10), b.pos.y + FX(20), 0, 0, -1);
    }

    if(b.phase == 0 && (b.timer % 120) == 60)
    {
        bullets_fire_fan(BULLET_BIG, b.pos, 3, FX_F(1.6), DEGREES(24));
    }
}

static void update_overmind(void)
{
    int amplitude = b.phase == 2 ? 30 : 18;
    b.pos.y = direction(b.timer * (140 + b.phase * 50), FX(amplitude)).y;

    switch(b.phase)
    {
    case 0:
        if((b.timer % 130) == 65)
        {
            bullets_fire_fan(BULLET_BIG, muzzle(), 5, FX_F(1.5), DEGREES(15));
        }
        break;

    case 1:
        {
            /* Needle waves from the eye, then a big ring. */
            int t = b.timer % 150;

            if(t < 48 && (t % 4) == 0)
            {
                fx vy = direction(t * 2048, FX_F(1.2)).y;
                bullets_fire(BULLET_NEEDLE, muzzle(), v2(FX(-3), vy));
            }
            else if(t == 90)
            {
                bullets_fire_ring(BULLET_BIG, muzzle(), 14, FX_F(1.3), b.timer * 211);
            }
        }
        break;

    default:
        if((b.timer % 4) == 0)
        {
            int base = b.timer * 900;
            bullets_fire(BULLET_SMALL, muzzle(), direction(base, FX_F(1.4)));
            bullets_fire(BULLET_SMALL, muzzle(), direction(base + ANGLE_LEFT, FX_F(1.4)));
        }

        if((b.timer % 70) == 0)
        {
            bullets_fire_fan(BULLET_BIG, muzzle(), 3, FX(2), DEGREES(14));
        }

        if((b.timer % 220) == 110)
        {
            enemies_spawn(ENEMY_MINE, b.pos.x + FX(20), FX(-60), 0, 0, -1);
            enemies_spawn(ENEMY_MINE, b.pos.x + FX(20), FX(60), 0, 0, -1);
        }
        break;
    }
}

static void update_pods(void)
{
    for(int i = 0; i < 2; ++i)
    {
        pod* p = &b.pods[i];

        if(! p->alive)
        {
            continue;
        }

        int side = i == 0 ? -1 : 1;
        fx bob = direction(b.timer * 400 + i * 20000, FX(6)).y;
        p->pos = v2(b.pos.x - FX(14) + bob, b.pos.y + FX(side * 44) + bob);

        if(p->flash)
        {
            --p->flash;
        }

        if(b.state == BOSS_FIGHT && ((b.timer + i * 30) % 60) == 0 && player_alive())
        {
            bullets_fire_aimed(BULLET_SMALL, p->pos, FX_F(1.8), 0);
        }
    }
}

static void update_dying(void)
{
    const boss_info* info = &infos[b.id];
    b.pos.y += FX_F(0.15);

    switch(b.timer)
    {
    case 1:
        bullets_cancel_all(true);
        break;

    case 2:
        enemies_destroy_all();
        break;

    case 3:
        game_add_score(info->score);
        break;

    default:
        break;
    }

    if((b.timer % 7) == 0)
    {
        vec2 offset = v2i(rng_range(&world.random, -28, 28), rng_range(&world.random, -24, 24));
        effects_explosion_small(v2_add(b.pos, offset), 0);
        audio_play(SFX_EXPLODE_ID);
    }

    if((b.timer % 30) == 0)
    {
        vec2 offset = v2i(rng_range(&world.random, -20, 20), rng_range(&world.random, -16, 16));
        effects_explosion_big(v2_add(b.pos, offset), 0);
        world_shake(15, 3);
        audio_play(SFX_EXPLODE_BIG_ID);

        if(b.id == BOSS_OVERMIND)
        {
            effects_explosion_big(v2_add(v2(b.pos.x + FX(REAR_OFFSET), b.pos.y), offset), 0);
        }
    }

    if(b.timer >= info->dying_frames)
    {
        effects_explosion_big(b.pos, 0);
        effects_explosion_big(v2(b.pos.x - FX(16), b.pos.y - FX(12)), 4);
        effects_explosion_big(v2(b.pos.x + FX(16), b.pos.y + FX(12)), 8);
        world_shake(40, 4);
        audio_play(SFX_EXPLODE_BIG_ID);
        b.pods[0].alive = false;
        b.pods[1].alive = false;
        b.state = BOSS_NONE;
        world_notify_boss_defeated();
    }
}

void boss_update(void)
{
    if(b.state == BOSS_NONE)
    {
        return;
    }

    ++b.timer;

    if(b.flash)
    {
        --b.flash;
    }

    if(b.state == BOSS_ENTER)
    {
        fx dx = (b.pos.x - b.home_x) / 24;

        if(dx < FX_F(0.3))
        {
            b.pos.x = b.home_x;
            b.state = BOSS_FIGHT;
            b.timer = 0;
        }
        else
        {
            b.pos.x -= dx;
        }
    }
    else if(b.state == BOSS_FIGHT)
    {
        switch(b.id)
        {
        case BOSS_WARDEN:
            update_warden();
            break;

        case BOSS_HIVE:
            update_hive();
            break;

        default:
            update_overmind();
            break;
        }
    }
    else
    {
        update_dying();

        if(b.state == BOSS_NONE)
        {
            return;
        }
    }

    if(b.id == BOSS_OVERMIND)
    {
        update_pods();
    }
}

void boss_render(void)
{
    if(b.state == BOSS_NONE)
    {
        return;
    }

    /* Flicker while breaking apart. */
    if(b.state == BOSS_DYING && b.timer >= 30 && (b.timer & 2))
    {
        return;
    }

    /* Hit flash as a strobe: under constant fire the boss must not stay solid white. */
    int flash = (b.flash && (b.timer & 2)) ? SPR_FLASH : 0;
    int frame;

    switch(b.id)
    {
    case BOSS_WARDEN:
        frame = b.open_frames ? 1 : 0;
        break;

    case BOSS_HIVE:
        frame = (b.open_frames || b.phase == 2) ? 2 : (b.timer >> 3) & 1;
        break;

    default:
        frame = b.phase;
        break;
    }

    /* front to back: pods, body, rear */
    for(int i = 0; i < 2; ++i)
    {
        if(b.pods[i].alive)
        {
            sprites_draw(GEN_SPR_BOSS_POD, (b.timer >> 4) & 1, b.pods[i].pos, b.pods[i].flash ? SPR_FLASH : 0);
        }
    }

    sprites_draw(infos[b.id].sheet, frame, b.pos, flash);

    if(b.id == BOSS_OVERMIND)
    {
        sprites_draw(GEN_SPR_BOSS_OVERMIND_REAR, (b.timer >> 2) & 1, v2(b.pos.x + FX(REAR_OFFSET), b.pos.y), flash);
    }
}
