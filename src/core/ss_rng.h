/* Deterministic xorshift32 random generator (same seed and inputs -> same game). */
#ifndef SS_RNG_H
#define SS_RNG_H

#include "ss_base.h"

typedef struct
{
    u32 state;
} rng;

static inline void rng_seed(rng* r, u32 seed)
{
    r->state = seed ? seed : 0x9E3779B9u;
}

static inline u32 rng_next(rng* r)
{
    u32 x = r->state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    r->state = x;
    return x;
}

/* 0 <= result < limit */
static inline int rng_int(rng* r, int limit)
{
    return (int) ((rng_next(r) >> 8) % (u32) limit);
}

/* min <= result < max */
static inline int rng_range(rng* r, int min, int max)
{
    return min + rng_int(r, max - min);
}

#endif
