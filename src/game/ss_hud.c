#include "ss_hud.h"

#include "gen_gfx.h"
#include "ss_boss.h"
#include "ss_bullets.h"
#include "ss_enemies.h"
#include "ss_frame.h"
#include "ss_power_data.h"
#include "ss_shots.h"
#include "ss_sprites.h"
#include "ss_text.h"
#include "ss_weapon_data.h"
#include "ss_world.h"

/* Text rows (8 px each). */
#define ROW_TOP 0
#define ROW_BOSS 2
#define ROW_DEBUG 3
#define ROW_BANNER 7
#define ROW_BANNER_SUB 9
#define ROW_PICKUP 17
#define ROW_STATUS 19

static struct
{
    bool visible;
    bool debug;
    int shown_score;
    int shown_hiscore;
    int shown_lives;
    int shown_status;
    bool boss_label;
    int banner_timer;
    int warning_timer;
    int pickup_timer;
    const char* pending_pickup;
} h;

void hud_init(void)
{
    h.visible = true;
    h.debug = false;
    h.shown_score = -1;
    h.shown_hiscore = -1;
    h.shown_lives = -1;
    h.shown_status = -1;
    h.boss_label = false;
    h.banner_timer = 0;
    h.warning_timer = 0;
    h.pickup_timer = 0;
    h.pending_pickup = NULL;
    text_clear_all();
}

void hud_set_visible(bool visible)
{
    h.visible = visible;

    if(! visible)
    {
        text_clear_all();
    }

    /* force a redraw when shown again */
    h.shown_score = -1;
    h.shown_status = -1;
    h.boss_label = false;
}

void hud_toggle_debug(void)
{
    h.debug = ! h.debug;
    text_clear_row(ROW_DEBUG);
}

void hud_show_banner(const char* title, const char* subtitle)
{
    text_clear_row(ROW_BANNER);
    text_clear_row(ROW_BANNER_SUB);
    text_center(ROW_BANNER, title, TEXT_YELLOW);
    text_center(ROW_BANNER_SUB, subtitle, TEXT_WHITE);
    h.banner_timer = 150;
    h.warning_timer = 0;
}

void hud_show_warning(void)
{
    h.warning_timer = 180;
    h.banner_timer = 0;
}

void hud_show_pickup(const char* label)
{
    h.pending_pickup = label;
}

/* ----- text items ------------------------------------------------------------------------------- */

static bool update_score(void)
{
    /* Redraw at most every 4th frame (15 Hz is plenty for a score counter). */
    bool refresh = (world.stage_frame & 3) == 0 || h.shown_score < 0;

    if(! refresh || (game.score == h.shown_score && game.hiscore == h.shown_hiscore && game.lives == h.shown_lives))
    {
        return false;
    }

    h.shown_score = game.score;
    h.shown_hiscore = game.hiscore;
    h.shown_lives = game.lives;

    char line[24];
    char* end = text_append(line, "SC ");
    text_append_number(end, game.score, 8);
    text_clear_row(ROW_TOP);
    text_print(ROW_TOP, 2, line, TEXT_WHITE);

    end = text_append(line, "HI ");
    text_append_number(end, game.hiscore, 8);
    text_print(ROW_TOP, 82, line, TEXT_YELLOW);

    end = text_append(line, "x");
    text_append_number(end, game.lives, 1);
    text_print(ROW_TOP, 224, line, TEXT_WHITE);
    return true;
}

/* e.g. "P9 S.LASER x3 DOT HMSL SHLD3 WAVE": power step, gun, gun count (ship + shooters), extras. */
static bool update_status(void)
{
    int power = game.gear.power;
    int status = power | (game.gear.shield << 4);

    if(status == h.shown_status)
    {
        return false;
    }

    h.shown_status = status;
    char line[48];
    char* end = text_append(line, "P");
    end = text_append_number(end, power, 1);
    end = text_append(end, " ");
    end = text_append(end, gun_defs[main_gun_of(power)].hud_name);

    if(shooter_count_of(power))
    {
        end = text_append(end, " x");
        end = text_append_number(end, shooter_count_of(power) + 1, 1);
    }

    if(has_power(power, POWER_HOMING_DOT))
    {
        end = text_append(end, " DOT");
    }

    missile_mode missiles = missile_mode_of(power);

    if(missiles != MISSILES_NONE)
    {
        end = text_append(end, missiles == MISSILES_HOMING ? " HMSL" : " MSL");
    }

    if(game.gear.shield)
    {
        end = text_append(end, " SHLD");
        end = text_append_number(end, game.gear.shield, 1);
    }

    if(has_power(power, POWER_SHOCKWAVE))
    {
        end = text_append(end, " WAVE");
    }

    text_clear_row(ROW_STATUS);
    text_print(ROW_STATUS, 2, line, TEXT_CYAN);
    return true;
}

static void update_debug(void)
{
    if(! h.debug || (world.stage_frame % 15) != 0)
    {
        return;
    }

    char line[48];
    char* end = text_append(line, "CPU");
    end = text_append_number(end, frame_info.cpu_pct, 2);
    end = text_append(end, " E");
    end = text_append_number(end, enemies_count(), 2);
    end = text_append(end, " B");
    end = text_append_number(end, bullets_count(), 2);
    end = text_append(end, " S");
    end = text_append_number(end, shots_count(), 2);
    end = text_append(end, " SPR");
    end = text_append_number(end, sprites_used(), 3);
    end = text_append(end, " F");
    text_append_number(end, world.stage_frame, 4);
    text_clear_row(ROW_DEBUG);
    text_print(ROW_DEBUG, 2, line, TEXT_YELLOW);
}

void hud_update(void)
{
    if(h.banner_timer && --h.banner_timer == 0)
    {
        text_clear_row(ROW_BANNER);
        text_clear_row(ROW_BANNER_SUB);
    }

    if(h.warning_timer)
    {
        /* blink the warning every 12 frames */
        --h.warning_timer;

        if(h.warning_timer % 12 == 11)
        {
            text_clear_row(ROW_BANNER);
            text_clear_row(ROW_BANNER_SUB);

            if((h.warning_timer / 12) % 2 == 0 && h.warning_timer)
            {
                text_center(ROW_BANNER, "! WARNING !", TEXT_RED);
                text_center(ROW_BANNER_SUB, "A HUGE ENEMY APPROACHES", TEXT_WHITE);
            }
        }

        if(! h.warning_timer)
        {
            text_clear_row(ROW_BANNER);
            text_clear_row(ROW_BANNER_SUB);
        }
    }

    if(h.pickup_timer && --h.pickup_timer == 0)
    {
        text_clear_row(ROW_PICKUP);
    }

    if(! h.visible)
    {
        return;
    }

    /* At most one text line is rendered per frame. */
    if(h.pending_pickup)
    {
        text_clear_row(ROW_PICKUP);
        text_center(ROW_PICKUP, h.pending_pickup, TEXT_CYAN);
        h.pickup_timer = 60;
        h.pending_pickup = NULL;
    }
    else if(! update_status())
    {
        update_score();
    }

    bool boss = boss_active() && boss_hp_max() > 0;

    if(boss != h.boss_label)
    {
        h.boss_label = boss;
        text_clear_row(ROW_BOSS);

        if(boss)
        {
            text_print(ROW_BOSS, 2, "BOSS", TEXT_RED);
        }
    }

    update_debug();
}

void hud_render(void)
{
    if(! h.visible)
    {
        return;
    }

    sprites_draw_px(GEN_SPR_LIFE_ICON, 0, 96, -76, SPR_HUD);

    if(boss_active() && boss_hp_max() > 0)
    {
        /* 128 px bar, 4 segments of 32 px, each frame shows 0..32 px in 2 px steps. */
        int filled = (boss_hp() * 128) / boss_hp_max();

        for(int i = 0; i < 4; ++i)
        {
            int pixels = SS_CLAMP(filled - i * 32, 0, 32);
            sprites_draw_px(GEN_SPR_BOSS_BAR, pixels / 2, -56 + i * 32, -60, SPR_HUD);
        }
    }
}
