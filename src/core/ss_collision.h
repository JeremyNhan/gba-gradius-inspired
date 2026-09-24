#ifndef SS_COLLISION_H
#define SS_COLLISION_H

#include "bn_fixed_point.h"

namespace ss
{

/**
 * Centre-based axis-aligned box. All game collisions are AABB tests: cheap on an ARM7 without an
 * FPU and fully deterministic (fixed-point, no floating point).
 */
struct hitbox
{
    bn::fixed_point center;
    bn::fixed half_w;
    bn::fixed half_h;

    [[nodiscard]] constexpr bool intersects(const hitbox& other) const
    {
        bn::fixed dx = center.x() - other.center.x();
        bn::fixed dy = center.y() - other.center.y();

        if(dx < 0)
        {
            dx = -dx;
        }

        if(dy < 0)
        {
            dy = -dy;
        }

        return dx < half_w + other.half_w && dy < half_h + other.half_h;
    }
};

[[nodiscard]] constexpr hitbox make_hitbox(const bn::fixed_point& center, int half_w, int half_h)
{
    return hitbox{ center, half_w, half_h };
}

/// True when a point is outside the screen by more than margin pixels (used to despawn).
[[nodiscard]] constexpr bool off_screen(const bn::fixed_point& p, int margin)
{
    return p.x() < -120 - margin || p.x() > 120 + margin || p.y() < -80 - margin || p.y() > 80 + margin;
}

}

#endif
