/*
 * Space Shooter - shared basics: hardware headers, fixed-point math, angles, screen constants.
 *
 * Coordinates: game positions are centre-origin like the screen centre (x in [-120, 120), y in
 * [-80, 80)), stored as 20.12 fixed point (fx). The ARM7TDMI has no FPU, so no float is used at
 * run time (FX_F(1.5) is folded to an integer by the compiler).
 */
#ifndef SS_BASE_H
#define SS_BASE_H

#include <stdbool.h>
#include <stdint.h>

#include <tonc.h>

#ifndef SS_DEBUG
    #define SS_DEBUG 0
#endif

#ifndef SS_TESTS
    #define SS_TESTS 0
#endif

/* Test hooks (invincibility, autofire, power override...) exist in debug and test builds only. */
#define SS_TEST_HOOKS (SS_DEBUG || SS_TESTS)

/* ----- fixed point (20.12) ------------------------------------------------------------------------ */

typedef int32_t fx;

#define FX_SHIFT 12
#define FX_ONE (1 << FX_SHIFT)
#define FX(n) ((fx) ((n) * FX_ONE))                    /* integer constant */
#define FX_F(f) ((fx) ((f) * (double) FX_ONE))          /* compile-time float constant */

static inline fx fx_mul(fx a, fx b)
{
    return (fx) (((int64_t) a * b) >> FX_SHIFT);
}

static inline fx fx_div_int(fx a, int n)
{
    return a / n;
}

static inline int fx_floor(fx a)       /* round toward -infinity */
{
    return a >> FX_SHIFT;
}

static inline int fx_trunc(fx a)       /* round toward zero */
{
    return a >= 0 ? a >> FX_SHIFT : -((-a) >> FX_SHIFT);
}

static inline int fx_round(fx a)
{
    return (a + FX_ONE / 2) >> FX_SHIFT;
}

static inline fx fx_abs(fx a)
{
    return a < 0 ? -a : a;
}

typedef struct
{
    fx x;
    fx y;
} vec2;

static inline vec2 v2(fx x, fx y)
{
    vec2 v = { x, y };
    return v;
}

static inline vec2 v2i(int x, int y)
{
    vec2 v = { FX(x), FX(y) };
    return v;
}

static inline vec2 v2_add(vec2 a, vec2 b)
{
    return v2(a.x + b.x, a.y + b.y);
}

/* ----- angles ------------------------------------------------------------------------------------- */
/*
 * Binary angles: 65536 units per turn, 0 = pointing right, increasing clockwise on screen (screen y
 * grows downwards). libtonc's lu_sin/lu_cos take the same units and return .12 fixed point.
 */
#define ANGLE_TURN 65536
#define ANGLE_LEFT 32768

static inline vec2 direction(int angle, fx speed)
{
    unsigned a = (unsigned) angle & 0xFFFF;
    return v2(fx_mul(lu_cos(a), speed), fx_mul(lu_sin(a), speed));
}

/* Angle from one point to another (BIOS ArcTan2 on whole-pixel deltas). */
static inline int angle_to(vec2 from, vec2 to)
{
    int dx = fx_round(to.x - from.x);
    int dy = fx_round(to.y - from.y);

    if(dx == 0 && dy == 0)
    {
        return ANGLE_LEFT;
    }

    /* keep the BIOS inputs well inside s16 */
    while(dx > 16000 || dx < -16000 || dy > 16000 || dy < -16000)
    {
        dx /= 2;
        dy /= 2;
    }

    return (int) ((uint16_t) ArcTan2((s16) dx, (s16) dy));
}

/* Signed smallest difference between two binary angles, in [-32768, 32767]. */
static inline int angle_delta(int from, int to)
{
    int delta = (to - from) & 0xFFFF;
    return delta >= 32768 ? delta - 65536 : delta;
}

#define DEGREES(d) (((d) * ANGLE_TURN) / 360)

/* ----- screen & layout ---------------------------------------------------------------------------- */

#define SCREEN_W 240
#define SCREEN_H 160
#define HALF_W 120
#define HALF_H 80

#define HUD_H 10
#define PLAY_TOP (-HALF_H + HUD_H)
#define PLAY_BOTTOM HALF_H

#define STAGE_COUNT 3

/* Entity pool capacities (OAM budget: docs/architecture.md). */
#define MAX_PLAYER_SHOTS 32
#define MAX_ENEMIES 16
#define MAX_ENEMY_BULLETS 32
#define MAX_EFFECTS 16
#define MAX_POWERUPS 4

/* Player tuning. */
#define START_LIVES 3
#define MAX_LIVES 9
#define RESPAWN_FRAMES 90
#define INVULNERABLE_FRAMES 150
#define MAX_SHIELD 3
#define CHARGE_FRAMES 45

/* Axis-aligned hitbox (centre + half extents, whole pixels). */
typedef struct
{
    vec2 c;
    int hw;
    int hh;
} hitbox;

static inline hitbox make_hitbox(vec2 c, int hw, int hh)
{
    hitbox h = { c, hw, hh };
    return h;
}

static inline bool hit_test(hitbox a, hitbox b)
{
    fx dx = fx_abs(a.c.x - b.c.x);
    fx dy = fx_abs(a.c.y - b.c.y);
    return dx < FX(a.hw + b.hw) && dy < FX(a.hh + b.hh);
}

/* True when a point is outside the screen by more than margin pixels (used to despawn). */
static inline bool off_screen(vec2 p, int margin)
{
    return p.x < FX(-120 - margin) || p.x > FX(120 + margin) || p.y < FX(-80 - margin) || p.y > FX(80 + margin);
}

/* ----- misc --------------------------------------------------------------------------------------- */

#define COUNT_OF(a) ((int) (sizeof(a) / sizeof((a)[0])))

/* Inclusive clamp (libtonc's CLAMP excludes the upper bound). MIN/MAX come from tonc_math.h. */
#define SS_CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : (v) > (hi) ? (hi) : (v))

#endif
