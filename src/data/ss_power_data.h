/*
 * The power ladder. Every power capsule advances the player one step; powers are cumulative by slot
 * (the main gun is upgraded in place, missiles become homing, the rest is added on top). Losing a
 * ship resets the ladder to POWER_NORMAL.
 */
#ifndef SS_POWER_DATA_H
#define SS_POWER_DATA_H

#include "ss_base.h"

typedef enum
{
    POWER_NORMAL,           /* start: single forward shot */
    POWER_HOMING_DOT,       /* + small homing dot fired with the main gun */
    POWER_MISSILE,          /* + forward missiles */
    POWER_LASER,            /* main gun -> piercing laser */
    POWER_SHIELD,           /* + 3-hit shield (granted once when the step is reached) */
    POWER_SPREAD_LASER,     /* main gun -> three-way laser */
    POWER_SHOOTER_1,        /* + one additional shooter (trailing drone) */
    POWER_SHOOTER_2,        /* + a second additional shooter */
    POWER_HOMING_MISSILE,   /* missiles fire in pairs and home in */
    POWER_SHOCKWAVE         /* + periodic shockwave: clears enemies and bullets, 0.5 s invulnerability */
} power_step;

#define MAX_POWER POWER_SHOCKWAVE

typedef enum
{
    GUN_NORMAL,
    GUN_LASER,
    GUN_SPREAD_LASER
} main_gun;

typedef enum
{
    MISSILES_NONE,
    MISSILES_FORWARD,
    MISSILES_HOMING
} missile_mode;

static inline bool has_power(int power, power_step step)
{
    return power >= (int) step;
}

static inline main_gun main_gun_of(int power)
{
    return has_power(power, POWER_SPREAD_LASER) ? GUN_SPREAD_LASER :
           has_power(power, POWER_LASER) ? GUN_LASER : GUN_NORMAL;
}

static inline missile_mode missile_mode_of(int power)
{
    return has_power(power, POWER_HOMING_MISSILE) ? MISSILES_HOMING :
           has_power(power, POWER_MISSILE) ? MISSILES_FORWARD : MISSILES_NONE;
}

static inline int shooter_count_of(int power)
{
    return has_power(power, POWER_SHOOTER_2) ? 2 : has_power(power, POWER_SHOOTER_1) ? 1 : 0;
}

/* Label shown when a step is reached (index = new power value). */
extern const char* const power_names[MAX_POWER + 1];

#endif
