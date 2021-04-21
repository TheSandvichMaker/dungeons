static inline void
InitializeRenderState(Bitmap *target, Font *world_font, Font *ui_font)
{
    render_state->target     = target;
    render_state->world_font = world_font;
    render_state->ui_font    = ui_font;
    if (((world_font->glyph_w % ui_font->glyph_w) != 0) ||
        ((world_font->glyph_h % ui_font->glyph_h) != 0))
    {
        platform->ReportError(PlatformError_Nonfatal, "Bad font metrics! The UI font does not evenly fit into the World font (world: %dx%d versus ui: %dx%d)", world_font->glyph_w, world_font->glyph_h, ui_font->glyph_w, ui_font->glyph_h);
    }

    if ((world_font->glyph_w < ui_font->glyph_w) ||
        (world_font->glyph_h < ui_font->glyph_h))
    {
        platform->ReportError(PlatformError_Nonfatal, "Bad font metrics! The UI font is bigger than the World font (world: %dx%d versus ui: %dx%d)", world_font->glyph_w, world_font->glyph_h, ui_font->glyph_w, ui_font->glyph_h);
    }

    render_state->wall_segment_lookup[Wall_Top|Wall_Bottom]                                          = 179;
    render_state->wall_segment_lookup[Wall_Top|Wall_Bottom|Wall_Left]                                = 180;
    render_state->wall_segment_lookup[Wall_Top|Wall_Bottom|Wall_LeftThick]                           = 181;
    render_state->wall_segment_lookup[Wall_TopThick|Wall_BottomThick|Wall_Left]                      = 182;
    render_state->wall_segment_lookup[Wall_Left|Wall_BottomThick]                                    = 183;
    render_state->wall_segment_lookup[Wall_LeftThick|Wall_Bottom]                                    = 184;
    render_state->wall_segment_lookup[Wall_LeftThick|Wall_TopThick|Wall_BottomThick]                 = 185;
    render_state->wall_segment_lookup[Wall_TopThick|Wall_BottomThick]                                = 186;
    render_state->wall_segment_lookup[Wall_LeftThick|Wall_BottomThick]                               = 187;
    render_state->wall_segment_lookup[Wall_LeftThick|Wall_TopThick]                                  = 188;
    render_state->wall_segment_lookup[Wall_Left|Wall_TopThick]                                       = 189;
    render_state->wall_segment_lookup[Wall_LeftThick|Wall_Top]                                       = 190;
    render_state->wall_segment_lookup[Wall_Left|Wall_Bottom]                                         = 191;
    render_state->wall_segment_lookup[Wall_Right|Wall_Top]                                           = 192;
    render_state->wall_segment_lookup[Wall_Left|Wall_Right|Wall_Top]                                 = 193;
    render_state->wall_segment_lookup[Wall_Left|Wall_Right|Wall_Bottom]                              = 194;
    render_state->wall_segment_lookup[Wall_Top|Wall_Bottom|Wall_Right]                               = 195;
    render_state->wall_segment_lookup[Wall_Left|Wall_Right]                                          = 196;
    render_state->wall_segment_lookup[Wall_Left|Wall_Right|Wall_Top|Wall_Bottom]                     = 197;
    render_state->wall_segment_lookup[Wall_Top|Wall_Bottom|Wall_RightThick]                          = 198;
    render_state->wall_segment_lookup[Wall_TopThick|Wall_BottomThick|Wall_Right]                     = 199;
    render_state->wall_segment_lookup[Wall_TopThick|Wall_RightThick]                                 = 200;
    render_state->wall_segment_lookup[Wall_BottomThick|Wall_RightThick]                              = 201;
    render_state->wall_segment_lookup[Wall_LeftThick|Wall_RightThick|Wall_TopThick]                  = 202;
    render_state->wall_segment_lookup[Wall_LeftThick|Wall_RightThick|Wall_BottomThick]               = 203;
    render_state->wall_segment_lookup[Wall_TopThick|Wall_BottomThick|Wall_RightThick]                = 204;
    render_state->wall_segment_lookup[Wall_LeftThick|Wall_RightThick]                                = 205;
    render_state->wall_segment_lookup[Wall_TopThick|Wall_BottomThick|Wall_LeftThick|Wall_RightThick] = 206;
    render_state->wall_segment_lookup[Wall_LeftThick|Wall_RightThick|Wall_Top]                       = 207;
    render_state->wall_segment_lookup[Wall_Left|Wall_Right|Wall_TopThick]                            = 208;
    render_state->wall_segment_lookup[Wall_LeftThick|Wall_RightThick|Wall_Bottom]                    = 209;
    render_state->wall_segment_lookup[Wall_Left|Wall_Right|Wall_BottomThick]                         = 210;
    render_state->wall_segment_lookup[Wall_TopThick|Wall_Right]                                      = 211;
    render_state->wall_segment_lookup[Wall_Top|Wall_RightThick]                                      = 212;
    render_state->wall_segment_lookup[Wall_RightThick|Wall_Bottom]                                   = 213;
    render_state->wall_segment_lookup[Wall_BottomThick|Wall_Right]                                   = 214;
    render_state->wall_segment_lookup[Wall_Left|Wall_TopThick|Wall_BottomThick|Wall_Right]           = 215;
    render_state->wall_segment_lookup[Wall_LeftThick|Wall_Top|Wall_Bottom|Wall_RightThick]           = 216;
    render_state->wall_segment_lookup[Wall_Left|Wall_Top]                                            = 217;
    render_state->wall_segment_lookup[Wall_Right|Wall_Bottom]                                        = 218;
}

static inline Sprite
MakeWall(uint32_t wall_flags, Color foreground = COLOR_WHITE, Color background = COLOR_BLACK)
{
    Sprite result = {};
    result.foreground = foreground;
    result.background = background;

    if (wall_flags < ArrayCount(render_state->wall_segment_lookup))
    {
        result.glyph = render_state->wall_segment_lookup[wall_flags];
    }

    if (!result.glyph)
    {
        result.glyph = '?';
    }

    return result;
}

static inline void
ClearBitmap(Bitmap *bitmap, Color clear_color)
{
    Color *at = bitmap->data;
    for (int i = 0; i < bitmap->pitch*bitmap->h; ++i)
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
BlitBitmap(Bitmap *dest, Bitmap *source, V2i p)
{
    int source_min_x = Max(0, -p.x);
    int source_min_y = Max(0, -p.y);
    int source_max_x = Min(source->w, dest->w - p.x);
    int source_max_y = Min(source->h, dest->h - p.y);
    int source_adjusted_w = source_max_x - source_min_x;
    int source_adjusted_h = source_max_y - source_min_y;

    p = Clamp(p, MakeV2i(0, 0), MakeV2i(dest->w, dest->h));

    Color *source_row = source->data + source_min_y*source->pitch + source_min_x;
    Color *dest_row = dest->data + p.y*dest->pitch + p.x;
    for (int y = 0; y < source_adjusted_h; ++y)
    {
        Color *source_pixel = source_row;
        Color *dest_pixel = dest_row;
        for (int x = 0; x < source_adjusted_w; ++x)
        {
            *dest_pixel++ = *source_pixel++;
        }
        source_row += source->pitch;
        dest_row += dest->pitch;
    }
}

static inline void
BlitBitmapMask(Bitmap *dest, Bitmap *source, V2i p, Color foreground, Color background)
{
    int source_min_x = Max(0, -p.x);
    int source_min_y = Max(0, -p.y);
    int source_max_x = Min(source->w, dest->w - p.x);
    int source_max_y = Min(source->h, dest->h - p.y);
    int source_adjusted_w = source_max_x - source_min_x;
    int source_adjusted_h = source_max_y - source_min_y;

    p = Clamp(p, MakeV2i(0, 0), MakeV2i(dest->w, dest->h));

    Color *source_row = source->data + source_min_y*source->pitch + source_min_x;
    Color *dest_row = dest->data + p.y*dest->pitch + p.x;
    for (int y = 0; y < source_adjusted_h; ++y)
    {
        Color *source_pixel = source_row;
        Color *dest_pixel = dest_row;
        for (int x = 0; x < source_adjusted_w; ++x)
        {
            Color color = *source_pixel++;
            if (color.a)
            {
                *dest_pixel = foreground;
            }
            else
            {
                *dest_pixel = background;
            }
            ++dest_pixel;
        }
        source_row += source->pitch;
        dest_row += dest->pitch;
    }
}

static inline Bitmap
MakeBitmapView(const Bitmap &source, Rect2i rect)
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
BlitCharMask(Bitmap *dest, Font *font, V2i p, Glyph glyph, Color foreground, Color background)
{
    if (glyph < font->glyph_count)
    {
        uint32_t glyph_x = font->glyph_w*(glyph % font->glyphs_per_row);
        uint32_t glyph_y = font->glyph_h*(font->glyphs_per_col - (glyph / font->glyphs_per_row) - 1);

        Bitmap glyph_bitmap = MakeBitmapView(font->bitmap,
                                             MakeRect2iMinDim(glyph_x, glyph_y, font->glyph_w, font->glyph_h));

        BlitBitmapMask(dest, &glyph_bitmap, p, foreground, background);
    }
}

static inline V2i
TileToScreen(DrawMode mode, V2i p)
{
    if (mode == Draw_World)
    {
        p -= render_state->camera_bottom_left;
        p *= MakeV2i(render_state->world_font->glyph_w, render_state->world_font->glyph_h);
    }
    else
    {
        Assert(mode == Draw_Ui);
        p *= MakeV2i(render_state->ui_font->glyph_w, render_state->ui_font->glyph_h);
    }
    return p;
}

static inline Font *
GetFont(DrawMode mode)
{
    if (mode == Draw_World)
    {
        return render_state->world_font;
    }
    else
    {
        return render_state->ui_font;
    }
}

static inline void
DrawTile(DrawMode mode, V2i tile_p, Sprite sprite)
{
    V2i screen_p = TileToScreen(mode, tile_p);
    BlitCharMask(render_state->target, GetFont(mode), screen_p, sprite.glyph, sprite.foreground, sprite.background);
}

static inline 
void DrawRect(DrawMode mode, const Rect2i &rect, Color foreground, Color background)
{
    uint32_t left   = Wall_Left;
    uint32_t right  = Wall_Right;
    uint32_t top    = Wall_Top;
    uint32_t bottom = Wall_Bottom;
#if 0
    if (flags & Format_OutlineThickHorz)
    {
        left  = Wall_LeftThick;
        right = Wall_RightThick;
    }
    if (flags & Format_OutlineThickVert)
    {
        top    = Wall_TopThick;
        bottom = Wall_BottomThick;
    }
#endif

    DrawTile(mode, rect.min, MakeWall(right|top, foreground, background));
    DrawTile(mode, MakeV2i(rect.max.x - 1, rect.min.y), MakeWall(left|top, foreground, background));
    DrawTile(mode, rect.max - MakeV2i(1, 1), MakeWall(left|bottom, foreground, background));
    DrawTile(mode, MakeV2i(rect.min.x, rect.max.y - 1), MakeWall(right|bottom, foreground, background));

    for (int32_t x = rect.min.x + 1; x < rect.max.x - 1; ++x)
    {
        DrawTile(mode, MakeV2i(x, rect.min.y), MakeWall(left|right, foreground, background));
        DrawTile(mode, MakeV2i(x, rect.max.y - 1), MakeWall(left|right, foreground, background));
    }

    for (int32_t y = rect.min.y + 1; y < rect.max.y - 1; ++y)
    {
        DrawTile(mode, MakeV2i(rect.min.x, y), MakeWall(top|bottom, foreground, background));
        DrawTile(mode, MakeV2i(rect.max.x - 1, y), MakeWall(top|bottom, foreground, background));
    }
}
