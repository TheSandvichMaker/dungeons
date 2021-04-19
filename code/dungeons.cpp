#include "dungeons.hpp"

#include "dungeons_image.cpp"

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

static inline void
ClearBitmap(Bitmap *bitmap, Color clear_color)
{
    Color *at = bitmap->data;
    for (int i = 0; i < bitmap->w*bitmap->h; ++i)
    {
        *at++ = clear_color;
    }
}

static inline void
BlitRect(Bitmap *bitmap, Rect2i rect, Color color)
{
    rect = Intersect(rect, MakeRect2iMinMax(MakeV2i(0, 0), MakeV2i(bitmap->w, bitmap->h)));
    for (int y = rect.min.y; y < rect.max.y; ++y)
    for (int x = rect.min.x; x < rect.max.x; ++x)
    {
        bitmap->data[y*bitmap->w + x] = color;
    }
}

static inline void
BlitBitmap(const Bitmap &dest, const Bitmap &source, V2i p)
{
    int source_min_x = Max(0, -p.x);
    int source_min_y = Max(0, -p.y);
    int source_max_x = Min(source.w, dest.w - p.x);
    int source_max_y = Min(source.h, dest.h - p.y);
    int source_adjusted_w = source_max_x - source_min_x;
    int source_adjusted_h = source_max_y - source_min_y;

    p = Clamp(p, MakeV2i(0, 0), MakeV2i(dest.w, dest.h));

    Color *source_row = source.data + source_min_y*source.pitch + source_min_x;
    Color *dest_row = dest.data + p.y*dest.pitch + p.x;
    for (int y = 0; y < source_adjusted_h; ++y)
    {
        Color *source_pixel = source_row;
        Color *dest_pixel = dest_row;
        for (int x = 0; x < source_adjusted_w; ++x)
        {
            *dest_pixel++ = *source_pixel++;
        }
        source_row += source.pitch;
        dest_row += dest.pitch;
    }
}

static inline void
BlitBitmapMask(const Bitmap &dest, const Bitmap &source, V2i p, Color foreground, Color background)
{
    int source_min_x = Max(0, -p.x);
    int source_min_y = Max(0, -p.y);
    int source_max_x = Min(source.w, dest.w - p.x);
    int source_max_y = Min(source.h, dest.h - p.y);
    int source_adjusted_w = source_max_x - source_min_x;
    int source_adjusted_h = source_max_y - source_min_y;

    p = Clamp(p, MakeV2i(0, 0), MakeV2i(dest.w, dest.h));

    Color *source_row = source.data + source_min_y*source.pitch + source_min_x;
    Color *dest_row = dest.data + p.y*dest.pitch + p.x;
    for (int y = 0; y < source_adjusted_h; ++y)
    {
        Color *source_pixel = source_row;
        Color *dest_pixel = dest_row;
        for (int x = 0; x < source_adjusted_w; ++x)
        {
            Color source = *source_pixel++;
            if (source.a)
            {
                *dest_pixel++ = foreground;
            }
            else
            {
                *dest_pixel++ = background;
            }
            ++dest_pixel;
        }
        source_row += source.pitch;
        dest_row += dest.pitch;
    }
}

static inline Bitmap
MakeBitmapView(Bitmap source, Rect2i rect)
{
    rect = Intersect(rect, 0, 0, source.w, source.h);

    Bitmap result = {};
    result.w = GetWidth(rect);
    result.h = GetHeight(rect);
    result.pitch = source.pitch;
    result.data = source.data + source.pitch*rect.min.y + rect.min.x;
    return result;
}

static inline void
BlitCharMask(const Bitmap &dest, const Font &font, V2i p, uint32_t glyph, Color foreground, Color background)
{
    if (glyph < font.glyph_count)
    {
        uint32_t glyph_x = font.glyph_w*(glyph % font.glyphs_per_row);
        uint32_t glyph_y = font.glyph_h*(font.glyphs_per_col - (glyph / font.glyphs_per_row) - 1);

        Bitmap glyph_bitmap = MakeBitmapView(font.bitmap,
                                             MakeRect2iMinDim(glyph_x, glyph_y, font.glyph_w, font.glyph_h));

        BlitBitmap(dest, glyph_bitmap, p);
    }
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

static int test_text_at;
static char test_text[512];

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

    for (PlatformEvent *event = nullptr; PopEvent(&event, PlatformEvent_Text);)
    {
        for (int i = 0; i < event->text_length; ++i)
        {
            if (test_text_at >= (int)ArrayCount(test_text))
            {
                break;
            }
            test_text[test_text_at++] = event->text[i];
        }
    }

    V2i mouse_p = MakeV2i(platform->mouse_x, platform->mouse_y);

    ClearBitmap(&platform->backbuffer, MakeColor(0, 0, 0));

    for (int i = 0; i < test_text_at; ++i)
    {
        BlitCharMask(platform->backbuffer, test_font,
                     mouse_p + MakeV2i(test_font.glyph_w*i, 0),
                     (uint32_t)test_text[i], MakeColor(255, 255, 255), MakeColor(0, 0, 0));
    }
}
