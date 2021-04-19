#ifndef DUNGEONS_MATH_HPP
#define DUNGEONS_MATH_HPP

// TODO: Eliminate this include for cool programmer brownie points
#include <math.h>

// Cool constants

#define Pi32 3.14159265f
#define Tau32 6.28318531f
#define Pi64 3.14159265358979324
#define Tau64 6.28318530717958648
#define RcpPi32 0.318309886f
#define RcpTau32 0.159154943f
#define RcpPi64 0.318309886183790672
#define RcpTau64 0.159154943091895336

// Constructor functions

DUNGEONS_INLINE V2 MakeV2(float s) { return (V2) { s, s }; }
DUNGEONS_INLINE V2 MakeV2(float x, float y) { return (V2) { x, y }; }
DUNGEONS_INLINE V3 MakeV3(float s) { return (V3) { s, s, s }; }
DUNGEONS_INLINE V3 MakeV3(V2 xy, float z) { return (V3) { xy[0], xy[1], z }; }
DUNGEONS_INLINE V3 MakeV3(float x, V2 yz) { return (V3) { x, yz[0], yz[1] }; }
DUNGEONS_INLINE V3 MakeV3(float x, float y, float z) { return (V3) { x, y, z }; }
DUNGEONS_INLINE V4 MakeV4(float s) { return (V4) { s, s, s, s }; }
DUNGEONS_INLINE V4 MakeV4(V2 xy, float z, float w) { return (V4) { xy[0], xy[1], z, w }; }
DUNGEONS_INLINE V4 MakeV4(float x, float y, V2 zw) { return (V4) { x, y, zw[0], zw[1] }; }
DUNGEONS_INLINE V4 MakeV4(V2 xy, V2 zw) { return (V4) { xy[0], xy[1], zw[0], zw[1] }; }
DUNGEONS_INLINE V4 MakeV4(V3 xyz, float w) { return (V4) { xyz[0], xyz[1], xyz[2], w }; }
DUNGEONS_INLINE V4 MakeV4(float x, V3 yzw) { return (V4) { x, yzw[0], yzw[1], yzw[2] }; }
DUNGEONS_INLINE V4 MakeV4(float x, float y, float z, float w) { return (V4) { x, y, z, w }; }

DUNGEONS_INLINE float Square(float x) { return x*x; }
DUNGEONS_INLINE float Sqrt(float x) { return sqrtf(x); }

DUNGEONS_INLINE float Sin(float x) { return sinf(x); }
DUNGEONS_INLINE float Cos(float x) { return cosf(x); }
DUNGEONS_INLINE float Tan(float x) { return tanf(x); }
DUNGEONS_INLINE float ASin(float x) { return asinf(x); }
DUNGEONS_INLINE float ACos(float x) { return acosf(x); }
DUNGEONS_INLINE float ATan(float x) { return atanf(x); }
DUNGEONS_INLINE float ATan2(float y, float x) { return atan2f(y, x); }
DUNGEONS_INLINE float SinH(float x) { return sinhf(x); }
DUNGEONS_INLINE float CosH(float x) { return coshf(x); }
DUNGEONS_INLINE float TanH(float x) { return tanhf(x); }
DUNGEONS_INLINE float ASinH(float x) { return asinhf(x); }
DUNGEONS_INLINE float ACosH(float x) { return acoshf(x); }
DUNGEONS_INLINE float ATanH(float x) { return atanhf(x); }
DUNGEONS_INLINE float Abs(float x) { return fabsf(x); }

DUNGEONS_INLINE int
DivFloor(int a, int b)
{
    Assert(b != 0);
    int res = a / b;
    int rem = a % b;
    int corr = (rem != 0 && ((rem < 0) != (b < 0)));
    return res - corr;
}

DUNGEONS_INLINE float
DegToRad(float deg)
{
    float rad = deg*(Pi32 / 180.0f);
    return rad;
}

DUNGEONS_INLINE float
RadToDeg(float rad)
{
    float deg = rad*(180.0f / Pi32);
    return deg;
}

DUNGEONS_INLINE uint32_t
RoundNextPow2(uint32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

DUNGEONS_INLINE float
Smoothstep(float x)
{
    float result = x*x*(3.0f - 2.0f*x);
    return result;
}

DUNGEONS_INLINE float
Smootherstep(float x)
{
    float result = x*x*x*(x*(x*6.0f - 15.0f) + 10.0f);
    return result;
}

DUNGEONS_INLINE uint32_t
HashCoordinate(int32_t x, int32_t y, int32_t z)
{
    uint32_t result = ((x*73856093)^(y*83492791)^(z*871603259));
    return result;
}

DUNGEONS_INLINE float
MapToRange(float t, float min, float max)
{
    float result = 0.0f;
    float range = max - min;
    if (range != 0.0f)
    {
        result = (t - min) / range;
    }
    return result;
}

DUNGEONS_INLINE float
SafeRatioN(float numerator, float divisor, float n)
{
    float result = n;
    
    if (divisor != 0.0f)
    {
        result = numerator / divisor;
    }
    
    return result;
}

DUNGEONS_INLINE float
SafeRatio0(float numerator, float divisor)
{
    float result = SafeRatioN(numerator, divisor, 0.0f);
    return result;
}

DUNGEONS_INLINE float
SafeRatio1(float numerator, float divisor)
{
    float result = SafeRatioN(numerator, divisor, 1.0f);
    return result;
}

DUNGEONS_INLINE float
SignOf(float a)
{
    return (a >= 0.0f ? 1.0f : 0.0f);
}

DUNGEONS_INLINE float
Min(float a, float b)
{
    return (a < b ? a : b);
}

DUNGEONS_INLINE V2
Min(V2 a, V2 b)
{
    V2 result;
    result.x = (a.x < b.x ? a.x : b.x);
    result.y = (a.y < b.y ? a.y : b.y);
    return result;
}

DUNGEONS_INLINE V3
Min(V3 a, V3 b)
{
    V3 result;
    result.x = (a.x < b.x ? a.x : b.x);
    result.y = (a.y < b.y ? a.y : b.y);
    result.z = (a.z < b.z ? a.z : b.z);
    return result;
}

DUNGEONS_INLINE V4
Min(V4 a, V4 b)
{
    V4 result;
    result.x = (a.x < b.x ? a.x : b.x);
    result.y = (a.y < b.y ? a.y : b.y);
    result.z = (a.z < b.z ? a.z : b.z);
    result.w = (a.w < b.w ? a.w : b.w);
    return result;
}

DUNGEONS_INLINE float
Max(float a, float b)
{
    return (a > b ? a : b);
}

DUNGEONS_INLINE V2
Max(V2 a, V2 b)
{
    V2 result;
    result.x = (a.x > b.x ? a.x : b.x);
    result.y = (a.y > b.y ? a.y : b.y);
    return result;
}

DUNGEONS_INLINE V3
Max(V3 a, V3 b)
{
    V3 result;
    result.x = (a.x > b.x ? a.x : b.x);
    result.y = (a.y > b.y ? a.y : b.y);
    result.z = (a.z > b.z ? a.z : b.z);
    return result;
}

DUNGEONS_INLINE V4
Max(V4 a, V4 b)
{
    V4 result;
    result.x = (a.x > b.x ? a.x : b.x);
    result.y = (a.y > b.y ? a.y : b.y);
    result.z = (a.z > b.z ? a.z : b.z);
    result.w = (a.w > b.w ? a.w : b.w);
    return result;
}

DUNGEONS_INLINE float
Clamp(float a, float min, float max)
{
    if (a < min) a = min;
    if (a > max) a = max;
    return a;
}

DUNGEONS_INLINE V2
Clamp(V2 a, V2 min, V2 max)
{
    if (a.x < min.x) a.x = min.x;
    if (a.x > max.x) a.x = max.x;
    if (a.y < min.y) a.y = min.y;
    if (a.y > max.y) a.y = max.y;
    return a;
}

DUNGEONS_INLINE V3
Clamp(V3 a, V3 min, V3 max)
{
    if (a.x < min.x) a.x = min.x;
    if (a.x > max.x) a.x = max.x;
    if (a.y < min.y) a.y = min.y;
    if (a.y > max.y) a.y = max.y;
    if (a.z < min.z) a.z = min.z;
    if (a.z > max.z) a.z = max.z;
    return a;
}

DUNGEONS_INLINE V4
Clamp(V4 a, V4 min, V4 max)
{
    if (a.x < min.x) a.x = min.x;
    if (a.x > max.x) a.x = max.x;
    if (a.y < min.y) a.y = min.y;
    if (a.y > max.y) a.y = max.y;
    if (a.z < min.z) a.z = min.z;
    if (a.z > max.z) a.z = max.z;
    if (a.w < min.w) a.w = min.w;
    if (a.w > max.w) a.w = max.w;
    return a;
}

DUNGEONS_INLINE float
Clamp01(float a)
{
    if (a < 0.0f) a = 0.0f;
    if (a > 1.0f) a = 1.0f;
    return a;
}

DUNGEONS_INLINE float
Clamp01MapToRange(float t, float min, float max)
{
    float result = Clamp01(MapToRange(t, min, max));
    return result;
}

DUNGEONS_INLINE V2
Clamp01(V2 a)
{
    if (a.x < 0.0f) a.x = 0.0f;
    if (a.x > 1.0f) a.x = 1.0f;
    if (a.y < 0.0f) a.y = 0.0f;
    if (a.y > 1.0f) a.y = 1.0f;
    return a;
}

DUNGEONS_INLINE V3
Clamp01(V3 a)
{
    if (a.x < 0.0f) a.x = 0.0f;
    if (a.x > 1.0f) a.x = 1.0f;
    if (a.y < 0.0f) a.y = 0.0f;
    if (a.y > 1.0f) a.y = 1.0f;
    if (a.z < 0.0f) a.z = 0.0f;
    if (a.z > 1.0f) a.z = 1.0f;
    return a;
}

DUNGEONS_INLINE V4
Clamp01(V4 a)
{
    if (a.x < 0.0f) a.x = 0.0f;
    if (a.x > 1.0f) a.x = 1.0f;
    if (a.y < 0.0f) a.y = 0.0f;
    if (a.y > 1.0f) a.y = 1.0f;
    if (a.z < 0.0f) a.z = 0.0f;
    if (a.z > 1.0f) a.z = 1.0f;
    if (a.w < 0.0f) a.w = 0.0f;
    if (a.w > 1.0f) a.w = 1.0f;
    return a;
}

DUNGEONS_INLINE float
NeighborhoodDistance(float a, float b, float period)
{
    float d0 = Abs(a - b);
    float d1 = Abs(a - period*SignOf(a) - b);
    float result = Min(d0, d1);
    return result;
}

DUNGEONS_INLINE float Dot(V2 a, V2 b) { return a.x*b.x + a.y*b.y; }
DUNGEONS_INLINE float Dot(V3 a, V3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
DUNGEONS_INLINE float Dot(V4 a, V4 b) { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }
DUNGEONS_INLINE float LengthSq(V2 a) { return a.x*a.x + a.y*a.y; }
DUNGEONS_INLINE float LengthSq(V3 a) { return a.x*a.x + a.y*a.y + a.z*a.z; }
DUNGEONS_INLINE float LengthSq(V4 a) { return a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w; }
DUNGEONS_INLINE float Length(V2 a) { return Sqrt(a.x*a.x + a.y*a.y); }
DUNGEONS_INLINE float Length(V3 a) { return Sqrt(a.x*a.x + a.y*a.y + a.z*a.z); }
DUNGEONS_INLINE float Length(V4 a) { return Sqrt(a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w); }

DUNGEONS_INLINE V2
Normalize(V2 a)
{
    float length = (a.x*a.x + a.y*a.y);
    return a*(1.0f / length);
};

DUNGEONS_INLINE V3
Normalize(V3 a)
{
    float length = (a.x*a.x + a.y*a.y + a.z*a.z);
    return a*(1.0f / length);
};

DUNGEONS_INLINE V4
Normalize(V4 a)
{
    float length = (a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w);
    return a*(1.0f / length);
};

DUNGEONS_INLINE float
Lerp(float a, float b, float t)
{
    return (1.0f - t)*a + t*b;
}

DUNGEONS_INLINE V2
Lerp(V2 a, V2 b, float t)
{
    V2 result;
    result.x = (1.0f - t)*a.x + t*b.x;
    result.y = (1.0f - t)*a.y + t*b.y;
    return result;
}

DUNGEONS_INLINE V2
Lerp(V2 a, V2 b, V2 t)
{
    V2 result;
    result.x = (1.0f - t.x)*a.x + t.x*b.x;
    result.y = (1.0f - t.y)*a.y + t.y*b.y;
    return result;
}

DUNGEONS_INLINE V3
Lerp(V3 a, V3 b, float t)
{
    V3 result;
    result.x = (1.0f - t)*a.x + t*b.x;
    result.y = (1.0f - t)*a.y + t*b.y;
    result.z = (1.0f - t)*a.z + t*b.z;
    return result;
}

DUNGEONS_INLINE V3
Lerp(V3 a, V3 b, V3 t)
{
    V3 result;
    result.x = (1.0f - t.x)*a.x + t.x*b.x;
    result.y = (1.0f - t.y)*a.y + t.y*b.y;
    result.z = (1.0f - t.z)*a.z + t.z*b.z;
    return result;
}

DUNGEONS_INLINE V4
Lerp(V4 a, V4 b, float t)
{
    V4 result;
    result.x = (1.0f - t)*a.x + t*b.x;
    result.y = (1.0f - t)*a.y + t*b.y;
    result.z = (1.0f - t)*a.z + t*b.z;
    result.w = (1.0f - t)*a.w + t*b.w;
    return result;
}

DUNGEONS_INLINE V4
Lerp(V4 a, V4 b, V4 t)
{
    V4 result;
    result.x = (1.0f - t.x)*a.x + t.x*b.x;
    result.y = (1.0f - t.y)*a.y + t.y*b.y;
    result.z = (1.0f - t.z)*a.z + t.z*b.z;
    result.w = (1.0f - t.w)*a.w + t.w*b.w;
    return result;
}

DUNGEONS_INLINE V3
Cross(V3 a, V3 b)
{
    V3 result;
    result.x = a.y*b.z - a.z*b.y;
    result.y = a.z*b.x - a.x*b.z;
    result.z = a.x*b.y - a.y*b.x;
    return result;
}

DUNGEONS_INLINE uint32_t LengthSq(V2i a) { return a.x*a.x + a.y*a.y; }
DUNGEONS_INLINE uint32_t LengthSq(V3i a) { return a.x*a.x + a.y*a.y + a.z*a.z; }
DUNGEONS_INLINE uint32_t LengthSq(V4i a) { return a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w; }
DUNGEONS_INLINE float Length(V2i a) { return Sqrt((float)(a.x*a.x + a.y*a.y)); }
DUNGEONS_INLINE float Length(V3i a) { return Sqrt((float)(a.x*a.x + a.y*a.y + a.z*a.z)); }
DUNGEONS_INLINE float Length(V4i a) { return Sqrt((float)(a.x*a.x + a.y*a.y + a.z*a.z + a.w*a.w)); }

DUNGEONS_INLINE int
Min(int a, int b)
{
    return (a < b ? a : b);
}

DUNGEONS_INLINE V2i
Min(V2i a, V2i b)
{
    V2i result;
    result.x = (a.x < b.x ? a.x : b.x);
    result.y = (a.y < b.y ? a.y : b.y);
    return result;
}

DUNGEONS_INLINE V3i
Min(V3i a, V3i b)
{
    V3i result;
    result.x = (a.x < b.x ? a.x : b.x);
    result.y = (a.y < b.y ? a.y : b.y);
    result.z = (a.z < b.z ? a.z : b.z);
    return result;
}

DUNGEONS_INLINE V4i
Min(V4i a, V4i b)
{
    V4i result;
    result.x = (a.x < b.x ? a.x : b.x);
    result.y = (a.y < b.y ? a.y : b.y);
    result.z = (a.z < b.z ? a.z : b.z);
    result.w = (a.w < b.w ? a.w : b.w);
    return result;
}

DUNGEONS_INLINE int
Max(int a, int b)
{
    return (a > b ? a : b);
}

DUNGEONS_INLINE V2i
Max(V2i a, V2i b)
{
    V2i result;
    result.x = (a.x > b.x ? a.x : b.x);
    result.y = (a.y > b.y ? a.y : b.y);
    return result;
}

DUNGEONS_INLINE V3i
Max(V3i a, V3i b)
{
    V3i result;
    result.x = (a.x > b.x ? a.x : b.x);
    result.y = (a.y > b.y ? a.y : b.y);
    result.z = (a.z > b.z ? a.z : b.z);
    return result;
}

DUNGEONS_INLINE V4i
Max(V4i a, V4i b)
{
    V4i result;
    result.x = (a.x > b.x ? a.x : b.x);
    result.y = (a.y > b.y ? a.y : b.y);
    result.z = (a.z > b.z ? a.z : b.z);
    result.w = (a.w > b.w ? a.w : b.w);
    return result;
}

DUNGEONS_INLINE int
Clamp(int a, int min, int max)
{
    if (a < min) a = min;
    if (a > max) a = max;
    return a;
}

DUNGEONS_INLINE V2i
Clamp(V2i a, V2i min, V2i max)
{
    if (a.x < min.x) a.x = min.x;
    if (a.x > max.x) a.x = max.x;
    if (a.y < min.y) a.y = min.y;
    if (a.y > max.y) a.y = max.y;
    return a;
}

DUNGEONS_INLINE V3i
Clamp(V3i a, V3i min, V3i max)
{
    if (a.x < min.x) a.x = min.x;
    if (a.x > max.x) a.x = max.x;
    if (a.y < min.y) a.y = min.y;
    if (a.y > max.y) a.y = max.y;
    if (a.z < min.z) a.z = min.z;
    if (a.z > max.z) a.z = max.z;
    return a;
}

DUNGEONS_INLINE V4i
Clamp(V4i a, V4i min, V4i max)
{
    if (a.x < min.x) a.x = min.x;
    if (a.x > max.x) a.x = max.x;
    if (a.y < min.y) a.y = min.y;
    if (a.y > max.y) a.y = max.y;
    if (a.z < min.z) a.z = min.z;
    if (a.z > max.z) a.z = max.z;
    if (a.w < min.w) a.w = min.w;
    if (a.w > max.w) a.w = max.w;
    return a;
}

DUNGEONS_INLINE int
Clamp01(int a)
{
    if (a < 0) a = 0;
    if (a > 1) a = 1;
    return a;
}

DUNGEONS_INLINE V2i
Clamp01(V2i a)
{
    if (a.x < 0) a.x = 0;
    if (a.x > 1) a.x = 1;
    if (a.y < 0) a.y = 0;
    if (a.y > 1) a.y = 1;
    return a;
}

DUNGEONS_INLINE V3i
Clamp01(V3i a)
{
    if (a.x < 0) a.x = 0;
    if (a.x > 1) a.x = 1;
    if (a.y < 0) a.y = 0;
    if (a.y > 1) a.y = 1;
    if (a.z < 0) a.z = 0;
    if (a.z > 1) a.z = 1;
    return a;
}

DUNGEONS_INLINE V4i
Clamp01(V4i a)
{
    if (a.x < 0) a.x = 0;
    if (a.x > 1) a.x = 1;
    if (a.y < 0) a.y = 0;
    if (a.y > 1) a.y = 1;
    if (a.z < 0) a.z = 0;
    if (a.z > 1) a.z = 1;
    if (a.w < 0) a.w = 0;
    if (a.w > 1) a.w = 1;
    return a;
}

DUNGEONS_INLINE int32_t
ManhattanDistance(V2i a, V2i b)
{
    int32_t result = Abs(a.x - b.x) + Abs(a.y - b.y);
    return result;
}

DUNGEONS_INLINE int32_t
ManhattanDistance(V3i a, V3i b, V3i c)
{
    int32_t result = Abs(a.x - b.x) + Abs(a.y - b.y) + Abs(a.z - b.z);
    return result;
}

DUNGEONS_INLINE float
DiagonalDistance(V2i a, V2i b, float diagonal_cost)
{
    int32_t dx = Abs(a.x - b.x);
    int32_t dy = Abs(a.y - b.y);
    float result = (float)(dx + dy) + (diagonal_cost - 2.0f)*(float)Min(dx, dy);
    return result;
}

DUNGEONS_INLINE float
DiagonalDistance(V3i a, V3i b, float diagonal_cost)
{
    int32_t dx = Abs(a.x - b.x);
    int32_t dy = Abs(a.y - b.y);
    int32_t dz = Abs(a.z - b.z);
    float result = (float)(dx + dy + dz) + (diagonal_cost - 2.0f)*(float)Min(Min(dx, dy), dz);
    return result;
}

//
// Rect2
//

DUNGEONS_INLINE Rect2
Rect2MinMax(V2 min, V2 max)
{
    Rect2 result;
    result.min = min;
    result.max = max;
    return result;
}

DUNGEONS_INLINE Rect2
Rect2MinDim(V2 min, V2 dim)
{
    Rect2 result;
    result.min = min;
    result.max = min + dim;
    return result;
}

DUNGEONS_INLINE Rect2
Rect2CenterDim(V2 center, V2 dim)
{
    Rect2 result;
    result.min = center - 0.5f*dim;
    result.max = center + 0.5f*dim;
    return result;
}

DUNGEONS_INLINE Rect2
Rect2CenterHalfDim(V2 center, V2 half_dim)
{
    Rect2 result;
    result.min = center - half_dim;
    result.max = center + half_dim;
    return result;
}

DUNGEONS_INLINE Rect2
AddDim(Rect2 a, V2 dim)
{
    Rect2 result;
    result.min = a.min - 0.5f*dim;
    result.max = a.max + 0.5f*dim;
    return result;
}

DUNGEONS_INLINE Rect2
AddHalfDim(Rect2 a, V2 half_dim)
{
    Rect2 result;
    result.min = a.min - half_dim;
    result.max = a.max + half_dim;
    return result;
}

DUNGEONS_INLINE Rect2
Offset(Rect2 a, V2 offset)
{
    Rect2 result;
    result.min = a.min + offset;
    result.max = a.max + offset;
    return result;
}

DUNGEONS_INLINE bool
IsInRect(Rect2 a, V2 p)
{
    bool result = (p.x >= a.min.x && p.x < a.max.x &&
                   p.y >= a.min.y && p.y < a.max.y);
    return result;
}

DUNGEONS_INLINE Rect2
GrowToContain(Rect2 a, V2 p)
{
    Rect2 result;
    result.min = Min(a.min, p);
    result.max = Max(a.max, p);
    return result;
}

DUNGEONS_INLINE Rect2
Union(Rect2 a, Rect2 b)
{
    Rect2 result;
    result.min = Min(a.min, b.min);
    result.max = Max(a.max, b.max);
    return result;
}

DUNGEONS_INLINE Rect2
Intersect(Rect2 a, Rect2 b)
{
    Rect2 result;
    result.min = Max(a.min, b.min);
    result.max = Min(a.max, b.max);
    return result;
}

//
// Rect2i
//

DUNGEONS_INLINE Rect2i
Rect2iMinMax(V2i min, V2i max)
{
    Rect2i result;
    result.min = min;
    result.max = max;
    return result;
}

DUNGEONS_INLINE Rect2i
Rect2iMinDim(V2i min, V2i dim)
{
    Rect2i result;
    result.min = min;
    result.max = min + dim;
    return result;
}

DUNGEONS_INLINE Rect2i
Rect2iCenterHalfDim(V2i center, V2i half_dim)
{
    Rect2i result;
    result.min = center - half_dim;
    result.max = center + half_dim;
    return result;
}

DUNGEONS_INLINE Rect2i
AddHalfDim(Rect2i a, V2i half_dim)
{
    Rect2i result;
    result.min = a.min - half_dim;
    result.max = a.max + half_dim;
    return result;
}

DUNGEONS_INLINE Rect2i
Offset(Rect2i a, V2i offset)
{
    Rect2i result;
    result.min = a.min + offset;
    result.max = a.max + offset;
    return result;
}

DUNGEONS_INLINE bool
IsInRect(Rect2i a, V2i p)
{
    bool result = (p.x >= a.min.x && p.x < a.max.x &&
                   p.y >= a.min.y && p.y < a.max.y);
    return result;
}

DUNGEONS_INLINE Rect2i
GrowToContain(Rect2i a, V2i p)
{
    Rect2i result;
    result.min = Min(a.min, p);
    result.max = Max(a.max, p);
    return result;
}

DUNGEONS_INLINE Rect2i
Union(Rect2i a, Rect2i b)
{
    Rect2i result;
    result.min = Min(a.min, b.min);
    result.max = Max(a.max, b.max);
    return result;
}

DUNGEONS_INLINE Rect2i
Intersect(Rect2i a, Rect2i b)
{
    Rect2i result;
    result.min = Max(a.min, b.min);
    result.max = Min(a.max, b.max);
    return result;
}

//
// Rect3
//

DUNGEONS_INLINE Rect3
Rect3MinMax(V3 min, V3 max)
{
    Rect3 result;
    result.min = min;
    result.max = max;
    return result;
}

DUNGEONS_INLINE Rect3
Rect3MinDim(V3 min, V3 dim)
{
    Rect3 result;
    result.min = min;
    result.max = min + dim;
    return result;
}

DUNGEONS_INLINE Rect3
Rect3CenterDim(V3 center, V3 dim)
{
    Rect3 result;
    result.min = center - 0.5f*dim;
    result.max = center + 0.5f*dim;
    return result;
}

DUNGEONS_INLINE Rect3
Rect3CenterHalfDim(V3 center, V3 half_dim)
{
    Rect3 result;
    result.min = center - half_dim;
    result.max = center + half_dim;
    return result;
}

DUNGEONS_INLINE Rect3
AddDim(Rect3 a, V3 dim)
{
    Rect3 result;
    result.min = a.min - 0.5f*dim;
    result.max = a.max + 0.5f*dim;
    return result;
}

DUNGEONS_INLINE Rect3
AddHalfDim(Rect3 a, V3 half_dim)
{
    Rect3 result;
    result.min = a.min - half_dim;
    result.max = a.max + half_dim;
    return result;
}

DUNGEONS_INLINE Rect3
Offset(Rect3 a, V3 offset)
{
    Rect3 result;
    result.min = a.min + offset;
    result.max = a.max + offset;
    return result;
}

DUNGEONS_INLINE bool
IsInRect(Rect3 a, V3 p)
{
    bool result = (p.x >= a.min.x && p.x < a.max.x &&
                   p.y >= a.min.y && p.y < a.max.y &&
                   p.y >= a.min.z && p.z < a.max.z);
    return result;
}

DUNGEONS_INLINE Rect3
GrowToContain(Rect3 a, V3 p)
{
    Rect3 result;
    result.min = Min(a.min, p);
    result.max = Max(a.max, p);
    return result;
}

DUNGEONS_INLINE Rect3
Union(Rect3 a, Rect3 b)
{
    Rect3 result;
    result.min = Min(a.min, b.min);
    result.max = Max(a.max, b.max);
    return result;
}

DUNGEONS_INLINE Rect3
Intersect(Rect3 a, Rect3 b)
{
    Rect3 result;
    result.min = Max(a.min, b.min);
    result.max = Min(a.max, b.max);
    return result;
}

//
// Rect3i
//

DUNGEONS_INLINE Rect3i
Rect3iMinMax(V3i min, V3i max)
{
    Rect3i result;
    result.min = min;
    result.max = max;
    return result;
}

DUNGEONS_INLINE Rect3i
Rect3iMinDim(V3i min, V3i dim)
{
    Rect3i result;
    result.min = min;
    result.max = min + dim;
    return result;
}

DUNGEONS_INLINE Rect3i
Rect3iCenterHalfDim(V3i center, V3i half_dim)
{
    Rect3i result;
    result.min = center - half_dim;
    result.max = center + half_dim;
    return result;
}

DUNGEONS_INLINE Rect3i
AddHalfDim(Rect3i a, V3i half_dim)
{
    Rect3i result;
    result.min = a.min - half_dim;
    result.max = a.max + half_dim;
    return result;
}

DUNGEONS_INLINE Rect3i
Offset(Rect3i a, V3i offset)
{
    Rect3i result;
    result.min = a.min + offset;
    result.max = a.max + offset;
    return result;
}

DUNGEONS_INLINE bool
IsInRect(Rect3i a, V3i p)
{
    bool result = (p.x >= a.min.x && p.x < a.max.x &&
                   p.y >= a.min.y && p.y < a.max.y &&
                   p.z >= a.min.y && p.y < a.max.z);
    return result;
}

DUNGEONS_INLINE Rect3i
GrowToContain(Rect3i a, V3i p)
{
    Rect3i result;
    result.min = Min(a.min, p);
    result.max = Max(a.max, p);
    return result;
}

DUNGEONS_INLINE Rect3i
Union(Rect3i a, Rect3i b)
{
    Rect3i result;
    result.min = Min(a.min, b.min);
    result.max = Max(a.max, b.max);
    return result;
}

DUNGEONS_INLINE Rect3i
Intersect(Rect3i a, Rect3i b)
{
    Rect3i result;
    result.min = Max(a.min, b.min);
    result.max = Min(a.max, b.max);
    return result;
}

#endif /* DUNGEONS_MATH_HPP */
