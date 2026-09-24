#ifndef SS_SHOTS_H
#define SS_SHOTS_H

#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_sprite_ptr.h"

#include "ss_collision.h"
#include "ss_constants.h"
#include "ss_pool.h"
#include "ss_weapon_data.h"

namespace ss
{

class world;

struct player_shot
{
    bool active = false;
    shot_kind kind = shot_kind::NORMAL;
    bn::fixed_point position;
    bn::fixed_point velocity;
    int damage = 0;
    int angle = 0;              // missiles and dots: binary angle (65536 = turn), y-down
    bool homing = false;        // missiles and dots: steer toward the nearest target
    int frame = -1;
    int life = 0;
    unsigned hit_mask = 0;      // piercing shots: enemies already hit (bit per enemy slot)
    int boss_cooldown = 0;      // charged beam: frames until the boss can be hit again
    bn::optional<bn::sprite_ptr> sprite;

    void clear()
    {
        active = false;
        sprite.reset();
    }

    [[nodiscard]] hitbox box() const;

    [[nodiscard]] bool pierces() const
    {
        return kind == shot_kind::BEAM || kind == shot_kind::LASER;
    }
};

class player_shots
{

public:
    void fire(world& w, shot_kind kind, const bn::fixed_point& position, const bn::fixed_point& velocity,
              int damage);

    void fire_missile(world& w, const bn::fixed_point& position, int angle, bool homing);

    void fire_dot(world& w, const bn::fixed_point& position);

    void update(world& w);

    void release(player_shot& shot)
    {
        _pool.release(shot);
    }

    [[nodiscard]] int count() const
    {
        return _pool.count();
    }

    [[nodiscard]] int dropped() const
    {
        return _pool.dropped();
    }

    /// Active shots of one kind (caps for missiles and dots).
    [[nodiscard]] int count_of(shot_kind kind) const;

    pool<player_shot, max_player_shots>& items()
    {
        return _pool;
    }

private:
    pool<player_shot, max_player_shots> _pool;

    void _steer(world& w, player_shot& shot, int turn_rate, bn::fixed speed);
};

}

#endif
