#ifndef DUNGEONS_INTRINSICS_HPP
#define DUNGEONS_INTRINSICS_HPP

#include <intrin.h>
#include <immintrin.h>
#include <xmmintrin.h>
#include <wmmintrin.h>

struct BitScanResult
{
    uint32_t found;
    uint32_t index;
};

static inline BitScanResult
FindLeastSignificantSetBit(uint32_t value)
{
    BitScanResult result = {};
    
#if COMPILER_MSVC
    result.found = _BitScanForward((unsigned long*)&result.index, value);
#else
    while (value)
    {
        if (value & 1)
        {
            result.found = true;
            break;
        }
        ++result.index;
        value = value >> 1;
    }
#endif
    return result;
}

#endif /* DUNGEONS_INTRINSICS_HPP */
