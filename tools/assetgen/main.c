/*
 * Space Shooter asset generator (host program).
 *
 *     assetgen OUT_DIR [--dump DIR]
 *
 * Produces every graphic as GBA-ready C data (OUT_DIR/gen_gfx.c + gen_gfx.h: 4bpp tiles in 1D
 * sprite order, deduplicated background tiles + maps, BGR555 palettes, font glyph rows) and every
 * audio file (OUT_DIR/audio/ *.mod music, *.wav effects) for Maxmod's mmutil. Files are only
 * rewritten when their bytes change, so running it before every build keeps builds incremental.
 * --dump writes each image as a binary PGM of palette indices (for checking the art).
 */
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "art.h"
#include "gen.h"

#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(path) _mkdir(path)
#else
    #define MKDIR(path) mkdir(path, 0755)
#endif

static const char* out_dir;
static const char* dump_dir;
static int files_total;
static int files_changed;

/* ----- byte buffers ------------------------------------------------------------------------------- */

void bytes_put(bytes* b, uint8_t v)
{
    bytes_append(b, &v, 1);
}

void bytes_append(bytes* b, const void* data, size_t size)
{
    if(b->size + size > b->capacity)
    {
        size_t capacity = b->capacity ? b->capacity * 2 : 4096;

        while(capacity < b->size + size)
        {
            capacity *= 2;
        }

        b->data = (uint8_t*) realloc(b->data, capacity);
        b->capacity = capacity;
    }

    memcpy(b->data + b->size, data, size);
    b->size += size;
}

static void bytes_printf(bytes* b, const char* fmt, ...)
{
    char small[1024];
    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(small, sizeof(small), fmt, args);
    va_end(args);

    if(n < (int) sizeof(small))
    {
        bytes_append(b, small, (size_t) n);
        return;
    }

    char* big = (char*) malloc((size_t) n + 1);
    va_start(args, fmt);
    vsnprintf(big, (size_t) n + 1, fmt, args);
    va_end(args);
    bytes_append(b, big, (size_t) n);
    free(big);
}

/* ----- files -------------------------------------------------------------------------------------- */

static void write_if_changed(const char* path, const bytes* data)
{
    ++files_total;
    FILE* f = fopen(path, "rb");

    if(f)
    {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);

        if(size == (long) data->size)
        {
            fseek(f, 0, SEEK_SET);
            uint8_t* old = (uint8_t*) malloc(data->size ? data->size : 1);
            size_t read = fread(old, 1, data->size, f);
            int same = read == data->size && memcmp(old, data->data, data->size) == 0;
            free(old);

            if(same)
            {
                fclose(f);
                return;
            }
        }

        fclose(f);
    }

    f = fopen(path, "wb");

    if(! f)
    {
        fprintf(stderr, "assetgen: cannot write %s\n", path);
        exit(1);
    }

    fwrite(data->data, 1, data->size, f);
    fclose(f);
    ++files_changed;
}

static void emit_audio(const char* name, const char* ext, const bytes* data)
{
    char path[1024];
    snprintf(path, sizeof(path), "%s/audio/%s.%s", out_dir, name, ext);
    write_if_changed(path, data);
}

static void dump_canvas(const char* name, const canvas* c)
{
    if(! dump_dir)
    {
        return;
    }

    char path[1024];
    snprintf(path, sizeof(path), "%s/%s.pgm", dump_dir, name);
    bytes b = { 0 };
    bytes_printf(&b, "P5\n%d %d\n255\n", c->w, c->h);
    bytes_append(&b, c->px, (size_t) (c->w * c->h));
    write_if_changed(path, &b);
    free(b.data);
}

/* ----- GBA formats -------------------------------------------------------------------------------- */

static unsigned short bgr555(rgb c)
{
    return (unsigned short) ((c.r >> 3) | ((c.g >> 3) << 5) | ((c.b >> 3) << 10));
}

/* One 4bpp tile (8 words, one per row; the left pixel is the low nibble). */
static void tile_words(const canvas* c, int x0, int y0, uint32_t* out)
{
    for(int y = 0; y < 8; ++y)
    {
        uint32_t row = 0;

        for(int x = 0; x < 8; ++x)
        {
            row |= (uint32_t) (canvas_get(c, x0 + x, y0 + y) & 0xF) << (4 * x);
        }

        out[y] = row;
    }
}

static void tile_flip(const uint32_t* in, int hflip, int vflip, uint32_t* out)
{
    for(int y = 0; y < 8; ++y)
    {
        uint32_t row = in[vflip ? 7 - y : y];

        if(hflip)
        {
            uint32_t r = 0;

            for(int x = 0; x < 8; ++x)
            {
                r |= ((row >> (4 * x)) & 0xF) << (4 * (7 - x));
            }

            row = r;
        }

        out[y] = row;
    }
}

static void emit_words(bytes* src, const char* decl, const uint32_t* words, int count)
{
    bytes_printf(src, "%s =\n{", decl);

    for(int i = 0; i < count; ++i)
    {
        bytes_printf(src, "%s0x%08X,", i % 8 ? " " : "\n    ", (unsigned) words[i]);
    }

    bytes_printf(src, "\n};\n\n");
}

static void emit_halfwords(bytes* src, const char* decl, const unsigned short* values, int count)
{
    bytes_printf(src, "%s =\n{", decl);

    for(int i = 0; i < count; ++i)
    {
        bytes_printf(src, "%s0x%04X,", i % 16 ? " " : "\n    ", values[i]);
    }

    bytes_printf(src, "\n};\n\n");
}

/* 2-D arrays: rows groups of cols values, each in its own braces. */
static void emit_words_2d(bytes* src, const char* decl, const uint32_t* words, int rows, int cols)
{
    bytes_printf(src, "%s =\n{\n", decl);

    for(int r = 0; r < rows; ++r)
    {
        bytes_printf(src, "    {");

        for(int i = 0; i < cols; ++i)
        {
            bytes_printf(src, "%s0x%08X,", i % 8 ? " " : "\n        ", (unsigned) words[r * cols + i]);
        }

        bytes_printf(src, "\n    },\n");
    }

    bytes_printf(src, "};\n\n");
}

static void emit_halfwords_2d(bytes* src, const char* decl, const unsigned short* values, int rows, int cols)
{
    bytes_printf(src, "%s =\n{\n", decl);

    for(int r = 0; r < rows; ++r)
    {
        bytes_printf(src, "    {");

        for(int i = 0; i < cols; ++i)
        {
            bytes_printf(src, "%s0x%04X,", i % 16 ? " " : "\n        ", values[r * cols + i]);
        }

        bytes_printf(src, "\n    },\n");
    }

    bytes_printf(src, "};\n\n");
}

static void palette16(const rgb* colors, int count, unsigned short* out)
{
    for(int i = 0; i < 16; ++i)
    {
        out[i] = i < count ? bgr555(colors[i]) : 0;
    }
}

static void upper(const char* in, char* out)
{
    while(*in)
    {
        *out++ = (char) toupper((unsigned char) *in++);
    }

    *out = 0;
}

/* GBA OBJ shape (0 square, 1 wide, 2 tall) and size (0..3) for a sprite of w x h pixels. */
static int obj_shape_size(int w, int h, int* shape, int* size)
{
    static const int dims[3][4][2] = {
        { { 8, 8 }, { 16, 16 }, { 32, 32 }, { 64, 64 } },
        { { 16, 8 }, { 32, 8 }, { 32, 16 }, { 64, 32 } },
        { { 8, 16 }, { 8, 32 }, { 16, 32 }, { 32, 64 } },
    };

    for(int s = 0; s < 3; ++s)
    {
        for(int z = 0; z < 4; ++z)
        {
            if(dims[s][z][0] == w && dims[s][z][1] == h)
            {
                *shape = s;
                *size = z;
                return 1;
            }
        }
    }

    return 0;
}

/* ----- generators --------------------------------------------------------------------------------- */

static void gen_sprites(bytes* src, bytes* hdr)
{
    bytes_printf(hdr, "enum\n{\n");

    for(int i = 0; i < sprite_def_count; ++i)
    {
        char name[64];
        upper(sprite_defs[i].name, name);
        bytes_printf(hdr, "    GEN_SPR_%s,\n", name);
    }

    bytes_printf(hdr, "    GEN_SPRITE_COUNT\n};\n\n");

    int total_tiles = 0;
    char table[16384] = "";

    for(int i = 0; i < sprite_def_count; ++i)
    {
        const sprite_def* def = &sprite_defs[i];
        int frame_h = 0;
        canvas* c = def->make(&frame_h);
        dump_canvas(def->name, c);
        int frames = c->h / frame_h;
        int shape, size;

        if(! obj_shape_size(c->w, frame_h, &shape, &size))
        {
            fprintf(stderr, "assetgen: %s: %dx%d is not a GBA sprite size\n", def->name, c->w, frame_h);
            exit(1);
        }

        int tiles_per_frame = (c->w / 8) * (frame_h / 8);
        int words = tiles_per_frame * frames * 8;
        uint32_t* data = (uint32_t*) malloc(sizeof(uint32_t) * (size_t) words);
        int w = 0;

        for(int f = 0; f < frames; ++f)
        {
            for(int ty = 0; ty < frame_h / 8; ++ty)
            {
                for(int tx = 0; tx < c->w / 8; ++tx)
                {
                    tile_words(c, tx * 8, f * frame_h + ty * 8, data + w);
                    w += 8;
                }
            }
        }

        char decl[128];
        snprintf(decl, sizeof(decl), "static const unsigned int spr_%s_tiles[%d]", def->name, words);
        emit_words(src, decl, data, words);
        free(data);
        total_tiles += tiles_per_frame * frames;

        char line[256];
        snprintf(line, sizeof(line), "    { spr_%s_tiles, %d, %d, %d, %d, %d, %d, %d },\n", def->name,
                 tiles_per_frame, c->w, frame_h, frames, shape, size, def->boss_palette);
        strcat(table, line);
        canvas_free(c);
    }

    bytes_printf(src, "const gen_sprite gen_sprites[GEN_SPRITE_COUNT] =\n{\n%s};\n\n", table);
    printf("  sprites: %d sheets, %d tiles\n", sprite_def_count, total_tiles);
}

static void gen_backgrounds(bytes* src, bytes* hdr)
{
    bytes_printf(hdr, "enum\n{\n");

    for(int i = 0; i < bg_def_count; ++i)
    {
        char name[64];
        upper(bg_defs[i].name, name);
        bytes_printf(hdr, "    GEN_BG_%s,\n", name);
    }

    bytes_printf(hdr, "    GEN_BG_COUNT\n};\n\n");
    char table[4096] = "";

    for(int i = 0; i < bg_def_count; ++i)
    {
        const bg_def* def = &bg_defs[i];
        canvas* c = def->make();
        dump_canvas(def->name, c);

        /* Deduplicate 8x8 tiles, reusing flipped copies (map entry bits 10/11). */
        static uint32_t unique[1024][8];
        static unsigned short map[32 * 32];
        int count = 0;

        for(int ty = 0; ty < 32; ++ty)
        {
            for(int tx = 0; tx < 32; ++tx)
            {
                uint32_t t[8];
                tile_words(c, tx * 8, ty * 8, t);
                int found = -1;
                int flags = 0;

                for(int u = 0; u < count && found < 0; ++u)
                {
                    for(int f = 0; f < 4; ++f)
                    {
                        uint32_t flipped[8];
                        tile_flip(unique[u], f & 1, f >> 1, flipped);

                        if(memcmp(flipped, t, sizeof(t)) == 0)
                        {
                            found = u;
                            flags = f;
                            break;
                        }
                    }
                }

                if(found < 0)
                {
                    if(count >= 1024)
                    {
                        fprintf(stderr, "assetgen: %s: too many tiles\n", def->name);
                        exit(1);
                    }

                    memcpy(unique[count], t, sizeof(t));
                    found = count++;
                }

                map[ty * 32 + tx] = (unsigned short) (found | (flags << 10));
            }
        }

        char decl[128];
        snprintf(decl, sizeof(decl), "static const unsigned int bg_%s_tiles[%d]", def->name, count * 8);
        emit_words(src, decl, &unique[0][0], count * 8);
        snprintf(decl, sizeof(decl), "static const unsigned short bg_%s_map[1024]", def->name);
        emit_halfwords(src, decl, map, 1024);
        unsigned short pal[16];
        palette16(def->pal->colors, def->pal->count, pal);
        snprintf(decl, sizeof(decl), "static const unsigned short bg_%s_palette[16]", def->name);
        emit_halfwords(src, decl, pal, 16);

        char line[256];
        snprintf(line, sizeof(line), "    { bg_%s_tiles, bg_%s_map, bg_%s_palette, %d },\n", def->name, def->name,
                 def->name, count);
        strcat(table, line);
        printf("  bg %-12s %d unique tiles\n", def->name, count);
        canvas_free(c);
    }

    bytes_printf(src, "const gen_bg gen_bgs[GEN_BG_COUNT] =\n{\n%s};\n\n", table);
}

static void gen_palettes_and_font(bytes* src)
{
    unsigned short pal[16];
    palette16(pal_master.colors, pal_master.count, pal);
    emit_halfwords(src, "const unsigned short gen_pal_master[16]", pal, 16);
    palette16(pal_boss.colors, pal_boss.count, pal);
    emit_halfwords(src, "const unsigned short gen_pal_boss[16]", pal, 16);

    /* Hit-flash palettes: every opaque colour white. */
    for(int i = 0; i < 16; ++i)
    {
        pal[i] = i == 0 ? bgr555(pal_master.colors[0]) : bgr555((rgb) { 248, 248, 248 });
    }

    emit_halfwords(src, "const unsigned short gen_pal_flash[16]", pal, 16);

    unsigned short fonts[4][16];

    for(int k = 0; k < 4; ++k)
    {
        palette16(pal_font[k], 4, fonts[k]);
    }

    emit_halfwords_2d(src, "const unsigned short gen_pal_font[4][16]", &fonts[0][0], 4, 16);

    uint32_t glyphs[FONT_GLYPHS][8];
    canvas* frames[FONT_GLYPHS];

    for(int i = 0; i < FONT_GLYPHS; ++i)
    {
        canvas* g = font_glyph((char) (FONT_FIRST + i));
        tile_words(g, 0, 0, glyphs[i]);
        frames[i] = g;
    }

    canvas* sheet = canvas_vstack(frames, FONT_GLYPHS);
    dump_canvas("font", sheet);
    canvas_free(sheet);
    emit_words_2d(src, "const unsigned int gen_font[94][8]", &glyphs[0][0], FONT_GLYPHS, 8);

    uint32_t terrain[2][7 * 8];
    unsigned short terrain_pal[2][16];

    for(int style = 0; style < 2; ++style)
    {
        canvas* strip = terrain_tiles(style);
        dump_canvas(style ? "terrain_metal" : "terrain_crystal", strip);

        for(int t = 0; t < 7; ++t)
        {
            tile_words(strip, 0, t * 8, &terrain[style][t * 8]);
        }

        canvas_free(strip);
        const palette* p = style ? &pal_terrain_metal : &pal_terrain_crystal;
        palette16(p->colors, p->count, terrain_pal[style]);
    }

    emit_words_2d(src, "const unsigned int gen_terrain_tiles[2][56]", &terrain[0][0], 2, 56);
    emit_halfwords_2d(src, "const unsigned short gen_pal_terrain[2][16]", &terrain_pal[0][0], 2, 16);
}

int main(int argc, char** argv)
{
    if(argc < 2)
    {
        fprintf(stderr, "usage: assetgen OUT_DIR [--dump DIR]\n");
        return 1;
    }

    out_dir = argv[1];

    for(int i = 2; i + 1 < argc; ++i)
    {
        if(strcmp(argv[i], "--dump") == 0)
        {
            dump_dir = argv[++i];
            MKDIR(dump_dir);
        }
    }

    char path[1024];
    MKDIR(out_dir);
    snprintf(path, sizeof(path), "%s/audio", out_dir);
    MKDIR(path);

    bytes src = { 0 };
    bytes hdr = { 0 };
    bytes_printf(&hdr,
        "/* Generated by tools/assetgen - do not edit. */\n"
        "#ifndef GEN_GFX_H\n#define GEN_GFX_H\n\n"
        "typedef struct\n{\n"
        "    const unsigned int* tiles;      /* 4bpp, frames one after another, 1D sprite order */\n"
        "    unsigned short tiles_per_frame;\n"
        "    unsigned char width, height, frames;\n"
        "    unsigned char shape, size;      /* OBJ attribute shape (0..2) and size (0..3) */\n"
        "    unsigned char boss_palette;     /* 0: master palette, 1: boss palette */\n"
        "} gen_sprite;\n\n"
        "typedef struct\n{\n"
        "    const unsigned int* tiles;      /* deduplicated 4bpp tiles */\n"
        "    const unsigned short* map;      /* 32x32 entries: tile index | flip bits (no palette bank) */\n"
        "    const unsigned short* palette;  /* 16 colours */\n"
        "    unsigned short tile_count;\n"
        "} gen_bg;\n\n");
    bytes_printf(&src, "/* Generated by tools/assetgen - do not edit. */\n#include \"gen_gfx.h\"\n\n");

    gen_sprites(&src, &hdr);
    gen_backgrounds(&src, &hdr);
    gen_palettes_and_font(&src);

    bytes_printf(&hdr,
        "extern const gen_sprite gen_sprites[GEN_SPRITE_COUNT];\n"
        "extern const gen_bg gen_bgs[GEN_BG_COUNT];\n"
        "extern const unsigned short gen_pal_master[16];\n"
        "extern const unsigned short gen_pal_boss[16];\n"
        "extern const unsigned short gen_pal_flash[16];      /* all-white hit flash */\n"
        "extern const unsigned short gen_pal_font[4][16];    /* white, yellow, cyan, red */\n"
        "extern const unsigned int gen_font[94][8];          /* glyphs '!'..'~', 4bpp rows: 1 shadow, 2-3 face */\n"
        "extern const unsigned int gen_terrain_tiles[2][56]; /* crystal, metal: 7 tiles each */\n"
        "extern const unsigned short gen_pal_terrain[2][16];\n\n"
        "#define GEN_FONT_FIRST '!'\n#define GEN_FONT_PITCH %d\n\n#endif\n", FONT_PITCH);

    snprintf(path, sizeof(path), "%s/gen_gfx.c", out_dir);
    write_if_changed(path, &src);
    snprintf(path, sizeof(path), "%s/gen_gfx.h", out_dir);
    write_if_changed(path, &hdr);
    free(src.data);
    free(hdr.data);

    music_build_all(emit_audio);
    sfx_build_all(emit_audio);
    printf("assetgen: %d files, %d updated\n", files_total, files_changed);
    return 0;
}
