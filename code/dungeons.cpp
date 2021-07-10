#include "dungeons.hpp"

#include "dungeons_string.cpp"
#include "dungeons_global_state.cpp"
#include "dungeons_image.cpp"
#include "dungeons_render.cpp"
#include "dungeons_controller.cpp"
#include "dungeons_entity.cpp"
#include "dungeons_worldgen.cpp"

DebugTable *debug_table;

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

static inline void
CalculateLightMap(LightMap *light_map)
{
    enum TileKind
    {
        Tile_None,
        Tile_Wall,
        Tile_Floor,
    };

    struct Row
    {
        Row *next;
        int at_col;
        int depth;
        float start_slope;
        float end_slope;
        int min_col;
        int max_col;
    };

    Arena *arena = platform->GetTempArena();
    ScopedMemory temp(arena);

    size_t bounce_list_size = 2*WORLD_SIZE_X + 2*WORLD_SIZE_Y;
    Entity **first_bounce_list = PushArray(arena, bounce_list_size, Entity *);
    Entity **second_bounce_list = PushArray(arena, bounce_list_size, Entity *);

    size_t source_count = 0;
    Entity **source = first_bounce_list;

    size_t dest_count = 0;
    Entity **dest = second_bounce_list;

    UNUSED_VARIABLE(dest_count);
    UNUSED_VARIABLE(dest);

    Entity *light_source = entity_manager->light_source;
    source[source_count++] = light_source;

    if (light_source)
    {
        V3 light_color = SRGBToLinear(light_source->sprites[light_source->sprite_index].foreground);
        V2i origin = light_source->p;

        ScopedMemory quadrant_temp(arena);
        for (int quadrant = 0; quadrant < 4; quadrant += 1)
        {
            TileKind prev_tile = Tile_None;

            Row *first_row = PushStruct(arena, Row);
            first_row->depth = 1;
            first_row->start_slope = -1.0f;
            first_row->end_slope = 1.0f;
            first_row->at_col = RoundUp((float)first_row->depth*first_row->start_slope);

            while (first_row)
            {
                Row *row = first_row;
                first_row = row->next;

                int max_col = RoundDown((float)row->depth*row->end_slope);
                for (int col = row->at_col; col <= max_col; col += 1)
                {
                    TileKind tile = Tile_None;
                    bool is_symmetric = (((float)col >= (float)row->depth*row->start_slope) &&
                                         ((float)col <= (float)row->depth*row->end_slope));

                    row->at_col = col + 1;

                    V2i rel_p = MakeV2i(col, row->depth);
                    V2i abs_p = rel_p;
                    switch (quadrant)
                    {
                        case 0: abs_p = origin + MakeV2i( rel_p.x,  rel_p.y); break;
                        case 1: abs_p = origin + MakeV2i( rel_p.x, -rel_p.y); break;
                        case 2: abs_p = origin + MakeV2i( rel_p.y,  rel_p.x); break;
                        case 3: abs_p = origin + MakeV2i(-rel_p.y,  rel_p.x); break;
                        INVALID_DEFAULT_CASE;
                    }

                    if ((abs_p.x < 0) ||
                        (abs_p.y < 0) ||
                        (abs_p.x >= WORLD_SIZE_X) ||
                        (abs_p.y >= WORLD_SIZE_Y))
                    {
                        prev_tile = Tile_None;
                        continue;
                    }

                    if (is_symmetric)
                    {
                        tile = Tile_Floor;
                        float length = (float)LengthSq(rel_p);
                        float light_strength = 1.0f / Max(1.0f, length);
                        light_map->map[abs_p.y][abs_p.x] += light_strength*light_color;
                    }
                    else
                    {
                        Entity *entities_on_tile = entity_manager->entity_grid[abs_p.x][abs_p.y];
                        for (Entity *e = entities_on_tile;
                             e;
                             e = e->next_on_tile)
                        {
                            if (HasProperty(e, EntityProperty_BlockSight))
                            {
                                tile = Tile_Wall;
                                float length = (float)LengthSq(rel_p);
                                float light_strength = 1.0f / Max(1.0f, length);
                                light_map->map[abs_p.y][abs_p.x] += light_strength*light_color;
                                break;
                            }
                        }
                    }

                    if ((prev_tile == Tile_Wall) && (tile == Tile_Floor))
                    {
                        row->start_slope = Slope(rel_p);
                        row->at_col = RoundUp((float)row->depth*row->start_slope);
                    }

                    if ((prev_tile == Tile_Floor) && (tile == Tile_Wall))
                    {
                        Row *next_row = PushStruct(arena, Row);
                        CopyStruct(row, next_row);
                        next_row->depth += 1;
                        next_row->end_slope = Slope(rel_p);

                        next_row->next = first_row;
                        first_row = next_row;
                    }

                    prev_tile = tile;
                }

                if (prev_tile == Tile_Floor)
                {
                    Row *next_row = PushStruct(arena, Row);
                    CopyStruct(row, next_row);
                    next_row->at_col = RoundUp((float)row->depth*row->start_slope);
                    next_row->depth += 1;

                    next_row->next = first_row;
                    first_row = next_row;
                }
            }
        }
    }
}

void
AppUpdateAndRender(Platform *platform_)
{
    platform = platform_;
#if DUNGEONS_INTERNAL
    debug_table = platform->debug_table;
#endif

    platform->mutex.lock();
    platform->DebugPrint("Hello\n");
    platform->mutex.unlock();

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
        WarmUpEntityVisibilityGrids();
    }

    BeginRender();

    if (game_state->world_generated)
    {
        UpdateAndRenderEntities();

        Entity *player = entity_manager->player;
        Rect2i viewport = render_state->viewport;
        for (int y = viewport.min.y; y < viewport.max.y; y += 1)
        for (int x = viewport.min.x; x < viewport.max.x; x += 1)
        {
            V2i p = MakeV2i(x, y);
            GenTile tile = GetTile(game_state->gen_tiles, p);
            if (game_state->debug_fullbright || SeenByPlayer(game_state->gen_tiles, MakeV2i(x, y)))
            {
                VisibilityGrid *grid = player ? player->visibility_grid : nullptr;
                bool currently_visible = game_state->debug_fullbright || IsVisible(grid, p);
                float visibility_mod = currently_visible ? 1.0f : 0.5f;

                if (tile == GenTile_Room)
                {
                    float perlin = OctavePerlinNoise(64.0f + 0.01f*(float)x, 64.0f + 0.02f*(float)y, 6, 0.75f);
                    Sprite sprite;
                    if (perlin < 0.45f)
                    {
                        sprite = MakeSprite(Glyph_Tone25, LinearToSRGB(perlin*perlin*visibility_mod*MakeColorF(0.75f, 0.35f, 0.0f)));
                    }
                    else
                    {
                        sprite = MakeSprite('=', LinearToSRGB(perlin*perlin*visibility_mod*MakeColorF(0.85f, 0.45f, 0.0f)));
                    }
                    DrawTile(Layer_Ground, MakeV2i(x, y), sprite);
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
                    DrawTile(Layer_Ground, MakeV2i(x, y), MakeSprite(Glyph_Tone25, foreground));
                }
            }
        }

        if (player)
        {
            StringList list = {};
            for (Entity *e: Linearize(&player->inventory))
            {
                EntityToString(e, &list);
            }
            if (!IsEmpty(&list))
            {
                V2i p = MakeV2i(2, render_state->ui_top_right.y - 2);

                Rect2i bounds = GetDrawnStringListBounds(Layer_Ui, p, &list);
                DrawRectOutline(Layer_Ui, GrowOutwardHalfDim(bounds, MakeV2i(1, 1)), COLOR_WHITE, COLOR_BLACK);

                DrawStringList(Layer_Ui, p, &list);
            }
        }

        V2i at_p = MakeV2i(40, render_state->ui_top_right.y - 3);
        for (PlatformLogLine *line = platform->GetFirstLogLine();
             line;
             line = platform->GetNextLogLine(line))
        {
            DrawText(Layer_Ui, at_p, line->string, COLOR_WHITE, COLOR_BLACK);
            at_p.y -= 1;
        }

        {
            StringList list = {};
            for (Entity *e = GetEntitesOnTile(input->world_mouse_p);
                 e;
                 e = e->next_on_tile)
            {
                if (e->seen_by_player)
                {
                    EntityToString(e, &list);
                }
            }

            if (!IsEmpty(&list))
            {
                V2i p = WorldToUi(MakeV2i(0, 1) + input->world_mouse_p);

                StringRenderSpec spec = {};
                spec.y_align_percentage = 1.0f;

                Rect2i bounds = GetDrawnStringListBounds(Layer_Ui, p, &list, spec);
                DrawRect(Layer_Ui, bounds, COLOR_BLACK);

                DrawStringList(Layer_Ui, p, &list, spec);
            }
        }

        {
            StringList list = {};
            int count = 0;
            for (Entity *e = GetEntitesOnTile(player->p);
                 e;
                 e = e->next_on_tile)
            {
                if (e->handle == player->handle)
                {
                    continue;
                }

                count += 1;
            }
            if (count > 0)
            {
                size_t i = 0;
                for (Entity *e = GetEntitesOnTile(player->p);
                     e;
                     e = e->next_on_tile)
                {
                    if (e->handle != player->handle)
                    {
                        if (entity_manager->looking_at_ground &&
                            (i == entity_manager->container_selection_index))
                        {
                            PushTempStringF(&list, ">");
                        }
                        EntityToString(e, &list);
                        i += 1;
                    }
                }
            }
            if (!IsEmpty(&list))
            {
                V2i p = WorldToUi(player->p + MakeV2i(0, 1));

                StringRenderSpec spec = {};
                spec.y_align_percentage = 1.0f;

                Rect2i bounds = GetDrawnStringListBounds(Layer_Ui, p, &list, spec);
                DrawRect(Layer_Ui, bounds, COLOR_BLACK);

                DrawStringList(Layer_Ui, p, &list, spec);
            }
        }

        if (entity_manager->looking_at_container)
        {
            Entity *container = entity_manager->looking_at_container;

            StringList list = {};
            EntityArray inventory = Linearize(&container->inventory);
            for (size_t i = 0; i < inventory.count; i += 1)
            {
                Entity *e = inventory[i];
                if (i == entity_manager->container_selection_index)
                {
                    PushTempStringF(&list, ">");
                }
                EntityToString(e, &list);
            }

            if (!IsEmpty(&list))
            {
                V2i p = WorldToUi(container->p + MakeV2i(0, 1));

                StringRenderSpec spec = {};
                spec.y_align_percentage = 1.0f;

                Rect2i bounds = GetDrawnStringListBounds(Layer_Ui, p, &list, spec);
                DrawRect(Layer_Ui, bounds, COLOR_BLACK);

                DrawStringList(Layer_Ui, p, &list, spec);
            }
        }
    }

    // CalculateLightMap(&render_state->light_map);
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

#if DUNGEONS_INTERNAL
#include "dungeons_debug.cpp"
#endif
