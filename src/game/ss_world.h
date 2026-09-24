#ifndef SS_WORLD_H
#define SS_WORLD_H

#include "bn_camera_ptr.h"
#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_random.h"

#include "ss_backgrounds.h"
#include "ss_boss.h"
#include "ss_bullets.h"
#include "ss_effects.h"
#include "ss_enemies.h"
#include "ss_hud.h"
#include "ss_player.h"
#include "ss_powerups.h"
#include "ss_session.h"
#include "ss_shots.h"
#include "ss_stage_data.h"
#include "ss_stage_runner.h"
#include "ss_terrain.h"

namespace ss
{

class text;

/**
 * Everything that exists while a stage is being played. One world object is created per stage
 * (in EWRAM, see ss_app.cpp) and destroyed on stage clear / game over, which releases all sprites
 * and backgrounds in one go.
 */
class world
{

public:
    enum class result : unsigned char
    {
        NONE,
        STAGE_CLEARED,
        GAME_OVER,
        GAME_COMPLETE
    };

    world(session& game_session, text& text_generator);

    /// One gameplay frame. Deterministic: depends only on input and the seeded RNG.
    [[nodiscard]] result update();

    /// Stage-clear outro: scrolling continues and the ship flies off-screen. Returns true when done.
    bool update_outro();

    // ----- shared services used by the entity systems --------------------------------------------
    session& game;
    text& txt;
    bn::optional<bn::camera_ptr> camera;
    const stage_def& stage;
    bn::random rng;
    bn::fixed scroll_x = 0;
    bn::fixed scroll_speed;
    int stage_frame = 0;

    terrain ground;             // created first: its tiles must start at a charblock boundary
    backgrounds bgs;
    player ship;
    player_shots shots;
    enemies foes;
    enemy_bullets bullets;
    powerups items;
    effects fx;
    boss big_boss;
    stage_runner runner;
    hud display;

    /// Short screen shake (amplitude in pixels).
    void shake(int frames, int amplitude);

    void add_score(int points);

    /// Moves the player to a step of the power ladder (grants the shield when the SHIELD step is reached).
    void set_power(int power);

    /// Shockwave (top power step): destroys every enemy and bullet, damages the boss, flashes the
    /// screen and makes the player briefly invulnerable.
    void shockwave();

    int shockwaves = 0;         // fired this stage (telemetry)

    /// Difficulty scale for enemy fire rates/speeds: 0 for stage 1, grows with each stage.
    [[nodiscard]] int difficulty() const
    {
        return game.stage;
    }

    /// Input helpers (include the debug test hooks).
    [[nodiscard]] bool fire_held() const;
    [[nodiscard]] bool charge_held() const;
    [[nodiscard]] bool invincible() const;

    /// Called by the player when its last life is gone.
    void notify_player_out_of_lives()
    {
        _game_over_timer = 120;
    }

    /// Called by the boss when its death sequence has finished.
    void notify_boss_defeated();

private:
    int _shake_frames = 0;
    int _shake_amplitude = 0;
    int _game_over_timer = 0;
    int _clear_timer = 0;
    int _flash_frames = 0;
    bool _debug_overlay = false;

    void _collide();
    void _update_camera();
    void _update_flash();
    void _update_telemetry();
};

}

#endif
