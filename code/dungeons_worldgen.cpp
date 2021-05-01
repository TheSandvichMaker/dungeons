static
PLATFORM_JOB(DoWorldGen)
{
    GenTiles *tiles = (GenTiles *)args;

    RandomSeries entropy = tiles->entropy;

    if (tiles->w % 2 == 0) tiles->w += 1;
    if (tiles->h % 2 == 0) tiles->h += 1;

    Arena *arena = &tiles->arena;
    tiles->data = PushArray(arena, tiles->w*tiles->h, GenTile);
    tiles->times_set = PushArray(arena, tiles->w*tiles->h, int);

    Rect2i map_bounds = MakeRect2iMinDim(0, 0, tiles->w % 2, tiles->h % 2);

    //
    // Place rooms
    //

    int min_dim = 6;
    int max_dim = 32;

    int room_attempts = 1000;

    tiles->room_count = 0;
    tiles->rooms = PushArray(arena, room_attempts, GenRoom);

    for (size_t attempt = 0; attempt < room_attempts; ++attempt)
    {
        V2i room_p = MakeV2i(RandomRange(&entropy, 0, tiles->w - min_dim),
                             RandomRange(&entropy, 0, tiles->h - min_dim));
        room_p = MakeV2i(1, 1) + 2*(room_p / 2);

        V2i room_dim = MakeV2i(RandomRange(&entropy, min_dim, max_dim),
                               RandomRange(&entropy, min_dim, max_dim));
        room_dim = MakeV2i(1, 1) + 2*(room_dim / 2);

        Rect2i room_rect = MakeRect2iMinDim(room_p, room_dim);
        room_rect = Intersect(room_rect, MakeRect2iMinDim(1, 1, tiles->w - 2, tiles->h - 2));

        bool overlap = false;
        for (int y = room_rect.min.y - 1; y < room_rect.max.y + 1; ++y)
        for (int x = room_rect.min.x - 1; x < room_rect.max.x + 1; ++x)
        {
            GenTile tile = tiles->data[y*tiles->w + x];
            if (tile != GenTile_Void)
            {
                overlap = true;
                break;
            }
        }

        if (!overlap)
        {
            for (int y = room_rect.min.y; y < room_rect.max.y; ++y)
            for (int x = room_rect.min.x; x < room_rect.max.x; ++x)
            {
                tiles->data[y*tiles->w + x] = GenTile_Room;
            }
            GenRoom *room = &tiles->rooms[tiles->room_count++];
            room->rect = room_rect;
        }
    }

    //
    // Place corridors
    //

    int dead_end_count = 0;
    V2i *dead_ends = PushArray(arena, 1024, V2i);

    {
        int corridor_at = 0;
        V2i *corridor = PushArray(arena, tiles->w*tiles->h, V2i);

        V2i start_p;
        do
        {
            start_p = MakeV2i(1, 1) + 2*(RandomInRect(&entropy, map_bounds) / 2);
        }
        while (GetTile(tiles, start_p) == GenTile_Room);

        V2i at_p = start_p;
        SetTile(tiles, at_p, GenTile_Room);

        bool backing_up = false;
        for (;;)
        {
            int directions_left = 4;
            V2i directions[] =
            {
                MakeV2i(-1, 0), MakeV2i(1, 0), MakeV2i(0, -1), MakeV2i(0, 1),
            };

            bool could_advance = false;
            while (directions_left)
            {
                size_t direction_index = RandomChoice(&entropy, directions_left);
                directions_left -= 1;

                V2i direction = directions[direction_index];
                V2i next_p = at_p + 2*direction;
                V2i inbetween_p = at_p + direction;

                for (size_t i = direction_index; i < ArrayCount(directions) - 1; ++i)
                {
                    directions[i] = directions[i + 1];
                }

                if ((GetTile(tiles, next_p) == GenTile_Void) &&
                    (GetTile(tiles, inbetween_p) == GenTile_Void))
                {
                    SetTile(tiles, next_p, GenTile_Room);
                    SetTile(tiles, inbetween_p, GenTile_Room);

                    corridor[corridor_at++] = next_p;

                    at_p = next_p;
                    could_advance = true;

                    break;
                }
            }

            if (could_advance)
            {
                backing_up = false;
            }
            else
            {
                if (corridor_at > 0)
                {
                    if (!backing_up && dead_end_count < 1024)
                    {
                        dead_ends[dead_end_count++] = at_p;
                    }
                    at_p = corridor[--corridor_at];
                    backing_up = true;
                }
                else
                {
                    break;
                }
            }
        }
    }

    //
    // Cull dead ends
    //

    for (int dead_end_index = 0; dead_end_index < dead_end_count; ++dead_end_index)
    {
        V2i at_p = dead_ends[dead_end_index];

        for (;;)
        {
            if (RandomChoice(&entropy, 64) == 0)
            {
                break;
            }

            V2i directions[] =
            {
                MakeV2i(-1, 0), MakeV2i(1, 0), MakeV2i(0, -1), MakeV2i(0, 1),
            };

            int neighbor_count = 0;
            V2i next_p = MakeV2i(0, 0);
            for (int direction_index = 0; direction_index < 4; ++direction_index)
            {
                V2i direction = directions[direction_index];
                V2i test_p = at_p + direction;
                if (GetTile(tiles, test_p) == GenTile_Room)
                {
                    neighbor_count += 1;
                    next_p = at_p + direction;
                }
            }

            if (neighbor_count == 1)
            {
                SetTile(tiles, at_p, GenTile_Void);
                at_p = next_p;
            }
            else
            {
                break;
            }
        }
    }
    
    //
    // Add doors to rooms
    //

    for (int room_index = 0; room_index < tiles->room_count; ++room_index)
    {
        GenRoom *room = &tiles->rooms[room_index];
        Rect2i rect = room->rect;

        rect.min.x -= 1;
        rect.min.y -= 1;

        ScopedMemory temp(arena);

        int option_count = 0;
        V2i *options = PushArray(arena, 2*(GetWidth(rect) - 2) + 2*(GetHeight(rect) - 2), V2i);

        for (int x = rect.min.x; x < rect.max.x; ++x)
        {
            options[option_count++] = MakeV2i(x, rect.min.y);
            options[option_count++] = MakeV2i(x, rect.max.y);
        }

        for (int y = rect.min.y; y < rect.max.y; ++y)
        {
            options[option_count++] = MakeV2i(rect.min.x, y);
            options[option_count++] = MakeV2i(rect.max.x, y);
        }

        int door_count = DiceRoll(&entropy, 3);
        for (int door_index = 0; door_index < door_count; ++door_index)
        {
            while (option_count > 0)
            {
                int option_index = RandomChoice(&entropy, option_count);
                V2i option = options[option_index];

                for (size_t i = option_index; i < option_count - 1; ++i)
                {
                    options[i] = options[i + 1];
                }

                option_count -= 1;

                bool door_connects = false;
                if ((option.x == rect.min.x) ||
                    (option.x == rect.max.x))
                {
                    door_connects = ((GetTile(tiles, option + MakeV2i(1, 0)) == GenTile_Room) &&
                                     (GetTile(tiles, option - MakeV2i(1, 0)) == GenTile_Room) &&
                                     (GetTile(tiles, option + MakeV2i(0, 1)) == GenTile_Void) &&
                                     (GetTile(tiles, option - MakeV2i(0, 1)) == GenTile_Void));
                }
                if ((option.y == rect.min.y) ||
                    (option.y == rect.max.y))
                {
                    door_connects = ((GetTile(tiles, option + MakeV2i(0, 1)) == GenTile_Room) &&
                                     (GetTile(tiles, option - MakeV2i(0, 1)) == GenTile_Room) &&
                                     (GetTile(tiles, option + MakeV2i(1, 0)) == GenTile_Void) &&
                                     (GetTile(tiles, option - MakeV2i(1, 0)) == GenTile_Void));
                }

                if (door_connects)
                {
                    SetTile(tiles, option, GenTile_Room);
                    break;
                }
            }
        }
    }

    //
    // Add walls
    //

    for (int y = 0; y < tiles->h; ++y)
    for (int x = 0; x < tiles->w; ++x)
    {
        GenTile tile = GetTile(tiles, MakeV2i(x, y));
        if (tile == GenTile_Room)
        {
            for (int search_y = y - 1; search_y <= y + 1; ++search_y)
            for (int search_x = x - 1; search_x <= x + 1; ++search_x)
            {
                if ((search_x == x) &&
                    (search_y == y))
                {
                    continue;
                }

                V2i search_p = MakeV2i(search_x, search_y);
                GenTile test_tile = GetTile(tiles, search_p);
                if (test_tile == GenTile_Void)
                {
                    SetTile(tiles, search_p, GenTile_Wall);
                }
            }
        }
    }

    //
    // Make entities
    //

    for (int y = 0; y < tiles->h; ++y)
    for (int x = 0; x < tiles->w; ++x)
    {
        V2i p = MakeV2i(x, y);
        GenTile tile = GetTile(tiles, p);

        if (tile == GenTile_Wall)
        {
            AddWall(p);
        }
    }

    tiles->complete = true;
}

static inline GenTiles *
BeginGenerateWorld(uint64_t seed)
{
    GenTiles *tiles = BootstrapPushStruct(GenTiles, arena, Megabytes(4));

    tiles->w = 256;
    tiles->h = 256;
    tiles->entropy = MakeRandomSeries(seed);

    platform->AddJob(platform->low_priority_queue, tiles, DoWorldGen);

    return tiles;
}

static inline bool
EndGenerateWorld(GenTiles **tiles_at)
{
    GenTiles *tiles = *tiles_at;

    bool result = false;
    if (tiles)
    {
        result = tiles->complete;
        if (result)
        {
            Release(&tiles->arena);
            *tiles_at = nullptr;
        }
    }

    return result;
}
