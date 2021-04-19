static inline void
ClearBitmap(const Bitmap &bitmap, Color clear_color)
{
    Color *at = bitmap.data;
    for (int i = 0; i < bitmap.pitch*bitmap.h; ++i)
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
        bitmap->data[y*bitmap->pitch + x] = color;
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
                *dest_pixel = foreground;
            }
            else
            {
                *dest_pixel = background;
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
BlitCharMask(const Bitmap &dest, const Font &font, V2i p, Glyph glyph, Color foreground, Color background)
{
    if (glyph < font.glyph_count)
    {
        uint32_t glyph_x = font.glyph_w*(glyph % font.glyphs_per_row);
        uint32_t glyph_y = font.glyph_h*(font.glyphs_per_col - (glyph / font.glyphs_per_row) - 1);

        Bitmap glyph_bitmap = MakeBitmapView(font.bitmap,
                                             MakeRect2iMinDim(glyph_x, glyph_y, font.glyph_w, font.glyph_h));

        BlitBitmapMask(dest, glyph_bitmap, p, foreground, background);
    }
}

static inline void
DrawTile(const Bitmap &dest, const Font &font, V2i tile_p, Glyph glyph, Color foreground, Color background)
{
    V2i screen_p = MakeV2i(font.glyph_w, font.glyph_h)*tile_p;
    BlitCharMask(dest, font, screen_p, glyph, foreground, background);
}

static inline 
void DrawRect(const Bitmap &dest, const Font &font, const Rect2i &rect, Color foreground, Color background)
{
    uint32_t left   = Wall_Left;
    uint32_t right  = Wall_Right;
    uint32_t top    = Wall_Top;
    uint32_t bottom = Wall_Bottom;
#if 0
    if (flags & Format_OutlineThickHorz) {
        left  = Wall_LeftThick;
        right = Wall_RightThick;
    }
    if (flags & Format_OutlineThickVert) {
        top    = Wall_TopThick;
        bottom = Wall_BottomThick;
    }
#endif

    DrawTile(dest, font, rect.min, MakeWall(right|top), foreground, background);
    DrawTile(dest, font, MakeV2i(rect.max.x - 1, rect.min.y), MakeWall(left|top), foreground, background);
    DrawTile(dest, font, rect.max - MakeV2i(1, 1), MakeWall(left|bottom), foreground, background);
    DrawTile(dest, font, MakeV2i(rect.min.x, rect.max.y - 1), MakeWall(right|bottom), foreground, background);

    for (int32_t x = rect.min.x + 1; x < rect.max.x - 1; ++x)
    {
        DrawTile(dest, font, MakeV2i(x, rect.min.y), MakeWall(left|right), foreground, background);
        DrawTile(dest, font, MakeV2i(x, rect.max.y - 1), MakeWall(left|right), foreground, background);
    }

    for (int32_t y = rect.min.y + 1; y < rect.max.y - 1; ++y)
    {
        DrawTile(dest, font, MakeV2i(rect.min.x, y), MakeWall(top|bottom), foreground, background);
        DrawTile(dest, font, MakeV2i(rect.max.x - 1, y), MakeWall(top|bottom), foreground, background);
    }
}
