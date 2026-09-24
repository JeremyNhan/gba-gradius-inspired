#ifndef SS_ENEMY_DATA_H
#define SS_ENEMY_DATA_H

#include "bn_fixed.h"
#include "bn_sprite_item.h"

namespace ss
{

enum class enemy_kind : unsigned char
{
    DART,           // A: basic straight fighter
    WAVER,          // B: sinusoidal saucer
    INTERCEPTOR,    // C: fast, stops, aims, dashes
    TURRET,         // D: fixed on the terrain, aims at the player
    HULK,           // E: large armoured gunship (mini encounter)
    SWARM,          // F: formation drone flying a loop
    MINE,           // G: drifting hazard that bursts into bullets
    ASTEROID_SMALL, // G: environmental debris
    ASTEROID_BIG,   // G: splits into small asteroids
    COUNT
};

enum class move_type : unsigned char
{
    STRAIGHT,
    SINE,
    INTERCEPT,
    GROUND,
    HOVER,
    LOOP,
    DRIFT,
    TUMBLE
};

enum class fire_type : unsigned char
{
    NONE,
    AIMED,          // one bullet at the player
    AIMED_SPREAD3,  // three bullets fanned around the player direction
    BURST3,         // three aimed bullets in quick succession
    RING8,          // eight-way ring (used on death by mines)
    STRAIGHT        // one bullet straight to the left
};

enum class explosion_size : unsigned char
{
    SMALL,
    BIG
};

struct enemy_def
{
    const bn::sprite_item* item;
    unsigned char frames;           // animation frames in the sprite sheet
    unsigned char anim_period;      // frames per animation step (0 = frame chosen by code)
    unsigned char half_w;           // hitbox half extents (pixels)
    unsigned char half_h;
    short hp;
    short score;
    move_type move;
    bn::fixed speed;
    fire_type fire;
    short fire_interval;            // frames between shots (stage difficulty shortens it)
    short first_fire_delay;
    unsigned char fire_from_stage;  // 0-based stage from which this enemy starts shooting
    explosion_size explosion;
    bool contact_damage_immune;     // survives ramming the player (big/armoured things)
};

}

#endif
