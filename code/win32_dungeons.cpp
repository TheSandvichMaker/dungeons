#include "win32_dungeons.hpp"

#include "glad.h"
#include "glad.c"

#include <stdio.h>

static bool g_running;
static LARGE_INTEGER g_perf_freq;
static WINDOWPLACEMENT g_window_position = { sizeof(g_window_position) };

extern "C"
{
__declspec(dllexport) unsigned long NvOptimusEnablement        = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

static inline WCHAR *
FormatLastError(void)
{
    WCHAR *message = NULL;
    DWORD error = GetLastError() & 0xFFFF;

    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER| 
                   FORMAT_MESSAGE_FROM_SYSTEM|
                   FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL,
                   error,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   (WCHAR *)&message,
                   0, NULL);

    return message;
}

static inline void
DisplayLastError(void)
{
    WCHAR *message = FormatLastError();
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
ExitWithError(const WCHAR *message, int exit_code = -1)
{
    MessageBoxW(0, message, L"Fatal Error", MB_OK);
    ExitProcess(exit_code);
}

static inline WCHAR *
Win32_Utf8ToUtf16(char *utf8)
{
    WCHAR *result = nullptr;

    int wchar_count = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    result = (WCHAR *)LocalAlloc(0, sizeof(WCHAR)*wchar_count);

    if (result)
    {
        if (!MultiByteToWideChar(CP_UTF8, 0, utf8, -1, result, wchar_count))
        {
            DisplayLastError();
        }
    }

    return result;
}

static inline void
Win32_ReportError(PlatformErrorType type, char *error, ...)
{
    // TODO: Handle varargs

    WCHAR *error_wide = Win32_Utf8ToUtf16(error);
    if (error_wide)
    {
        MessageBoxW(0, error_wide, L"Error", MB_OK);
    }
    LocalFree(error_wide);

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

static HWND
Win32_CreateWindow(HINSTANCE instance, int x, int y, int w, int h, const WCHAR *title)
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
    window_class.lpfnWndProc = DefWindowProcW;
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

static inline void
ResizeOffscreenBuffer(PlatformOffscreenBuffer *buffer, int32_t w, int32_t h)
{
    if (w != buffer->w ||
        h != buffer->h)
    {
        if (buffer->data)
        {
            VirtualFree(buffer->data, 0, MEM_RELEASE);
        }

        buffer->data = (Color *)VirtualAlloc(nullptr, sizeof(Color)*w*h, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
        buffer->w = w;
        buffer->h = h;
    }
}

static Platform platform_;
Platform *platform = &platform_;

int
WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR command_line, int show_cmd)
{
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);

    platform->page_size = system_info.dwPageSize;
    platform->ReportError = Win32_ReportError;

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
        bool exit_requested = false;

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

                    if (released && (vk_code == VK_ESCAPE))
                    {
                        exit_requested = true;
                    }
                } break;

                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP:
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:

                default:
                {
                    TranslateMessage(&message);
                    DispatchMessageW(&message);
                } break;
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

        BITMAPINFO bitmap_info = {};
        BITMAPINFOHEADER *bitmap_header = &bitmap_info.bmiHeader;
        bitmap_header->biSize = sizeof(*bitmap_header);
        bitmap_header->biWidth = buffer->w;
        bitmap_header->biHeight = buffer->h;
        bitmap_header->biPlanes = 1;
        bitmap_header->biBitCount = 32;
        bitmap_header->biCompression = BI_RGB;

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

        HDC dc = GetDC(window);

        StretchDIBits(dc,
                      0, 0, buffer->w, buffer->h,
                      0, 0, buffer->w, buffer->h,
                      buffer->data,
                      &bitmap_info,
                      DIB_RGB_COLORS,
                      SRCCOPY);

        ReleaseDC(window, dc);

        if (composition_enabled)
        {
            DwmFlush();
        }
    }
}
