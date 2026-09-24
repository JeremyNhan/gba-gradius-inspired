#include "ss_title_screen.h"

#include "bn_keypad.h"
#include "bn_string.h"

#include "bn_regular_bg_items_bg_stars.h"
#include "bn_regular_bg_items_bg_title_logo.h"
#include "bn_sprite_items_player.h"

#include "ss_audio.h"
#include "ss_constants.h"
#include "ss_math.h"
#include "ss_text.h"

namespace ss
{

title_screen::title_screen(text& text_generator, int hiscore) :
    _text(text_generator),
    _stars(bn::regular_bg_items::bg_stars.create_bg(8, 0)),
    _logo(bn::regular_bg_items::bg_title_logo.create_bg(0, 0))
{
    _stars.set_priority(bg_priority_stars);
    _logo.set_priority(bg_priority_logo);

    _text.centered(28, "PRESS START", _press_start, text_color::YELLOW);

    bn::string<24> high("HI-SCORE ");
    format_number(hiscore, 8, high);
    _text.centered(46, high, _info);
    _text.centered(58, "A:SHOT  B:CHARGE  START:PAUSE", _info, text_color::CYAN);
    _text.centered(70, "(C) 2026  ORIGINAL GBA HOMEBREW", _info);

    // A small ship cruising under the logo.
    _ship = bn::sprite_items::player.create_sprite(-140, 12);
    _ship->set_bg_priority(sprite_bg_priority);

    if(SS_DEBUG)
    {
        _draw_stage_select();
    }

    audio::play_music(audio::music::TITLE);
}

void title_screen::_draw_stage_select()
{
    _stage_select.clear();
    bn::string<16> label("DEBUG STAGE ");
    format_number(_stage + 1, 1, label);
    _text.left(-118, -78, label, _stage_select, text_color::RED);
}

bool title_screen::update()
{
    ++_timer;
    _stars.set_x(8 - bn::fixed(_timer) / 4);

    bool show = (_timer / 30) % 2 == 0;

    for(bn::sprite_ptr& sprite : _press_start)
    {
        sprite.set_visible(show);
    }

    if(_ship)
    {
        bn::fixed x = _ship->x() + bn::fixed(0.75);

        if(x > 140)
        {
            x = -140;
        }

        _ship->set_position(x, 12 + direction(_timer * 300, 3).y());
        _ship->set_tiles(bn::sprite_items::player.tiles_item(), (_timer >> 2) & 1);
    }

    if(SS_DEBUG)
    {
        if(bn::keypad::r_pressed())
        {
            _stage = (_stage + 1) % stage_count;
            _draw_stage_select();
        }
        else if(bn::keypad::l_pressed())
        {
            _stage = (_stage + stage_count - 1) % stage_count;
            _draw_stage_select();
        }
    }

    if(bn::keypad::start_pressed() && _timer > 20)
    {
        audio::play(audio::sfx::SELECT);
        return true;
    }

    return false;
}

}
