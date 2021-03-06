#ifndef WIN32_DUNGEONS_HPP
#define WIN32_DUNGEONS_HPP

#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
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

struct Win32AppCode
{
    bool valid;

    HMODULE dll;
    uint64_t last_write_time;

    AppUpdateAndRenderType *UpdateAndRender;
#if DUNGEONS_INTERNAL
    AppDebugEndFrameType *DebugEndFrame;
#endif
};

struct PlatformJobEntry
{
    PlatformJobProc *proc;
    void *args;
};

struct PlatformJobQueue
{
    HANDLE stop;
    HANDLE done;
    HANDLE run;

    int thread_count;
    HANDLE *threads;
    ThreadLocalContext *tls;

    volatile uint32_t jobs_in_flight;
    volatile uint32_t next_write;
    volatile uint32_t next_read;
    PlatformJobEntry jobs[256];

    StaticAssert(IsPow2(ArrayCount(jobs)), "Jobs array must be a power of 2");
};

struct ThreadLocalContext
{
    ThreadLocalContext *next;

    Arena *temp_arena;
    Arena *prev_temp_arena;

    Arena temp_arena_1_;
    Arena temp_arena_2_;

    HANDLE pause_event;
};

struct Win32State
{
    Arena arena;
    Arena temp_arena;

    wchar_t *exe_folder;
    wchar_t *dll_path;
    Win32AppCode app_code;

    DWORD thread_local_index;

    Win32AllocationHeader allocation_sentinel;
    ThreadLocalContext *first_thread_local_context;

    TicketMutex log_mutex;
    int log_line_count;
    int log_line_first;
    PlatformLogLine log_lines[PLATFORM_MAX_LOG_LINES];
};

#endif /* WIN32_DUNGEONS_HPP */
