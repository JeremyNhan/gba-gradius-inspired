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

struct powerup
{
    bool active = false;
    bn::fixed_point position;
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
        return make_hitbox(position, 6, 6);
    }
};

class powerups
{

public:
    /// Drops a power capsule (enemies roll their drop chance before calling this).
    void drop(world& w, const bn::fixed_point& position);

    void update(world& w);

    /// Advances the player one step on the power ladder (at the top: shield refill and bonus score).
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
};

}

#endif
