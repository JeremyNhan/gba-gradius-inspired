#ifndef SS_SESSION_H
#define SS_SESSION_H

#include "ss_constants.h"

namespace ss
{

enum class weapon_type : unsigned char
{
    NORMAL,
    SPREAD
};

/// Player equipment carried between stages (lost partially on death).
struct loadout
{
    weapon_type weapon = weapon_type::NORMAL;
    int weapon_level = 1;       // 1..max_weapon_level
    int missile_level = 0;      // 0..max_missile_level
    int speed_level = 1;        // 1..max_speed_level
    int shield = 0;             // hits absorbed, 0..max_shield
};

/// Data that lives for a whole play-through (title -> game over / ending).
struct session
{
    int stage = 0;              // 0-based
    int lives = start_lives;
    int score = 0;
    int hiscore = 0;
    int deaths = 0;
    loadout gear;

    void add_score(int points)
    {
        score += points;

        if(score > 99999990)
        {
            score = 99999990;
        }

        if(score > hiscore)
        {
            hiscore = score;
        }
    }
};

}

#endif
