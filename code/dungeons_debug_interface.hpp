#ifndef DUNGEONS_DEBUG_INTERFACE_HPP
#define DUNGEONS_DEBUG_INTERFACE_HPP

#if DUNGEONS_INTERNAL

#define APP_DEBUG_END_FRAME(name) void name()
typedef APP_DEBUG_END_FRAME(AppDebugEndFrameType);
extern "C" DUNGEONS_EXPORT APP_DEBUG_END_FRAME(AppDebugEndFrame);

#define MAX_DEBUG_EVENTS (1 << 16)

enum DebugEventKind
{
    DebugEvent_BeginProfileZone,
    DebugEvent_EndProfileZone,
};

struct DebugEvent
{
    const char *guid;
    const char *name;
    uint64_t clock;
    uint32_t thread_id;
    DebugEventKind kind;
};

struct DebugTable
{
    uint32_t current_buffer_index;
    volatile uint32_t event_count__buffer_index;
    DebugEvent event_buffers[2][MAX_DEBUG_EVENTS];
};
extern DebugTable *debug_table;

static inline DebugEvent *
DebugRecordEvent(DebugEventKind kind, const char *guid, const char *name)
{
    uint32_t event_index__buffer_index = AtomicIncrement(&debug_table->event_count__buffer_index); 
    uint32_t event_index  = event_index__buffer_index & 0x7FFFFFFF;
    uint32_t buffer_index = event_index__buffer_index >> 31;

    DebugEvent *event = &debug_table->event_buffers[buffer_index][event_index]; 
    event->kind = kind;
    event->guid = guid; 
    event->name = name; 

    unsigned int ignored; 
    event->clock = __rdtscp(&ignored); 
    event->thread_id = GetThreadID();

    return event;
}

struct ProfileScopeHelper
{
    const char *guid, *name;
    ProfileScopeHelper(const char *guid_init, const char *name_init)
        : guid(guid_init)
        , name(name_init)
    {
        DebugRecordEvent(DebugEvent_BeginProfileZone, guid, name);
    }

    ~ProfileScopeHelper()
    {
        DebugRecordEvent(DebugEvent_EndProfileZone, guid, name);
    }
};
#define ProfileScope() ProfileScopeHelper MACRO_VAR(profile_scope)(LOCATION_STRING(), __FUNCTION__)

#else /* !DUNGEONS_INTERNAL */

#define ProfileTimestamp(...)
#define ProfileScope(...)

#endif /* DUNGEONS_INTERNAL */

#endif /* DUNGEONS_DEBUG_INTERFACE_HPP */
