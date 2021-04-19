#ifndef DUNGEONS_MEMORY_HPP
#define DUNGEONS_MEMORY_HPP

#define DEFAULT_ARENA_CAPACITY Gigabytes(8)

static inline size_t
GetAlignOffset(Arena *arena, size_t Align)
{
    size_t Offset = (size_t)(arena->base + arena->used) & (Align - 1);
    if (Offset)
    {
        Offset = Align - Offset;
    }
    return Offset;
}

static inline char *
GetNextAllocationLocation(Arena *arena, size_t Align)
{
    size_t AlignOffset = GetAlignOffset(arena, Align);
    char* Result = arena->base + arena->used + AlignOffset;
    return Result;
}

static inline size_t
GetSizeRemaining(Arena *arena, size_t Align)
{
    size_t AlignOffset = GetAlignOffset(arena, Align);
    size_t Result = arena->capacity - (arena->used + AlignOffset);
    return Result;
}

static inline void
Clear(Arena *arena)
{
    Assert(arena->temp_count == 0);
    arena->used = 0;
    arena->temp_count = 0;
}

static inline void
Release(Arena *arena)
{
    Assert(arena->temp_count == 0);
    platform->DeallocateMemory(arena->base);
    ZeroStruct(arena);
}

static inline void
ResetTo(Arena *arena, char *Target)
{
    Assert((Target >= arena->base) && (Target <= (arena->base + arena->used)));
    arena->used = (Target - arena->base);
}

static inline void
InitWithMemory(Arena *arena, size_t MemorySize, void *Memory)
{
    ZeroStruct(arena);
    arena->capacity = MemorySize;
    // NOTE: There's an assumption here that the memory passed in is valid, committed memory.
    //       If you want an Arena that exploits virtual memory to progressively commit, you
    //       shouldn't init it with any existing memory.
    arena->committed = MemorySize;
    arena->base = (char *)Memory;
}

static inline void
CheckArena(Arena* arena)
{
    Assert(arena->temp_count == 0);
}

#define PushStruct(arena, Type) \
    (Type *)PushSize_(arena, sizeof(Type), alignof(Type), true, LOCATION_STRING(#arena))
#define PushAlignedStruct(arena, Type, Align) \
    (Type *)PushSize_(arena, sizeof(Type), Align, true, LOCATION_STRING(#arena))
#define PushStructNoClear(arena, Type) \
    (Type *)PushSize_(arena, sizeof(Type), alignof(Type), false, LOCATION_STRING(#arena))
#define PushAlignedStructNoClear(arena, Type, Align) \
    (Type *)PushSize_(arena, sizeof(Type), Align, false, LOCATION_STRING(#arena))

#define PushArray(arena, Count, Type) \
    (Type *)PushSize_(arena, sizeof(Type)*(Count), alignof(Type), true, LOCATION_STRING(#arena))
#define PushAlignedArray(arena, Count, Type, Align) \
    (Type *)PushSize_(arena, sizeof(Type)*(Count), Align, true, LOCATION_STRING(#arena))
#define PushArrayNoClear(arena, Count, Type) \
    (Type *)PushSize_(arena, sizeof(Type)*(Count), alignof(Type), false, LOCATION_STRING(#arena))
#define PushAlignedArrayNoClear(arena, Count, Type, Align) \
    (Type *)PushSize_(arena, sizeof(Type)*(Count), Align, false, LOCATION_STRING(#arena))

#define PushSize(arena, size) \
    PushSize_(arena, size, 1, true, LOCATION_STRING(#arena))
#define PushSizeNoClear(arena, size) \
    PushSize_(arena, size, 1, false, LOCATION_STRING(#arena))

static inline void *
PushSize_(Arena *arena, size_t Size, size_t Align, bool Clear, const char *Tag)
{
    if (!arena->capacity)
    {
        Assert(!arena->base);
        arena->capacity = DEFAULT_ARENA_CAPACITY;
    }

    if (!arena->base)
    {
        // NOTE: Let's align up to page size because that's the minimum allocation granularity anyway,
        //       and the code doing the commit down below assumes our capacity is page aligned.
        arena->capacity = AlignPow2(arena->capacity, platform->page_size);
        arena->base = (char *)platform->ReserveMemory(arena->capacity, PlatformMemFlag_NoLeakCheck, Tag);
    }

    size_t AlignOffset = GetAlignOffset(arena, Align);
    size_t AlignedSize = Size + AlignOffset;

    Assert((arena->used + AlignedSize) <= arena->capacity);

    char *Unalignedbase = arena->base + arena->used;

    if (arena->committed < (arena->used + AlignedSize))
    {
        size_t CommitSize = AlignPow2(AlignedSize, platform->page_size);
        platform->CommitMemory(arena->base + arena->committed, CommitSize);
        arena->committed += CommitSize;
        Assert(arena->committed >= (arena->used + AlignedSize));
    }

    void *Result = Unalignedbase + AlignOffset;
    arena->used += AlignedSize;

    if (Clear) {
        ZeroSize(AlignedSize, Result);
    }

    return Result;
}

#define BootstrapPushStruct(Type, Member)                                             \
    (Type *)BootstrapPushStruct_(sizeof(Type), alignof(Type), offsetof(Type, Member), \
                                 LOCATION_STRING("Bootstrap " #Type "::" #Member))
static inline void *
BootstrapPushStruct_(size_t Size, size_t Align, size_t arenaOffset, const char *Tag)
{
    Arena arena = {};
    void *State = PushSize_(&arena, Size, Align, true, Tag);
    *(Arena *)((char *)State + arenaOffset) = arena;
    return State;
}

struct TemporaryMemory
{
    Arena *arena;
    size_t used;
};

static inline TemporaryMemory
BeginTemporaryMemory(Arena *arena)
{
    TemporaryMemory Result =
    {
        .arena = arena,
        .used  = arena->used,
    };
    ++arena->temp_count;
    return Result;
}

static inline void
EndTemporaryMemory(TemporaryMemory Temp)
{
    if (Temp.arena)
    {
        Assert(Temp.used <= Temp.arena->used);
        Temp.arena->used = Temp.used;
        --Temp.arena->temp_count;
    }
}

static inline void
CommitTemporaryMemory(TemporaryMemory *Temp)
{
    Arena *arena = Temp->arena;
    if (arena)
    {
        Assert(arena->temp_count > 0);
        --arena->temp_count;
        Temp->arena = NULL;
    }
}

struct ScopedMemory
{
    TemporaryMemory temp;
    ScopedMemory(Arena *arena) { temp = BeginTemporaryMemory(arena); }
    ScopedMemory(TemporaryMemory temp) : temp(temp) {}
    ~ScopedMemory() { EndTemporaryMemory(temp); }
    operator TemporaryMemory *() { return &temp; }
};

#endif /* DUNGEONS_MEMORY_HPP */
