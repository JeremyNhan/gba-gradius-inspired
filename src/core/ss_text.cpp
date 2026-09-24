#include "ss_text.h"

#include "bn_sprite_font.h"
#include "bn_string.h"
#include "bn_utf8_characters_map_ref.h"

#include "bn_sprite_items_font.h"
#include "bn_sprite_palette_items_font_cyan.h"
#include "bn_sprite_palette_items_font_red.h"
#include "bn_sprite_palette_items_font_yellow.h"

#include "ss_constants.h"

namespace ss
{

namespace
{
    // Fixed 6 px pitch for every glyph: index 0 is the space character, then '!'..'~' (94 glyphs).
    constexpr int8_t font_widths[] = {
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6
    };

    static_assert(sizeof(font_widths) == 1 + bn::sprite_font::minimum_graphics);

    constexpr bn::sprite_font font(bn::sprite_items::font, bn::utf8_characters_map_ref(), font_widths);
}

text::text() :
    _generator(font)
{
    _generator.set_bg_priority(hud_bg_priority);
    _generator.set_z_order(z_hud);
}

void text::_prepare(text_color color, bn::sprite_text_generator::alignment_type alignment)
{
    switch(color)
    {

    case text_color::YELLOW:
        _generator.set_palette_item(bn::sprite_palette_items::font_yellow);
        break;

    case text_color::CYAN:
        _generator.set_palette_item(bn::sprite_palette_items::font_cyan);
        break;

    case text_color::RED:
        _generator.set_palette_item(bn::sprite_palette_items::font_red);
        break;

    default:
        _generator.set_palette_item(bn::sprite_items::font.palette_item());
        break;
    }

    _generator.set_alignment(alignment);
}

void format_number(int value, int digits, bn::istring& out)
{
    char buffer[12];
    int length = 0;

    if(value < 0)
    {
        value = 0;
    }

    do
    {
        buffer[length++] = char('0' + (value % 10));
        value /= 10;
    }
    while(value > 0 && length < 11);

    for(int index = length; index < digits; ++index)
    {
        out.push_back('0');
    }

    for(int index = length - 1; index >= 0; --index)
    {
        out.push_back(buffer[index]);
    }
}

}
