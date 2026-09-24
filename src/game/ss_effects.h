#ifndef SS_EFFECTS_H
#define SS_EFFECTS_H

#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_sprite_ptr.h"

#include "ss_constants.h"
#include "ss_pool.h"

namespace ss
{

class world;

enum class effect_kind : unsigned char
{
    EXPLOSION_SMALL,
    EXPLOSION_BIG,
    SPARK
};

struct effect
{
    bool active = false;
    effect_kind kind = effect_kind::SPARK;
    bn::fixed_point position;
    bn::fixed_point velocity;
    int timer = 0;
    int delay = 0;              // frames before the effect appears (explosion chains)
    int frame = -1;
    bn::optional<bn::sprite_ptr> sprite;

    void clear()
    {
        active = false;
        sprite.reset();
    }
};

/// Purely visual, short-lived sprites. When the pool is full, new effects are skipped.
class effects
{

public:
    void explosion_small(world& w, const bn::fixed_point& position, int delay = 0);

    void explosion_big(world& w, const bn::fixed_point& position, int delay = 0);

    void spark(world& w, const bn::fixed_point& position);

    void update(world& w);

    [[nodiscard]] int count() const
    {
        return _pool.count();
    }

    [[nodiscard]] int dropped() const
    {
        return _pool.dropped();
    }

private:
    pool<effect, max_effects> _pool;

    void _spawn(world& w, effect_kind kind, const bn::fixed_point& position, const bn::fixed_point& velocity,
                int delay);
};

}

#endif
