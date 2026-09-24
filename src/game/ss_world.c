#include "ss_world.h"

#include "ss_audio.h"
#include "ss_boss.h"
#include "ss_bullets.h"
#include "ss_effects.h"
#include "ss_enemies.h"
#include "ss_frame.h"
#include "ss_hud.h"
#include "ss_input.h"
#include "ss_level.h"
#include "ss_player.h"
#include "ss_power_data.h"
#include "ss_powerups.h"
#include "ss_save.h"
#include "ss_shots.h"
#include "ss_video.h"

session game;
world_state world;

#if SS_TEST_HOOKS
test_controls test_ctl;
#endif

/* ----- session ------------------------------------------------------------------------------------ */

void session_new_game(int stage)
{
    int hiscore = game.hiscore;
    game.stage = stage;
    game.lives = START_LIVES;
    game.score = 0;
    game.hiscore = hiscore;
    game.deaths = 0;
    game.gear.power = 0;
    game.gear.shield = 0;
}

void game_add_score(int points)
{
    game.score = MIN(game.score + points, 99999990);

    if(game.score > game.hiscore)
    {
        game.hiscore = game.score;
    }
}

/* ----- stage lifetime ----------------------------------------------------------------------------- */

void world_init(void)
{
    const stage_def* stage = stage_definition(game.stage);
    world.stage = stage;
    rng_seed(&world.random, 0x5EED0000u + (u32) game.stage * 7919u);
    world.scroll_x = 0;
    world.scroll_speed = stage->scroll_speed;
    world.stage_frame = 0;
    world.shake_frames = 0;
    world.shake_amplitude = 0;
    world.game_over_timer = 0;
    world.clear_timer = 0;
    world.flash_frames = 0;
    world.shockwaves = 0;
    video_camera_x = 0;
    video_camera_y = 0;

    shots_reset();
    enemies_reset();
    bullets_reset();
    effects_reset();
    powerups_reset();
    boss_reset();
    runner_reset();
    video_load_boss_graphics(game.stage);
    terrain_init(stage);
    backgrounds_init((backdrop_type) stage->backdrop);
    hud_init();
    player_init();

    terrain_update(world.scroll_x);
    backgrounds_update(world.scroll_x);
    hud_show_banner(stage->name, stage->subtitle);
    audio_play_music((music_id) stage->music);
}

void world_release(void)
{
    video_show_terrain(-1);         /* hides BG1 */
    video_show_backgrounds(BACKDROP_NONE);
    video_camera_x = 0;
    video_camera_y = 0;
}

/* ----- services used by the entity systems --------------------------------------------------------- */

bool world_fire_held(void)
{
#if SS_TEST_HOOKS
    if(test_ctl.autofire)
    {
        return true;
    }
#endif

    return input_held(KEY_A);
}

bool world_charge_held(void)
{
    return input_held(KEY_B);
}

bool world_invincible(void)
{
#if SS_TEST_HOOKS
    if(test_ctl.invincible)
    {
        return true;
    }
#endif

    return world.clear_timer > 0;
}

void world_shake(int frames, int amplitude)
{
    if(amplitude >= world.shake_amplitude || world.shake_frames == 0)
    {
        world.shake_frames = frames;
        world.shake_amplitude = amplitude;
    }
}

void world_notify_boss_defeated(void)
{
    if(! world.game_over_timer)
    {
        world.clear_timer = 150;
    }
}

void world_notify_player_out_of_lives(void)
{
    world.game_over_timer = 120;
}

void world_set_power(int power)
{
    loadout* gear = &game.gear;
    int previous = gear->power;
    gear->power = SS_CLAMP(power, 0, MAX_POWER);

    if(has_power(gear->power, POWER_SHIELD) && ! has_power(previous, POWER_SHIELD))
    {
        gear->shield = MAX_SHIELD;
    }

    player_refresh_power(previous);
}

void world_shockwave(void)
{
    ++world.shockwaves;
    enemies_shockwave();
    bullets_cancel_all(false);
    boss_shockwave_hit();
    player_shockwave_invulnerability(SHOCKWAVE_INVULNERABLE_FRAMES);
    effects_explosion_big(v2(player_position().x + FX(24), player_position().y), 1);
    world_shake(20, 3);
    audio_play(SFX_EXPLODE_BIG_ID);
    world.flash_frames = 10;
}

/* ----- per frame ---------------------------------------------------------------------------------- */

static void collide(void)
{
    /* player projectiles vs enemies and boss */
    for(int i = 0; i < MAX_PLAYER_SHOTS; ++i)
    {
        player_shot* s = &shots[i];

        if(! s->active)
        {
            continue;
        }

        hitbox box = shot_box(s);

        for(int k = 0; k < MAX_ENEMIES; ++k)
        {
            enemy* e = &enemies[k];

            if(! e->active || e->pos.x > FX(124) || ! hit_test(box, enemy_box(e)))
            {
                continue;
            }

            if(shot_pierces(s))
            {
                u32 bit = 1u << k;

                if(! (s->hit_mask & bit))
                {
                    s->hit_mask |= bit;
                    enemies_damage(e, s->damage);
                }
            }
            else
            {
                enemies_damage(e, s->damage);
                effects_spark(s->pos);
                shots_release(s);
                break;
            }
        }

        if(! s->active || ! boss_fighting())
        {
            continue;
        }

        /* The charged beam passes through the boss (with a hit cooldown); everything else, lasers
         * included, is stopped by it. */
        if(s->kind == SHOT_BEAM)
        {
            if(! s->boss_cooldown && boss_take_hit(box, s->damage))
            {
                s->boss_cooldown = 10;
            }
        }
        else if(boss_take_hit(box, s->damage))
        {
            effects_spark(s->pos);
            shots_release(s);
        }
    }

    if(! player_alive())
    {
        return;
    }

    hitbox core = player_core_hitbox();

    /* player vs enemy bodies */
    for(int k = 0; k < MAX_ENEMIES; ++k)
    {
        enemy* e = &enemies[k];

        if(e->active && hit_test(core, enemy_box(e)))
        {
            bool survived = player_hit();

            if(! enemy_defs[e->kind].contact_damage_immune)
            {
                enemies_destroy(e, true);
            }

            if(! survived)
            {
                return;
            }
        }
    }

    /* player vs enemy bullets */
    for(int k = 0; k < MAX_ENEMY_BULLETS; ++k)
    {
        enemy_bullet* b = &bullets[k];

        if(b->active && hit_test(core, bullet_box(b)))
        {
            bullets_release(b);

            if(! player_hit())
            {
                return;
            }
        }
    }

    /* player vs boss body and terrain */
    if(boss_touches(core) && ! player_hit())
    {
        return;
    }

    if(terrain_blocks(core, world.scroll_x) && ! player_hit())
    {
        return;
    }

    /* player vs capsules */
    hitbox pickup = player_pickup_hitbox();

    for(int k = 0; k < MAX_POWERUPS; ++k)
    {
        if(powerups[k].active && hit_test(pickup, powerup_box(&powerups[k])))
        {
            powerups_collect(&powerups[k]);
        }
    }
}

static void update_camera(void)
{
    if(world.shake_frames > 0)
    {
        --world.shake_frames;
        int amplitude = world.shake_frames > 8 ? world.shake_amplitude
                                               : (world.shake_amplitude * world.shake_frames + 7) / 8;
        int dx = (world.stage_frame & 1) ? amplitude : -amplitude;
        int dy = (world.stage_frame & 2) ? amplitude : -amplitude;
        video_camera_x = dx;
        video_camera_y = dy / 2;
    }
    else
    {
        video_camera_x = 0;
        video_camera_y = 0;
    }
}

static void update_flash(void)
{
    /* Shockwave: white screen flash fading out (hardware brightness, no palette work). */
    if(world.flash_frames)
    {
        --world.flash_frames;
        video_set_fade(world.flash_frames, true, FADE_ALL);
    }
}

static void test_hooks(void)
{
#if SS_TEST_HOOKS
    if(SS_DEBUG && input_pressed(KEY_SELECT))
    {
        hud_toggle_debug();
    }

    bool skip = test_ctl.skip_to_boss || (SS_DEBUG && input_held(KEY_L) && input_pressed(KEY_R));

    if(skip && ! runner_boss_started())
    {
        test_ctl.skip_to_boss = false;
        enemies_destroy_all();
        bullets_cancel_all(true);
        runner_skip_to_boss();
    }

    if(test_ctl.set_power)
    {
        world_set_power(MIN(test_ctl.set_power - 1, MAX_POWER));
        test_ctl.set_power = 0;
    }
#endif
}

world_result world_update(void)
{
    ++world.stage_frame;
    test_hooks();
    world.scroll_x += world.scroll_speed;

    PROF_BEGIN();
    runner_update();
    PROF_MARK(PROF_STAGE);
    player_update();
    PROF_MARK(PROF_PLAYER);
    shots_update();
    PROF_MARK(PROF_SHOTS);
    enemies_update();
    PROF_MARK(PROF_ENEMIES);
    boss_update();
    PROF_MARK(PROF_BOSS);
    bullets_update();
    PROF_MARK(PROF_BULLETS);
    powerups_update();
    PROF_MARK(PROF_POWERUPS);
    collide();
    PROF_MARK(PROF_COLLIDE);
    effects_update();
    PROF_MARK(PROF_EFFECTS);
    terrain_update(world.scroll_x);
    PROF_MARK(PROF_TERRAIN);
    backgrounds_update(world.scroll_x);
    update_camera();
    update_flash();
    PROF_MARK(PROF_BACKGROUNDS);
    hud_update();
    PROF_MARK(PROF_HUD);

    if(world.game_over_timer && --world.game_over_timer == 0)
    {
        return WORLD_GAME_OVER;
    }

    if(world.clear_timer && --world.clear_timer == 0)
    {
        return game.stage + 1 >= STAGE_COUNT ? WORLD_GAME_COMPLETE : WORLD_STAGE_CLEARED;
    }

    return WORLD_RUNNING;
}

bool world_update_outro(void)
{
    ++world.stage_frame;
    world.scroll_x += world.scroll_speed;
    player_update();
    shots_update();
    effects_update();
    terrain_update(world.scroll_x);
    backgrounds_update(world.scroll_x);
    update_camera();
    update_flash();
    return player_outro_done();
}

void world_render(void)
{
    /* front to back */
    hud_render();
    player_render();
    effects_render();
    bullets_render();
    shots_render();
    powerups_render();
    enemies_render();
    boss_render();
    PROF_MARK(PROF_RENDER);
}
