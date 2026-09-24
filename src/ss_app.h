/*
 * Top-level state machine (docs/architecture.md, section 3). Each state has one update handler
 * returning the next state; app_enter() performs the transition side effects.
 */
#ifndef SS_APP_H
#define SS_APP_H

#include "ss_base.h"

typedef enum
{
    STATE_TITLE,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_STAGE_CLEAR,
    STATE_GAME_OVER,
    STATE_ENDING
} game_state;

void app_init(void);
void app_update(void);
game_state app_state(void);

#if SS_TESTS
/* Test driver: jump straight into a new game at a stage (skips the title). */
void app_start_game(int stage);
void app_go_to_title(void);
#endif

#endif
