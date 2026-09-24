#include "ss_powerups.h"

#include "gen_gfx.h"
#include "ss_audio.h"
#include "ss_hud.h"
#include "ss_player.h"
#include "ss_power_data.h"
#include "ss_sprites.h"
#include "ss_world.h"

powerup powerups[MAX_POWERUPS];
static int active_count;

void powerups_reset(void)
{
    for(int i = 0; i < MAX_POWERUPS; ++i)
    {
        powerups[i].active = false;
    }

    active_count = 0;
}

int powerups_count(void)
{
    return active_count;
}

void powerups_drop(vec2 pos)
{
    for(int i = 0; i < MAX_POWERUPS; ++i)
    {
        powerup* p = &powerups[i];

        if(! p->active)
        {
            p->active = true;
            p->pos = pos;
            p->timer = 0;
            ++active_count;
            return;
        }
    }

    /* four capsules already on screen: this drop is lost */
}

static void release(powerup* p)
{
    p->active = false;
    --active_count;
}

void powerups_collect(powerup* p)
{
    int power = game.gear.power;

    if(power < MAX_POWER)
    {
        world_set_power(power + 1);
        game_add_score(200);
        hud_show_pickup(power_names[power + 1]);
        audio_play(power + 1 == MAX_POWER ? SFX_POWER_MAX_ID : SFX_PICKUP_ID);
    }
    else
    {
        /* Full power: refill the shield and score a bonus. */
        game.gear.shield = MAX_SHIELD;
        game_add_score(1000);
        hud_show_pickup("FULL POWER");
        audio_play(SFX_PICKUP_ID);
    }

    release(p);
}

void powerups_update(void)
{
    for(int i = 0; i < MAX_POWERUPS; ++i)
    {
        powerup* p = &powerups[i];

        if(! p->active)
        {
            continue;
        }

        ++p->timer;

        /* Drift left slowly while bobbing. */
        fx bob = direction(p->timer * 1024, FX_F(0.35)).y;
        p->pos.x -= FX_F(0.45);
        p->pos.y = SS_CLAMP(p->pos.y + bob, FX(PLAY_TOP + 8), FX(PLAY_BOTTOM - 8));

        if(p->pos.x < FX(-130) || p->timer > 900)
        {
            release(p);
        }
    }
}

void powerups_render(void)
{
    for(int i = 0; i < MAX_POWERUPS; ++i)
    {
        const powerup* p = &powerups[i];

        /* blink during the last two seconds before expiring */
        if(p->active && (p->timer < 780 || (p->timer & 4)))
        {
            sprites_draw(GEN_SPR_POWERUP, (p->timer >> 3) & 1, p->pos, 0);
        }
    }
}
