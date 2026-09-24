#ifndef SS_SPRITE_UTIL_H
#define SS_SPRITE_UTIL_H

#include "bn_camera_ptr.h"
#include "bn_optional.h"
#include "bn_sprite_item.h"
#include "bn_sprite_ptr.h"

#include "ss_constants.h"

namespace ss
{

/**
 * Creates a gameplay sprite attached to the world camera (for screen shake).
 * Uses create_optional: if the hardware sprite table is exhausted the result is empty and the
 * entity simply stays invisible for that time instead of asserting.
 */
[[nodiscard]] inline bn::optional<bn::sprite_ptr> make_sprite(
        const bn::sprite_item& item, const bn::fixed_point& position, int graphics_index, int z_order,
        const bn::optional<bn::camera_ptr>& camera)
{
    bn::optional<bn::sprite_ptr> result = item.create_sprite_optional(position, graphics_index);

    if(result)
    {
        bn::sprite_ptr& sprite = *result;
        sprite.set_bg_priority(sprite_bg_priority);
        sprite.set_z_order(z_order);

        if(camera)
        {
            sprite.set_camera(*camera);
        }
    }

    return result;
}

/// Changes the animation frame only when it differs (set_tiles has a lookup cost).
inline void set_frame(bn::optional<bn::sprite_ptr>& sprite, const bn::sprite_item& item, int& current, int frame)
{
    if(sprite && frame != current)
    {
        sprite->set_tiles(item.tiles_item(), frame);
        current = frame;
    }
}

}

#endif
