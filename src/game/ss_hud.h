/*
 * Gameplay HUD: score/hi-score/lives (text row 0), loadout status line (row 19), boss bar (sprites),
 * stage banner, boss warning and pickup labels. Text rendering costs CPU, so at most one text item
 * is redrawn per frame (pickup label, then status line, then score).
 */
#ifndef SS_HUD_H
#define SS_HUD_H

#include "ss_base.h"

void hud_init(void);
void hud_update(void);
void hud_render(void);                  /* sprites: life icon, boss bar */
void hud_set_visible(bool visible);
void hud_show_banner(const char* title, const char* subtitle);
void hud_show_warning(void);
void hud_show_pickup(const char* label);    /* label must be a static string */
void hud_toggle_debug(void);

#endif
