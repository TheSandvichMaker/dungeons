#include "dungeons.hpp"

#include "dungeons_string.cpp"
#include "dungeons_global_state.cpp"
#include "dungeons_image.cpp"
#include "dungeons_render.cpp"
#include "dungeons_controller.cpp"
#include "dungeons_entity.cpp"
#include "dungeons_worldgen.cpp"

// Ryan's text controls example: https://hatebin.com/ovcwtpsfmj

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
        // game_state->tileset    = LoadFontFromDisk(&game_state->transient_arena, StringLiteral("tileset.bmp"), 16, 16);
        game_state->world_font = LoadFontFromDisk(&game_state->transient_arena, StringLiteral("font16x16_alt1.bmp"), 16, 16);
        game_state->ui_font    = LoadFontFromDisk(&game_state->transient_arena, StringLiteral("font8x16.bmp"), 8, 16);
        InitializeRenderState(&game_state->transient_arena, &platform->backbuffer, &game_state->world_font, &game_state->ui_font);
        InitializeInputBindings();

        game_state->gen_tiles = BeginGenerateWorld(0xDEADBEFC);

        platform->app_initialized = true;
    }

    HandleInput();

    if (!game_state->world_generated)
    {
        game_state->world_generated = EndGenerateWorld(&game_state->gen_tiles);
    }

    BeginRender();

    if (game_state->world_generated)
    {
        UpdateAndRenderEntities();

        Entity *player = entity_manager->player;

        PushRectOutline(Layer_Ui, MakeRect2iMinDim(2, render_state->ui_top_right.y - 14, 36, 13), COLOR_WHITE, COLOR_BLACK);

        Bitmap *target = render_state->target;
        Font *world_font = render_state->world_font;
        int viewport_w = (target->w + world_font->glyph_w - 1) / world_font->glyph_w;
        int viewport_h = (target->h + world_font->glyph_h - 1) / world_font->glyph_h;

        Rect2i viewport = MakeRect2iMinDim(render_state->camera_bottom_left, MakeV2i(viewport_w, viewport_h));
        for (int y = viewport.min.y; y < viewport.max.y; y += 1)
        for (int x = viewport.min.x; x < viewport.max.x; x += 1)
        {
            GenTile tile = GetTile(game_state->gen_tiles, MakeV2i(x, y));
            if (tile == GenTile_Room)
            {
                float perlin = OctavePerlinNoise(64.0f + 0.01f*(float)x, 64.0f + 0.02f*(float)y, 6, 0.75f);
                Sprite sprite;
                if (perlin < 0.45f)
                {
                    sprite = MakeSprite(Glyph_Tone25, LinearToSRGB(perlin*perlin*MakeColorF(0.75f, 0.35f, 0.0f)));
                }
                else
                {
                    sprite = MakeSprite('=', LinearToSRGB(perlin*perlin*MakeColorF(0.85f, 0.45f, 0.0f)));
                }
                PushTile(Layer_Floor, MakeV2i(x, y), sprite);
            }
            else
            {
                float perlin = OctavePerlinNoise(0.01f*(float)x, 0.01f*(float)y, 6, 0.75f);
                perlin = (perlin > 0.5f ? 1.0f : 0.0f);
                V3 color = Lerp(Square(MakeV3(0.25f, 0.15f, 0.0f)), Square(MakeV3(0.1f, 0.25f, 0.1f)), perlin);
                if (tile == GenTile_Corridor)
                {
                    color *= 3.0f;
                }
                Color foreground = LinearToSRGB(color);
                PushTile(Layer_Floor, MakeV2i(x, y), MakeSprite(Glyph_Tone25, foreground));
            }
        }

        if (player)
        {
            render_state->camera_bottom_left = player->p - MakeV2i(42, 20);

            V2i at_p = MakeV2i(4, render_state->ui_top_right.y - 3);
            for (Entity *item = player->first_in_inventory;
                 item;
                 item = item->next_in_inventory)
            {
                PushText(Layer_Ui, at_p,
                         FormatTempString("Item: %.*s", StringExpand(item->name)),
                         COLOR_WHITE, COLOR_BLACK);
                at_p.y -= 1;
            }
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
}
