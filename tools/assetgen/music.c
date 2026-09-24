/*
 * Original music for Space Shooter, written as ProTracker 4-channel modules (M.K. format), which
 * Maxmod plays natively. Instruments are tiny single-cycle waveforms plus three synthesized drums.
 *
 * Notation: melodies are strings of "NOTE:ROWS" tokens ("E5:2 G5:2 -:4", '-' = rest); one row is a
 * 16th note. Chords are "ROOT[quality]" per bar (16 rows), quality m (minor), M (major), 7 (dom 7th).
 * Channels: 1 lead, 2 arpeggio, 3 bass, 4 drums.
 */
#include "gen.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.14159265358979323846

static const int PERIODS[36] = {
    856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
    428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
    214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
};

enum { INS_LEAD = 1, INS_PULSE, INS_BASS, INS_ARP, INS_KICK, INS_SNARE, INS_HAT, INS_COUNT = 7 };

static int octave_shift(int ins)
{
    return ins == INS_BASS ? 1 : 2;
}

static int semitone_of(const char* name, size_t len)
{
    static const char* const names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    for(int i = 0; i < 12; ++i)
    {
        if(strlen(names[i]) == len && strncmp(names[i], name, len) == 0)
        {
            return i;
        }
    }

    fprintf(stderr, "assetgen: bad note name '%.*s'\n", (int) len, name);
    exit(1);
}

static int period_for(int semitone, int sci_octave, int ins)
{
    int idx = (sci_octave - octave_shift(ins) - 1) * 12 + semitone;

    while(idx < 0)
    {
        idx += 12;
    }

    while(idx >= 36)
    {
        idx -= 12;
    }

    return PERIODS[idx];
}

/* (semitone, octave) + interval -> new (semitone, octave). */
static void transpose(int semitone, int octave, int interval, int* out_semitone, int* out_octave)
{
    int total = octave * 12 + semitone + interval;
    *out_semitone = total % 12;
    *out_octave = total / 12;
}

/* ----- instruments -------------------------------------------------------------------------------- */

typedef struct
{
    const char* name;
    int8_t* data;
    int length;         /* bytes */
    int volume;
    int loop;
} instrument;

static int8_t to_s8(double v, double amp)
{
    return (int8_t) (ag_round(v * amp) & 0xFF);
}

static void make_instruments(instrument* ins)
{
    static int8_t square[32], pulse[32], tri[64], saw[32], kick[2400], snare[2600], hat[700];

    for(int i = 0; i < 32; ++i)
    {
        double t = (double) i / 32;
        square[i] = to_s8(t < 0.5 ? 0.75 : -0.75, 100);
        pulse[i] = to_s8(t < 0.25 ? 0.7 : -0.7, 100);
        saw[i] = to_s8((2 * t - 1) * 0.8, 100);
    }

    for(int i = 0; i < 64; ++i)
    {
        double t = (double) i / 64;
        tri[i] = to_s8(4 * fabs(t - 0.5) - 1, 120);
    }

    double phase = 0.0;

    for(int i = 0; i < 2400; ++i)
    {
        double f = 45 + 160 * exp(-i / 350.0);
        phase += f / 8363;
        kick[i] = to_s8(sin(2 * PI * phase) * exp(-i / 1200.0), 125);
    }

    lcg rng;
    lcg_init(&rng, 3);
    phase = 0.0;

    for(int i = 0; i < 2600; ++i)
    {
        phase += 190.0 / 8363;
        double noise = (lcg_next(&rng) % 2001) / 1000.0 - 1;
        double v = (noise * 0.7 + sin(2 * PI * phase) * 0.4 * exp(-i / 500.0)) * exp(-i / 700.0);
        snare[i] = to_s8(v, 110);
    }

    lcg_init(&rng, 9);
    double prev = 0.0;

    for(int i = 0; i < 700; ++i)
    {
        double v = (lcg_next(&rng) % 2001) / 1000.0 - 1;
        hat[i] = to_s8((v - prev) * 0.6 * exp(-i / 160.0), 110);
        prev = v;
    }

    instrument list[INS_COUNT] = {
        { "lead", square, 32, 40, 1 },
        { "pulse", pulse, 32, 34, 1 },
        { "bass", tri, 64, 52, 1 },
        { "arp", saw, 32, 16, 1 },
        { "kick", kick, 2400, 60, 0 },
        { "snare", snare, 2600, 42, 0 },
        { "hat", hat, 700, 22, 0 },
    };
    memcpy(ins, list, sizeof(list));
}

/* ----- patterns ----------------------------------------------------------------------------------- */

typedef struct
{
    uint8_t ins;
    uint16_t period;
    uint8_t effect;
    uint8_t param;
} cell;

typedef struct
{
    cell rows[64][4];
} pattern;

#define MAX_PATTERNS 4
#define MAX_ORDER 8

typedef struct
{
    const char* title;
    int speed;
    int bpm;
    pattern patterns[MAX_PATTERNS];
    int pattern_count;
    int order[MAX_ORDER];
    int order_count;
} song;

static void write_melody(pattern* p, int ch, const char* text, int ins)
{
    int r = 0;
    const char* at = text;

    while(*at)
    {
        while(*at == ' ')
        {
            ++at;
        }

        if(! *at)
        {
            break;
        }

        const char* colon = strchr(at, ':');
        int length = atoi(colon + 1);

        if(r >= 64)
        {
            break;
        }

        if(*at == '-')
        {
            p->rows[r][ch] = (cell) { 0, 0, 0xC, 0 };
        }
        else
        {
            size_t name_len = (size_t) (colon - at);
            int semitone = semitone_of(at, name_len - 1);
            int octave = at[name_len - 1] - '0';
            p->rows[r][ch] = (cell) { (uint8_t) ins, (uint16_t) period_for(semitone, octave, ins), 0, 0 };
        }

        r += length;
        at = colon + 1;

        while(*at && *at != ' ')
        {
            ++at;
        }
    }

    if(r > 64)
    {
        fprintf(stderr, "assetgen: melody too long: %d rows\n", r);
        exit(1);
    }
}

static void write_chords(pattern* p, const char* const* chords, const char* bass_style, int arp)
{
    static const struct
    {
        const char* name;
        const char* rhythm;
    } styles[] = {
        { "eighths", "R.R.R.R.R.R.R.O." },
        { "drive", "RRO.RRO.RRO.RROR" },
        { "slow", "R.......O......." },
        { "pulse", "R...R...R...O.R." },
        { "walk", "R...F...O...F..." },
    };
    const char* rhythm = NULL;

    for(size_t i = 0; i < sizeof(styles) / sizeof(styles[0]); ++i)
    {
        if(strcmp(styles[i].name, bass_style) == 0)
        {
            rhythm = styles[i].rhythm;
        }
    }

    for(int bar = 0; bar < 4; ++bar)
    {
        const char* chord = chords[bar];
        size_t len = strlen(chord);
        char quality = chord[len - 1];
        int intervals[4] = { 0, 4, 7, 12 };

        if(quality == 'm')
        {
            intervals[1] = 3;
        }
        else if(quality == '7')
        {
            intervals[3] = 10;
        }

        int has_quality = quality == 'm' || quality == 'M' || quality == '7';
        int rs = semitone_of(chord, has_quality ? len - 1 : len);

        for(int i = 0; i < 16; ++i)
        {
            int r = bar * 16 + i;
            int s, o;

            if(arp)
            {
                transpose(rs, 4, intervals[i % 8 < 6 ? i % 3 : 3], &s, &o);
                p->rows[r][1] = (cell) { INS_ARP, (uint16_t) period_for(s, o, INS_ARP), 0, 0 };
            }

            char step = rhythm[i];

            if(step == '.')
            {
                continue;
            }

            if(step == 'R')
            {
                s = rs;
                o = 2;
            }
            else if(step == 'O')
            {
                s = rs;
                o = 3;
            }
            else
            {
                transpose(rs, 2, 7, &s, &o);    /* F: fifth */
            }

            p->rows[r][2] = (cell) { INS_BASS, (uint16_t) period_for(s, o, INS_BASS), 0, 0 };
        }
    }
}

static void write_drums(pattern* p, const char* text)
{
    char compact[64];
    int n = 0;

    for(const char* at = text; *at && n < 63; ++at)
    {
        if(*at != ' ')
        {
            compact[n++] = *at;
        }
    }

    if(! n)
    {
        return;
    }

    for(int r = 0; r < 64; ++r)
    {
        char ch = compact[r % n];
        int ins = ch == 'K' ? INS_KICK : ch == 'S' ? INS_SNARE : ch == 'H' ? INS_HAT : 0;

        if(ins)
        {
            p->rows[r][3] = (cell) { (uint8_t) ins, 428, 0, 0 };
        }
    }
}

static int add_pattern(song* s, const char* lead, const char* const* chords, const char* bass_style,
                       const char* drums, int arp, int lead_ins)
{
    pattern* p = &s->patterns[s->pattern_count];
    memset(p, 0, sizeof(*p));
    write_melody(p, 0, lead, lead_ins);
    write_chords(p, chords, bass_style, arp);
    write_drums(p, drums);
    p->rows[0][2].effect = 0xF;
    p->rows[0][2].param = (uint8_t) s->speed;
    p->rows[0][3].effect = 0xF;
    p->rows[0][3].param = (uint8_t) s->bpm;
    return s->pattern_count++;
}

/* ----- MOD serialisation -------------------------------------------------------------------------- */

static void put_be16(bytes* out, int v)
{
    bytes_put(out, (uint8_t) (v >> 8));
    bytes_put(out, (uint8_t) v);
}

static void mod_bytes(const song* s, bytes* out)
{
    instrument ins[INS_COUNT];
    make_instruments(ins);
    char title[20] = { 0 };
    memcpy(title, s->title, strlen(s->title) < 20 ? strlen(s->title) : 20);
    bytes_append(out, title, 20);

    for(int i = 0; i < 31; ++i)
    {
        char name[22] = { 0 };

        if(i < INS_COUNT)
        {
            int length = ins[i].length / 2;
            memcpy(name, ins[i].name, strlen(ins[i].name));
            bytes_append(out, name, 22);
            put_be16(out, length);
            bytes_put(out, 0);
            bytes_put(out, (uint8_t) ins[i].volume);
            put_be16(out, 0);
            put_be16(out, ins[i].loop ? length : 1);
        }
        else
        {
            bytes_append(out, name, 22);
            put_be16(out, 0);
            bytes_put(out, 0);
            bytes_put(out, 0);
            put_be16(out, 0);
            put_be16(out, 1);
        }
    }

    bytes_put(out, (uint8_t) s->order_count);
    bytes_put(out, 127);

    for(int i = 0; i < 128; ++i)
    {
        bytes_put(out, (uint8_t) (i < s->order_count ? s->order[i] : 0));
    }

    bytes_append(out, "M.K.", 4);

    for(int p = 0; p < s->pattern_count; ++p)
    {
        for(int r = 0; r < 64; ++r)
        {
            for(int ch = 0; ch < 4; ++ch)
            {
                const cell* c = &s->patterns[p].rows[r][ch];
                bytes_put(out, (uint8_t) ((c->ins & 0xF0) | ((c->period >> 8) & 0x0F)));
                bytes_put(out, (uint8_t) (c->period & 0xFF));
                bytes_put(out, (uint8_t) (((c->ins & 0x0F) << 4) | c->effect));
                bytes_put(out, c->param);
            }
        }
    }

    for(int i = 0; i < INS_COUNT; ++i)
    {
        for(int k = 0; k < ins[i].length; ++k)
        {
            /* one-shot samples start with two silent bytes (ProTracker convention) */
            bytes_put(out, (uint8_t) ((! ins[i].loop && k < 2) ? 0 : ins[i].data[k]));
        }
    }
}

/* ----- the songs ---------------------------------------------------------------------------------- */

#define DR_BASIC "K...H...S...H... K...H...S...H.H."
#define DR_DRIVE "K.H.S.H.K.K.S.H. K.H.S.H.K.H.S.HH"
#define DR_HALF "K.......S....... K.......S...H..."
#define DR_BOSS "K.HKS.HKK.HKS.HS"

static void song_init(song* s, const char* title, int speed, int bpm)
{
    memset(s, 0, sizeof(*s));
    s->title = title;
    s->speed = speed;
    s->bpm = bpm;
}

static void set_order(song* s, const int* order, int count)
{
    memcpy(s->order, order, sizeof(int) * (size_t) count);
    s->order_count = count;
}

static void song_title(song* s)
{
    static const char* const ch[4] = { "Am", "FM", "CM", "GM" };
    song_init(s, "SS title", 6, 120);
    int a = add_pattern(s, "A4:4 C5:2 E5:2 D5:4 C5:2 B4:2 A4:6 F4:2 A4:4 C5:4 "
                           "G4:4 E4:2 G4:2 C5:4 B4:2 C5:2 D5:8 B4:4 G4:4", ch, "eighths", DR_BASIC, 1, INS_LEAD);
    int b = add_pattern(s, "E5:4 D5:2 C5:2 B4:4 C5:2 D5:2 C5:6 A4:2 F4:4 A4:4 "
                           "G4:4 C5:4 E5:4 G5:4 F5:4 E5:4 D5:4 B4:4", ch, "eighths", DR_BASIC, 1, INS_LEAD);
    int intro = add_pattern(s, "", ch, "slow", DR_HALF, 1, INS_LEAD);
    int order[] = { intro, a, b, a, b };
    set_order(s, order, 5);
}

static void song_stage1(song* s)
{
    static const char* const ch1[4] = { "Em", "CM", "DM", "BM" };
    static const char* const ch2[4] = { "Em", "CM", "GM", "DM" };
    song_init(s, "SS stage 1", 5, 140);
    int a = add_pattern(s, "E5:2 -:2 E5:2 G5:2 F#5:2 E5:2 D5:2 B4:2 C5:4 E5:4 G5:4 E5:4 "
                           "D5:2 -:2 D5:2 F#5:2 A5:4 F#5:4 B4:8 D#5:4 F#5:4", ch1, "drive", DR_DRIVE, 1, INS_LEAD);
    int b = add_pattern(s, "G5:4 F#5:2 E5:2 B4:4 E5:4 C5:2 D5:2 E5:4 G5:4 A5:4 "
                           "G5:4 F#5:4 E5:4 D5:4 F#5:8 D5:4 A4:4", ch2, "drive", DR_DRIVE, 1, INS_LEAD);
    int c = add_pattern(s, "", ch1, "pulse", DR_BASIC, 1, INS_LEAD);
    int order[] = { c, a, b, a, b, c };
    set_order(s, order, 6);
}

static void song_stage2(song* s)
{
    static const char* const ch[4] = { "Dm", "A#M", "Gm", "AM" };
    song_init(s, "SS stage 2", 6, 110);
    int a = add_pattern(s, "D5:6 E5:2 F5:4 A5:4 G5:8 F5:4 D5:4 E5:6 D5:2 A#4:4 G4:4 A4:8 C#5:8", ch, "walk",
                        DR_HALF, 1, INS_PULSE);
    int b = add_pattern(s, "A5:4 G5:2 F5:2 E5:4 F5:4 D5:8 A#4:8 G4:4 A#4:4 D5:4 G5:4 E5:8 C#5:4 A4:4", ch, "walk",
                        DR_BASIC, 1, INS_PULSE);
    int c = add_pattern(s, "", ch, "slow", DR_HALF, 1, INS_LEAD);
    int order[] = { c, a, b, a, b };
    set_order(s, order, 5);
}

static void song_stage3(song* s)
{
    static const char* const ch[4] = { "Cm", "G#M", "A#M", "GM" };
    song_init(s, "SS stage 3", 5, 150);
    int a = add_pattern(s, "C5:2 C5:2 D#5:2 G5:2 -:2 G5:2 F5:2 D#5:2 G#4:4 C5:4 D#5:4 G#5:4 "
                           "A#4:2 A#4:2 D5:2 F5:2 A#5:4 F5:4 G5:8 D5:4 B4:4", ch, "drive", DR_DRIVE, 1, INS_LEAD);
    int b = add_pattern(s, "D#5:4 D5:4 C5:4 G4:4 G#4:4 A#4:4 C5:8 D5:4 D#5:4 F5:4 A#5:4 B4:4 D5:4 G5:8", ch, "drive",
                        DR_DRIVE, 1, INS_LEAD);
    int c = add_pattern(s, "", ch, "drive", DR_BASIC, 1, INS_LEAD);
    int order[] = { c, a, b, a, b };
    set_order(s, order, 5);
}

static void song_boss(song* s)
{
    static const char* const ch[4] = { "F#m", "F#m", "DM", "EM" };
    song_init(s, "SS boss", 4, 150);
    int a = add_pattern(s, "F#5:2 -:2 F#5:2 -:2 A5:2 G#5:2 F#5:2 C#5:2 D5:4 C#5:4 A4:4 C#5:4 "
                           "F#5:2 -:2 F#5:2 -:2 A5:2 B5:2 A5:2 G#5:2 E5:8 G#5:8", ch, "drive", DR_BOSS, 1, INS_LEAD);
    int b = add_pattern(s, "C#5:4 D5:4 E5:4 F#5:4 D5:8 A4:8 E5:4 F#5:4 G#5:4 A5:4 B5:8 G#5:8", ch, "drive", DR_BOSS, 1,
                        INS_LEAD);
    int order[] = { a, b };
    set_order(s, order, 2);
}

static void song_ending(song* s)
{
    static const char* const ch1[4] = { "CM", "GM", "Am", "FM" };
    static const char* const ch2[4] = { "FM", "GM", "CM", "CM" };
    song_init(s, "SS ending", 6, 104);
    int a = add_pattern(s, "E5:4 G5:4 C5:4 E5:4 D5:6 B4:2 G4:8 A4:4 C5:4 E5:4 A5:4 F5:8 E5:4 D5:4", ch1, "walk",
                        DR_BASIC, 1, INS_LEAD);
    int b = add_pattern(s, "C5:4 A4:4 F4:4 A4:4 B4:4 D5:4 G5:8 E5:4 G5:4 C5:4 E5:4 C5:16", ch2, "walk", DR_HALF, 1,
                        INS_LEAD);
    int order[] = { a, b };
    set_order(s, order, 2);
}

static void jingle_clear(song* s)
{
    static const char* const ch[4] = { "CM", "FM", "CM", "CM" };
    song_init(s, "SS clear", 5, 140);
    int p = add_pattern(s, "G4:2 C5:2 E5:2 G5:6 -:2 F5:2 A5:2 G5:4 E5:2 C5:2 E5:4 G5:4 C5:12 -:4", ch, "slow",
                        "K...K...K...S...", 1, INS_LEAD);
    int order[] = { p };
    set_order(s, order, 1);
}

static void jingle_gameover(song* s)
{
    static const char* const ch[4] = { "Am", "Dm", "Am", "Am" };
    song_init(s, "SS game over", 7, 100);
    int p = add_pattern(s, "A4:4 G4:4 F4:4 E4:4 D4:4 C4:4 B3:4 E4:4 A3:16 -:16", ch, "slow", "", 0, INS_LEAD);
    int order[] = { p };
    set_order(s, order, 1);
}

void music_build_all(named_blob_fn emit)
{
    static const struct
    {
        const char* name;
        void (*make)(song*);
    } songs[] = {
        { "jingle_clear", jingle_clear },
        { "jingle_gameover", jingle_gameover },
        { "music_boss", song_boss },
        { "music_ending", song_ending },
        { "music_stage1", song_stage1 },
        { "music_stage2", song_stage2 },
        { "music_stage3", song_stage3 },
        { "music_title", song_title },
    };
    static song s;

    for(size_t i = 0; i < sizeof(songs) / sizeof(songs[0]); ++i)
    {
        bytes out = { 0 };
        songs[i].make(&s);
        mod_bytes(&s, &out);
        emit(songs[i].name, "mod", &out);
        free(out.data);
    }
}
