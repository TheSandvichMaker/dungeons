#include "dungeons.hpp"

#include "dungeons_string.cpp"
#include "dungeons_global_state.cpp"
#include "dungeons_image.cpp"
#include "dungeons_render.cpp"
#include "dungeons_entity.cpp"

// Ryan's text controls example: https://hatebin.com/ovcwtpsfmj

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

static inline StringContainer
PushEmptyStringContainer(Arena *arena, size_t capacity)
{
    StringContainer result = {};
    result.capacity = capacity;
    result.data = PushArray(arena, capacity, uint8_t);
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

static Font
LoadFontFromDisk(Arena *arena, String filename, int glyph_w, int glyph_h)
{
    Font result = {};

    Buffer file = platform->ReadFile(arena, filename);
    if (!file.size)
    {
        platform->ReportError(PlatformError_Nonfatal,
                              "Could not open file '%.*s' while loading font.", StringExpand(filename));
        return result;
    }

    Bitmap bitmap = ParseBitmap(file);
    if (!bitmap.data)
    {
        platform->ReportError(PlatformError_Nonfatal,
                              "Failed to parse bitmap '%.*s' while loading font.", StringExpand(filename));
        return result;
    }

    result = MakeFont(bitmap, glyph_w, glyph_h);
    return result;
}

void
App_UpdateAndRender(Platform *platform_)
{
    platform = platform_;

    GameState *game_state = (GameState *)platform->persistent_app_data;
    if (!platform->app_initialized)
    {
        game_state = BootstrapPushStruct(GameState, permanent_arena);
        platform->persistent_app_data = game_state;
    }
    
    if (platform->exe_reloaded)
    {
        RestoreGlobalState(&game_state->transient_arena);
    }

    if (!platform->app_initialized)
    {
        game_state->world_font = LoadFontFromDisk(&game_state->transient_arena, StringLiteral("font16x16.bmp"), 16, 16);
        game_state->ui_font    = LoadFontFromDisk(&game_state->transient_arena, StringLiteral("font8x16.bmp"), 8, 16);
        InitializeRenderState(&platform->backbuffer, &game_state->world_font, &game_state->ui_font);

        AddRoom(MakeRect2iMinDim(2, 2, 8, 8));
        AddEntity(MakeV2i(4, 4), MakeSprite(Glyph_Dwarf1, MakeColor(255, 0, 0)));

        platform->app_initialized = true;
    }

    V2i mouse_p = MakeV2i(platform->mouse_x, platform->mouse_y);

    ClearBitmap(&platform->backbuffer, MakeColor(0, 0, 0));
    UpdateAndRenderEntities();
}
