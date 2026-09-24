"""
Sound-effect synthesiser: writes 8-bit unsigned mono PCM WAV files (Butano/Maxmod recommend
"8-bits 22050 Hz", see docs/research.md). All noise comes from a seeded LCG, so output is deterministic.
"""

import math
import struct

from pixelart import LCG

RATE = 22050


def wav_bytes(samples):
    """samples: floats in [-1, 1]."""
    data = bytes(max(0, min(255, int(round(128 + s * 127)))) for s in samples)
    if len(data) % 2:
        data += b'\x80'
    header = b'RIFF' + struct.pack('<I', 36 + len(data)) + b'WAVE'
    fmt = b'fmt ' + struct.pack('<IHHIIHH', 16, 1, 1, RATE, RATE, 1, 8)
    return header + fmt + b'data' + struct.pack('<I', len(data)) + data


def _env(i, n, attack=0.005, curve=1.0):
    t = i / RATE
    a = min(1.0, t / attack) if attack > 0 else 1.0
    d = (1.0 - i / n) ** curve
    return a * d


def _square(phase, duty=0.5):
    return 1.0 if (phase % 1.0) < duty else -1.0


def _sweep(f0, f1, dur, wave='square', vol=0.6, curve=1.0, duty=0.5):
    n = int(dur * RATE)
    out = []
    phase = 0.0
    for i in range(n):
        t = i / n
        f = f0 * (f1 / f0) ** t
        phase += f / RATE
        if wave == 'square':
            v = _square(phase, duty)
        elif wave == 'saw':
            v = 2.0 * (phase % 1.0) - 1.0
        elif wave == 'tri':
            v = 4.0 * abs((phase % 1.0) - 0.5) - 1.0
        else:
            v = math.sin(2 * math.pi * phase)
        out.append(v * vol * _env(i, n, curve=curve))
    return out


def _noise(dur, vol=0.7, seed=1, curve=1.5, lowpass_start=1.0, lowpass_end=0.1, hold=1):
    n = int(dur * RATE)
    rng = LCG(seed)
    out = []
    y = 0.0
    cur = 0.0
    for i in range(n):
        if i % hold == 0:
            cur = (rng.next() % 2001) / 1000.0 - 1.0
        k = lowpass_start + (lowpass_end - lowpass_start) * (i / n)
        y += (cur - y) * k
        out.append(y * vol * _env(i, n, curve=curve) * (1.6 if k < 0.3 else 1.0))
    return out


def _mix(*tracks):
    n = max(len(t) for t in tracks)
    return [max(-1.0, min(1.0, sum(t[i] for t in tracks if i < len(t)))) for i in range(n)]


def _concat(*parts):
    out = []
    for p in parts:
        out += p
    return out


def _tones(freqs, each, wave='square', vol=0.5, duty=0.5):
    """Sequence of fixed-pitch notes; a frequency of 0 is a rest."""
    return _concat(*[_sweep(f, f, each, wave, vol, curve=0.6, duty=duty) if f > 0 else [0.0] * int(each * RATE)
                     for f in freqs])


def build_all():
    s = {}
    s['sfx_shot'] = _sweep(1400, 500, 0.06, 'square', 0.35, curve=1.2, duty=0.25)
    s['sfx_spread'] = _mix(_sweep(1000, 420, 0.07, 'square', 0.3, duty=0.5), _noise(0.05, 0.15, seed=3))
    s['sfx_missile'] = _mix(_noise(0.14, 0.45, seed=5, lowpass_start=0.6, lowpass_end=0.05),
                            _sweep(300, 900, 0.14, 'saw', 0.15))
    s['sfx_charge_ready'] = _tones([880, 1320], 0.05, 'square', 0.35, duty=0.25)
    s['sfx_beam'] = _mix(_sweep(1800, 90, 0.38, 'saw', 0.5, curve=0.8),
                         _noise(0.38, 0.35, seed=7, lowpass_start=0.9, lowpass_end=0.05))
    s['sfx_hit'] = _mix(_noise(0.035, 0.5, seed=11, lowpass_start=1.0, lowpass_end=0.6),
                        _sweep(2200, 1400, 0.03, 'square', 0.2))
    s['sfx_explode'] = _noise(0.40, 0.95, seed=13, curve=1.6, lowpass_start=0.8, lowpass_end=0.04, hold=2)
    s['sfx_explode_big'] = _mix(_noise(1.0, 1.0, seed=17, curve=1.3, lowpass_start=0.5, lowpass_end=0.02, hold=3),
                                _sweep(120, 35, 1.0, 'sine', 0.6, curve=1.2))
    s['sfx_pickup'] = _tones([660, 880, 1320], 0.05, 'square', 0.4, duty=0.25)
    s['sfx_power_max'] = _tones([523, 659, 784, 1047, 784, 1047], 0.06, 'square', 0.4, duty=0.5)
    s['sfx_player_death'] = _mix(_sweep(900, 60, 0.9, 'square', 0.35, curve=0.9, duty=0.5),
                                 _noise(0.9, 0.6, seed=19, curve=1.1, lowpass_start=0.7, lowpass_end=0.03, hold=2))
    s['sfx_warning'] = _concat(*([_sweep(880, 880, 0.18, 'square', 0.45, curve=0.2),
                                  _sweep(620, 620, 0.18, 'square', 0.45, curve=0.2)] * 3))
    s['sfx_select'] = _tones([988, 1319], 0.045, 'square', 0.4, duty=0.25)
    s['sfx_pause'] = _tones([784, 0, 784], 0.05, 'square', 0.35)
    s['sfx_shield_hit'] = _mix(_sweep(1600, 1200, 0.25, 'tri', 0.6, curve=1.5),
                               _sweep(2400, 1800, 0.25, 'sine', 0.3, curve=2.0))
    return {k: wav_bytes(v) for k, v in s.items()}
