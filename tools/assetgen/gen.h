/* Shared helpers for the asset generator's audio and output code. */
#ifndef AG_GEN_H
#define AG_GEN_H

#include <stddef.h>
#include <stdint.h>

#include "canvas.h"

typedef struct
{
    uint8_t* data;
    size_t size;
    size_t capacity;
} bytes;

void bytes_put(bytes* b, uint8_t v);
void bytes_append(bytes* b, const void* data, size_t size);

/* Receives one generated file (name without extension). */
typedef void (*named_blob_fn)(const char* name, const char* ext, const bytes* data);

void music_build_all(named_blob_fn emit);
void sfx_build_all(named_blob_fn emit);

#endif
