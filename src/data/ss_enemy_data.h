#ifndef SS_ENEMY_DATA_H
#define SS_ENEMY_DATA_H

#include "ss_base.h"

typedef enum
{
    ENEMY_DART,             /* A: basic straight fighter */
    ENEMY_WAVER,            /* B: sinusoidal saucer */
    ENEMY_INTERCEPTOR,      /* C: fast, stops, aims, dashes */
    ENEMY_TURRET,           /* D: fixed on the terrain, aims at the player */
    ENEMY_HULK,             /* E: large armoured gunship (mini encounter) */
    ENEMY_SWARM,            /* F: formation drone flying a loop */
    ENEMY_MINE,             /* G: drifting hazard that bursts into bullets */
    ENEMY_ASTEROID_SMALL,   /* G: environmental debris */
    ENEMY_ASTEROID_BIG,     /* G: splits into small asteroids */
    ENEMY_KIND_COUNT
} enemy_kind;

typedef enum
{
    MOVE_STRAIGHT,
    MOVE_SINE,
    MOVE_INTERCEPT,
    MOVE_GROUND,
    MOVE_HOVER,
    MOVE_LOOP,
    MOVE_DRIFT,
    MOVE_TUMBLE
} move_type;

typedef enum
{
    FIRE_NONE,
    FIRE_AIMED,             /* one bullet at the player */
    FIRE_AIMED_SPREAD3,     /* three bullets fanned around the player direction */
    FIRE_BURST3,            /* three aimed bullets in quick succession */
    FIRE_RING8,             /* eight-way ring (mines, on death) */
    FIRE_STRAIGHT           /* one bullet straight to the left */
} fire_type;

typedef struct
{
    u8 sheet;               /* GEN_SPR_* */
    u8 frames;              /* animation frames in the sheet */
    u8 anim_period;         /* frames per animation step (0 = frame chosen by code) */
    u8 half_w;              /* hitbox half extents (pixels) */
    u8 half_h;
    s16 hp;
    s16 score;
    u8 move;                /* move_type */
    fx speed;
    u8 fire;                /* fire_type */
    s16 fire_interval;      /* frames between shots (stage difficulty shortens it) */
    s16 first_fire_delay;
    u8 fire_from_stage;     /* 0-based stage from which this enemy starts shooting */
    bool big_explosion;
    bool contact_damage_immune;     /* survives ramming the player (big/armoured things) */
    u8 drop_chance;         /* percent chance to drop a power capsule when destroyed by the player */
} enemy_def;

extern const enemy_def enemy_defs[ENEMY_KIND_COUNT];

#endif
