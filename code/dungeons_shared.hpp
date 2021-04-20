#ifndef DUNGEONS_SHARED_HPP
#define DUNGEONS_SHARED_HPP

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
CopySize(size_t Size, const void *Source, void *Dest)
{
#ifdef _MSVC_VER
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

#define CopyArray(Count, Source, Dest) CopySize(sizeof(*(Source))*Count, Source, Dest)

#endif /* DUNGEONS_SHARED_HPP */
