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
};

struct Win32State
{
    Arena arena;
    Arena temp_arena;

    wchar_t *exe_folder;
    wchar_t *dll_path;

    Win32AllocationHeader allocation_sentinel;
};

#endif /* WIN32_DUNGEONS_HPP */
