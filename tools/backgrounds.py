"""
Procedural background art. All BGs are 4bpp (16 colours) 256x256 regular backgrounds that wrap
horizontally, which lets the game scroll them forever with the hardware scroll registers.

Unique-tile counts are printed by the generator so VRAM use stays visible (a 256x256 4bpp BG can
need up to 1024 tiles = 32 KB; we aim for a few hundred at most).
"""

import math

from pixelart import Canvas, LCG
from font import glyph_rows

# ------------------------------------------------------------------------------------------------
# Far starfield (shared by all stages and the title screen)
# ------------------------------------------------------------------------------------------------

STARS_PAL = [
    (8, 8, 32),        # 0 backdrop (deep space)
    (56, 64, 104),     # 1 dim star
    (112, 120, 168),   # 2 mid star
    (200, 208, 248),   # 3 bright star
    (248, 248, 248),   # 4 white
    (248, 216, 136),   # 5 warm star
    (136, 192, 248),   # 6 blue star
]


def starfield(seed=2026, count=170):
    c = Canvas(256, 256)
    rng = LCG(seed)
    for _ in range(count):
        x, y = rng.randint(0, 255), rng.randint(0, 255)
        kind = rng.randint(0, 99)
        if kind < 55:
            c.set(x, y, 1)
        elif kind < 80:
            c.set(x, y, 2)
        elif kind < 90:
            c.set(x, y, rng.randint(5, 6))
        else:
            # small cross-shaped bright star
            c.set(x, y, 4)
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                c.set((x + dx) % 256, (y + dy) % 256, 3 if kind < 97 else 6)
    return c


# ------------------------------------------------------------------------------------------------
# Stage backdrops (BG2, parallax x0.5)
# ------------------------------------------------------------------------------------------------

NEBULA_PAL = [
    (0, 0, 0),         # 0 transparent (shows the starfield)
    (32, 16, 64),      # 1 nebula dark
    (56, 24, 96),      # 2 nebula mid
    (96, 40, 136),     # 3 nebula light
    (152, 72, 168),    # 4 nebula glow
    (16, 40, 72),      # 5 blue haze dark
    (32, 72, 120),     # 6 blue haze
    (72, 120, 168),    # 7 planet light
    (40, 72, 120),     # 8 planet mid
    (24, 40, 80),      # 9 planet dark
    (200, 176, 120),   # 10 ring light
    (136, 112, 80),    # 11 ring dark
    (12, 16, 40),      # 12 planet shadow
]


def _dither(x, y, level):
    """Ordered 4x4 Bayer dithering: returns True if pixel is 'on' for a 0..16 level."""
    bayer = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]
    return bayer[y & 3][x & 3] < level


def nebula_backdrop():
    c = Canvas(256, 256)
    # Soft nebula bands built from a few overlapping blobs, dithered into 4 tones.
    blobs = [(60, 70, 70, 26), (150, 90, 60, 20), (210, 60, 50, 18), (90, 170, 80, 22), (200, 190, 60, 20)]
    for y in range(256):
        for x in range(256):
            v = 0.0
            for bx, by, rx, ry in blobs:
                for wrap in (-256, 0, 256):
                    dx = (x - bx - wrap) / rx
                    dy = (y - by) / ry
                    d = dx * dx + dy * dy
                    if d < 1.0:
                        v += (1.0 - d)
            level = int(min(v, 1.0) * 48)
            if level <= 0:
                continue
            tone = min(3, level // 12)
            frac = (level % 12) * 16 // 12
            if _dither(x, y, frac):
                tone += 1
            if tone:
                c.set(x, y, min(4, tone))
    # Ringed planet in the upper part of the map.
    px, py, pr = 188, 128, 26
    for y in range(py - pr, py + pr + 1):
        for x in range(px - pr, px + pr + 1):
            d = math.hypot(x - px, y - py)
            if d <= pr:
                light = (x - px) * -0.5 + (y - py) * -0.7
                shade = light / pr
                if shade > 0.35:
                    col = 7
                elif shade > -0.15:
                    col = 8
                elif shade > -0.55:
                    col = 9
                else:
                    col = 12
                # horizontal cloud bands
                if (y // 5) % 3 == 0 and col in (7, 8) and _dither(x, y, 6):
                    col = 8 if col == 7 else 9
                c.set(x, y, col)
    for x in range(px - 44, px + 45):
        t = (x - px) / 44.0
        y = py + int(round(t * 9))
        if abs(x - px) < pr - 2 and y < py:  # ring hidden behind the planet's upper half
            continue
        c.set(x, y, 10)
        c.set(x, y + 1, 11)
    return c


CAVE_PAL = [
    (0, 0, 0),         # 0 transparent
    (14, 8, 24),       # 1 rock darkest
    (24, 12, 40),      # 2 rock dark
    (36, 18, 58),      # 3 rock mid
    (52, 26, 80),      # 4 rock light
    (24, 64, 80),      # 5 crystal dark
    (48, 104, 120),    # 6 crystal mid
    (96, 144, 160),    # 7 crystal light
]


def cave_backdrop():
    """Distant cave wall (visible band y 0..159; the game places this BG at y=48): repeating stalactite/stalagmite silhouettes with crystal glints."""
    c = Canvas(256, 256)
    rng = LCG(99)
    # 64-pixel wide repeating unit keeps the tile count small.
    unit = Canvas(64, 256)
    for x in range(64):
        top = 20 + int(8 * math.sin(x * 2 * math.pi / 64)) + int(5 * math.sin(x * 6 * math.pi / 64))
        bot = 140 - int(8 * math.cos(x * 2 * math.pi / 64)) - int(5 * math.sin(x * 8 * math.pi / 64))
        for y in range(0, top):
            unit.set(x, y, 2 if y < top - 6 else 3)
        for y in range(bot, 160):
            unit.set(x, y, 2 if y > bot + 6 else 3)
    # stalactites / stalagmites
    for (x, length) in ((8, 26), (27, 14), (44, 34), (56, 18)):
        for i in range(length):
            w = max(0, 3 - i * 3 // length)
            top = 20 + int(8 * math.sin(x * 2 * math.pi / 64))
            for dx in range(-w, w + 1):
                unit.set(x + dx, top + i, 3 if dx > -w else 4)
    for (x, length) in ((16, 20), (36, 30), (52, 12)):
        bot = 140 - int(8 * math.cos(x * 2 * math.pi / 64))
        for i in range(length):
            w = max(0, 3 - i * 3 // length)
            for dx in range(-w, w + 1):
                unit.set(x + dx, bot - i, 3 if dx > -w else 4)
    # crystals embedded in the walls
    for _ in range(6):
        x = rng.randint(2, 61)
        y = rng.randint(2, 12) if rng.chance(1, 2) else rng.randint(150, 157)
        unit.set(x, y, 7)
        unit.set(x, y + 1, 6)
        unit.set(x - 1, y + 1, 5)
        unit.set(x + 1, y + 1, 5)
    for ox in range(0, 256, 64):
        c.blit(unit, ox, 0)
    return c


HULL_PAL = [
    (0, 0, 0),         # 0 transparent
    (14, 14, 24),      # 1 panel shadow
    (22, 22, 36),      # 2 panel dark
    (32, 32, 50),      # 3 panel mid
    (44, 44, 68),      # 4 panel light
    (64, 24, 28),      # 5 warning dark
    (104, 56, 36),     # 6 warning light
    (24, 72, 76),      # 7 light strip
    (48, 120, 112),    # 8 light glow
]


def hull_backdrop():
    """Interior of the enemy dreadnought (visible band y 0..159, BG placed at y=48): girders and light strips, 32 px repeating tiles."""
    c = Canvas(256, 256)
    for y in range(256):
        for x in range(256):
            ux, uy = x % 32, y % 64
            col = 0
            if uy < 4 or uy >= 60:
                col = 2
            if ux in (0, 1):
                col = 3
            if ux == 2:
                col = 1
            if 28 <= uy < 31 and ux > 3:
                col = 7 if ux % 8 else 8
            if uy in (4, 59):
                col = 4
            c.set(x, y, col)
    # heavy bulkheads top & bottom
    for y in list(range(0, 22)) + list(range(138, 160)):
        for x in range(256):
            ux = x % 64
            edge = y in (21, 138)
            col = 4 if edge else (3 if (ux // 8) % 2 == 0 else 2)
            if 8 <= y < 12 or 148 <= y < 152:
                col = 6 if ((x + y) // 4) % 2 == 0 else 5
            c.set(x, y, col)
    return c


# ------------------------------------------------------------------------------------------------
# Terrain tiles (BG1, streamed by the game from stage height data)
# ------------------------------------------------------------------------------------------------
# Tile order (must match terrain.h): 0 empty, 1 fill, 2 fill alt, 3 floor edge, 4 floor edge alt,
# 5 ceiling edge, 6 ceiling edge alt.

# Terrain must read instantly as "solid, deadly": bright rims and hazard stripes on every exposed
# edge, and body colours clearly lighter/more saturated than the (darkened) distant backdrops.
CRYSTAL_TERRAIN_PAL = [
    (0, 0, 0),         # 0 transparent
    (24, 8, 40),       # 1 crack / outline
    (88, 48, 136),     # 2 rock body
    (128, 80, 184),    # 3 rock light
    (184, 136, 232),   # 4 rim highlight
    (32, 128, 168),    # 5 crystal dark
    (104, 216, 240),   # 6 crystal mid
    (224, 255, 255),   # 7 crystal light
    (56, 24, 88),      # 8 rock shadow
]

METAL_TERRAIN_PAL = [
    (0, 0, 0),         # 0 transparent
    (24, 24, 32),      # 1 seam
    (112, 120, 144),   # 2 plate
    (160, 168, 192),   # 3 plate light
    (224, 232, 248),   # 4 rim
    (32, 24, 16),      # 5 hazard black
    (248, 200, 48),    # 6 hazard yellow
    (120, 232, 216),   # 7 light strip
    (72, 80, 104),     # 8 plate shadow
]


def _hazard(tile, y0, rows):
    for y in range(y0, y0 + rows):
        for x in range(8):
            tile.set(x, y, 6 if ((x + y) // 2) % 2 == 0 else 5)


def terrain_tiles(style):
    tiles = [Canvas(8, 8) for _ in range(7)]
    if style == 'crystal':
        for t in (1, 2):
            tiles[t].rect(0, 0, 8, 8, 2)
            tiles[t].set(7, 3, 8)
            tiles[t].set(0, 7, 8)
        for (x, y) in ((1, 1), (5, 4), (2, 6), (6, 1)):
            tiles[1].set(x, y, 3)
        for (x, y) in ((3, 2), (1, 5)):
            tiles[2].set(x, y, 1)
            tiles[2].set(x + 1, y, 1)
            tiles[2].set(x + 1, y + 1, 3)
        # floor edge: bright rim + crystal line on the exposed top
        for t in (3, 4):
            tiles[t].rect(0, 0, 8, 8, 2)
            tiles[t].rect(0, 0, 8, 1, 7)
            tiles[t].rect(0, 1, 8, 1, 6)
            tiles[t].rect(0, 2, 8, 1, 4)
            tiles[t].rect(0, 3, 8, 1, 3)
        tiles[4].set(2, 4, 5)
        tiles[4].set(5, 5, 5)
        # ceiling edge: same, mirrored (exposed bottom)
        for t in (5, 6):
            tiles[t].rect(0, 0, 8, 8, 2)
            tiles[t].rect(0, 7, 8, 1, 7)
            tiles[t].rect(0, 6, 8, 1, 6)
            tiles[t].rect(0, 5, 8, 1, 4)
            tiles[t].rect(0, 4, 8, 1, 3)
        tiles[6].set(3, 3, 5)
        tiles[6].set(6, 2, 5)
    else:
        for t in (1, 2):
            tiles[t].rect(0, 0, 8, 8, 2)
            tiles[t].rect(0, 0, 8, 1, 3)
            tiles[t].rect(0, 0, 1, 8, 3)
            tiles[t].rect(7, 0, 1, 8, 8)
            tiles[t].rect(0, 7, 8, 1, 8)
        tiles[1].set(3, 3, 1)
        tiles[1].set(4, 4, 3)
        tiles[2].rect(2, 3, 4, 1, 7)
        # floor edge: bright rim + hazard stripes on the exposed top
        for t in (3, 4):
            tiles[t].rect(0, 0, 8, 8, 2)
            tiles[t].rect(0, 0, 8, 1, 4)
            _hazard(tiles[t], 1, 3)
            tiles[t].rect(0, 4, 8, 1, 1)
        tiles[4].rect(1, 6, 6, 1, 7)
        for t in (5, 6):
            tiles[t].rect(0, 0, 8, 8, 2)
            tiles[t].rect(0, 7, 8, 1, 4)
            _hazard(tiles[t], 4, 3)
            tiles[t].rect(0, 3, 8, 1, 1)
        tiles[6].rect(1, 1, 6, 1, 7)
    strip = Canvas(8, 8 * len(tiles))
    for i, t in enumerate(tiles):
        strip.blit(t, 0, i * 8)
    return strip


# ------------------------------------------------------------------------------------------------
# Title logo (BG0 on the title screen)
# ------------------------------------------------------------------------------------------------

LOGO_PAL = [
    (0, 0, 0),         # 0 transparent
    (16, 8, 40),       # 1 outline
    (248, 248, 200),   # 2 highlight
    (248, 224, 72),    # 3 yellow
    (248, 160, 40),    # 4 orange
    (232, 80, 40),     # 5 red
    (152, 32, 56),     # 6 deep red
    (40, 48, 120),     # 7 shadow blue
    (88, 200, 248),    # 8 cyan
    (48, 120, 216),    # 9 blue
    (248, 248, 248),   # 10 white
]


def _big_text(c, text, ox, oy, scale, gradient, bevel=True):
    """Render text from the 5x7 font as chunky blocks with a vertical colour gradient."""
    x = ox
    for ch in text:
        if ch == ' ':
            x += 3 * scale
            continue
        rows = glyph_rows(ch)
        for gy, row in enumerate(rows):
            for gx, v in enumerate(row):
                if v != '#':
                    continue
                for sy in range(scale):
                    for sx in range(scale):
                        py = gy * scale + sy
                        t = py / (7 * scale)
                        col = gradient[min(len(gradient) - 1, int(t * len(gradient)))]
                        if bevel and sy == 0 and (gy == 0 or rows[gy - 1][gx] != '#'):
                            col = 2
                        c.set(x + gx * scale + sx, oy + py, col)
        x += 6 * scale


def title_logo():
    # Screen (sx, sy) maps to bitmap (sx + 8, sy + 48) because Butano centres a 256x256 BG.
    c = Canvas(256, 256)
    grad = [3, 3, 4, 4, 5, 5, 6]
    shadow = Canvas(256, 256)
    _big_text(shadow, "SPACE", 8 + 60 + 3, 48 + 14 + 3, 4, [7], bevel=False)
    _big_text(shadow, "SHOOTER", 8 + 20 + 3, 48 + 50 + 3, 4, [7], bevel=False)
    c.blit(shadow, 0, 0)
    _big_text(c, "SPACE", 8 + 60, 48 + 14, 4, grad)
    _big_text(c, "SHOOTER", 8 + 20, 48 + 50, 4, grad)
    c.outline(1)
    # thin cyan speed lines under the logo
    for i, (x0, x1) in enumerate(((8 + 24, 8 + 216), (8 + 44, 8 + 196))):
        y = 48 + 84 + i * 3
        for x in range(x0, x1):
            c.set(x, y, 8 if i == 0 else 9)
    return c
