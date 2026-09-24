#include "ss_world.h"

#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_sprites.h"
#include "bn_timer.h"
#include "bn_timers.h"

#include "ss_audio.h"
#include "ss_telemetry.h"
#include "ss_text.h"

namespace ss
{

world::world(session& game_session, text& text_generator) :
    game(game_session),
    txt(text_generator),
    camera(bn::camera_ptr::create(0, 0)),
    stage(stage_definition(game_session.stage)),
    scroll_speed(stage.scroll_speed),
    ground(stage.terrain, stage.terrain_keys, camera),
    bgs(stage.backdrop, camera),
    ship(*this),
    display(text_generator)
{
    // bn::random starts from a fixed seed; advance it per stage so each stage has its own
    // (but always identical) sequence.
    for(int index = 0; index < game.stage * 17; ++index)
    {
        rng.update();
    }

    ground.update(scroll_x);
    bgs.update(scroll_x);
    display.show_banner(stage.name, stage.subtitle);
    audio::play_music(stage.music);
    ss_telemetry.cpu_pct_max = 0;
}

bool world::fire_held() const
{
#if SS_TEST_HOOKS
    if(ss_telemetry.ctl_autofire)
    {
        return true;
    }
#endif

    return bn::keypad::a_held();
}

bool world::charge_held() const
{
    return bn::keypad::b_held();
}

bool world::invincible() const
{
#if SS_TEST_HOOKS
    if(ss_telemetry.ctl_invincible)
    {
        return true;
    }
#endif

    return _clear_timer > 0;
}

void world::shake(int frames, int amplitude)
{
    if(amplitude >= _shake_amplitude || _shake_frames == 0)
    {
        _shake_frames = frames;
        _shake_amplitude = amplitude;
    }
}

void world::add_score(int points)
{
    game.add_score(points);
}

void world::notify_boss_defeated()
{
    if(! _game_over_timer)
    {
        _clear_timer = 150;
    }
}

world::result world::update()
{
    ++stage_frame;

#if SS_DEBUG
    if(bn::keypad::select_pressed())
    {
        display.toggle_debug();
    }
#endif

#if SS_TEST_HOOKS
    bool skip = ss_telemetry.ctl_skip_to_boss || (SS_DEBUG && bn::keypad::l_held() && bn::keypad::r_pressed());

    if(skip && ! runner.boss_started())
    {
        ss_telemetry.ctl_skip_to_boss = 0;
        foes.destroy_all(*this);
        bullets.cancel_all(*this);
        runner.skip_to_boss(*this);
    }
#endif

    scroll_x += scroll_speed;

    // Per-system cost (test-hook builds only): each SS_PROFILE stores the ticks spent since the previous
    // one into ss_telemetry.prof[slot], in 1/1000 of a frame. tests/full_run.lua reports them.
#if SS_TEST_HOOKS
    bn::timer prof_timer;
    #define SS_PROFILE(slot) ss_telemetry.prof[slot] =             uint16_t(prof_timer.elapsed_ticks_with_restart() * 1000 / bn::timers::ticks_per_frame())
#else
    #define SS_PROFILE(slot) (void) 0
#endif

    runner.update(*this);
    SS_PROFILE(0);
    ship.update(*this);
    SS_PROFILE(1);
    shots.update(*this);
    SS_PROFILE(2);
    foes.update(*this);
    SS_PROFILE(3);
    big_boss.update(*this);
    SS_PROFILE(4);
    bullets.update(*this);
    SS_PROFILE(5);
    items.update(*this);
    SS_PROFILE(6);
    _collide();
    SS_PROFILE(7);
    fx.update(*this);
    SS_PROFILE(8);
    ground.update(scroll_x);
    SS_PROFILE(9);
    bgs.update(scroll_x);
    _update_camera();
    SS_PROFILE(10);
    display.update(*this);
    SS_PROFILE(11);
    #undef SS_PROFILE
    _update_telemetry();

    if(_game_over_timer && --_game_over_timer == 0)
    {
        return result::GAME_OVER;
    }

    if(_clear_timer && --_clear_timer == 0)
    {
        return game.stage + 1 >= stage_count ? result::GAME_COMPLETE : result::STAGE_CLEARED;
    }

    return result::NONE;
}

bool world::update_outro()
{
    ++stage_frame;
    scroll_x += scroll_speed;
    ship.update(*this);
    shots.update(*this);
    fx.update(*this);
    ground.update(scroll_x);
    bgs.update(scroll_x);
    _update_camera();
    _update_telemetry();
    return ship.outro_done();
}

void world::_collide()
{
    // --- player projectiles vs enemies and boss -----------------------------------------------------
    for(player_shot& shot : shots.items())
    {
        if(! shot.active)
        {
            continue;
        }

        hitbox shot_box = shot.box();

        for(enemy& e : foes.items())
        {
            if(! e.active || e.position.x() > 124 || ! shot_box.intersects(e.box()))
            {
                continue;
            }

            if(shot.pierces())
            {
                unsigned bit = 1u << foes.items().index_of(e);

                if(! (shot.hit_mask & bit))
                {
                    shot.hit_mask |= bit;
                    foes.damage(*this, e, shot.damage);
                }
            }
            else
            {
                foes.damage(*this, e, shot.damage);
                fx.spark(*this, shot.position);
                shots.release(shot);
                break;
            }
        }

        if(! shot.active || ! big_boss.fighting())
        {
            continue;
        }

        if(shot.pierces())
        {
            if(! shot.boss_cooldown && big_boss.take_hit(*this, shot_box, shot.damage))
            {
                shot.boss_cooldown = 10;
            }
        }
        else if(big_boss.take_hit(*this, shot_box, shot.damage))
        {
            fx.spark(*this, shot.position);
            shots.release(shot);
        }
    }

    if(! ship.alive())
    {
        return;
    }

    hitbox core = ship.core_hitbox();

    // --- player vs enemy bodies --------------------------------------------------------------------
    for(enemy& e : foes.items())
    {
        if(e.active && core.intersects(e.box()))
        {
            bool survived = ship.hit(*this);

            if(! e.def->contact_damage_immune)
            {
                foes.destroy(*this, e, true);
            }

            if(! survived)
            {
                return;
            }
        }
    }

    // --- player vs enemy bullets -------------------------------------------------------------------
    for(enemy_bullet& bullet : bullets.items())
    {
        if(bullet.active && core.intersects(bullet.box()))
        {
            bullets.release(bullet);

            if(! ship.hit(*this))
            {
                return;
            }
        }
    }

    // --- player vs boss body and terrain -------------------------------------------------------------
    if(big_boss.touches(core) && ! ship.hit(*this))
    {
        return;
    }

    if(ground.blocks(core, scroll_x) && ! ship.hit(*this))
    {
        return;
    }

    // --- player vs power-ups -----------------------------------------------------------------------
    hitbox pickup = ship.pickup_hitbox();

    for(powerup& item : items.items())
    {
        if(item.active && pickup.intersects(item.box()))
        {
            items.collect(*this, item);
        }
    }
}

void world::_update_camera()
{
    if(_shake_frames > 0)
    {
        --_shake_frames;
        int amplitude = _shake_frames > 8 ? _shake_amplitude : (_shake_amplitude * _shake_frames + 7) / 8;
        int dx = (stage_frame & 1) ? amplitude : -amplitude;
        int dy = (stage_frame & 2) ? amplitude : -amplitude;
        camera->set_position(dx, dy / 2);
    }
    else
    {
        camera->set_position(0, 0);
    }
}

void world::_update_telemetry()
{
    volatile ss_telemetry_block& t = ss_telemetry;
    t.stage_frame = unsigned(stage_frame);
    t.score = unsigned(game.score);
    t.hiscore = unsigned(game.hiscore);
    t.stage = (uint8_t) game.stage;
    t.lives = (uint8_t) game.lives;
    t.boss_active = big_boss.active() ? 1 : 0;
    t.player_x = (int16_t) ship.position().x().round_integer();
    t.player_y = (int16_t) ship.position().y().round_integer();
    t.boss_hp = (int16_t) big_boss.hp();
    t.boss_hp_max = (int16_t) big_boss.hp_max();
    t.boss_y = (int16_t) big_boss.position().y().round_integer();
    t.enemies = (uint8_t) foes.count();
    t.enemy_bullets = (uint8_t) bullets.count();
    t.player_shots = (uint8_t) shots.count();
    t.effects = (uint8_t) fx.count();
    t.powerups = (uint8_t) items.count();
    t.weapon = (uint8_t) game.gear.weapon;
    t.weapon_level = (uint8_t) game.gear.weapon_level;
    t.missile_level = (uint8_t) game.gear.missile_level;
    t.speed_level = (uint8_t) game.gear.speed_level;
    t.shield = (uint8_t) game.gear.shield;
    t.pool_drops = (uint16_t) (shots.dropped() + foes.dropped() + bullets.dropped() + fx.dropped());
    t.player_alive = ship.alive() ? 1 : 0;
    t.deaths = (uint8_t) game.deaths;
    t.sprites_used = (uint8_t) bn::sprites::used_items_count();
}

}
