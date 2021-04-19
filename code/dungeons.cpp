#include "dungeons.hpp"

#include "dungeons_image.cpp"
#include "dungeons_render.cpp"

static inline Bitmap
PushBitmap(Arena *arena, int w, int h)
{
    Bitmap result = {};
    result.w = w;
    result.h = h;
    result.pitch = w;
    result.data = PushArray(arena, w*h, Color);
    return result;
}

static Font
MakeFont(Bitmap bitmap, int32_t glyph_w, int32_t glyph_h)
{
    Font font = {};

    font.w = bitmap.w;
    font.h = bitmap.h;
    font.pitch = bitmap.pitch;
    font.glyph_w = glyph_w;
    font.glyph_h = glyph_h;

    // you probably got it wrong if there's slop
    Assert(font.w % glyph_w == 0);
    Assert(font.h % glyph_h == 0);

    font.glyphs_per_row = font.w / glyph_w;
    font.glyphs_per_col = font.h / glyph_h;
    font.glyph_count = font.glyphs_per_col*font.glyphs_per_row;

    font.data = bitmap.data;

    return font;
}

static Arena test_arena;
static Bitmap test_bitmap;
static Font test_font;
static bool initialized;

static int test_text_count;
static int test_text_at;
static char test_text[64];

extern "C" void
App_UpdateAndRender(Platform *platform_)
{
    platform = platform_;

    if (!initialized)
    {
        Buffer test_file = platform->ReadFile(&test_arena, "font8x12.bmp");
        Assert(test_file.size);

        test_bitmap = ParseBitmap(test_file);
        test_font = MakeFont(test_bitmap, 8, 12);

        initialized = true;
    }

    for (PlatformEvent *event; PopEvent(&event, PlatformEventFilter_KeyDown|PlatformEventFilter_Text);)
    {
        switch (event->type)
        {
            case PlatformEvent_KeyDown:
            {
                if (event->key_code == PlatformKeyCode_Back)
                {
                    if (test_text_at > 0)
                    {
                        test_text_at -= 1;
                        for (int i = test_text_at; i < test_text_count - 1; ++i)
                        {
                            test_text[i] = test_text[i + 1];
                        }
                        test_text_count -= 1;
                    }
                }
                else if (event->key_code == PlatformKeyCode_Left)
                {
                    if (test_text_at > 0)
                    {
                        test_text_at -= 1;
                    }
                }
                else if (event->key_code == PlatformKeyCode_Right)
                {
                    if (test_text_at < test_text_count)
                    {
                        test_text_at += 1;
                    }
                }
            } break;

            case PlatformEvent_Text:
            {
                for (int i = 0; i < event->text_length; ++i)
                {
                    if (test_text_at >= (int)ArrayCount(test_text))
                    {
                        break;
                    }
                    if (event->text[i] >= 32 && event->text[i] < 127)
                    {
                        test_text_count += 1;
                        for (int i = test_text_count - 1; i >= test_text_at; --i)
                        {
                                test_text[i] = test_text[i - 1];
                        }
                        test_text[test_text_at++] = event->text[i];
                    }
                }
            } break;

            INVALID_DEFAULT_CASE
        }
    }

    V2i mouse_p = MakeV2i(platform->mouse_x, platform->mouse_y);

    ClearBitmap(platform->backbuffer, MakeColor(0, 0, 0));

    DrawRect(platform->backbuffer, test_font, MakeRect2iMinDim(4, 4, 68, 5),
             MakeColor(255, 255, 255), MakeColor(0, 0, 0));
    for (int i = 0; i < test_text_count; ++i)
    {
        Color foreground = MakeColor(255, 255, 255);
        Color background = MakeColor(0, 0, 0);

        if (i == test_text_at)
        {
            Swap(foreground, background);
        }

        DrawTile(platform->backbuffer, test_font, MakeV2i(6 + i, 6), (Glyph)test_text[i],
                 foreground, background);
    }
}
