#ifndef DUNGEONS_WORLDGEN_HPP
#define DUNGEONS_WORLDGEN_HPP

struct GenRoom
{
    GenRoom *next;
    Rect2i rect;
};

enum GenTile
{
    GenTile_Void = 0,
    GenTile_Room,
    GenTile_Corridor,
    GenTile_Door,
    GenTile_Wall,
    GenTile_RoomWall,
    GenTile_NotAllowed,
};

static inline bool
Walkable(GenTile tile)
{
    return ((tile == GenTile_Room) ||
            (tile == GenTile_Corridor));
}

struct GenTiles
{
    Arena arena;

    bool initialized;
    bool complete;

    RandomSeries entropy;

    int room_count;
    GenRoom *rooms;

    int w, h;
    GenTile *data;
    bool *seen_by_player;
};

static inline bool
SeenByPlayer(GenTiles *tiles, V2i p)
{
    if (tiles->data &&
        (p.x >= 0) && 
        (p.y >= 0) &&
        (p.x < tiles->w) &&
        (p.y < tiles->h))
    {
        return tiles->seen_by_player[p.y*tiles->w + p.x];
    }
    return false;
}

static inline void
SetSeenByPlayer(GenTiles *tiles, V2i p, bool value = true)
{
    if (tiles->data &&
        (p.x >= 0) && 
        (p.y >= 0) &&
        (p.x < tiles->w) &&
        (p.y < tiles->h))
    {
        tiles->seen_by_player[p.y*tiles->w + p.x] = value;
    }
}

static inline GenTile
GetTile(GenTiles *tiles, V2i p)
{
    if (tiles->data &&
        (p.x >= 0) && 
        (p.y >= 0) &&
        (p.x < tiles->w) &&
        (p.y < tiles->h))
    {
        return tiles->data[p.y*tiles->w + p.x];
    }
    else
    {
        return GenTile_NotAllowed;
    }
}

static inline void
SetTile(GenTiles *tiles, V2i p, GenTile value)
{
    Assert((p.x >= 0) && 
           (p.y >= 0) &&
           (p.x < tiles->w) &&
           (p.y < tiles->h));
    tiles->data[p.y*tiles->w + p.x] = value;
}

#endif /* DUNGEONS_WORLDGEN_HPP */
