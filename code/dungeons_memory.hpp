#ifndef DUNGEONS_MEMORY_HPP
#define DUNGEONS_MEMORY_HPP

#define DEFAULT_ARENA_CAPACITY Gigabytes(8)

static inline size_t
GetAlignOffset(Arena *arena, size_t align)
{
    size_t offset = (size_t)(arena->base + arena->used) & (align - 1);
    if (offset)
    {
        offset = align - offset;
    }
    return offset;
}

static inline uint8_t *
GetNextAllocationLocation(Arena *arena, size_t align)
{
    size_t align_offset = GetAlignOffset(arena, align);
    uint8_t *result = arena->base + arena->used + align_offset;
    return result;
}

static inline size_t
GetSizeRemaining(Arena *arena, size_t align)
{
    size_t align_offset = GetAlignOffset(arena, align);
    size_t result = arena->capacity - (arena->used + align_offset);
    return result;
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
}

static inline void
ResetTo(Arena *arena, uint8_t *target)
{
    Assert((target >= arena->base) && (target <= (arena->base + arena->used)));
    arena->used = (target - arena->base);
}

static inline void
SetCapacity(Arena *arena, size_t capacity)
{
    Assert(!arena->capacity);
    Assert(!arena->base);
    arena->capacity = capacity;
}

static inline void
InitWithMemory(Arena *arena, size_t memory_size, void *memory)
{
    ZeroStruct(arena);
    arena->capacity = memory_size;
    // NOTE: There's an assumption here that the memory passed in is valid, committed memory.
    //       If you want an Arena that exploits virtual memory to progressively commit, you
    //       shouldn't init it with any existing memory.
    arena->committed = memory_size;
    arena->base = (uint8_t *)memory;
}

static inline void
CheckArena(Arena* arena)
{
    Assert(arena->temp_count == 0);
}

#define PushStruct(arena, Type) \
    (Type *)PushSize_(arena, sizeof(Type), alignof(Type), true, LOCATION_STRING(#arena))
#define PushAlignedStruct(arena, Type, align) \
    (Type *)PushSize_(arena, sizeof(Type), align, true, LOCATION_STRING(#arena))
#define PushStructNoClear(arena, Type) \
    (Type *)PushSize_(arena, sizeof(Type), alignof(Type), false, LOCATION_STRING(#arena))
#define PushAlignedStructNoClear(arena, Type, align) \
    (Type *)PushSize_(arena, sizeof(Type), align, false, LOCATION_STRING(#arena))

#define PushArray(arena, Count, Type) \
    (Type *)PushSize_(arena, sizeof(Type)*(Count), alignof(Type), true, LOCATION_STRING(#arena))
#define PushAlignedArray(arena, Count, Type, align) \
    (Type *)PushSize_(arena, sizeof(Type)*(Count), align, true, LOCATION_STRING(#arena))
#define PushArrayNoClear(arena, Count, Type) \
    (Type *)PushSize_(arena, sizeof(Type)*(Count), alignof(Type), false, LOCATION_STRING(#arena))
#define PushAlignedArrayNoClear(arena, Count, Type, align) \
    (Type *)PushSize_(arena, sizeof(Type)*(Count), align, false, LOCATION_STRING(#arena))

#define PushSize(arena, size) \
    PushSize_(arena, size, 1, true, LOCATION_STRING(#arena))
#define PushSizeNoClear(arena, size) \
    PushSize_(arena, size, 1, false, LOCATION_STRING(#arena))
#define PushAlignedSize(arena, size, align) \
    PushSize_(arena, size, align, true, LOCATION_STRING(#arena))
#define PushAlignedSizeNoClear(arena, size, align) \
    PushSize_(arena, size, align, false, LOCATION_STRING(#arena))

static inline void *
PushSize_(Arena *arena, size_t size, size_t align, bool clear, const char *tag)
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
        arena->base = (uint8_t *)platform->ReserveMemory(arena->capacity, PlatformMemFlag_NoLeakCheck, tag);
    }

    size_t align_offset = GetAlignOffset(arena, align);
    size_t aligned_size = size + align_offset;

    Assert((arena->used + aligned_size) <= arena->capacity);

    uint8_t *unaligned_base = arena->base + arena->used;

    if (arena->committed < (arena->used + aligned_size))
    {
        size_t commit_size = AlignPow2(aligned_size, platform->page_size);
        platform->CommitMemory(arena->base + arena->committed, commit_size);
        arena->committed += commit_size;
        Assert(arena->committed >= (arena->used + aligned_size));
    }

    void *result = unaligned_base + align_offset;
    arena->used += aligned_size;

    if (clear) {
        ZeroSize(size, result);
    }

    return result;
}

#define BootstrapPushStruct(Type, Member, ...)                                        \
    (Type *)BootstrapPushStruct_(sizeof(Type), alignof(Type), offsetof(Type, Member), \
                                 LOCATION_STRING("Bootstrap " #Type "::" #Member), ##__VA_ARGS__)
static inline void *
BootstrapPushStruct_(size_t size, size_t align, size_t arena_offset, const char *tag, size_t capacity = DEFAULT_ARENA_CAPACITY)
{
    Arena arena = {};
    SetCapacity(&arena, capacity);
    void *State = PushSize_(&arena, size, align, true, tag);
    *(Arena *)((char *)State + arena_offset) = arena;
    return State;
}

static inline char *
PushNullTerminatedString(Arena *arena, size_t size, char *data)
{
    char *result = PushArrayNoClear(arena, size + 1, char);

    CopyArray(size, data, result);
    result[size] = 0;

    return result;
}

static inline char *
PushNullTerminatedString(Arena *arena, String string)
{
    char *result = PushArrayNoClear(arena, string.size + 1, char);

    CopyArray(string.size, string.data, result);
    result[string.size] = 0;

    return result;
}

struct TemporaryMemory
{
    Arena *arena;
    size_t used;
};

static inline TemporaryMemory
BeginTemporaryMemory(Arena *arena)
{
    TemporaryMemory result = {};
    result.arena = arena;
    result.used  = arena->used;
    ++arena->temp_count;
    return result;
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

template <typename T>
static inline Array<T>
PushArrayContainer(Arena *arena, size_t capacity)
{
    Array<T> result = {};
    result.capacity = capacity;
    if (result.capacity)
    {
        result.data = PushArray(arena, capacity, T);
    }
    return result;
}

static inline String
PushString(Arena *arena, String string)
{
    String result = {};
    result.size = string.size;
    result.data = PushArray(arena, result.size + 1, uint8_t);
    CopySize(result.size, string.data, result.data);
    result.data[result.size] = 0;
    return result;
}

#endif /* DUNGEONS_MEMORY_HPP */
