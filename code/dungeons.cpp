#include "dungeons.hpp"

#include "dungeons_string.cpp"
#include "dungeons_global_state.cpp"
#include "dungeons_image.cpp"
#include "dungeons_render.cpp"
#include "dungeons_controller.cpp"
#include "dungeons_entity.cpp"
#include "dungeons_worldgen.cpp"

// Ryan's text controls example: https://hatebin.com/ovcwtpsfmj

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
AppUpdateAndRender(Platform *platform_)
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
        RestoreGlobalState(&game_state->global_state, &game_state->transient_arena);
    }

    if (!platform->app_initialized)
    {
        game_state->world_font = LoadFontFromDisk(&game_state->transient_arena, StringLiteral("font16x16.bmp"), 16, 16);
        game_state->ui_font    = LoadFontFromDisk(&game_state->transient_arena, StringLiteral("font8x16.bmp"), 8, 16);
        InitializeRenderState(&platform->backbuffer, &game_state->world_font, &game_state->ui_font);

#if 1
        AddRoom(MakeRect2iMinDim(1, 1, 16, 16));

        Entity *martins = AddEntity(MakeV2i(4, 4), MakeSprite(Glyph_Dwarf1));
        SetProperty(martins, EntityProperty_Martins|EntityProperty_Invulnerable);

        AddPlayer(MakeV2i(5, 6));

        RandomSeries entropy = MakeRandomSeries(0);
        for (size_t i = 0; i < 15; ++i)
        {
            for (size_t attempt = 0; attempt < 100; ++attempt)
            {
                V2i spawn_p = MakeV2i(RandomRange(&entropy, 2, 16),
                                      RandomRange(&entropy, 2, 16));
                if (!GetEntityAt(spawn_p))
                {
                    Entity *c = AddEntity(spawn_p, MakeSprite('c'));
                    SetProperty(c, EntityProperty_C);
                    if (0 == RandomChance(&entropy, 4))
                    {
                        c->sprite_anim_rate = 0.25f;
                        c->sprite_anim_pause_time = 1.0f;
                        c->sprites[1] = MakeSprite('c');
                        c->sprites[2] = MakeSprite('+', MakeColor(255, 0, 0));
                        c->sprites[3] = MakeSprite('+', MakeColor(0, 255, 0));
                        c->sprite_count = 4;
                    }
                    break;
                }
            }
        }
#else
        GenerateWorld(0xDEADBEEF);
#endif

        platform->app_initialized = true;
    }

    HandleController();

    static bool erase = false;
    if (Pressed(controller->alt_interact))
    {
        if (GetEntityAt(controller->world_mouse_p))
        {
            erase = true;
        }
        else
        {
            erase = false;
        }
    }

    if (controller->alt_interact.ended_down)
    {
        Entity *e = GetEntityAt(controller->world_mouse_p);
        if (erase && e)
        {
            KillEntity(e);
        }
        else if (!e)
        {
            AddWall(controller->world_mouse_p);
        }
    }

    BeginRender(Draw_World);
    UpdateAndRenderEntities();
    EndRender();
}
