#include "ss_level.h"

#include "ss_audio.h"
#include "ss_boss.h"
#include "ss_enemies.h"
#include "ss_hud.h"
#include "ss_video.h"
#include "ss_world.h"

/* =================================================================================================== */
/* Terrain                                                                                             */
/*                                                                                                     */
/* Cave walls / fortress corridors in a 32x32 map on BG1. The map wraps horizontally and the screen    */
/* shows at most 31 columns, so whenever the scroll enters a new 8 px column the map column about to    */
/* scroll in is rewritten (during VBlank). Collision uses the same heights as the renderer.            */
/* =================================================================================================== */

#define COLUMNS 32
#define VISIBLE_ROWS 20

enum { TILE_EMPTY, TILE_FILL, TILE_FILL_ALT, TILE_FLOOR, TILE_FLOOR_ALT, TILE_CEILING, TILE_CEILING_ALT };

static struct
{
    const terrain_key* keys;
    int key_count;
    int next_column;        /* next world column to write */
    s8 ceiling_cache[COLUMNS];
    s8 floor_cache[COLUMNS];
    int pending[COLUMNS];   /* world columns to write to VRAM in the next VBlank */
    int pending_count;
} ter;

static int world_column_at(fx screen_x, fx scroll_x)
{
    return fx_floor(scroll_x + screen_x + FX(HALF_W)) >> 3;
}

/* Interpolated heights (tiles) from the key list; only used when a column is written. */
static void height_at(int column, int* ceiling, int* floor)
{
    *ceiling = 0;
    *floor = 0;

    if(! ter.key_count)
    {
        return;
    }

    const terrain_key* previous = &ter.keys[0];

    if(column <= previous->column)
    {
        *ceiling = previous->ceiling;
        *floor = previous->floor;
        return;
    }

    for(int i = 0; i < ter.key_count; ++i)
    {
        const terrain_key* key = &ter.keys[i];

        if(column <= key->column)
        {
            int span = key->column - previous->column;
            int t = column - previous->column;
            *ceiling = previous->ceiling + ((key->ceiling - previous->ceiling) * t) / span;
            *floor = previous->floor + ((key->floor - previous->floor) * t) / span;
            return;
        }

        previous = key;
    }

    *ceiling = previous->ceiling;
    *floor = previous->floor;
}

/* Heights of columns currently in the map ring come from a cache (no search, no division). */
static void cached_height_at(int column, int* ceiling, int* floor)
{
    if(column >= ter.next_column - COLUMNS && column < ter.next_column)
    {
        *ceiling = ter.ceiling_cache[column & (COLUMNS - 1)];
        *floor = ter.floor_cache[column & (COLUMNS - 1)];
    }
    else
    {
        height_at(column, ceiling, floor);
    }
}

static void write_column(int column)
{
    int ceiling, floor;
    height_at(column, &ceiling, &floor);
    ter.ceiling_cache[column & (COLUMNS - 1)] = (s8) ceiling;
    ter.floor_cache[column & (COLUMNS - 1)] = (s8) floor;

    if(ter.pending_count < COLUMNS)
    {
        ter.pending[ter.pending_count++] = column;
    }
}

void terrain_init(const stage_def* stage)
{
    ter.keys = stage->terrain;
    ter.key_count = stage->terrain_count;
    ter.next_column = 0;
    ter.pending_count = 0;
    video_show_terrain(stage->terrain_style);

    if(! ter.key_count)
    {
        return;
    }

    for(int column = 0; column < COLUMNS; ++column)
    {
        write_column(column);
    }

    ter.next_column = COLUMNS;
}

bool terrain_enabled(void)
{
    return ter.key_count > 0;
}

void terrain_update(fx scroll_x)
{
    if(! ter.key_count)
    {
        return;
    }

    /* Keep columns [first_visible, first_visible + 31] written. */
    int first_visible = fx_floor(scroll_x) >> 3;

    while(ter.next_column <= first_visible + COLUMNS - 1)
    {
        write_column(ter.next_column++);
    }

    video_set_scroll(1, fx_floor(scroll_x), 0);
}

void terrain_commit(void)
{
    for(int k = 0; k < ter.pending_count; ++k)
    {
        int column = ter.pending[k];
        int ceiling = ter.ceiling_cache[column & (COLUMNS - 1)];
        int floor = ter.floor_cache[column & (COLUMNS - 1)];
        u16* map = &se_mem[SBB_TERRAIN][column & (COLUMNS - 1)];

        for(int row = 0; row < VISIBLE_ROWS; ++row)
        {
            int tile = TILE_EMPTY;
            bool alt = ((column * 7 + row * 3) % 5) == 0;

            if(row < ceiling)
            {
                tile = row == ceiling - 1 ? (alt ? TILE_CEILING_ALT : TILE_CEILING) : (alt ? TILE_FILL_ALT : TILE_FILL);
            }
            else if(row >= VISIBLE_ROWS - floor)
            {
                tile = row == VISIBLE_ROWS - floor ? (alt ? TILE_FLOOR_ALT : TILE_FLOOR) : (alt ? TILE_FILL_ALT : TILE_FILL);
            }

            map[row * 32] = (u16) ((VRAM_TILE_TERRAIN + tile) | SE_PALBANK(PAL_BG_TERRAIN));
        }
    }

    ter.pending_count = 0;
}

bool terrain_blocks(hitbox box, fx scroll_x)
{
    if(! ter.key_count)
    {
        return false;
    }

    int first = world_column_at(box.c.x - FX(box.hw), scroll_x);
    int last = world_column_at(box.c.x + FX(box.hw), scroll_x);
    fx top = box.c.y - FX(box.hh);
    fx bottom = box.c.y + FX(box.hh);

    for(int column = first; column <= last; ++column)
    {
        int ceiling, floor;
        cached_height_at(column, &ceiling, &floor);

        if(ceiling > 0 && top < FX(-HALF_H + ceiling * 8))
        {
            return true;
        }

        if(floor > 0 && bottom > FX(HALF_H - floor * 8))
        {
            return true;
        }
    }

    return false;
}

int terrain_floor_y(fx screen_x, fx scroll_x)
{
    int ceiling, floor;
    cached_height_at(world_column_at(screen_x, scroll_x), &ceiling, &floor);
    return HALF_H - floor * 8;
}

int terrain_ceiling_y(fx screen_x, fx scroll_x)
{
    int ceiling, floor;
    cached_height_at(world_column_at(screen_x, scroll_x), &ceiling, &floor);
    return -HALF_H + ceiling * 8;
}

/* =================================================================================================== */
/* Parallax backgrounds: stars x0.25 (BG3), stage backdrop x0.5 (BG2).                                */
/* =================================================================================================== */

static int backdrop_y;

void backgrounds_init(backdrop_type backdrop)
{
    video_show_backgrounds(backdrop);

    /* The nebula is drawn for map rows 48..207; the cave and hull for rows 0..159. */
    backdrop_y = backdrop == BACKDROP_NEBULA ? 48 : 0;
    backgrounds_update(0);
}

void backgrounds_update(fx scroll_x)
{
    video_set_scroll(3, fx_floor(scroll_x / 4), 48);
    video_set_scroll(2, fx_floor(scroll_x / 2), backdrop_y);
}

/* =================================================================================================== */
/* Stage timeline                                                                                     */
/* =================================================================================================== */

static int next_event;
static bool boss_started;

void runner_reset(void)
{
    next_event = 0;
    boss_started = false;
}

bool runner_boss_started(void)
{
    return boss_started;
}

void runner_skip_to_boss(void)
{
    const stage_def* stage = world.stage;

    for(int i = next_event; i < stage->event_count; ++i)
    {
        if(stage->events[i].type == EVENT_WARNING || stage->events[i].type == EVENT_BOSS)
        {
            next_event = i;
            world.stage_frame = stage->events[i].frame;
            return;
        }
    }
}

static void run(const stage_event* event)
{
    switch(event->type)
    {
    case EVENT_SPAWN:
        {
            fx x = FX((event->d & FLAG_FROM_LEFT) ? -136 : 136);
            fx y = FX(event->b);

            if(event->a == ENEMY_TURRET)
            {
                /* Turrets sit on the terrain surface where they scroll in. */
                y = FX((event->d & FLAG_CEILING) ? terrain_ceiling_y(x, world.scroll_x) + 8
                                                 : terrain_floor_y(x, world.scroll_x) - 8);
            }

            enemies_spawn((enemy_kind) event->a, x, y, event->c, event->d, -1);
        }
        break;

    case EVENT_FORMATION:
        enemies_spawn_formation(event->a, event->b, event->c, event->d);
        break;

    case EVENT_SCROLL_SPEED:
        world.scroll_speed = FX(event->b) / 100;
        break;

    case EVENT_WARNING:
        hud_show_warning();
        audio_play(SFX_WARNING_ID);
        audio_stop_music();
        break;

    case EVENT_BOSS:
        boss_started = true;
        audio_play_music(MUSIC_BOSS);
        boss_start((boss_id) event->a);
        break;

    default:
        break;
    }
}

void runner_update(void)
{
    const stage_def* stage = world.stage;

    while(next_event < stage->event_count && stage->events[next_event].frame <= world.stage_frame)
    {
        run(&stage->events[next_event++]);
    }
}
