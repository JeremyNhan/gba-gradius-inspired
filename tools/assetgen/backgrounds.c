/*
 * Procedural background art. All BGs are 4bpp (16 colours) 256x256 regular backgrounds that wrap
 * horizontally, so the game can scroll them forever with the hardware scroll registers.
 */
#include "art.h"

#include <math.h>
#include <stdlib.h>

#define PI 3.14159265358979323846

/* ----- far starfield (all stages and the title screen) ------------------------------------------ */

static canvas* starfield(void)
{
    static const int cross[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
    canvas* c = canvas_new(256, 256);
    lcg rng;
    lcg_init(&rng, 2026);

    for(int i = 0; i < 170; ++i)
    {
        int x = lcg_randint(&rng, 0, 255);
        int y = lcg_randint(&rng, 0, 255);
        int kind = lcg_randint(&rng, 0, 99);

        if(kind < 55)
        {
            canvas_set(c, x, y, 1);
        }
        else if(kind < 80)
        {
            canvas_set(c, x, y, 2);
        }
        else if(kind < 90)
        {
            canvas_set(c, x, y, lcg_randint(&rng, 5, 6));
        }
        else
        {
            /* small cross-shaped bright star */
            canvas_set(c, x, y, 4);

            for(int k = 0; k < 4; ++k)
            {
                canvas_set(c, ag_mod(x + cross[k][0], 256), ag_mod(y + cross[k][1], 256), kind < 97 ? 3 : 6);
            }
        }
    }

    return c;
}

/* ----- stage backdrops (parallax x0.5) ------------------------------------------------------------ */

/* Ordered 4x4 Bayer dithering: true if the pixel is 'on' for a 0..16 level. */
static int dither(int x, int y, int level)
{
    static const int bayer[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
    return bayer[y & 3][x & 3] < level;
}

static canvas* nebula_backdrop(void)
{
    canvas* c = canvas_new(256, 256);

    /* Soft nebula bands built from a few overlapping blobs, dithered into 4 tones. */
    static const double blobs[5][4] = {
        { 60, 70, 70, 26 }, { 150, 90, 60, 20 }, { 210, 60, 50, 18 }, { 90, 170, 80, 22 }, { 200, 190, 60, 20 },
    };
    static const int wraps[3] = { -256, 0, 256 };

    for(int y = 0; y < 256; ++y)
    {
        for(int x = 0; x < 256; ++x)
        {
            double v = 0.0;

            for(int b = 0; b < 5; ++b)
            {
                for(int w = 0; w < 3; ++w)
                {
                    double dx = (x - blobs[b][0] - wraps[w]) / blobs[b][2];
                    double dy = (y - blobs[b][1]) / blobs[b][3];
                    double d = dx * dx + dy * dy;

                    if(d < 1.0)
                    {
                        v += (1.0 - d);
                    }
                }
            }

            int level = (int) ((v < 1.0 ? v : 1.0) * 48);

            if(level <= 0)
            {
                continue;
            }

            int tone = level / 12 < 3 ? level / 12 : 3;
            int frac = (level % 12) * 16 / 12;

            if(dither(x, y, frac))
            {
                ++tone;
            }

            if(tone)
            {
                canvas_set(c, x, y, tone < 4 ? tone : 4);
            }
        }
    }

    /* Ringed planet in the upper part of the map. */
    int px = 188;
    int py = 128;
    int pr = 26;

    for(int y = py - pr; y < py + pr + 1; ++y)
    {
        for(int x = px - pr; x < px + pr + 1; ++x)
        {
            double d = hypot(x - px, y - py);

            if(d <= pr)
            {
                double light = (x - px) * -0.5 + (y - py) * -0.7;
                double shade = light / pr;
                int col = shade > 0.35 ? 7 : shade > -0.15 ? 8 : shade > -0.55 ? 9 : 12;

                /* horizontal cloud bands */
                if((y / 5) % 3 == 0 && (col == 7 || col == 8) && dither(x, y, 6))
                {
                    col = col == 7 ? 8 : 9;
                }

                canvas_set(c, x, y, col);
            }
        }
    }

    for(int x = px - 44; x < px + 45; ++x)
    {
        double t = (x - px) / 44.0;
        int y = py + ag_round(t * 9);

        if(abs(x - px) < pr - 2 && y < py)    /* ring hidden behind the planet's upper half */
        {
            continue;
        }

        canvas_set(c, x, y, 10);
        canvas_set(c, x, y + 1, 11);
    }

    return c;
}

/* Distant cave wall (visible band y 0..159): repeating stalactite/stalagmite silhouettes with
 * crystal glints. */
static canvas* cave_backdrop(void)
{
    canvas* c = canvas_new(256, 256);
    lcg rng;
    lcg_init(&rng, 99);

    /* 64-pixel wide repeating unit keeps the tile count small. */
    canvas* unit = canvas_new(64, 256);

    for(int x = 0; x < 64; ++x)
    {
        int top = 20 + (int) (8 * sin(x * 2 * PI / 64)) + (int) (5 * sin(x * 6 * PI / 64));
        int bot = 140 - (int) (8 * cos(x * 2 * PI / 64)) - (int) (5 * sin(x * 8 * PI / 64));

        for(int y = 0; y < top; ++y)
        {
            canvas_set(unit, x, y, y < top - 6 ? 2 : 3);
        }

        for(int y = bot; y < 160; ++y)
        {
            canvas_set(unit, x, y, y > bot + 6 ? 2 : 3);
        }
    }

    /* stalactites / stalagmites */
    static const int tites[4][2] = { { 8, 26 }, { 27, 14 }, { 44, 34 }, { 56, 18 } };

    for(int k = 0; k < 4; ++k)
    {
        int x = tites[k][0];
        int length = tites[k][1];

        for(int i = 0; i < length; ++i)
        {
            int w = 3 - i * 3 / length;
            w = w > 0 ? w : 0;
            int top = 20 + (int) (8 * sin(x * 2 * PI / 64));

            for(int dx = -w; dx < w + 1; ++dx)
            {
                canvas_set(unit, x + dx, top + i, dx > -w ? 3 : 4);
            }
        }
    }

    static const int mites[3][2] = { { 16, 20 }, { 36, 30 }, { 52, 12 } };

    for(int k = 0; k < 3; ++k)
    {
        int x = mites[k][0];
        int length = mites[k][1];
        int bot = 140 - (int) (8 * cos(x * 2 * PI / 64));

        for(int i = 0; i < length; ++i)
        {
            int w = 3 - i * 3 / length;
            w = w > 0 ? w : 0;

            for(int dx = -w; dx < w + 1; ++dx)
            {
                canvas_set(unit, x + dx, bot - i, dx > -w ? 3 : 4);
            }
        }
    }

    /* crystals embedded in the walls */
    for(int k = 0; k < 6; ++k)
    {
        int x = lcg_randint(&rng, 2, 61);
        int y = lcg_chance(&rng, 1, 2) ? lcg_randint(&rng, 2, 12) : lcg_randint(&rng, 150, 157);
        canvas_set(unit, x, y, 7);
        canvas_set(unit, x, y + 1, 6);
        canvas_set(unit, x - 1, y + 1, 5);
        canvas_set(unit, x + 1, y + 1, 5);
    }

    for(int ox = 0; ox < 256; ox += 64)
    {
        canvas_blit(c, unit, ox, 0);
    }

    canvas_free(unit);
    return c;
}

/* Interior of the enemy dreadnought (visible band y 0..159): girders and light strips. */
static canvas* hull_backdrop(void)
{
    canvas* c = canvas_new(256, 256);

    for(int y = 0; y < 256; ++y)
    {
        for(int x = 0; x < 256; ++x)
        {
            int ux = x % 32;
            int uy = y % 64;
            int col = 0;

            if(uy < 4 || uy >= 60)
            {
                col = 2;
            }

            if(ux == 0 || ux == 1)
            {
                col = 3;
            }

            if(ux == 2)
            {
                col = 1;
            }

            if(uy >= 28 && uy < 31 && ux > 3)
            {
                col = ux % 8 ? 7 : 8;
            }

            if(uy == 4 || uy == 59)
            {
                col = 4;
            }

            canvas_set(c, x, y, col);
        }
    }

    /* heavy bulkheads top & bottom */
    for(int y = 0; y < 160; ++y)
    {
        if(! (y < 22 || y >= 138))
        {
            continue;
        }

        for(int x = 0; x < 256; ++x)
        {
            int ux = x % 64;
            int edge = y == 21 || y == 138;
            int col = edge ? 4 : ((ux / 8) % 2 == 0 ? 3 : 2);

            if((y >= 8 && y < 12) || (y >= 148 && y < 152))
            {
                col = ((x + y) / 4) % 2 == 0 ? 6 : 5;
            }

            canvas_set(c, x, y, col);
        }
    }

    return c;
}

/* ----- terrain tiles (streamed by the game from stage height data) ------------------------------- */

static void hazard(canvas* tile, int y0, int rows)
{
    for(int y = y0; y < y0 + rows; ++y)
    {
        for(int x = 0; x < 8; ++x)
        {
            canvas_set(tile, x, y, ((x + y) / 2) % 2 == 0 ? 6 : 5);
        }
    }
}

canvas* terrain_tiles(int style)
{
    canvas* t[7];

    for(int i = 0; i < 7; ++i)
    {
        t[i] = canvas_new(8, 8);
    }

    if(style == 0)
    {
        for(int k = 1; k <= 2; ++k)
        {
            canvas_rect(t[k], 0, 0, 8, 8, 2);
            canvas_set(t[k], 7, 3, 8);
            canvas_set(t[k], 0, 7, 8);
        }

        static const int specks[4][2] = { { 1, 1 }, { 5, 4 }, { 2, 6 }, { 6, 1 } };

        for(int k = 0; k < 4; ++k)
        {
            canvas_set(t[1], specks[k][0], specks[k][1], 3);
        }

        static const int cracks[2][2] = { { 3, 2 }, { 1, 5 } };

        for(int k = 0; k < 2; ++k)
        {
            int x = cracks[k][0];
            int y = cracks[k][1];
            canvas_set(t[2], x, y, 1);
            canvas_set(t[2], x + 1, y, 1);
            canvas_set(t[2], x + 1, y + 1, 3);
        }

        /* floor edge: bright rim + crystal line on the exposed top */
        for(int k = 3; k <= 4; ++k)
        {
            canvas_rect(t[k], 0, 0, 8, 8, 2);
            canvas_rect(t[k], 0, 0, 8, 1, 7);
            canvas_rect(t[k], 0, 1, 8, 1, 6);
            canvas_rect(t[k], 0, 2, 8, 1, 4);
            canvas_rect(t[k], 0, 3, 8, 1, 3);
        }

        canvas_set(t[4], 2, 4, 5);
        canvas_set(t[4], 5, 5, 5);

        /* ceiling edge: same, mirrored (exposed bottom) */
        for(int k = 5; k <= 6; ++k)
        {
            canvas_rect(t[k], 0, 0, 8, 8, 2);
            canvas_rect(t[k], 0, 7, 8, 1, 7);
            canvas_rect(t[k], 0, 6, 8, 1, 6);
            canvas_rect(t[k], 0, 5, 8, 1, 4);
            canvas_rect(t[k], 0, 4, 8, 1, 3);
        }

        canvas_set(t[6], 3, 3, 5);
        canvas_set(t[6], 6, 2, 5);
    }
    else
    {
        for(int k = 1; k <= 2; ++k)
        {
            canvas_rect(t[k], 0, 0, 8, 8, 2);
            canvas_rect(t[k], 0, 0, 8, 1, 3);
            canvas_rect(t[k], 0, 0, 1, 8, 3);
            canvas_rect(t[k], 7, 0, 1, 8, 8);
            canvas_rect(t[k], 0, 7, 8, 1, 8);
        }

        canvas_set(t[1], 3, 3, 1);
        canvas_set(t[1], 4, 4, 3);
        canvas_rect(t[2], 2, 3, 4, 1, 7);

        /* floor edge: bright rim + hazard stripes on the exposed top */
        for(int k = 3; k <= 4; ++k)
        {
            canvas_rect(t[k], 0, 0, 8, 8, 2);
            canvas_rect(t[k], 0, 0, 8, 1, 4);
            hazard(t[k], 1, 3);
            canvas_rect(t[k], 0, 4, 8, 1, 1);
        }

        canvas_rect(t[4], 1, 6, 6, 1, 7);

        for(int k = 5; k <= 6; ++k)
        {
            canvas_rect(t[k], 0, 0, 8, 8, 2);
            canvas_rect(t[k], 0, 7, 8, 1, 4);
            hazard(t[k], 4, 3);
            canvas_rect(t[k], 0, 3, 8, 1, 1);
        }

        canvas_rect(t[6], 1, 1, 6, 1, 7);
    }

    canvas* strip = canvas_new(8, 56);

    for(int i = 0; i < 7; ++i)
    {
        canvas_blit(strip, t[i], 0, i * 8);
        canvas_free(t[i]);
    }

    return strip;
}

/* ----- title logo --------------------------------------------------------------------------------- */

/* Text from the 5x7 font as chunky blocks with a vertical colour gradient. */
static void big_text(canvas* c, const char* text, int ox, int oy, int scale, const int* gradient, int gradient_count,
                     int bevel)
{
    int x = ox;

    for(const char* p = text; *p; ++p)
    {
        if(*p == ' ')
        {
            x += 3 * scale;
            continue;
        }

        const char* const* rows = font_rows(*p);

        for(int gy = 0; gy < 7; ++gy)
        {
            for(int gx = 0; gx < 5; ++gx)
            {
                if(rows[gy][gx] != '#')
                {
                    continue;
                }

                for(int sy = 0; sy < scale; ++sy)
                {
                    for(int sx = 0; sx < scale; ++sx)
                    {
                        int py = gy * scale + sy;
                        double t = (double) py / (7 * scale);
                        int gi = (int) (t * gradient_count);
                        int col = gradient[gi < gradient_count - 1 ? gi : gradient_count - 1];

                        if(bevel && sy == 0 && (gy == 0 || rows[gy - 1][gx] != '#'))
                        {
                            col = 2;
                        }

                        canvas_set(c, x + gx * scale + sx, oy + py, col);
                    }
                }
            }
        }

        x += 6 * scale;
    }
}

/* Screen (sx, sy) maps to bitmap (sx + 8, sy + 48): the game scrolls this BG by (8, 48). */
static canvas* title_logo(void)
{
    static const int grad[7] = { 3, 3, 4, 4, 5, 5, 6 };
    static const int shadow_grad[1] = { 7 };
    canvas* c = canvas_new(256, 256);
    canvas* shadow = canvas_new(256, 256);
    big_text(shadow, "SPACE", 8 + 60 + 3, 48 + 14 + 3, 4, shadow_grad, 1, 0);
    big_text(shadow, "SHOOTER", 8 + 20 + 3, 48 + 50 + 3, 4, shadow_grad, 1, 0);
    canvas_blit(c, shadow, 0, 0);
    canvas_free(shadow);
    big_text(c, "SPACE", 8 + 60, 48 + 14, 4, grad, 7, 1);
    big_text(c, "SHOOTER", 8 + 20, 48 + 50, 4, grad, 7, 1);
    canvas_outline(c, 1, 0);

    /* thin cyan speed lines under the logo */
    static const int spans[2][2] = { { 8 + 24, 8 + 216 }, { 8 + 44, 8 + 196 } };

    for(int i = 0; i < 2; ++i)
    {
        int y = 48 + 84 + i * 3;

        for(int x = spans[i][0]; x < spans[i][1]; ++x)
        {
            canvas_set(c, x, y, i == 0 ? 8 : 9);
        }
    }

    return c;
}

/* Order is the BG_* enum order in the generated header. */
const bg_def bg_defs[] = {
    { "stars", starfield, &pal_stars },
    { "nebula", nebula_backdrop, &pal_nebula },
    { "cave", cave_backdrop, &pal_cave },
    { "hull", hull_backdrop, &pal_hull },
    { "title_logo", title_logo, &pal_logo },
};
const int bg_def_count = (int) (sizeof(bg_defs) / sizeof(bg_defs[0]));
