#include "ss_app.h"

#include "bn_bg_maps.h"
#include "bn_bg_palettes.h"
#include "bn_bg_tiles.h"
#include "bn_core.h"
#include "bn_sprite_tiles.h"
#include "bn_keypad.h"
#include "bn_music.h"
#include "bn_sprite_palettes.h"
#include "bn_string.h"

#include "ss_audio.h"
#include "ss_save.h"
#include "ss_telemetry.h"

namespace ss
{

app::app()
{
    bn::bg_palettes::set_transparent_color(bn::color(1, 1, 4));
    _saved_hiscore = save::load_hiscore();
    _session.hiscore = _saved_hiscore;
    _enter(game_state::TITLE);
}

void app::update()
{
    game_state next = _state;

    switch(_state)
    {

    case game_state::TITLE:
        next = _update_title();
        break;

    case game_state::PLAYING:
        next = _update_playing();
        break;

    case game_state::PAUSED:
        next = _update_paused();
        break;

    case game_state::STAGE_CLEAR:
        next = _update_stage_clear();
        break;

    case game_state::GAME_OVER:
        next = _update_game_over();
        break;

    case game_state::ENDING:
        next = _update_ending();
        break;

    default:
        break;
    }

    if(next != _state)
    {
        _enter(next);
    }

    _update_fade();
    audio::update();

    volatile ss_telemetry_block& t = ss_telemetry;
    t.frame = t.frame + 1;
    t.state = (uint8_t) _state;
    t.music_playing = bn::music::playing() && ! bn::music::paused() ? 1 : 0;
    t.score = unsigned(_session.score);
    t.hiscore = unsigned(_session.hiscore);
    t.sprite_tiles_used = (uint16_t) bn::sprite_tiles::used_tiles_count();
    t.bg_tiles_used = (uint16_t) bn::bg_tiles::used_tiles_count();
    t.bg_map_cells_used = (uint16_t) bn::bg_maps::used_cells_count();
    int cpu = (bn::core::last_cpu_usage() * 100).integer();
    t.cpu_pct = (uint8_t) bn::min(cpu, 255);

    // Skip the first frames of a stage: they include the (one-off) stage loading work.
    if(_state == game_state::PLAYING && t.stage_frame > 3 && t.cpu_pct > t.cpu_pct_max)
    {
        t.cpu_pct_max = t.cpu_pct;
        t.cpu_max_stage_frame = t.stage_frame;
        t.cpu_max_frame = t.frame;
    }

    // Frames dropped during gameplay (stage loading and screen transitions are not counted).
    if(_state == game_state::PLAYING && t.stage_frame > 3)
    {
        t.missed_frames = (uint16_t) (t.missed_frames + bn::core::last_missed_frames());
    }
}

void app::_update_fade()
{
    if(_fade_in <= 0)
    {
        return;
    }

    --_fade_in;
    bn::fixed intensity = bn::fixed(_fade_in) / fade_frames;
    bn::bg_palettes::set_fade(bn::color(0, 0, 0), intensity);

    // The ending screen drives the sprite fade itself (page transitions).
    if(_state != game_state::ENDING)
    {
        bn::sprite_palettes::set_fade(bn::color(0, 0, 0), intensity);
    }
}

void app::_save_hiscore()
{
    if(_session.hiscore > _saved_hiscore)
    {
        save::store_hiscore(_session.hiscore);
        _saved_hiscore = _session.hiscore;
    }
}

// ----- state handlers ------------------------------------------------------------------------------

game_state app::_update_title()
{
    return _title->update() ? game_state::PLAYING : game_state::TITLE;
}

game_state app::_update_playing()
{
    if(bn::keypad::start_pressed())
    {
        return game_state::PAUSED;
    }

    switch(_world->update())
    {

    case world::result::STAGE_CLEARED:
        _final_clear = false;
        return game_state::STAGE_CLEAR;

    case world::result::GAME_COMPLETE:
        _final_clear = true;
        return game_state::STAGE_CLEAR;

    case world::result::GAME_OVER:
        return game_state::GAME_OVER;

    default:
        return game_state::PLAYING;
    }
}

game_state app::_update_paused()
{
    // Gameplay is frozen: the world is not updated at all while paused.
    ++_timer;

    for(bn::sprite_ptr& sprite : _overlay)
    {
        sprite.set_visible(((_timer / 20) & 1) == 0 || _overlay.size() < 2);
    }

    return bn::keypad::start_pressed() ? game_state::PLAYING : game_state::PAUSED;
}

game_state app::_update_stage_clear()
{
    ++_timer;
    bool outro_done = _world->update_outro();

    if(_timer >= 300 && outro_done)
    {
        // Fade to black before loading the next screen.
        if(_fade_out < fade_frames)
        {
            ++_fade_out;
            bn::fixed intensity = bn::fixed(_fade_out) / fade_frames;
            bn::bg_palettes::set_fade(bn::color(0, 0, 0), intensity);
            bn::sprite_palettes::set_fade(bn::color(0, 0, 0), intensity);
            return game_state::STAGE_CLEAR;
        }

        if(_final_clear)
        {
            return game_state::ENDING;
        }

        ++_session.stage;
        return game_state::PLAYING;
    }

    return game_state::STAGE_CLEAR;
}

game_state app::_update_game_over()
{
    ++_timer;

    if((_timer > 90 && bn::keypad::start_pressed()) || _timer > 900)
    {
        return game_state::TITLE;
    }

    return game_state::GAME_OVER;
}

game_state app::_update_ending()
{
    return _ending->update() ? game_state::TITLE : game_state::ENDING;
}

// ----- transitions ---------------------------------------------------------------------------------

void app::_enter(game_state next)
{
    game_state previous = _state;
    _state = next;
    _timer = 0;
    _fade_out = 0;
    _overlay.clear();

    bool new_screen = next == game_state::TITLE || next == game_state::ENDING ||
            (next == game_state::PLAYING && previous != game_state::PAUSED);

    if(new_screen)
    {
        // Start fully black; _update_fade() brings the palettes back over fade_frames frames.
        _fade_in = fade_frames + 1;
        bn::bg_palettes::set_fade(bn::color(0, 0, 0), 1);
        bn::sprite_palettes::set_fade(bn::color(0, 0, 0), 1);
    }

    switch(next)
    {

    case game_state::TITLE:
        _save_hiscore();
        _world.reset();
        _ending.reset();
        _title.emplace(_text, _session.hiscore);
        break;

    case game_state::PLAYING:
        if(previous == game_state::PAUSED)
        {
            audio::resume_music();
            audio::play(audio::sfx::PAUSE);
            _world->display.set_visible(true);
            break;
        }

        if(previous == game_state::TITLE)
        {
            int stage = SS_DEBUG ? _title->selected_stage() : 0;
            _title.reset();
            int hiscore = _session.hiscore;
            _session = session();
            _session.hiscore = hiscore;
            _session.stage = stage;
        }

        // New stage: destroy the old world first so its sprites/BGs/VRAM are released.
        _world.reset();
        _world.emplace(_session, _text);
        break;

    case game_state::PAUSED:
        audio::pause_music();
        audio::play(audio::sfx::PAUSE);
        _text.centered(-16, "PAUSED", _overlay, text_color::YELLOW);
        _text.centered(0, "PRESS START", _overlay);
        break;

    case game_state::STAGE_CLEAR:
        {
            int bonus = 5000 * (_session.stage + 1) + 1000 * _session.lives;
            _session.add_score(bonus);
            _world->ship.start_outro();
            audio::play_music(audio::music::JINGLE_CLEAR);

            bn::string<24> title(_final_clear ? "MISSION COMPLETE" : "STAGE ");

            if(! _final_clear)
            {
                format_number(_session.stage + 1, 1, title);
                title.append(" CLEAR!");
            }

            _text.centered(-28, title, _overlay, text_color::YELLOW);
            bn::string<24> bonus_text("BONUS ");
            format_number(bonus, 1, bonus_text);
            _text.centered(-12, bonus_text, _overlay);
        }
        break;

    case game_state::GAME_OVER:
        {
            audio::play_music(audio::music::JINGLE_GAME_OVER);
            bool record = _session.score > _saved_hiscore && _session.score >= _session.hiscore;
            _save_hiscore();
            bn::bg_palettes::set_fade(bn::color(0, 0, 0), bn::fixed(0.5));
            _world->display.set_visible(false);
            _text.centered(-28, "GAME OVER", _overlay, text_color::RED);
            bn::string<24> score("SCORE ");
            format_number(_session.score, 8, score);
            _text.centered(-10, score, _overlay);

            if(record)
            {
                _text.centered(4, "NEW HI-SCORE!", _overlay, text_color::YELLOW);
            }

            _text.centered(24, "PRESS START", _overlay, text_color::CYAN);
        }
        break;

    case game_state::ENDING:
        _save_hiscore();
        _world.reset();
        _ending.emplace(_text, _session);
        break;

    default:
        break;
    }
}

}
