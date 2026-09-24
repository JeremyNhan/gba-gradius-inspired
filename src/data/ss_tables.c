/* Constant game tables (enemies, guns, power ladder labels). They stay in ROM. */
#include "gen_gfx.h"
#include "ss_enemy_data.h"
#include "ss_power_data.h"
#include "ss_weapon_data.h"

const enemy_def enemy_defs[ENEMY_KIND_COUNT] = {
    /*                     sheet                     frm anim hw  hh  hp  score  move           speed         fire                intv first from big    immune drop% */
    [ENEMY_DART] =         { GEN_SPR_ENEMY_DART,         2, 4,  6,  4,  2,  100, MOVE_STRAIGHT,  FX_F(1.8), FIRE_AIMED,          220,  70, 1, false, false,   8 },
    [ENEMY_WAVER] =        { GEN_SPR_ENEMY_WAVER,        3, 6,  6,  4,  2,  150, MOVE_SINE,      FX_F(1.2), FIRE_STRAIGHT,       200, 110, 0, false, false,   8 },
    [ENEMY_INTERCEPTOR] =  { GEN_SPR_ENEMY_INTERCEPTOR,  2, 3,  6,  3,  3,  250, MOVE_INTERCEPT, FX_F(4.0), FIRE_BURST3,         999,  20, 0, false, false,  12 },
    [ENEMY_TURRET] =       { GEN_SPR_ENEMY_TURRET,       5, 0,  6,  5,  5,  300, MOVE_GROUND,    FX(0),     FIRE_AIMED,          110,  40, 0, false, true,   15 },
    [ENEMY_HULK] =         { GEN_SPR_ENEMY_HULK,         2, 8, 13, 10, 40, 2000, MOVE_HOVER,     FX_F(0.8), FIRE_AIMED_SPREAD3,   80,  50, 0, true,  true,  100 },
    [ENEMY_SWARM] =        { GEN_SPR_ENEMY_SWARM,        4, 4,  5,  5,  1,  100, MOVE_LOOP,      FX_F(2.0), FIRE_NONE,             0,   0, 0, false, false,   5 },
    [ENEMY_MINE] =         { GEN_SPR_ENEMY_MINE,         2, 10, 5,  5,  4,  200, MOVE_DRIFT,     FX_F(0.6), FIRE_RING8,            0,   0, 0, false, false,  10 },
    [ENEMY_ASTEROID_SMALL] = { GEN_SPR_ASTEROID_SMALL,   4, 12, 6,  6,  5,   50, MOVE_TUMBLE,    FX_F(1.0), FIRE_NONE,             0,   0, 0, false, true,    3 },
    [ENEMY_ASTEROID_BIG] = { GEN_SPR_ASTEROID_BIG,       4, 16, 12, 12, 18, 400, MOVE_TUMBLE,    FX_F(0.6), FIRE_NONE,             0,   0, 0, true,  true,   25 },
};

const gun_def gun_defs[3] = {
    /* NORMAL: fast single bolt */
    { "SHOT", SHOT_NORMAL, 7, 2, 1, { { 0, FX(7), 0 } } },
    /* LASER: long bolt that pierces enemies (stopped by the boss and terrain) */
    { "LASER", SHOT_LASER, 9, 2, 1, { { 0, FX(9), 0 } } },
    /* SPREAD LASER: three lasers; the diagonal slope matches the laser sprite frames (tools/assetgen) */
    { "S.LASER", SHOT_LASER, 9, 2, 3, { { 0, FX(9), 0 }, { -3, FX(9), FX_F(-1.7) }, { 3, FX(9), FX_F(1.7) } } },
};

const char* const power_names[MAX_POWER + 1] = {
    "NORMAL SHOT", "HOMING DOT", "MISSILE", "LASER", "SHIELD", "SPREAD LASER", "SHOOTER", "2 SHOOTERS",
    "HOMING MISSILE", "SHOCKWAVE",
};
