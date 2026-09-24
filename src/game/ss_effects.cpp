#include "ss_effects.h"

#include "bn_sprite_items_explosion_big.h"
#include "bn_sprite_items_explosion_small.h"
#include "bn_sprite_items_spark.h"

#include "ss_sprite_util.h"
#include "ss_world.h"

namespace ss
{

namespace
{
    struct effect_info
    {
        const bn::sprite_item* item;
        int frames;
        int frame_period;
    };

    constexpr effect_info effect_infos[] = {
        { &bn::sprite_items::explosion_small, 6, 4 },
        { &bn::sprite_items::explosion_big, 6, 5 },
        { &bn::sprite_items::spark, 3, 3 },
    };
}

void effects::_spawn(world& w, effect_kind kind, const bn::fixed_point& position, const bn::fixed_point& velocity,
                     int delay)
{
    effect* fx = _pool.spawn();

    if(! fx)
    {
        return;
    }

    fx->kind = kind;
    fx->position = position;
    fx->velocity = velocity;
    fx->timer = 0;
    fx->delay = delay;
    fx->frame = 0;

    if(delay == 0)
    {
        fx->sprite = make_sprite(*effect_infos[int(kind)].item, position, 0, z_effects, w.camera);
    }
}

void effects::explosion_small(world& w, const bn::fixed_point& position, int delay)
{
    _spawn(w, effect_kind::EXPLOSION_SMALL, position, bn::fixed_point(-w.scroll_speed / 2, 0), delay);
}

void effects::explosion_big(world& w, const bn::fixed_point& position, int delay)
{
    _spawn(w, effect_kind::EXPLOSION_BIG, position, bn::fixed_point(-w.scroll_speed / 2, 0), delay);

    // A couple of debris sparks flying outwards.
    _spawn(w, effect_kind::SPARK, position, bn::fixed_point(-1, -1), delay + 2);
    _spawn(w, effect_kind::SPARK, position, bn::fixed_point(1, 1), delay + 4);
}

void effects::spark(world& w, const bn::fixed_point& position)
{
    _spawn(w, effect_kind::SPARK, position, bn::fixed_point(0, 0), 0);
}

void effects::update(world& w)
{
    for(effect& fx : _pool)
    {
        if(! fx.active)
        {
            continue;
        }

        const effect_info& info = effect_infos[int(fx.kind)];

        if(fx.delay > 0)
        {
            if(--fx.delay == 0)
            {
                fx.sprite = make_sprite(*info.item, fx.position, 0, z_effects, w.camera);
            }

            continue;
        }

        ++fx.timer;
        int frame = fx.timer / info.frame_period;

        if(frame >= info.frames)
        {
            _pool.release(fx);
            continue;
        }

        fx.position += fx.velocity;
        set_frame(fx.sprite, *info.item, fx.frame, frame);

        if(fx.sprite)
        {
            fx.sprite->set_position(fx.position);
        }
    }
}

}
