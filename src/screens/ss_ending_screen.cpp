#include "ss_ending_screen.h"

#include "bn_bg_palettes.h"
#include "bn_keypad.h"
#include "bn_sprite_palettes.h"
#include "bn_string.h"

#include "bn_regular_bg_items_bg_stars.h"

#include "ss_audio.h"
#include "ss_constants.h"
#include "ss_session.h"
#include "ss_text.h"

namespace ss
{

namespace
{
    constexpr int page_frames = 420;
    constexpr int fade_frames = 30;
    constexpr int page_count = 3;
}

ending_screen::ending_screen(text& text_generator, const session& game) :
    _text(text_generator),
    _game(game),
    _stars(bn::regular_bg_items::bg_stars.create_bg(8, 0))
{
    _stars.set_priority(bg_priority_stars);
    audio::play_music(audio::music::ENDING);
    _show_page(0);
}

void ending_screen::_show_page(int page)
{
    _page = page;
    _timer = 0;
    _lines.clear();

    switch(page)
    {

    case 0:
        _text.centered(-48, "THE OVERMIND IS DESTROYED.", _lines);
        _text.centered(-32, "THE DREADNOUGHT BREAKS APART", _lines);
        _text.centered(-20, "AND FALLS INTO THE RED STAR.", _lines);
        _text.centered(0, "THE OUTER COLONIES ARE SAFE.", _lines, text_color::CYAN);
        _text.centered(20, "...FOR NOW.", _lines, text_color::YELLOW);
        break;

    case 1:
        _text.centered(-56, "SPACE SHOOTER", _lines, text_color::YELLOW);
        _text.centered(-36, "GAME DESIGN, CODE, PIXEL ART", _lines);
        _text.centered(-24, "MUSIC AND SOUND EFFECTS", _lines);
        _text.centered(-12, "ALL GENERATED FROM SOURCE", _lines);
        _text.centered(12, "MADE WITH BUTANO + DEVKITARM", _lines, text_color::CYAN);
        _text.centered(24, "TESTED IN MGBA", _lines, text_color::CYAN);
        break;

    default:
        {
            _text.centered(-50, "MISSION COMPLETE", _lines, text_color::YELLOW);
            bn::string<24> score("FINAL SCORE ");
            format_number(_game.score, 8, score);
            _text.centered(-28, score, _lines);
            bn::string<24> high("HI-SCORE    ");
            format_number(_game.hiscore, 8, high);
            _text.centered(-16, high, _lines);
            bn::string<24> deaths("SHIPS LOST  ");
            format_number(_game.deaths, 1, deaths);
            _text.centered(-4, deaths, _lines);
            _text.centered(20, "THANK YOU FOR PLAYING!", _lines, text_color::CYAN);
            _text.centered(40, "PRESS START", _lines, text_color::YELLOW);
        }
        break;
    }
}

bool ending_screen::update()
{
    ++_timer;
    _stars.set_x(8 - bn::fixed(_timer + _page * page_frames) / 3);

    // Fade each page in and out with the global palette fade (last page stays on).
    bn::fixed fade = 0;

    if(_timer < fade_frames)
    {
        fade = bn::fixed(fade_frames - _timer) / fade_frames;
    }
    else if(_page < page_count - 1 && _timer > page_frames - fade_frames)
    {
        fade = bn::fixed(_timer - (page_frames - fade_frames)) / fade_frames;
    }

    bn::sprite_palettes::set_fade(bn::color(0, 0, 0), fade);

    if(_page < page_count - 1)
    {
        if(_timer >= page_frames || (bn::keypad::start_pressed() && _timer > fade_frames))
        {
            _show_page(_page + 1);
        }

        return false;
    }

    if(bn::keypad::start_pressed() && _timer > 60)
    {
        bn::sprite_palettes::set_fade(bn::color(0, 0, 0), 0);
        return true;
    }

    return false;
}

}
