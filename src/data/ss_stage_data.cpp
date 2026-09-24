#include "ss_stage_data.h"

#include "bn_assert.h"

#include "ss_constants.h"

namespace ss
{

namespace
{
    // ----- event builders (keep the tables readable) -------------------------------------------
    constexpr stage_event spawn(int frame, enemy_kind kind, int y, int param = 0, int flags = 0)
    {
        return { (unsigned short) frame, event_type::SPAWN, (unsigned char) kind, (short) y, (short) param,
                 (short) flags };
    }

    constexpr stage_event turret(int frame, bool ceiling)
    {
        return spawn(frame, enemy_kind::TURRET, 0, 0, ceiling ? flag_ceiling : 0);
    }

    constexpr stage_event formation(int frame, formation_type type, int y, int count, int flags = 0)
    {
        return { (unsigned short) frame, event_type::FORMATION, (unsigned char) type, (short) y, (short) count,
                 (short) flags };
    }

    constexpr stage_event scroll_speed(int frame, int hundredths)
    {
        return { (unsigned short) frame, event_type::SCROLL_SPEED, 0, (short) hundredths, 0, 0 };
    }

    constexpr stage_event warning(int frame)
    {
        return { (unsigned short) frame, event_type::WARNING, 0, 0, 0, 0 };
    }

    constexpr stage_event boss(int frame, boss_id id)
    {
        return { (unsigned short) frame, event_type::BOSS, (unsigned char) id, 0, 0, 0 };
    }

    using ek = enemy_kind;
    using ft = formation_type;
    constexpr int C = flag_carrier;

    // ----- Stage 1: OUTER RIM — open space, asteroids, introduces every basic enemy ---------------
    constexpr stage_event stage1_events[] = {
        formation(120, ft::LINE, -35, 4),
        formation(240, ft::LINE, 35, 4),
        formation(360, ft::WAVE, 0, 4),
        formation(520, ft::SWARM_LOOP, -30, 6, C),
        spawn(700, ek::ASTEROID_SMALL, -40, 20),
        spawn(760, ek::ASTEROID_SMALL, 20, -30),
        spawn(820, ek::ASTEROID_SMALL, 50, -10),
        formation(900, ft::V, 0, 5),
        formation(1060, ft::WAVE, -40, 4),
        formation(1120, ft::WAVE, 40, 4, C),
        spawn(1300, ek::INTERCEPTOR, -40),
        spawn(1330, ek::INTERCEPTOR, 40),
        formation(1450, ft::SWARM_LOOP, 30, 6, C),
        spawn(1650, ek::ASTEROID_BIG, -30, 10),
        spawn(1700, ek::ASTEROID_SMALL, 40, -20),
        spawn(1760, ek::ASTEROID_BIG, 35, -10),
        formation(1950, ft::COLUMN, 0, 4),
        spawn(2100, ek::HULK, 0, 0, C),                 // mini encounter
        formation(2160, ft::LINE, -55, 3),
        formation(2400, ft::LINE, 55, 3),
        spawn(2700, ek::INTERCEPTOR, -20),
        spawn(2730, ek::INTERCEPTOR, 20),
        spawn(2760, ek::INTERCEPTOR, 0),
        formation(2900, ft::MINE_FIELD, 0, 3),
        formation(3100, ft::SWARM_LOOP, -30, 6, C),
        formation(3250, ft::SWARM_LOOP, 30, 6),
        formation(3450, ft::WAVE, 0, 5, C),
        spawn(3500, ek::INTERCEPTOR, -50),
        spawn(3700, ek::ASTEROID_BIG, -40, 15),
        spawn(3760, ek::ASTEROID_SMALL, 0, -10),
        spawn(3820, ek::ASTEROID_SMALL, 50, -30),
        spawn(3880, ek::ASTEROID_BIG, 20, -10),
        spawn(3950, ek::ASTEROID_SMALL, -55, 20),
        spawn(4020, ek::ASTEROID_SMALL, 10, 0),
        spawn(4200, ek::HULK, -35),
        spawn(4260, ek::HULK, 35, 0, C),
        formation(4500, ft::V, -20, 5),
        formation(4620, ft::V, 20, 5),
        warning(4900),
        boss(5100, boss_id::WARDEN),
    };

    // ----- Stage 2: CRYSTAL CAVERNS — terrain, turrets, mines ------------------------------------
    constexpr terrain_key stage2_terrain[] = {
        { 0, 0, 0 }, { 40, 0, 0 }, { 60, 3, 3 }, { 100, 4, 2 }, { 130, 2, 5 }, { 160, 5, 3 },
        { 190, 3, 3 }, { 220, 4, 4 }, { 250, 3, 5 }, { 280, 5, 3 }, { 310, 3, 3 }, { 400, 3, 3 },
    };

    constexpr stage_event stage2_events[] = {
        formation(150, ft::WAVE, 0, 4),
        formation(300, ft::LINE, -20, 4),
        formation(450, ft::SWARM_LOOP, 10, 6, C),
        turret(700, false),
        turret(760, true),
        turret(820, false),
        formation(950, ft::MINE_FIELD, 0, 3),
        formation(1150, ft::WAVE, -10, 5, C),
        turret(1350, false),
        turret(1380, true),
        spawn(1500, ek::INTERCEPTOR, 0),
        formation(1650, ft::SWARM_LOOP, -10, 6, C),
        turret(1850, false),
        turret(1900, false),
        turret(1950, true),
        spawn(2100, ek::HULK, 0, 0, C),                 // mini encounter
        formation(2500, ft::MINE_FIELD, 0, 4),
        formation(2700, ft::COLUMN, 0, 4),
        turret(2800, true),
        turret(2850, false),
        spawn(3000, ek::INTERCEPTOR, -20),
        spawn(3030, ek::INTERCEPTOR, 20),
        formation(3200, ft::SWARM_LOOP, 0, 6, C),
        turret(3400, false),
        turret(3430, true),
        turret(3460, false),
        turret(3490, true),
        formation(3650, ft::WAVE, 0, 5),
        spawn(3850, ek::HULK, -10, 0, C),
        formation(4200, ft::MINE_FIELD, 0, 4),
        formation(4400, ft::V, 0, 5),
        warning(4700),
        boss(4900, boss_id::HIVE),
    };

    // ----- Stage 3: DREADNOUGHT — tight metal corridors, everything at once ---------------------
    constexpr terrain_key stage3_terrain[] = {
        { 0, 2, 2 }, { 30, 2, 2 }, { 50, 4, 4 }, { 80, 3, 5 }, { 110, 5, 3 }, { 140, 4, 4 }, { 170, 5, 4 },
        { 200, 3, 6 }, { 230, 6, 3 }, { 260, 4, 4 }, { 300, 5, 4 }, { 340, 3, 3 }, { 600, 3, 3 },
    };

    constexpr stage_event stage3_events[] = {
        formation(150, ft::LINE, 0, 4),
        turret(300, false),
        turret(330, true),
        formation(450, ft::SWARM_LOOP, 0, 6, C),
        spawn(650, ek::INTERCEPTOR, -10),
        spawn(680, ek::INTERCEPTOR, 10),
        turret(800, false),
        turret(840, false),
        turret(860, true),
        turret(880, false),
        formation(1000, ft::WAVE, 0, 5, C),
        spawn(1200, ek::HULK, 0),                       // mini encounter
        formation(1500, ft::MINE_FIELD, 0, 4),
        turret(1700, true),
        turret(1740, false),
        turret(1780, true),
        formation(1900, ft::COLUMN, 0, 4),
        formation(2000, ft::V, 0, 5, C),
        spawn(2200, ek::INTERCEPTOR, -15),
        spawn(2220, ek::INTERCEPTOR, 15),
        spawn(2240, ek::INTERCEPTOR, 0),
        spawn(2400, ek::HULK, -20),
        spawn(2460, ek::HULK, 20, 0, C),
        formation(2800, ft::SWARM_LOOP, -10, 6),
        formation(2900, ft::SWARM_LOOP, 10, 6, C),
        turret(3100, false),
        turret(3130, true),
        turret(3160, false),
        turret(3190, true),
        formation(3300, ft::MINE_FIELD, 0, 5),
        formation(3500, ft::WAVE, 0, 5, C),
        spawn(3700, ek::INTERCEPTOR, -20),
        spawn(3720, ek::INTERCEPTOR, 20),
        spawn(3740, ek::INTERCEPTOR, 0),
        spawn(3900, ek::HULK, 0, 0, C),
        formation(4300, ft::LINE, -15, 4),
        formation(4360, ft::LINE, 15, 4),
        warning(4600),
        scroll_speed(4700, 50),
        boss(4800, boss_id::OVERMIND),
    };

    constexpr stage_def stages[] = {
        { "STAGE 1", "OUTER RIM", backdrop_type::NEBULA, terrain_style::NONE, audio::music::STAGE1,
          bn::fixed(0.5), stage1_events, bn::span<const terrain_key>() },
        { "STAGE 2", "CRYSTAL CAVERNS", backdrop_type::CAVE, terrain_style::CRYSTAL, audio::music::STAGE2,
          bn::fixed(0.5), stage2_events, stage2_terrain },
        { "STAGE 3", "THE DREADNOUGHT", backdrop_type::HULL, terrain_style::METAL, audio::music::STAGE3,
          bn::fixed(0.75), stage3_events, stage3_terrain },
    };

    static_assert(sizeof(stages) / sizeof(stages[0]) == stage_count);

    constexpr bool events_sorted(const bn::span<const stage_event>& events)
    {
        for(int index = 1; index < events.size(); ++index)
        {
            if(events[index].frame < events[index - 1].frame)
            {
                return false;
            }
        }

        return true;
    }

    static_assert(events_sorted(stage1_events));
    static_assert(events_sorted(stage2_events));
    static_assert(events_sorted(stage3_events));
}

const stage_def& stage_definition(int stage_index)
{
    BN_ASSERT(stage_index >= 0 && stage_index < stage_count, "Invalid stage: ", stage_index);
    return stages[stage_index];
}

}
