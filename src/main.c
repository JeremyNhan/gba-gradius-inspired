/*
 * Space Shooter - a horizontal shoot-'em-up for the Game Boy Advance.
 * C99 on libtonc + Maxmod (devkitARM). No heap: all state is static.
 */
#include <maxmod.h>

#include "ss_app.h"
#include "ss_audio.h"
#include "ss_frame.h"
#include "ss_input.h"
#include "ss_level.h"
#include "ss_sprites.h"
#include "ss_text.h"
#include "ss_video.h"

#if SS_TESTS
#include "ss_test.h"
#endif

static volatile u32 vblank_count;

static void vblank_isr(void)
{
    ++vblank_count;
    mmVBlank();         /* Maxmod resets its sound DMA every VBlank */
}

int main(void)
{
    /* Cartridge ROM access: 3/1 wait states with the prefetch buffer (the power-on default is 4/2
     * without prefetch, which makes code running from ROM about twice as slow). SRAM stays at 8. */
    REG_WAITCNT = WS_STANDARD;

    irq_init(NULL);
    irq_add(II_VBLANK, vblank_isr);

    video_init();
    text_init();
    audio_init();
    app_init();

#if SS_TESTS
    test_init();
#endif

    u32 last_vblank = vblank_count;

    while(1)
    {
        VBlankIntrWait();
        frame_timer_start();

        /* VBlank: copy the frame prepared by the previous iteration to the hardware. */
        sprites_commit();
        text_commit();
        terrain_commit();
        video_commit();

        u32 now = vblank_count;
        frame_info.missed_frames += now - last_vblank - 1;
        last_vblank = now;
        ++frame_info.frame;

        audio_frame();
        input_update();
        sprites_begin();

#if SS_TESTS
        test_frame();
#endif

        app_update();
        frame_info.cpu_pct = (int) (frame_timer_ticks() * 100 / CYCLES_PER_FRAME);
    }
}
