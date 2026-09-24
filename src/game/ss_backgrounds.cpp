#include "ss_backgrounds.h"

#include "bn_regular_bg_items_bg_cave.h"
#include "bn_regular_bg_items_bg_hull.h"
#include "bn_regular_bg_items_bg_nebula.h"
#include "bn_regular_bg_items_bg_stars.h"

#include "ss_constants.h"

namespace ss
{

namespace
{
    // A 256x256 BG is centred on screen by Butano; x = 8 aligns map column 0 with the screen's left
    // edge and y = 48 aligns map row 0 with the screen's top edge (the backdrops are drawn for that band).
    constexpr int bg_left_x = 8;
    constexpr int bg_top_y = 48;

    bn::optional<bn::regular_bg_ptr> create_backdrop(backdrop_type backdrop)
    {
        switch(backdrop)
        {

        case backdrop_type::NEBULA:
            return bn::regular_bg_items::bg_nebula.create_bg(bg_left_x, 0);

        case backdrop_type::CAVE:
            return bn::regular_bg_items::bg_cave.create_bg(bg_left_x, bg_top_y);

        case backdrop_type::HULL:
            return bn::regular_bg_items::bg_hull.create_bg(bg_left_x, bg_top_y);

        default:
            return bn::optional<bn::regular_bg_ptr>();
        }
    }
}

backgrounds::backgrounds(backdrop_type backdrop, const bn::optional<bn::camera_ptr>& camera) :
    _stars(bn::regular_bg_items::bg_stars.create_bg(bg_left_x, 0)),
    _backdrop(create_backdrop(backdrop))
{
    _stars.set_priority(bg_priority_stars);

    if(_backdrop)
    {
        _backdrop->set_priority(bg_priority_backdrop);
    }

    if(camera)
    {
        _stars.set_camera(*camera);

        if(_backdrop)
        {
            _backdrop->set_camera(*camera);
        }
    }
}

void backgrounds::update(bn::fixed scroll_x)
{
    _stars.set_x(bg_left_x - scroll_x / 4);

    if(_backdrop)
    {
        _backdrop->set_x(bg_left_x - scroll_x / 2);
    }
}

}
