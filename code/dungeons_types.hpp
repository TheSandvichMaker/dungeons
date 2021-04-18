#ifndef DUNGEONS_TYPES_HPP
#define DUNGEONS_TYPES_HPP

#include <stdint.h>
#include <stddef.h>

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

#endif /* DUNGEONS_TYPES_HPP */
