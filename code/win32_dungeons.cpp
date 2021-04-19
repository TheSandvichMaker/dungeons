#include "win32_dungeons.hpp"

#include <stdio.h>

#define MAX_PLATFORM_EVENTS 4096

static bool g_running;
static LARGE_INTEGER g_perf_freq;
static WINDOWPLACEMENT g_window_position = { sizeof(g_window_position) };

static Win32State g_win32_state;

static Platform platform_;

extern "C"
{
__declspec(dllexport) unsigned long NvOptimusEnablement        = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

static void *
Win32_Reserve(size_t size, uint32_t flags, const char *tag)
{
    Win32State *state = &g_win32_state;

    size_t page_size = platform->page_size;
    size_t total_size = page_size + size;

    Win32AllocationHeader *header = (Win32AllocationHeader *)VirtualAlloc(0, total_size, MEM_RESERVE, PAGE_NOACCESS);
    VirtualAlloc(header, page_size, MEM_COMMIT, PAGE_READWRITE);

    header->size = total_size;
    header->base = (char *)header + page_size;
    header->flags = flags;
    header->tag = tag;

    header->next = &state->allocation_sentinel;
    header->prev = state->allocation_sentinel.prev;
    header->next->prev = header;
    header->prev->next = header;

    return header->base;
}

static void *
Win32_Commit(void *Pointer, size_t Size)
{
    void *Result = VirtualAlloc(Pointer, Size, MEM_COMMIT, PAGE_READWRITE);
    return Result;
}

static void *
Win32_Allocate(size_t Size, uint32_t Flags, const char *Tag)
{
    void *Result = Win32_Reserve(Size, Flags, Tag);
    if (Result)
    {
        Result = Win32_Commit(Result, Size);
    }
    return Result;
}

static void
Win32_Decommit(void *pointer, size_t size)
{
    if (pointer)
    {
        VirtualFree(pointer, size, MEM_DECOMMIT);
    }
}

static void
Win32_Deallocate(void *pointer)
{
    if (pointer)
    {
        Win32AllocationHeader *header = (Win32AllocationHeader *)((char *)pointer - platform->page_size);
        header->prev->next = header->next;
        header->next->prev = header->prev;
        VirtualFree(header, 0, MEM_RELEASE);
    }
}

static inline wchar_t *
FormatLastError(void)
{
    wchar_t *message = NULL;
    DWORD error = GetLastError() & 0xFFFF;

    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER| 
                   FORMAT_MESSAGE_FROM_SYSTEM|
                   FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL,
                   error,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (wchar_t *)&message,
                   0, NULL);

    return message;
}

static inline void
DisplayLastError(void)
{
    wchar_t *message = FormatLastError();
    MessageBoxW(0, message, L"Error", MB_OK);
    LocalFree(message);
}

static inline void
ExitWithLastError(int exit_code = -1)
{
    DisplayLastError();
    ExitProcess(exit_code);
}

static inline void
ExitWithError(const wchar_t *message, int exit_code = -1)
{
    MessageBoxW(0, message, L"Fatal Error", MB_OK);
    ExitProcess(exit_code);
}

static inline wchar_t *
Win32_Utf8ToUtf16(Arena *arena, const char *utf8)
{
    wchar_t *result = nullptr;

    int wchar_count = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    result = PushArrayNoClear(arena, wchar_count, wchar_t);

    if (result)
    {
        if (!MultiByteToWideChar(CP_UTF8, 0, utf8, -1, result, wchar_count))
        {
            DisplayLastError();
        }
    }

    return result;
}

static inline char *
Win32_Utf16ToUtf8(Arena *arena, const wchar_t *utf16, int *out_length = nullptr)
{
    char *result = nullptr;

    int char_count = WideCharToMultiByte(CP_UTF8, 0, utf16, -1, nullptr, 0, nullptr, nullptr);
    result = PushArrayNoClear(arena, char_count, char);

    if (result)
    {
        if (!WideCharToMultiByte(CP_UTF8, 0, utf16, -1, result, char_count, nullptr, nullptr))
        {
            DisplayLastError();
        }

        if (out_length) *out_length = char_count - 1;
    }
    else
    {
        if (out_length) *out_length = 0;
    }

    return result;
}

static inline char *
FormatStringV(Arena *arena, char *fmt, va_list args_init)
{
    va_list args_size;
    va_copy(args_size, args_init);

    va_list args_fmt;
    va_copy(args_fmt, args_init);

    int chars_required = vsnprintf(nullptr, 0, fmt, args_size) + 1;
    va_end(args_size);

    char *result = PushArrayNoClear(arena, chars_required, char);
    vsnprintf(result, chars_required, fmt, args_fmt);
    va_end(args_fmt);

    return result;
}

static inline char *
FormatString(Arena *arena, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char *result = FormatStringV(arena, fmt, args);
    va_end(args);
    return result;
}

static inline void
DebugPrint(char *fmt, ...)
{
    ScopedMemory temp(&g_win32_state.temp_arena);

    va_list args;
    va_start(args, fmt);
    char *formatted = FormatStringV(&g_win32_state.temp_arena, fmt, args);
    va_end(args);

    wchar_t *fmt_wide = Win32_Utf8ToUtf16(&g_win32_state.temp_arena, formatted);

    OutputDebugStringW(fmt_wide);
}

static inline void
Win32_ReportError(PlatformErrorType type, char *error, ...)
{
    ScopedMemory temp(&g_win32_state.temp_arena);

    va_list args;
    va_start(args, error);
    char *formatted_error = FormatStringV(&g_win32_state.temp_arena, error, args);
    va_end(args);

    wchar_t *error_wide = Win32_Utf8ToUtf16(&g_win32_state.temp_arena, formatted_error);
    if (error_wide)
    {
        MessageBoxW(0, error_wide, L"Error", MB_OK);
    }

    if (type == PlatformError_Fatal)
    {
#if DUNGEONS_INTERNAL
        __debugbreak();
#else
        ExitProcess(-1);
#endif
    }
}

static Buffer
Win32_ReadFile(Arena *arena, const char *file)
{
    Buffer result = {};

    Win32State *state = &g_win32_state;

    ScopedMemory temp_memory(&state->temp_arena);
    wchar_t *file_wide = Win32_Utf8ToUtf16(&state->temp_arena, file);

    HANDLE handle = CreateFileW(file_wide, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (handle != INVALID_HANDLE_VALUE)
    {
        DWORD file_size_high;
        DWORD file_size_low = GetFileSize(handle, &file_size_high);
        // NOTE: Right now we're just doing 32 bit file IO.
        if (!file_size_high) {
            ScopedMemory temp_memory(arena);

            result.data = PushArrayNoClear(arena, file_size_low + 1, uint8_t);

            DWORD bytes_read;
            if (ReadFile(handle, result.data, file_size_low, &bytes_read, 0))
            {
                if (bytes_read == file_size_low)
                {
                    result.size = file_size_low;
                    result.data[file_size_low] = 0; // null terminate, just for convenience.
                    CommitTemporaryMemory(temp_memory);
                }
                else
                {
                    DebugPrint("Did not read expected number of bytes from file '%s'", file);
                }
            }
            else
            {
                DebugPrint("Could not read file '%s'", file);
            }
        }
    }
    else
    {
        DebugPrint("Could not open file '%s'", file);
    }

    return result;
}

static inline void
Win32_ToggleFullscreen(HWND window)
{
    // Raymond Chen's fullscreen toggle recipe:
    // http://blogs.msdn.com/b/oldnewthing/archive/2010/04/12/9994016.aspx
    DWORD style = GetWindowLong(window, GWL_STYLE);
    if (style & WS_OVERLAPPEDWINDOW) {
        MONITORINFO mi = { sizeof(mi) };
        if (GetWindowPlacement(window, &g_window_position) &&
            GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &mi)) {
            SetWindowLong(window, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(window, HWND_TOP,
                mi.rcMonitor.left, mi.rcMonitor.top,
                mi.rcMonitor.right - mi.rcMonitor.left,
                mi.rcMonitor.bottom - mi.rcMonitor.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    } else {
        SetWindowLong(window, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window, &g_window_position);
        SetWindowPos(window, NULL, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

static inline void
ResizeOffscreenBuffer(Bitmap *buffer, int32_t w, int32_t h)
{
    if (buffer->data &&
        w != buffer->w &&
        h != buffer->h)
    {
        Win32_Deallocate(buffer->data);
        ZeroStruct(buffer);
    }

    if (!buffer->data &&
        w > 0 &&
        h > 0)
    {
        buffer->data = (Color *)Win32_Allocate(Align16(sizeof(Color)*w*h), 0, LOCATION_STRING("Win32OffscreenBuffer"));
        buffer->w = w;
        buffer->h = h;
        buffer->pitch = w;
    }
}

static inline void
DisplayOffscreenBuffer(HWND window, Bitmap *buffer)
{
    HDC dc = GetDC(window);

    RECT client_rect;
    GetClientRect(window, &client_rect);

    int dst_w = client_rect.right;
    int dst_h = client_rect.bottom;

    BITMAPINFO bitmap_info = {};
    BITMAPINFOHEADER *bitmap_header = &bitmap_info.bmiHeader;
    bitmap_header->biSize = sizeof(*bitmap_header);
    bitmap_header->biWidth = buffer->w;
    bitmap_header->biHeight = buffer->h;
    bitmap_header->biPlanes = 1;
    bitmap_header->biBitCount = 32;
    bitmap_header->biCompression = BI_RGB;

    StretchDIBits(dc,
                  0, 0, dst_w, dst_h,
                  0, 0, buffer->w, buffer->h,
                  buffer->data,
                  &bitmap_info,
                  DIB_RGB_COLORS,
                  SRCCOPY);

    ReleaseDC(window, dc);
}

static LRESULT CALLBACK 
Win32_WindowProc(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
    LRESULT result = 0;
    switch (message)
    {
        case WM_CLOSE:
        case WM_DESTROY:
        {
            g_running = false;
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            BeginPaint(window, &paint);

            RECT client_rect;
            GetClientRect(window, &client_rect);

            platform->render_w = client_rect.right;
            platform->render_h = client_rect.bottom;

            ResizeOffscreenBuffer(&platform->backbuffer, platform->render_w, platform->render_h);

            App_UpdateAndRender(platform);
            DisplayOffscreenBuffer(window, &platform->backbuffer);

            EndPaint(window, &paint);
        } break;
        
        default:
        {
            result = DefWindowProcW(window, message, w_param, l_param);
        } break;
    }
    return result;
}

static HWND
Win32_CreateWindow(HINSTANCE instance, int x, int y, int w, int h, const wchar_t *title)
{
    int window_x = x;
    int window_y = y;
    int render_w = w;
    int render_h = h;

    RECT window_rect = { window_x, window_y, window_x + render_w, window_y + render_h };
    AdjustWindowRectEx(&window_rect, WS_OVERLAPPEDWINDOW, false, 0);

    int window_w = window_rect.right - window_rect.left;
    int window_h = window_rect.bottom - window_rect.top;

    WNDCLASSW window_class = {};
    window_class.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    window_class.lpfnWndProc = Win32_WindowProc;
    window_class.hInstance = instance;
    window_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    window_class.lpszClassName = L"Win32WindowClass";

    if (!RegisterClassW(&window_class))
    {
        ExitWithLastError();
    }

    HWND window_handle = CreateWindowW(window_class.lpszClassName,
                                       title,
                                       WS_OVERLAPPEDWINDOW,
                                       window_x, window_y,
                                       window_w, window_h,
                                       NULL, NULL, instance, NULL);

    if (!window_handle)
    {
        ExitWithLastError();
    }

    return window_handle;
}

static inline PlatformEvent *
PushEvent()
{
    Win32State *state = &g_win32_state;
    PlatformEvent *result = &state->dummy_platform_event;
    if (state->next_platform_event < state->max_platform_events)
    {
        result = &state->platform_event_buffer[state->next_platform_event++];
        if (platform->first_event)
        {
            platform->last_event = platform->last_event->next = result;
        }
        else
        {
            platform->first_event = platform->last_event = result;
        }
    }
    else
    {
        state->platform_event_underflow = true;
    }
    ZeroStruct(result);
    return result;
}

static bool
Win32_HandleSpecialKeys(HWND window, int vk_code, bool pressed, bool alt_is_down)
{
    bool processed = false;

    if (!pressed)
    {
        if (alt_is_down)
        {
            processed = true;
            switch (vk_code)
            {
                case VK_RETURN:
                {
                    Win32_ToggleFullscreen(window);
                } break;

                case VK_F4:
                {
                    g_running = false;
                } break;

                default:
                {
                    processed = false;
                } break;
            }
        }
    }

    return processed;
}

static PlatformHighResTime
Win32_GetTime(void)
{
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);

    PlatformHighResTime result;
    result.opaque = (uint64_t &)time.QuadPart;

    return result;
}

static double
Win32_SecondsElapsed(PlatformHighResTime start, PlatformHighResTime end)
{
    double result = (double)(end.opaque - start.opaque) / (double)g_perf_freq.QuadPart;
    return result;
}

int
WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show_cmd)
{
    platform = &platform_;

    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);

    QueryPerformanceFrequency(&g_perf_freq);

    Win32State *state = &g_win32_state;
    state->allocation_sentinel.next = &state->allocation_sentinel;
    state->allocation_sentinel.prev = &state->allocation_sentinel;

    platform->page_size = system_info.dwPageSize;
    platform->ReportError = Win32_ReportError;
    platform->AllocateMemory = Win32_Allocate;
    platform->ReserveMemory = Win32_Reserve;
    platform->CommitMemory = Win32_Commit;
    platform->DecommitMemory = Win32_Decommit;
    platform->DeallocateMemory = Win32_Deallocate;
    platform->GetTime = Win32_GetTime;
    platform->SecondsElapsed = Win32_SecondsElapsed;
    platform->ReadFile = Win32_ReadFile;

    state->max_platform_events = MAX_PLATFORM_EVENTS;
    state->next_platform_event = 0;
    state->platform_event_underflow = false;
    state->platform_event_buffer = (PlatformEvent *)Win32_Allocate(sizeof(PlatformEvent)*state->max_platform_events,
                                                                   PlatformMemFlag_NoLeakCheck,
                                                                   LOCATION_STRING("Win32Events"));

    HCURSOR arrow_cursor = LoadCursorW(nullptr, IDC_ARROW);
    HWND window = Win32_CreateWindow(instance, 32, 32, 1280, 720, L"Dungeons");
    if (!window)
    {
        ExitWithError(L"Could not create window");
    }

    ShowWindow(window, show_cmd);

    BOOL composition_enabled;
    if (DwmIsCompositionEnabled(&composition_enabled) != S_OK)
    {
        DisplayLastError();
    }

    double smooth_frametime = 1.0f / 60.0f;
    PlatformHighResTime frame_start_time = Win32_GetTime();

    platform->dt = 1.0f / 60.0f;

    g_running = true;
    while (g_running)
    {
        bool exit_requested = false;
        platform->first_event = platform->last_event = nullptr;

        MSG message;
        while (PeekMessageW(&message, 0, 0, 0, PM_REMOVE))
        {
            switch (message.message)
            {
                case WM_CLOSE:
                case WM_QUIT:
                {
                    exit_requested = true;
                } break;

                case WM_KEYUP:
                case WM_KEYDOWN:
                case WM_SYSKEYUP:
                case WM_SYSKEYDOWN:
                {
					int vk_code = (int)message.wParam;
					bool pressed = (message.lParam & (1ull << 31)) == 0;
                    bool released = (message.lParam & (1 << 30)) != 0;
                    bool alt_is_down = (message.lParam & (1 << 29)) != 0;

                    if (!Win32_HandleSpecialKeys(window, vk_code, pressed, alt_is_down))
                    {
                        PlatformEvent *event = PushEvent();
                        event->type = (pressed ? PlatformEvent_KeyDown : PlatformEvent_KeyUp);
                        event->pressed = pressed;
                        event->key_code = (PlatformKeyCode)vk_code;

                        TranslateMessage(&message);
                    }
                } break;

                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP:
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                {
                    PlatformEvent *event = PushEvent();
                    event->pressed = (message.message == WM_LBUTTONDOWN ||
                                      message.message == WM_MBUTTONDOWN ||
                                      message.message == WM_RBUTTONDOWN);
                    event->type = (event->pressed ? PlatformEvent_MouseDown : PlatformEvent_MouseUp);
                    switch (message.message)
                    {
                        case WM_LBUTTONDOWN: case WM_LBUTTONUP:
                        {
                            event->mouse_button = PlatformMouseButton_Left;
                        } break;
                        case WM_MBUTTONDOWN: case WM_MBUTTONUP:
                        {
                            event->mouse_button = PlatformMouseButton_Middle;
                        } break;
                        case WM_RBUTTONDOWN: case WM_RBUTTONUP:
                        {
                            event->mouse_button = PlatformMouseButton_Right;
                        } break;
                    }
                } break;

                case WM_CHAR:
                {
                    PlatformEvent *event = PushEvent();
                    wchar_t buf[] = { (wchar_t)message.wParam, 0 };
                    event->type = PlatformEvent_Text;
                    event->text = Win32_Utf16ToUtf8(&state->temp_arena, buf, &event->text_length);
                } break;

                default:
                {
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                } break;
            }
        }

        RECT client_rect;
        GetClientRect(window, &client_rect);

        int32_t client_w = client_rect.right;
        int32_t client_h = client_rect.bottom;

        platform->render_w = client_w;
        platform->render_h = client_h;

        POINT cursor_pos;
        GetCursorPos(&cursor_pos);
        ScreenToClient(window, &cursor_pos);

        platform->mouse_x = cursor_pos.x;
        platform->mouse_y = client_h - cursor_pos.y - 1;
        platform->mouse_y_flipped = cursor_pos.y;
        platform->mouse_in_window = (platform->mouse_x >= 0 && platform->mouse_x < client_w &&
                                     platform->mouse_y >= 0 && platform->mouse_y < client_h);

        if (platform->mouse_in_window)
        {
            SetCursor(arrow_cursor);
        }

        ResizeOffscreenBuffer(&platform->backbuffer,
                              platform->render_w,
                              platform->render_h);

        Bitmap *buffer = &platform->backbuffer;

        App_UpdateAndRender(platform);
        DisplayOffscreenBuffer(window, buffer);

        if (composition_enabled)
        {
            DwmFlush();
        }

        PlatformHighResTime frame_end_time = Win32_GetTime();
        double seconds_elapsed = Win32_SecondsElapsed(frame_start_time, frame_end_time);
        Swap(frame_start_time, frame_end_time);

        platform->dt = (float)seconds_elapsed;
        if (platform->dt > 1.0f / 15.0f)
        {
            platform->dt = 1.0f / 15.0f;
        }

        if (exit_requested)
        {
            g_running = false;
        }
    }

    ResizeOffscreenBuffer(&platform->backbuffer, 0, 0);

    bool leaked_memory = false;
    for (Win32AllocationHeader *header = state->allocation_sentinel.next;
         header != &state->allocation_sentinel;
         header = header->next)
    {
        DebugPrint("Allocated Block, Size: %llu, Tag: %s, NoLeakCheck: %s\n",
                   header->size,
                   header->tag,
                   (header->flags & PlatformMemFlag_NoLeakCheck ? "true" : "false"));
        if (!(header->flags & PlatformMemFlag_NoLeakCheck))
        {
            leaked_memory = true;
        }
    }

    if (leaked_memory)
    {
        Win32_ReportError(PlatformError_Nonfatal, "Potential Memory Leak Detected");
    }
}
