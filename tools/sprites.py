"""
Original sprite art for Space Shooter.

Small sprites are hand-authored ASCII art in the master palette (letters from palettes.MASTER).
Interior colours are drawn by hand and a 'K' outline is added automatically, which gives every
sprite the same crisp 16-bit look. Effects and bosses are drawn procedurally.

Each function returns (canvas, frame_height) where the canvas is a vertical strip of frames.
"""

import math

from pixelart import Canvas, vstack, LCG
from palettes import MASTER, BOSS, charmap_of
from font import glyph_rows

M = charmap_of(MASTER)
B = charmap_of(BOSS)


def art(rows, w=16, h=16, outline=True, cmap=M, diag=False):
    c = Canvas(w, h)
    c.blit_ascii(rows, cmap)
    if outline:
        c.outline(cmap['K'], diagonal=diag)
    return c


# ------------------------------------------------------------------------------------------------
# Player
# ------------------------------------------------------------------------------------------------

PLAYER = [
    "................",
    "................",
    "...nb...........",
    "...nbb..........",
    "....nbb.........",
    "....dlllb.......",
    "..dllllllccc....",
    "oydlllllllllllW.",
    "..ddlllllddddd..",
    "....dddlb.......",
    "....nbb.........",
    "...nbb..........",
    "...nb...........",
    "................",
    "................",
    "................",
]


def _shear(rows, direction):
    """Bank the ship: rear columns move opposite to the nose to fake a 3/4 tilt."""
    out = [list(r) for r in rows]
    h = len(rows)
    for x in range(len(rows[0])):
        shift = direction if x < 6 else 0
        if shift:
            col = [rows[y][x] for y in range(h)]
            for y in range(h):
                src = y - shift
                out[y][x] = col[src] if 0 <= src < h else '.'
    return [''.join(r) for r in out]


def player():
    frames = []
    for bank in (0, -1, 1):          # level, nose up, nose down
        rows = _shear(PLAYER, bank) if bank else PLAYER
        for flame in (0, 1):
            r = [list(x) for x in rows]
            # engine flame flicker: alternate the two exhaust pixels
            for y in range(16):
                for x in range(2):
                    if r[y][x] in 'oy':
                        r[y][x] = ('y' if x == 1 else 'o') if flame == 0 else ('o' if x == 1 else 'r')
            frames.append(art([''.join(x) for x in r]))
    return vstack(frames), 16


def life_icon():
    rows = [
        "........",
        ".nb.....",
        ".dlllc..",
        "ylllllW.",
        ".dddd...",
        ".nb.....",
        "........",
        "........",
    ]
    return vstack([art(rows, 8, 8)]), 8


# ------------------------------------------------------------------------------------------------
# Enemies
# ------------------------------------------------------------------------------------------------

DART = [
    "................",
    "................",
    "................",
    "..........mm....",
    "........mrrm....",
    "......mrrrd.....",
    "...ldrroolldd...",
    ".Wlllrooyoolldo.",
    "...ldrroolldd...",
    "......mrrrd.....",
    "........mrrm....",
    "..........mm....",
    "................",
    "................",
    "................",
    "................",
]


def dart():
    a = art(DART)
    b_rows = [r.replace('do.', 'dy.') for r in DART]
    return vstack([a, art(b_rows)]), 16


def waver():
    base = [
        "................",
        "................",
        "................",
        "................",
        ".....GGGGGG.....",
        "...GGggggggGG...",
        "..GggWWggggggG..",
        ".dddddddddddddd.",
        ".llcllllcllllcl.",
        "..dddddddddddd..",
        "....GGGGGGGG....",
        "................",
        "................",
        "................",
        "................",
        "................",
    ]
    frames = []
    for phase in range(3):
        rows = list(base)
        light = list(rows[8])
        for x in range(1, 15):
            if light[x] in 'lc':
                light[x] = 'y' if (x + phase * 2) % 6 == 0 else 'l'
        rows[8] = ''.join(light)
        frames.append(art(rows))
    return vstack(frames), 16


def interceptor():
    base = [
        "................",
        "................",
        "................",
        "................",
        "................",
        "........vvvv....",
        "....vvvvpppvv...",
        ".Wpppppppppcvvyo",
        "....vvvvpppvv...",
        "........vvvv....",
        "................",
        "................",
        "................",
        "................",
        "................",
        "................",
    ]
    b = [r.replace('yo', 'oy') for r in base]
    return vstack([art(base), art(b)]), 16


def turret():
    """Ground turret, 5 barrel directions: left, up-left, up, up-right, right (floor-mounted)."""
    frames = []
    for i in range(5):
        c = Canvas(16, 16)
        ang = math.pi - i * math.pi / 4   # 180 deg (left) .. 0 deg (right)
        cx, cy = 7.5, 9.5
        c.ellipse(7.5, 12.5, 5.5, 4.5, M['d'])
        c.ellipse(7.5, 11.5, 4, 3, M['l'])
        c.rect(1, 14, 14, 2, M['n'])
        # two-pixel-thick barrel drawn over the dome: light line with a dark underside, red muzzle
        nx, ny = math.sin(ang), math.cos(ang)
        for t in range(2, 8):
            x = cx + math.cos(ang) * t
            y = cy - math.sin(ang) * t
            c.set(int(round(x + nx * 0.8)), int(round(y + ny * 0.8)), M['d'])
            c.set(int(round(x)), int(round(y)), M['l'])
        c.set(int(round(cx + math.cos(ang) * 7.5)), int(round(cy - math.sin(ang) * 7.5)), M['r'])
        c.set(7, 11, M['o'])
        c.set(8, 11, M['o'])
        c.outline(M['K'])
        frames.append(c)
    return vstack(frames), 16


def hulk():
    c = Canvas(32, 32)
    # main armoured body
    c.polygon([(3, 16), (9, 6), (26, 5), (30, 10), (30, 22), (26, 27), (9, 26)], M['d'])
    c.polygon([(6, 16), (11, 9), (25, 8), (28, 12), (28, 20), (25, 24), (11, 23)], M['l'])
    # armour plates
    for y in (11, 15, 19):
        c.line(13, y, 26, y, M['d'])
    c.rect(20, 9, 2, 15, M['d'])
    # cannon & glowing core
    c.rect(1, 14, 8, 4, M['m'])
    c.rect(0, 15, 3, 2, M['r'])
    c.ellipse(15, 16, 3.5, 3.5, M['m'])
    c.ellipse(15, 16, 2.5, 2.5, M['r'])
    c.set(14, 15, M['y'])
    # engines
    c.rect(29, 11, 3, 3, M['o'])
    c.rect(29, 19, 3, 3, M['o'])
    c.outline(M['K'])
    f2 = c.copy()
    f2.replace(M['o'], M['y'])
    return vstack([c, f2]), 32


def swarm():
    frames = []
    for i in range(4):
        c = Canvas(16, 16)
        c.ring(7.5, 7.5, 5.5, 3.2, M['o'])
        ang = i * math.pi / 4
        for k in range(3):
            a = ang + k * 2 * math.pi / 3
            c.set(int(round(7.5 + math.cos(a) * 4.4 - 0.5)), int(round(7.5 + math.sin(a) * 4.4 - 0.5)), M['y'])
        c.ellipse(7.5, 7.5, 2, 2, M['c'])
        c.set(7, 7, M['W'])
        c.outline(M['K'])
        frames.append(c)
    return vstack(frames), 16


def mine():
    frames = []
    for i in range(2):
        c = Canvas(16, 16)
        for k in range(8):
            a = k * math.pi / 4
            c.line(7.5 + math.cos(a) * 3, 7.5 + math.sin(a) * 3,
                   7.5 + math.cos(a) * 6.5, 7.5 + math.sin(a) * 6.5, M['d'])
        c.ellipse(7.5, 7.5, 4.5, 4.5, M['m'])
        c.ellipse(7, 7, 3, 3, M['r'] if i == 0 else M['o'])
        c.set(6, 6, M['W'])
        c.outline(M['K'])
        frames.append(c)
    return vstack(frames), 16


def asteroid_small():
    frames = []
    rng = LCG(77)
    base = Canvas(16, 16)
    pts = []
    for k in range(9):
        a = k * 2 * math.pi / 9
        r = 5.2 + rng.randint(0, 20) / 10
        pts.append((7.5 + math.cos(a) * r, 7.5 + math.sin(a) * r))
    base.polygon(pts, M['d'])
    base.ellipse(6.5, 6.5, 3.5, 3, M['l'])
    base.ellipse(9, 9.5, 1.5, 1.2, M['d'])
    base.ellipse(5, 9, 1, 1, M['n'])
    base.outline(M['K'])
    for i in range(4):
        frames.append(base.rotated90(i))
    return vstack(frames), 16


def asteroid_big():
    frames = []
    rng = LCG(1234)
    base = Canvas(32, 32)
    pts = []
    for k in range(12):
        a = k * 2 * math.pi / 12
        r = 11.5 + rng.randint(0, 30) / 10
        pts.append((15.5 + math.cos(a) * r, 15.5 + math.sin(a) * r))
    base.polygon(pts, M['d'])
    base.ellipse(13, 12, 8, 7, M['l'])
    for (x, y, r) in ((19, 19, 3), (10, 18, 2), (18, 9, 1.6), (22, 14, 1.2)):
        base.ellipse(x, y, r, r, M['d'])
        base.ellipse(x + 0.6, y + 0.6, r * 0.6, r * 0.6, M['n'])
    base.outline(M['K'])
    for i in range(4):
        frames.append(base.rotated90(i))
    return vstack(frames), 32


# ------------------------------------------------------------------------------------------------
# Projectiles
# ------------------------------------------------------------------------------------------------

def player_shot():
    rows = [
        "........",
        "........",
        "........",
        "bccWWWc.",
        "bccWWWc.",
        "........",
        "........",
        "........",
    ]
    return vstack([art(rows, 8, 8, outline=False)]), 8


def spread_shot():
    rows = [
        "........",
        "........",
        "...cc...",
        "..cWWb..",
        "..cWWb..",
        "...bb...",
        "........",
        "........",
    ]
    return vstack([art(rows, 8, 8, outline=False)]), 8


def missile():
    """8 directions, frame i = i * 45 degrees counter-clockwise from 'right'."""
    frames = []
    for i in range(8):
        c = Canvas(8, 8)
        a = i * math.pi / 4
        dx, dy = math.cos(a), -math.sin(a)
        for t in range(-3, 3):
            c.set(int(round(3.5 + dx * t)), int(round(3.5 + dy * t)), M['l'] if t < 2 else M['W'])
        c.set(int(round(3.5 + dx * 2.5)), int(round(3.5 + dy * 2.5)), M['r'])
        c.set(int(round(3.5 - dx * 3.5)), int(round(3.5 - dy * 3.5)), M['o'])
        c.outline(M['K'])
        frames.append(c)
    return vstack(frames), 8


def laser():
    """32x8 laser bolt. Frames: level, rising (spread laser upper), falling (spread laser lower).
    The diagonal slope (about 0.19) matches the spread laser velocity in ss_weapon_data.h."""
    frames = []
    for slope in (0.0, -0.19, 0.19):
        c = Canvas(32, 8)
        for x in range(3, 30):
            y = 3.5 + (x - 16) * slope
            yi = int(round(y))
            c.set(x, yi - 1, M['b'])
            c.set(x, yi, M['c'] if x < 8 else M['W'])
            c.set(x, yi + 1, M['c'] if slope == 0 or x < 8 else M['b'])
        frames.append(c)
    return vstack(frames), 8


def shooter():
    """16x16 additional shooter (trailing drone). Two pulse frames."""
    frames = []
    for i in range(2):
        c = Canvas(16, 16)
        r = 5.5 if i == 0 else 6
        c.ellipse(7.5, 7.5, r, r * 0.85, M['m'])
        c.ellipse(7.5, 7.5, r - 1, r * 0.85 - 1, M['o'])
        c.ellipse(8, 7, 3, 2.5, M['y'])
        c.ellipse(9, 6, 1.2, 1, M['W'])
        c.outline(M['K'])
        frames.append(c)
    return vstack(frames), 16


def charge_shot():
    frames = []
    for i in range(2):
        c = Canvas(32, 16)
        c.ellipse(14 + i, 7.5, 13, 7, M['b'])
        c.ellipse(17 + i, 7.5, 12, 6, M['c'])
        c.ellipse(20 + i, 7.5, 9, 4, M['W'])
        c.ellipse(9 - i, 7.5, 8, 5.5, 0)
        frames.append(c)
    return vstack(frames), 16


def enemy_bullet():
    frames = []
    for i in range(2):
        c = Canvas(8, 8)
        c.ellipse(3.5, 3.5, 3 if i == 0 else 2.6, 3 if i == 0 else 2.6, M['p'])
        c.ellipse(3.5, 3.5, 1.6, 1.6, M['W'])
        frames.append(c)
    return vstack(frames), 8


def enemy_bullet_big():
    frames = []
    for i in range(2):
        c = Canvas(8, 8)
        c.ellipse(3.5, 3.5, 3.9, 3.9, M['r'] if i == 0 else M['o'])
        c.ellipse(3.5, 3.5, 2.6, 2.6, M['o'] if i == 0 else M['y'])
        c.ellipse(3, 3, 1.2, 1.2, M['W'])
        frames.append(c)
    return vstack(frames), 8


def enemy_needle():
    rows = [
        "........",
        "........",
        "........",
        ".WyyooR.",
        "........",
        "........",
        "........",
        "........",
    ]
    rows = [r.replace('R', 'r') for r in rows]
    return vstack([art(rows, 8, 8, outline=False)]), 8


# ------------------------------------------------------------------------------------------------
# Effects
# ------------------------------------------------------------------------------------------------

def explosion(size, seed):
    frames = []
    rng = LCG(seed)
    half = size / 2
    debris = [(rng.randint(0, 628) / 100, rng.randint(30, 100) / 100) for _ in range(10)]
    stages = [
        (0.25, 0.00, 'W', 'y'),
        (0.45, 0.10, 'y', 'o'),
        (0.62, 0.25, 'o', 'r'),
        (0.75, 0.45, 'r', 'm'),
        (0.85, 0.62, 'm', 'd'),
        (0.92, 0.80, 'd', 'n'),
    ]
    for i, (r_out, r_in, c_out, c_in) in enumerate(stages):
        c = Canvas(size, size)
        R = half * r_out
        c.ellipse(half, half, R, R, M[c_out])
        c.ellipse(half - R * 0.15, half - R * 0.15, R * 0.6, R * 0.6, M[c_in])
        if r_in > 0:
            c.ellipse(half, half, half * r_in, half * r_in, 0)
        for (a, s) in debris:
            d = half * min(0.95, r_out + 0.1) * s
            x = half + math.cos(a) * d
            y = half + math.sin(a) * d
            c.set(int(x), int(y), M['y' if i < 3 else 'o' if i < 5 else 'd'])
        frames.append(c)
    return vstack(frames), size


def spark():
    frames = []
    for i in range(3):
        c = Canvas(8, 8)
        r = 1 + i
        col = M['W'] if i == 0 else M['y'] if i == 1 else M['o']
        c.line(3.5 - r, 3.5, 3.5 + r, 3.5, col)
        c.line(3.5, 3.5 - r, 3.5, 3.5 + r, col)
        if i < 2:
            c.set(3, 3, M['W'])
        frames.append(c)
    return vstack(frames), 8


def shield():
    frames = []
    for i in range(2):
        c = Canvas(32, 32)
        for k in range(24):
            a = k * 2 * math.pi / 24 + i * math.pi / 24
            x = 15.5 + math.cos(a) * 12
            y = 15.5 + math.sin(a) * 10
            c.set(int(round(x)), int(round(y)), M['c'] if k % 2 else M['W'])
            c.set(int(round(15.5 + math.cos(a) * 11)), int(round(15.5 + math.sin(a) * 9)), M['b'])
        frames.append(c)
    return vstack(frames), 32


def charge_glow():
    frames = []
    for i in range(4):
        c = Canvas(16, 16)
        r = 2 + i * 1.6
        c.ellipse(7.5, 7.5, r, r, M['b'])
        c.ellipse(7.5, 7.5, r * 0.7, r * 0.7, M['c'])
        c.ellipse(7.5, 7.5, r * 0.35, r * 0.35, M['W'])
        frames.append(c)
    return vstack(frames), 16


def _letter(c, ch, ox, oy, col):
    for y, row in enumerate(glyph_rows(ch)):
        for x, v in enumerate(row):
            if v == '#':
                c.set(ox + x, oy + y, col)


def powerups():
    """Power capsule: each one collected advances the power ladder by one step.
    Frame 0 normal, frame 1 highlighted (blink)."""
    specs = [('P', 'r', 'm'), ('P', 'o', 'r')]
    frames = []
    for ch, light, dark in specs:
        c = Canvas(16, 16)
        c.ellipse(7.5, 7.5, 7, 6.5, M[dark])
        c.ellipse(7.5, 7, 6, 5.5, M[light])
        c.ellipse(5.5, 4.5, 2, 1.2, M['W'])
        _letter(c, ch, 5, 5, M['K'])
        c.outline(M['K'])
        frames.append(c)
    return vstack(frames), 16


def boss_bar():
    """32x8 bar segments, 17 frames: 0..32 filled pixels in steps of 2."""
    frames = []
    for i in range(17):
        c = Canvas(32, 8)
        c.rect(0, 1, 32, 6, M['K'])
        c.rect(0, 2, 32, 4, M['n'])
        fill = i * 2
        if fill:
            c.rect(0, 2, fill, 4, M['r'])
            c.rect(0, 2, fill, 1, M['o'])
        frames.append(c)
    return vstack(frames), 8


# ------------------------------------------------------------------------------------------------
# Bosses (boss palette)
# ------------------------------------------------------------------------------------------------

def boss_warden():
    """Stage 1 boss: armoured carrier with a front eye core. Frames: normal, core open."""
    frames = []
    for opened in (0, 1):
        c = Canvas(64, 64)
        c.polygon([(4, 32), (14, 14), (40, 8), (60, 14), (62, 50), (40, 56), (14, 50)], B['3'])
        c.polygon([(8, 32), (17, 17), (40, 12), (57, 17), (58, 46), (40, 52), (17, 47)], B['2'])
        c.polygon([(12, 30), (20, 19), (38, 15), (54, 19), (54, 30)], B['1'])
        # hangar slots
        for y in (22, 40):
            c.rect(30, y, 22, 4, B['4'])
            c.rect(32, y + 1, 18, 2, B['3'])
        # vents
        for x in range(36, 56, 5):
            c.rect(x, 30, 2, 5, B['4'])
        # engines
        c.rect(58, 20, 6, 6, B['o'])
        c.rect(58, 38, 6, 6, B['o'])
        c.rect(60, 21, 4, 4, B['y'])
        c.rect(60, 39, 4, 4, B['y'])
        # front core
        c.ellipse(14, 32, 9, 9, B['4'])
        if opened:
            c.ellipse(14, 32, 7, 7, B['m'])
            c.ellipse(13, 31, 5, 5, B['r'])
            c.ellipse(12, 30, 2.5, 2.5, B['y'])
            c.set(11, 29, B['W'])
        else:
            c.ellipse(14, 32, 7, 7, B['3'])
            c.line(8, 32, 20, 32, B['4'])
            c.ellipse(14, 32, 2, 2, B['r'])
        c.outline(B['K'])
        frames.append(c)
    return vstack(frames), 64


def boss_hive():
    """Stage 2 boss: crystal hive around a pulsing core. Frames: closed, pulse, open."""
    frames = []
    rng = LCG(4242)
    shards = [(k * 2 * math.pi / 9 + rng.randint(0, 40) / 100, 18 + rng.randint(0, 10)) for k in range(9)]
    for f in range(3):
        c = Canvas(64, 64)
        for a, L in shards:
            spread = 0.28
            tip = (31.5 + math.cos(a) * (L + f), 31.5 + math.sin(a) * (L + f))
            b1 = (31.5 + math.cos(a - spread) * 10, 31.5 + math.sin(a - spread) * 10)
            b2 = (31.5 + math.cos(a + spread) * 10, 31.5 + math.sin(a + spread) * 10)
            c.polygon([b1, tip, b2], B['v'])
            c.line(b1[0], b1[1], tip[0], tip[1], B['p'])
        c.ellipse(31.5, 31.5, 15, 15, B['u'])
        c.ellipse(31.5, 31.5, 13, 13, B['v'])
        core_r = [6, 8, 10][f]
        c.ellipse(31.5, 31.5, core_r + 1, core_r + 1, B['m'])
        c.ellipse(31.5, 31.5, core_r, core_r, B['r'])
        c.ellipse(30, 30, core_r * 0.6, core_r * 0.6, B['o'])
        c.ellipse(29, 29, core_r * 0.25, core_r * 0.25, B['y'])
        c.outline(B['K'])
        frames.append(c)
    return vstack(frames), 64


def boss_overmind_front():
    """Final boss front half: armoured skull-like prow with a huge eye. Frames: eye closed/open/angry."""
    frames = []
    for f in range(3):
        c = Canvas(64, 64)
        c.polygon([(2, 32), (12, 10), (40, 2), (64, 4), (64, 60), (40, 62), (12, 54)], B['3'])
        c.polygon([(6, 32), (15, 13), (40, 6), (64, 8), (64, 56), (40, 58), (15, 51)], B['2'])
        c.polygon([(10, 26), (18, 15), (40, 10), (64, 11), (64, 22), (30, 22)], B['1'])
        # jaw plates
        for i in range(4):
            x = 16 + i * 10
            c.polygon([(x, 46), (x + 8, 44), (x + 8, 54), (x + 2, 52)], B['4'])
        # teal circuitry
        c.line(40, 16, 62, 16, B['t'])
        c.line(44, 48, 62, 48, B['t'])
        c.line(62, 16, 62, 48, B['t'])
        # eye socket
        c.ellipse(24, 32, 12, 9, B['4'])
        if f == 0:
            c.ellipse(24, 32, 10, 3, B['m'])
            c.line(14, 32, 34, 32, B['r'])
        else:
            c.ellipse(24, 32, 10, 7.5, B['W'] if f == 1 else B['o'])
            c.ellipse(22, 32, 5.5, 6, B['r'] if f == 1 else B['m'])
            c.ellipse(21, 31, 2.5, 3.5, B['4'] if f == 1 else B['y'])
            c.set(19, 29, B['W'])
        c.outline(B['K'])
        frames.append(c)
    return vstack(frames), 64


def boss_overmind_rear():
    frames = []
    for f in range(2):
        c = Canvas(64, 64)
        c.polygon([(0, 4), (40, 8), (56, 18), (56, 46), (40, 56), (0, 60)], B['3'])
        c.polygon([(0, 8), (38, 11), (52, 20), (52, 44), (38, 53), (0, 56)], B['2'])
        c.rect(0, 12, 40, 6, B['1'])
        for y in (24, 32, 40):
            c.rect(4, y, 34, 3, B['4'])
            c.rect(6, y + 1, 30, 1, B['t'] if f == 0 else B['c'])
        # engine nozzles
        for y in (14, 28, 42):
            c.rect(52, y, 8, 8, B['4'])
            c.rect(58, y + 1, 6, 6, B['o'] if f == 0 else B['y'])
            c.rect(60, y + 2, 4, 4, B['y'] if f == 0 else B['W'])
        c.outline(B['K'])
        frames.append(c)
    return vstack(frames), 64


def boss_pod():
    """Small detachable turret pod used by the final boss (boss palette)."""
    frames = []
    for f in range(2):
        c = Canvas(16, 16)
        c.ellipse(7.5, 7.5, 6.5, 5.5, B['3'])
        c.ellipse(7.5, 6.5, 5, 3.5, B['2'])
        c.rect(0, 7, 5, 2, B['4'])
        c.ellipse(7.5, 7.5, 2.5, 2.5, B['r'] if f == 0 else B['y'])
        c.outline(B['K'])
        frames.append(c)
    return vstack(frames), 16
