#ifndef SS_STAGE_DATA_H
#define SS_STAGE_DATA_H

#include "ss_audio.h"
#include "ss_base.h"
#include "ss_video.h"

typedef enum
{
    EVENT_SPAWN,            /* a = enemy_kind, b = y, c = param, d = flags */
    EVENT_FORMATION,        /* a = formation_type, b = y, c = count, d = flags */
    EVENT_SCROLL_SPEED,     /* b = speed in 1/100 px per frame */
    EVENT_WARNING,          /* boss warning banner + siren */
    EVENT_BOSS              /* a = boss_id */
} event_type;

typedef enum
{
    FORMATION_LINE,         /* darts in a horizontal line (one after another) */
    FORMATION_V,            /* darts in a V */
    FORMATION_COLUMN,       /* darts in a vertical column */
    FORMATION_WAVE,         /* wavers following each other */
    FORMATION_SWARM_LOOP,   /* swarm drones looping */
    FORMATION_MINE_FIELD    /* mines scattered vertically */
} formation_type;

typedef enum
{
    BOSS_WARDEN,
    BOSS_HIVE,
    BOSS_OVERMIND
} boss_id;

/* Spawn flags. */
#define FLAG_BONUS 1        /* bonus formation: +500 when every member is destroyed */
#define FLAG_CEILING 2      /* turret mounted on the ceiling */
#define FLAG_FROM_LEFT 4    /* enters from the left edge */

typedef struct
{
    u16 frame;
    u8 type;                /* event_type */
    u8 a;
    s16 b;
    s16 c;
    s16 d;
} stage_event;

/* Terrain key point: at world column `column` (8 px), ceiling/floor are this many tiles thick.
 * Heights are linearly interpolated between keys. */
typedef struct
{
    s16 column;
    s8 ceiling;
    s8 floor;
} terrain_key;

typedef struct
{
    const char* name;
    const char* subtitle;
    u8 backdrop;            /* backdrop_type */
    s8 terrain_style;       /* -1 none, 0 crystal, 1 metal */
    u8 music;               /* music_id */
    fx scroll_speed;
    const stage_event* events;
    int event_count;
    const terrain_key* terrain;
    int terrain_count;
} stage_def;

const stage_def* stage_definition(int stage);

#endif
