#include "ss_player.h"

#include "bn_keypad.h"

#include "bn_sprite_items_charge_glow.h"
#include "bn_sprite_items_player.h"
#include "bn_sprite_items_shield.h"

#include "ss_audio.h"
#include "ss_math.h"
#include "ss_sprite_util.h"
#include "ss_weapon_data.h"
#include "ss_world.h"

namespace ss
{

namespace
{
    constexpr int min_x = -112;
    constexpr int max_x = 112;
    constexpr int min_y = play_top + 6;
    constexpr int max_y = play_bottom - 16;     // keep clear of the status line
    const bn::fixed_point spawn_position(-80, 0);
}

player::player(world& w) :
    _position(spawn_position)
{
    _sprite = make_sprite(bn::sprite_items::player, _position, 0, z_player, w.camera);
    _frame = 0;
    _invulnerable_frames = 60;
    refresh_shield_sprite(w);
}

void player::refresh_shield_sprite(world& w)
{
    if(w.game.gear.shield > 0 && (_state == state::FLYING || _state == state::OUTRO))
    {
        if(! _shield_sprite)
        {
            _shield_sprite = make_sprite(bn::sprite_items::shield, _position, 0, z_player - 1, w.camera);
        }
    }
    else
    {
        _shield_sprite.reset();
    }
}

void player::update(world& w)
{
    ++_anim_counter;

    switch(_state)
    {

    case state::FLYING:
        _move(w);
        _fire(w);
        _update_charge(w);

        if(_invulnerable_frames)
        {
            --_invulnerable_frames;
        }
        break;

    case state::DEAD:
        if(--_dead_frames <= 0)
        {
            if(w.game.lives > 0)
            {
                _respawn(w);
            }
            else
            {
                _state = state::GONE;
            }
        }
        break;

    case state::OUTRO:
        _position.set_x(_position.x() + bn::fixed(0.5) + bn::fixed(_anim_counter & 63) / 16);
        _charge = 0;
        _charge_sprite.reset();
        break;

    default:
        break;
    }

    _update_sprites(w);
}

void player::_move(world& w)
{
    int dx = int(bn::keypad::right_held()) - int(bn::keypad::left_held());
    int dy = int(bn::keypad::down_held()) - int(bn::keypad::up_held());
    bn::fixed speed = speed_by_level[w.game.gear.speed_level - 1];

    if(dx && dy)
    {
        speed *= bn::fixed(0.72);   // keep diagonal speed close to straight speed
    }

    bn::fixed x = bn::clamp(_position.x() + speed * dx, bn::fixed(min_x), bn::fixed(max_x));
    bn::fixed y = bn::clamp(_position.y() + speed * dy, bn::fixed(min_y), bn::fixed(max_y));
    _position = bn::fixed_point(x, y);

    // Banking: lean after holding a vertical direction for a few frames.
    if(dy != _bank)
    {
        if(++_bank_timer >= 4)
        {
            _bank = dy;
            _bank_timer = 0;
        }
    }
    else
    {
        _bank_timer = 0;
    }
}

void player::_fire(world& w)
{
    const loadout& gear = w.game.gear;

    if(_fire_cooldown)
    {
        --_fire_cooldown;
    }

    if(_missile_cooldown)
    {
        --_missile_cooldown;
    }

    if(! w.fire_held())
    {
        return;
    }

    if(! _fire_cooldown)
    {
        const weapon_def& weapon = weapon_defs[int(gear.weapon)];
        const weapon_level_def& level = weapon.levels[gear.weapon_level - 1];

        for(int index = 0; index < level.count; ++index)
        {
            const shot_spec& spec = level.shots[index];
            w.shots.fire(w, weapon.kind, _position + bn::fixed_point(10, spec.dy), bn::fixed_point(spec.vx, spec.vy),
                         level.damage);
        }

        _fire_cooldown = level.fire_interval;
        audio::play(weapon.kind == shot_kind::SPREAD ? audio::sfx::SPREAD : audio::sfx::SHOT);
    }

    if(gear.missile_level > 0 && ! _missile_cooldown && w.shots.missile_count() < gear.missile_level * 2)
    {
        w.shots.fire_missile(w, _position + bn::fixed_point(2, 6), degrees(35));

        if(gear.missile_level > 1)
        {
            w.shots.fire_missile(w, _position + bn::fixed_point(2, -6), degrees(-35));
        }

        _missile_cooldown = missile_interval;
        audio::play(audio::sfx::MISSILE);
    }
}

void player::_update_charge(world& w)
{
    if(w.charge_held())
    {
        if(_charge < charge_frames)
        {
            ++_charge;

            if(_charge == charge_frames)
            {
                audio::play(audio::sfx::CHARGE_READY);
            }
        }

        if(_charge > 8)
        {
            int frame = bn::min(3, (_charge - 8) * 4 / (charge_frames - 8));

            if(_charge >= charge_frames)
            {
                frame = 2 + ((_anim_counter >> 2) & 1);
            }

            if(! _charge_sprite)
            {
                _charge_sprite = make_sprite(bn::sprite_items::charge_glow, _position, frame, z_player - 1, w.camera);
                _charge_frame = frame;
            }

            set_frame(_charge_sprite, bn::sprite_items::charge_glow, _charge_frame, frame);
        }

        return;
    }

    if(_charge >= charge_frames)
    {
        w.shots.fire(w, shot_kind::BEAM, _position + bn::fixed_point(20, 0), bn::fixed_point(beam_speed, 0),
                     beam_damage);
        audio::play(audio::sfx::BEAM);
        w.shake(6, 1);
    }

    _charge = 0;
    _charge_sprite.reset();
    _charge_frame = -1;
}

bool player::hit(world& w)
{
    if(_state != state::FLYING || _invulnerable_frames > 0 || w.invincible())
    {
        return true;
    }

    loadout& gear = w.game.gear;

    if(gear.shield > 0)
    {
        --gear.shield;
        _invulnerable_frames = 60;
        audio::play(audio::sfx::SHIELD_HIT);
        w.fx.spark(w, _position);
        refresh_shield_sprite(w);
        return true;
    }

    // Destroyed.
    w.fx.explosion_big(w, _position);
    w.fx.explosion_small(w, _position + bn::fixed_point(-8, -6), 6);
    w.fx.explosion_small(w, _position + bn::fixed_point(6, 7), 12);
    w.shake(30, 3);
    audio::play(audio::sfx::PLAYER_DEATH);

    _state = state::DEAD;
    _dead_frames = respawn_frames;
    _charge = 0;
    _charge_sprite.reset();
    _shield_sprite.reset();

    --w.game.lives;
    ++w.game.deaths;

    // Losing a ship costs one level of every upgrade.
    gear.shield = 0;
    gear.weapon_level = bn::max(1, gear.weapon_level - 1);
    gear.missile_level = bn::max(0, gear.missile_level - 1);
    gear.speed_level = bn::max(1, gear.speed_level - 1);

    if(w.game.lives <= 0)
    {
        w.game.lives = 0;
        w.notify_player_out_of_lives();
    }

    return false;
}

void player::_respawn(world& w)
{
    _state = state::FLYING;
    _position = bn::fixed_point(-110, 0);
    _invulnerable_frames = invulnerable_frames;
    _bank = 0;
    _fire_cooldown = 10;
    w.bullets.cancel_all(w);
}

void player::_update_sprites(world& w)
{
    bool visible = _state == state::FLYING || _state == state::OUTRO;

    // Slide in from the left edge after a respawn.
    if(_state == state::FLYING && _invulnerable_frames > invulnerable_frames - 30 && _position.x() < -80)
    {
        _position.set_x(_position.x() + 1);
    }

    if(_sprite)
    {
        int bank_index = _bank == 0 ? 0 : _bank < 0 ? 1 : 2;
        int frame = bank_index * 2 + ((_anim_counter >> 2) & 1);
        set_frame(_sprite, bn::sprite_items::player, _frame, frame);
        _sprite->set_position(_position);

        bool blink = _invulnerable_frames > 0 && (_anim_counter & 2);
        _sprite->set_visible(visible && ! blink);
    }

    if(_shield_sprite)
    {
        _shield_sprite->set_position(_position);
        _shield_sprite->set_visible(visible && (_anim_counter & 1));
        _shield_sprite->set_tiles(bn::sprite_items::shield.tiles_item(), (_anim_counter >> 3) & 1);
    }

    if(_charge_sprite)
    {
        _charge_sprite->set_position(_position + bn::fixed_point(12, 0));
    }

    (void) w;
}

}
