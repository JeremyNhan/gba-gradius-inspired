#include "ss_shots.h"

#include "bn_sprite_items_shot_charge.h"
#include "bn_sprite_items_shot_laser.h"
#include "bn_sprite_items_shot_missile.h"
#include "bn_sprite_items_shot_normal.h"
#include "bn_sprite_items_shot_spread.h"

#include "ss_math.h"
#include "ss_sprite_util.h"
#include "ss_world.h"

namespace ss
{

namespace
{
    const bn::sprite_item& item_of(shot_kind kind)
    {
        switch(kind)
        {

        case shot_kind::LASER:
            return bn::sprite_items::shot_laser;

        case shot_kind::DOT:
            return bn::sprite_items::shot_spread;

        case shot_kind::MISSILE:
            return bn::sprite_items::shot_missile;

        case shot_kind::BEAM:
            return bn::sprite_items::shot_charge;

        default:
            return bn::sprite_items::shot_normal;
        }
    }

    /// Laser frames: 0 level, 1 rising, 2 falling (spread laser diagonals).
    int laser_frame(const bn::fixed_point& velocity)
    {
        return velocity.y() < 0 ? 1 : velocity.y() > 0 ? 2 : 0;
    }

    /// Missile sprite frames are drawn every 45 degrees counter-clockwise (visually) from "right".
    int missile_frame(int angle)
    {
        return (((angle_turn - angle) + 4096) >> 13) & 7;
    }
}

hitbox player_shot::box() const
{
    switch(kind)
    {

    case shot_kind::BEAM:
        return make_hitbox(position, 13, 7);

    case shot_kind::LASER:
        return make_hitbox(position, 12, 2);

    case shot_kind::MISSILE:
    case shot_kind::DOT:
        return make_hitbox(position, 3, 3);

    default:
        return make_hitbox(position, 4, 2);
    }
}

void player_shots::fire(world& w, shot_kind kind, const bn::fixed_point& position, const bn::fixed_point& velocity,
                        int damage)
{
    player_shot* shot = _pool.spawn();

    if(! shot)
    {
        return;     // pool full: the projectile is simply not fired
    }

    shot->kind = kind;
    shot->position = position;
    shot->velocity = velocity;
    shot->damage = damage;
    shot->frame = 0;
    shot->life = 0;
    shot->hit_mask = 0;
    shot->boss_cooldown = 0;
    shot->homing = false;

    if(kind == shot_kind::LASER)
    {
        shot->frame = laser_frame(velocity);
    }

    shot->sprite = make_sprite(item_of(kind), position, shot->frame, z_shots, w.camera);
}

void player_shots::fire_dot(world& w, const bn::fixed_point& position)
{
    player_shot* shot = _pool.spawn();

    if(! shot)
    {
        return;
    }

    shot->kind = shot_kind::DOT;
    shot->position = position;
    shot->angle = 0;
    shot->homing = true;
    shot->velocity = bn::fixed_point(dot_speed, 0);
    shot->damage = dot_damage;
    shot->life = 0;
    shot->hit_mask = 0;
    shot->boss_cooldown = 0;
    shot->frame = 0;
    shot->sprite = make_sprite(bn::sprite_items::shot_spread, position, 0, z_shots, w.camera);
}

void player_shots::fire_missile(world& w, const bn::fixed_point& position, int angle, bool homing)
{
    player_shot* shot = _pool.spawn();

    if(! shot)
    {
        return;
    }

    shot->kind = shot_kind::MISSILE;
    shot->position = position;
    shot->angle = angle & 0xFFFF;
    shot->homing = homing;
    shot->velocity = direction(shot->angle, missile_speed);
    shot->damage = missile_damage;
    shot->life = 0;
    shot->hit_mask = 0;
    shot->boss_cooldown = 0;
    shot->frame = missile_frame(shot->angle);
    shot->sprite = make_sprite(bn::sprite_items::shot_missile, position, shot->frame, z_shots, w.camera);
}

int player_shots::count_of(shot_kind kind) const
{
    int result = 0;

    for(const player_shot& shot : _pool)
    {
        if(shot.active && shot.kind == kind)
        {
            ++result;
        }
    }

    return result;
}

void player_shots::_steer(world& w, player_shot& shot, int turn_rate, bn::fixed speed)
{
    bn::optional<bn::fixed_point> target = w.foes.nearest_target(shot.position);

    if(! target)
    {
        target = w.big_boss.target_point();
    }

    if(target && shot.life > 6)
    {
        int wanted = angle_to(shot.position, *target);
        int delta = angle_delta(shot.angle, wanted);

        if(delta > turn_rate)
        {
            delta = turn_rate;
        }
        else if(delta < -turn_rate)
        {
            delta = -turn_rate;
        }

        shot.angle = (shot.angle + delta) & 0xFFFF;
    }

    shot.velocity = direction(shot.angle, speed);
}

void player_shots::update(world& w)
{
    for(player_shot& shot : _pool)
    {
        if(! shot.active)
        {
            continue;
        }

        ++shot.life;

        if(shot.boss_cooldown)
        {
            --shot.boss_cooldown;
        }

        if(shot.kind == shot_kind::MISSILE)
        {
            if(shot.homing)
            {
                _steer(w, shot, missile_turn_rate, missile_speed);
                set_frame(shot.sprite, bn::sprite_items::shot_missile, shot.frame, missile_frame(shot.angle));
            }
        }
        else if(shot.kind == shot_kind::DOT)
        {
            _steer(w, shot, dot_turn_rate, dot_speed);
        }
        else if(shot.kind == shot_kind::BEAM)
        {
            set_frame(shot.sprite, bn::sprite_items::shot_charge, shot.frame, (shot.life >> 2) & 1);
        }

        shot.position += shot.velocity;

        bool dead = off_screen(shot.position, 24) || shot.life > 150;

        // Terrain stops everything except the beam.
        if(! dead && shot.kind != shot_kind::BEAM && w.ground.blocks(shot.box(), w.scroll_x))
        {
            w.fx.spark(w, shot.position);
            dead = true;
        }

        if(dead)
        {
            _pool.release(shot);
        }
        else if(shot.sprite)
        {
            shot.sprite->set_position(shot.position);
        }
    }
}

}
