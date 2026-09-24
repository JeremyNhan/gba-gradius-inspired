/*
 * Test scenarios. Each suite is a protothread: T_WAIT / T_WAIT_UNTIL return to the game loop and
 * resume on a later frame, so everything that must survive a wait lives in a static struct.
 */
#include "ss_test.h"

#include "ss_app.h"
#include "ss_audio.h"
#include "ss_boss.h"
#include "ss_bullets.h"
#include "ss_enemies.h"
#include "ss_frame.h"
#include "ss_player.h"
#include "ss_power_data.h"
#include "ss_powerups.h"
#include "ss_save.h"
#include "ss_shots.h"
#include "ss_sprites.h"
#include "ss_world.h"

static const char* const state_names[] = { "TITLE", "PLAYING", "PAUSED", "STAGE_CLEAR", "GAME_OVER", "ENDING" };

static int px(void)
{
    return fx_round(player_position().x);
}

static int py(void)
{
    return fx_round(player_position().y);
}

/* One-line summary of the game state for check details. */
static const char* snapshot(void)
{
    static char line[160];
    snprintf(line, sizeof(line), "state=%s stage=%d sf=%d lives=%d score=%d boss=%d/%d E=%d B=%d S=%d cpu=%d%%",
             state_names[app_state()], game.stage, world.stage_frame, game.lives, game.score, boss_hp(),
             boss_hp_max(), enemies_count(), bullets_count(), shots_count(), frame_info.cpu_pct);
    return line;
}

static const char* power_line(void)
{
    static char line[96];
    int p = game.gear.power;
    snprintf(line, sizeof(line), "power=%d gun=%d missiles=%d shooters=%d shield=%d waves=%d", p, main_gun_of(p),
             missile_mode_of(p), shooter_count_of(p), game.gear.shield, world.shockwaves);
    return line;
}

/* Missed frames during gameplay (stage loading and screen changes excluded). */
static u32 gameplay_missed;
static u32 last_missed;

static void reset_missed(void)
{
    gameplay_missed = 0;
    last_missed = frame_info.missed_frames;
}

static void track_missed(void)
{
    u32 delta = frame_info.missed_frames - last_missed;

    if(delta)
    {
        test_log("missed %u frame(s): %s", (unsigned) delta, snapshot());
    }

    if(app_state() == STATE_PLAYING && world.stage_frame > 3)
    {
        gameplay_missed += delta;
    }

    last_missed = frame_info.missed_frames;
}

#define PRESS(t, key) do { test_key_down(key); T_WAIT(t, 1); test_key_up(key); } while(0)
#define HOLD(t, key, frames) do { test_key_down(key); T_WAIT(t, frames); test_key_up(key); } while(0)

/* =================================================================================================== */
/* smoke: boot, title, start, shooting, charge shot, pause, movement, screen bounds, combat, audio      */
/* =================================================================================================== */

static struct
{
    test_thread t;
    int sf;
    int x0;
    int y0;
    int loop;
    u16 dir;
    int max_enemies;
} sm;

static void smoke_track(void)
{
    track_missed();
    sm.max_enemies = MAX(sm.max_enemies, enemies_count());
}

bool suite_smoke(void)
{
    test_thread* t = &sm.t;
    T_BEGIN(t);
    reset_missed();
    test_set_hooks(smoke_track, NULL);
    T_WAIT(t, 120);
    test_check("boots to TITLE", app_state() == STATE_TITLE, "%s", state_names[app_state()]);
    test_shot("smoke_01_title");
    test_check("title music playing", audio_music_playing(), NULL);
    test_check("sound hardware enabled (master, Direct Sound, sample timer)",
               (REG_SNDSTAT & 0x80) && (REG_SNDDSCNT & 0x3300) && (REG_TM0CNT & 0x80),
               "SNDSTAT=%04X SNDDSCNT=%04X TM0CNT=%04X", REG_SNDSTAT, REG_SNDDSCNT, REG_TM0CNT);

    PRESS(t, KEY_START);
    T_WAIT(t, 20);
    test_check("START begins game", app_state() == STATE_PLAYING, "%s", state_names[app_state()]);
    test_check("starts on stage 1", game.stage == 0, NULL);
    test_check("starts with 3 lives", game.lives == 3, "%d", game.lives);
    T_WAIT(t, 40);
    test_shot("smoke_02_stage_banner");

    /* shooting */
    test_key_down(KEY_A);
    T_WAIT(t, 15);
    test_check("A fires shots", shots_count() > 0, "%d", shots_count());
    test_shot("smoke_03_shooting");
    test_key_up(KEY_A);

    /* charged shot: hold B past the charge time, release, one piercing wave comes out */
    T_WAIT(t, 45);
    test_check("normal shots leave the screen", shots_count() == 0, "%d", shots_count());
    HOLD(t, KEY_B, 60);
    T_WAIT(t, 2);
    test_check("B charge + release fires a charged shot", shots_count() > 0, "%d", shots_count());

    /* pause freezes gameplay completely */
    PRESS(t, KEY_START);
    T_WAIT(t, 5);
    test_check("START pauses", app_state() == STATE_PAUSED, "%s", state_names[app_state()]);
    sm.sf = world.stage_frame;
    sm.x0 = px();
    HOLD(t, KEY_RIGHT, 90);
    test_check("pause freezes stage clock", world.stage_frame == sm.sf, "%d vs %d", sm.sf, world.stage_frame);
    test_check("pause ignores movement input", px() == sm.x0, "%d vs %d", sm.x0, px());
    test_check("music paused while paused", ! audio_music_playing(), NULL);
    test_shot("smoke_04_paused");
    PRESS(t, KEY_START);
    T_WAIT(t, 10);
    test_check("START resumes", app_state() == STATE_PLAYING, "%s", state_names[app_state()]);
    test_check("stage clock runs again", world.stage_frame > sm.sf, NULL);
    test_check("stage music resumes", audio_music_playing(), NULL);

    /* movement */
    sm.x0 = px();
    sm.y0 = py();
    HOLD(t, KEY_RIGHT, 30);
    test_check("RIGHT moves ship", px() > sm.x0, "%d -> %d", sm.x0, px());
    HOLD(t, KEY_DOWN, 20);
    test_check("DOWN moves ship", py() > sm.y0, "%d -> %d", sm.y0, py());

    /* screen bounds (only meaningful while the ship is alive) */
    HOLD(t, KEY_UP, 150);
    test_check("ship stays below HUD", py() >= -64, "%d", py());
    HOLD(t, KEY_LEFT, 200);
    test_check("ship stays inside left edge", px() >= -112, "%d", px());
    HOLD(t, KEY_DOWN, 200);
    test_check("ship stays above status line", py() <= 64, "%d", py());
    HOLD(t, KEY_RIGHT, 250);
    test_check("ship stays inside right edge", px() <= 112, "%d", px());

    /* combat: sweep up and down while firing; enemies spawn and are destroyed */
    HOLD(t, KEY_LEFT, 150);
    test_key_down(KEY_A);
    sm.dir = KEY_UP;

    for(sm.loop = 1; sm.loop <= 14; ++sm.loop)
    {
        HOLD(t, sm.dir, 40);
        sm.dir = sm.dir == KEY_UP ? KEY_DOWN : KEY_UP;
        T_WAIT(t, 20);

        if(sm.loop == 7)
        {
            test_shot("smoke_05_combat");
        }
    }

    test_key_up(KEY_A);
    test_check("enemies spawned", sm.max_enemies > 0, "max on screen %d", sm.max_enemies);
    test_check("score increased (enemies destroyed)", game.score > 0, "%d", game.score);
    test_check("no gameplay frame missed", gameplay_missed == 0, "%u", (unsigned) gameplay_missed);
    test_log("deaths during smoke run: %d", game.deaths);
    T_END(t);
}

/* =================================================================================================== */
/* death: respawn, invulnerability, lives, game over, back to title, new game                        */
/* =================================================================================================== */

static struct
{
    test_thread t;
    int lives0;
} de;

bool suite_death(void)
{
    test_thread* t = &de.t;
    T_BEGIN(t);
    app_start_game(1);
    T_WAIT(t, 10);
    test_check("started on stage 2", app_state() == STATE_PLAYING && game.stage == 1, "%s", snapshot());

    /* Wait (invincible, so stray enemies can't interfere) for the cave ceiling to grow. */
    test_ctl.invincible = true;
    T_WAIT(t, 900);
    test_ctl.invincible = false;
    T_WAIT(t, 2);
    de.lives0 = game.lives;
    test_check("3 lives at start", de.lives0 == 3, "%d", de.lives0);

    test_key_down(KEY_UP);
    T_WAIT_UNTIL(t, ! player_alive(), 300);
    test_check("touching terrain destroys the ship", ! player_alive(), "%s", snapshot());
    test_check("a life is lost", game.lives == de.lives0 - 1, "%d", game.lives);
    T_WAIT(t, 20);
    test_shot("death_01_explosion");
    test_key_up(KEY_UP);

    T_WAIT_UNTIL(t, player_alive(), 200);
    test_check("ship respawns", player_alive(), "%s", snapshot());
    T_WAIT(t, 10);
    test_shot("death_02_respawn");

    /* Invulnerable right after respawn: flying into the ceiling must not kill immediately. */
    de.lives0 = game.lives;
    test_key_down(KEY_UP);
    T_WAIT(t, 60);
    test_check("respawn invulnerability", player_alive() && game.lives == de.lives0, "%s", snapshot());

    /* Lose the remaining ships. */
    T_WAIT_UNTIL(t, app_state() == STATE_GAME_OVER, 1500);
    test_key_up(KEY_UP);
    test_check("losing all lives -> GAME_OVER", app_state() == STATE_GAME_OVER, "%s", snapshot());
    test_check("deaths counted", game.deaths == 3, "%d", game.deaths);
    T_WAIT(t, 100);
    test_shot("death_03_game_over");

    PRESS(t, KEY_START);
    T_WAIT(t, 30);
    test_check("START on game over -> TITLE", app_state() == STATE_TITLE, "%s", state_names[app_state()]);
    test_shot("death_04_title");

    PRESS(t, KEY_START);
    T_WAIT(t, 30);
    test_check("new game after game over", app_state() == STATE_PLAYING && game.lives == 3 && game.score == 0,
               "%s", snapshot());
    T_END(t);
}

/* =================================================================================================== */
/* power: every step of the ladder, the shockwave, reset on death                                    */
/* =================================================================================================== */

/* Expected loadout per step: main gun, missile mode, additional shooters. */
static const u8 expect[9][3] = {
    { 0, 0, 0 }, { 0, 0, 0 }, { 0, 1, 0 }, { 1, 1, 0 }, { 1, 1, 0 }, { 2, 1, 0 }, { 2, 1, 1 }, { 2, 1, 2 }, { 2, 2, 2 },
};

static const char* const step_names[10] = { "normal", "homing_dot", "missile", "laser", "shield", "spread_laser",
                                            "shooter_1", "shooter_2", "homing_missile", "shockwave" };

static struct
{
    test_thread t;
    int step;
    int i;
    int most;
    int waves0;
} pw;

bool suite_power(void)
{
    test_thread* t = &pw.t;
    T_BEGIN(t);
    app_start_game(1);
    T_WAIT(t, 60);
    test_check("started on stage 2 at power 0", game.stage == 1 && game.gear.power == 0, "%s", power_line());
    test_ctl.invincible = true;

    for(pw.step = 0; pw.step <= 8; ++pw.step)
    {
        test_ctl.set_power = pw.step + 1;
        T_WAIT(t, 3);

        {
            int p = game.gear.power;
            char name[48];
            snprintf(name, sizeof(name), "step %d %s loadout", pw.step, step_names[pw.step]);
            test_check(name, p == pw.step && (int) main_gun_of(p) == expect[pw.step][0] &&
                       (int) missile_mode_of(p) == expect[pw.step][1] && shooter_count_of(p) == expect[pw.step][2],
                       "%s", power_line());
        }

        /* Fire for a while; every step must put shots on screen. */
        test_key_down(KEY_A);
        pw.most = 0;

        for(pw.i = 0; pw.i < 40; ++pw.i)
        {
            T_WAIT(t, 1);
            pw.most = MAX(pw.most, shots_count());
        }

        test_key_up(KEY_A);

        {
            char name[32];
            snprintf(name, sizeof(name), "step %d fires", pw.step);
            test_check(name, pw.most > 0, "max shots %d", pw.most);
        }

        if(pw.step == 5 || pw.step == 7)
        {
            test_key_down(KEY_A);
            T_WAIT(t, 12);
            test_shot("power_step%d_%s", pw.step, step_names[pw.step]);
            test_key_up(KEY_A);
        }

        if(pw.step == 7)
        {
            test_check("spread laser from ship + 2 shooters fills the screen", pw.most >= 9, "%d", pw.most);
        }

        T_WAIT(t, 30);
    }

    test_check("shield granted at the SHIELD step", game.gear.shield == 3, "%s", power_line());

    /* Shockwave: the first one fires as soon as the step is reached, then every 600 frames. */
    pw.waves0 = world.shockwaves;
    test_ctl.set_power = 10;
    T_WAIT_UNTIL(t, world.shockwaves > pw.waves0, 10);
    test_check("reaching SHOCKWAVE fires one immediately", world.shockwaves > pw.waves0, "%s", power_line());
    T_WAIT(t, 1);
    test_shot("power_shockwave_flash");
    test_check("shockwave clears enemies and bullets", enemies_count() == 0 && bullets_count() == 0, "%s", snapshot());
    T_WAIT_UNTIL(t, world.shockwaves > pw.waves0 + 1, 620);
    test_check("shockwave repeats periodically (600 frames)", world.shockwaves > pw.waves0 + 1, "%s", power_line());

    /* Losing a ship resets the ladder. Drop to the laser (shield kept) and fly into the ceiling. */
    test_ctl.set_power = 4;
    T_WAIT(t, 3);
    test_check("power can go back down (test hook)", game.gear.power == 3 && shooter_count_of(game.gear.power) == 0,
               "%s", power_line());
    test_ctl.invincible = false;
    T_WAIT(t, 40);
    test_key_down(KEY_UP);
    T_WAIT_UNTIL(t, ! player_alive(), 400);
    test_key_up(KEY_UP);
    test_check("ship destroyed", ! player_alive(), "%s", snapshot());
    test_check("death resets power to the normal shot", game.gear.power == 0 && game.gear.shield == 0, "%s",
               power_line());
    T_END(t);
}

/* =================================================================================================== */
/* full_run: bot plays all three stages to the ending (invincible + autofire); performance stats     */
/* =================================================================================================== */

static struct
{
    test_thread t;
    int stage;
    int hp0;
    int cpu_max;
    u32 cpu_sum;
    u32 cpu_frames;
    int sprites_max;
    int bullets_max;
    int enemies_max;
    int shots_max;
    int powerups_max;
    int power_max;
    int heavy_logged;
    u32 prof_sum[PROF_COUNT];
    u16 prof_max[PROF_COUNT];
} fr;

static const char* const prof_names[PROF_COUNT] = { "stage", "player", "shots", "enemies", "boss", "bullets",
                                                    "powerups", "collide", "effects", "terrain", "bgs", "hud",
                                                    "render" };

static void full_run_bot(void)
{
    test_ctl.invincible = true;
    test_ctl.autofire = true;

    if(app_state() != STATE_PLAYING)
    {
        test_key_up(KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT);
        return;
    }

    int ty = boss_active() ? fx_round(boss_position().y) : fx_trunc(direction(world.stage_frame * 149, FX(40)).y);
    int y = py();
    int x = px();

    test_key_up(KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT);

    if(y < ty - 2)
    {
        test_key_down(KEY_DOWN);
    }
    else if(y > ty + 2)
    {
        test_key_down(KEY_UP);
    }

    if(x < -75)
    {
        test_key_down(KEY_RIGHT);
    }
    else if(x > -65)
    {
        test_key_down(KEY_LEFT);
    }
}

static void full_run_track(void)
{
    track_missed();

    if(app_state() != STATE_PLAYING || world.stage_frame <= 3)
    {
        return;
    }

    /* cpu_pct and prof_permille both describe the previous frame (measured at its end). */
    int cpu = frame_info.cpu_pct;

    if(cpu >= 80 && fr.heavy_logged < 20)
    {
        ++fr.heavy_logged;
        test_log("heavy frame: %s | collide %d shots %d enemies %d boss %d bullets %d render %d (1/1000)", snapshot(),
                 prof_permille[PROF_COLLIDE], prof_permille[PROF_SHOTS], prof_permille[PROF_ENEMIES],
                 prof_permille[PROF_BOSS], prof_permille[PROF_BULLETS], prof_permille[PROF_RENDER]);
    }

    fr.cpu_max = MAX(fr.cpu_max, cpu);
    fr.cpu_sum += (u32) cpu;
    ++fr.cpu_frames;
    fr.sprites_max = MAX(fr.sprites_max, sprites_used());
    fr.bullets_max = MAX(fr.bullets_max, bullets_count());
    fr.enemies_max = MAX(fr.enemies_max, enemies_count());
    fr.shots_max = MAX(fr.shots_max, shots_count());
    fr.powerups_max = MAX(fr.powerups_max, powerups_count());
    fr.power_max = MAX(fr.power_max, game.gear.power);

    for(int i = 0; i < PROF_COUNT; ++i)
    {
        fr.prof_sum[i] += prof_permille[i];
        fr.prof_max[i] = MAX(fr.prof_max[i], prof_permille[i]);
    }
}

bool suite_full_run(void)
{
    test_thread* t = &fr.t;
    T_BEGIN(t);
    PRESS(t, KEY_START);
    T_WAIT(t, 10);
    test_check("game started", app_state() == STATE_PLAYING, "%s", snapshot());
    reset_missed();
    test_set_hooks(full_run_bot, full_run_track);

    for(fr.stage = 0; fr.stage < 3; ++fr.stage)
    {
        {
            char name[32];
            snprintf(name, sizeof(name), "stage %d running", fr.stage + 1);
            test_check(name, game.stage == fr.stage && app_state() == STATE_PLAYING, "%s", snapshot());
        }

        T_WAIT(t, 1500);
        test_shot("full_run_stage%d_a", fr.stage + 1);
        T_WAIT(t, 1500);
        test_shot("full_run_stage%d_b", fr.stage + 1);

        T_WAIT_UNTIL(t, boss_active(), 4000);
        {
            char name[32];
            snprintf(name, sizeof(name), "stage %d boss appears", fr.stage + 1);
            test_check(name, boss_active(), "%s", snapshot());
        }

        T_WAIT(t, 200);
        test_shot("full_run_stage%d_boss", fr.stage + 1);
        fr.hp0 = boss_hp();
        {
            char name[32];
            snprintf(name, sizeof(name), "stage %d boss has HP", fr.stage + 1);
            test_check(name, fr.hp0 > 0, "%d", fr.hp0);
        }

        T_WAIT(t, 400);
        {
            char name[40];
            snprintf(name, sizeof(name), "stage %d boss takes damage", fr.stage + 1);
            test_check(name, boss_hp() < fr.hp0, "%d -> %d", fr.hp0, boss_hp());
        }

        T_WAIT(t, 300);
        test_shot("full_run_stage%d_boss_late", fr.stage + 1);

        T_WAIT_UNTIL(t, app_state() == STATE_STAGE_CLEAR || app_state() == STATE_ENDING, 9000);
        {
            char name[32];
            snprintf(name, sizeof(name), "stage %d cleared", fr.stage + 1);
            test_check(name, app_state() == STATE_STAGE_CLEAR || app_state() == STATE_ENDING, "%s", snapshot());
        }

        test_log("stage %d power at clear: %s", fr.stage + 1, power_line());
        T_WAIT(t, 30);
        test_shot("full_run_stage%d_clear", fr.stage + 1);

        if(fr.stage < 2)
        {
            T_WAIT_UNTIL(t, app_state() == STATE_PLAYING && game.stage == fr.stage + 1, 900);
            {
                char name[32];
                snprintf(name, sizeof(name), "advances to stage %d", fr.stage + 2);
                test_check(name, app_state() == STATE_PLAYING && game.stage == fr.stage + 1, "%s", snapshot());
            }

            T_WAIT(t, 60);
            test_shot("full_run_stage%d_start", fr.stage + 2);
        }
    }

    T_WAIT_UNTIL(t, app_state() == STATE_ENDING, 900);
    test_check("final boss leads to ENDING", app_state() == STATE_ENDING, "%s", snapshot());
    T_WAIT(t, 120);
    test_shot("full_run_ending_1");
    PRESS(t, KEY_START);
    T_WAIT(t, 120);
    test_shot("full_run_ending_2");
    PRESS(t, KEY_START);
    T_WAIT(t, 120);
    test_shot("full_run_ending_3");
    PRESS(t, KEY_START);
    T_WAIT(t, 30);
    test_check("ending returns to TITLE", app_state() == STATE_TITLE, "%s", state_names[app_state()]);
    test_check("hi-score updated", game.hiscore >= game.score && game.hiscore > DEFAULT_HISCORE, "%d", game.hiscore);
    test_shot("full_run_title_after");

    test_log("max cpu %d%%, max sprites %d, max enemy bullets %d, max enemies %d, max shots %d", fr.cpu_max,
             fr.sprites_max, fr.bullets_max, fr.enemies_max, fr.shots_max);
    test_log("average cpu %d.%d%% over %u gameplay frames", (int) (fr.cpu_sum / MAX(1u, fr.cpu_frames)),
             (int) ((fr.cpu_sum * 10 / MAX(1u, fr.cpu_frames)) % 10), (unsigned) fr.cpu_frames);

    {
        char line[400];
        int n = snprintf(line, sizeof(line), "system cost 1/1000 frame (avg/max):");

        for(int i = 0; i < PROF_COUNT && n < (int) sizeof(line) - 24; ++i)
        {
            n += snprintf(line + n, sizeof(line) - (size_t) n, " %s %u/%u", prof_names[i],
                          (unsigned) (fr.prof_sum[i] / MAX(1u, fr.cpu_frames)), (unsigned) fr.prof_max[i]);
        }

        test_log("%s", line);
    }

    test_log("pool drops: shots %d, enemies %d, bullets %d, sprites over OAM %d", shots_dropped(), enemies_dropped(),
             bullets_dropped(), sprites_dropped());
    test_check("CPU budget: worst frame under 100%", fr.cpu_max < 100, "%d%%", fr.cpu_max);
    test_check("no missed frames", gameplay_missed == 0, "%u", (unsigned) gameplay_missed);
    test_check("sprite budget respected (<=128)", fr.sprites_max <= 128 && sprites_dropped() == 0, "%d", fr.sprites_max);
    test_check("power capsules dropped by destroyed enemies", fr.powerups_max > 0, "%d", fr.powerups_max);
    test_check("power ladder advanced by collecting capsules", fr.power_max >= 3, "%d", fr.power_max);
    T_END(t);
}

/* =================================================================================================== */
/* save_check (second boot): the high score from the first boot came back from SRAM                  */
/* =================================================================================================== */

static struct
{
    test_thread t;
} sc;

bool suite_save_check(void)
{
    test_thread* t = &sc.t;
    T_BEGIN(t);
    T_WAIT(t, 90);
    test_check("boots to TITLE", app_state() == STATE_TITLE, "%s", state_names[app_state()]);
    test_check("hi-score loaded from SRAM after restart", game.hiscore > DEFAULT_HISCORE, "%d", game.hiscore);
    test_check("hi-score matches previous run", game.hiscore == test_expected_hiscore, "%d vs %d", game.hiscore,
               test_expected_hiscore);
    test_shot("save_check_title");
    T_END(t);
}
