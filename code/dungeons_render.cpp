static inline void
InitializeRenderState(Arena *arena, Bitmap *target, Font *world_font, Font *ui_font)
{
    render_state->target     = target;
    render_state->world_font = world_font;
    render_state->ui_font    = ui_font;
    if (((world_font->glyph_w % ui_font->glyph_w) != 0) ||
        ((world_font->glyph_h % ui_font->glyph_h) != 0))
    {
        platform->ReportError(PlatformError_Nonfatal, "Bad font metrics! The UI font does not evenly fit into the World font (world: %dx%d versus ui: %dx%d)",
                              world_font->glyph_w, world_font->glyph_h, ui_font->glyph_w, ui_font->glyph_h);
    }

    if ((world_font->glyph_w < ui_font->glyph_w) ||
        (world_font->glyph_h < ui_font->glyph_h))
    {
        platform->ReportError(PlatformError_Nonfatal, "Bad font metrics! The UI font is bigger than the World font (world: %dx%d versus ui: %dx%d)",
                              world_font->glyph_w, world_font->glyph_h, ui_font->glyph_w, ui_font->glyph_h);
    }

    render_state->cb_size = Megabytes(4); // random choice
    render_state->command_buffer = PushArrayNoClear(arena, render_state->cb_size, char);

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

    render_state->ui_top_right = MakeV2i(platform->render_w, platform->render_h) / MakeV2i(render_state->ui_font->glyph_w,
                                                                                           render_state->ui_font->glyph_h);
}

static inline V2i
GlyphDim(Font *font)
{
    return MakeV2i(font->glyph_w, font->glyph_h);
}

DUNGEONS_INLINE Color
LinearToSRGB(V3 linear)
{
    Color result = MakeColor((uint8_t)(SquareRoot(linear.x)*255.0f),
                             (uint8_t)(SquareRoot(linear.y)*255.0f),
                             (uint8_t)(SquareRoot(linear.z)*255.0f));
    return result;
}

DUNGEONS_INLINE V3
SRGBToLinear(Color color)
{
    V3 result = MakeV3((1.0f / 255.0f)*(float)color.r,
                       (1.0f / 255.0f)*(float)color.g,
                       (1.0f / 255.0f)*(float)color.b);
    result.x *= result.x;
    result.y *= result.y;
    result.z *= result.z;
    return result;
}

static inline Bitmap
PushBitmap(Arena *arena, int w, int h)
{
    Bitmap result = {};
    result.w = w;
    result.h = h;
    result.pitch = w;
    result.data = PushAlignedArray(arena, w*h, Color, 16);
    return result;
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

static inline V2i
ScreenToUi(V2i p)
{
    p.x /= render_state->ui_font->glyph_w;
    p.y /= render_state->ui_font->glyph_h;
    return p;
}

static inline V2i
ScreenToWorld(V2i p)
{
    p.x /= render_state->world_font->glyph_w;
    p.y /= render_state->world_font->glyph_h;
    p += render_state->camera_bottom_left;
    return p;
}

static inline void
ClearBitmap(Bitmap *bitmap, Color clear_color = MakeColor(0, 0, 0, 0))
{
    Color *row = bitmap->data;
    for (int y = 0; y < bitmap->h; ++y)
    {
        Color *at = row;
        for (int x = 0; x < bitmap->w; ++x)
        {
            *at++ = clear_color;
        }
        row += bitmap->pitch;
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

static inline void
BlitBitmapMaskSSE(Bitmap *dest, Bitmap *source, V2i p, Color foreground, Color background)
{
    int source_min_x = Max(0, -p.x);
    int source_min_y = Max(0, -p.y);
    int source_max_x = Min(source->w, dest->w - p.x);
    int source_max_y = Min(source->h, dest->h - p.y);
    int source_adjusted_w = source_max_x - source_min_x;
    int source_adjusted_h = source_max_y - source_min_y;

    Assert((dest->pitch % 4) == 0);
    Assert((source->pitch % 4) == 0);
    Assert((source_adjusted_w % 4) == 0);

    p = Clamp(p, MakeV2i(0, 0), MakeV2i(dest->w, dest->h));

    __m128i mask_ff = _mm_set1_epi32(0xFF);
    __m128i zero = _mm_set1_epi32(0);
    __m128i foreground_wide = _mm_set1_epi32(foreground.u32);
    __m128i background_wide = _mm_set1_epi32(background.u32);

    Color *source_row = source->data + source_min_y*source->pitch + source_min_x;
    Color *dest_row = dest->data + p.y*dest->pitch + p.x;
    for (int y = 0; y < source_adjusted_h; ++y)
    {
        Color *source_pixel = source_row;
        Color *dest_pixel = dest_row;
        for (int x = 0; x < source_adjusted_w; x += 4)
        {
            __m128i source_color = _mm_loadu_si128((__m128i *)source_pixel);
            __m128i source_a = _mm_and_si128(_mm_srli_epi32(source_color, 24), mask_ff);
            __m128i alpha_mask = _mm_cmpgt_epi32(source_a, zero);
            __m128i dest_color = _mm_and_si128(alpha_mask, foreground_wide);
            dest_color = _mm_or_si128(dest_color, _mm_andnot_si128(alpha_mask, background_wide));
            _mm_storeu_si128((__m128i *)dest_pixel, dest_color);

            source_pixel += 4;
            dest_pixel += 4;
        }
        source_row += source->pitch;
        dest_row += dest->pitch;
    }
}

static inline Bitmap
MakeBitmapView(Bitmap *source, Rect2i rect)
{
    rect = Intersect(rect, 0, 0, source->w, source->h);

    Bitmap result = {};
    result.w = GetWidth(rect);
    result.h = GetHeight(rect);
    result.pitch = source->pitch;
    result.data = source->data + source->pitch*rect.min.y + rect.min.x;
    return result;
}

static inline Rect2i
GetGlyphRect(Font *font, Glyph glyph)
{
    AssertSlow(glyph < font->glyph_count);
    uint32_t glyph_x = font->glyph_w*(glyph % font->glyphs_per_row);
    uint32_t glyph_y = font->glyph_h*(font->glyphs_per_col - (glyph / font->glyphs_per_col) - 1);
    Rect2i result = MakeRect2iMinDim(glyph_x, glyph_y, font->glyph_w, font->glyph_h);
    return result;
}

static inline void
BlitCharMask(Bitmap *dest, Font *font, V2i p, Glyph glyph, Color foreground, Color background)
{
    if (glyph < font->glyph_count)
    {
        Bitmap glyph_bitmap = MakeBitmapView(&font->bitmap, GetGlyphRect(font, glyph));
        BlitBitmapMask(dest, &glyph_bitmap, p, foreground, background);
    }
}

static inline RenderCommand *
PushRenderCommand(RenderLayer layer, RenderCommandKind kind)
{
    RenderCommand *command = &render_state->null_command;

    size_t size_required = sizeof(RenderCommand) + sizeof(RenderSortKey);
    size_t size_left     = render_state->cb_sort_key_at - render_state->cb_command_at;

    if (size_left >= size_required)
    {
        render_state->cb_sort_key_at -= sizeof(RenderSortKey);
        RenderSortKey *sort_key = (RenderSortKey *)(render_state->command_buffer + render_state->cb_sort_key_at);

        uint32_t offset = render_state->cb_command_at;
        command = (RenderCommand *)(render_state->command_buffer + offset);
        render_state->cb_command_at += sizeof(RenderCommand);

        command->kind = kind;

        sort_key->offset = offset;
        sort_key->layer  = layer;
    }
    else
    {
        INVALID_CODE_PATH;
    }

    return command;
}

static inline void
EndPushRenderCommand(RenderCommand *command)
{
    if (command != &render_state->null_command)
    {
        // render_state->command_buffer_hash = HashData(render_state->command_buffer_hash, sizeof(*sort_key), sort_key);
        // render_state->command_buffer_hash = HashData(render_state->command_buffer_hash, sizeof(*command), command);
    }
}

static inline void
DrawTile(RenderLayer layer, V2i tile_p, Sprite sprite)
{
    RenderCommand *command = PushRenderCommand(layer, RenderCommand_Sprite);
    command->p = tile_p;
    command->sprite = sprite;
}

static inline void
DrawText(RenderLayer layer, V2i p, String text, Color foreground, Color background)
{
    V2i at = p;

    Sprite sprite = {};
    sprite.foreground = foreground;
    sprite.background = background;

    for (size_t i = 0; i < text.size; ++i)
    {
        sprite.glyph = text.data[i];
        DrawTile(layer, at, sprite);
        at.x += 1;
    }
}

static inline void
DrawStringList(RenderLayer layer, StringList *list, V2i p, StringRenderSpec spec = {})
{
    if (spec.horizontal_advance == 0)
    {
        spec.horizontal_advance = 1;
    }

    if (spec.vertical_advance == 0)
    {
        spec.vertical_advance = -1;
    }

    int abs_horizontal_advance = Abs(spec.horizontal_advance);
    int sign_horizontal_advance = SignOf(spec.horizontal_advance);

    int abs_vertical_advance = Abs(spec.vertical_advance);

    int total_w = 0;
    int total_h = 1;

    int current_w = 0;
    for (StringNode *node = list->first;
         node;
         node = node->next)
    {
        String string = node->string;
        for (size_t i = 0; i < string.size; i += 1)
        {
            if (string.data[i] == '\n')
            {
                total_h += 1;
                current_w = 0;
            }
            else
            {
                current_w += 1;
            }

            if (total_w < current_w)
            {
                total_w = current_w;
            }
        }
    }

    total_w *= abs_horizontal_advance;
    total_h *= abs_vertical_advance;

    V2i start_p = p;
    if (spec.vertical_advance > 0)
    {
        if (spec.vertical_align == Align_Center)
        {
            start_p.y -= total_h / 2;
        }
        else if (spec.vertical_align == Align_Top)
        {
            start_p.y -= total_h - 1;
        }
    }
    else
    {
        if (spec.vertical_align == Align_Center)
        {
            start_p.y += total_h / 2;
        }
        else if (spec.vertical_align == Align_Left)
        {
            start_p.y += total_h - 1;
        }
    }

    V2i at_p = start_p;
    size_t scan_at = 0;
    StringNode *scan_node  = list->first;
    while (scan_node)
    {
        int line_w = 0;

        size_t print_at = scan_at;
        size_t print_end_at = print_at;
        StringNode *print_node = scan_node;

        while (scan_node)
        {
            String *scan = &scan_node->string;
            if (scan_at < scan->size)
            {
                uint8_t glyph = scan->data[scan_at];
                scan_at += 1;
                line_w += 1;

                if (glyph == '\n')
                {
                    break;
                }

                print_end_at = scan_at;
            }
            else
            {
                scan_node = scan_node->next;
                scan_at = 0;
                print_end_at = 0;
            }
        }

        line_w *= abs_horizontal_advance;

        at_p.x = start_p.x;
        if (spec.horizontal_advance > 0)
        {
            if (spec.horizontal_align == Align_Center)
            {
                at_p.x -= line_w / 2;
            }
            else if (spec.horizontal_align == Align_Right)
            {
                at_p.x -= line_w - 1;
            }
        }
        else
        {
            if (spec.horizontal_align == Align_Center)
            {
                at_p.x += line_w / 2;
            }
            else if (spec.horizontal_align == Align_Left)
            {
                at_p.x += line_w - 1;
            }
        }

        while (!((print_node == scan_node) &&
                 (print_at   == print_end_at)))
        {
            String *print = &print_node->string;
            if (print_at < print->size)
            {
                uint8_t glyph = print->data[print_at];
                print_at += 1;

                Sprite sprite = MakeSprite(glyph, print_node->foreground, print_node->background);
                DrawTile(layer, at_p, sprite);
                at_p.x += sign_horizontal_advance;

                sprite.glyph = ' ';
                for (int i = 1; i < abs_horizontal_advance; i += 1)
                {
                    DrawTile(layer, at_p, sprite);
                    at_p.x += sign_horizontal_advance;
                }
            }
            else
            {
                print_node = print_node->next;
                print_at = 0;
            }
        }

        at_p.y += spec.vertical_advance;
    }
}

static inline void
DrawRect(RenderLayer layer, const Rect2i &rect, Color color)
{
    RenderCommand *command = PushRenderCommand(layer, RenderCommand_Rect);
    command->rect = rect;
    command->color = color;
}

static inline void
DrawRectOutline(RenderLayer layer, const Rect2i &rect, Color foreground, Color background)
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

    DrawRect(layer, MakeRect2iMinMax(rect.min + MakeV2i(1, 1), rect.max - MakeV2i(1, 1)), background);

    DrawTile(layer, rect.min, MakeWall(right|top, foreground, background));
    DrawTile(layer, MakeV2i(rect.max.x - 1, rect.min.y), MakeWall(left|top, foreground, background));
    DrawTile(layer, rect.max - MakeV2i(1, 1), MakeWall(left|bottom, foreground, background));
    DrawTile(layer, MakeV2i(rect.min.x, rect.max.y - 1), MakeWall(right|bottom, foreground, background));

    for (int32_t x = rect.min.x + 1; x < rect.max.x - 1; ++x)
    {
        DrawTile(layer, MakeV2i(x, rect.min.y), MakeWall(left|right, foreground, background));
        DrawTile(layer, MakeV2i(x, rect.max.y - 1), MakeWall(left|right, foreground, background));
    }

    for (int32_t y = rect.min.y + 1; y < rect.max.y - 1; ++y)
    {
        DrawTile(layer, MakeV2i(rect.min.x, y), MakeWall(top|bottom, foreground, background));
        DrawTile(layer, MakeV2i(rect.max.x - 1, y), MakeWall(top|bottom, foreground, background));
    }
}

static void
BeginRender(void)
{
    Arena *arena = platform->GetTempArena();
    render_state->arena = arena;

    render_state->fonts[Layer_Ground] = render_state->world_font;
    render_state->fonts[Layer_Floor] = render_state->world_font;
    render_state->fonts[Layer_World] = render_state->world_font;
    render_state->fonts[Layer_Ui] = render_state->ui_font;

    render_state->cb_command_at = 0;
    render_state->cb_sort_key_at = render_state->cb_size;

    Swap(render_state->command_buffer_hash, render_state->prev_command_buffer_hash);
    render_state->command_buffer_hash = 0;

    ZeroStruct(&render_state->light_map);
}

struct TiledRenderJobParams
{
    Rect2i clip_rect;
    Bitmap *target;
};

static
PLATFORM_JOB(TiledRenderJob)
{
    TiledRenderJobParams *params = (TiledRenderJobParams *)args;

    Rect2i clip_rect = params->clip_rect;

    Bitmap target = MakeBitmapView(params->target, clip_rect);
    ClearBitmap(&target, COLOR_BLACK);

    char *command_buffer = render_state->command_buffer;
    RenderSortKey *sort_keys = (RenderSortKey *)(command_buffer + render_state->cb_sort_key_at);

    RenderSortKey *end = (RenderSortKey *)(command_buffer + render_state->cb_size);
    for (RenderSortKey *at = sort_keys; at < end; at += 1)
    {
        RenderCommand *command = (RenderCommand *)(command_buffer + at->offset);
        Font *font = render_state->fonts[at->layer];
        V2i glyph_dim = GlyphDim(font);

        switch (command->kind)
        {
            case RenderCommand_Sprite:
            {
                V2i p = MakeV2i(command->p.x, command->p.y);
                Sprite *sprite = &command->sprite;

                V3 light = MakeV3(1);
                if (LayerUsesCamera((RenderLayer)at->layer))
                {
                    // light = render_state->light_map.map[p.y][p.x];
                    p -= render_state->camera_bottom_left;
                }

                p *= glyph_dim;
                if (RectanglesOverlap(clip_rect, MakeRect2iMinDim(p, MakeV2i(font->glyph_w, font->glyph_h))))
                {
                    p -= clip_rect.min;

                    Color foreground = LinearToSRGB(SRGBToLinear(sprite->foreground)*light);

                    Rect2i glyph_rect = GetGlyphRect(font, sprite->glyph);
                    Bitmap glyph_bitmap = MakeBitmapView(&font->bitmap, glyph_rect);
                    BlitBitmapMaskSSE(&target, &glyph_bitmap, p, foreground, sprite->background);
                }
            } break;

            case RenderCommand_Rect:
            {
                Rect2i rect = command->rect;

                if (LayerUsesCamera((RenderLayer)at->layer))
                {
                    rect.min -= render_state->camera_bottom_left;
                    rect.max -= render_state->camera_bottom_left;
                }

                rect.min *= glyph_dim;
                rect.max *= glyph_dim;

                if (RectanglesOverlap(clip_rect, rect))
                {
                    rect.min -= clip_rect.min;
                    rect.max -= clip_rect.min;

                    BlitRect(&target, rect, command->color);
                }
            } break;
        }
    }
}

#if 0
static inline void
MergeSortInternal(uint32_t count, uint32_t *a, uint32_t *b)
{
    if (count <= 1)
    {
        // Nothing to do
    }
    else
    {
        uint32_t count_l = count / 2;
        uint32_t count_r = count - count_l;
        
        MergeSortInternal(count_l, b, a);
        MergeSortInternal(count_r, b + count_l, a + count_l);
        
        uint32_t *middle = b + count_l;
        uint32_t *end    = b + count;
        
        uint32_t *l = b;
        uint32_t *r = b + count_l;
        
        uint32_t *out = a;
        for (size_t i = 0; i < count; ++i)
        {
            if ((l < middle) &&
                ((r >= end) || (*l <= *r)))
            {
                *out++ = *l++;
            }
            else
            {
                *out++ = *r++;
            }
        }
    }
}

static inline void
MergeSort(uint32_t count, uint32_t *a, uint32_t *b)
{
    CopyArray(count, a, b);
    MergeSortInternal(count, a, b);
}
#endif

static inline void
RadixSort(uint32_t count, uint32_t *data, uint32_t *temp)
{
    uint32_t *source = data;
    uint32_t *dest = temp;

    for (int byte_index = 0; byte_index < 4; ++byte_index)
    {
        uint32_t offsets[256] = {};

        // NOTE: First pass - count how many of each key
        for (size_t i = 0; i < count; ++i)
        {
            uint32_t radix_piece = (source[i] >> 8*byte_index) & 0xFF;
            offsets[radix_piece] += 1;
        }

        // NOTE: Change counts to offsets
        uint32_t total = 0;
        for (size_t i = 0; i < ArrayCount(offsets); ++i)
        {
            uint32_t piece_count = offsets[i];
            offsets[i] = total;
            total += piece_count;
        }

        // NOTE: Second pass - place elements into dest in order
        for (size_t i = 0; i < count; ++i)
        {
            uint32_t radix_piece = (source[i] >> 8*byte_index) & 0xFF;
            dest[offsets[radix_piece]++] = source[i];
        }

        Swap(dest, source);
    }
}

static inline void
RenderCommandsToBitmap(Bitmap *target)
{
    Rect2i target_bounds = MakeRect2iMinDim(0, 0, target->w, target->h);

    uint32_t sort_key_count = (render_state->cb_size - render_state->cb_sort_key_at) / sizeof(RenderSortKey);
    RenderSortKey *sort_keys = (RenderSortKey *)(render_state->command_buffer + render_state->cb_sort_key_at);
    RenderSortKey *sort_scratch = PushArrayNoClear(render_state->arena, sort_key_count, RenderSortKey);
    RadixSort(sort_key_count, &sort_keys->u32, &sort_scratch->u32);

#if DUNGEONS_SLOW
    for (size_t i = 1; i < sort_key_count; ++i)
    {
        Assert(sort_keys[i].u32 > sort_keys[i - 1].u32);
    }
#endif

    int tile_count_x = 8;
    int tile_count_y = 8;
    int tile_count = tile_count_x*tile_count_y;
    TiledRenderJobParams *tiles = PushArray(render_state->arena, tile_count, TiledRenderJobParams);

    int tile_w = (target->w + tile_count_x - 1) / tile_count_x;
    int tile_h = (target->h + tile_count_y - 1) / tile_count_y;

    tile_w = ((tile_w + 3) / 4)*4;

    for (int tile_y = 0; tile_y < tile_count_y; ++tile_y)
    for (int tile_x = 0; tile_x < tile_count_x; ++tile_x)
    {
        int tile_index = tile_y*tile_count_x + tile_x;

        TiledRenderJobParams *params = &tiles[tile_index];
        params->target = target;

        Rect2i clip_rect = MakeRect2iMinDim(tile_x*tile_w, tile_y*tile_h, tile_w, tile_h);
        clip_rect = Intersect(clip_rect, target_bounds); 
        params->clip_rect = clip_rect;

        platform->AddJob(platform->high_priority_queue, params, TiledRenderJob);
    }

    platform->WaitForJobs(platform->high_priority_queue);
}

static void
EndRender(void)
{
    // if (render_state->command_buffer_hash != render_state->prev_command_buffer_hash)
    {
        RenderCommandsToBitmap(render_state->target);
    }
}

static inline void
PrintRenderCommandsUnderCursor(void)
{
    char *command_buffer = render_state->command_buffer;
    RenderSortKey *sort_keys = (RenderSortKey *)(command_buffer + render_state->cb_sort_key_at);

    RenderSortKey *end = (RenderSortKey *)(command_buffer + render_state->cb_size);
    int index = 0;
    for (RenderSortKey *at = sort_keys; at < end; at += 1)
    {
        RenderCommand *command = (RenderCommand *)(command_buffer + at->offset);

        Font *font = render_state->fonts[at->layer];
        V2i glyph_dim = GlyphDim(font);

        char *type = "Huh";
        Rect2i surface_area = {};
        switch (command->kind)
        {
            case RenderCommand_Sprite:
            {
                type = "sprite";

                V2i p = MakeV2i(command->p.x, command->p.y);

                if (at->layer == Layer_World || at->layer == Layer_Floor)
                {
                    p -= render_state->camera_bottom_left;
                }

                p *= glyph_dim;
                surface_area = MakeRect2iMinDim(p, glyph_dim);
            } break;

            case RenderCommand_Rect:
            {
                type = "rect";

                Rect2i rect = command->rect;

                if (at->layer == Layer_World || at->layer == Layer_Floor)
                {
                    rect.min -= render_state->camera_bottom_left;
                    rect.max -= render_state->camera_bottom_left;
                }

                rect.min *= glyph_dim;
                rect.max *= glyph_dim;
                surface_area = rect;
            } break;
        }

        if (IsInRect(surface_area, input->mouse_p))
        {
            platform->DebugPrint("type: %s, index: %d\n", type, index);
        }

        ++index;
    }
}
