#include "ss_terrain.h"

#include "bn_bg_tiles.h"
#include "bn_memory.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_regular_bg_map_ptr.h"

#include "bn_bg_palette_items_terrain_crystal_palette.h"
#include "bn_bg_palette_items_terrain_metal_palette.h"
#include "bn_regular_bg_tiles_items_terrain_crystal.h"
#include "bn_regular_bg_tiles_items_terrain_metal.h"

#include "ss_constants.h"

namespace ss
{

namespace
{
    // Tile indices in the terrain tile strip (tools/backgrounds.py: terrain_tiles()).
    constexpr int tile_empty = 0;
    constexpr int tile_fill = 1;
    constexpr int tile_fill_alt = 2;
    constexpr int tile_floor = 3;
    constexpr int tile_floor_alt = 4;
    constexpr int tile_ceiling = 5;
    constexpr int tile_ceiling_alt = 6;

    // Same alignment as the other 256x256 BGs: map (0, 0) at the screen's top-left corner.
    constexpr int bg_left_x = 8;
    constexpr int bg_top_y = 48;

    [[nodiscard]] int world_column_at(bn::fixed screen_x, bn::fixed scroll_x)
    {
        return (scroll_x + screen_x + half_w).right_shift_integer() >> 3;
    }
}

terrain::terrain(terrain_style style, const bn::span<const terrain_key>& keys,
                 const bn::optional<bn::camera_ptr>& camera) :
    _style(keys.empty() ? terrain_style::NONE : style),
    _keys(keys)
{
    if(_style == terrain_style::NONE)
    {
        return;
    }

    bn::memory::clear(_map.cells);
    _map_item.emplace(_map.cells[0], bn::size(columns, rows));

    for(int column = 0; column < columns; ++column)
    {
        _write_column(column);
    }

    _next_column = columns;

    // Tile index 0 must be the first tile of our tile set (as in Butano's dynamic_regular_bg example).
    bn::bg_tiles::set_allow_offset(false);

    if(_style == terrain_style::CRYSTAL)
    {
        bn::regular_bg_item item(bn::regular_bg_tiles_items::terrain_crystal,
                                 bn::bg_palette_items::terrain_crystal_palette, *_map_item);
        _bg = item.create_bg(bg_left_x, bg_top_y);
    }
    else
    {
        bn::regular_bg_item item(bn::regular_bg_tiles_items::terrain_metal,
                                 bn::bg_palette_items::terrain_metal_palette, *_map_item);
        _bg = item.create_bg(bg_left_x, bg_top_y);
    }

    bn::bg_tiles::set_allow_offset(true);
    _bg->set_priority(bg_priority_terrain);

    if(camera)
    {
        _bg->set_camera(*camera);
    }
}

void terrain::_height_at(int world_column, int& ceiling, int& floor) const
{
    ceiling = 0;
    floor = 0;

    if(_keys.empty())
    {
        return;
    }

    const terrain_key* previous = &_keys[0];

    if(world_column <= previous->column)
    {
        ceiling = previous->ceiling;
        floor = previous->floor;
        return;
    }

    for(const terrain_key& key : _keys)
    {
        if(world_column <= key.column)
        {
            int span = key.column - previous->column;
            int t = world_column - previous->column;
            ceiling = previous->ceiling + ((key.ceiling - previous->ceiling) * t) / span;
            floor = previous->floor + ((key.floor - previous->floor) * t) / span;
            return;
        }

        previous = &key;
    }

    ceiling = previous->ceiling;
    floor = previous->floor;
}

int terrain::ceiling_tiles(int world_column) const
{
    int ceiling, floor;
    _height_at(world_column, ceiling, floor);
    return ceiling;
}

int terrain::floor_tiles(int world_column) const
{
    int ceiling, floor;
    _height_at(world_column, ceiling, floor);
    return floor;
}

void terrain::_write_column(int world_column)
{
    int ceiling, floor;
    _height_at(world_column, ceiling, floor);

    int map_column = world_column & (columns - 1);
    bn::regular_bg_map_item& item = *_map_item;

    for(int row = 0; row < visible_rows; ++row)
    {
        int tile = tile_empty;
        bool alt = ((world_column * 7 + row * 3) % 5) == 0;

        if(row < ceiling)
        {
            tile = row == ceiling - 1 ? (alt ? tile_ceiling_alt : tile_ceiling) : (alt ? tile_fill_alt : tile_fill);
        }
        else if(row >= visible_rows - floor)
        {
            tile = row == visible_rows - floor ? (alt ? tile_floor_alt : tile_floor) : (alt ? tile_fill_alt : tile_fill);
        }

        bn::regular_bg_map_cell_info info;
        info.set_tile_index(tile);
        info.set_palette_id(0);
        _map.cells[item.cell_index(map_column, row)] = info.cell();
    }

    _dirty = true;
}

void terrain::update(bn::fixed scroll_x)
{
    if(! _bg)
    {
        return;
    }

    // Keep columns [first_visible, first_visible + 31] written.
    int first_visible = scroll_x.right_shift_integer() >> 3;

    while(_next_column <= first_visible + columns - 1)
    {
        _write_column(_next_column);
        ++_next_column;
    }

    if(_dirty)
    {
        bn::regular_bg_map_ptr map = _bg->map();
        map.reload_cells_ref();
        _dirty = false;
    }

    _bg->set_x(bg_left_x - scroll_x);
}

bool terrain::blocks(const hitbox& box, bn::fixed scroll_x) const
{
    if(_style == terrain_style::NONE)
    {
        return false;
    }

    int first = world_column_at(box.center.x() - box.half_w, scroll_x);
    int last = world_column_at(box.center.x() + box.half_w, scroll_x);
    bn::fixed top = box.center.y() - box.half_h;
    bn::fixed bottom = box.center.y() + box.half_h;

    for(int column = first; column <= last; ++column)
    {
        int ceiling, floor;
        _height_at(column, ceiling, floor);

        if(ceiling > 0 && top < -half_h + ceiling * 8)
        {
            return true;
        }

        if(floor > 0 && bottom > half_h - floor * 8)
        {
            return true;
        }
    }

    return false;
}

int terrain::floor_y(bn::fixed screen_x, bn::fixed scroll_x) const
{
    return half_h - floor_tiles(world_column_at(screen_x, scroll_x)) * 8;
}

int terrain::ceiling_y(bn::fixed screen_x, bn::fixed scroll_x) const
{
    return -half_h + ceiling_tiles(world_column_at(screen_x, scroll_x)) * 8;
}

}
