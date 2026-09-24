#ifndef SS_MATH_H
#define SS_MATH_H

#include "bn_fixed_point.h"
#include "bn_math.h"

namespace ss
{

/**
 * Angles are "binary angles": 65536 units per full turn, 0 = pointing right, increasing clockwise
 * on screen (because screen y grows downwards). They map directly onto Butano's fixed_t<16>
 * turn-based bn::sin / bn::cos / bn::atan2, so no floating point is involved.
 */
constexpr int angle_turn = 65536;
constexpr int angle_left = 32768;

[[nodiscard]] inline bn::fixed_point direction(int angle, bn::fixed speed)
{
    bn::fixed_t<16> turns = bn::fixed_t<16>::from_data(angle & 0xFFFF);
    return bn::fixed_point(bn::cos(turns) * speed, bn::sin(turns) * speed);
}

[[nodiscard]] inline int angle_to(const bn::fixed_point& from, const bn::fixed_point& to)
{
    int dx = (to.x() - from.x()).round_integer();
    int dy = (to.y() - from.y()).round_integer();

    if(dx == 0 && dy == 0)
    {
        return angle_left;
    }

    return bn::atan2(dy, dx).data() & 0xFFFF;
}

/// Signed smallest difference between two binary angles, in [-32768, 32767].
[[nodiscard]] constexpr int angle_delta(int from, int to)
{
    int delta = (to - from) & 0xFFFF;
    return delta >= 32768 ? delta - 65536 : delta;
}

/// Degrees (integer) to binary angle.
[[nodiscard]] constexpr int degrees(int value)
{
    return (value * angle_turn) / 360;
}

}

#endif
