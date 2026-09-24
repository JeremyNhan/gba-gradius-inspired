/*
 * Sound-effect synthesiser: 8-bit unsigned mono PCM WAV files at 22050 Hz (the rate Maxmod
 * examples use). All noise comes from a seeded LCG, so the output is deterministic.
 */
#include "gen.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.14159265358979323846
#define RATE 22050

typedef struct
{
    double* s;
    int n;
} track;

static track track_new(int n)
{
    track t;
    t.n = n;
    t.s = (double*) calloc((size_t) (n > 0 ? n : 1), sizeof(double));
    return t;
}

static double envelope(int i, int n, double curve)
{
    const double attack = 0.005;
    double t = (double) i / RATE;
    double a = t / attack < 1.0 ? t / attack : 1.0;
    double d = pow(1.0 - (double) i / n, curve);
    return a * d;
}

enum wave { SQUARE, SAW, TRI, SINE };

static track sweep(double f0, double f1, double dur, enum wave wave, double vol, double curve, double duty)
{
    int n = (int) (dur * RATE);
    track out = track_new(n);
    double phase = 0.0;

    for(int i = 0; i < n; ++i)
    {
        double t = (double) i / n;
        double f = f0 * pow(f1 / f0, t);
        phase += f / RATE;
        double frac = fmod(phase, 1.0);
        double v;

        switch(wave)
        {
        case SQUARE:
            v = frac < duty ? 1.0 : -1.0;
            break;

        case SAW:
            v = 2.0 * frac - 1.0;
            break;

        case TRI:
            v = 4.0 * fabs(frac - 0.5) - 1.0;
            break;

        default:
            v = sin(2 * PI * phase);
            break;
        }

        out.s[i] = v * vol * envelope(i, n, curve);
    }

    return out;
}

static track noise(double dur, double vol, uint32_t seed, double curve, double lowpass_start, double lowpass_end,
                   int hold)
{
    int n = (int) (dur * RATE);
    track out = track_new(n);
    lcg rng;
    lcg_init(&rng, seed);
    double y = 0.0;
    double cur = 0.0;

    for(int i = 0; i < n; ++i)
    {
        if(i % hold == 0)
        {
            cur = (lcg_next(&rng) % 2001) / 1000.0 - 1.0;
        }

        double k = lowpass_start + (lowpass_end - lowpass_start) * ((double) i / n);
        y += (cur - y) * k;
        out.s[i] = y * vol * envelope(i, n, curve) * (k < 0.3 ? 1.6 : 1.0);
    }

    return out;
}

/* Sum of two tracks, clipped to [-1, 1]; frees the inputs. */
static track mix(track a, track b)
{
    int n = a.n > b.n ? a.n : b.n;
    track out = track_new(n);

    for(int i = 0; i < n; ++i)
    {
        double v = 0.0;

        if(i < a.n)
        {
            v += a.s[i];
        }

        if(i < b.n)
        {
            v += b.s[i];
        }

        out.s[i] = v < -1.0 ? -1.0 : v > 1.0 ? 1.0 : v;
    }

    free(a.s);
    free(b.s);
    return out;
}

/* a followed by b; frees the inputs. */
static track concat(track a, track b)
{
    track out = track_new(a.n + b.n);
    memcpy(out.s, a.s, sizeof(double) * (size_t) a.n);
    memcpy(out.s + a.n, b.s, sizeof(double) * (size_t) b.n);
    free(a.s);
    free(b.s);
    return out;
}

/* Sequence of fixed-pitch notes; a frequency of 0 is a rest. */
static track tones(const double* freqs, int count, double each, enum wave wave, double vol, double duty)
{
    track out = track_new(0);
    out.n = 0;

    for(int i = 0; i < count; ++i)
    {
        track note = freqs[i] > 0 ? sweep(freqs[i], freqs[i], each, wave, vol, 0.6, duty)
                                  : track_new((int) (each * RATE));
        out = concat(out, note);
    }

    return out;
}

static void wav_bytes(const track* t, bytes* out)
{
    size_t n = (size_t) t->n;
    size_t data_size = n + (n % 2);
    uint32_t v32;
    uint16_t v16;

    bytes_append(out, "RIFF", 4);
    v32 = (uint32_t) (36 + data_size);
    bytes_append(out, &v32, 4);                 /* little-endian host (x86/ARM) */
    bytes_append(out, "WAVEfmt ", 8);
    v32 = 16;
    bytes_append(out, &v32, 4);
    v16 = 1;                                    /* PCM */
    bytes_append(out, &v16, 2);
    v16 = 1;                                    /* mono */
    bytes_append(out, &v16, 2);
    v32 = RATE;
    bytes_append(out, &v32, 4);
    v32 = RATE;                                 /* byte rate */
    bytes_append(out, &v32, 4);
    v16 = 1;                                    /* block align */
    bytes_append(out, &v16, 2);
    v16 = 8;                                    /* bits per sample */
    bytes_append(out, &v16, 2);
    bytes_append(out, "data", 4);
    v32 = (uint32_t) data_size;
    bytes_append(out, &v32, 4);

    for(size_t i = 0; i < n; ++i)
    {
        int s = ag_round(128 + t->s[i] * 127);
        bytes_put(out, (uint8_t) (s < 0 ? 0 : s > 255 ? 255 : s));
    }

    if(n % 2)
    {
        bytes_put(out, 0x80);
    }
}

static void emit_track(named_blob_fn emit, const char* name, track t)
{
    bytes out = { 0 };
    wav_bytes(&t, &out);
    emit(name, "wav", &out);
    free(out.data);
    free(t.s);
}

void sfx_build_all(named_blob_fn emit)
{
    emit_track(emit, "sfx_shot", sweep(1400, 500, 0.06, SQUARE, 0.35, 1.2, 0.25));
    emit_track(emit, "sfx_spread", mix(sweep(1000, 420, 0.07, SQUARE, 0.3, 1.0, 0.5),
                                       noise(0.05, 0.15, 3, 1.5, 1.0, 0.1, 1)));
    emit_track(emit, "sfx_missile", mix(noise(0.14, 0.45, 5, 1.5, 0.6, 0.05, 1),
                                        sweep(300, 900, 0.14, SAW, 0.15, 1.0, 0.5)));
    {
        static const double f[] = { 880, 1320 };
        emit_track(emit, "sfx_charge_ready", tones(f, 2, 0.05, SQUARE, 0.35, 0.25));
    }
    emit_track(emit, "sfx_beam", mix(sweep(1800, 90, 0.38, SAW, 0.5, 0.8, 0.5),
                                     noise(0.38, 0.35, 7, 1.5, 0.9, 0.05, 1)));
    emit_track(emit, "sfx_hit", mix(noise(0.035, 0.5, 11, 1.5, 1.0, 0.6, 1),
                                    sweep(2200, 1400, 0.03, SQUARE, 0.2, 1.0, 0.5)));
    emit_track(emit, "sfx_explode", noise(0.40, 0.95, 13, 1.6, 0.8, 0.04, 2));
    emit_track(emit, "sfx_explode_big", mix(noise(1.0, 1.0, 17, 1.3, 0.5, 0.02, 3),
                                            sweep(120, 35, 1.0, SINE, 0.6, 1.2, 0.5)));
    {
        static const double f[] = { 660, 880, 1320 };
        emit_track(emit, "sfx_pickup", tones(f, 3, 0.05, SQUARE, 0.4, 0.25));
    }
    {
        static const double f[] = { 523, 659, 784, 1047, 784, 1047 };
        emit_track(emit, "sfx_power_max", tones(f, 6, 0.06, SQUARE, 0.4, 0.5));
    }
    emit_track(emit, "sfx_player_death", mix(sweep(900, 60, 0.9, SQUARE, 0.35, 0.9, 0.5),
                                             noise(0.9, 0.6, 19, 1.1, 0.7, 0.03, 2)));
    {
        track siren = track_new(0);
        siren.n = 0;

        for(int i = 0; i < 3; ++i)
        {
            siren = concat(siren, sweep(880, 880, 0.18, SQUARE, 0.45, 0.2, 0.5));
            siren = concat(siren, sweep(620, 620, 0.18, SQUARE, 0.45, 0.2, 0.5));
        }

        emit_track(emit, "sfx_warning", siren);
    }
    {
        static const double f[] = { 988, 1319 };
        emit_track(emit, "sfx_select", tones(f, 2, 0.045, SQUARE, 0.4, 0.25));
    }
    {
        static const double f[] = { 784, 0, 784 };
        emit_track(emit, "sfx_pause", tones(f, 3, 0.05, SQUARE, 0.35, 0.5));
    }
    emit_track(emit, "sfx_shield_hit", mix(sweep(1600, 1200, 0.25, TRI, 0.6, 1.5, 0.5),
                                           sweep(2400, 1800, 0.25, SINE, 0.3, 2.0, 0.5)));
}
