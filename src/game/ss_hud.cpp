#include "ss_hud.h"

#include "bn_core.h"
#include "bn_sprites.h"
#include "bn_string.h"

#include "bn_sprite_items_boss_bar.h"
#include "bn_sprite_items_life_icon.h"

#include "ss_power_data.h"
#include "ss_text.h"
#include "ss_weapon_data.h"
#include "ss_world.h"

namespace ss
{

namespace
{
    constexpr int top_y = -80;
    constexpr int bottom_y = 70;
    constexpr int bar_y = -64;

    void setup_hud_sprite(bn::sprite_ptr& sprite)
    {
        sprite.set_bg_priority(hud_bg_priority);
        sprite.set_z_order(z_hud);
    }
}

hud::hud(text& text_generator) :
    _text(text_generator)
{
    _life_icon = bn::sprite_items::life_icon.create_sprite(96, top_y + 4);
    setup_hud_sprite(*_life_icon);
}

void hud::set_visible(bool visible)
{
    _visible = visible;

    auto apply = [visible](auto& sprites)
    {
        for(bn::sprite_ptr& sprite : sprites)
        {
            sprite.set_visible(visible);
        }
    };

    apply(_score_sprites);
    apply(_status_sprites);
    apply(_lives_sprites);
    apply(_bar_sprites);
    apply(_pickup_sprites);

    if(_life_icon)
    {
        _life_icon->set_visible(visible);
    }
}

void hud::show_banner(const char* title, const char* subtitle)
{
    _banner_sprites.clear();
    _text.centered(-24, title, _banner_sprites, text_color::YELLOW);
    _text.centered(-12, subtitle, _banner_sprites, text_color::WHITE);
    _banner_timer = 150;
    _warning_timer = 0;
}

void hud::show_warning()
{
    _banner_sprites.clear();
    _text.centered(-24, "! WARNING !", _banner_sprites, text_color::RED);
    _text.centered(-10, "A HUGE ENEMY APPROACHES", _banner_sprites, text_color::WHITE);
    _warning_timer = 180;
    _banner_timer = 0;
}

void hud::show_pickup(const char* label)
{
    _pending_pickup = label;
}

bool hud::_update_score(world& w)
{
    const session& game = w.game;

    // Redrawing text renders glyphs into sprite tiles in software, so the score line is refreshed
    // at most every 4th frame (15 Hz is plenty for a score counter).
    bool refresh_frame = (w.stage_frame & 3) == 0 || _shown_score < 0;
    bool drew = false;

    if(refresh_frame && (game.score != _shown_score || game.hiscore != _shown_hiscore))
    {
        _shown_score = game.score;
        _shown_hiscore = game.hiscore;
        _score_sprites.clear();

        bn::string<24> line("SC ");
        format_number(game.score, 8, line);
        _text.left(-118, top_y, line, _score_sprites);

        bn::string<24> high("HI ");
        format_number(game.hiscore, 8, high);
        _text.left(-38, top_y, high, _score_sprites, text_color::YELLOW);
        drew = true;
    }

    if(game.lives != _shown_lives)
    {
        _shown_lives = game.lives;
        _lives_sprites.clear();

        bn::string<8> lives("x");
        format_number(game.lives, 1, lives);
        _text.left(103, top_y, lives, _lives_sprites);
    }

    return drew;
}

bool hud::_update_status(world& w)
{
    const loadout& gear = w.game.gear;
    int status = gear.power | (gear.shield << 4);

    if(status == _shown_status)
    {
        return false;
    }

    _shown_status = status;
    _status_sprites.clear();

    // e.g. "P9 S.LASER x3 DOT HMSL SHLD3 WAVE": power step, main gun, gun count (ship + shooters),
    // then the extra powers owned.
    int power = gear.power;
    bn::string<40> line("P");
    format_number(power, 1, line);
    line.append(" ");
    line.append(gun_defs[int(main_gun_of(power))].hud_name);

    if(int shooters = shooter_count_of(power))
    {
        line.append(" x");
        format_number(shooters + 1, 1, line);
    }

    if(has_power(power, power_step::HOMING_DOT))
    {
        line.append(" DOT");
    }

    missile_mode missiles = missile_mode_of(power);

    if(missiles != missile_mode::NONE)
    {
        line.append(missiles == missile_mode::HOMING ? " HMSL" : " MSL");
    }

    if(gear.shield)
    {
        line.append(" SHLD");
        format_number(gear.shield, 1, line);
    }

    if(has_power(power, power_step::SHOCKWAVE))
    {
        line.append(" WAVE");
    }

    _text.left(-118, bottom_y, line, _status_sprites, text_color::CYAN);
    return true;
}

void hud::_update_boss_bar(world& w)
{
    const boss& b = w.big_boss;

    if(! b.active() || b.hp_max() == 0)
    {
        _bar_sprites.clear();

        for(int& frame : _bar_frames)
        {
            frame = -1;
        }

        return;
    }

    if(_bar_sprites.empty())
    {
        _text.left(-118, bar_y - 4, "BOSS", _bar_sprites, text_color::RED);

        for(int index = 0; index < 4; ++index)
        {
            bn::sprite_ptr segment = bn::sprite_items::boss_bar.create_sprite(-72 + 16 + index * 32, bar_y, 16);
            setup_hud_sprite(segment);
            _bar_sprites.push_back(segment);
            _bar_frames[index] = 16;
        }
    }

    // 128 px bar, 4 segments of 32 px, each frame shows 0..32 px in 2 px steps.
    int filled = (b.hp() * 128) / b.hp_max();
    int first_segment = _bar_sprites.size() - 4;

    for(int index = 0; index < 4; ++index)
    {
        int pixels = bn::clamp(filled - index * 32, 0, 32);
        int frame = pixels / 2;

        if(frame != _bar_frames[index])
        {
            _bar_frames[index] = frame;
            _bar_sprites[first_segment + index].set_tiles(bn::sprite_items::boss_bar.tiles_item(), frame);
        }
    }
}

void hud::_update_debug(world& w)
{
#if SS_DEBUG
    if(! _debug || (w.stage_frame % 15) != 0)
    {
        return;
    }

    _debug_sprites.clear();
    bn::string<48> line("CPU");
    format_number((bn::core::last_cpu_usage() * 100).integer(), 2, line);
    line.append(" E");
    format_number(w.foes.count(), 2, line);
    line.append(" B");
    format_number(w.bullets.count(), 2, line);
    line.append(" S");
    format_number(w.shots.count(), 2, line);
    line.append(" SPR");
    format_number(bn::sprites::used_items_count(), 3, line);
    line.append(" F");
    format_number(w.stage_frame, 4, line);
    _text.left(-118, -70, line, _debug_sprites, text_color::YELLOW);
#else
    (void) w;
#endif
}

void hud::update(world& w)
{
    if(! _visible)
    {
        return;
    }

    // Rendering text draws glyphs into sprite tiles in software (up to ~15 % of a frame for a full
    // line), so at most one text item is redrawn per frame: pickup label, then status line, then
    // score. A capsule pickup therefore spreads its redraws over three frames.
    bool drew = false;

    if(_pending_pickup)
    {
        _pickup_sprites.clear();
        _text.centered(56, _pending_pickup, _pickup_sprites, text_color::CYAN);
        _pickup_timer = 60;
        _pending_pickup = nullptr;
        drew = true;
    }

    if(! drew)
    {
        drew = _update_status(w);
    }

    if(! drew)
    {
        _update_score(w);
    }

    _update_boss_bar(w);
    _update_debug(w);

    if(_banner_timer && --_banner_timer == 0)
    {
        _banner_sprites.clear();
    }

    if(_warning_timer)
    {
        --_warning_timer;
        bool show = (_warning_timer / 12) % 2 == 0;

        for(bn::sprite_ptr& sprite : _banner_sprites)
        {
            sprite.set_visible(show);
        }

        if(_warning_timer == 0)
        {
            _banner_sprites.clear();
        }
    }

    if(_pickup_timer && --_pickup_timer == 0)
    {
        _pickup_sprites.clear();
    }
}

}
