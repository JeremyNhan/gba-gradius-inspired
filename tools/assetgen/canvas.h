/*
 * Tiny indexed-colour pixel-art toolkit used by the asset generator (host program, C99).
 *
 * Everything is deterministic. Rounding follows the original art definitions: round() is
 * round-half-to-even (nearbyint in the default rounding mode) and (int) truncates toward zero.
 */
#ifndef AG_CANVAS_H
#define AG_CANVAS_H

#include <stdint.h>

typedef struct
{
    int w;
    int h;
    uint8_t* px;        /* w * h palette indices, row-major; 0 = transparent */
} canvas;

typedef struct
{
    double x;
    double y;
} point;

canvas* canvas_new(int w, int h);
canvas* canvas_copy(const canvas* c);
void canvas_free(canvas* c);

int canvas_get(const canvas* c, int x, int y);
void canvas_set(canvas* c, int x, int y, int color);

void canvas_rect(canvas* c, int x0, int y0, int w, int h, int color);
void canvas_ellipse(canvas* c, double cx, double cy, double rx, double ry, int color);
void canvas_ring(canvas* c, double cx, double cy, double r_out, double r_in, int color);
void canvas_polygon(canvas* c, const point* pts, int count, int color);
void canvas_line(canvas* c, double x0, double y0, double x1, double y1, int color);

/* ASCII art: rows of equal length; '.' and ' ' are transparent; other characters map through map[]. */
void canvas_blit_ascii(canvas* c, const char* const* rows, int row_count, const int* map, int ox, int oy);
void canvas_blit(canvas* dst, const canvas* src, int ox, int oy);

/* Paints every transparent pixel that touches an opaque one (4 or 8 neighbours). */
void canvas_outline(canvas* c, int color, int diagonal);
void canvas_replace(canvas* c, int from, int to);
canvas* canvas_rotated90(const canvas* c, int times);

/* Stacks equally sized frames vertically (takes ownership of the frames). */
canvas* canvas_vstack(canvas** frames, int count);

/* Python-compatible helpers. */
int ag_round(double v);             /* round half to even */
int ag_mod(int a, int b);           /* result has the sign of b (Python %) */

typedef struct
{
    uint32_t s;
} lcg;

void lcg_init(lcg* g, uint32_t seed);
uint32_t lcg_next(lcg* g);
int lcg_randint(lcg* g, int a, int b);
int lcg_chance(lcg* g, int num, int den);

#endif
