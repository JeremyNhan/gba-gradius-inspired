#ifndef SS_TEXT_H
#define SS_TEXT_H

#include "bn_sprite_text_generator.h"
#include "bn_string_view.h"
#include "bn_vector.h"

namespace ss
{

enum class text_color : unsigned char
{
    WHITE,
    YELLOW,
    CYAN,
    RED
};

/**
 * Owns the sprite text generator (8x8 original font, 6 px pitch) and offers small helpers.
 * Text is built from sprites; callers keep the generated sprites in a bn::vector and clear it to
 * erase the text.
 */
class text
{

public:
    text();

    /// Horizontally centred text; y is the top edge in Butano coordinates.
    template<int MaxSprites>
    void centered(int y, const bn::string_view& str, bn::vector<bn::sprite_ptr, MaxSprites>& out,
                  text_color color = text_color::WHITE)
    {
        _prepare(color, bn::sprite_text_generator::alignment_type::CENTER);
        _generator.generate(0, y + 4, str, out);
    }

    template<int MaxSprites>
    void left(int x, int y, const bn::string_view& str, bn::vector<bn::sprite_ptr, MaxSprites>& out,
              text_color color = text_color::WHITE)
    {
        _prepare(color, bn::sprite_text_generator::alignment_type::LEFT);
        _generator.generate(x, y + 4, str, out);
    }

    template<int MaxSprites>
    void right(int x, int y, const bn::string_view& str, bn::vector<bn::sprite_ptr, MaxSprites>& out,
               text_color color = text_color::WHITE)
    {
        _prepare(color, bn::sprite_text_generator::alignment_type::RIGHT);
        _generator.generate(x, y + 4, str, out);
    }

    [[nodiscard]] int width(const bn::string_view& str) const
    {
        return _generator.width(str);
    }

    static constexpr int char_width = 6;

private:
    bn::sprite_text_generator _generator;

    void _prepare(text_color color, bn::sprite_text_generator::alignment_type alignment);
};

/// Formats an unsigned number with leading zeros ("0001230").
void format_number(int value, int digits, bn::istring& out);

}

#endif
