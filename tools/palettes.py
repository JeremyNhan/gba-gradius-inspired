"""
Colour palettes for Space Shooter. Colours are 8-bit RGB but chosen as multiples of 8 so they map
exactly to the GBA's 15-bit BGR555 format.

The master sprite palette is shared by almost every gameplay sprite, which keeps the art coherent and
uses a single hardware palette bank.
"""

TRANSPARENT = (248, 0, 248)  # never displayed (index 0)

# Master sprite palette. The letters are the ASCII-art keys used in sprite definitions.
MASTER = [
    ('.', TRANSPARENT),
    ('K', (16, 16, 40)),      # outline / near black
    ('W', (248, 248, 248)),   # white
    ('l', (176, 184, 208)),   # light steel
    ('d', (88, 96, 128)),     # dark steel
    ('c', (104, 232, 248)),   # cyan
    ('b', (48, 128, 240)),    # blue
    ('n', (24, 48, 136)),     # navy
    ('y', (248, 232, 72)),    # yellow
    ('o', (248, 144, 32)),    # orange
    ('r', (232, 48, 48)),     # red
    ('m', (128, 16, 48)),     # maroon
    ('g', (104, 224, 88)),    # green
    ('G', (24, 120, 64)),     # dark green
    ('p', (248, 104, 208)),   # pink
    ('v', (144, 72, 216)),    # violet
]

# Boss palette: metallic hull ramp + organic/energy colours.
BOSS = [
    ('.', TRANSPARENT),
    ('K', (16, 8, 32)),       # outline
    ('W', (248, 248, 248)),   # highlight
    ('1', (200, 200, 224)),   # hull light
    ('2', (136, 136, 168)),   # hull mid
    ('3', (80, 80, 112)),     # hull dark
    ('4', (40, 40, 64)),      # hull shadow
    ('r', (248, 64, 48)),     # core red
    ('o', (248, 160, 48)),    # core orange
    ('y', (248, 240, 120)),   # core yellow
    ('p', (208, 96, 232)),    # crystal light
    ('v', (120, 48, 176)),    # crystal mid
    ('u', (64, 24, 104)),     # crystal dark
    ('c', (96, 232, 216)),    # teal light
    ('t', (32, 136, 136)),    # teal dark
    ('m', (144, 24, 40)),     # core dark
]

# Font: transparent, shadow, face.
FONT_WHITE = [TRANSPARENT, (24, 16, 56), (248, 248, 248), (160, 168, 200)]
FONT_YELLOW = [TRANSPARENT, (56, 24, 16), (248, 224, 64), (200, 120, 40)]
FONT_CYAN = [TRANSPARENT, (8, 32, 64), (112, 240, 248), (48, 144, 200)]
FONT_RED = [TRANSPARENT, (48, 8, 16), (248, 88, 72), (160, 40, 40)]


def pad16(colors):
    return list(colors) + [(0, 0, 0)] * (16 - len(colors))


def colors_of(pal):
    return [c for _, c in pal]


def charmap_of(pal):
    return {k: i for i, (k, _) in enumerate(pal)}


def flash_of(pal):
    """Same size palette where every opaque colour is white (hit-flash effect)."""
    return [pal[0][1]] + [(248, 248, 248)] * (len(pal) - 1)
