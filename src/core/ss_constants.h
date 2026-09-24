#ifndef SS_CONSTANTS_H
#define SS_CONSTANTS_H

#include "bn_fixed.h"

#ifndef SS_DEBUG
    #define SS_DEBUG 0
#endif

// Test hooks (telemetry ctl_* fields: invincibility, autofire, skip to boss). On in debug builds and
// in the PROFILE build (release code + hooks, used to measure real performance); off in release.
#ifndef SS_TEST_HOOKS
    #define SS_TEST_HOOKS SS_DEBUG
#endif

namespace ss
{
    // Screen (Butano uses centre-origin coordinates: x in [-120, 120), y in [-80, 80)).
    constexpr int screen_w = 240;
    constexpr int screen_h = 160;
    constexpr int half_w = screen_w / 2;
    constexpr int half_h = screen_h / 2;

    // Playfield: the top 10 pixels hold the HUD line.
    constexpr int hud_h = 10;
    constexpr int play_top = -half_h + hud_h;
    constexpr int play_bottom = half_h;

    // Entity pool capacities. Sum stays under the 128 hardware sprites (see docs/architecture.md).
    constexpr int max_player_shots = 24;
    constexpr int max_enemies = 16;
    constexpr int max_enemy_bullets = 32;
    constexpr int max_effects = 16;
    constexpr int max_powerups = 4;

    // Player tuning.
    constexpr int start_lives = 3;
    constexpr int max_lives = 9;
    constexpr int respawn_frames = 90;
    constexpr int invulnerable_frames = 150;
    constexpr int max_speed_level = 3;
    constexpr int max_weapon_level = 3;
    constexpr int max_missile_level = 2;
    constexpr int max_shield = 3;
    constexpr int charge_frames = 45;

    // Sprite layering (lower z_order = drawn in front within the same BG priority).
    constexpr int z_hud = 0;
    constexpr int z_player = 10;
    constexpr int z_effects = 12;
    constexpr int z_bullets = 14;
    constexpr int z_shots = 16;
    constexpr int z_powerups = 18;
    constexpr int z_enemies = 20;
    constexpr int z_boss = 30;

    // Background layering (BG priority 0 = front).
    constexpr int bg_priority_logo = 0;
    constexpr int bg_priority_terrain = 1;
    constexpr int bg_priority_backdrop = 2;
    constexpr int bg_priority_stars = 3;
    constexpr int sprite_bg_priority = 1;   // sprites drawn in front of the terrain layer
    constexpr int hud_bg_priority = 0;

    constexpr int stage_count = 3;
}

#endif
