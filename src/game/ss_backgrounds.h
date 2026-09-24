#ifndef SS_BACKGROUNDS_H
#define SS_BACKGROUNDS_H

#include "bn_camera_ptr.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"

namespace ss
{

enum class backdrop_type : unsigned char
{
    NONE,
    NEBULA,
    CAVE,
    HULL
};

/**
 * Parallax layers (mode 0 regular BGs, hardware scrolled):
 *   BG priority 3: far starfield, scrolls at 1/4 of the camera speed
 *   BG priority 2: stage backdrop, scrolls at 1/2 of the camera speed
 * The terrain layer (priority 1, 1:1 speed) lives in ss_terrain.
 */
class backgrounds
{

public:
    backgrounds(backdrop_type backdrop, const bn::optional<bn::camera_ptr>& camera);

    /// scroll_x: world camera position in pixels (monotonically increasing).
    void update(bn::fixed scroll_x);

private:
    bn::regular_bg_ptr _stars;
    bn::optional<bn::regular_bg_ptr> _backdrop;
};

}

#endif
