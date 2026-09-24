#include "ss_text.h"

#include "gen_gfx.h"
#include "ss_video.h"

#define ROW_TILES 30

/* 20 rows x 30 tiles x 8 words = 19.2 KB: kept in EWRAM (IWRAM is only 32 KB). */
EWRAM_BSS static u32 tiles[TEXT_ROWS][ROW_TILES][8];
static u8 cell_color[TEXT_ROWS][ROW_TILES];
static u32 dirty_rows;

void text_init(void)
{
    text_clear_all();

    /* Map: row r, column c shows tile r * 30 + c; columns 30-31 (never on screen) use tile 0. */
    for(int r = 0; r < 32; ++r)
    {
        for(int c = 0; c < 32; ++c)
        {
            int tile = (r < TEXT_ROWS && c < ROW_TILES) ? r * ROW_TILES + c : 0;
            se_mem[SBB_TEXT][r * 32 + c] = (u16) (tile | SE_PALBANK(PAL_BG_FONT));
        }
    }

    dirty_rows = (1u << TEXT_ROWS) - 1;
    text_commit();
}

void text_clear_row(int row)
{
    if(row < 0 || row >= TEXT_ROWS)
    {
        return;
    }

    memset32(tiles[row], 0, ROW_TILES * 8);
    dirty_rows |= 1u << row;
}

void text_clear_all(void)
{
    for(int r = 0; r < TEXT_ROWS; ++r)
    {
        text_clear_row(r);
    }
}

int text_width(const char* s)
{
    int n = 0;

    while(s[n])
    {
        ++n;
    }

    return n * TEXT_PITCH;
}

void text_print(int row, int x, const char* s, text_color color)
{
    if(row < 0 || row >= TEXT_ROWS)
    {
        return;
    }

    for(; *s; ++s, x += TEXT_PITCH)
    {
        int ch = (unsigned char) *s;

        if(ch >= 'a' && ch <= 'z')
        {
            ch -= 'a' - 'A';
        }

        if(ch < GEN_FONT_FIRST || ch > '~' || x < 0 || x > SCREEN_W - TEXT_PITCH)
        {
            continue;
        }

        const unsigned int* glyph = gen_font[ch - GEN_FONT_FIRST];
        int cell = x >> 3;
        int shift = (x & 7) * 4;

        for(int y = 0; y < 8; ++y)
        {
            tiles[row][cell][y] |= glyph[y] << shift;

            if(shift && cell + 1 < ROW_TILES)
            {
                tiles[row][cell + 1][y] |= glyph[y] >> (32 - shift);
            }
        }

        cell_color[row][cell] = (u8) color;

        if(cell + 1 < ROW_TILES && (x & 7) > 8 - TEXT_PITCH)
        {
            cell_color[row][cell + 1] = (u8) color;
        }
    }

    dirty_rows |= 1u << row;
}

void text_center(int row, const char* s, text_color color)
{
    text_print(row, (SCREEN_W - text_width(s)) / 2, s, color);
}

void text_commit(void)
{
    if(! dirty_rows)
    {
        return;
    }

    for(int r = 0; r < TEXT_ROWS; ++r)
    {
        if(! (dirty_rows & (1u << r)))
        {
            continue;
        }

        dma3_cpy(&tile_mem[2][r * ROW_TILES], tiles[r], sizeof(tiles[r]));

        for(int c = 0; c < ROW_TILES; ++c)
        {
            se_mem[SBB_TEXT][r * 32 + c] = (u16) ((r * ROW_TILES + c) | SE_PALBANK(PAL_BG_FONT + cell_color[r][c]));
        }
    }

    dirty_rows = 0;
}

char* text_append(char* out, const char* s)
{
    while(*s)
    {
        *out++ = *s++;
    }

    *out = 0;
    return out;
}

char* text_append_number(char* out, int value, int digits)
{
    char buf[12];
    int n = 0;
    unsigned v = value < 0 ? 0u : (unsigned) value;

    do
    {
        buf[n++] = (char) ('0' + v % 10);
        v /= 10;
    }
    while(v);

    while(n < digits && n < (int) sizeof(buf))
    {
        buf[n++] = '0';
    }

    while(n)
    {
        *out++ = buf[--n];
    }

    *out = 0;
    return out;
}
