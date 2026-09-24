#include "ss_sprites.h"

#include "ss_video.h"

static OBJ_ATTR shadow[128];
static int count;
static int last_count;
static int dropped;

void sprites_begin(void)
{
    count = 0;
}

void sprites_draw_px(int sheet, int frame, int cx, int cy, int flags)
{
    const gen_sprite* s = &gen_sprites[sheet];

    if(! (flags & SPR_HUD))
    {
        cx -= video_camera_x;
        cy -= video_camera_y;
    }

    int x = cx - s->width / 2 + HALF_W;
    int y = cy - s->height / 2 + HALF_H;

    /* Cull sprites that are completely off screen (OAM coordinates wrap). */
    if(x <= -s->width || x >= SCREEN_W || y <= -s->height || y >= SCREEN_H)
    {
        return;
    }

    if(count >= 128)
    {
        ++dropped;
        return;
    }

    int tile = video_sprite_tile(sheet) + frame * s->tiles_per_frame;
    int palbank = (flags & SPR_FLASH) ? PAL_OBJ_FLASH : s->boss_palette ? PAL_OBJ_BOSS : PAL_OBJ_MASTER;
    OBJ_ATTR* o = &shadow[count++];
    o->attr0 = (u16) (ATTR0_Y(y & 0xFF) | ATTR0_SHAPE(s->shape) | ATTR0_4BPP);
    o->attr1 = (u16) (ATTR1_X(x & 0x1FF) | ATTR1_SIZE(s->size) | ((flags & SPR_HFLIP) ? ATTR1_HFLIP : 0) |
                      ((flags & SPR_VFLIP) ? ATTR1_VFLIP : 0));
    o->attr2 = (u16) (ATTR2_ID(tile) | ATTR2_PRIO((flags & SPR_HUD) ? 0 : 1) | ATTR2_PALBANK(palbank));
}

void sprites_draw(int sheet, int frame, vec2 pos, int flags)
{
    sprites_draw_px(sheet, frame, fx_round(pos.x), fx_round(pos.y), flags);
}

void sprites_commit(void)
{
    /* Called in VBlank: the list is the one drawn by the previous frame's logic. */
    oam_copy(oam_mem, shadow, (uint) count);

    for(int i = count; i < last_count; ++i)
    {
        oam_mem[i].attr0 = ATTR0_HIDE;
    }

    last_count = count;
}

int sprites_used(void)
{
    return last_count;
}

int sprites_dropped(void)
{
    return dropped;
}
