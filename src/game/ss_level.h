/*
 * Stage scenery and timeline: terrain (streamed BG1 walls + collision), parallax backgrounds and
 * the stage event runner.
 */
#ifndef SS_LEVEL_H
#define SS_LEVEL_H

#include "ss_base.h"
#include "ss_stage_data.h"

/* ----- terrain ------------------------------------------------------------------------------------ */
void terrain_init(const stage_def* stage);
void terrain_update(fx scroll_x);
void terrain_commit(void);          /* VBlank: writes the map columns that scrolled in */
bool terrain_enabled(void);
bool terrain_blocks(hitbox box, fx scroll_x);
int terrain_floor_y(fx screen_x, fx scroll_x);
int terrain_ceiling_y(fx screen_x, fx scroll_x);

/* ----- parallax backgrounds ------------------------------------------------------------------------ */
void backgrounds_init(backdrop_type backdrop);
void backgrounds_update(fx scroll_x);

/* ----- stage timeline ------------------------------------------------------------------------------ */
void runner_reset(void);
void runner_update(void);
void runner_skip_to_boss(void);
bool runner_boss_started(void);

#endif
