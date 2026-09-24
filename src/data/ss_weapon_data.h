#ifndef SS_WEAPON_DATA_H
#define SS_WEAPON_DATA_H

#include "bn_fixed.h"

#include "ss_session.h"

namespace ss
{

enum class shot_kind : unsigned char
{
    NORMAL,
    SPREAD,
    MISSILE,
    BEAM
};

/// One projectile of a volley: vertical offset from the ship's nose and velocity (pixels/frame).
struct shot_spec
{
    signed char dy;
    bn::fixed vx;
    bn::fixed vy;
};

struct weapon_level_def
{
    int fire_interval;          // frames between volleys while A is held
    int damage;                 // per projectile
    int count;                  // projectiles per volley
    shot_spec shots[5];
};

struct weapon_def
{
    const char* hud_name;
    shot_kind kind;
    weapon_level_def levels[max_weapon_level];
};

// Primary weapons, indexed by weapon_type. Data-driven: the player code only reads these tables.
constexpr weapon_def weapon_defs[] = {
    {   // NORMAL: fast, focused forward stream
        "SHOT", shot_kind::NORMAL, {
            { 7, 2, 1, { { 0, 7, 0 } } },
            { 7, 2, 2, { { -3, 7, 0 }, { 3, 7, 0 } } },
            { 6, 2, 3, { { -5, 7, 0 }, { 0, 7, 0 }, { 5, 7, 0 } } }
        }
    },
    {   // SPREAD: wide fan, weaker pellets, slower
        "WIDE", shot_kind::SPREAD, {
            { 11, 1, 3, { { 0, 5, 0 }, { -2, bn::fixed(4.8), bn::fixed(-1.1) }, { 2, bn::fixed(4.8), bn::fixed(1.1) } } },
            { 11, 1, 5, { { 0, 5, 0 }, { -2, bn::fixed(4.8), bn::fixed(-1.1) }, { 2, bn::fixed(4.8), bn::fixed(1.1) },
                          { -3, bn::fixed(4.2), bn::fixed(-2.2) }, { 3, bn::fixed(4.2), bn::fixed(2.2) } } },
            { 9, 2, 5, { { 0, 5, 0 }, { -2, bn::fixed(4.8), bn::fixed(-1.1) }, { 2, bn::fixed(4.8), bn::fixed(1.1) },
                         { -3, bn::fixed(4.2), bn::fixed(-2.2) }, { 3, bn::fixed(4.2), bn::fixed(2.2) } } }
        }
    }
};

// Secondary weapon: homing missiles, fired automatically with A once collected.
constexpr int missile_interval = 34;
constexpr int missile_damage = 3;
constexpr bn::fixed missile_speed = 3;
constexpr int missile_turn_rate = 900;     // binary angle units per frame (65536 = full turn)

// Charged shot (hold B): piercing wave.
constexpr int beam_damage = 12;
constexpr bn::fixed beam_speed = 5;

// Movement speed per speed level (pixels/frame).
constexpr bn::fixed speed_by_level[max_speed_level] = { bn::fixed(1.5), bn::fixed(2.0), bn::fixed(2.5) };

}

#endif
