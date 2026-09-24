/*
 * Frame timing: CPU use per frame (hardware timers 2+3 cascaded, CPU clock ticks; Maxmod owns
 * timer 0) and missed frames (VBlank interrupts counted by the main loop). Test builds also time
 * each game system (prof_mark).
 */
#ifndef SS_FRAME_H
#define SS_FRAME_H

#include "ss_base.h"

#define CYCLES_PER_FRAME 280896         /* 228 lines x 1232 cycles */

void frame_timer_start(void);
u32 frame_timer_ticks(void);            /* CPU cycles since frame_timer_start() */

typedef struct
{
    int cpu_pct;                        /* last frame, percent of a frame */
    u32 missed_frames;                  /* total since boot */
    u32 frame;                          /* frames since boot */
} frame_stats;

extern frame_stats frame_info;

#if SS_TESTS
enum
{
    PROF_STAGE, PROF_PLAYER, PROF_SHOTS, PROF_ENEMIES, PROF_BOSS, PROF_BULLETS, PROF_POWERUPS, PROF_COLLIDE,
    PROF_EFFECTS, PROF_TERRAIN, PROF_BACKGROUNDS, PROF_HUD, PROF_RENDER, PROF_COUNT
};

extern u16 prof_permille[PROF_COUNT];  /* cost of each system in the last gameplay frame, 1/1000 frame */
void prof_begin(void);
void prof_mark(int slot);
#define PROF_BEGIN() prof_begin()
#define PROF_MARK(slot) prof_mark(slot)
#else
#define PROF_BEGIN() ((void) 0)
#define PROF_MARK(slot) ((void) 0)
#endif

#endif
