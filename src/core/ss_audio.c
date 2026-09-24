#include "ss_audio.h"

#include <maxmod.h>

#include "soundbank.h"
#include "soundbank_bin.h"

/* 4 channels for the MOD music + 4 for effects. */
#define MIX_CHANNELS 8
#define MUSIC_VOLUME 614                /* of 1024 (0.6) */

typedef struct
{
    u16 sample;
    u8 cooldown_frames;                 /* minimum frames between two plays (rapid fire) */
    u8 volume;                          /* 0..255 */
} sfx_info;

static const sfx_info sfx_table[SFX_COUNT] = {
    [SFX_SHOT_ID] = { SFX_SFX_SHOT, 6, 115 },
    [SFX_SPREAD_ID] = { SFX_SFX_SPREAD, 6, 115 },
    [SFX_MISSILE_ID] = { SFX_SFX_MISSILE, 12, 102 },
    [SFX_CHARGE_READY_ID] = { SFX_SFX_CHARGE_READY, 0, 153 },
    [SFX_BEAM_ID] = { SFX_SFX_BEAM, 0, 204 },
    [SFX_HIT_ID] = { SFX_SFX_HIT, 4, 102 },
    [SFX_EXPLODE_ID] = { SFX_SFX_EXPLODE, 4, 178 },
    [SFX_EXPLODE_BIG_ID] = { SFX_SFX_EXPLODE_BIG, 10, 230 },
    [SFX_PICKUP_ID] = { SFX_SFX_PICKUP, 0, 178 },
    [SFX_POWER_MAX_ID] = { SFX_SFX_POWER_MAX, 0, 204 },
    [SFX_PLAYER_DEATH_ID] = { SFX_SFX_PLAYER_DEATH, 0, 230 },
    [SFX_WARNING_ID] = { SFX_SFX_WARNING, 0, 204 },
    [SFX_SELECT_ID] = { SFX_SFX_SELECT, 0, 178 },
    [SFX_PAUSE_ID] = { SFX_SFX_PAUSE, 0, 178 },
    [SFX_SHIELD_HIT_ID] = { SFX_SFX_SHIELD_HIT, 0, 204 },
};

static const u16 music_modules[MUSIC_NONE] = {
    [MUSIC_TITLE] = MOD_MUSIC_TITLE,
    [MUSIC_STAGE1] = MOD_MUSIC_STAGE1,
    [MUSIC_STAGE2] = MOD_MUSIC_STAGE2,
    [MUSIC_STAGE3] = MOD_MUSIC_STAGE3,
    [MUSIC_BOSS] = MOD_MUSIC_BOSS,
    [MUSIC_ENDING] = MOD_MUSIC_ENDING,
    [MUSIC_JINGLE_CLEAR] = MOD_JINGLE_CLEAR,
    [MUSIC_JINGLE_GAME_OVER] = MOD_JINGLE_GAMEOVER,
};

static u8 cooldowns[SFX_COUNT];
static bool music_on;
static bool music_paused;

void audio_init(void)
{
    mmInitDefault((mm_addr) soundbank_bin, MIX_CHANNELS);
    mmSetModuleVolume(MUSIC_VOLUME);
}

void audio_frame(void)
{
    mmFrame();

    for(int i = 0; i < SFX_COUNT; ++i)
    {
        if(cooldowns[i])
        {
            --cooldowns[i];
        }
    }

    if(music_on && ! music_paused && ! mmActive())
    {
        music_on = false;       /* a one-shot jingle ended */
    }
}

void audio_play(sfx_id id)
{
    const sfx_info* info = &sfx_table[id];

    if(cooldowns[id])
    {
        return;
    }

    cooldowns[id] = info->cooldown_frames;
    mm_sound_effect effect = { { info->sample }, 1 << 10, 0, info->volume, 128 };
    mmEffectEx(&effect);
}

void audio_play_music(music_id id)
{
    if(id >= MUSIC_NONE)
    {
        audio_stop_music();
        return;
    }

    bool loop = id != MUSIC_JINGLE_CLEAR && id != MUSIC_JINGLE_GAME_OVER;
    mmStart(music_modules[id], loop ? MM_PLAY_LOOP : MM_PLAY_ONCE);
    music_on = true;
    music_paused = false;
}

void audio_stop_music(void)
{
    if(music_on)
    {
        mmStop();
    }

    music_on = false;
    music_paused = false;
}

void audio_pause_music(void)
{
    if(music_on && ! music_paused)
    {
        mmPause();
        music_paused = true;
    }
}

void audio_resume_music(void)
{
    if(music_paused)
    {
        mmResume();
        music_paused = false;
    }
}

bool audio_music_playing(void)
{
    return music_on && ! music_paused;
}
