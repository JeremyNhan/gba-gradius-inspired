#ifndef SS_HUD_H
#define SS_HUD_H

#include "bn_optional.h"
#include "bn_sprite_ptr.h"
#include "bn_string_view.h"
#include "bn_vector.h"

#include "ss_powerups.h"

namespace ss
{

class world;
class text;

/**
 * Heads-up display built from text sprites. Text is regenerated only when the displayed values
 * change (sprite text generation is comparatively expensive).
 *
 *   top line:    SC 00001230     HI 00020000          [ship] x3
 *   under it:    boss HP bar (boss fights only)
 *   bottom line: SHOT LV2  MSL 1  SPD 2  SHLD 3
 */
class hud
{

public:
    explicit hud(text& text_generator);

    void update(world& w);

    void show_banner(const char* title, const char* subtitle);

    void show_warning();

    /// Shows a short label under the ship area. `label` must be a string literal / static string:
    /// it is rendered on the next HUD update, not immediately.
    void show_pickup(const char* label);

    void set_visible(bool visible);

    void toggle_debug()
    {
        _debug = ! _debug;
        _debug_sprites.clear();
    }

private:
    text& _text;
    bn::vector<bn::sprite_ptr, 8> _score_sprites;
    bn::vector<bn::sprite_ptr, 10> _status_sprites;
    bn::vector<bn::sprite_ptr, 3> _lives_sprites;
    bn::vector<bn::sprite_ptr, 10> _banner_sprites;
    bn::vector<bn::sprite_ptr, 5> _pickup_sprites;
    bn::vector<bn::sprite_ptr, 10> _debug_sprites;
    bn::vector<bn::sprite_ptr, 5> _bar_sprites;
    bn::optional<bn::sprite_ptr> _life_icon;
    int _bar_frames[4] = { -1, -1, -1, -1 };
    int _shown_score = -1;
    int _shown_hiscore = -1;
    int _shown_lives = -1;
    int _shown_status = -1;
    int _banner_timer = 0;
    int _warning_timer = 0;
    int _pickup_timer = 0;
    const char* _pending_pickup = nullptr;
    bool _debug = false;
    bool _visible = true;

    bool _update_score(world& w);
    bool _update_status(world& w);
    void _update_boss_bar(world& w);
    void _update_debug(world& w);
};

}

#endif
