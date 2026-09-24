"""
Tiny indexed-color pixel-art toolkit used by the asset generator.

Everything here is deterministic: the same inputs always produce byte-identical files.
Only the Python standard library is used.
"""

import math
import struct


class Canvas:
    """A width x height grid of palette indices (0 = transparent)."""

    def __init__(self, width, height, fill=0):
        self.w = width
        self.h = height
        self.px = [[fill] * width for _ in range(height)]

    # ----- basic access -------------------------------------------------------------------------
    def get(self, x, y):
        if 0 <= x < self.w and 0 <= y < self.h:
            return self.px[y][x]
        return 0

    def set(self, x, y, c):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[y][x] = c

    def copy(self):
        out = Canvas(self.w, self.h)
        out.px = [row[:] for row in self.px]
        return out

    # ----- primitives ---------------------------------------------------------------------------
    def rect(self, x0, y0, w, h, c):
        for y in range(y0, y0 + h):
            for x in range(x0, x0 + w):
                self.set(x, y, c)

    def ellipse(self, cx, cy, rx, ry, c):
        """Filled ellipse centred on (cx, cy); radii may be fractional."""
        for y in range(int(cy - ry) - 1, int(cy + ry) + 2):
            for x in range(int(cx - rx) - 1, int(cx + rx) + 2):
                dx = (x + 0.5 - cx) / max(rx, 0.01)
                dy = (y + 0.5 - cy) / max(ry, 0.01)
                if dx * dx + dy * dy <= 1.0:
                    self.set(x, y, c)

    def ring(self, cx, cy, r_out, r_in, c):
        for y in range(int(cy - r_out) - 1, int(cy + r_out) + 2):
            for x in range(int(cx - r_out) - 1, int(cx + r_out) + 2):
                d = math.hypot(x + 0.5 - cx, y + 0.5 - cy)
                if r_in <= d <= r_out:
                    self.set(x, y, c)

    def polygon(self, pts, c):
        """Even-odd scanline fill of a polygon given as [(x, y), ...] (pixel-centre sampling)."""
        ys = [p[1] for p in pts]
        for y in range(int(min(ys)), int(math.ceil(max(ys))) + 1):
            sy = y + 0.5
            xs = []
            n = len(pts)
            for i in range(n):
                (x1, y1), (x2, y2) = pts[i], pts[(i + 1) % n]
                if (y1 <= sy < y2) or (y2 <= sy < y1):
                    xs.append(x1 + (sy - y1) * (x2 - x1) / (y2 - y1))
            xs.sort()
            for i in range(0, len(xs) - 1, 2):
                for x in range(int(math.ceil(xs[i] - 0.5)), int(math.floor(xs[i + 1] - 0.5)) + 1):
                    self.set(x, y, c)

    def line(self, x0, y0, x1, y1, c):
        steps = int(max(abs(x1 - x0), abs(y1 - y0))) + 1
        for i in range(steps + 1):
            t = i / steps
            self.set(int(round(x0 + (x1 - x0) * t)), int(round(y0 + (y1 - y0) * t)), c)

    def blit_ascii(self, rows, charmap, ox=0, oy=0):
        """Draw ASCII art. '.' and ' ' are transparent (skipped)."""
        for y, row in enumerate(rows):
            for x, ch in enumerate(row):
                if ch in '. ':
                    continue
                self.set(ox + x, oy + y, charmap[ch])

    def blit(self, other, ox, oy):
        for y in range(other.h):
            for x in range(other.w):
                c = other.px[y][x]
                if c:
                    self.set(ox + x, oy + y, c)

    # ----- effects ------------------------------------------------------------------------------
    def outline(self, c, diagonal=False):
        """Paint every transparent pixel that touches an opaque pixel with colour c."""
        src = self.copy()
        nbrs = [(1, 0), (-1, 0), (0, 1), (0, -1)]
        if diagonal:
            nbrs += [(1, 1), (-1, -1), (1, -1), (-1, 1)]
        for y in range(self.h):
            for x in range(self.w):
                if src.px[y][x] == 0 and any(src.get(x + dx, y + dy) for dx, dy in nbrs):
                    self.px[y][x] = c
        return self

    def replace(self, a, b):
        for row in self.px:
            for i, v in enumerate(row):
                if v == a:
                    row[i] = b
        return self

    def flip_h(self):
        out = Canvas(self.w, self.h)
        out.px = [row[::-1] for row in self.px]
        return out

    def rotated90(self, times=1):
        out = self
        for _ in range(times % 4):
            r = Canvas(out.h, out.w)
            for y in range(out.h):
                for x in range(out.w):
                    r.px[x][out.h - 1 - y] = out.px[y][x]
            out = r
        return out


def vstack(canvases):
    """Stack equally-sized frames vertically (Butano sprite sheets are vertical strips)."""
    w = canvases[0].w
    out = Canvas(w, sum(c.h for c in canvases))
    y = 0
    for c in canvases:
        assert c.w == w, 'frame width mismatch'
        _blit_raw(out, c, 0, y)
        y += c.h
    return out


def _blit_raw(dst, src, ox, oy):
    for y in range(src.h):
        dst.px[oy + y][ox:ox + src.w] = src.px[y][:]


class LCG:
    """Deterministic pseudo-random generator (independent of Python's random module version)."""

    def __init__(self, seed):
        self.s = seed & 0xFFFFFFFF

    def next(self):
        self.s = (self.s * 1664525 + 1013904223) & 0xFFFFFFFF
        return self.s >> 8

    def randint(self, a, b):
        return a + self.next() % (b - a + 1)

    def chance(self, num, den):
        return self.next() % den < num


# ----- BMP writing --------------------------------------------------------------------------------

def bmp_bytes(canvas, palette):
    """Serialise an indexed canvas as an uncompressed 4bpp (<=16 colours) or 8bpp BMP.

    palette: list of (r, g, b) 0..255 tuples. Index 0 is the transparent/backdrop colour.
    """
    ncolors = len(palette)
    bpp = 4 if ncolors <= 16 else 8
    pal_entries = 16 if bpp == 4 else 256
    pal = list(palette) + [(0, 0, 0)] * (pal_entries - ncolors)

    row_bytes = (canvas.w * bpp + 7) // 8
    row_stride = (row_bytes + 3) & ~3
    pixel_data = bytearray()
    for y in range(canvas.h - 1, -1, -1):  # BMP rows are stored bottom-up
        row = canvas.px[y]
        if bpp == 4:
            b = bytearray()
            for x in range(0, canvas.w, 2):
                hi = row[x] & 0xF
                lo = row[x + 1] & 0xF if x + 1 < canvas.w else 0
                b.append((hi << 4) | lo)
        else:
            b = bytearray(v & 0xFF for v in row)
        b += bytes(row_stride - len(b))
        pixel_data += b

    header_size = 14 + 40
    pal_size = pal_entries * 4
    offset = header_size + pal_size
    file_size = offset + len(pixel_data)
    out = bytearray()
    out += b'BM' + struct.pack('<IHHI', file_size, 0, 0, offset)
    out += struct.pack('<IiiHHIIiiII', 40, canvas.w, canvas.h, 1, bpp, 0, len(pixel_data), 2835, 2835,
                       pal_entries, 0)
    for r, g, b in pal:
        out += struct.pack('<BBBB', b, g, r, 0)
    out += pixel_data
    return bytes(out)


def count_unique_tiles(canvas):
    """Number of distinct 8x8 tiles (counting flipped duplicates as the same), for VRAM budgeting."""
    seen = set()
    for ty in range(0, canvas.h, 8):
        for tx in range(0, canvas.w, 8):
            t = tuple(tuple(canvas.px[ty + y][tx:tx + 8]) for y in range(8))
            h = tuple(r[::-1] for r in t)
            v = t[::-1]
            hv = h[::-1]
            if not ({t, h, v, hv} & seen):
                seen.add(t)
    return len(seen)
