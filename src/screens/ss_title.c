#include "ss_screens.h"

#include "gen_gfx.h"
#include "ss_audio.h"
#include "ss_input.h"
#include "ss_sprites.h"
#include "ss_text.h"
#include "ss_video.h"

#define ROW_PRESS_START 13

static struct
{
    int timer;
    int stage;
    fx ship_x;
    bool press_start_shown;
} t;

static void draw_stage_select(void)
{
    char label[24];
    char* end = text_append(label, "DEBUG STAGE ");
    text_append_number(end, t.stage + 1, 1);
    text_clear_row(0);
    text_print(0, 2, label, TEXT_RED);
}

void title_init(int hiscore)
{
    t.timer = 0;
    t.stage = 0;
    t.ship_x = FX(-140);
    t.press_start_shown = false;

    video_show_backgrounds(BACKDROP_TITLE_LOGO);
    video_set_scroll(1, 8, 48);         /* logo drawn with a (8, 48) offset in its 256x256 map */
    video_set_scroll(3, 0, 48);

    text_clear_all();
    char line[40];
    char* end = text_append(line, "HI-SCORE ");
    text_append_number(end, hiscore, 8);
    text_center(15, line, TEXT_WHITE);
    text_center(17, "A:SHOT  B:CHARGE  START:PAUSE", TEXT_CYAN);
    text_center(19, GAME_VERSION "  (C) 2026 HOMEBREW", TEXT_WHITE);

    if(SS_DEBUG)
    {
        draw_stage_select();
    }

    audio_play_music(MUSIC_TITLE);
}

int title_selected_stage(void)
{
    return t.stage;
}

bool title_update(void)
{
    ++t.timer;
    video_set_scroll(3, t.timer / 4, 48);

    bool show = (t.timer / 30) % 2 == 0;

    if(show != t.press_start_shown)
    {
        t.press_start_shown = show;
        text_clear_row(ROW_PRESS_START);

        if(show)
        {
            text_center(ROW_PRESS_START, "PRESS START", TEXT_YELLOW);
        }
    }

    t.ship_x += FX_F(0.75);

    if(t.ship_x > FX(140))
    {
        t.ship_x = FX(-140);
    }

    if(SS_DEBUG)
    {
        if(input_pressed(KEY_R))
        {
            t.stage = (t.stage + 1) % STAGE_COUNT;
            draw_stage_select();
        }
        else if(input_pressed(KEY_L))
        {
            t.stage = (t.stage + STAGE_COUNT - 1) % STAGE_COUNT;
            draw_stage_select();
        }
    }

    if(input_pressed(KEY_START) && t.timer > 20)
    {
        audio_play(SFX_SELECT_ID);
        return true;
    }

    return false;
}

void title_render(void)
{
    /* A small ship cruising under the logo. */
    vec2 pos = v2(t.ship_x, FX(12) + direction(t.timer * 300, FX(3)).y);
    sprites_draw(GEN_SPR_PLAYER, (t.timer >> 2) & 1, pos, 0);
}
