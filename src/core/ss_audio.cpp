#include "ss_audio.h"

#include "bn_fixed.h"
#include "bn_music.h"
#include "bn_sound.h"

#include "bn_music_items.h"
#include "bn_sound_items.h"

namespace ss::audio
{

namespace
{
    struct sfx_info
    {
        bn::sound_item item;
        int cooldown_frames;    // minimum frames between two plays of this effect
        int priority;           // higher wins when all SFX channels are busy
        bn::fixed volume;
    };

    constexpr sfx_info sfx_table[] = {
        { bn::sound_items::sfx_shot, 6, 0, bn::fixed(0.45) },
        { bn::sound_items::sfx_spread, 6, 0, bn::fixed(0.45) },
        { bn::sound_items::sfx_missile, 12, 0, bn::fixed(0.4) },
        { bn::sound_items::sfx_charge_ready, 0, 2, bn::fixed(0.6) },
        { bn::sound_items::sfx_beam, 0, 3, bn::fixed(0.8) },
        { bn::sound_items::sfx_hit, 4, 0, bn::fixed(0.4) },
        { bn::sound_items::sfx_explode, 4, 2, bn::fixed(0.7) },
        { bn::sound_items::sfx_explode_big, 10, 4, bn::fixed(0.9) },
        { bn::sound_items::sfx_pickup, 0, 3, bn::fixed(0.7) },
        { bn::sound_items::sfx_power_max, 0, 4, bn::fixed(0.8) },
        { bn::sound_items::sfx_player_death, 0, 5, bn::fixed(0.9) },
        { bn::sound_items::sfx_warning, 0, 5, bn::fixed(0.8) },
        { bn::sound_items::sfx_select, 0, 3, bn::fixed(0.7) },
        { bn::sound_items::sfx_pause, 0, 3, bn::fixed(0.7) },
        { bn::sound_items::sfx_shield_hit, 0, 4, bn::fixed(0.8) },
    };

    static_assert(sizeof(sfx_table) / sizeof(sfx_table[0]) == int(sfx::COUNT));

    unsigned char cooldowns[int(sfx::COUNT)] = {};
}

void play(sfx effect)
{
    int index = int(effect);
    const sfx_info& info = sfx_table[index];

    if(cooldowns[index])
    {
        return;
    }

    cooldowns[index] = (unsigned char) info.cooldown_frames;
    bn::sound::play_with_priority(info.priority, info.item, info.volume);
}

void play_music(music track)
{
    switch(track)
    {

    case music::TITLE:
        bn::music::play(bn::music_items::music_title, 0.6);
        break;

    case music::STAGE1:
        bn::music::play(bn::music_items::music_stage1, 0.6);
        break;

    case music::STAGE2:
        bn::music::play(bn::music_items::music_stage2, 0.6);
        break;

    case music::STAGE3:
        bn::music::play(bn::music_items::music_stage3, 0.6);
        break;

    case music::BOSS:
        bn::music::play(bn::music_items::music_boss, 0.6);
        break;

    case music::ENDING:
        bn::music::play(bn::music_items::music_ending, 0.6);
        break;

    case music::JINGLE_CLEAR:
        bn::music::play(bn::music_items::jingle_clear, 0.6, false);
        break;

    case music::JINGLE_GAME_OVER:
        bn::music::play(bn::music_items::jingle_gameover, 0.6, false);
        break;

    default:
        stop_music();
        break;
    }
}

void stop_music()
{
    if(bn::music::playing())
    {
        bn::music::stop();
    }
}

void pause_music()
{
    if(bn::music::playing() && ! bn::music::paused())
    {
        bn::music::pause();
    }
}

void resume_music()
{
    if(bn::music::paused())
    {
        bn::music::resume();
    }
}

void update()
{
    for(unsigned char& cooldown : cooldowns)
    {
        if(cooldown)
        {
            --cooldown;
        }
    }
}

}
