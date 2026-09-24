/*
 * Original sprite art for Space Shooter.
 *
 * Small sprites are hand-authored ASCII art in the master palette; a 'K' outline is added
 * automatically, which gives every sprite the same crisp 16-bit look. Effects and bosses are drawn
 * procedurally. Each generator returns a vertical strip of frames and sets *frame_h.
 */
#include "art.h"

#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846
#define MAX_FRAMES 32

static int M(char k)
{
    return pal_index(&pal_master, k);
}

static int B(char k)
{
    return pal_index(&pal_boss, k);
}

/* ASCII rows (w x h) in the master palette, optionally outlined in 'K'. */
static canvas* art(const char* const* rows, int w, int h, int outline)
{
    int map[256];
    palette_charmap(&pal_master, map);
    canvas* c = canvas_new(w, h);
    canvas_blit_ascii(c, rows, h, map, 0, 0);

    if(outline)
    {
        canvas_outline(c, M('K'), 0);
    }

    return c;
}

/* Mutable copy of ASCII rows (max 16 x 16). */
typedef struct
{
    char r[16][17];
} rows16;

static rows16 rows_of(const char* const* src, int h)
{
    rows16 out;
    memset(&out, 0, sizeof(out));

    for(int y = 0; y < h; ++y)
    {
        strcpy(out.r[y], src[y]);
    }

    return out;
}

static canvas* art16(const rows16* rows, int outline)
{
    const char* ptrs[16];

    for(int y = 0; y < 16; ++y)
    {
        ptrs[y] = rows->r[y];
    }

    return art(ptrs, 16, 16, outline);
}

/* Python str.replace on every row. Pattern and replacement must have the same length. */
static void rows_replace(rows16* rows, int h, const char* from, const char* to)
{
    size_t n = strlen(from);

    for(int y = 0; y < h; ++y)
    {
        char* at = rows->r[y];

        while((at = strstr(at, from)) != NULL)
        {
            memcpy(at, to, n);
            at += n;
        }
    }
}

/* ----- player ----------------------------------------------------------------------------------- */

static const char* const PLAYER[16] = {
    "................",
    "................",
    "...nb...........",
    "...nbb..........",
    "....nbb.........",
    "....dlllb.......",
    "..dllllllccc....",
    "oydlllllllllllW.",
    "..ddlllllddddd..",
    "....dddlb.......",
    "....nbb.........",
    "...nbb..........",
    "...nb...........",
    "................",
    "................",
    "................",
};

/* Bank the ship: rear columns move opposite to the nose to fake a 3/4 tilt. */
static rows16 shear(const char* const* rows, int direction)
{
    rows16 out = rows_of(rows, 16);

    for(int x = 0; x < 16; ++x)
    {
        int shift = x < 6 ? direction : 0;

        if(shift)
        {
            for(int y = 0; y < 16; ++y)
            {
                int src = y - shift;
                out.r[y][x] = (src >= 0 && src < 16) ? rows[src][x] : '.';
            }
        }
    }

    return out;
}

static canvas* player(int* frame_h)
{
    static const int banks[3] = { 0, -1, 1 };     /* level, nose up, nose down */
    canvas* frames[MAX_FRAMES];
    int n = 0;

    for(int b = 0; b < 3; ++b)
    {
        rows16 base = banks[b] ? shear(PLAYER, banks[b]) : rows_of(PLAYER, 16);

        for(int flame = 0; flame < 2; ++flame)
        {
            rows16 r = base;

            /* engine flame flicker: alternate the two exhaust pixels */
            for(int y = 0; y < 16; ++y)
            {
                for(int x = 0; x < 2; ++x)
                {
                    if(r.r[y][x] == 'o' || r.r[y][x] == 'y')
                    {
                        r.r[y][x] = flame == 0 ? (x == 1 ? 'y' : 'o') : (x == 1 ? 'o' : 'r');
                    }
                }
            }

            frames[n++] = art16(&r, 1);
        }
    }

    *frame_h = 16;
    return canvas_vstack(frames, n);
}

static canvas* life_icon(int* frame_h)
{
    static const char* const rows[8] = {
        "........",
        ".nb.....",
        ".dlllc..",
        "ylllllW.",
        ".dddd...",
        ".nb.....",
        "........",
        "........",
    };
    canvas* frames[1] = { art(rows, 8, 8, 1) };
    *frame_h = 8;
    return canvas_vstack(frames, 1);
}

/* ----- enemies ---------------------------------------------------------------------------------- */

static const char* const DART[16] = {
    "................",
    "................",
    "................",
    "..........mm....",
    "........mrrm....",
    "......mrrrd.....",
    "...ldrroolldd...",
    ".Wlllrooyoolldo.",
    "...ldrroolldd...",
    "......mrrrd.....",
    "........mrrm....",
    "..........mm....",
    "................",
    "................",
    "................",
    "................",
};

static canvas* dart(int* frame_h)
{
    rows16 b = rows_of(DART, 16);
    rows_replace(&b, 16, "do.", "dy.");
    canvas* frames[2] = { art(DART, 16, 16, 1), art16(&b, 1) };
    *frame_h = 16;
    return canvas_vstack(frames, 2);
}

static canvas* waver(int* frame_h)
{
    static const char* const base[16] = {
        "................",
        "................",
        "................",
        "................",
        ".....GGGGGG.....",
        "...GGggggggGG...",
        "..GggWWggggggG..",
        ".dddddddddddddd.",
        ".llcllllcllllcl.",
        "..dddddddddddd..",
        "....GGGGGGGG....",
        "................",
        "................",
        "................",
        "................",
        "................",
    };
    canvas* frames[3];

    for(int phase = 0; phase < 3; ++phase)
    {
        rows16 r = rows_of(base, 16);

        for(int x = 1; x < 15; ++x)
        {
            char ch = r.r[8][x];

            if(ch == 'l' || ch == 'c')
            {
                r.r[8][x] = (x + phase * 2) % 6 == 0 ? 'y' : 'l';
            }
        }

        frames[phase] = art16(&r, 1);
    }

    *frame_h = 16;
    return canvas_vstack(frames, 3);
}

static canvas* interceptor(int* frame_h)
{
    static const char* const base[16] = {
        "................",
        "................",
        "................",
        "................",
        "................",
        "........vvvv....",
        "....vvvvpppvv...",
        ".Wpppppppppcvvyo",
        "....vvvvpppvv...",
        "........vvvv....",
        "................",
        "................",
        "................",
        "................",
        "................",
        "................",
    };
    rows16 b = rows_of(base, 16);
    rows_replace(&b, 16, "yo", "oy");
    canvas* frames[2] = { art(base, 16, 16, 1), art16(&b, 1) };
    *frame_h = 16;
    return canvas_vstack(frames, 2);
}

/* Ground turret, 5 barrel directions: left, up-left, up, up-right, right (floor-mounted). */
static canvas* turret(int* frame_h)
{
    canvas* frames[5];

    for(int i = 0; i < 5; ++i)
    {
        canvas* c = canvas_new(16, 16);
        double ang = PI - i * PI / 4;
        double cx = 7.5;
        double cy = 9.5;
        canvas_ellipse(c, 7.5, 12.5, 5.5, 4.5, M('d'));
        canvas_ellipse(c, 7.5, 11.5, 4, 3, M('l'));
        canvas_rect(c, 1, 14, 14, 2, M('n'));

        /* two-pixel-thick barrel drawn over the dome: light line with a dark underside, red muzzle */
        double nx = sin(ang);
        double ny = cos(ang);

        for(int t = 2; t < 8; ++t)
        {
            double x = cx + cos(ang) * t;
            double y = cy - sin(ang) * t;
            canvas_set(c, ag_round(x + nx * 0.8), ag_round(y + ny * 0.8), M('d'));
            canvas_set(c, ag_round(x), ag_round(y), M('l'));
        }

        canvas_set(c, ag_round(cx + cos(ang) * 7.5), ag_round(cy - sin(ang) * 7.5), M('r'));
        canvas_set(c, 7, 11, M('o'));
        canvas_set(c, 8, 11, M('o'));
        canvas_outline(c, M('K'), 0);
        frames[i] = c;
    }

    *frame_h = 16;
    return canvas_vstack(frames, 5);
}

static canvas* hulk(int* frame_h)
{
    canvas* c = canvas_new(32, 32);
    const point body[] = { { 3, 16 }, { 9, 6 }, { 26, 5 }, { 30, 10 }, { 30, 22 }, { 26, 27 }, { 9, 26 } };
    const point plate[] = { { 6, 16 }, { 11, 9 }, { 25, 8 }, { 28, 12 }, { 28, 20 }, { 25, 24 }, { 11, 23 } };
    canvas_polygon(c, body, 7, M('d'));
    canvas_polygon(c, plate, 7, M('l'));

    /* armour plates */
    static const int ys[3] = { 11, 15, 19 };

    for(int i = 0; i < 3; ++i)
    {
        canvas_line(c, 13, ys[i], 26, ys[i], M('d'));
    }

    canvas_rect(c, 20, 9, 2, 15, M('d'));

    /* cannon & glowing core */
    canvas_rect(c, 1, 14, 8, 4, M('m'));
    canvas_rect(c, 0, 15, 3, 2, M('r'));
    canvas_ellipse(c, 15, 16, 3.5, 3.5, M('m'));
    canvas_ellipse(c, 15, 16, 2.5, 2.5, M('r'));
    canvas_set(c, 14, 15, M('y'));

    /* engines */
    canvas_rect(c, 29, 11, 3, 3, M('o'));
    canvas_rect(c, 29, 19, 3, 3, M('o'));
    canvas_outline(c, M('K'), 0);
    canvas* f2 = canvas_copy(c);
    canvas_replace(f2, M('o'), M('y'));
    canvas* frames[2] = { c, f2 };
    *frame_h = 32;
    return canvas_vstack(frames, 2);
}

static canvas* swarm(int* frame_h)
{
    canvas* frames[4];

    for(int i = 0; i < 4; ++i)
    {
        canvas* c = canvas_new(16, 16);
        canvas_ring(c, 7.5, 7.5, 5.5, 3.2, M('o'));
        double ang = i * PI / 4;

        for(int k = 0; k < 3; ++k)
        {
            double a = ang + k * 2 * PI / 3;
            canvas_set(c, ag_round(7.5 + cos(a) * 4.4 - 0.5), ag_round(7.5 + sin(a) * 4.4 - 0.5), M('y'));
        }

        canvas_ellipse(c, 7.5, 7.5, 2, 2, M('c'));
        canvas_set(c, 7, 7, M('W'));
        canvas_outline(c, M('K'), 0);
        frames[i] = c;
    }

    *frame_h = 16;
    return canvas_vstack(frames, 4);
}

static canvas* mine(int* frame_h)
{
    canvas* frames[2];

    for(int i = 0; i < 2; ++i)
    {
        canvas* c = canvas_new(16, 16);

        for(int k = 0; k < 8; ++k)
        {
            double a = k * PI / 4;
            canvas_line(c, 7.5 + cos(a) * 3, 7.5 + sin(a) * 3, 7.5 + cos(a) * 6.5, 7.5 + sin(a) * 6.5, M('d'));
        }

        canvas_ellipse(c, 7.5, 7.5, 4.5, 4.5, M('m'));
        canvas_ellipse(c, 7, 7, 3, 3, i == 0 ? M('r') : M('o'));
        canvas_set(c, 6, 6, M('W'));
        canvas_outline(c, M('K'), 0);
        frames[i] = c;
    }

    *frame_h = 16;
    return canvas_vstack(frames, 2);
}

static canvas* asteroid_small(int* frame_h)
{
    lcg rng;
    lcg_init(&rng, 77);
    canvas* base = canvas_new(16, 16);
    point pts[9];

    for(int k = 0; k < 9; ++k)
    {
        double a = k * 2 * PI / 9;
        double r = 5.2 + lcg_randint(&rng, 0, 20) / 10.0;
        pts[k].x = 7.5 + cos(a) * r;
        pts[k].y = 7.5 + sin(a) * r;
    }

    canvas_polygon(base, pts, 9, M('d'));
    canvas_ellipse(base, 6.5, 6.5, 3.5, 3, M('l'));
    canvas_ellipse(base, 9, 9.5, 1.5, 1.2, M('d'));
    canvas_ellipse(base, 5, 9, 1, 1, M('n'));
    canvas_outline(base, M('K'), 0);
    canvas* frames[4];

    for(int i = 0; i < 4; ++i)
    {
        frames[i] = canvas_rotated90(base, i);
    }

    canvas_free(base);
    *frame_h = 16;
    return canvas_vstack(frames, 4);
}

static canvas* asteroid_big(int* frame_h)
{
    lcg rng;
    lcg_init(&rng, 1234);
    canvas* base = canvas_new(32, 32);
    point pts[12];

    for(int k = 0; k < 12; ++k)
    {
        double a = k * 2 * PI / 12;
        double r = 11.5 + lcg_randint(&rng, 0, 30) / 10.0;
        pts[k].x = 15.5 + cos(a) * r;
        pts[k].y = 15.5 + sin(a) * r;
    }

    canvas_polygon(base, pts, 12, M('d'));
    canvas_ellipse(base, 13, 12, 8, 7, M('l'));
    static const double craters[4][3] = { { 19, 19, 3 }, { 10, 18, 2 }, { 18, 9, 1.6 }, { 22, 14, 1.2 } };

    for(int i = 0; i < 4; ++i)
    {
        double x = craters[i][0];
        double y = craters[i][1];
        double r = craters[i][2];
        canvas_ellipse(base, x, y, r, r, M('d'));
        canvas_ellipse(base, x + 0.6, y + 0.6, r * 0.6, r * 0.6, M('n'));
    }

    canvas_outline(base, M('K'), 0);
    canvas* frames[4];

    for(int i = 0; i < 4; ++i)
    {
        frames[i] = canvas_rotated90(base, i);
    }

    canvas_free(base);
    *frame_h = 32;
    return canvas_vstack(frames, 4);
}

/* ----- projectiles ------------------------------------------------------------------------------ */

static canvas* player_shot(int* frame_h)
{
    static const char* const rows[8] = {
        "........", "........", "........", "bccWWWc.", "bccWWWc.", "........", "........", "........",
    };
    canvas* frames[1] = { art(rows, 8, 8, 0) };
    *frame_h = 8;
    return canvas_vstack(frames, 1);
}

static canvas* spread_shot(int* frame_h)
{
    static const char* const rows[8] = {
        "........", "........", "...cc...", "..cWWb..", "..cWWb..", "...bb...", "........", "........",
    };
    canvas* frames[1] = { art(rows, 8, 8, 0) };
    *frame_h = 8;
    return canvas_vstack(frames, 1);
}

/* 8 directions, frame i = i * 45 degrees counter-clockwise from 'right'. */
static canvas* missile(int* frame_h)
{
    canvas* frames[8];

    for(int i = 0; i < 8; ++i)
    {
        canvas* c = canvas_new(8, 8);
        double a = i * PI / 4;
        double dx = cos(a);
        double dy = -sin(a);

        for(int t = -3; t < 3; ++t)
        {
            canvas_set(c, ag_round(3.5 + dx * t), ag_round(3.5 + dy * t), t < 2 ? M('l') : M('W'));
        }

        canvas_set(c, ag_round(3.5 + dx * 2.5), ag_round(3.5 + dy * 2.5), M('r'));
        canvas_set(c, ag_round(3.5 - dx * 3.5), ag_round(3.5 - dy * 3.5), M('o'));
        canvas_outline(c, M('K'), 0);
        frames[i] = c;
    }

    *frame_h = 8;
    return canvas_vstack(frames, 8);
}

/* 32x8 laser bolt. Frames: level, rising (spread laser upper), falling (spread laser lower). The
 * diagonal slope (about 0.19) matches the spread laser velocity in the game's weapon table. */
static canvas* laser(int* frame_h)
{
    static const double slopes[3] = { 0.0, -0.19, 0.19 };
    canvas* frames[3];

    for(int i = 0; i < 3; ++i)
    {
        double slope = slopes[i];
        canvas* c = canvas_new(32, 8);

        for(int x = 3; x < 30; ++x)
        {
            double y = 3.5 + (x - 16) * slope;
            int yi = ag_round(y);
            canvas_set(c, x, yi - 1, M('b'));
            canvas_set(c, x, yi, x < 8 ? M('c') : M('W'));
            canvas_set(c, x, yi + 1, (slope == 0 || x < 8) ? M('c') : M('b'));
        }

        frames[i] = c;
    }

    *frame_h = 8;
    return canvas_vstack(frames, 3);
}

/* 16x16 additional shooter (trailing drone). Two pulse frames. */
static canvas* shooter(int* frame_h)
{
    canvas* frames[2];

    for(int i = 0; i < 2; ++i)
    {
        canvas* c = canvas_new(16, 16);
        double r = i == 0 ? 5.5 : 6;
        canvas_ellipse(c, 7.5, 7.5, r, r * 0.85, M('m'));
        canvas_ellipse(c, 7.5, 7.5, r - 1, r * 0.85 - 1, M('o'));
        canvas_ellipse(c, 8, 7, 3, 2.5, M('y'));
        canvas_ellipse(c, 9, 6, 1.2, 1, M('W'));
        canvas_outline(c, M('K'), 0);
        frames[i] = c;
    }

    *frame_h = 16;
    return canvas_vstack(frames, 2);
}

static canvas* charge_shot(int* frame_h)
{
    canvas* frames[2];

    for(int i = 0; i < 2; ++i)
    {
        canvas* c = canvas_new(32, 16);
        canvas_ellipse(c, 14 + i, 7.5, 13, 7, M('b'));
        canvas_ellipse(c, 17 + i, 7.5, 12, 6, M('c'));
        canvas_ellipse(c, 20 + i, 7.5, 9, 4, M('W'));
        canvas_ellipse(c, 9 - i, 7.5, 8, 5.5, 0);
        frames[i] = c;
    }

    *frame_h = 16;
    return canvas_vstack(frames, 2);
}

static canvas* enemy_bullet(int* frame_h)
{
    canvas* frames[2];

    for(int i = 0; i < 2; ++i)
    {
        canvas* c = canvas_new(8, 8);
        double r = i == 0 ? 3 : 2.6;
        canvas_ellipse(c, 3.5, 3.5, r, r, M('p'));
        canvas_ellipse(c, 3.5, 3.5, 1.6, 1.6, M('W'));
        frames[i] = c;
    }

    *frame_h = 8;
    return canvas_vstack(frames, 2);
}

static canvas* enemy_bullet_big(int* frame_h)
{
    canvas* frames[2];

    for(int i = 0; i < 2; ++i)
    {
        canvas* c = canvas_new(8, 8);
        canvas_ellipse(c, 3.5, 3.5, 3.9, 3.9, i == 0 ? M('r') : M('o'));
        canvas_ellipse(c, 3.5, 3.5, 2.6, 2.6, i == 0 ? M('o') : M('y'));
        canvas_ellipse(c, 3, 3, 1.2, 1.2, M('W'));
        frames[i] = c;
    }

    *frame_h = 8;
    return canvas_vstack(frames, 2);
}

static canvas* enemy_needle(int* frame_h)
{
    static const char* const rows[8] = {
        "........", "........", "........", ".Wyyoor.", "........", "........", "........", "........",
    };
    canvas* frames[1] = { art(rows, 8, 8, 0) };
    *frame_h = 8;
    return canvas_vstack(frames, 1);
}

/* ----- effects ---------------------------------------------------------------------------------- */

static canvas* explosion(int size, uint32_t seed, int* frame_h)
{
    typedef struct
    {
        double r_out, r_in;
        char c_out, c_in;
    } stage;

    static const stage stages[6] = {
        { 0.25, 0.00, 'W', 'y' },
        { 0.45, 0.10, 'y', 'o' },
        { 0.62, 0.25, 'o', 'r' },
        { 0.75, 0.45, 'r', 'm' },
        { 0.85, 0.62, 'm', 'd' },
        { 0.92, 0.80, 'd', 'n' },
    };
    lcg rng;
    lcg_init(&rng, seed);
    double half = size / 2.0;
    double debris[10][2];

    for(int k = 0; k < 10; ++k)
    {
        debris[k][0] = lcg_randint(&rng, 0, 628) / 100.0;
        debris[k][1] = lcg_randint(&rng, 30, 100) / 100.0;
    }

    canvas* frames[6];

    for(int i = 0; i < 6; ++i)
    {
        const stage* s = &stages[i];
        canvas* c = canvas_new(size, size);
        double R = half * s->r_out;
        canvas_ellipse(c, half, half, R, R, M(s->c_out));
        canvas_ellipse(c, half - R * 0.15, half - R * 0.15, R * 0.6, R * 0.6, M(s->c_in));

        if(s->r_in > 0)
        {
            canvas_ellipse(c, half, half, half * s->r_in, half * s->r_in, 0);
        }

        for(int k = 0; k < 10; ++k)
        {
            double reach = s->r_out + 0.1 < 0.95 ? s->r_out + 0.1 : 0.95;
            double d = half * reach * debris[k][1];
            double x = half + cos(debris[k][0]) * d;
            double y = half + sin(debris[k][0]) * d;
            canvas_set(c, (int) x, (int) y, M(i < 3 ? 'y' : i < 5 ? 'o' : 'd'));
        }

        frames[i] = c;
    }

    *frame_h = size;
    return canvas_vstack(frames, 6);
}

static canvas* explosion_small(int* frame_h)
{
    return explosion(16, 5, frame_h);
}

static canvas* explosion_big(int* frame_h)
{
    return explosion(32, 8, frame_h);
}

static canvas* spark(int* frame_h)
{
    canvas* frames[3];

    for(int i = 0; i < 3; ++i)
    {
        canvas* c = canvas_new(8, 8);
        int r = 1 + i;
        int col = i == 0 ? M('W') : i == 1 ? M('y') : M('o');
        canvas_line(c, 3.5 - r, 3.5, 3.5 + r, 3.5, col);
        canvas_line(c, 3.5, 3.5 - r, 3.5, 3.5 + r, col);

        if(i < 2)
        {
            canvas_set(c, 3, 3, M('W'));
        }

        frames[i] = c;
    }

    *frame_h = 8;
    return canvas_vstack(frames, 3);
}

static canvas* shield(int* frame_h)
{
    canvas* frames[2];

    for(int i = 0; i < 2; ++i)
    {
        canvas* c = canvas_new(32, 32);

        for(int k = 0; k < 24; ++k)
        {
            double a = k * 2 * PI / 24 + i * PI / 24;
            double x = 15.5 + cos(a) * 12;
            double y = 15.5 + sin(a) * 10;
            canvas_set(c, ag_round(x), ag_round(y), k % 2 ? M('c') : M('W'));
            canvas_set(c, ag_round(15.5 + cos(a) * 11), ag_round(15.5 + sin(a) * 9), M('b'));
        }

        frames[i] = c;
    }

    *frame_h = 32;
    return canvas_vstack(frames, 2);
}

static canvas* charge_glow(int* frame_h)
{
    canvas* frames[4];

    for(int i = 0; i < 4; ++i)
    {
        canvas* c = canvas_new(16, 16);
        double r = 2 + i * 1.6;
        canvas_ellipse(c, 7.5, 7.5, r, r, M('b'));
        canvas_ellipse(c, 7.5, 7.5, r * 0.7, r * 0.7, M('c'));
        canvas_ellipse(c, 7.5, 7.5, r * 0.35, r * 0.35, M('W'));
        frames[i] = c;
    }

    *frame_h = 16;
    return canvas_vstack(frames, 4);
}

static void letter(canvas* c, char ch, int ox, int oy, int col)
{
    const char* const* rows = font_rows(ch);

    for(int y = 0; y < 7; ++y)
    {
        for(int x = 0; x < 5; ++x)
        {
            if(rows[y][x] == '#')
            {
                canvas_set(c, ox + x, oy + y, col);
            }
        }
    }
}

/* Power capsule: frame 0 normal, frame 1 highlighted (blink). */
static canvas* powerups(int* frame_h)
{
    static const char specs[2][2] = { { 'r', 'm' }, { 'o', 'r' } };
    canvas* frames[2];

    for(int i = 0; i < 2; ++i)
    {
        canvas* c = canvas_new(16, 16);
        canvas_ellipse(c, 7.5, 7.5, 7, 6.5, M(specs[i][1]));
        canvas_ellipse(c, 7.5, 7, 6, 5.5, M(specs[i][0]));
        canvas_ellipse(c, 5.5, 4.5, 2, 1.2, M('W'));
        letter(c, 'P', 5, 5, M('K'));
        canvas_outline(c, M('K'), 0);
        frames[i] = c;
    }

    *frame_h = 16;
    return canvas_vstack(frames, 2);
}

/* 32x8 bar segments, 17 frames: 0..32 filled pixels in steps of 2. */
static canvas* boss_bar(int* frame_h)
{
    canvas* frames[17];

    for(int i = 0; i < 17; ++i)
    {
        canvas* c = canvas_new(32, 8);
        canvas_rect(c, 0, 1, 32, 6, M('K'));
        canvas_rect(c, 0, 2, 32, 4, M('n'));
        int fill = i * 2;

        if(fill)
        {
            canvas_rect(c, 0, 2, fill, 4, M('r'));
            canvas_rect(c, 0, 2, fill, 1, M('o'));
        }

        frames[i] = c;
    }

    *frame_h = 8;
    return canvas_vstack(frames, 17);
}

/* ----- bosses (boss palette) -------------------------------------------------------------------- */

/* Stage 1 boss: armoured carrier with a front eye core. Frames: normal, core open. */
static canvas* boss_warden(int* frame_h)
{
    canvas* frames[2];

    for(int opened = 0; opened < 2; ++opened)
    {
        canvas* c = canvas_new(64, 64);
        const point hull[] = { { 4, 32 }, { 14, 14 }, { 40, 8 }, { 60, 14 }, { 62, 50 }, { 40, 56 }, { 14, 50 } };
        const point mid[] = { { 8, 32 }, { 17, 17 }, { 40, 12 }, { 57, 17 }, { 58, 46 }, { 40, 52 }, { 17, 47 } };
        const point top[] = { { 12, 30 }, { 20, 19 }, { 38, 15 }, { 54, 19 }, { 54, 30 } };
        canvas_polygon(c, hull, 7, B('3'));
        canvas_polygon(c, mid, 7, B('2'));
        canvas_polygon(c, top, 5, B('1'));

        /* hangar slots */
        static const int slot_ys[2] = { 22, 40 };

        for(int k = 0; k < 2; ++k)
        {
            canvas_rect(c, 30, slot_ys[k], 22, 4, B('4'));
            canvas_rect(c, 32, slot_ys[k] + 1, 18, 2, B('3'));
        }

        /* vents */
        for(int x = 36; x < 56; x += 5)
        {
            canvas_rect(c, x, 30, 2, 5, B('4'));
        }

        /* engines */
        canvas_rect(c, 58, 20, 6, 6, B('o'));
        canvas_rect(c, 58, 38, 6, 6, B('o'));
        canvas_rect(c, 60, 21, 4, 4, B('y'));
        canvas_rect(c, 60, 39, 4, 4, B('y'));

        /* front core */
        canvas_ellipse(c, 14, 32, 9, 9, B('4'));

        if(opened)
        {
            canvas_ellipse(c, 14, 32, 7, 7, B('m'));
            canvas_ellipse(c, 13, 31, 5, 5, B('r'));
            canvas_ellipse(c, 12, 30, 2.5, 2.5, B('y'));
            canvas_set(c, 11, 29, B('W'));
        }
        else
        {
            canvas_ellipse(c, 14, 32, 7, 7, B('3'));
            canvas_line(c, 8, 32, 20, 32, B('4'));
            canvas_ellipse(c, 14, 32, 2, 2, B('r'));
        }

        canvas_outline(c, B('K'), 0);
        frames[opened] = c;
    }

    *frame_h = 64;
    return canvas_vstack(frames, 2);
}

/* Stage 2 boss: crystal hive around a pulsing core. Frames: closed, pulse, open. */
static canvas* boss_hive(int* frame_h)
{
    lcg rng;
    lcg_init(&rng, 4242);
    double shards[9][2];

    for(int k = 0; k < 9; ++k)
    {
        shards[k][0] = k * 2 * PI / 9 + lcg_randint(&rng, 0, 40) / 100.0;
        shards[k][1] = 18 + lcg_randint(&rng, 0, 10);
    }

    canvas* frames[3];

    for(int f = 0; f < 3; ++f)
    {
        canvas* c = canvas_new(64, 64);

        for(int k = 0; k < 9; ++k)
        {
            double a = shards[k][0];
            double L = shards[k][1];
            double spread = 0.28;
            point tri[3] = {
                { 31.5 + cos(a - spread) * 10, 31.5 + sin(a - spread) * 10 },
                { 31.5 + cos(a) * (L + f), 31.5 + sin(a) * (L + f) },
                { 31.5 + cos(a + spread) * 10, 31.5 + sin(a + spread) * 10 },
            };
            canvas_polygon(c, tri, 3, B('v'));
            canvas_line(c, tri[0].x, tri[0].y, tri[1].x, tri[1].y, B('p'));
        }

        canvas_ellipse(c, 31.5, 31.5, 15, 15, B('u'));
        canvas_ellipse(c, 31.5, 31.5, 13, 13, B('v'));
        static const int core_radii[3] = { 6, 8, 10 };
        double core_r = core_radii[f];
        canvas_ellipse(c, 31.5, 31.5, core_r + 1, core_r + 1, B('m'));
        canvas_ellipse(c, 31.5, 31.5, core_r, core_r, B('r'));
        canvas_ellipse(c, 30, 30, core_r * 0.6, core_r * 0.6, B('o'));
        canvas_ellipse(c, 29, 29, core_r * 0.25, core_r * 0.25, B('y'));
        canvas_outline(c, B('K'), 0);
        frames[f] = c;
    }

    *frame_h = 64;
    return canvas_vstack(frames, 3);
}

/* Final boss front half: armoured prow with a huge eye. Frames: eye closed / open / angry. */
static canvas* boss_overmind_front(int* frame_h)
{
    canvas* frames[3];

    for(int f = 0; f < 3; ++f)
    {
        canvas* c = canvas_new(64, 64);
        const point outer[] = { { 2, 32 }, { 12, 10 }, { 40, 2 }, { 64, 4 }, { 64, 60 }, { 40, 62 }, { 12, 54 } };
        const point mid[] = { { 6, 32 }, { 15, 13 }, { 40, 6 }, { 64, 8 }, { 64, 56 }, { 40, 58 }, { 15, 51 } };
        const point top[] = { { 10, 26 }, { 18, 15 }, { 40, 10 }, { 64, 11 }, { 64, 22 }, { 30, 22 } };
        canvas_polygon(c, outer, 7, B('3'));
        canvas_polygon(c, mid, 7, B('2'));
        canvas_polygon(c, top, 6, B('1'));

        /* jaw plates */
        for(int i = 0; i < 4; ++i)
        {
            double x = 16 + i * 10;
            const point jaw[] = { { x, 46 }, { x + 8, 44 }, { x + 8, 54 }, { x + 2, 52 } };
            canvas_polygon(c, jaw, 4, B('4'));
        }

        /* teal circuitry */
        canvas_line(c, 40, 16, 62, 16, B('t'));
        canvas_line(c, 44, 48, 62, 48, B('t'));
        canvas_line(c, 62, 16, 62, 48, B('t'));

        /* eye socket */
        canvas_ellipse(c, 24, 32, 12, 9, B('4'));

        if(f == 0)
        {
            canvas_ellipse(c, 24, 32, 10, 3, B('m'));
            canvas_line(c, 14, 32, 34, 32, B('r'));
        }
        else
        {
            canvas_ellipse(c, 24, 32, 10, 7.5, f == 1 ? B('W') : B('o'));
            canvas_ellipse(c, 22, 32, 5.5, 6, f == 1 ? B('r') : B('m'));
            canvas_ellipse(c, 21, 31, 2.5, 3.5, f == 1 ? B('4') : B('y'));
            canvas_set(c, 19, 29, B('W'));
        }

        canvas_outline(c, B('K'), 0);
        frames[f] = c;
    }

    *frame_h = 64;
    return canvas_vstack(frames, 3);
}

static canvas* boss_overmind_rear(int* frame_h)
{
    canvas* frames[2];

    for(int f = 0; f < 2; ++f)
    {
        canvas* c = canvas_new(64, 64);
        const point outer[] = { { 0, 4 }, { 40, 8 }, { 56, 18 }, { 56, 46 }, { 40, 56 }, { 0, 60 } };
        const point inner[] = { { 0, 8 }, { 38, 11 }, { 52, 20 }, { 52, 44 }, { 38, 53 }, { 0, 56 } };
        canvas_polygon(c, outer, 6, B('3'));
        canvas_polygon(c, inner, 6, B('2'));
        canvas_rect(c, 0, 12, 40, 6, B('1'));
        static const int stripe_ys[3] = { 24, 32, 40 };

        for(int k = 0; k < 3; ++k)
        {
            canvas_rect(c, 4, stripe_ys[k], 34, 3, B('4'));
            canvas_rect(c, 6, stripe_ys[k] + 1, 30, 1, f == 0 ? B('t') : B('c'));
        }

        /* engine nozzles */
        static const int nozzle_ys[3] = { 14, 28, 42 };

        for(int k = 0; k < 3; ++k)
        {
            int y = nozzle_ys[k];
            canvas_rect(c, 52, y, 8, 8, B('4'));
            canvas_rect(c, 58, y + 1, 6, 6, f == 0 ? B('o') : B('y'));
            canvas_rect(c, 60, y + 2, 4, 4, f == 0 ? B('y') : B('W'));
        }

        canvas_outline(c, B('K'), 0);
        frames[f] = c;
    }

    *frame_h = 64;
    return canvas_vstack(frames, 2);
}

/* Small detachable turret pod used by the final boss. */
static canvas* boss_pod(int* frame_h)
{
    canvas* frames[2];

    for(int f = 0; f < 2; ++f)
    {
        canvas* c = canvas_new(16, 16);
        canvas_ellipse(c, 7.5, 7.5, 6.5, 5.5, B('3'));
        canvas_ellipse(c, 7.5, 6.5, 5, 3.5, B('2'));
        canvas_rect(c, 0, 7, 5, 2, B('4'));
        canvas_ellipse(c, 7.5, 7.5, 2.5, 2.5, f == 0 ? B('r') : B('y'));
        canvas_outline(c, B('K'), 0);
        frames[f] = c;
    }

    *frame_h = 16;
    return canvas_vstack(frames, 2);
}

/* Order is the SPR_* enum order in the generated header. */
const sprite_def sprite_defs[] = {
    { "player", player, 0 },
    { "life_icon", life_icon, 0 },
    { "enemy_dart", dart, 0 },
    { "enemy_waver", waver, 0 },
    { "enemy_interceptor", interceptor, 0 },
    { "enemy_turret", turret, 0 },
    { "enemy_hulk", hulk, 0 },
    { "enemy_swarm", swarm, 0 },
    { "enemy_mine", mine, 0 },
    { "asteroid_small", asteroid_small, 0 },
    { "asteroid_big", asteroid_big, 0 },
    { "shot_normal", player_shot, 0 },
    { "shot_spread", spread_shot, 0 },
    { "shot_missile", missile, 0 },
    { "shot_charge", charge_shot, 0 },
    { "shot_laser", laser, 0 },
    { "shooter", shooter, 0 },
    { "bullet_small", enemy_bullet, 0 },
    { "bullet_big", enemy_bullet_big, 0 },
    { "bullet_needle", enemy_needle, 0 },
    { "explosion_small", explosion_small, 0 },
    { "explosion_big", explosion_big, 0 },
    { "spark", spark, 0 },
    { "shield", shield, 0 },
    { "charge_glow", charge_glow, 0 },
    { "powerup", powerups, 0 },
    { "boss_bar", boss_bar, 0 },
    { "boss_warden", boss_warden, 1 },
    { "boss_hive", boss_hive, 1 },
    { "boss_overmind_front", boss_overmind_front, 1 },
    { "boss_overmind_rear", boss_overmind_rear, 1 },
    { "boss_pod", boss_pod, 1 },
};
const int sprite_def_count = (int) (sizeof(sprite_defs) / sizeof(sprite_defs[0]));
