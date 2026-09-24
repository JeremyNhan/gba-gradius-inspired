#include "ss_player.h"

#include "gen_gfx.h"
#include "ss_audio.h"
#include "ss_bullets.h"
#include "ss_effects.h"
#include "ss_input.h"
#include "ss_power_data.h"
#include "ss_shots.h"
#include "ss_sprites.h"
#include "ss_weapon_data.h"
#include "ss_world.h"

#define MIN_X (-112)
#define MAX_X 112
#define MIN_Y (PLAY_TOP + 6)
#define MAX_Y (PLAY_BOTTOM - 16)    /* keep clear of the status line */

/* Additional shooters follow the ship's path: the trail records the ship position every frame it
 * moves, and shooter i sits (i + 1) * SHOOTER_SPACING entries behind the head. */
#define TRAIL_SIZE 32
#define SHOOTER_SPACING 12

typedef enum
{
    STATE_FLYING,
    STATE_DEAD,
    STATE_OUTRO,
    STATE_GONE
} player_state;

static struct
{
    player_state state;
    vec2 pos;
    int bank;               /* -1 nose up, 0 level, 1 nose down (smoothed) */
    int bank_timer;
    int anim;
    int invulnerable;
    int dead_frames;
    int fire_cooldown;
    int missile_cooldown;
    int dot_cooldown;
    int shooter_volley;     /* shooters still to fire this volley (one per frame) */
    int wave_timer;
    int charge;
    vec2 trail[TRAIL_SIZE];
    int trail_head;
} p;

static void reset_trail(void)
{
    /* A straight line behind the ship (1 px per entry): shooters start in a row behind it. */
    for(int age = 0; age < TRAIL_SIZE; ++age)
    {
        p.trail[(p.trail_head - age) & (TRAIL_SIZE - 1)] = v2(p.pos.x - FX(age), p.pos.y);
    }
}

static void record_trail(void)
{
    vec2 last = p.trail[p.trail_head];

    if(last.x != p.pos.x || last.y != p.pos.y)
    {
        p.trail_head = (p.trail_head + 1) & (TRAIL_SIZE - 1);
        p.trail[p.trail_head] = p.pos;
    }
}

static vec2 shooter_position(int index)
{
    return p.trail[(p.trail_head - (index + 1) * SHOOTER_SPACING) & (TRAIL_SIZE - 1)];
}

void player_init(void)
{
    p.state = STATE_FLYING;
    p.pos = v2i(-80, 0);
    p.bank = 0;
    p.bank_timer = 0;
    p.anim = 0;
    p.invulnerable = 60;
    p.dead_frames = 0;
    p.fire_cooldown = 0;
    p.missile_cooldown = 0;
    p.dot_cooldown = 0;
    p.shooter_volley = 0;
    p.wave_timer = 0;
    p.charge = 0;
    p.trail_head = 0;
    reset_trail();
    player_refresh_power(0);
}

bool player_alive(void)
{
    return p.state == STATE_FLYING;
}

vec2 player_position(void)
{
    return p.pos;
}

hitbox player_core_hitbox(void)
{
    return make_hitbox(p.pos, 2, 2);
}

hitbox player_pickup_hitbox(void)
{
    return make_hitbox(p.pos, 9, 7);
}

void player_start_outro(void)
{
    if(p.state == STATE_FLYING)
    {
        p.state = STATE_OUTRO;
    }
}

bool player_outro_done(void)
{
    return p.state == STATE_OUTRO && p.pos.x > FX(140);
}

void player_shockwave_invulnerability(int frames)
{
    p.invulnerable = MAX(p.invulnerable, frames);
}

void player_refresh_power(int previous_power)
{
    int power = game.gear.power;

    if(has_power(power, POWER_SHOCKWAVE) && ! has_power(previous_power, POWER_SHOCKWAVE))
    {
        p.wave_timer = SHOCKWAVE_INTERVAL - 1;     /* first shockwave right away */
    }
    else if(! has_power(power, POWER_SHOCKWAVE))
    {
        p.wave_timer = 0;
    }
}

/* ----- update ------------------------------------------------------------------------------------- */

static void move(void)
{
    int dx = (int) input_held(KEY_RIGHT) - (int) input_held(KEY_LEFT);
    int dy = (int) input_held(KEY_DOWN) - (int) input_held(KEY_UP);
    fx speed = PLAYER_SPEED;

    if(dx && dy)
    {
        speed = fx_mul(speed, FX_F(0.72));      /* keep diagonal speed close to straight speed */
    }

    p.pos.x = SS_CLAMP(p.pos.x + speed * dx, FX(MIN_X), FX(MAX_X));
    p.pos.y = SS_CLAMP(p.pos.y + speed * dy, FX(MIN_Y), FX(MAX_Y));
    record_trail();

    /* Banking: lean after holding a vertical direction for a few frames. */
    if(dy != p.bank)
    {
        if(++p.bank_timer >= 4)
        {
            p.bank = dy;
            p.bank_timer = 0;
        }
    }
    else
    {
        p.bank_timer = 0;
    }
}

static void fire_gun(vec2 origin)
{
    const gun_def* gun = &gun_defs[main_gun_of(game.gear.power)];

    for(int i = 0; i < gun->count; ++i)
    {
        const shot_spec* spec = &gun->shots[i];
        shots_fire((shot_kind) gun->kind, v2(origin.x + FX(10), origin.y + FX(spec->dy)), v2(spec->vx, spec->vy),
                   gun->damage);
    }
}

static void fire(void)
{
    int power = game.gear.power;
    int shooters = shooter_count_of(power);

    if(p.fire_cooldown)
    {
        --p.fire_cooldown;
    }

    if(p.missile_cooldown)
    {
        --p.missile_cooldown;
    }

    if(p.dot_cooldown)
    {
        --p.dot_cooldown;
    }

    /* Shooters fire the ship's volley one frame after another. */
    if(p.shooter_volley > 0)
    {
        if(p.shooter_volley <= shooters)
        {
            fire_gun(shooter_position(shooters - p.shooter_volley));
        }

        --p.shooter_volley;
    }

    if(! world_fire_held())
    {
        return;
    }

    if(! p.fire_cooldown)
    {
        const gun_def* gun = &gun_defs[main_gun_of(power)];
        fire_gun(p.pos);
        p.shooter_volley = shooters;
        p.fire_cooldown = gun->fire_interval;
        audio_play(gun->kind == SHOT_LASER ? SFX_SPREAD_ID : SFX_SHOT_ID);
    }

    if(has_power(power, POWER_HOMING_DOT) && ! p.dot_cooldown && shots_count_of(SHOT_DOT) < MAX_DOTS)
    {
        shots_fire_dot(v2(p.pos.x + FX(6), p.pos.y - FX(4)));
        p.dot_cooldown = DOT_INTERVAL;
    }

    missile_mode missiles = missile_mode_of(power);

    if(missiles == MISSILES_FORWARD)
    {
        if(! p.missile_cooldown && shots_count_of(SHOT_MISSILE) < MAX_FORWARD_MISSILES)
        {
            shots_fire_missile(v2(p.pos.x + FX(2), p.pos.y + FX(6)), DEGREES(20), false);
            p.missile_cooldown = MISSILE_INTERVAL;
            audio_play(SFX_MISSILE_ID);
        }
    }
    else if(missiles == MISSILES_HOMING)
    {
        if(! p.missile_cooldown && shots_count_of(SHOT_MISSILE) < MAX_HOMING_MISSILES)
        {
            shots_fire_missile(v2(p.pos.x + FX(2), p.pos.y + FX(6)), DEGREES(35), true);
            shots_fire_missile(v2(p.pos.x + FX(2), p.pos.y - FX(6)), DEGREES(-35), true);
            p.missile_cooldown = MISSILE_INTERVAL;
            audio_play(SFX_MISSILE_ID);
        }
    }
}

static void update_charge(void)
{
    if(world_charge_held())
    {
        if(p.charge < CHARGE_FRAMES && ++p.charge == CHARGE_FRAMES)
        {
            audio_play(SFX_CHARGE_READY_ID);
        }

        return;
    }

    if(p.charge >= CHARGE_FRAMES)
    {
        shots_fire(SHOT_BEAM, v2(p.pos.x + FX(20), p.pos.y), v2(BEAM_SPEED, 0), BEAM_DAMAGE);
        audio_play(SFX_BEAM_ID);
        world_shake(6, 1);
    }

    p.charge = 0;
}

static void update_shockwave(void)
{
    if(has_power(game.gear.power, POWER_SHOCKWAVE) && ++p.wave_timer >= SHOCKWAVE_INTERVAL)
    {
        p.wave_timer = 0;
        world_shockwave();
    }
}

static void respawn(void)
{
    p.state = STATE_FLYING;
    p.pos = v2i(-110, 0);
    p.invulnerable = INVULNERABLE_FRAMES;
    p.bank = 0;
    p.fire_cooldown = 10;
    reset_trail();
    bullets_cancel_all(true);
}

void player_update(void)
{
    ++p.anim;

    switch(p.state)
    {
    case STATE_FLYING:
        move();
        fire();
        update_charge();
        update_shockwave();

        if(p.invulnerable)
        {
            --p.invulnerable;
        }

        /* Slide in from the left edge after a respawn. */
        if(p.invulnerable > INVULNERABLE_FRAMES - 30 && p.pos.x < FX(-80))
        {
            p.pos.x += FX(1);
        }
        break;

    case STATE_DEAD:
        if(--p.dead_frames <= 0)
        {
            if(game.lives > 0)
            {
                respawn();
            }
            else
            {
                p.state = STATE_GONE;
            }
        }
        break;

    case STATE_OUTRO:
        p.pos.x += FX_F(0.5) + FX(p.anim & 63) / 16;
        p.charge = 0;
        record_trail();
        break;

    default:
        break;
    }
}

bool player_hit(void)
{
    if(p.state != STATE_FLYING || p.invulnerable > 0 || world_invincible())
    {
        return true;
    }

    loadout* gear = &game.gear;

    if(gear->shield > 0)
    {
        --gear->shield;
        p.invulnerable = 60;
        audio_play(SFX_SHIELD_HIT_ID);
        effects_spark(p.pos);
        return true;
    }

    /* Destroyed. */
    effects_explosion_big(p.pos, 0);
    effects_explosion_small(v2(p.pos.x - FX(8), p.pos.y - FX(6)), 6);
    effects_explosion_small(v2(p.pos.x + FX(6), p.pos.y + FX(7)), 12);
    world_shake(30, 3);
    audio_play(SFX_PLAYER_DEATH_ID);

    p.state = STATE_DEAD;
    p.dead_frames = RESPAWN_FRAMES;
    p.charge = 0;
    --game.lives;
    ++game.deaths;

    /* Losing a ship resets the power ladder to the normal shot. */
    int previous = gear->power;
    gear->shield = 0;
    gear->power = 0;
    player_refresh_power(previous);

    if(game.lives <= 0)
    {
        game.lives = 0;
        world_notify_player_out_of_lives();
    }

    return false;
}

/* ----- render ------------------------------------------------------------------------------------- */

void player_render(void)
{
    if(p.state != STATE_FLYING && p.state != STATE_OUTRO)
    {
        return;
    }

    /* front to back: shield, charge glow, ship, shooters */
    if(game.gear.shield > 0 && (p.anim & 1))
    {
        sprites_draw(GEN_SPR_SHIELD, (p.anim >> 3) & 1, p.pos, 0);
    }

    if(p.charge > 8)
    {
        int frame = MIN(3, (p.charge - 8) * 4 / (CHARGE_FRAMES - 8));

        if(p.charge >= CHARGE_FRAMES)
        {
            frame = 2 + ((p.anim >> 2) & 1);
        }

        sprites_draw(GEN_SPR_CHARGE_GLOW, frame, v2(p.pos.x + FX(12), p.pos.y), 0);
    }

    bool blink = p.invulnerable > 0 && (p.anim & 2);

    if(! blink)
    {
        int bank_index = p.bank == 0 ? 0 : p.bank < 0 ? 1 : 2;
        sprites_draw(GEN_SPR_PLAYER, bank_index * 2 + ((p.anim >> 2) & 1), p.pos, 0);
    }

    for(int i = 0; i < shooter_count_of(game.gear.power); ++i)
    {
        sprites_draw(GEN_SPR_SHOOTER, ((p.anim >> 3) + i) & 1, shooter_position(i), 0);
    }
}
