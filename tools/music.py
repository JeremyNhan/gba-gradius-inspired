"""
Original music for Space Shooter, written as ProTracker 4-channel modules (M.K. format).

Maxmod (Butano's audio backend) plays *.mod files natively (docs/research.md). Instruments are tiny
single-cycle waveforms plus three synthesized drums, so every song is only a few KB.

Composition notation
--------------------
Melodies are strings of "NOTE:ROWS" tokens, e.g. "E5:2 G5:2 -:4" ('-' = rest). One row is a 16th note.
Chords are "ROOT[quality]" per bar (16 rows): quality m (minor), M (major), 7 (dominant 7th).
Channel layout: 1 lead, 2 arpeggio, 3 bass, 4 drums.
"""

import math
import struct

from pixelart import LCG

# Amiga periods, finetune 0, MOD octaves 1..3.
PERIODS = [856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
           428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
           214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113]
SEMITONE = {'C': 0, 'C#': 1, 'D': 2, 'D#': 3, 'E': 4, 'F': 5, 'F#': 6, 'G': 7, 'G#': 8, 'A': 9,
            'A#': 10, 'B': 11}
CHORDS = {'m': (0, 3, 7, 12), 'M': (0, 4, 7, 12), '7': (0, 4, 7, 10)}

# ----- instruments --------------------------------------------------------------------------------
# (name, data, default volume, loop) ; octave_shift maps scientific pitch to MOD octave.
INS_LEAD, INS_PULSE, INS_BASS, INS_ARP, INS_KICK, INS_SNARE, INS_HAT = 1, 2, 3, 4, 5, 6, 7
OCTAVE_SHIFT = {INS_LEAD: 2, INS_PULSE: 2, INS_ARP: 2, INS_BASS: 1}


def _cycle(fn, n):
    return [fn(i / n) for i in range(n)]


def _to_s8(vals, amp=100):
    return bytes((int(round(v * amp)) & 0xFF) for v in vals)


def _drum_kick():
    n, out, phase = 2400, [], 0.0
    for i in range(n):
        f = 45 + 160 * math.exp(-i / 350)
        phase += f / 8363
        out.append(math.sin(2 * math.pi * phase) * math.exp(-i / 1200))
    return out


def _drum_snare():
    rng, n, out, phase = LCG(3), 2600, [], 0.0
    for i in range(n):
        phase += 190 / 8363
        noise = (rng.next() % 2001) / 1000 - 1
        out.append((noise * 0.7 + math.sin(2 * math.pi * phase) * 0.4 * math.exp(-i / 500)) * math.exp(-i / 700))
    return out


def _drum_hat():
    rng, n, out, prev = LCG(9), 700, [], 0.0
    for i in range(n):
        v = (rng.next() % 2001) / 1000 - 1
        out.append((v - prev) * 0.6 * math.exp(-i / 160))
        prev = v
    return out


def instruments():
    square = _cycle(lambda t: 0.75 if t < 0.5 else -0.75, 32)
    pulse = _cycle(lambda t: 0.7 if t < 0.25 else -0.7, 32)
    tri = _cycle(lambda t: 4 * abs(t - 0.5) - 1, 64)
    saw = _cycle(lambda t: (2 * t - 1) * 0.8, 32)
    return [
        ('lead', _to_s8(square), 40, True),
        ('pulse', _to_s8(pulse), 34, True),
        ('bass', _to_s8(tri, 120), 52, True),
        ('arp', _to_s8(saw), 16, True),
        ('kick', _to_s8(_drum_kick(), 125), 60, False),
        ('snare', _to_s8(_drum_snare(), 110), 42, False),
        ('hat', _to_s8(_drum_hat(), 110), 22, False),
    ]


# ----- note helpers -------------------------------------------------------------------------------

def parse_note(name):
    """'C#5' -> (semitone, octave)."""
    pitch, octave = name[:-1], int(name[-1])
    return SEMITONE[pitch], octave


def period_for(semitone, sci_octave, ins):
    idx = (sci_octave - OCTAVE_SHIFT[ins] - 1) * 12 + semitone
    while idx < 0:
        idx += 12
    while idx >= len(PERIODS):
        idx -= 12
    return PERIODS[idx]


def transpose(semitone, octave, interval):
    total = octave * 12 + semitone + interval
    return total % 12, total // 12


# ----- pattern building ---------------------------------------------------------------------------

class Song:
    def __init__(self, title, speed, bpm):
        self.title = title
        self.speed = speed
        self.bpm = bpm
        self.patterns = []   # each: list of 64 rows, each row: list of 4 cells (ins, period, eff, param)
        self.order = []

    def add_pattern(self, lead, chords, bass_style, drums, arp=True, lead_ins=INS_LEAD, end_row=None):
        rows = [[(0, 0, 0, 0) for _ in range(4)] for _ in range(64)]
        _write_melody(rows, 0, lead, lead_ins)
        _write_chords(rows, chords, bass_style, arp)
        _write_drums(rows, drums)
        rows[0][2] = rows[0][2][:2] + (0xF, self.speed)
        rows[0][3] = rows[0][3][:2] + (0xF, self.bpm)
        if end_row is not None:
            rows[end_row][1] = rows[end_row][1][:2] + (0xD, 0)  # pattern break -> end of song
        self.patterns.append(rows)
        return len(self.patterns) - 1


def _write_melody(rows, ch, text, ins):
    if not text:
        return
    r = 0
    for tok in text.split():
        note, length = tok.split(':')
        length = int(length)
        if r >= 64:
            break
        if note == '-':
            rows[r][ch] = (0, 0, 0xC, 0)
        else:
            s, o = parse_note(note)
            rows[r][ch] = (ins, period_for(s, o, ins), 0, 0)
        r += length
    assert r <= 64, 'melody too long: %d rows' % r


def _chord_tones(chord):
    quality = chord[-1] if chord[-1] in CHORDS else 'M'
    root = chord[:-1] if chord[-1] in CHORDS else chord
    return root, CHORDS[quality]


def _write_chords(rows, chords, bass_style, arp):
    for bar, chord in enumerate(chords):
        root, ivs = _chord_tones(chord)
        rs = SEMITONE[root]
        base = bar * 16
        for i in range(16):
            r = base + i
            if arp:
                s, o = transpose(rs, 4, ivs[i % 3 if i % 8 < 6 else 3])
                rows[r][1] = (INS_ARP, period_for(s, o, INS_ARP), 0, 0)
            pat = {
                'eighths': 'R.R.R.R.R.R.R.O.',
                'drive': 'RRO.RRO.RRO.RROR',
                'slow': 'R.......O.......',
                'pulse': 'R...R...R...O.R.',
                'walk': 'R...F...O...F...',
            }[bass_style]
            ch = pat[i]
            if ch == '.':
                continue
            if ch == 'R':
                s, o = rs, 2
            elif ch == 'O':
                s, o = rs, 3
            else:  # F: fifth
                s, o = transpose(rs, 2, 7)
            rows[r][2] = (INS_BASS, period_for(s, o, INS_BASS), 0, 0)


DRUM_INS = {'K': INS_KICK, 'S': INS_SNARE, 'H': INS_HAT}


def _write_drums(rows, pattern):
    if not pattern:
        return
    pattern = pattern.replace(' ', '')
    for r in range(64):
        ch = pattern[r % len(pattern)]
        if ch in DRUM_INS:
            rows[r][3] = (DRUM_INS[ch], 428, 0, 0)


# ----- MOD serialisation --------------------------------------------------------------------------

def mod_bytes(song):
    ins = instruments()
    out = bytearray()
    out += song.title.encode('ascii')[:20].ljust(20, b'\0')
    for i in range(31):
        if i < len(ins):
            name, data, vol, loop = ins[i]
            length = len(data) // 2
            out += name.encode('ascii').ljust(22, b'\0')
            out += struct.pack('>HBBHH', length, 0, vol, 0 if loop else 0, length if loop else 1)
        else:
            out += bytes(22) + struct.pack('>HBBHH', 0, 0, 0, 0, 1)
    out += bytes([len(song.order), 127])
    out += bytes(song.order) + bytes(128 - len(song.order))
    out += b'M.K.'
    for pat in song.patterns:
        for row in pat:
            for (inst, per, eff, param) in row:
                out += bytes([(inst & 0xF0) | ((per >> 8) & 0x0F), per & 0xFF, ((inst & 0x0F) << 4) | eff, param])
    for _, data, _, loop in ins:
        if not loop:
            data = b'\0\0' + data[2:]
        out += data
    return bytes(out)


# ----- the songs ----------------------------------------------------------------------------------
DR_BASIC = "K...H...S...H... K...H...S...H.H."
DR_DRIVE = "K.H.S.H.K.K.S.H. K.H.S.H.K.H.S.HH"
DR_HALF = "K.......S....... K.......S...H..."
DR_BOSS = "K.HKS.HKK.HKS.HS"
DR_NONE = ""


def song_title():
    s = Song('SS title', 6, 120)
    ch = ['Am', 'FM', 'CM', 'GM']
    a = s.add_pattern("A4:4 C5:2 E5:2 D5:4 C5:2 B4:2 A4:6 F4:2 A4:4 C5:4 "
                      "G4:4 E4:2 G4:2 C5:4 B4:2 C5:2 D5:8 B4:4 G4:4", ch, 'eighths', DR_BASIC)
    b = s.add_pattern("E5:4 D5:2 C5:2 B4:4 C5:2 D5:2 C5:6 A4:2 F4:4 A4:4 "
                      "G4:4 C5:4 E5:4 G5:4 F5:4 E5:4 D5:4 B4:4", ch, 'eighths', DR_BASIC)
    intro = s.add_pattern("", ch, 'slow', DR_HALF)
    s.order = [intro, a, b, a, b]
    return s


def song_stage1():
    s = Song('SS stage 1', 5, 140)
    ch1 = ['Em', 'CM', 'DM', 'BM']
    ch2 = ['Em', 'CM', 'GM', 'DM']
    a = s.add_pattern("E5:2 -:2 E5:2 G5:2 F#5:2 E5:2 D5:2 B4:2 C5:4 E5:4 G5:4 E5:4 "
                      "D5:2 -:2 D5:2 F#5:2 A5:4 F#5:4 B4:8 D#5:4 F#5:4", ch1, 'drive', DR_DRIVE)
    b = s.add_pattern("G5:4 F#5:2 E5:2 B4:4 E5:4 C5:2 D5:2 E5:4 G5:4 A5:4 "
                      "G5:4 F#5:4 E5:4 D5:4 F#5:8 D5:4 A4:4", ch2, 'drive', DR_DRIVE)
    c = s.add_pattern("", ch1, 'pulse', DR_BASIC)
    s.order = [c, a, b, a, b, c]
    return s


def song_stage2():
    s = Song('SS stage 2', 6, 110)
    ch = ['Dm', 'A#M', 'Gm', 'AM']
    a = s.add_pattern("D5:6 E5:2 F5:4 A5:4 G5:8 F5:4 D5:4 E5:6 D5:2 A#4:4 G4:4 A4:8 C#5:8",
                      ch, 'walk', DR_HALF, lead_ins=INS_PULSE)
    b = s.add_pattern("A5:4 G5:2 F5:2 E5:4 F5:4 D5:8 A#4:8 G4:4 A#4:4 D5:4 G5:4 E5:8 C#5:4 A4:4",
                      ch, 'walk', DR_BASIC, lead_ins=INS_PULSE)
    c = s.add_pattern("", ch, 'slow', DR_HALF)
    s.order = [c, a, b, a, b]
    return s


def song_stage3():
    s = Song('SS stage 3', 5, 150)
    ch = ['Cm', 'G#M', 'A#M', 'GM']
    a = s.add_pattern("C5:2 C5:2 D#5:2 G5:2 -:2 G5:2 F5:2 D#5:2 G#4:4 C5:4 D#5:4 G#5:4 "
                      "A#4:2 A#4:2 D5:2 F5:2 A#5:4 F5:4 G5:8 D5:4 B4:4", ch, 'drive', DR_DRIVE)
    b = s.add_pattern("D#5:4 D5:4 C5:4 G4:4 G#4:4 A#4:4 C5:8 D5:4 D#5:4 F5:4 A#5:4 B4:4 D5:4 G5:8",
                      ch, 'drive', DR_DRIVE)
    c = s.add_pattern("", ch, 'drive', DR_BASIC)
    s.order = [c, a, b, a, b]
    return s


def song_boss():
    s = Song('SS boss', 4, 150)
    ch = ['F#m', 'F#m', 'DM', 'EM']
    a = s.add_pattern("F#5:2 -:2 F#5:2 -:2 A5:2 G#5:2 F#5:2 C#5:2 D5:4 C#5:4 A4:4 C#5:4 "
                      "F#5:2 -:2 F#5:2 -:2 A5:2 B5:2 A5:2 G#5:2 E5:8 G#5:8", ch, 'drive', DR_BOSS)
    b = s.add_pattern("C#5:4 D5:4 E5:4 F#5:4 D5:8 A4:8 E5:4 F#5:4 G#5:4 A5:4 B5:8 G#5:8",
                      ch, 'drive', DR_BOSS)
    s.order = [a, b]
    return s


def song_ending():
    s = Song('SS ending', 6, 104)
    ch1 = ['CM', 'GM', 'Am', 'FM']
    ch2 = ['FM', 'GM', 'CM', 'CM']
    a = s.add_pattern("E5:4 G5:4 C5:4 E5:4 D5:6 B4:2 G4:8 A4:4 C5:4 E5:4 A5:4 F5:8 E5:4 D5:4",
                      ch1, 'walk', DR_BASIC)
    b = s.add_pattern("C5:4 A4:4 F4:4 A4:4 B4:4 D5:4 G5:8 E5:4 G5:4 C5:4 E5:4 C5:16",
                      ch2, 'walk', DR_HALF)
    s.order = [a, b]
    return s


def jingle_clear():
    s = Song('SS clear', 5, 140)
    p = s.add_pattern("G4:2 C5:2 E5:2 G5:6 -:2 F5:2 A5:2 G5:4 E5:2 C5:2 E5:4 G5:4 C5:12 -:4",
                      ['CM', 'FM', 'CM', 'CM'], 'slow', "K...K...K...S...", end_row=None)
    s.order = [p]
    return s


def jingle_gameover():
    s = Song('SS game over', 7, 100)
    p = s.add_pattern("A4:4 G4:4 F4:4 E4:4 D4:4 C4:4 B3:4 E4:4 A3:16 -:16",
                      ['Am', 'Dm', 'Am', 'Am'], 'slow', "", arp=False)
    s.order = [p]
    return s


def build_all():
    songs = {
        'music_title': song_title(),
        'music_stage1': song_stage1(),
        'music_stage2': song_stage2(),
        'music_stage3': song_stage3(),
        'music_boss': song_boss(),
        'music_ending': song_ending(),
        'jingle_clear': jingle_clear(),
        'jingle_gameover': jingle_gameover(),
    }
    return {k: mod_bytes(v) for k, v in songs.items()}
