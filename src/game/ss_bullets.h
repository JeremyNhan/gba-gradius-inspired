#ifndef SS_BULLETS_H
#define SS_BULLETS_H

#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_sprite_ptr.h"

#include "ss_collision.h"
#include "ss_constants.h"
#include "ss_pool.h"

namespace ss
{

class world;

enum class bullet_kind : unsigned char
{
    SMALL,      // pink orb, standard
    BIG,        // orange orb, slightly larger hitbox
    NEEDLE      // fast horizontal dart
};

struct enemy_bullet
{
    bool active = false;
    bullet_kind kind = bullet_kind::SMALL;
    bn::fixed_point position;
    bn::fixed_point velocity;
    int timer = 0;
    int frame = -1;
    bn::optional<bn::sprite_ptr> sprite;

    void clear()
    {
        active = false;
        sprite.reset();
    }

    [[nodiscard]] hitbox box() const
    {
        return make_hitbox(position, kind == bullet_kind::BIG ? 3 : 2, kind == bullet_kind::NEEDLE ? 1 : 2);
    }
};

/**
 * Enemy projectile pool. If the pool is full, new bullets are silently not created (documented
 * overflow behaviour; see ss_pool.h) — this also acts as a natural cap on bullet density.
 */
class enemy_bullets
{

public:
    void fire(world& w, bullet_kind kind, const bn::fixed_point& position, const bn::fixed_point& velocity);

    /// Bullet towards the player, rotated by angle_offset (binary angle units).
    void fire_aimed(world& w, bullet_kind kind, const bn::fixed_point& position, bn::fixed speed, int angle_offset);

    /// count bullets evenly around a circle, starting at binary angle `phase`.
    void fire_ring(world& w, bullet_kind kind, const bn::fixed_point& position, int count, bn::fixed speed,
                   int phase);

    /// Fan of count bullets centred on the player direction, spread_step apart.
    void fire_fan(world& w, bullet_kind kind, const bn::fixed_point& position, int count, bn::fixed speed,
                  int spread_step);

    void update(world& w);

    /// Turns every bullet into a spark (boss death, player respawn safety).
    void cancel_all(world& w);

    void release(enemy_bullet& bullet)
    {
        _pool.release(bullet);
    }

    pool<enemy_bullet, max_enemy_bullets>& items()
    {
        return _pool;
    }

    [[nodiscard]] int count() const
    {
        return _pool.count();
    }

    [[nodiscard]] int dropped() const
    {
        return _pool.dropped();
    }

private:
    pool<enemy_bullet, max_enemy_bullets> _pool;
};

}

#endif
