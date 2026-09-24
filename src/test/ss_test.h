/*
 * Automated test driver (test ROM only, make TESTS=1).
 *
 * Scenarios are sequential C code (protothread-style macros below) run once per frame before the
 * game logic: they inject keys through ss_input and read the game state directly. Results go to a
 * text log in RAM; tests/launcher.lua (mGBA) saves the log and takes the requested screenshots.
 */
#ifndef SS_TEST_H
#define SS_TEST_H

#include "ss_base.h"

void test_init(void);
void test_frame(void);

/* ----- scenario helpers (ss_test_suites.c) --------------------------------------------------------- */

typedef struct
{
    int line;
    u32 wake;
} test_thread;

extern u32 test_now;                 /* frames since the tests started */

#define T_BEGIN(t) switch((t)->line) { case 0:
#define T_END(t) } (t)->line = -1; return true
#define T_WAIT(t, frames) \
    do { (t)->wake = test_now + (frames); (t)->line = __LINE__; __attribute__((fallthrough)); case __LINE__: \
         if(test_now < (t)->wake) return false; } while(0)
#define T_WAIT_UNTIL(t, cond, max_frames) \
    do { (t)->wake = test_now + (max_frames); (t)->line = __LINE__; __attribute__((fallthrough)); case __LINE__: \
         if(! (cond) && test_now < (t)->wake) return false; } while(0)

/* The launcher saves a screenshot at the end of the frame it was requested in, after that frame's
 * game logic ran: wait one frame so input that follows cannot change the screen first. */
#define T_SHOT(t, ...) do { test_shot(__VA_ARGS__); T_WAIT(t, 1); } while(0)

void test_log(const char* fmt, ...);
void test_check(const char* name, bool ok, const char* detail_fmt, ...);
void test_shot(const char* fmt, ...);
void test_key_down(u16 key);
void test_key_up(u16 key);
void test_keys_release(void);

/* Suites: each returns true when finished. */
bool suite_smoke(void);
bool suite_death(void);
bool suite_power(void);
bool suite_full_run(void);
bool suite_save_check(void);

/* Per-frame hooks run before the suite step (bot, statistics). */
typedef void (*test_hook)(void);
void test_set_hooks(test_hook a, test_hook b);

/* Value the save check expects (stored in SRAM by the first boot). */
extern int test_expected_hiscore;

#endif
