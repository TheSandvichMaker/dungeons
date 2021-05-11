static const uint8_t perlin_permutations[] = 
{
    151,160,137,91,90,15,
    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
    77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
    135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
    5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
    223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
    129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
    251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,

    151,160,137,91,90,15,
    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
    77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
    135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
    5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
    223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
    129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
    251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
};

static inline float
PerlinGradient(int hash, float x, float y)
{
    switch(hash % 8)
    {
        case 0: return  x + y;
        case 1: return -x + y;
        case 2: return  x - y;
        case 3: return -x - y;
        case 4: return  x;
        case 5: return -x;
        case 6: return  y;
        case 7: return -y;
        default: return 0; // never happens
    }
}

static inline float
EvaluatePerlinNoise(float x, float y)
{
    int xi = (int)x;
    int yi = (int)y;

    float xf = x - (float)xi;
    float yf = y - (float)yi;

    xi &= 255;
    yi &= 255;

    float u = Smootherstep(xf);
    float v = Smootherstep(yf);

    int aa = perlin_permutations[perlin_permutations[xi    ] + yi    ];
    int ba = perlin_permutations[perlin_permutations[xi + 1] + yi    ];
    int ab = perlin_permutations[perlin_permutations[xi    ] + yi + 1];
    int bb = perlin_permutations[perlin_permutations[xi + 1] + yi + 1];

    float x0 = Lerp(PerlinGradient(aa, xf       , yf       ),
                    PerlinGradient(ba, xf - 1.0f, yf       ),
                    u);
    float x1 = Lerp(PerlinGradient(ab, xf       , yf - 1.0f),
                    PerlinGradient(bb, xf - 1.0f, yf - 1.0f),
                    u);
    float result = 0.5f + 0.5f*Lerp(x0, x1, v);
    return result;
}

static inline float
OctavePerlinNoise(float x, float y, int octaves, float persistence)
{
    float perlin = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float weight = 0.0f;
    for (int i = 0; i < octaves; ++i)
    {
        perlin += amplitude*EvaluatePerlinNoise(frequency*x, frequency*y);
        weight += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }
    perlin /= weight;
    return perlin;
}

static
PLATFORM_JOB(DoWorldGen)
{
    GenTiles *tiles = (GenTiles *)args;

    RandomSeries entropy = tiles->entropy;

    if (tiles->w % 2 == 0) tiles->w += 1;
    if (tiles->h % 2 == 0) tiles->h += 1;

    Arena *arena = &tiles->arena;
    tiles->data = PushArray(arena, tiles->w*tiles->h, GenTile);
    tiles->seen_by_player = PushArray(arena, tiles->w*tiles->h, bool);
    tiles->associated_rooms = PushArray(arena, tiles->w*tiles->h, GenRoom *);

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
            GenRoom *room = &tiles->rooms[tiles->room_count++];
            room->rect = room_rect;

            for (int y = room_rect.min.y; y < room_rect.max.y; ++y)
            for (int x = room_rect.min.x; x < room_rect.max.x; ++x)
            {
                V2i p = MakeV2i(x, y);
                SetTile(tiles, p, GenTile_Room);
                tiles->associated_rooms[IndexP(tiles, p)] = room;
            }

            bool pillars = true; // (RandomChoice(&entropy, 10) == 0);
            if (pillars)
            {
                int room_w = GetWidth(room_rect);
                int room_h = GetHeight(room_rect);
                int x_offset = room_w / 4;
                int y_offset = room_h / 4;
                SetTile(tiles, Corner00(room_rect) + MakeV2i( x_offset,  y_offset), GenTile_Wall);
                SetTile(tiles, Corner10(room_rect) + MakeV2i(-x_offset,  y_offset), GenTile_Wall);
                SetTile(tiles, Corner01(room_rect) + MakeV2i( x_offset, -y_offset), GenTile_Wall);
                SetTile(tiles, Corner11(room_rect) + MakeV2i(-x_offset, -y_offset), GenTile_Wall);
            }
        }
    }

    //
    // Place corridors
    //

    int max_dead_ends = 4096;
    int dead_end_count = 0;
    V2i *dead_ends = PushArray(arena, max_dead_ends, V2i);

    {
        int corridor_at = 0;
        V2i *corridor = PushArray(arena, tiles->w*tiles->h, V2i);

        V2i start_p;
        do
        {
            start_p = MakeV2i(1, 1) + 2*(RandomInRect(&entropy, map_bounds) / 2);
        }
        while (GetTile(tiles, start_p) != GenTile_Void);

        V2i at_p = start_p;
        SetTile(tiles, at_p, GenTile_Corridor);

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
                    SetTile(tiles, next_p, GenTile_Corridor);
                    SetTile(tiles, inbetween_p, GenTile_Corridor);

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
                    if (!backing_up && dead_end_count < max_dead_ends)
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
                    door_connects = (Walkable(GetTile(tiles, option + MakeV2i(1, 0))) &&
                                     Walkable(GetTile(tiles, option - MakeV2i(1, 0))) &&
                                     !Walkable(GetTile(tiles, option + MakeV2i(0, 1))) &&
                                     !Walkable(GetTile(tiles, option - MakeV2i(0, 1))));
                }
                if ((option.y == rect.min.y) ||
                    (option.y == rect.max.y))
                {
                    door_connects = (Walkable(GetTile(tiles, option + MakeV2i(0, 1))) &&
                                     Walkable(GetTile(tiles, option - MakeV2i(0, 1))) &&
                                     !Walkable(GetTile(tiles, option + MakeV2i(1, 0))) &&
                                     !Walkable(GetTile(tiles, option - MakeV2i(1, 0))));
                }

                if (door_connects)
                {
                    SetTile(tiles, option, GenTile_Door);
                    tiles->associated_rooms[IndexP(tiles, option)] = room;
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
            // if (RandomChoice(&entropy, 64) == 0)
            // {
            //     break;
            // }

            V2i directions[] =
            {
                MakeV2i(-1, 0), MakeV2i(1, 0), MakeV2i(0, -1), MakeV2i(0, 1),
            };

            int neighbor_count = 0;
            V2i neighbors[4];

            for (int direction_index = 0; direction_index < 4; ++direction_index)
            {
                V2i direction = directions[direction_index];
                V2i test_p = at_p + direction;
                if ((GetTile(tiles, test_p) == GenTile_Corridor) ||
                    (GetTile(tiles, test_p) == GenTile_Door))
                {
                    neighbors[neighbor_count++] = test_p;
                }
            }

            if (neighbor_count == 1)
            {
                SetTile(tiles, at_p, GenTile_Void);
                at_p = neighbors[0];
            }
#if 0
            else if (neighbor_count > 1)
            {
                // this doesn't seem vibin'
                bool create_loop = (RandomChoice(&entropy, 32) == 0);
                if (create_loop)
                {
                    SetTile(tiles, neighbors[RandomChoice(&entropy, neighbor_count)], GenTile_Void);
                    break;
                }
            }
#endif
            else
            {
                break;
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
        if (tile == GenTile_Void)
        {
            bool make_wall = false;
            bool borders_room = false;
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
                if (Walkable(test_tile))
                {
                    make_wall = true;
                    if (test_tile == GenTile_Room)
                    {
                        borders_room = true;
                    }
                }
            }
            if (make_wall)
            {
                SetTile(tiles, MakeV2i(x, y), borders_room ? GenTile_RoomWall : GenTile_Wall);
            }
        }
    }

    //
    // Make entities
    //

#if 1
    for (int y = 0; y < tiles->h; ++y)
    for (int x = 0; x < tiles->w; ++x)
    {
        V2i p = MakeV2i(x, y);
        GenTile tile = GetTile(tiles, p);

        Entity *e = nullptr;
        if (tile == GenTile_Wall)
        {
            e = AddWall(p);
        }
        else if (tile == GenTile_RoomWall)
        {
            e = AddWall(p);
            e->sprites[0] = MakeSprite(Glyph_Solid, MakeColor(196, 196, 196));
        }
        else if (tile == GenTile_Door)
        {
            e = AddDoor(p);
        }

        if (e)
        {
            GenRoom *room = tiles->associated_rooms[IndexP(tiles, p)];
            if (room)
            {
                PushToList(arena, &room->associated_entities, e->handle);
            }
        }
    }

    GenRoom *starting_room = &tiles->rooms[RandomChoice(&entropy, tiles->room_count)];
    V2i player_spawn_p = (starting_room->rect.min + starting_room->rect.max) / 2;
    AddPlayer(player_spawn_p);

    Entity *chest_key = AddEntity(StringLiteral("Chest Key"), player_spawn_p - MakeV2i(2, 0), MakeSprite(Glyph_Male, MakeColor(125, 255, 0)));
    SetContactTrigger(chest_key, Trigger_PickUp);
    
    Entity *chest = AddChest(player_spawn_p + MakeV2i(2, 0));
    LockWithKey(chest, chest_key);
    AddToInventory(chest, AddGold({}, 420));

    Entity *leet_gold = AddGold({}, 1337);
    leet_gold->name = StringLiteral("L33T G0LD");
    AddToInventory(chest, leet_gold);

    entity_manager->light_source = AddEntity("Torch"_str, player_spawn_p - MakeV2i(3, 5), MakeSprite('6', MakeColor(255, 192, 128)));

    AddOrc(player_spawn_p - MakeV2i(3, 3));

    Entity *key = AddEntity(StringLiteral("Shiny Key"), {}, MakeSprite(Glyph_Male, MakeColor(255, 200, 0)));
    AddToInventory(chest, key);

    for (EntityNode *node = starting_room->associated_entities.first;
         node;
         node = node->next)
    {
        Entity *e = EntityFromHandle(node->handle);
        if (e && HasProperty(e, EntityProperty_Door))
        {
            LockWithKey(e, key);
        }
    }
#else
    V2i player_spawn_p = MakeV2i(12, 12);
    AddPlayer(player_spawn_p);

    Rect2i room_rect = MakeRect2iMinDim(0, 0, 25, 25);
    for (int x = room_rect.min.x; x <= room_rect.max.x; x += 1)
    {
        AddWall(MakeV2i(x, room_rect.min.y));
        AddWall(MakeV2i(x, room_rect.max.y));
    }

    for (int y = room_rect.min.y; y <= room_rect.max.y; y += 1)
    {
        AddWall(MakeV2i(room_rect.min.x, y));
        AddWall(MakeV2i(room_rect.max.x, y));
    }

    AddWall(MakeV2i(11, 18));
    AddWall(MakeV2i(12, 18));
    AddWall(MakeV2i(13, 18));
#endif

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
            // Release(&tiles->arena);
            // *tiles_at = nullptr;
        }
    }

    return result;
}
