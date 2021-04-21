#ifndef DUNGEONS_TYPES_HPP
#define DUNGEONS_TYPES_HPP

#include <stdint.h>
#include <stddef.h>
#include <float.h>

#include "dungeons_math_types.hpp"

struct Arena
{
    size_t capacity;
    size_t committed;
    size_t used;
    char *base;
    uint32_t temp_count;
};

struct Buffer
{
    size_t size;
    uint8_t *data;
};
typedef Buffer String;

#define StringLiteral(lit) String { sizeof(lit) - 1, (uint8_t *)lit }
#define StringExpand(string) (int)(string).size, (char *)(string).data

struct StringContainer
{
    union
    {
        struct
        {
            size_t size;
            uint8_t *data;
        };
        String string;
    };
    size_t capacity;
};

struct Range
{
    // convention is [start .. end)
    int start, end;
};

// NOTE: These colors are in BGRA byte order
struct Color
{
    union
    {
        uint32_t u32;

        struct
        {
            uint8_t b, g, r, a;
        };
    };
};

struct Bitmap
{
    int32_t w, h, pitch;
    Color *data;
};

struct Font
{
    union
    {
        struct
        {
            int32_t w, h, pitch;
            Color *data;
        };
        Bitmap bitmap;
    };

    int32_t glyph_w, glyph_h;
    uint32_t glyphs_per_row, glyphs_per_col, glyph_count;
};

#endif /* DUNGEONS_TYPES_HPP */
