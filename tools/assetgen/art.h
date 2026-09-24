/*
 * Art sources for Space Shooter: palettes, font, sprite and background generators.
 * Every generator returns a vertical strip of frames (sprites) or a full image (backgrounds).
 */
#ifndef AG_ART_H
#define AG_ART_H

#include "canvas.h"

typedef struct
{
    uint8_t r, g, b;
} rgb;

typedef struct
{
    const char* keys;       /* one ASCII-art key per colour ('.' = index 0) */
    const rgb* colors;
    int count;
} palette;

/* ----- palettes (palettes.c) --------------------------------------------------------------------- */
extern const palette pal_master;        /* shared gameplay sprite palette */
extern const palette pal_boss;
extern const palette pal_stars;
extern const palette pal_nebula;
extern const palette pal_cave;
extern const palette pal_hull;
extern const palette pal_logo;
extern const palette pal_terrain_crystal;
extern const palette pal_terrain_metal;
extern const rgb pal_font[4][4];        /* white, yellow, cyan, red: transparent, shadow, face, face dark */

/* Character -> colour index for a palette's ASCII-art keys (256 entries, -1 if unused). */
void palette_charmap(const palette* p, int* map256);

/* Colour index of key `k` in the palette (aborts if missing). */
int pal_index(const palette* p, char k);

/* ----- font (font.c) ------------------------------------------------------------------------------ */
#define FONT_FIRST '!'
#define FONT_GLYPHS 94
#define FONT_PITCH 6            /* 5 px glyph + 1 px shadow: fixed-pitch HUD text */

/* 7 rows of 5 characters ('#' or '.') for a character, or NULL if it has no glyph. */
const char* const* font_rows(char ch);

/* 8x8 glyph cell: 1 = shadow, 2 = face, 3 = face (bottom row). */
canvas* font_glyph(char ch);

/* ----- sprites (sprites.c) ------------------------------------------------------------------------- */
typedef canvas* (*sprite_fn)(int* frame_h);

typedef struct
{
    const char* name;
    sprite_fn make;
    int boss_palette;       /* 0: master palette, 1: boss palette */
} sprite_def;

extern const sprite_def sprite_defs[];
extern const int sprite_def_count;

/* ----- backgrounds (backgrounds.c) ----------------------------------------------------------------- */
typedef struct
{
    const char* name;
    canvas* (*make)(void);
    const palette* pal;
} bg_def;

extern const bg_def bg_defs[];
extern const int bg_def_count;

/* 7 terrain tiles as an 8x56 strip: empty, fill, fill alt, floor edge, floor edge alt, ceiling
 * edge, ceiling edge alt. style: 0 crystal, 1 metal. */
canvas* terrain_tiles(int style);

#endif
