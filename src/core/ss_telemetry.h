#ifndef SS_TELEMETRY_H
#define SS_TELEMETRY_H

#include <stdint.h>

/**
 * A small block of game-state values mirrored every frame at a fixed RAM address.
 *
 * Automated tests (tests/*.lua, run in mGBA) read it with emu:read8/16/32; the address comes from
 * the ELF symbol table (arm-none-eabi-nm). The `ctl_*` fields are written by tests and are only
 * honoured when SS_TEST_HOOKS is set (debug and profile builds), never in the release ROM.
 * Field order/sizes must match FIELDS in tests/lib/harness.lua.
 */
struct ss_telemetry_block
{
    char magic[8];              // "SSTELEM1"
    uint32_t frame;             // frames since boot
    uint32_t stage_frame;       // frames since stage start (stage timeline clock)
    uint32_t score;
    uint32_t hiscore;
    uint8_t state;              // ss::game_state
    uint8_t stage;              // 0-based stage index
    uint8_t lives;
    uint8_t boss_active;
    int16_t player_x;
    int16_t player_y;
    int16_t boss_hp;
    int16_t boss_hp_max;
    int16_t boss_y;
    uint8_t enemies;
    uint8_t enemy_bullets;
    uint8_t player_shots;
    uint8_t effects;
    uint8_t powerups;
    uint8_t weapon;
    uint8_t weapon_level;
    uint8_t missile_level;
    uint8_t speed_level;
    uint8_t shield;
    uint8_t cpu_pct;            // CPU usage of the last frame, percent
    uint8_t cpu_pct_max;        // worst frame since the stage started
    uint16_t missed_frames;     // total frames missed (logic slower than 1 frame) since boot
    uint16_t pool_drops;        // spawn requests dropped because a pool was full
    uint8_t player_alive;
    uint8_t deaths;             // player deaths this game
    uint8_t sprites_used;       // hardware sprites in use
    uint8_t ctl_invincible;     // [debug] test hook: player cannot die
    uint8_t ctl_autofire;       // [debug] test hook: fire held
    uint8_t ctl_skip_to_boss;   // [debug] test hook: jump stage timeline to the boss
    uint8_t music_playing;      // bn::music::playing()
    uint8_t pad;
    uint32_t cpu_max_stage_frame;   // stage_frame at which cpu_pct_max was recorded
    uint32_t cpu_max_frame;         // global frame of the same
};

extern "C" volatile ss_telemetry_block ss_telemetry;

#endif
