#ifndef WIN32_DUNGEONS_HPP
#define WIN32_DUNGEONS_HPP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dwmapi.h>

#include "dungeons_platform.hpp"
#include "dungeons_shared.hpp"
#include "dungeons_memory.hpp"

struct Win32AllocationHeader
{
    Win32AllocationHeader *next, *prev;
    size_t size;
    char *base;
    uint32_t flags;
    const char *tag;
};

struct Win32State
{
    uint32_t max_platform_events;
    uint32_t next_platform_event;
    bool platform_event_underflow;
    PlatformEvent dummy_platform_event;
    PlatformEvent *platform_event_buffer;

    Win32AllocationHeader allocation_sentinel;
};

#endif /* WIN32_DUNGEONS_HPP */
