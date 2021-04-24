struct GenRoom
{
    GenRoom *next;
    Rect2i rect;
};

static void
GenerateWorld(uint64_t seed)
{
    Arena *arena = GetTempArena();
    ScopedMemory temp(arena);

    RandomSeries entropy = MakeRandomSeries(seed);

    GenRoom *first_room = nullptr;

    for (size_t i = 0; i < 16; ++i)
    {
        for (size_t attempt = 0; attempt < 100; ++attempt)
        {
            V2i room_p = MakeV2i(RandomRange(&entropy, 1, 100),
                                 RandomRange(&entropy, 1, 100));

            V2i room_dim = MakeV2i(RandomRange(&entropy, 6, 16),
                                   RandomRange(&entropy, 6, 16));

            Rect2i room_rect = MakeRect2iMinDim(room_p, room_dim);

            bool overlap = false;
            for (GenRoom *test_room = first_room;
                 test_room;
                 test_room = test_room->next)
            {
                if (RectanglesOverlap(test_room->rect, room_rect))
                {
                    overlap = true;
                    break;
                }
            }

            if (!overlap)
            {
                AddRoom(room_rect);

                GenRoom *room = PushStruct(arena, GenRoom);
                room->next = first_room;
                first_room = room;

                room->rect = room_rect;

                break;
            }
        }
    }
}
