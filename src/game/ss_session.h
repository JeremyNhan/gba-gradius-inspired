#ifndef SS_SESSION_H
#define SS_SESSION_H

#include "ss_constants.h"

namespace ss
{

/// Player equipment carried between stages (reset when a ship is lost).
struct loadout
{
    int power = 0;              // step on the power ladder, 0..max_power (see ss_power_data.h)
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
