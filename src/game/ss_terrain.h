#ifndef SS_TERRAIN_H
#define SS_TERRAIN_H

#include "bn_camera_ptr.h"
#include "bn_optional.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_map_cell.h"
#include "bn_regular_bg_map_item.h"
#include "bn_regular_bg_ptr.h"
#include "bn_span.h"

#include "ss_collision.h"
#include "ss_stage_data.h"

namespace ss
{

/**
 * Cave walls / fortress corridors, streamed into a 32x32-cell (256x256 px) regular BG.
 *
 * The map wraps horizontally, and the screen shows at most 31 columns, so whenever the camera
 * enters a new 8-pixel column we rewrite the single map column that is about to scroll in and
 * copy the cell buffer to VRAM (reload_cells_ref, 2 KB, done during VBlank by Butano).
 * Collision uses the same height function as the renderer, so what you see is what you hit.
 */
class terrain
{

public:
    terrain(terrain_style style, const bn::span<const terrain_key>& keys, const bn::optional<bn::camera_ptr>& camera);

    void update(bn::fixed scroll_x);

    [[nodiscard]] bool enabled() const
    {
        return _style != terrain_style::NONE;
    }

    /// Ceiling / floor thickness in tiles at a world column.
    [[nodiscard]] int ceiling_tiles(int world_column) const;
    [[nodiscard]] int floor_tiles(int world_column) const;

    /// True if the box (screen coordinates) overlaps solid terrain.
    [[nodiscard]] bool blocks(const hitbox& box, bn::fixed scroll_x) const;

    /// Screen y of the floor surface / ceiling surface below/above a screen x.
    [[nodiscard]] int floor_y(bn::fixed screen_x, bn::fixed scroll_x) const;
    [[nodiscard]] int ceiling_y(bn::fixed screen_x, bn::fixed scroll_x) const;

private:
    static constexpr int columns = 32;
    static constexpr int rows = 32;
    static constexpr int visible_rows = 20;

    struct map_data
    {
        alignas(int) bn::regular_bg_map_cell cells[columns * rows];
    };

    terrain_style _style;
    bn::span<const terrain_key> _keys;
    map_data _map;
    bn::optional<bn::regular_bg_map_item> _map_item;
    bn::optional<bn::regular_bg_ptr> _bg;
    int _next_column = 0;       // next world column to write
    bool _dirty = false;

    // Heights of the columns currently in the map ring (world columns [_next_column - columns, _next_column)),
    // indexed like the map. Collision queries hit this instead of interpolating the key list, which costs a
    // linear search and two software divisions (the ARM7 has no divide instruction) per column.
    signed char _ceiling_cache[columns] = {};
    signed char _floor_cache[columns] = {};

    void _height_at(int world_column, int& ceiling, int& floor) const;
    void _cached_height_at(int world_column, int& ceiling, int& floor) const;
    void _write_column(int world_column);
};

}

#endif
