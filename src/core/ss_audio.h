#ifndef SS_AUDIO_H
#define SS_AUDIO_H

namespace ss::audio
{

enum class sfx : unsigned char
{
    SHOT,
    SPREAD,
    MISSILE,
    CHARGE_READY,
    BEAM,
    HIT,
    EXPLODE,
    EXPLODE_BIG,
    PICKUP,
    ONE_UP,
    PLAYER_DEATH,
    WARNING,
    SELECT,
    PAUSE,
    SHIELD_HIT,
    COUNT
};

enum class music : unsigned char
{
    TITLE,
    STAGE1,
    STAGE2,
    STAGE3,
    BOSS,
    ENDING,
    JINGLE_CLEAR,
    JINGLE_GAME_OVER,
    NONE
};

/// Plays a sound effect. Frequent effects are rate-limited so they don't monopolise Maxmod's
/// 4 SFX channels (BN_CFG_AUDIO_MAX_SOUND_CHANNELS).
void play(sfx effect);

void play_music(music track);

void stop_music();

void pause_music();

void resume_music();

/// Call once per frame (cooldown bookkeeping).
void update();

}

#endif
