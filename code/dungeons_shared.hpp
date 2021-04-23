#ifndef DUNGEONS_SHARED_HPP
#define DUNGEONS_SHARED_HPP

static constexpr bool
IsPow2(size_t size)
{
    return (size != 0 && (size & (size - 1)) == 0);
}

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

#define COLOR_BLACK MakeColor(0, 0, 0)
#define COLOR_WHITE MakeColor(255, 255, 255)

template <typename T>
struct DeferDoodad
{
    T lambda;
    DeferDoodad(T lambda) : lambda(lambda) {}
    ~DeferDoodad() { lambda(); }
};

struct DeferDoodadHelp
{
    template <typename T>
    DeferDoodad<T> operator + (T t) { return t; }
};

#define defer const auto Paste(defer_, __LINE__) = DeferDoodadHelp() + [&]()

#define ArrayCount(x) (sizeof(x) / sizeof((x)[0]))

static inline void
ZeroSize(size_t size_init, void *data_init)
{
    size_t size = size_init;
    char *data = (char *)data_init;
    while (size--)
    {
        *data++ = 0;
    }
}

#define ZeroStruct(Struct) ZeroSize(sizeof(*(Struct)), Struct)
#define ZeroArray(Count, Data) ZeroSize(sizeof(*(Data))*(Count), Data)

static inline bool
MemoryIsEqual(size_t Count, void *AInit, void *BInit)
{
    char *A = (char *)AInit;
    char *B = (char *)BInit;

    bool Result = true;
    while (Count--)
    {
        if (*A++ != *B++)
        {
            Result = false;
            break;
        }
    }
    return Result;
}

#define StructsAreEqual(A, B) (Assert(sizeof(*(A)) == sizeof(*(B))), MemoryIsEqual(sizeof(*(A)), A, B))

static inline void
CopySize(size_t Size, const void *SourceInit, void *DestInit)
{
    const unsigned char *Source = (const unsigned char *)SourceInit;
    unsigned char *Dest = (unsigned char *)DestInit;
#if COMPILER_MSVC
    __movsb(Dest, Source, Size);
#elif defined(__i386__) || defined(__x86_64__)
    __asm__ __volatile__("rep movsb" : "+c"(Size), "+S"(Source), "+D"(Dest): : "memory");
#else
    while (--Size)
    {
        *Dest++ = *Source++;
    }
#endif
}
#define CopyStruct(source, dest) CopySize(sizeof(*(source)), source, dest)

static inline uint64_t
HashString(String string)
{
    char seed[16] = {};
    __m128i hash = _mm_loadu_si128((__m128i*)seed);

    uint64_t len16 = string.size / 16;
    __m128i *at = (__m128i *)string.data;
    while (len16--)
    {
        hash = _mm_aesdec_si128(hash, _mm_loadu_si128(at));
        ++at;
    }

    uint8_t overhang[16] = {};
    CopySize(string.size % 16, string.data, overhang);
    hash = _mm_aesdec_si128(hash, _mm_loadu_si128((__m128i*)overhang));

    uint64_t result = _mm_cvtsi128_si64(hash);
    return result;
}

#define CopyArray(Count, Source, Dest) CopySize(sizeof(*(Source))*Count, Source, Dest)

#endif /* DUNGEONS_SHARED_HPP */
