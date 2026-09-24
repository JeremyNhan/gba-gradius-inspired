#ifndef SS_POWER_DATA_H
#define SS_POWER_DATA_H

namespace ss
{

/**
 * The power ladder. Every power capsule advances the player one step; powers are cumulative by slot
 * (the main gun is upgraded in place, missiles become homing, the rest is added on top). Losing a
 * ship resets the ladder to NORMAL.
 */
enum class power_step : unsigned char
{
    NORMAL,             // start: single forward shot
    HOMING_DOT,         // + small homing dot fired with the main gun
    MISSILE,            // + forward missiles
    LASER,              // main gun -> piercing laser
    SHIELD,             // + 3-hit shield (granted once when the step is reached)
    SPREAD_LASER,       // main gun -> three-way laser
    SHOOTER_1,          // + one additional shooter (trailing drone)
    SHOOTER_2,          // + a second additional shooter
    HOMING_MISSILE,     // missiles fire in pairs and home in
    SHOCKWAVE           // + periodic shockwave: clears enemies and bullets, 0.5 s invulnerability
};

constexpr int max_power = int(power_step::SHOCKWAVE);

enum class main_gun : unsigned char
{
    NORMAL,
    LASER,
    SPREAD_LASER
};

enum class missile_mode : unsigned char
{
    NONE,
    FORWARD,
    HOMING
};

[[nodiscard]] constexpr bool has_power(int power, power_step step)
{
    return power >= int(step);
}

[[nodiscard]] constexpr main_gun main_gun_of(int power)
{
    return has_power(power, power_step::SPREAD_LASER) ? main_gun::SPREAD_LASER :
           has_power(power, power_step::LASER) ? main_gun::LASER : main_gun::NORMAL;
}

[[nodiscard]] constexpr missile_mode missile_mode_of(int power)
{
    return has_power(power, power_step::HOMING_MISSILE) ? missile_mode::HOMING :
           has_power(power, power_step::MISSILE) ? missile_mode::FORWARD : missile_mode::NONE;
}

[[nodiscard]] constexpr int shooter_count_of(int power)
{
    return has_power(power, power_step::SHOOTER_2) ? 2 : has_power(power, power_step::SHOOTER_1) ? 1 : 0;
}

/// Banner shown when a step is reached (index = new power value).
constexpr const char* power_names[max_power + 1] = {
    "NORMAL SHOT", "HOMING DOT", "MISSILE", "LASER", "SHIELD", "SPREAD LASER", "SHOOTER", "2 SHOOTERS",
    "HOMING MISSILE", "SHOCKWAVE"
};

}

#endif
