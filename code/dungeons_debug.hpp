#ifndef DUNGEONS_DEBUG_HPP
#define DUNGEONS_DEBUG_HPP

struct DebugEntry
{
    const char *guid;
    const char *name;
    uint64_t clocks;
    uint64_t hit_count;
};

struct OpenDebugEntry
{
    OpenDebugEntry *next;

    uint64_t begin_clock;
    uint32_t begin_thread_id;
    DebugEntry *entry;
};

struct DebugState
{
    Arena arena;

    OpenDebugEntry *first_open_entry;
    OpenDebugEntry *first_free_open_entry;

    uint32_t entry_count;
    DebugEntry entries[4096];
    DebugEntry null_entry;
};
static DebugState *debug_state;

#endif /* DUNGEONS_DEBUG_HPP */
