/*
 * Colour palettes. Colours are 8-bit RGB chosen as multiples of 8 so they map exactly to the GBA's
 * 15-bit BGR555 format. The master sprite palette is shared by almost every gameplay sprite.
 */
#include "art.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRANSPARENT { 248, 0, 248 }

static const rgb master_colors[] = {
    TRANSPARENT,
    { 16, 16, 40 },     /* K outline / near black */
    { 248, 248, 248 },  /* W white */
    { 176, 184, 208 },  /* l light steel */
    { 88, 96, 128 },    /* d dark steel */
    { 104, 232, 248 },  /* c cyan */
    { 48, 128, 240 },   /* b blue */
    { 24, 48, 136 },    /* n navy */
    { 248, 232, 72 },   /* y yellow */
    { 248, 144, 32 },   /* o orange */
    { 232, 48, 48 },    /* r red */
    { 128, 16, 48 },    /* m maroon */
    { 104, 224, 88 },   /* g green */
    { 24, 120, 64 },    /* G dark green */
    { 248, 104, 208 },  /* p pink */
    { 144, 72, 216 },   /* v violet */
};
const palette pal_master = { ".KWldcbnyormgGpv", master_colors, 16 };

static const rgb boss_colors[] = {
    TRANSPARENT,
    { 16, 8, 32 },      /* K outline */
    { 248, 248, 248 },  /* W highlight */
    { 200, 200, 224 },  /* 1 hull light */
    { 136, 136, 168 },  /* 2 hull mid */
    { 80, 80, 112 },    /* 3 hull dark */
    { 40, 40, 64 },     /* 4 hull shadow */
    { 248, 64, 48 },    /* r core red */
    { 248, 160, 48 },   /* o core orange */
    { 248, 240, 120 },  /* y core yellow */
    { 208, 96, 232 },   /* p crystal light */
    { 120, 48, 176 },   /* v crystal mid */
    { 64, 24, 104 },    /* u crystal dark */
    { 96, 232, 216 },   /* c teal light */
    { 32, 136, 136 },   /* t teal dark */
    { 144, 24, 40 },    /* m core dark */
};
const palette pal_boss = { ".KW1234royp" "vuctm", boss_colors, 16 };

static const rgb stars_colors[] = {
    { 8, 8, 32 },       /* 0 backdrop (deep space) */
    { 56, 64, 104 },    /* 1 dim star */
    { 112, 120, 168 },  /* 2 mid star */
    { 200, 208, 248 },  /* 3 bright star */
    { 248, 248, 248 },  /* 4 white */
    { 248, 216, 136 },  /* 5 warm star */
    { 136, 192, 248 },  /* 6 blue star */
};
const palette pal_stars = { NULL, stars_colors, 7 };

static const rgb nebula_colors[] = {
    { 0, 0, 0 }, { 32, 16, 64 }, { 56, 24, 96 }, { 96, 40, 136 }, { 152, 72, 168 }, { 16, 40, 72 },
    { 32, 72, 120 }, { 72, 120, 168 }, { 40, 72, 120 }, { 24, 40, 80 }, { 200, 176, 120 }, { 136, 112, 80 },
    { 12, 16, 40 },
};
const palette pal_nebula = { NULL, nebula_colors, 13 };

static const rgb cave_colors[] = {
    { 0, 0, 0 }, { 14, 8, 24 }, { 24, 12, 40 }, { 36, 18, 58 }, { 52, 26, 80 }, { 24, 64, 80 },
    { 48, 104, 120 }, { 96, 144, 160 },
};
const palette pal_cave = { NULL, cave_colors, 8 };

static const rgb hull_colors[] = {
    { 0, 0, 0 }, { 14, 14, 24 }, { 22, 22, 36 }, { 32, 32, 50 }, { 44, 44, 68 }, { 64, 24, 28 },
    { 104, 56, 36 }, { 24, 72, 76 }, { 48, 120, 112 },
};
const palette pal_hull = { NULL, hull_colors, 9 };

static const rgb logo_colors[] = {
    { 0, 0, 0 }, { 16, 8, 40 }, { 248, 248, 200 }, { 248, 224, 72 }, { 248, 160, 40 }, { 232, 80, 40 },
    { 152, 32, 56 }, { 40, 48, 120 }, { 88, 200, 248 }, { 48, 120, 216 }, { 248, 248, 248 },
};
const palette pal_logo = { NULL, logo_colors, 11 };

static const rgb crystal_colors[] = {
    { 0, 0, 0 }, { 24, 8, 40 }, { 88, 48, 136 }, { 128, 80, 184 }, { 184, 136, 232 }, { 32, 128, 168 },
    { 104, 216, 240 }, { 224, 255, 255 }, { 56, 24, 88 },
};
const palette pal_terrain_crystal = { NULL, crystal_colors, 9 };

static const rgb metal_colors[] = {
    { 0, 0, 0 }, { 24, 24, 32 }, { 112, 120, 144 }, { 160, 168, 192 }, { 224, 232, 248 }, { 32, 24, 16 },
    { 248, 200, 48 }, { 120, 232, 216 }, { 72, 80, 104 },
};
const palette pal_terrain_metal = { NULL, metal_colors, 9 };

const rgb pal_font[4][4] = {
    { TRANSPARENT, { 24, 16, 56 }, { 248, 248, 248 }, { 160, 168, 200 } },     /* white */
    { TRANSPARENT, { 56, 24, 16 }, { 248, 224, 64 }, { 200, 120, 40 } },       /* yellow */
    { TRANSPARENT, { 8, 32, 64 }, { 112, 240, 248 }, { 48, 144, 200 } },       /* cyan */
    { TRANSPARENT, { 48, 8, 16 }, { 248, 88, 72 }, { 160, 40, 40 } },          /* red */
};

void palette_charmap(const palette* p, int* map256)
{
    for(int i = 0; i < 256; ++i)
    {
        map256[i] = -1;
    }

    for(int i = 0; p->keys && p->keys[i]; ++i)
    {
        map256[(unsigned char) p->keys[i]] = i;
    }
}

int pal_index(const palette* p, char k)
{
    const char* at = p->keys ? strchr(p->keys, k) : NULL;

    if(! at || ! k)
    {
        fprintf(stderr, "assetgen: colour key '%c' not in palette\n", k);
        exit(1);
    }

    return (int) (at - p->keys);
}
