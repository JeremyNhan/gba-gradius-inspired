#include "canvas.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

canvas* canvas_new(int w, int h)
{
    canvas* c = (canvas*) malloc(sizeof(canvas));
    c->w = w;
    c->h = h;
    c->px = (uint8_t*) calloc((size_t) (w * h), 1);

    if(! c->px)
    {
        fprintf(stderr, "assetgen: out of memory\n");
        exit(1);
    }

    return c;
}

canvas* canvas_copy(const canvas* c)
{
    canvas* out = canvas_new(c->w, c->h);
    memcpy(out->px, c->px, (size_t) (c->w * c->h));
    return out;
}

void canvas_free(canvas* c)
{
    if(c)
    {
        free(c->px);
        free(c);
    }
}

int canvas_get(const canvas* c, int x, int y)
{
    if(x >= 0 && x < c->w && y >= 0 && y < c->h)
    {
        return c->px[y * c->w + x];
    }

    return 0;
}

void canvas_set(canvas* c, int x, int y, int color)
{
    if(x >= 0 && x < c->w && y >= 0 && y < c->h)
    {
        c->px[y * c->w + x] = (uint8_t) color;
    }
}

void canvas_rect(canvas* c, int x0, int y0, int w, int h, int color)
{
    for(int y = y0; y < y0 + h; ++y)
    {
        for(int x = x0; x < x0 + w; ++x)
        {
            canvas_set(c, x, y, color);
        }
    }
}

void canvas_ellipse(canvas* c, double cx, double cy, double rx, double ry, int color)
{
    double srx = rx > 0.01 ? rx : 0.01;
    double sry = ry > 0.01 ? ry : 0.01;

    for(int y = (int) (cy - ry) - 1; y < (int) (cy + ry) + 2; ++y)
    {
        for(int x = (int) (cx - rx) - 1; x < (int) (cx + rx) + 2; ++x)
        {
            double dx = (x + 0.5 - cx) / srx;
            double dy = (y + 0.5 - cy) / sry;

            if(dx * dx + dy * dy <= 1.0)
            {
                canvas_set(c, x, y, color);
            }
        }
    }
}

void canvas_ring(canvas* c, double cx, double cy, double r_out, double r_in, int color)
{
    for(int y = (int) (cy - r_out) - 1; y < (int) (cy + r_out) + 2; ++y)
    {
        for(int x = (int) (cx - r_out) - 1; x < (int) (cx + r_out) + 2; ++x)
        {
            double d = hypot(x + 0.5 - cx, y + 0.5 - cy);

            if(r_in <= d && d <= r_out)
            {
                canvas_set(c, x, y, color);
            }
        }
    }
}

static int compare_doubles(const void* a, const void* b)
{
    double da = *(const double*) a;
    double db = *(const double*) b;
    return (da > db) - (da < db);
}

void canvas_polygon(canvas* c, const point* pts, int count, int color)
{
    /* Even-odd scanline fill with pixel-centre sampling. */
    double min_y = pts[0].y;
    double max_y = pts[0].y;

    for(int i = 1; i < count; ++i)
    {
        min_y = pts[i].y < min_y ? pts[i].y : min_y;
        max_y = pts[i].y > max_y ? pts[i].y : max_y;
    }

    for(int y = (int) min_y; y < (int) ceil(max_y) + 1; ++y)
    {
        double sy = y + 0.5;
        double xs[64];
        int n = 0;

        for(int i = 0; i < count; ++i)
        {
            point p1 = pts[i];
            point p2 = pts[(i + 1) % count];

            if((p1.y <= sy && sy < p2.y) || (p2.y <= sy && sy < p1.y))
            {
                xs[n++] = p1.x + (sy - p1.y) * (p2.x - p1.x) / (p2.y - p1.y);
            }
        }

        qsort(xs, (size_t) n, sizeof(double), compare_doubles);

        for(int i = 0; i + 1 < n; i += 2)
        {
            for(int x = (int) ceil(xs[i] - 0.5); x < (int) floor(xs[i + 1] - 0.5) + 1; ++x)
            {
                canvas_set(c, x, y, color);
            }
        }
    }
}

void canvas_line(canvas* c, double x0, double y0, double x1, double y1, int color)
{
    double ax = fabs(x1 - x0);
    double ay = fabs(y1 - y0);
    int steps = (int) (ax > ay ? ax : ay) + 1;

    for(int i = 0; i < steps + 1; ++i)
    {
        double t = (double) i / steps;
        canvas_set(c, ag_round(x0 + (x1 - x0) * t), ag_round(y0 + (y1 - y0) * t), color);
    }
}

void canvas_blit_ascii(canvas* c, const char* const* rows, int row_count, const int* map, int ox, int oy)
{
    for(int y = 0; y < row_count; ++y)
    {
        for(int x = 0; rows[y][x]; ++x)
        {
            char ch = rows[y][x];

            if(ch != '.' && ch != ' ')
            {
                canvas_set(c, ox + x, oy + y, map[(unsigned char) ch]);
            }
        }
    }
}

void canvas_blit(canvas* dst, const canvas* src, int ox, int oy)
{
    for(int y = 0; y < src->h; ++y)
    {
        for(int x = 0; x < src->w; ++x)
        {
            int v = src->px[y * src->w + x];

            if(v)
            {
                canvas_set(dst, ox + x, oy + y, v);
            }
        }
    }
}

void canvas_outline(canvas* c, int color, int diagonal)
{
    static const int nbrs[8][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 }, { 1, 1 }, { -1, -1 }, { 1, -1 },
                                    { -1, 1 } };
    canvas* src = canvas_copy(c);
    int count = diagonal ? 8 : 4;

    for(int y = 0; y < c->h; ++y)
    {
        for(int x = 0; x < c->w; ++x)
        {
            if(src->px[y * c->w + x] != 0)
            {
                continue;
            }

            for(int k = 0; k < count; ++k)
            {
                if(canvas_get(src, x + nbrs[k][0], y + nbrs[k][1]))
                {
                    c->px[y * c->w + x] = (uint8_t) color;
                    break;
                }
            }
        }
    }

    canvas_free(src);
}

void canvas_replace(canvas* c, int from, int to)
{
    for(int i = 0; i < c->w * c->h; ++i)
    {
        if(c->px[i] == from)
        {
            c->px[i] = (uint8_t) to;
        }
    }
}

canvas* canvas_rotated90(const canvas* c, int times)
{
    canvas* out = canvas_copy(c);

    for(int k = 0; k < ((times % 4) + 4) % 4; ++k)
    {
        canvas* r = canvas_new(out->h, out->w);

        /* r.px[x][out.h - 1 - y] = out.px[y][x]: new row x, new column out.h - 1 - y */
        for(int y = 0; y < out->h; ++y)
        {
            for(int x = 0; x < out->w; ++x)
            {
                r->px[x * r->w + (out->h - 1 - y)] = out->px[y * out->w + x];
            }
        }

        canvas_free(out);
        out = r;
    }

    return out;
}

canvas* canvas_vstack(canvas** frames, int count)
{
    int w = frames[0]->w;
    int h = 0;

    for(int i = 0; i < count; ++i)
    {
        if(frames[i]->w != w)
        {
            fprintf(stderr, "assetgen: frame width mismatch\n");
            exit(1);
        }

        h += frames[i]->h;
    }

    canvas* out = canvas_new(w, h);
    int y = 0;

    for(int i = 0; i < count; ++i)
    {
        memcpy(out->px + y * w, frames[i]->px, (size_t) (w * frames[i]->h));
        y += frames[i]->h;
        canvas_free(frames[i]);
    }

    return out;
}

int ag_round(double v)
{
    return (int) nearbyint(v);
}

int ag_mod(int a, int b)
{
    int r = a % b;
    return (r != 0 && ((r < 0) != (b < 0))) ? r + b : r;
}

void lcg_init(lcg* g, uint32_t seed)
{
    g->s = seed;
}

uint32_t lcg_next(lcg* g)
{
    g->s = g->s * 1664525u + 1013904223u;
    return g->s >> 8;
}

int lcg_randint(lcg* g, int a, int b)
{
    return a + (int) (lcg_next(g) % (uint32_t) (b - a + 1));
}

int lcg_chance(lcg* g, int num, int den)
{
    return (int) (lcg_next(g) % (uint32_t) den) < num;
}
