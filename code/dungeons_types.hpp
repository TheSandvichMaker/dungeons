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
    uint8_t *base;
    uint32_t temp_count;
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

// TODO: where tf do I put this
DUNGEONS_INLINE Color
MakeColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
    Color color;
    color.r = r;
    color.g = g;
    color.b = b;
    color.a = a;
    return color;
}

struct Buffer
{
    size_t size;
    uint8_t *data;
};
typedef Buffer String;

constexpr String
operator ""_str(const char *data, size_t size)
{
    String result = {};
    result.size = size;
    result.data = (uint8_t *)data;
    return result;
}

#define StringLiteral(c_string) Paste(c_string, _str)
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

struct StringNode
{
    StringNode *next;
    Color foreground;
    Color background;
    String string;
};

struct StringList
{
    StringNode *first = nullptr;
    StringNode *last = nullptr;
    size_t total_size = 0;
    size_t node_count = 0;
    Color foreground = MakeColor(255, 255, 255);
    Color background = MakeColor(0, 0, 0);
};

struct Range
{
    // convention is [start .. end)
    int start, end;
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
