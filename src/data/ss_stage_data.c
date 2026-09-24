/* Stage timelines (frame-stamped events) and terrain height keys. Events must be sorted by frame. */
#include "ss_stage_data.h"

#include "ss_enemy_data.h"

#define SPAWN(frame, kind, y, param, flags) { frame, EVENT_SPAWN, kind, y, param, flags }
#define TURRET(frame, ceiling) { frame, EVENT_SPAWN, ENEMY_TURRET, 0, 0, (ceiling) ? FLAG_CEILING : 0 }
#define FORMATION(frame, type, y, count, flags) { frame, EVENT_FORMATION, type, y, count, flags }
#define SCROLL(frame, hundredths) { frame, EVENT_SCROLL_SPEED, 0, hundredths, 0, 0 }
#define WARNING(frame) { frame, EVENT_WARNING, 0, 0, 0, 0 }
#define BOSS(frame, id) { frame, EVENT_BOSS, id, 0, 0, 0 }

#define B FLAG_BONUS

/* ----- Stage 1: OUTER RIM - open space, asteroids, introduces every basic enemy ------------------ */
static const stage_event stage1_events[] = {
    FORMATION(120, FORMATION_LINE, -35, 4, 0),
    FORMATION(240, FORMATION_LINE, 35, 4, 0),
    FORMATION(360, FORMATION_WAVE, 0, 4, 0),
    FORMATION(520, FORMATION_SWARM_LOOP, -30, 6, B),
    SPAWN(700, ENEMY_ASTEROID_SMALL, -40, 20, 0),
    SPAWN(760, ENEMY_ASTEROID_SMALL, 20, -30, 0),
    SPAWN(820, ENEMY_ASTEROID_SMALL, 50, -10, 0),
    FORMATION(900, FORMATION_V, 0, 5, 0),
    FORMATION(1060, FORMATION_WAVE, -40, 4, 0),
    FORMATION(1120, FORMATION_WAVE, 40, 4, B),
    SPAWN(1300, ENEMY_INTERCEPTOR, -40, 0, 0),
    SPAWN(1330, ENEMY_INTERCEPTOR, 40, 0, 0),
    FORMATION(1450, FORMATION_SWARM_LOOP, 30, 6, B),
    SPAWN(1650, ENEMY_ASTEROID_BIG, -30, 10, 0),
    SPAWN(1700, ENEMY_ASTEROID_SMALL, 40, -20, 0),
    SPAWN(1760, ENEMY_ASTEROID_BIG, 35, -10, 0),
    FORMATION(1950, FORMATION_COLUMN, 0, 4, 0),
    SPAWN(2100, ENEMY_HULK, 0, 0, 0),                   /* mini encounter */
    FORMATION(2160, FORMATION_LINE, -55, 3, 0),
    FORMATION(2400, FORMATION_LINE, 55, 3, 0),
    SPAWN(2700, ENEMY_INTERCEPTOR, -20, 0, 0),
    SPAWN(2730, ENEMY_INTERCEPTOR, 20, 0, 0),
    SPAWN(2760, ENEMY_INTERCEPTOR, 0, 0, 0),
    FORMATION(2900, FORMATION_MINE_FIELD, 0, 3, 0),
    FORMATION(3100, FORMATION_SWARM_LOOP, -30, 6, B),
    FORMATION(3250, FORMATION_SWARM_LOOP, 30, 6, 0),
    FORMATION(3450, FORMATION_WAVE, 0, 5, B),
    SPAWN(3500, ENEMY_INTERCEPTOR, -50, 0, 0),
    SPAWN(3700, ENEMY_ASTEROID_BIG, -40, 15, 0),
    SPAWN(3760, ENEMY_ASTEROID_SMALL, 0, -10, 0),
    SPAWN(3820, ENEMY_ASTEROID_SMALL, 50, -30, 0),
    SPAWN(3880, ENEMY_ASTEROID_BIG, 20, -10, 0),
    SPAWN(3950, ENEMY_ASTEROID_SMALL, -55, 20, 0),
    SPAWN(4020, ENEMY_ASTEROID_SMALL, 10, 0, 0),
    SPAWN(4200, ENEMY_HULK, -35, 0, 0),
    SPAWN(4260, ENEMY_HULK, 35, 0, 0),
    FORMATION(4500, FORMATION_V, -20, 5, 0),
    FORMATION(4620, FORMATION_V, 20, 5, 0),
    WARNING(4900),
    BOSS(5100, BOSS_WARDEN),
};

/* ----- Stage 2: CRYSTAL CAVERNS - terrain, turrets, mines ------------------------------------------ */
static const terrain_key stage2_terrain[] = {
    { 0, 0, 0 }, { 40, 0, 0 }, { 60, 3, 3 }, { 100, 4, 2 }, { 130, 2, 5 }, { 160, 5, 3 },
    { 190, 3, 3 }, { 220, 4, 4 }, { 250, 3, 5 }, { 280, 5, 3 }, { 310, 3, 3 }, { 400, 3, 3 },
};

static const stage_event stage2_events[] = {
    FORMATION(150, FORMATION_WAVE, 0, 4, 0),
    FORMATION(300, FORMATION_LINE, -20, 4, 0),
    FORMATION(450, FORMATION_SWARM_LOOP, 10, 6, B),
    TURRET(700, false),
    TURRET(760, true),
    TURRET(820, false),
    FORMATION(950, FORMATION_MINE_FIELD, 0, 3, 0),
    FORMATION(1150, FORMATION_WAVE, -10, 5, B),
    TURRET(1350, false),
    TURRET(1380, true),
    SPAWN(1500, ENEMY_INTERCEPTOR, 0, 0, 0),
    FORMATION(1650, FORMATION_SWARM_LOOP, -10, 6, B),
    TURRET(1850, false),
    TURRET(1900, false),
    TURRET(1950, true),
    SPAWN(2100, ENEMY_HULK, 0, 0, 0),                   /* mini encounter */
    FORMATION(2500, FORMATION_MINE_FIELD, 0, 4, 0),
    FORMATION(2700, FORMATION_COLUMN, 0, 4, 0),
    TURRET(2800, true),
    TURRET(2850, false),
    SPAWN(3000, ENEMY_INTERCEPTOR, -20, 0, 0),
    SPAWN(3030, ENEMY_INTERCEPTOR, 20, 0, 0),
    FORMATION(3200, FORMATION_SWARM_LOOP, 0, 6, B),
    TURRET(3400, false),
    TURRET(3430, true),
    TURRET(3460, false),
    TURRET(3490, true),
    FORMATION(3650, FORMATION_WAVE, 0, 5, 0),
    SPAWN(3850, ENEMY_HULK, -10, 0, 0),
    FORMATION(4200, FORMATION_MINE_FIELD, 0, 4, 0),
    FORMATION(4400, FORMATION_V, 0, 5, 0),
    WARNING(4700),
    BOSS(4900, BOSS_HIVE),
};

/* ----- Stage 3: DREADNOUGHT - tight metal corridors, everything at once --------------------------- */
static const terrain_key stage3_terrain[] = {
    { 0, 2, 2 }, { 30, 2, 2 }, { 50, 4, 4 }, { 80, 3, 5 }, { 110, 5, 3 }, { 140, 4, 4 }, { 170, 5, 4 },
    { 200, 3, 6 }, { 230, 6, 3 }, { 260, 4, 4 }, { 300, 5, 4 }, { 340, 3, 3 }, { 600, 3, 3 },
};

static const stage_event stage3_events[] = {
    FORMATION(150, FORMATION_LINE, 0, 4, 0),
    TURRET(300, false),
    TURRET(330, true),
    FORMATION(450, FORMATION_SWARM_LOOP, 0, 6, B),
    SPAWN(650, ENEMY_INTERCEPTOR, -10, 0, 0),
    SPAWN(680, ENEMY_INTERCEPTOR, 10, 0, 0),
    TURRET(800, false),
    TURRET(840, false),
    TURRET(860, true),
    TURRET(880, false),
    FORMATION(1000, FORMATION_WAVE, 0, 5, B),
    SPAWN(1200, ENEMY_HULK, 0, 0, 0),                   /* mini encounter */
    FORMATION(1500, FORMATION_MINE_FIELD, 0, 4, 0),
    TURRET(1700, true),
    TURRET(1740, false),
    TURRET(1780, true),
    FORMATION(1900, FORMATION_COLUMN, 0, 4, 0),
    FORMATION(2000, FORMATION_V, 0, 5, B),
    SPAWN(2200, ENEMY_INTERCEPTOR, -15, 0, 0),
    SPAWN(2220, ENEMY_INTERCEPTOR, 15, 0, 0),
    SPAWN(2240, ENEMY_INTERCEPTOR, 0, 0, 0),
    SPAWN(2400, ENEMY_HULK, -20, 0, 0),
    SPAWN(2460, ENEMY_HULK, 20, 0, 0),
    FORMATION(2800, FORMATION_SWARM_LOOP, -10, 6, 0),
    FORMATION(2900, FORMATION_SWARM_LOOP, 10, 6, B),
    TURRET(3100, false),
    TURRET(3130, true),
    TURRET(3160, false),
    TURRET(3190, true),
    FORMATION(3300, FORMATION_MINE_FIELD, 0, 5, 0),
    FORMATION(3500, FORMATION_WAVE, 0, 5, B),
    SPAWN(3700, ENEMY_INTERCEPTOR, -20, 0, 0),
    SPAWN(3720, ENEMY_INTERCEPTOR, 20, 0, 0),
    SPAWN(3740, ENEMY_INTERCEPTOR, 0, 0, 0),
    SPAWN(3900, ENEMY_HULK, 0, 0, 0),
    FORMATION(4300, FORMATION_LINE, -15, 4, 0),
    FORMATION(4360, FORMATION_LINE, 15, 4, 0),
    WARNING(4600),
    SCROLL(4700, 50),
    BOSS(4800, BOSS_OVERMIND),
};

static const stage_def stages[STAGE_COUNT] = {
    { "STAGE 1", "OUTER RIM", BACKDROP_NEBULA, -1, MUSIC_STAGE1, FX_F(0.5),
      stage1_events, COUNT_OF(stage1_events), NULL, 0 },
    { "STAGE 2", "CRYSTAL CAVERNS", BACKDROP_CAVE, 0, MUSIC_STAGE2, FX_F(0.5),
      stage2_events, COUNT_OF(stage2_events), stage2_terrain, COUNT_OF(stage2_terrain) },
    { "STAGE 3", "THE DREADNOUGHT", BACKDROP_HULL, 1, MUSIC_STAGE3, FX_F(0.75),
      stage3_events, COUNT_OF(stage3_events), stage3_terrain, COUNT_OF(stage3_terrain) },
};

const stage_def* stage_definition(int stage)
{
    return &stages[SS_CLAMP(stage, 0, STAGE_COUNT - 1)];
}
