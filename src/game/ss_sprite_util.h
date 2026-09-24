#ifndef SS_SPRITE_UTIL_H
#define SS_SPRITE_UTIL_H

#include "bn_camera_ptr.h"
#include "bn_optional.h"
#include "bn_sprite_builder.h"
#include "bn_sprite_item.h"
#include "bn_sprite_ptr.h"

#include "ss_constants.h"

namespace ss
{

/**
 * Creates a gameplay sprite attached to the world camera (for screen shake).
 * A sprite_builder sets priority, z order and camera before the sprite is inserted, so Butano sorts
 * it once (setting them on a created sprite re-sorts it for every setter, which made creation about
 * four times more expensive).
 * Uses build_optional: if the hardware sprite table is exhausted the result is empty and the
 * entity simply stays invisible for that time instead of asserting.
 */
[[nodiscard]] inline bn::optional<bn::sprite_ptr> make_sprite(
        const bn::sprite_item& item, const bn::fixed_point& position, int graphics_index, int z_order,
        const bn::optional<bn::camera_ptr>& camera)
{
    bn::sprite_builder builder(item, graphics_index);
    builder.set_position(position);
    builder.set_bg_priority(sprite_bg_priority);
    builder.set_z_order(z_order);
    builder.set_camera(camera);
    return builder.release_build_optional();
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
