#ifndef SS_STAGE_DATA_H
#define SS_STAGE_DATA_H

#include "bn_fixed.h"
#include "bn_span.h"

#include "ss_audio.h"
#include "ss_backgrounds.h"
#include "ss_enemy_data.h"

namespace ss
{

enum class event_type : unsigned char
{
    SPAWN,          // a = enemy_kind, b = y, c = param, d = flags
    FORMATION,      // a = formation_type, b = y, c = count, d = flags
    SCROLL_SPEED,   // b = speed in 1/100 px per frame
    WARNING,        // boss warning banner + siren + boss music
    BOSS,           // a = boss_id
};

enum class formation_type : unsigned char
{
    LINE,           // darts in a horizontal line (one after another)
    V,              // darts in a V
    COLUMN,         // darts in a vertical column
    WAVE,           // wavers following each other
    SWARM_LOOP,     // swarm drones looping
    MINE_FIELD      // mines scattered vertically
};

enum class boss_id : unsigned char
{
    WARDEN,
    HIVE,
    OVERMIND
};

// Spawn flags.
constexpr short flag_carrier = 1;       // drops a power-up (for formations: when all are destroyed)
constexpr short flag_ceiling = 2;       // turret mounted on the ceiling
constexpr short flag_from_left = 4;     // enters from the left edge

struct stage_event
{
    unsigned short frame;
    event_type type;
    unsigned char a;
    short b;
    short c;
    short d;
};

/// Terrain key point: at world column `column` (8 px), ceiling/floor are this many tiles thick.
/// Heights are linearly interpolated between keys.
struct terrain_key
{
    short column;
    signed char ceiling;
    signed char floor;
};

enum class terrain_style : unsigned char
{
    NONE,
    CRYSTAL,
    METAL
};

struct stage_def
{
    const char* name;
    const char* subtitle;
    backdrop_type backdrop;
    terrain_style terrain;
    audio::music music;
    bn::fixed scroll_speed;
    bn::span<const stage_event> events;
    bn::span<const terrain_key> terrain_keys;
};

[[nodiscard]] const stage_def& stage_definition(int stage_index);

}

#endif
