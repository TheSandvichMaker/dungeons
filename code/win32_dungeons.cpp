#include "win32_dungeons.hpp"

#include <stdio.h>

#define MAX_PLATFORM_EVENTS 4096

static bool g_running;
static LARGE_INTEGER g_perf_freq;
static WINDOWPLACEMENT g_window_position = { sizeof(g_window_position) };

static Win32State g_win32_state;

static Platform platform_;
Platform *platform = &platform_;

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

static void *temp_buffer;
static size_t temp_at;
static size_t temp_committed;

static inline size_t
GetTempMarker(void)
{
    return temp_at;
}

static inline void
ResetTemp(size_t to)
{
    Assert(to <= temp_at);
    temp_at = to;
}

static inline void *
PushTempMemory(size_t size_init)
{
    const size_t buffer_size = Gigabytes(8);

    size_t size = Align16(size_init);
    Assert((temp_at + size) <= buffer_size);

    if (!temp_buffer)
    {
        temp_buffer = Win32_Reserve(buffer_size, PlatformMemFlag_NoLeakCheck, LOCATION_STRING("Win32TempMemory"));
        Assert(temp_buffer);
    }

    if (temp_committed < size)
    {
        size_t to_commit = AlignPow2(size - temp_committed, platform->page_size);
        temp_buffer = Win32_Commit((char *)temp_buffer + temp_at, to_commit);
        Assert(temp_buffer);

        temp_committed += to_commit;
    }

    void *result = (char *)temp_buffer + temp_at;
    temp_at += size;

    return result;
}

struct ScopedTempMemory
{
    size_t marker;
    ScopedTempMemory()
    {
        marker = GetTempMarker();
    }

    ~ScopedTempMemory()
    {
        ResetTemp(marker);
    }
};

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
Win32_TempUtf8ToUtf16(char *utf8)
{
    wchar_t *result = nullptr;

    int wchar_count = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    result = (wchar_t *)PushTempMemory(sizeof(wchar_t)*wchar_count);

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
Win32_TempUtf16ToUtf8(wchar_t *utf16)
{
    char *result = nullptr;

    int char_count = WideCharToMultiByte(CP_UTF8, 0, utf16, -1, nullptr, 0, nullptr, nullptr);
    result = (char *)PushTempMemory(char_count);

    if (result)
    {
        if (!WideCharToMultiByte(CP_UTF8, 0, utf16, -1, result, char_count, nullptr, nullptr))
        {
            DisplayLastError();
        }
    }

    return result;
}

static inline char *
FormatTempStringV(char *fmt, va_list args_init)
{
    va_list args_size;
    va_copy(args_size, args_init);

    va_list args_fmt;
    va_copy(args_fmt, args_init);

    int chars_required = vsnprintf(nullptr, 0, fmt, args_size) + 1;
    va_end(args_size);

    size_t temp_marker = GetTempMarker();

    char *result = (char *)PushTempMemory(chars_required);
    vsnprintf(result, chars_required, fmt, args_fmt);
    va_end(args_fmt);

    return result;
}

static inline char *
FormatTempString(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char *result = FormatTempStringV(fmt, args);
    va_end(args);
    return result;
}

static inline void
DebugPrint(char *fmt, ...)
{
    ScopedTempMemory temp;

    va_list args;
    va_start(args, fmt);
    char *formatted = FormatTempStringV(fmt, args);
    va_end(args);

    wchar_t *fmt_wide = Win32_TempUtf8ToUtf16(formatted);

    OutputDebugStringW(fmt_wide);
}

static inline void
Win32_ReportError(PlatformErrorType type, char *error, ...)
{
    ScopedTempMemory temp;

    va_list args;
    va_start(args, error);
    char *formatted_error = FormatTempStringV(error, args);
    va_end(args);

    wchar_t *error_wide = Win32_TempUtf8ToUtf16(formatted_error);
    if (error_wide)
    {
        MessageBoxW(0, error_wide, L"Error", MB_OK);
    }

    if (type == PlatformError_Fatal)
    {
        ExitProcess(-1);
    }
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
ResizeOffscreenBuffer(PlatformOffscreenBuffer *buffer, int32_t w, int32_t h)
{
    if (buffer->data &&
        w != buffer->w &&
        h != buffer->h)
    {
        Win32_Deallocate(buffer->data);
    }

    if (!buffer->data &&
        w > 0 &&
        h > 0)
    {
        buffer->data = (Color *)Win32_Allocate(sizeof(Color)*w*h, 0, LOCATION_STRING("Win32OffscreenBuffer"));
        buffer->w = w;
        buffer->h = h;
    }
}

static inline void
DrawTestPattern(PlatformOffscreenBuffer *buffer)
{
    static int frame_counter = 0;
    for (int y = 0; y < buffer->h; ++y)
    for (int x = 0; x < buffer->w; ++x)
    {
        Color *pixel = &buffer->data[y*buffer->w + x];
        pixel->r = (x + frame_counter) & 255;
        pixel->g = (y + frame_counter) & 255;
        pixel->b = 0;
        pixel->a = 255;
    }
    ++frame_counter;
}

static inline void
DisplayOffscreenBuffer(HWND window, PlatformOffscreenBuffer *buffer)
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

            DrawTestPattern(&platform->backbuffer);
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

    HCURSOR ArrowCursor = LoadCursorW(nullptr, IDC_ARROW);
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

int
WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show_cmd)
{
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);

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

    state->max_platform_events = MAX_PLATFORM_EVENTS;
    state->next_platform_event = 0;
    state->platform_event_underflow = false;
    state->platform_event_buffer = (PlatformEvent *)Win32_Allocate(sizeof(PlatformEvent)*state->max_platform_events, PlatformMemFlag_NoLeakCheck, LOCATION_STRING("Win32Events"));

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

    g_running = true;
    while (g_running)
    {
        ResetTemp(0);

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

                    if (pressed && alt_is_down && (vk_code == VK_RETURN))
                    {
                        Win32_ToggleFullscreen(window);
                    }

                    if (released && alt_is_down && (vk_code == VK_F4))
                    {
                        exit_requested = true;
                    }

                    PlatformEvent *event = PushEvent();
                    event->type = (pressed ? PlatformEvent_KeyDown : PlatformEvent_KeyUp);
                    event->pressed = pressed;
                    event->key_code = (PlatformKeyCode)vk_code;

                    TranslateMessage(&message);
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
                    event->text = Win32_TempUtf16ToUtf8(buf);
                } break;

                default:
                {
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                } break;
            }
        }

        for (PlatformEvent *event = nullptr; (event = PopEvent(platform));)
        {
            switch (event->type)
            {
                case PlatformEvent_MouseUp:
                case PlatformEvent_MouseDown:
                {
                    DebugPrint("Mouse Button %s was %s.\r\n",
                               PlatformMouseButton_Name[event->mouse_button],
                               event->pressed ? "pressed" : "released");
                } break;

                case PlatformEvent_KeyUp:
                case PlatformEvent_KeyDown:
                {
                    DebugPrint("Key 0x%X was %s.\r\n",
                               event->key_code,
                               event->pressed ? "pressed" : "released");
                } break;

                case PlatformEvent_Text:
                {
                    DebugPrint("%s", event->text);
                } break;

                default: break;
            }
        }

        RECT client_rect;
        GetClientRect(window, &client_rect);

        platform->render_w = client_rect.right;
        platform->render_h = client_rect.bottom;

        ResizeOffscreenBuffer(&platform->backbuffer,
                              platform->render_w,
                              platform->render_h);

        PlatformOffscreenBuffer *buffer = &platform->backbuffer;
        DrawTestPattern(buffer);
        DisplayOffscreenBuffer(window, buffer);

        if (composition_enabled)
        {
            DwmFlush();
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
