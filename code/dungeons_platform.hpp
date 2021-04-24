#ifndef DUNGEONS_PLATFORM_HPP
#define DUNGEONS_PLATFORM_HPP

#if DUNGEONS_BUILD_DLL
#define DUNGEONS_EXPORT __declspec(dllexport)
#else
#define DUNGEONS_EXPORT
#endif

#define SimpleAssert(x) ((x) ? 1 : (__debugbreak(), 0))
#define Assert(x) \
    ((x) ? 1 \
         : (platform->ReportError(PlatformError_Fatal, \
                                  "Assertion Failed: " #x " at file %s, line %d", __FILE__, __LINE__), 0))

// should this be inline or static inline
#define DUNGEONS_INLINE inline

#ifdef DEBUG_BUILD
#define AssertSlow(x) Assert(x)
#else
#define AssertSlow(x) 
#endif

#define StaticAssert(condition, message) static_assert(condition, message)

#define UNUSED_VARIABLE(x) (void)(x)

#define FILE_AND_LINE_STRING__(File, Line) File ":" #Line
#define FILE_AND_LINE_STRING_(File, Line) FILE_AND_LINE_STRING__(File, Line)
#define FILE_AND_LINE_STRING FILE_AND_LINE_STRING_(__FILE__, __LINE__)
#define LOCATION_STRING(...) FILE_AND_LINE_STRING " (" __VA_ARGS__ ")"

#define INVALID_CODE_PATH Assert(!"Invalid Code Path!");
#define INVALID_DEFAULT_CASE default: { Assert(!"Invalid Default Case!"); } break;
#define INCOMPLETE_SWITCH default: { /* nah */ } break;

#define Swap(a, b) do { auto swap_temp_ = a; a = b; b = swap_temp_; } while(0)

#define Paste__(a, b) a##b
#define Paste_(a, b) Paste__(a, b)
#define Paste(a, b) Paste_(a, b)
#define Stringize__(x) #x
#define Stringize_(x) Stringize__(x)
#define Stringize(x) Stringize_(x)
#define Expand_(x) x
#define Expand(x) Expand(x)

#define BitIsSet(mask, bit) ((mask) & ((u64)1 << bit))
#define SetBit(mask, bit)   ((mask) |= ((u64)1 << bit))
#define UnsetBit(mask, bit) ((mask) &= ~((u64)1 << bit))

#define AlignPow2(value, align) (((value) + ((align) - 1)) & ~((align) - 1))
#define Align4(value) ((value + 3) & ~3)
#define Align8(value) ((value + 7) & ~7)
#define Align16(value) ((value + 15) & ~15)

#define Kilobytes(x) ((x)*1024ull)
#define Megabytes(x) (Kilobytes(x)*1024ull)
#define Gigabytes(x) (Megabytes(x)*1024ull)
#define Terabytes(x) (Gigabytes(x)*1024ull)

#include "dungeons_types.hpp"
#include "dungeons_intrinsics.hpp"

enum PlatformEventType
{
    PlatformEvent_None,
    PlatformEvent_Any = PlatformEvent_None,
    PlatformEvent_MouseUp,
    PlatformEvent_MouseDown,
    PlatformEvent_KeyUp,
    PlatformEvent_KeyDown,
    PlatformEvent_Text,
    PlatformEvent_COUNT,
};

typedef uint32_t PlatformEventFilter;
enum PlatformEventFilter_ENUM
{
    PlatformEventFilter_MouseUp   = 1 << 0,
    PlatformEventFilter_MouseDown = 1 << 1,
    PlatformEventFilter_Mouse     = PlatformEventFilter_MouseUp|PlatformEventFilter_MouseDown,
    PlatformEventFilter_KeyUp     = 1 << 2,
    PlatformEventFilter_KeyDown   = 1 << 3,
    PlatformEventFilter_Keyboard  = PlatformEventFilter_KeyUp|PlatformEventFilter_KeyDown,
    PlatformEventFilter_Text      = 1 << 4,
    PlatformEventFilter_ANY       = 0xFFFFFFFF,
};

static inline bool
MatchFilter(PlatformEventType type, PlatformEventFilter filter)
{
    bool result = !!(filter & (1 << (type - 1)));
    return result;
}

enum PlatformMouseButton
{
    PlatformMouseButton_None,

    PlatformMouseButton_Left,
    PlatformMouseButton_Middle,
    PlatformMouseButton_Right,

    PlatformMouseButton_COUNT,
};

enum PlatformKeyCode
{
    PlatformKeyCode_None           = 0x0,
    PlatformKeyCode_LButton        = 0x1,
    PlatformKeyCode_RButton        = 0x2,
    PlatformKeyCode_Cancel         = 0x3,
    PlatformKeyCode_MButton        = 0x4,
    PlatformKeyCode_XButton1       = 0x5,
    PlatformKeyCode_XButton2       = 0x6,
    PlatformKeyCode_Back           = 0x8,
    PlatformKeyCode_Tab            = 0x9,
    PlatformKeyCode_Clear          = 0xC,
    PlatformKeyCode_Return         = 0xD,
    PlatformKeyCode_Shift          = 0x10,
    PlatformKeyCode_Control        = 0x11,
    PlatformKeyCode_Alt            = 0x12,
    PlatformKeyCode_Pause          = 0x13,
    PlatformKeyCode_CapsLock       = 0x14,
    PlatformKeyCode_Kana           = 0x15,
    PlatformKeyCode_Hangul         = 0x15,
    PlatformKeyCode_Junja          = 0x17,
    PlatformKeyCode_Final          = 0x18,
    PlatformKeyCode_Hanja          = 0x19,
    PlatformKeyCode_Kanji          = 0x19,
    PlatformKeyCode_Escape         = 0x1B,
    PlatformKeyCode_Convert        = 0x1C,
    PlatformKeyCode_NonConvert     = 0x1D,
    PlatformKeyCode_Accept         = 0x1E,
    PlatformKeyCode_ModeChange     = 0x1F,
    PlatformKeyCode_Space          = 0x20,
    PlatformKeyCode_PageUp         = 0x21,
    PlatformKeyCode_PageDown       = 0x22,
    PlatformKeyCode_End            = 0x23,
    PlatformKeyCode_Home           = 0x24,
    PlatformKeyCode_Left           = 0x25,
    PlatformKeyCode_Up             = 0x26,
    PlatformKeyCode_Right          = 0x27,
    PlatformKeyCode_Down           = 0x28,
    PlatformKeyCode_Select         = 0x29,
    PlatformKeyCode_Print          = 0x2A,
    PlatformKeyCode_Execute        = 0x2B,
    PlatformKeyCode_PrintScreen    = 0x2C,
    PlatformKeyCode_Insert         = 0x2D,
    PlatformKeyCode_Delete         = 0x2E,
    PlatformKeyCode_Help           = 0x2F,
    PlatformKeyCode_0              = '0',
    PlatformKeyCode_1              = '1',
    PlatformKeyCode_2              = '2',
    PlatformKeyCode_3              = '3',
    PlatformKeyCode_4              = '4',
    PlatformKeyCode_5              = '5',
    PlatformKeyCode_6              = '6',
    PlatformKeyCode_7              = '7',
    PlatformKeyCode_8              = '8',
    PlatformKeyCode_9              = '9',
    /* 0x3A - 0x40: undefined */
    PlatformKeyCode_A              = 'A',
    PlatformKeyCode_B              = 'B',
    PlatformKeyCode_C              = 'C',
    PlatformKeyCode_D              = 'D',
    PlatformKeyCode_E              = 'E',
    PlatformKeyCode_F              = 'F',
    PlatformKeyCode_G              = 'G',
    PlatformKeyCode_H              = 'H',
    PlatformKeyCode_I              = 'I',
    PlatformKeyCode_J              = 'J',
    PlatformKeyCode_K              = 'K',
    PlatformKeyCode_L              = 'L',
    PlatformKeyCode_M              = 'M',
    PlatformKeyCode_N              = 'N',
    PlatformKeyCode_O              = 'O',
    PlatformKeyCode_P              = 'P',
    PlatformKeyCode_Q              = 'Q',
    PlatformKeyCode_R              = 'R',
    PlatformKeyCode_S              = 'S',
    PlatformKeyCode_T              = 'T',
    PlatformKeyCode_U              = 'U',
    PlatformKeyCode_V              = 'V',
    PlatformKeyCode_W              = 'W',
    PlatformKeyCode_X              = 'X',
    PlatformKeyCode_Y              = 'Y',
    PlatformKeyCode_Z              = 'Z',
    PlatformKeyCode_LSys           = 0x5B,
    PlatformKeyCode_RSys           = 0x5C,
    PlatformKeyCode_Apps           = 0x5D,
    PlatformKeyCode_Sleep          = 0x5f,
    PlatformKeyCode_Numpad0        = 0x60,
    PlatformKeyCode_Numpad1        = 0x61,
    PlatformKeyCode_Numpad2        = 0x62,
    PlatformKeyCode_Numpad3        = 0x63,
    PlatformKeyCode_Numpad4        = 0x64,
    PlatformKeyCode_Numpad5        = 0x65,
    PlatformKeyCode_Numpad6        = 0x66,
    PlatformKeyCode_Numpad7        = 0x67,
    PlatformKeyCode_Numpad8        = 0x68,
    PlatformKeyCode_Numpad9        = 0x69,
    PlatformKeyCode_Multiply       = 0x6A,
    PlatformKeyCode_Add            = 0x6B,
    PlatformKeyCode_Separator      = 0x6C,
    PlatformKeyCode_Subtract       = 0x6D,
    PlatformKeyCode_Decimal        = 0x6E,
    PlatformKeyCode_Divide         = 0x6f,
    PlatformKeyCode_F1             = 0x70,
    PlatformKeyCode_F2             = 0x71,
    PlatformKeyCode_F3             = 0x72,
    PlatformKeyCode_F4             = 0x73,
    PlatformKeyCode_F5             = 0x74,
    PlatformKeyCode_F6             = 0x75,
    PlatformKeyCode_F7             = 0x76,
    PlatformKeyCode_F8             = 0x77,
    PlatformKeyCode_F9             = 0x78,
    PlatformKeyCode_F10            = 0x79,
    PlatformKeyCode_F11            = 0x7A,
    PlatformKeyCode_F12            = 0x7B,
    PlatformKeyCode_F13            = 0x7C,
    PlatformKeyCode_F14            = 0x7D,
    PlatformKeyCode_F15            = 0x7E,
    PlatformKeyCode_F16            = 0x7F,
    PlatformKeyCode_F17            = 0x80,
    PlatformKeyCode_F18            = 0x81,
    PlatformKeyCode_F19            = 0x82,
    PlatformKeyCode_F20            = 0x83,
    PlatformKeyCode_F21            = 0x84,
    PlatformKeyCode_F22            = 0x85,
    PlatformKeyCode_F23            = 0x86,
    PlatformKeyCode_F24            = 0x87,
    PlatformKeyCode_Numlock        = 0x90,
    PlatformKeyCode_Scroll         = 0x91,
    PlatformKeyCode_LShift         = 0xA0,
    PlatformKeyCode_RShift         = 0xA1,
    PlatformKeyCode_LControl       = 0xA2,
    PlatformKeyCode_RControl       = 0xA3,
    PlatformKeyCode_LAlt           = 0xA4,
    PlatformKeyCode_RAlt           = 0xA5,
    /* 0xA6 - 0xAC: browser keys, not sure what's up with that */
    PlatformKeyCode_VolumeMute     = 0xAD,
    PlatformKeyCode_VolumeDown     = 0xAE,
    PlatformKeyCode_VolumeUp       = 0xAF,
    PlatformKeyCode_MediaNextTrack = 0xB0,
    PlatformKeyCode_MediaPrevTrack = 0xB1,
    /* 0xB5 - 0xB7: "launch" keys, not sure what's up with that */
    PlatformKeyCode_Oem1           = 0xBA, // misc characters, us standard: ';:'
    PlatformKeyCode_Plus           = 0xBB,
    PlatformKeyCode_Comma          = 0xBC,
    PlatformKeyCode_Minus          = 0xBD,
    PlatformKeyCode_Period         = 0xBE,
    PlatformKeyCode_Oem2           = 0xBF, // misc characters, us standard: '/?'
    PlatformKeyCode_Oem3           = 0xC0, // misc characters, us standard: '~'
    /* 0xC1 - 0xDA: reserved / unassigned */
    /* 0xDB - 0xF5: more miscellanious OEM codes I'm ommitting for now */
    /* 0xF6 - 0xF9: keys I've never heard of */
    PlatformKeyCode_Play           = 0xFA,
    PlatformKeyCode_Zoom           = 0xFB,
    PlatformKeyCode_OemClear       = 0xFE,
    PlatformKeyCode_COUNT,
};

struct PlatformEvent
{
    PlatformEvent *next;

    PlatformEventType type;

    bool pressed;
    bool alt_down;
    bool ctrl_down;
    bool shift_down;

    PlatformMouseButton mouse_button;
    PlatformKeyCode key_code;

    int text_length;
    char *text;

    bool consumed_;
};

enum PlatformErrorType
{
    PlatformError_Fatal,
    PlatformError_Nonfatal,
};

enum PlatformMemFlag
{
    PlatformMemFlag_NoLeakCheck = 0x1,
};

struct PlatformHighResTime
{
    uint64_t opaque;
};

struct ThreadLocalContext
{
    Arena *temp_arena;
    Arena *prev_temp_arena;

    Arena temp_arena_1_;
    Arena temp_arena_2_;
};

struct PlatformJobQueue;
#define PLATFORM_JOB(name) void name(void *args)
typedef PLATFORM_JOB(PlatformJobProc);

struct Platform
{
    bool exit_requested;

    bool app_initialized;
    bool exe_reloaded;
    void *persistent_app_data;

    float dt;

    PlatformEvent *first_event;
    PlatformEvent *last_event;

    int32_t mouse_x, mouse_y, mouse_y_flipped;
    int32_t mouse_dx, mouse_dy;
    int32_t mouse_in_window;

    int32_t render_w, render_h;
    Bitmap backbuffer;

    void (*DebugPrint)(char *message, ...);
    void (*ReportError)(PlatformErrorType type, char *message, ...);

    size_t page_size;
    PlatformJobQueue *job_queue;

    void *(*AllocateMemory)(size_t size, uint32_t flags, const char *tag);
    void *(*ReserveMemory)(size_t size, uint32_t flags, const char *tag);
    void *(*CommitMemory)(void *location, size_t size);
    void (*DecommitMemory)(void *location, size_t size);
    void (*DeallocateMemory)(void *memory);

    ThreadLocalContext *(*GetThreadLocalContext)(void);

    void (*AddJob)(PlatformJobQueue *queue, void *arg, PlatformJobProc *proc);
    void (*WaitForJobs)(PlatformJobQueue *queue);

    Buffer (*ReadFile)(Arena *arena, String filename);

    PlatformHighResTime (*GetTime)(void);
    double (*SecondsElapsed)(PlatformHighResTime start, PlatformHighResTime end);
};

static Platform *platform;

static inline Arena *
GetTempArena(void)
{
    ThreadLocalContext *context = platform->GetThreadLocalContext();
    Arena *result = context->temp_arena;
    return result;
}

static inline void
LeaveUnhandled(PlatformEvent *event)
{
    event->consumed_ = false;
}

static inline PlatformEvent *
PopEvent(PlatformEventFilter filter = PlatformEventFilter_ANY)
{
    PlatformEvent *it = platform->first_event;
    while (it->consumed_ || MatchFilter(it->type, filter))
    {
        it = it->next;
    }
    if (it)
    {
        it->consumed_ = true;
    }
    return it;
}

static inline bool
NextEvent(PlatformEvent **out_event, PlatformEventFilter filter = PlatformEventFilter_ANY)
{
    bool result = false;

    PlatformEvent *it = *out_event;
    if (!it)
    {
        it = platform->first_event;
    }
    else
    {
        it = it->next;
    }

    while (it)
    {
        if (!it->consumed_ && MatchFilter(it->type, filter))
        {
            result = true;
            it->consumed_ = true;
            *out_event = it;
            break;
        }
        it = it->next;
    }

    return result;
}

#define APP_UPDATE_AND_RENDER(name) void name(Platform *platform)
typedef APP_UPDATE_AND_RENDER(AppUpdateAndRenderType);
extern "C" DUNGEONS_EXPORT APP_UPDATE_AND_RENDER(AppUpdateAndRender);

#endif /* DUNGEONS_PLATFORM_HPP */
