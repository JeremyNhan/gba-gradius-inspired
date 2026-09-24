/*
 * Everything that exists while a stage is being played. There is exactly one world at a time
 * (static state, no heap): world_init() resets every entity system for game.stage.
 */
#ifndef SS_WORLD_H
#define SS_WORLD_H

#include "ss_base.h"
#include "ss_rng.h"
#include "ss_stage_data.h"

/* ----- the play-through (title -> game over / ending) --------------------------------------------- */

typedef struct
{
    int power;              /* step on the power ladder, 0..MAX_POWER (ss_power_data.h) */
    int shield;             /* hits absorbed, 0..MAX_SHIELD */
} loadout;

typedef struct
{
    int stage;              /* 0-based */
    int lives;
    int score;
    int hiscore;
    int deaths;
    loadout gear;
} session;

extern session game;

void session_new_game(int stage);
void game_add_score(int points);

/* ----- the current stage -------------------------------------------------------------------------- */

typedef enum
{
    WORLD_RUNNING,
    WORLD_STAGE_CLEARED,
    WORLD_GAME_OVER,
    WORLD_GAME_COMPLETE
} world_result;

typedef struct
{
    const stage_def* stage;
    rng random;
    fx scroll_x;
    fx scroll_speed;
    int stage_frame;
    int shake_frames;
    int shake_amplitude;
    int game_over_timer;
    int clear_timer;
    int flash_frames;
    int shockwaves;         /* fired this stage */
    bool debug_overlay;
} world_state;

extern world_state world;

void world_init(void);
world_result world_update(void);
bool world_update_outro(void);      /* stage clear: ship flies off; true when done */
void world_render(void);
void world_release(void);           /* leaving gameplay: hide the stage layers */

void world_shake(int frames, int amplitude);
void world_set_power(int power);
void world_shockwave(void);
void world_notify_boss_defeated(void);
void world_notify_player_out_of_lives(void);

bool world_fire_held(void);
bool world_charge_held(void);
bool world_invincible(void);

static inline int world_difficulty(void)
{
    return game.stage;
}

#if SS_TEST_HOOKS
/* Written by the test driver (and debug keys); ignored in release builds. */
typedef struct
{
    bool invincible;
    bool autofire;
    bool skip_to_boss;
    int set_power;          /* > 0: set the power ladder to step set_power - 1 */
} test_controls;

extern test_controls test_ctl;
#endif

#endif
