/*
 * Video setup and per-frame commit (mode 0, four regular backgrounds, 1D 4bpp sprites).
 *
 * VRAM layout (docs/architecture.md):
 *   BG charblocks 0-1 (1024 tiles): stars at tile 0, backdrop / title logo at VRAM_TILE_BACKDROP,
 *                                   terrain at VRAM_TILE_TERRAIN
 *   BG charblocks 2-3: text layer tiles (20 rows x 30 tiles, see ss_text.c)
 *   screenblocks 28-31: stars, backdrop / logo, terrain, text maps (32x32 each)
 *   OBJ VRAM (1024 tiles): every common sprite sheet from tile 0, the stage boss from VRAM_TILE_BOSS
 *
 * Layers: BG0 text/HUD (priority 0), BG1 terrain or title logo (1), BG2 backdrop (2), BG3 stars (3).
 * Gameplay sprites use priority 1 (in front of the terrain), HUD sprites priority 0.
 */
#ifndef SS_VIDEO_H
#define SS_VIDEO_H

#include "ss_base.h"
#include "gen_gfx.h"

#define SBB_STARS 28
#define SBB_BACKDROP 29
#define SBB_TERRAIN 30
#define SBB_TEXT 31

#define VRAM_TILE_BACKDROP 128
#define VRAM_TILE_TERRAIN 560
#define VRAM_TILE_BOSS 512          /* OBJ tile where the current boss sheets start */

/* BG palette banks. */
#define PAL_BG_STARS 0
#define PAL_BG_BACKDROP 1
#define PAL_BG_TERRAIN 2
#define PAL_BG_FONT 3               /* 3..6: white, yellow, cyan, red */

/* OBJ palette banks. */
#define PAL_OBJ_MASTER 0
#define PAL_OBJ_BOSS 1
#define PAL_OBJ_FLASH 2

typedef enum
{
    BACKDROP_NONE,
    BACKDROP_NEBULA,
    BACKDROP_CAVE,
    BACKDROP_HULL,
    BACKDROP_TITLE_LOGO
} backdrop_type;

void video_init(void);

/* Loads the boss sheets used by one stage (0..2) into OBJ VRAM at VRAM_TILE_BOSS. */
void video_load_boss_graphics(int stage);

/* OBJ tile index of a sprite sheet's first tile. */
int video_sprite_tile(int sheet);

/* BG2 backdrop, or the title logo on BG1 (BACKDROP_NONE hides BG2; BG1 is left to the terrain). */
void video_show_backgrounds(backdrop_type backdrop);
void video_show_terrain(int style);         /* style: 0 crystal, 1 metal, -1 off */

/* Scroll positions in pixels (map pixel (x, y) shown at the screen's top-left), applied in VBlank. */
void video_set_scroll(int layer, int x, int y);

/* Camera shake offset (pixels), added to every BG scroll and subtracted from gameplay sprites. */
extern int video_camera_x;
extern int video_camera_y;

/* Hardware brightness fade: level 0 (off) .. 16 (full), towards black or white, on a set of layers. */
#define FADE_BG0 BLD_BG0
#define FADE_WORLD (BLD_BG1 | BLD_BG2 | BLD_BG3 | BLD_OBJ | BLD_BACKDROP)
#define FADE_ALL (FADE_WORLD | BLD_BG0)
void video_set_fade(int level, bool white, int layers);

/* Copies the frame's display state to the hardware. Call at the start of VBlank. */
void video_commit(void);

#endif
