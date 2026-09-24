#ifndef SS_ENDING_SCREEN_H
#define SS_ENDING_SCREEN_H

#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

namespace ss
{

class text;
struct session;

/// Ending: story epilogue, credits and final score, shown as timed pages over a starfield.
class ending_screen
{

public:
    ending_screen(text& text_generator, const session& game);

    /// Returns true when the player leaves the ending (START on the last page).
    [[nodiscard]] bool update();

private:
    text& _text;
    const session& _game;
    bn::regular_bg_ptr _stars;
    bn::vector<bn::sprite_ptr, 40> _lines;
    int _page = -1;
    int _timer = 0;

    void _show_page(int page);
};

}

#endif
