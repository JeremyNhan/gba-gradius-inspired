#ifndef SS_SCREENS_H
#define SS_SCREENS_H

#include "ss_base.h"

#define GAME_VERSION "V0.0.1 ALPHA"

/* Title: logo, scrolling stars, cruising ship, hi-score. Returns true when START is pressed. */
void title_init(int hiscore);
bool title_update(void);
void title_render(void);
int title_selected_stage(void);     /* debug builds: L/R choose the starting stage */

/* Ending: story, credits and final score pages. Returns true when the player leaves it. */
void ending_init(void);
bool ending_update(void);

#endif
