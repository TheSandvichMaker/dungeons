#ifndef DUNGEONS_PLATFORM_HPP
#define DUNGEONS_PLATFORM_HPP

#define Assert(x) \
    ((x) ? 1 \
         : (platform->ReportError(PlatformError_Fatal, \
                                  "Assertion Failed: " #x " at file %s, line %d", __FILE__, __LINE__), 0))
     

#ifdef DEBUG_BUILD
#define AssertSlow(x) Assert(x)
#else
#define AssertSlow(x) 
#endif

#define FILE_AND_LINE_STRING__(File, Line) File ":" #Line
#define FILE_AND_LINE_STRING_(File, Line) FILE_AND_LINE_STRING__(File, Line)
#define FILE_AND_LINE_STRING FILE_AND_LINE_STRING_(__FILE__, __LINE__)
#define LOCATION_STRING(...) FILE_AND_LINE_STRING " (" __VA_ARGS__ ")"

#define INVALID_CODE_PATH Assert(!"Invalid Code Path!");
#define INVALID_DEFAULT_CASE default: { Assert(!"Invalid Default Case!"); } break;
#define INCOMPLETE_SWITCH default: { /* nah */ } break;

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

const char *PlatformEventType_Name[] =
{
    "PlatformEvent_None",
    "PlatformEvent_MouseUp",
    "PlatformEvent_MouseDown",
    "PlatformEvent_KeyUp",
    "PlatformEvent_KeyDown",
    "PlatformEvent_COUNT",
};

enum PlatformMouseButton
{
    PlatformMouseButton_Left,
    PlatformMouseButton_Middle,
    PlatformMouseButton_Right,
    PlatformMouseButton_COUNT,
};

const char *PlatformMouseButton_Name[] =
{
    "PlatformMouseButton_Left",
    "PlatformMouseButton_Middle",
    "PlatformMouseButton_Right",
    "PlatformMouseButton_COUNT",
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
    /* 0x30 - 0x39: ascii numerals */
    /* 0x3A - 0x40: undefined */
    /* 0x41 - 0x5A: ascii alphabet */
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
};

struct PlatformEvent
{
    PlatformEvent *next;
    PlatformEventType type;
    bool pressed;
    PlatformMouseButton mouse_button;
    PlatformKeyCode key_code;
    char *text;
};

struct PlatformOffscreenBuffer
{
    int32_t w, h;
    Color *data;
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

struct Platform
{
    PlatformEvent *first_event;
    PlatformEvent *last_event;

    int32_t render_w, render_h;
    PlatformOffscreenBuffer backbuffer;

    void (*ReportError)(PlatformErrorType type, char *message, ...);

    size_t page_size;
    void *(*AllocateMemory)(size_t size, uint32_t flags, const char *tag);
    void *(*ReserveMemory)(size_t size, uint32_t flags, const char *tag);
    void *(*CommitMemory)(void *location, size_t size);
    void (*DecommitMemory)(void *location, size_t size);
    void (*DeallocateMemory)(void *memory);
};

static inline PlatformEvent *
PopEvent(Platform *platform, PlatformEventType target_type = PlatformEvent_Any)
{
    PlatformEvent *result = nullptr;
    for (PlatformEvent **event_at = &platform->first_event;
         *event_at;
         )
    {
        PlatformEvent *event = *event_at;
        if (target_type == PlatformEvent_Any ||
            event->type == target_type)
        {
            result = event;
            *event_at = result->next;
            if (result == platform->last_event)
            {
                Assert(platform->first_event == nullptr);
                platform->last_event = nullptr;
            }

            break;
        }

        event_at = &(*event_at)->next;
    }
    return result;
}

extern Platform *platform;
extern "C" void App_UpdateAndRender(void);

#endif /* DUNGEONS_PLATFORM_HPP */
