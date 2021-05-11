#include "dungeons.hpp"

#include "dungeons_string.cpp"
#include "dungeons_global_state.cpp"
#include "dungeons_image.cpp"
#include "dungeons_render.cpp"
#include "dungeons_controller.cpp"
#include "dungeons_entity.cpp"
#include "dungeons_worldgen.cpp"

// Ryan's text controls example: https://hatebin.com/ovcwtpsfmj

static inline void
SetDebugDelay(int milliseconds, int frame_count)
{
    game_state->debug_delay = milliseconds;
    game_state->debug_delay_frame_count = frame_count;
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

DUNGEONS_INLINE Color
LinearToSRGB(V3 linear)
{
    Color result = MakeColor((uint8_t)(SquareRoot(linear.x)*255.0f),
                             (uint8_t)(SquareRoot(linear.y)*255.0f),
                             (uint8_t)(SquareRoot(linear.z)*255.0f));
    return result;
}

void
AppUpdateAndRender(Platform *platform_)
{
    platform = platform_;

    game_state = (GameState *)platform->persistent_app_data;
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
        // game_state->tileset    = LoadFontFromDisk(&game_state->transient_arena, "tileset.bmp"_str, 16, 16);
        game_state->world_font = LoadFontFromDisk(&game_state->transient_arena, "font16x16_alt1.bmp"_str, 16, 16);
        game_state->ui_font    = LoadFontFromDisk(&game_state->transient_arena, "font8x16.bmp"_str, 8, 16);
        InitializeRenderState(&game_state->transient_arena, &platform->backbuffer, &game_state->world_font, &game_state->ui_font);
        InitializeInputBindings(&game_state->transient_arena);

        game_state->gen_tiles = BeginGenerateWorld(0xDEADBEFC);

        platform->app_initialized = true;
    }

    if (Pressed(input->f_keys[2]))
    {
        platform->DebugPauseThread();
    }

    HandleInput();

    if (Pressed(input->f_keys[1]))
    {
        game_state->debug_fullbright = !game_state->debug_fullbright;
    }

    if (!game_state->world_generated)
    {
        game_state->world_generated = EndGenerateWorld(&game_state->gen_tiles);
    }

    BeginRender();

    if (game_state->world_generated)
    {
        UpdateAndRenderEntities();

        Bitmap *target = render_state->target;
        Font *world_font = render_state->world_font;

        Entity *player = entity_manager->player;
        if (player)
        {
            V2i render_tile_dim = MakeV2i(platform->render_w / world_font->glyph_w, platform->render_h / world_font->glyph_h);
            render_state->camera_bottom_left = player->p - render_tile_dim / 2;
        }

        int viewport_w = (target->w + world_font->glyph_w - 1) / world_font->glyph_w;
        int viewport_h = (target->h + world_font->glyph_h - 1) / world_font->glyph_h;

        Entity *torch = entity_manager->light_source;
        VisibilityGrid torch_grid = PushVisibilityGrid(platform->GetTempArena(), MakeRect2iCenterHalfDim(torch->p, MakeV2i(12, 12)));
        CalculateVisibilityRecursiveShadowcast(&torch_grid, torch);

        Rect2i viewport = MakeRect2iMinDim(render_state->camera_bottom_left, MakeV2i(viewport_w, viewport_h));
        for (int y = viewport.min.y; y < viewport.max.y; y += 1)
        for (int x = viewport.min.x; x < viewport.max.x; x += 1)
        {
            V2i p = MakeV2i(x, y);
            GenTile tile = GetTile(game_state->gen_tiles, p);
            if (game_state->debug_fullbright || SeenByPlayer(game_state->gen_tiles, MakeV2i(x, y)))
            {
                VisibilityGrid *grid = &entity_manager->player_visibility;
                bool currently_visible = game_state->debug_fullbright || IsVisible(grid, p);
                float visibility_mod = currently_visible ? 1.0f : 0.5f;

                V3 light = MakeV3(0.25f);
                if (IsVisible(&torch_grid, p))
                {
                    light += MakeV3(1.0f, 0.8f, 0.5f)*(1.0f / Max(1.0f, Length(p - torch->p)));
                }

                if (tile == GenTile_Room)
                {
                    float perlin = OctavePerlinNoise(64.0f + 0.01f*(float)x, 64.0f + 0.02f*(float)y, 6, 0.75f);
                    Sprite sprite;
                    if (perlin < 0.45f)
                    {
                        sprite = MakeSprite(Glyph_Tone25, LinearToSRGB(light*perlin*perlin*visibility_mod*MakeColorF(0.75f, 0.35f, 0.0f)));
                    }
                    else
                    {
                        sprite = MakeSprite('=', LinearToSRGB(light*perlin*perlin*visibility_mod*MakeColorF(0.85f, 0.45f, 0.0f)));
                    }
                    PushTile(Layer_Ground, MakeV2i(x, y), sprite);
                }
                else
                {
                    float perlin = OctavePerlinNoise(0.01f*(float)x, 0.01f*(float)y, 6, 0.75f);
                    perlin = (perlin > 0.5f ? 1.0f : 0.0f);
                    V3 color = visibility_mod*Lerp(Square(MakeV3(0.25f, 0.15f, 0.0f)), Square(MakeV3(0.1f, 0.25f, 0.1f)), perlin);
                    if (tile == GenTile_Corridor)
                    {
                        color *= 3.0f;
                    }
                    Color foreground = LinearToSRGB(color);
                    PushTile(Layer_Ground, MakeV2i(x, y), MakeSprite(Glyph_Tone25, foreground));
                }
            }
        }

        if (player)
        {
            DrawEntityList(&player->inventory, MakeRect2iMinDim(2, render_state->ui_top_right.y - 4, 36, 13));
        }

        V2i at_p = MakeV2i(40, render_state->ui_top_right.y - 3);
        for (PlatformLogLine *line = platform->GetFirstLogLine();
             line;
             line = platform->GetNextLogLine(line))
        {
            PushText(Layer_Ui, at_p, line->string, COLOR_WHITE, COLOR_BLACK);
            at_p.y -= 1;
        }
    }

    EndRender();

    if (Pressed(input->interact))
    {
        PrintRenderCommandsUnderCursor();
    }

    if (game_state->debug_delay_frame_count > 0)
    {
        platform->SleepThread(game_state->debug_delay);
        game_state->debug_delay_frame_count -= 1;
    }
}
