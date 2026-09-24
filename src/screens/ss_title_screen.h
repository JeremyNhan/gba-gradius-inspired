#ifndef SS_TITLE_SCREEN_H
#define SS_TITLE_SCREEN_H

#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

namespace ss
{

class text;

class title_screen
{

public:
    title_screen(text& text_generator, int hiscore);

    /// Returns true on the frame START is pressed.
    [[nodiscard]] bool update();

    /// Debug builds: stage chosen with L/R on the title screen (0-based).
    [[nodiscard]] int selected_stage() const
    {
        return _stage;
    }

private:
    text& _text;
    bn::regular_bg_ptr _stars;
    bn::regular_bg_ptr _logo;
    bn::optional<bn::sprite_ptr> _ship;
    bn::vector<bn::sprite_ptr, 8> _press_start;
    bn::vector<bn::sprite_ptr, 24> _info;
    bn::vector<bn::sprite_ptr, 6> _stage_select;
    int _timer = 0;
    int _stage = 0;

    void _draw_stage_select();
};

}

#endif
