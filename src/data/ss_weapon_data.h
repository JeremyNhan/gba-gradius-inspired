#ifndef SS_WEAPON_DATA_H
#define SS_WEAPON_DATA_H

#include "ss_base.h"

typedef enum
{
    SHOT_NORMAL,
    SHOT_LASER,
    SHOT_DOT,               /* homing dot */
    SHOT_MISSILE,
    SHOT_BEAM               /* charged shot */
} shot_kind;

/* One projectile of a volley: vertical offset from the gun and velocity (pixels/frame). */
typedef struct
{
    s8 dy;
    fx vx;
    fx vy;
} shot_spec;

typedef struct
{
    const char* hud_name;
    u8 kind;                /* shot_kind */
    u8 fire_interval;       /* frames between volleys while A is held */
    u8 damage;              /* per projectile (lasers hit every enemy they pass through) */
    u8 count;               /* projectiles per volley */
    shot_spec shots[3];
} gun_def;

/* Main guns, indexed by main_gun (ss_power_data.h). Additional shooters fire the same volley. */
extern const gun_def gun_defs[3];

/* Homing dot: small pellet that steers toward the nearest target. */
#define DOT_INTERVAL 20
#define DOT_DAMAGE 1
#define DOT_SPEED FX(4)
#define DOT_TURN_RATE 1400          /* binary angle units per frame */
#define MAX_DOTS 2

/* Missiles: forward (one at a time, angled down-forward) or homing (pairs, up and down). */
#define MISSILE_INTERVAL 34
#define MISSILE_DAMAGE 3
#define MISSILE_SPEED FX(3)
#define MISSILE_TURN_RATE 900
#define MAX_FORWARD_MISSILES 2
#define MAX_HOMING_MISSILES 4

/* Charged shot (hold B): piercing wave. */
#define BEAM_DAMAGE 12
#define BEAM_SPEED FX(5)

/* Player movement speed (pixels/frame). */
#define PLAYER_SPEED FX(2)

/* Shockwave (top of the power ladder): fires automatically every SHOCKWAVE_INTERVAL frames. */
#define SHOCKWAVE_INTERVAL 600
#define SHOCKWAVE_INVULNERABLE_FRAMES 30
#define SHOCKWAVE_BOSS_DAMAGE_DIVISOR 10    /* boss loses hp_max / 10 per shockwave */

#endif
