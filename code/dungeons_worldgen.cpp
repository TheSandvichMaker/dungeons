static
PLATFORM_JOB(DoWorldGen)
{
    GenTiles *tiles = (GenTiles *)args;

    RandomSeries entropy = tiles->entropy;

    Arena *arena = &tiles->arena;
    tiles->data = PushArray(arena, tiles->w*tiles->h, GenTile);
    tiles->times_set = PushArray(arena, tiles->w*tiles->h, int);

    //
    // Place rooms
    //

    int min_dim = 8;
    int max_dim = 32;

    tiles->room_count = 0;
    tiles->rooms = PushArray(arena, 100, GenRoom);

    for (size_t attempt = 0; attempt < 100; ++attempt)
    {
        V2i room_p = MakeV2i(RandomRange(&entropy, 0, tiles->w - min_dim),
                             RandomRange(&entropy, 0, tiles->h - min_dim));

        V2i room_dim = MakeV2i(RandomRange(&entropy, min_dim, max_dim),
                               RandomRange(&entropy, min_dim, max_dim));

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

    // V2i *corridor = PushArray(arena, 1024, V2i);

#if 0
    for (int y = 1; y < tiles->h - 1; y += 2)
    for (int x = 1; x < tiles->w - 1; x += 2)
    {
        //
        // See if we can start a corridor
        //

        bool suitable_start = true;
        for (int search_y = y - 1; search_y <= y + 1; ++search_y)
        for (int search_x = x - 1; search_x <= x + 1; ++search_x)
        {
            GenTile tile = GetTile(tiles, MakeV2i(search_x, search_y));
            if (tile != GenTile_Void)
            {
                suitable_start = false;
                break;
            }
        }

        if (!suitable_start)
        {
            continue;
        }

        //
        // Do a corridor
        //

        V2i at_p = MakeV2i(x, y);
        SetTile(tiles, at_p, GenTile_Room);

        // int corridor_at = 0;

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
                size_t direction_index = RandomChance(&entropy, directions_left);
                directions_left -= 1;

                V2i direction = directions[direction_index];
                V2i next_p = at_p + direction;

                for (size_t i = direction_index; i < ArrayCount(directions) - 1; ++i)
                {
                    directions[i] = directions[i + 1];
                }

                if ((next_p.x < 1) ||
                    (next_p.y < 1) ||
                    (next_p.x >= tiles->w - 1) ||
                    (next_p.y >= tiles->h - 1))
                {
                    continue;
                }

                V2i perp = Perpendicular(direction);
                V2i inv_perp = MakeV2i(-perp.x, -perp.y); // TODO: Unary minus for vectors

                V2i test_directions[5];
                test_directions[0] = direction;
                test_directions[1] = perp;
                test_directions[2] = inv_perp;

                test_directions[3] = direction;
                if (test_directions[3].x == 0) { test_directions[3].x = perp.x; }
                if (test_directions[3].y == 0) { test_directions[3].y = perp.y; }

                test_directions[4] = direction;
                if (test_directions[4].x == 0) { test_directions[4].x = inv_perp.x; }
                if (test_directions[4].y == 0) { test_directions[4].y = inv_perp.y; }

                bool suitable_direction = true;
                for (size_t test_index = 0;
                     test_index < ArrayCount(test_directions);
                     ++test_index)
                {
                    V2i test_direction = test_directions[test_index];
                    V2i test_p = next_p + test_direction;
                    if (!AreEqual(test_p, at_p))
                    {
                        if (GetTile(tiles, test_p) != GenTile_Void)
                        {
                            suitable_direction = false;
                            break;
                        }
                    }
                }

                if (suitable_direction)
                {
                    at_p = next_p;
                    SetTile(tiles, at_p, GenTile_Room);
                    could_advance = true;
                    break;
                }
            }

            if (!could_advance)
            {
                break;
            }
        }
    }
#else
    for (int i = 0; i < 100; ++i)
    {
        int room_1_index = RandomChance(&entropy, tiles->room_count);
        int room_2_index;
        do
        {
            room_2_index = RandomChance(&entropy, tiles->room_count);
        }
        while (room_2_index == room_1_index);

        GenRoom *room_1 = &tiles->rooms[room_1_index];
        GenRoom *room_2 = &tiles->rooms[room_2_index];
        V2i start = (room_1->rect.min + room_1->rect.max) / 2;
        V2i end = (room_2->rect.min + room_2->rect.max) / 2;
        V2i at = start;

        int sleep_counter = 0;
        while (!IsInRect(room_2->rect, at))
        {
            V2i directions[] =
            {
                MakeV2i(-1, 0), MakeV2i(1, 0), MakeV2i(0, -1), MakeV2i(0, 1),
            };
            V2i direction = directions[RandomChance(&entropy, ArrayCount(directions))];

            V2i next_p = at + direction;
            if (next_p.x < 0) next_p.x += tiles->w;
            if (next_p.y < 0) next_p.y += tiles->h;
            if (next_p.x >= tiles->w) next_p.x -= tiles->w;
            if (next_p.y >= tiles->h) next_p.y -= tiles->h;

            at = next_p;
            SetTile(tiles, at, GenTile_Room);

            sleep_counter += 1;
            if (sleep_counter > 1000)
            {
                sleep_counter -= 1000;
                platform->SleepThread(1);
            }
        }
    }
#endif

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

    bool result = tiles->complete;
    if (result)
    {
        Release(&tiles->arena);
        *tiles_at = nullptr;
    }

    return result;
}
