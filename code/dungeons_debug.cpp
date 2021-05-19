#include "dungeons_debug.hpp"

static DebugEntry *
GetDebugEntry(DebugEvent *event)
{
    DebugEntry *result = &debug_state->null_entry;

    for (size_t i = 0; i < debug_state->entry_count; i += 1)
    {
        DebugEntry *entry = &debug_state->entries[i];
        if (AreEqual(entry->guid, event->guid))
        {
            result = entry;
            break;
        }
    }

    if (result == &debug_state->null_entry)
    {
        if (debug_state->entry_count < ArrayCount(debug_state->entries))
        {
            result = &debug_state->entries[debug_state->entry_count++];
            result->guid = event->guid;
            result->name = event->name;
        }
    }

    return result;
}

static DebugState *
DebugInit(void)
{
    DebugState *result = BootstrapPushStruct(DebugState, arena);
    return result;
}

void
AppDebugEndFrame(void)
{
    if (!platform->debug_state)
    {
        platform->debug_state = DebugInit();
    }
    debug_state = platform->debug_state;

    debug_table->current_buffer_index = !debug_table->current_buffer_index;
    uint32_t event_index__buffer_index = AtomicExchange(&debug_table->event_count__buffer_index,
                                                        debug_table->current_buffer_index << 31);
    uint32_t event_count  = event_index__buffer_index & 0x7FFFFFFF;
    uint32_t buffer_index = event_index__buffer_index >> 31;

    Assert(buffer_index <= 1);

    DebugEvent *events = debug_table->event_buffers[buffer_index];
    for (size_t i = 0; i < event_count; i += 1)
    {
        DebugEvent *event = &events[i];
        DebugEntry *entry = GetDebugEntry(event);

        switch (event->kind)
        {
            case DebugEvent_BeginProfileZone:
            {
                if (!debug_state->first_free_open_entry)
                {
                    debug_state->first_free_open_entry = PushStruct(&debug_state->arena, OpenDebugEntry);
                }
                OpenDebugEntry *open_entry = debug_state->first_free_open_entry;
                debug_state->first_free_open_entry = open_entry->next;

                open_entry->entry = entry;
                open_entry->begin_clock = event->clock;
                open_entry->begin_thread_id = event->thread_id;

                open_entry->next = debug_state->first_open_entry;
                debug_state->first_open_entry = open_entry;
            } break;

            case DebugEvent_EndProfileZone:
            {
                OpenDebugEntry *open_entry = debug_state->first_open_entry;
                debug_state->first_open_entry = open_entry->next;

                if (open_entry->begin_thread_id == event->thread_id)
                {
                    uint64_t clocks_elapsed = event->clock - open_entry->begin_clock;

                    entry->clocks += clocks_elapsed;
                    entry->hit_count += 1;

                    open_entry->next = debug_state->first_free_open_entry;
                    debug_state->first_free_open_entry = open_entry;
                }
                else
                {
                    // TODO: Handle multiple threads gracefully
                    INVALID_CODE_PATH;
                }
            } break;
        }
    }
}
