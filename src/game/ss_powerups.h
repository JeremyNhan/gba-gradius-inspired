#ifndef SS_POWERUPS_H
#define SS_POWERUPS_H

#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_sprite_ptr.h"

#include "ss_collision.h"
#include "ss_constants.h"
#include "ss_pool.h"

namespace ss
{

class world;

/// Order matches the frames of the powerup sprite sheet (tools/sprites.py: powerups()).
enum class powerup_type : unsigned char
{
    SPEED,
    SHOT,
    SPREAD,
    MISSILE,
    SHIELD,
    LIFE
};

struct powerup
{
    bool active = false;
    powerup_type type = powerup_type::SHOT;
    bn::fixed_point position;
    int timer = 0;
    bn::optional<bn::sprite_ptr> sprite;

    void clear()
    {
        active = false;
        sprite.reset();
    }

    [[nodiscard]] hitbox box() const
    {
        return make_hitbox(position, 6, 6);
    }
};

class powerups
{

public:
    /// Drops the next capsule of the deterministic rotation (skipping upgrades already maxed).
    void drop(world& w, const bn::fixed_point& position);

    void update(world& w);

    /// Applies the capsule's effect to the player's loadout.
    void collect(world& w, powerup& item);

    pool<powerup, max_powerups>& items()
    {
        return _pool;
    }

    [[nodiscard]] int count() const
    {
        return _pool.count();
    }

private:
    pool<powerup, max_powerups> _pool;
    int _rotation = 0;

    [[nodiscard]] bool _useful(const world& w, powerup_type type) const;
};

}

#endif
