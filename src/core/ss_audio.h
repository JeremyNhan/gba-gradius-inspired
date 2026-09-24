/*
 * Music and sound effects through Maxmod (music: generated 4-channel MODs, effects: 8-bit WAVs,
 * both packed into the soundbank by mmutil at build time).
 */
#ifndef SS_AUDIO_H
#define SS_AUDIO_H

#include "ss_base.h"

typedef enum
{
    SFX_SHOT_ID,
    SFX_SPREAD_ID,
    SFX_MISSILE_ID,
    SFX_CHARGE_READY_ID,
    SFX_BEAM_ID,
    SFX_HIT_ID,
    SFX_EXPLODE_ID,
    SFX_EXPLODE_BIG_ID,
    SFX_PICKUP_ID,
    SFX_POWER_MAX_ID,       /* reaching the top of the power ladder */
    SFX_PLAYER_DEATH_ID,
    SFX_WARNING_ID,
    SFX_SELECT_ID,
    SFX_PAUSE_ID,
    SFX_SHIELD_HIT_ID,
    SFX_COUNT
} sfx_id;

typedef enum
{
    MUSIC_TITLE,
    MUSIC_STAGE1,
    MUSIC_STAGE2,
    MUSIC_STAGE3,
    MUSIC_BOSS,
    MUSIC_ENDING,
    MUSIC_JINGLE_CLEAR,
    MUSIC_JINGLE_GAME_OVER,
    MUSIC_NONE
} music_id;

void audio_init(void);

/* Once per frame (after VBlank): Maxmod mixing and effect cooldowns. */
void audio_frame(void);

void audio_play(sfx_id id);
void audio_play_music(music_id id);
void audio_stop_music(void);
void audio_pause_music(void);
void audio_resume_music(void);
bool audio_music_playing(void);     /* playing and not paused */

#endif
