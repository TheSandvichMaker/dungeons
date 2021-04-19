#ifndef DUNGEONS_TYPES_HPP
#define DUNGEONS_TYPES_HPP

#include <stdint.h>
#include <stddef.h>

typedef float V2 __attribute__((ext_vector_type(2)));
typedef float V3 __attribute__((ext_vector_type(3)));
typedef float V4 __attribute__((ext_vector_type(4)));

typedef int32_t V2i __attribute__((ext_vector_type(2)));
typedef int32_t V3i __attribute__((ext_vector_type(3)));
typedef int32_t V4i __attribute__((ext_vector_type(4)));

#if 0
struct V2
{
    union
    {
        struct
        {
            float x, y;
        };

        float e[2];
    };
};

struct V3
{
    union
    {
        struct
        {
            float x, y, z;
        };

        float e[3];
    };
};

struct V4
{
    union
    {
        struct
        {
            float x, y, z, w;
        };

        float e[4];
    };
};

struct V2i
{
    union
    {
        struct
        {
            int32_t x, y;
        };

        int32_t e[2];
    };
};

struct V3i
{
    union
    {
        struct
        {
            int32_t x, y, z;
        };

        int32_t e[3];
    };
};

struct V4i
{
    union
    {
        struct
        {
            int32_t x, y, z, w;
        };

        int32_t e[4];
    };
};
#endif

struct Rect2
{
    V2 min, max;
};

struct Rect3
{
    V3 min, max;
};

struct Rect2i
{
    V2i min, max;
};

struct Rect3i
{
    V3i min, max;
};

struct Buffer
{
    size_t size;
    uint8_t *data;
};
typedef Buffer String;

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
    uint32_t w, h;
    Color *data;
};

#endif /* DUNGEONS_TYPES_HPP */
