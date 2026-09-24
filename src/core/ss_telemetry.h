#ifndef SS_TELEMETRY_H
#define SS_TELEMETRY_H

#include <stdint.h>

/**
 * A small block of game-state values mirrored every frame at a fixed RAM address.
 *
 * Automated tests (Lua scripts in tests/, run in mGBA) read it with emu:read8/16/32; the address comes from
 * the ELF symbol table (arm-none-eabi-nm). The `ctl_*` fields are written by tests and are only
 * honoured when SS_TEST_HOOKS is set (debug and profile builds), never in the release ROM.
 * Field order/sizes must match FIELDS in tests/lib/harness.lua.
 */
struct ss_telemetry_block
{
    char magic[8] = { 'S', 'S', 'T', 'E', 'L', 'E', 'M', '1' };              // "SSTELEM1"
    uint32_t frame = 0;             // frames since boot
    uint32_t stage_frame = 0;       // frames since stage start (stage timeline clock)
    uint32_t score = 0;
    uint32_t hiscore = 0;
    uint8_t state = 0;              // ss::game_state
    uint8_t stage = 0;              // 0-based stage index
    uint8_t lives = 0;
    uint8_t boss_active = 0;
    int16_t player_x = 0;
    int16_t player_y = 0;
    int16_t boss_hp = 0;
    int16_t boss_hp_max = 0;
    int16_t boss_y = 0;
    uint8_t enemies = 0;
    uint8_t enemy_bullets = 0;
    uint8_t player_shots = 0;
    uint8_t effects = 0;
    uint8_t powerups = 0;
    uint8_t weapon = 0;
    uint8_t weapon_level = 0;
    uint8_t missile_level = 0;
    uint8_t speed_level = 0;
    uint8_t shield = 0;
    uint8_t cpu_pct = 0;            // CPU usage of the last frame, percent
    uint8_t cpu_pct_max = 0;        // worst frame since the stage started
    uint16_t missed_frames = 0;     // total frames missed (logic slower than 1 frame) since boot
    uint16_t pool_drops = 0;        // spawn requests dropped because a pool was full
    uint8_t player_alive = 0;
    uint8_t deaths = 0;             // player deaths this game
    uint8_t sprites_used = 0;       // hardware sprites in use
    uint8_t ctl_invincible = 0;     // [debug] test hook: player cannot die
    uint8_t ctl_autofire = 0;       // [debug] test hook: fire held
    uint8_t ctl_skip_to_boss = 0;   // [debug] test hook: jump stage timeline to the boss
    uint8_t music_playing = 0;      // bn::music::playing()
    uint8_t pad = 0;
    uint32_t cpu_max_stage_frame = 0;   // stage_frame at which cpu_pct_max was recorded
    uint32_t cpu_max_frame = 0;         // global frame of the same
    uint16_t sprite_tiles_used = 0;     // 4bpp tiles allocated in OBJ VRAM (1024 max = 32 KB)
    uint16_t bg_tiles_used = 0;         // 4bpp tiles allocated in BG VRAM
    uint16_t bg_map_cells_used = 0;     // BG map cells allocated
    uint16_t prof[12] = {};             // [test hooks] per-system cost of the last gameplay frame, 1/1000 frame
};

extern "C" volatile ss_telemetry_block ss_telemetry;

#endif
