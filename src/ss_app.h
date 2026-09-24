#ifndef SS_APP_H
#define SS_APP_H

#include "bn_optional.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "ss_ending_screen.h"
#include "ss_game_state.h"
#include "ss_session.h"
#include "ss_text.h"
#include "ss_title_screen.h"
#include "ss_world.h"

namespace ss
{

/**
 * Top-level state machine (see docs/architecture.md, section 3).
 *
 * Each state has one update handler returning the next state; _enter() performs the transition
 * side effects (creating/destroying screens and the world, music, overlays). Only one heavy
 * screen object exists at a time, so VRAM/OAM are never shared between screens.
 */
class app
{

public:
    app();

    void update();

private:
    game_state _state = game_state::TITLE;
    text _text;
    session _session;
    int _saved_hiscore = 0;
    bn::optional<title_screen> _title;
    bn::optional<world> _world;
    bn::optional<ending_screen> _ending;
    bn::vector<bn::sprite_ptr, 24> _overlay;
    int _timer = 0;
    bool _final_clear = false;

    [[nodiscard]] game_state _update_title();
    [[nodiscard]] game_state _update_playing();
    [[nodiscard]] game_state _update_paused();
    [[nodiscard]] game_state _update_stage_clear();
    [[nodiscard]] game_state _update_game_over();
    [[nodiscard]] game_state _update_ending();

    void _enter(game_state next);
    void _save_hiscore();
};

}

#endif
