#!/usr/bin/env python3
"""
Space Shooter asset generator.

    python tools/gen_assets.py [--out assets/generated] [--preview DIR]

Produces every graphic (indexed BMP + Butano JSON descriptor) and every audio file (MOD music,
WAV effects) from the sources in tools/*.py. Files are only rewritten when their bytes change,
so running it before every build (Makefile EXTTOOL) keeps incremental builds fast.
"""

import argparse
import json
import os
import struct
import sys
import zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import backgrounds as bg          # noqa: E402
import font                        # noqa: E402
import music                       # noqa: E402
import palettes as pal             # noqa: E402
import sfx                         # noqa: E402
import sprites as spr              # noqa: E402
from pixelart import Canvas, bmp_bytes, count_unique_tiles  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

MASTER = pal.colors_of(pal.MASTER)
BOSS = pal.colors_of(pal.BOSS)


def sprite_items():
    """(name, generator, palette colours)"""
    return [
        ('player', spr.player, MASTER),
        ('life_icon', spr.life_icon, MASTER),
        ('enemy_dart', spr.dart, MASTER),
        ('enemy_waver', spr.waver, MASTER),
        ('enemy_interceptor', spr.interceptor, MASTER),
        ('enemy_turret', spr.turret, MASTER),
        ('enemy_hulk', spr.hulk, MASTER),
        ('enemy_swarm', spr.swarm, MASTER),
        ('enemy_mine', spr.mine, MASTER),
        ('asteroid_small', spr.asteroid_small, MASTER),
        ('asteroid_big', spr.asteroid_big, MASTER),
        ('shot_normal', spr.player_shot, MASTER),
        ('shot_spread', spr.spread_shot, MASTER),
        ('shot_missile', spr.missile, MASTER),
        ('shot_charge', spr.charge_shot, MASTER),
        ('shot_laser', spr.laser, MASTER),
        ('shooter', spr.shooter, MASTER),
        ('bullet_small', spr.enemy_bullet, MASTER),
        ('bullet_big', spr.enemy_bullet_big, MASTER),
        ('bullet_needle', spr.enemy_needle, MASTER),
        ('explosion_small', lambda: spr.explosion(16, 5), MASTER),
        ('explosion_big', lambda: spr.explosion(32, 8), MASTER),
        ('spark', spr.spark, MASTER),
        ('shield', spr.shield, MASTER),
        ('charge_glow', spr.charge_glow, MASTER),
        ('powerup', spr.powerups, MASTER),
        ('boss_bar', spr.boss_bar, MASTER),
        ('boss_warden', spr.boss_warden, BOSS),
        ('boss_hive', spr.boss_hive, BOSS),
        ('boss_overmind_front', spr.boss_overmind_front, BOSS),
        ('boss_overmind_rear', spr.boss_overmind_rear, BOSS),
        ('boss_pod', spr.boss_pod, BOSS),
        ('font', lambda: (font.font_sheet(), 8), pal.FONT_WHITE),
    ]


def sprite_palettes():
    return [
        ('flash_master', pal.flash_of(pal.MASTER)),
        ('flash_boss', pal.flash_of(pal.BOSS)),
        ('font_yellow', pal.FONT_YELLOW),
        ('font_cyan', pal.FONT_CYAN),
        ('font_red', pal.FONT_RED),
    ]


def background_items():
    return [
        ('bg_stars', bg.starfield, bg.STARS_PAL),
        ('bg_nebula', bg.nebula_backdrop, bg.NEBULA_PAL),
        ('bg_cave', bg.cave_backdrop, bg.CAVE_PAL),
        ('bg_hull', bg.hull_backdrop, bg.HULL_PAL),
        ('bg_title_logo', bg.title_logo, bg.LOGO_PAL),
    ]


class Writer:
    def __init__(self, out_dir):
        self.out = out_dir
        self.changed = 0
        self.total = 0

    def write(self, rel, data):
        path = os.path.join(self.out, rel)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        self.total += 1
        if isinstance(data, str):
            data = data.encode('utf-8')
        try:
            with open(path, 'rb') as f:
                if f.read() == data:
                    return
        except FileNotFoundError:
            pass
        with open(path, 'wb') as f:
            f.write(data)
        self.changed += 1

    def json(self, rel, obj):
        self.write(rel, json.dumps(obj, indent=4, sort_keys=True) + '\n')


def png_bytes(canvas, palette, scale=3):
    """Minimal PNG encoder (for previews only)."""
    raw = bytearray()
    for y in range(canvas.h):
        row = bytearray([0])
        for x in range(canvas.w):
            idx = canvas.px[y][x]
            r, g, b = palette[idx] if idx < len(palette) else (0, 0, 0)
            if idx == 0:
                r, g, b = (40, 40, 40) if ((x // 4 + y // 4) % 2) else (60, 60, 60)
            row += bytes([r, g, b]) * scale
        raw += bytes(row) * scale

    def chunk(t, d):
        return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xFFFFFFFF)

    ihdr = struct.pack('>IIBBBBB', canvas.w * scale, canvas.h * scale, 8, 2, 0, 0, 0)
    return b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', ihdr) + chunk(b'IDAT', zlib.compress(bytes(raw), 9)) + \
        chunk(b'IEND', b'')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', default=os.path.join(ROOT, 'assets', 'generated'))
    ap.add_argument('--preview', default=None, help='also write scaled PNG previews to this folder')
    ap.add_argument('--quiet', action='store_true')
    args = ap.parse_args()

    w = Writer(args.out)
    gfx = 'graphics'
    report = []

    for name, fn, colors in sprite_items():
        canvas, frame_h = fn()
        w.write(f'{gfx}/{name}.bmp', bmp_bytes(canvas, colors))
        w.json(f'{gfx}/{name}.json', {'type': 'sprite', 'height': frame_h, 'bpp_mode': 'bpp_4'})
        report.append((name, f'{canvas.w}x{frame_h} x{canvas.h // frame_h}'))
        if args.preview:
            os.makedirs(args.preview, exist_ok=True)
            with open(os.path.join(args.preview, name + '.png'), 'wb') as f:
                f.write(png_bytes(canvas, colors, 4 if canvas.w <= 32 else 2))

    for name, colors in sprite_palettes():
        c = Canvas(8, 8)
        w.write(f'{gfx}/{name}.bmp', bmp_bytes(c, colors))
        w.json(f'{gfx}/{name}.json', {'type': 'sprite_palette', 'bpp_mode': 'bpp_4'})

    for name, fn, colors in background_items():
        canvas = fn()
        tiles = count_unique_tiles(canvas)
        assert tiles <= 1024, f'{name}: too many tiles ({tiles})'
        w.write(f'{gfx}/{name}.bmp', bmp_bytes(canvas, colors))
        w.json(f'{gfx}/{name}.json', {'type': 'regular_bg', 'bpp_mode': 'bpp_4_manual'})
        report.append((name, f'256x256, {tiles} unique tiles ({tiles * 32} bytes)'))
        if args.preview:
            with open(os.path.join(args.preview, name + '.png'), 'wb') as f:
                f.write(png_bytes(canvas, colors, 2))

    for name, style, colors in (('terrain_crystal', 'crystal', bg.CRYSTAL_TERRAIN_PAL),
                                ('terrain_metal', 'metal', bg.METAL_TERRAIN_PAL)):
        strip = bg.terrain_tiles(style)
        w.write(f'{gfx}/{name}.bmp', bmp_bytes(strip, colors))
        w.json(f'{gfx}/{name}.json', {'type': 'regular_bg_tiles', 'bpp_mode': 'bpp_4'})
        w.write(f'{gfx}/{name}_palette.bmp', bmp_bytes(Canvas(8, 8), colors))
        w.json(f'{gfx}/{name}_palette.json', {'type': 'bg_palette', 'bpp_mode': 'bpp_4'})
        if args.preview:
            with open(os.path.join(args.preview, name + '.png'), 'wb') as f:
                f.write(png_bytes(strip, colors, 6))

    audio_bytes = 0
    for name, data in sorted(music.build_all().items()):
        w.write(f'audio/{name}.mod', data)
        audio_bytes += len(data)
    for name, data in sorted(sfx.build_all().items()):
        w.write(f'audio/{name}.wav', data)
        audio_bytes += len(data)
    report.append(('audio', f'{audio_bytes} bytes source audio'))

    if not args.quiet:
        for name, info in report:
            print(f'  {name:22s} {info}')
    print(f'gen_assets: {w.total} files, {w.changed} updated')


if __name__ == '__main__':
    main()
