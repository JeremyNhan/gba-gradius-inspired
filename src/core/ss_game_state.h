#ifndef SS_GAME_STATE_H
#define SS_GAME_STATE_H

namespace ss
{

/// Top-level game states. Transitions are listed in docs/architecture.md and implemented in ss_app.cpp.
enum class game_state : unsigned char
{
    TITLE,
    PLAYING,
    PAUSED,
    STAGE_CLEAR,
    GAME_OVER,
    ENDING
};

}

#endif
