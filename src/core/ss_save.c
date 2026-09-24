/*
 * High score in cartridge SRAM (32 KB at 0x0E000000, 8-bit bus: byte accesses only).
 * Layout: "SSHS", version, hiscore, checksum (little-endian 32-bit fields).
 */
#include "ss_save.h"

#define SAVE_VERSION 1

/* Emulators and flash carts detect the save type from this string in the ROM. */
__attribute__((used, aligned(4))) const char save_type_tag[] = "SRAM_V113";

static u32 read32(int offset)
{
    u32 v = 0;

    for(int i = 0; i < 4; ++i)
    {
        v |= (u32) sram_mem[offset + i] << (8 * i);
    }

    return v;
}

static void write32(int offset, u32 v)
{
    for(int i = 0; i < 4; ++i)
    {
        sram_mem[offset + i] = (u8) (v >> (8 * i));
    }
}

static u32 checksum_of(u32 hiscore)
{
    return (hiscore ^ 0x5A5A1234u) + SAVE_VERSION * 7919u;
}

int save_load_hiscore(void)
{
    bool magic = sram_mem[0] == 'S' && sram_mem[1] == 'S' && sram_mem[2] == 'H' && sram_mem[3] == 'S';
    u32 version = read32(4);
    u32 hiscore = read32(8);
    u32 checksum = read32(12);

    if(! magic || version != SAVE_VERSION || checksum != checksum_of(hiscore) || hiscore > 99999990u)
    {
        return DEFAULT_HISCORE;
    }

    return (int) hiscore;
}

void save_store_hiscore(int hiscore)
{
    sram_mem[0] = 'S';
    sram_mem[1] = 'S';
    sram_mem[2] = 'H';
    sram_mem[3] = 'S';
    write32(4, SAVE_VERSION);
    write32(8, (u32) hiscore);
    write32(12, checksum_of((u32) hiscore));
}
