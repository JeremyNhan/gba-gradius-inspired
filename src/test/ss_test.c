#include "ss_test.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "ss_app.h"
#include "ss_input.h"
#include "ss_world.h"

/* Block read by tests/launcher.lua (its address comes from the ELF symbol table). Layout:
 *   +0 magic "SSTEST01", +8 done, +12 shot_seq, +16 shot_name[32], +48 log_len, +52 ready, +56 log[]
 * `ready` is set by the launcher: mGBA attaches the script shortly after the ROM starts, so the tests
 * wait for it (otherwise the first screenshot requests would be missed). */
typedef struct
{
    char magic[8];
    u32 done;
    u32 shot_seq;
    char shot_name[32];
    u32 log_len;
    u32 ready;
    char log[24 * 1024];
} test_io_block;

EWRAM_BSS volatile test_io_block ss_test_io;

u32 test_now;
int test_expected_hiscore;

#define MARKER_OFFSET 0x100
static const char marker[8] = "SSTEST2";

typedef bool (*suite_fn)(void);

typedef struct
{
    const char* name;
    suite_fn run;
} suite;

static const suite phase1[] = {
    { "smoke", suite_smoke },
    { "death", suite_death },
    { "power", suite_power },
    { "full_run", suite_full_run },
};

static const suite phase2[] = {
    { "save_check", suite_save_check },
};

static const suite* suites;
static int suite_count;
static int current;
static int settle;                  /* frames to wait between suites */
static int checks;
static int failures;
static int total_checks;
static int total_failures;
static u16 keys;
static test_hook hooks[2];

/* ----- log ---------------------------------------------------------------------------------------- */

static void log_append(const char* s)
{
    u32 len = ss_test_io.log_len;

    while(*s && len < sizeof(ss_test_io.log) - 1)
    {
        ss_test_io.log[len++] = *s++;
    }

    ss_test_io.log[len] = 0;
    ss_test_io.log_len = len;
}

void test_log(const char* fmt, ...)
{
    char line[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);
    log_append(line);
    log_append("\n");
}

void test_check(const char* name, bool ok, const char* detail_fmt, ...)
{
    char detail[160] = "";

    if(detail_fmt)
    {
        va_list args;
        va_start(args, detail_fmt);
        vsnprintf(detail, sizeof(detail), detail_fmt, args);
        va_end(args);
    }

    ++checks;
    failures += ! ok;
    test_log("[%s] %s%s%s%s", ok ? "PASS" : "FAIL", name, detail[0] ? "  (" : "", detail, detail[0] ? ")" : "");
}

void test_shot(const char* fmt, ...)
{
    char name[32];
    va_list args;
    va_start(args, fmt);
    vsnprintf(name, sizeof(name), fmt, args);
    va_end(args);

    for(int i = 0; i < 32; ++i)
    {
        ss_test_io.shot_name[i] = name[i];
    }

    ++ss_test_io.shot_seq;
}

/* ----- input -------------------------------------------------------------------------------------- */

void test_key_down(u16 key)
{
    keys |= key;
    input_inject(keys);
}

void test_key_up(u16 key)
{
    keys &= (u16) ~key;
    input_inject(keys);
}

void test_keys_release(void)
{
    keys = 0;
    input_inject(0);
}

void test_set_hooks(test_hook a, test_hook b)
{
    hooks[0] = a;
    hooks[1] = b;
}

/* ----- sequencer ---------------------------------------------------------------------------------- */

void test_init(void)
{
    memcpy((void*) ss_test_io.magic, "SSTEST01", 8);
    ss_test_io.done = 0;
    ss_test_io.shot_seq = 0;
    ss_test_io.log_len = 0;

    /* SRAM has an 8-bit bus: byte accesses only (no memcmp/memcpy). */
    bool second_boot = true;

    for(int i = 0; i < 8; ++i)
    {
        second_boot = second_boot && sram_mem[MARKER_OFFSET + i] == (u8) marker[i];
    }

    if(second_boot)
    {
        test_expected_hiscore = sram_mem[MARKER_OFFSET + 8] | (sram_mem[MARKER_OFFSET + 9] << 8) |
                (sram_mem[MARKER_OFFSET + 10] << 16) | (sram_mem[MARKER_OFFSET + 11] << 24);

        for(int i = 0; i < 12; ++i)
        {
            sram_mem[MARKER_OFFSET + i] = 0;
        }

        suites = phase2;
        suite_count = COUNT_OF(phase2);
    }
    else
    {
        suites = phase1;
        suite_count = COUNT_OF(phase1);
    }

    current = 0;
    settle = 0;
    test_keys_release();
    test_log("=== %s ===", suites[0].name);
}

static void finish_suite(void)
{
    test_log("SUMMARY %s: %d checks, %d failed", suites[current].name, checks, failures);
    total_checks += checks;
    total_failures += failures;
    checks = 0;
    failures = 0;
    hooks[0] = hooks[1] = NULL;
    test_keys_release();
    memset(&test_ctl, 0, sizeof(test_ctl));

    if(++current < suite_count)
    {
        test_log("=== %s ===", suites[current].name);
        app_go_to_title();
        settle = 30;
        return;
    }

    if(suites == phase1)
    {
        /* Leave a marker so the next boot (launcher restarts the emulator) runs the save check. */
        for(int i = 0; i < 8; ++i)
        {
            sram_mem[MARKER_OFFSET + i] = (u8) marker[i];
        }

        for(int i = 0; i < 4; ++i)
        {
            sram_mem[MARKER_OFFSET + 8 + i] = (u8) (game.hiscore >> (8 * i));
        }
    }

    test_log("TOTAL: %d checks, %d failed", total_checks, total_failures);
    test_log("RESULT: %s", total_failures ? "FAIL" : "PASS");
    ss_test_io.done = 1;
}

void test_frame(void)
{
    if(ss_test_io.done || ! ss_test_io.ready)
    {
        return;
    }

    ++test_now;

    if(settle > 0)
    {
        --settle;
        return;
    }

    for(int i = 0; i < 2; ++i)
    {
        if(hooks[i])
        {
            hooks[i]();
        }
    }

    if(suites[current].run())
    {
        finish_suite();
    }
}
