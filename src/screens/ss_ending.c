#include "ss_screens.h"

#include "ss_audio.h"
#include "ss_input.h"
#include "ss_text.h"
#include "ss_video.h"
#include "ss_world.h"

#define PAGE_FRAMES 420
#define FADE_FRAMES 30
#define PAGE_COUNT 3

static struct
{
    int page;
    int timer;
} e;

static void show_page(int page)
{
    e.page = page;
    e.timer = 0;
    text_clear_all();

    switch(page)
    {
    case 0:
        text_center(4, "THE OVERMIND IS DESTROYED.", TEXT_WHITE);
        text_center(6, "THE DREADNOUGHT BREAKS APART", TEXT_WHITE);
        text_center(7, "AND FALLS INTO THE RED STAR.", TEXT_WHITE);
        text_center(10, "THE OUTER COLONIES ARE SAFE.", TEXT_CYAN);
        text_center(12, "...FOR NOW.", TEXT_YELLOW);
        break;

    case 1:
        text_center(3, "SPACE SHOOTER", TEXT_YELLOW);
        text_center(5, "GAME DESIGN, CODE, PIXEL ART", TEXT_WHITE);
        text_center(6, "MUSIC AND SOUND EFFECTS", TEXT_WHITE);
        text_center(7, "ALL GENERATED FROM C SOURCE", TEXT_WHITE);
        text_center(10, "MADE WITH LIBTONC + MAXMOD", TEXT_CYAN);
        text_center(11, "TESTED IN MGBA", TEXT_CYAN);
        break;

    default:
        {
            char line[32];
            text_center(3, "MISSION COMPLETE", TEXT_YELLOW);
            text_append_number(text_append(line, "FINAL SCORE "), game.score, 8);
            text_center(6, line, TEXT_WHITE);
            text_append_number(text_append(line, "HI-SCORE    "), game.hiscore, 8);
            text_center(7, line, TEXT_WHITE);
            text_append_number(text_append(line, "SHIPS LOST  "), game.deaths, 1);
            text_center(9, line, TEXT_WHITE);
            text_center(12, "THANK YOU FOR PLAYING!", TEXT_CYAN);
            text_center(15, "PRESS START", TEXT_YELLOW);
        }
        break;
    }
}

void ending_init(void)
{
    video_show_backgrounds(BACKDROP_NONE);
    audio_play_music(MUSIC_ENDING);
    show_page(0);
}

bool ending_update(void)
{
    ++e.timer;
    video_set_scroll(3, (e.timer + e.page * PAGE_FRAMES) / 3, 48);

    /* Fade each page's text in and out (the last page stays on). */
    int fade = 0;

    if(e.timer < FADE_FRAMES)
    {
        fade = (FADE_FRAMES - e.timer) * 16 / FADE_FRAMES;
    }
    else if(e.page < PAGE_COUNT - 1 && e.timer > PAGE_FRAMES - FADE_FRAMES)
    {
        fade = (e.timer - (PAGE_FRAMES - FADE_FRAMES)) * 16 / FADE_FRAMES;
    }

    video_set_fade(fade, false, FADE_BG0);

    if(e.page < PAGE_COUNT - 1)
    {
        if(e.timer >= PAGE_FRAMES || (input_pressed(KEY_START) && e.timer > FADE_FRAMES))
        {
            show_page(e.page + 1);
        }

        return false;
    }

    if(input_pressed(KEY_START) && e.timer > 60)
    {
        video_set_fade(0, false, 0);
        return true;
    }

    return false;
}
