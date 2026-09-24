#include "ss_frame.h"

frame_stats frame_info;

void frame_timer_start(void)
{
    REG_TM2CNT = 0;
    REG_TM3CNT = 0;
    REG_TM2D = 0;
    REG_TM3D = 0;
    REG_TM3CNT = TM_ENABLE | TM_CASCADE;
    REG_TM2CNT = TM_ENABLE | TM_FREQ_1;
}

u32 frame_timer_ticks(void)
{
    u32 hi = REG_TM3D;
    u32 lo = REG_TM2D;

    if(REG_TM3D != hi)          /* the low counter overflowed between the two reads */
    {
        hi = REG_TM3D;
        lo = REG_TM2D;
    }

    return (hi << 16) | lo;
}

#if SS_TESTS
u16 prof_permille[PROF_COUNT];
static u32 prof_last;

void prof_begin(void)
{
    prof_last = frame_timer_ticks();
}

void prof_mark(int slot)
{
    u32 now = frame_timer_ticks();
    prof_permille[slot] = (u16) (((now - prof_last) * 1000u) / CYCLES_PER_FRAME);
    prof_last = now;
}
#endif
