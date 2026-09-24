/*
 * Per-frame sprite list. Nothing is allocated: every system draws its sprites into a shadow OAM
 * each frame (in front-to-back order: HUD first, boss last), and video's VBlank commit copies it
 * to OAM. Earlier sprites are drawn in front of later ones (GBA OAM priority). If more than 128
 * sprites are drawn in a frame the rest are dropped and counted.
 */
#ifndef SS_SPRITES_H
#define SS_SPRITES_H

#include "ss_base.h"

#define SPR_HFLIP 1
#define SPR_VFLIP 2
#define SPR_HUD 4               /* priority 0 (above the text layer), ignores the camera shake */
#define SPR_FLASH 8             /* white hit-flash palette */

void sprites_begin(void);

/* Draws frame `frame` of a generated sheet centred on pos (game coordinates). */
void sprites_draw(int sheet, int frame, vec2 pos, int flags);

/* Same with whole-pixel centre coordinates. */
void sprites_draw_px(int sheet, int frame, int cx, int cy, int flags);

/* Hides the unused entries and copies the list to OAM (call in VBlank). */
void sprites_commit(void);

int sprites_used(void);         /* sprites drawn in the last finished frame */
int sprites_dropped(void);      /* total sprites dropped because OAM was full */

#endif
