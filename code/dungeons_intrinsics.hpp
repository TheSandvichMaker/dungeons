#ifndef DUNGEONS_INTRINSICS_HPP
#define DUNGEONS_INTRINSICS_HPP

#include <intrin.h>
#include <immintrin.h>
#include <xmmintrin.h>
#include <wmmintrin.h>

DUNGEONS_INLINE int32_t
I32FromF32Round(float x)
{
    int32_t result = _mm_cvtss_si32(_mm_set_ss(x));    
    return result;
}

DUNGEONS_INLINE int32_t
I32FromF32Floor(float x)
{
    int32_t result = _mm_cvttss_si32(_mm_set_ss(x));    
    return result;
}

static inline uint32_t
RotateLeft(uint32_t value, int32_t amount)
{
#if COMPILER_MSVC
    uint32_t result = _rotl(value, amount);
#else
    amount &= 31;
    uint32_t result = ((value <<  amount) | (value >> (32 - amount)));
#endif

    return result;
}

static inline uint32_t
RotateRight(uint32_t value, int32_t amount)
{
#if COMPILER_MSVC
    uint32_t result = _rotr(value, amount);
#else
    amount &= 31;
    uint32_t result = ((value >>  amount) | (value << (32 - amount)));
#endif
    return result;
}
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

static inline uint64_t
ExtractU64(__m128i v, int index)
{
    AssertSlow(index < 2);
    return ((uint64_t *)&v)[index];
}

#endif /* DUNGEONS_INTRINSICS_HPP */
