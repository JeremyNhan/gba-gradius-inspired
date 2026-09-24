#ifndef SS_WEAPON_DATA_H
#define SS_WEAPON_DATA_H

#include "bn_fixed.h"

namespace ss
{

enum class shot_kind : unsigned char
{
    NORMAL,
    LASER,
    DOT,            // homing dot
    MISSILE,
    BEAM            // charged shot
};

/// One projectile of a volley: vertical offset from the gun and velocity (pixels/frame).
struct shot_spec
{
    signed char dy;
    bn::fixed vx;
    bn::fixed vy;
};

struct gun_def
{
    const char* hud_name;
    shot_kind kind;
    int fire_interval;          // frames between volleys while A is held
    int damage;                 // per projectile (lasers hit every enemy they pass through)
    int count;                  // projectiles per volley
    shot_spec shots[3];
};

// Main guns, indexed by main_gun (ss_power_data.h). Data-driven: the player code only reads these tables.
// Additional shooters fire the same volley from their own position.
constexpr gun_def gun_defs[] = {
    {   // NORMAL: fast single bolt
        "SHOT", shot_kind::NORMAL, 7, 2, 1, { { 0, 7, 0 } }
    },
    {   // LASER: long bolt that pierces enemies (stopped by the boss and terrain)
        "LASER", shot_kind::LASER, 9, 2, 1, { { 0, 9, 0 } }
    },
    {   // SPREAD LASER: three lasers; the diagonal slope matches the laser sprite frames (tools/sprites.py)
        "S.LASER", shot_kind::LASER, 9, 2, 3, { { 0, 9, 0 }, { -3, 9, bn::fixed(-1.7) }, { 3, 9, bn::fixed(1.7) } }
    }
};

// Homing dot: small pellet that steers toward the nearest target.
constexpr int dot_interval = 20;
constexpr int dot_damage = 1;
constexpr bn::fixed dot_speed = 4;
constexpr int dot_turn_rate = 1400;        // binary angle units per frame (65536 = full turn)
constexpr int max_dots = 2;

// Missiles: forward (one at a time, angled down-forward) or homing (pairs, up and down).
constexpr int missile_interval = 34;
constexpr int missile_damage = 3;
constexpr bn::fixed missile_speed = 3;
constexpr int missile_turn_rate = 900;
constexpr int max_forward_missiles = 2;
constexpr int max_homing_missiles = 4;

// Charged shot (hold B): piercing wave.
constexpr int beam_damage = 12;
constexpr bn::fixed beam_speed = 5;

// Player movement speed (pixels/frame).
constexpr bn::fixed player_speed = 2;

// Shockwave (top of the power ladder): fires automatically every shockwave_interval frames.
constexpr int shockwave_interval = 600;
constexpr int shockwave_invulnerable_frames = 30;
constexpr int shockwave_boss_damage_divisor = 10;   // boss loses hp_max / 10 per shockwave

}

#endif
