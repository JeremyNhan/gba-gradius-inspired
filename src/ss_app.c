#include "ss_app.h"

#include "ss_audio.h"
#include "ss_frame.h"
#include "ss_hud.h"
#include "ss_input.h"
#include "ss_player.h"
#include "ss_save.h"
#include "ss_screens.h"
#include "ss_text.h"
#include "ss_video.h"
#include "ss_world.h"

#define FADE_FRAMES 16

static struct
{
    game_state state;
    int timer;
    int fade_in;            /* frames left of the fade-in after a screen change */
    int fade_out;           /* frames of the stage-clear fade-out done so far */
    bool final_clear;
    bool overlay_shown;
    int saved_hiscore;
} a;

static void app_enter(game_state next);

game_state app_state(void)
{
    return a.state;
}

void app_init(void)
{
    a.saved_hiscore = save_load_hiscore();
    game.hiscore = a.saved_hiscore;
    a.state = STATE_TITLE;
    app_enter(STATE_TITLE);
}

static void save_hiscore(void)
{
    if(game.hiscore > a.saved_hiscore)
    {
        save_store_hiscore(game.hiscore);
        a.saved_hiscore = game.hiscore;
    }
}

/* ----- state handlers ----------------------------------------------------------------------------- */

static game_state update_title(void)
{
    return title_update() ? STATE_PLAYING : STATE_TITLE;
}

static game_state update_playing(void)
{
    if(input_pressed(KEY_START))
    {
        return STATE_PAUSED;
    }

    switch(world_update())
    {
    case WORLD_STAGE_CLEARED:
        a.final_clear = false;
        return STATE_STAGE_CLEAR;

    case WORLD_GAME_COMPLETE:
        a.final_clear = true;
        return STATE_STAGE_CLEAR;

    case WORLD_GAME_OVER:
        return STATE_GAME_OVER;

    default:
        return STATE_PLAYING;
    }
}

static game_state update_paused(void)
{
    /* Gameplay is frozen: the world is not updated at all while paused. */
    ++a.timer;
    bool show = ((a.timer / 20) & 1) == 0;

    if(show != a.overlay_shown)
    {
        a.overlay_shown = show;
        text_clear_row(8);
        text_clear_row(10);

        if(show)
        {
            text_center(8, "PAUSED", TEXT_YELLOW);
            text_center(10, "PRESS START", TEXT_WHITE);
        }
    }

    return input_pressed(KEY_START) ? STATE_PLAYING : STATE_PAUSED;
}

static game_state update_stage_clear(void)
{
    ++a.timer;
    bool outro_done = world_update_outro();

    if(a.timer >= 300 && outro_done)
    {
        /* Fade to black before loading the next screen. */
        if(a.fade_out < FADE_FRAMES)
        {
            ++a.fade_out;
            video_set_fade(a.fade_out, false, FADE_ALL);
            return STATE_STAGE_CLEAR;
        }

        if(a.final_clear)
        {
            return STATE_ENDING;
        }

        ++game.stage;
        return STATE_PLAYING;
    }

    return STATE_STAGE_CLEAR;
}

static game_state update_game_over(void)
{
    ++a.timer;
    return (a.timer > 90 && input_pressed(KEY_START)) || a.timer > 900 ? STATE_TITLE : STATE_GAME_OVER;
}

static game_state update_ending(void)
{
    return ending_update() ? STATE_TITLE : STATE_ENDING;
}

/* ----- transitions -------------------------------------------------------------------------------- */

static void start_fade_in(void)
{
    a.fade_in = FADE_FRAMES + 1;
    video_set_fade(16, false, FADE_ALL);
}

static void app_enter(game_state next)
{
    game_state previous = a.state;
    a.state = next;
    a.timer = 0;
    a.fade_out = 0;
    a.overlay_shown = false;

    switch(next)
    {
    case STATE_TITLE:
        save_hiscore();
        world_release();
        start_fade_in();
        title_init(game.hiscore);
        break;

    case STATE_PLAYING:
        if(previous == STATE_PAUSED)
        {
            text_clear_row(8);
            text_clear_row(10);
            audio_resume_music();
            audio_play(SFX_PAUSE_ID);
            break;
        }

        if(previous == STATE_TITLE)
        {
            session_new_game(SS_DEBUG ? title_selected_stage() : 0);
        }

        start_fade_in();
        world_init();
        break;

    case STATE_PAUSED:
        audio_pause_music();
        audio_play(SFX_PAUSE_ID);
        break;

    case STATE_STAGE_CLEAR:
        {
            int bonus = 5000 * (game.stage + 1) + 1000 * game.lives;
            game_add_score(bonus);
            player_start_outro();
            audio_play_music(MUSIC_JINGLE_CLEAR);

            char line[32];
            char* end = text_append(line, a.final_clear ? "MISSION COMPLETE" : "STAGE ");

            if(! a.final_clear)
            {
                end = text_append_number(end, game.stage + 1, 1);
                text_append(end, " CLEAR!");
            }

            text_clear_row(6);
            text_center(6, line, TEXT_YELLOW);
            text_append_number(text_append(line, "BONUS "), bonus, 1);
            text_clear_row(8);
            text_center(8, line, TEXT_WHITE);
        }
        break;

    case STATE_GAME_OVER:
        {
            audio_play_music(MUSIC_JINGLE_GAME_OVER);
            bool record = game.score > a.saved_hiscore && game.score >= game.hiscore;
            save_hiscore();
            video_set_fade(8, false, BLD_BG1 | BLD_BG2 | BLD_BG3 | BLD_BACKDROP);
            hud_set_visible(false);

            char line[32];
            text_center(6, "GAME OVER", TEXT_RED);
            text_append_number(text_append(line, "SCORE "), game.score, 8);
            text_center(9, line, TEXT_WHITE);

            if(record)
            {
                text_center(11, "NEW HI-SCORE!", TEXT_YELLOW);
            }

            text_center(13, "PRESS START", TEXT_CYAN);
        }
        break;

    case STATE_ENDING:
        /* The ending fades its own text layer in (one brightness register for the whole screen). */
        save_hiscore();
        world_release();
        a.fade_in = 0;
        ending_init();
        break;

    default:
        break;
    }
}

void app_update(void)
{
    game_state next = a.state;

    switch(a.state)
    {
    case STATE_TITLE:
        next = update_title();
        break;

    case STATE_PLAYING:
        next = update_playing();
        break;

    case STATE_PAUSED:
        next = update_paused();
        break;

    case STATE_STAGE_CLEAR:
        next = update_stage_clear();
        break;

    case STATE_GAME_OVER:
        next = update_game_over();
        break;

    case STATE_ENDING:
        next = update_ending();
        break;
    }

    if(next != a.state)
    {
        app_enter(next);
    }

    if(a.fade_in > 0)
    {
        --a.fade_in;
        video_set_fade(a.fade_in * 16 / FADE_FRAMES, false, FADE_ALL);
    }

    /* Sprites for this frame. */
    if(a.state == STATE_TITLE)
    {
        title_render();
    }
    else if(a.state != STATE_ENDING)
    {
        world_render();
    }
}

#if SS_TESTS
void app_start_game(int stage)
{
    session_new_game(stage);
    a.state = STATE_PLAYING;
    a.timer = 0;
    start_fade_in();
    world_init();
}

void app_go_to_title(void)
{
    app_enter(STATE_TITLE);
}
#endif
