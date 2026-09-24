#include "ss_video.h"

#include <string.h>

int video_camera_x;
int video_camera_y;

static int sprite_tiles[GEN_SPRITE_COUNT];     /* OBJ tile of each sheet (bosses: only when loaded) */
static u16 scroll[4][2];
static u16 dispcnt;
static u16 bldcnt;
static u16 bldy;

static bool is_boss_sheet(int sheet)
{
    return gen_sprites[sheet].boss_palette;
}

static int sheet_tiles(int sheet)
{
    return gen_sprites[sheet].tiles_per_frame * gen_sprites[sheet].frames;
}

static void copy_sheet(int sheet, int tile)
{
    memcpy32(&tile_mem_obj[0][tile], gen_sprites[sheet].tiles, (uint) sheet_tiles(sheet) * 8);
    sprite_tiles[sheet] = tile;
}

void video_init(void)
{
    REG_DISPCNT = DCNT_MODE0 | DCNT_BLANK;

    /* Common sprite sheets, packed from OBJ tile 0 (bosses are loaded per stage). */
    int tile = 0;

    for(int sheet = 0; sheet < GEN_SPRITE_COUNT; ++sheet)
    {
        if(! is_boss_sheet(sheet))
        {
            copy_sheet(sheet, tile);
            tile += sheet_tiles(sheet);
        }
    }

    memcpy16(pal_obj_bank[PAL_OBJ_MASTER], gen_pal_master, 16);
    memcpy16(pal_obj_bank[PAL_OBJ_BOSS], gen_pal_boss, 16);
    memcpy16(pal_obj_bank[PAL_OBJ_FLASH], gen_pal_flash, 16);

    for(int k = 0; k < 4; ++k)
    {
        memcpy16(pal_bg_bank[PAL_BG_FONT + k], gen_pal_font[k], 16);
    }

    /* Stars never change: load once. Colour 0 of BG bank 0 is the screen backdrop (deep space). */
    const gen_bg* stars = &gen_bgs[GEN_BG_STARS];
    memcpy32(&tile_mem[0][0], stars->tiles, (uint) stars->tile_count * 8);
    memcpy16(pal_bg_bank[PAL_BG_STARS], stars->palette, 16);

    for(int i = 0; i < 1024; ++i)
    {
        se_mem[SBB_STARS][i] = (u16) (stars->map[i] | SE_PALBANK(PAL_BG_STARS));
    }

    REG_BG0CNT = BG_CBB(2) | BG_SBB(SBB_TEXT) | BG_4BPP | BG_REG_32x32 | BG_PRIO(0);
    REG_BG1CNT = BG_CBB(0) | BG_SBB(SBB_TERRAIN) | BG_4BPP | BG_REG_32x32 | BG_PRIO(1);
    REG_BG2CNT = BG_CBB(0) | BG_SBB(SBB_BACKDROP) | BG_4BPP | BG_REG_32x32 | BG_PRIO(2);
    REG_BG3CNT = BG_CBB(0) | BG_SBB(SBB_STARS) | BG_4BPP | BG_REG_32x32 | BG_PRIO(3);

    oam_init(oam_mem, 128);
    dispcnt = DCNT_MODE0 | DCNT_OBJ | DCNT_OBJ_1D | DCNT_BG0 | DCNT_BG3;
    REG_DISPCNT = dispcnt;
}

void video_load_boss_graphics(int stage)
{
    static const int boss_sheets[STAGE_COUNT][4] = {
        { GEN_SPR_BOSS_WARDEN, -1, -1, -1 },
        { GEN_SPR_BOSS_HIVE, -1, -1, -1 },
        { GEN_SPR_BOSS_OVERMIND_FRONT, GEN_SPR_BOSS_OVERMIND_REAR, GEN_SPR_BOSS_POD, -1 },
    };
    int tile = VRAM_TILE_BOSS;

    for(int k = 0; k < 4 && boss_sheets[stage][k] >= 0; ++k)
    {
        copy_sheet(boss_sheets[stage][k], tile);
        tile += sheet_tiles(boss_sheets[stage][k]);
    }
}

int video_sprite_tile(int sheet)
{
    return sprite_tiles[sheet];
}

static void load_bg(int gen_index, int sbb, int tile_base, int palbank)
{
    const gen_bg* bg = &gen_bgs[gen_index];
    memcpy32(&tile_mem[0][tile_base], bg->tiles, (uint) bg->tile_count * 8);
    memcpy16(pal_bg_bank[palbank], bg->palette, 16);

    for(int i = 0; i < 1024; ++i)
    {
        se_mem[sbb][i] = (u16) ((bg->map[i] + tile_base) | SE_PALBANK(palbank));
    }
}

void video_show_backgrounds(backdrop_type backdrop)
{
    /* BG1 is shared: the title logo here, the terrain in stages (video_show_terrain). */
    dispcnt &= (u16) ~DCNT_BG2;

    switch(backdrop)
    {
    case BACKDROP_NEBULA:
        load_bg(GEN_BG_NEBULA, SBB_BACKDROP, VRAM_TILE_BACKDROP, PAL_BG_BACKDROP);
        break;

    case BACKDROP_CAVE:
        load_bg(GEN_BG_CAVE, SBB_BACKDROP, VRAM_TILE_BACKDROP, PAL_BG_BACKDROP);
        break;

    case BACKDROP_HULL:
        load_bg(GEN_BG_HULL, SBB_BACKDROP, VRAM_TILE_BACKDROP, PAL_BG_BACKDROP);
        break;

    case BACKDROP_TITLE_LOGO:
        /* The logo sits on BG1 (in front of the stars, behind the text). */
        load_bg(GEN_BG_TITLE_LOGO, SBB_TERRAIN, VRAM_TILE_BACKDROP, PAL_BG_BACKDROP);
        REG_BG1CNT = BG_CBB(0) | BG_SBB(SBB_TERRAIN) | BG_4BPP | BG_REG_32x32 | BG_PRIO(1);
        dispcnt |= DCNT_BG1;
        return;

    default:
        return;
    }

    dispcnt |= DCNT_BG2;
}

void video_show_terrain(int style)
{
    if(style < 0)
    {
        dispcnt &= (u16) ~DCNT_BG1;
        return;
    }

    memcpy32(&tile_mem[0][VRAM_TILE_TERRAIN], gen_terrain_tiles[style], 7 * 8);
    memcpy16(pal_bg_bank[PAL_BG_TERRAIN], gen_pal_terrain[style], 16);
    memset16(se_mem[SBB_TERRAIN], (u16) (VRAM_TILE_TERRAIN | SE_PALBANK(PAL_BG_TERRAIN)), 1024);
    dispcnt |= DCNT_BG1;
}

void video_set_scroll(int layer, int x, int y)
{
    scroll[layer][0] = (u16) x;
    scroll[layer][1] = (u16) y;
}

void video_set_fade(int level, bool white, int layers)
{
    level = SS_CLAMP(level, 0, 16);
    bldcnt = level ? (u16) (layers | (white ? BLD_WHITE : BLD_BLACK)) : 0;
    bldy = (u16) level;
}

void video_commit(void)
{
    /* The text layer (BG0) is the HUD: it never shakes. */
    REG_BG0HOFS = scroll[0][0];
    REG_BG0VOFS = scroll[0][1];

    for(int layer = 1; layer < 4; ++layer)
    {
        REG_BG_OFS[layer].x = (u16) (scroll[layer][0] + video_camera_x);
        REG_BG_OFS[layer].y = (u16) (scroll[layer][1] + video_camera_y);
    }

    REG_BLDCNT = bldcnt;
    REG_BLDY = bldy;
    REG_DISPCNT = dispcnt;
}
