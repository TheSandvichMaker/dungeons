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
    GenTile_Wall,
    GenTile_Door,
    GenTile_NotAllowed,
};

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
    int *times_set;
};

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
    GenTile current = tiles->data[p.y*tiles->w + p.x];
    if (current == value)
    {
        tiles->times_set[p.y*tiles->w + p.x] += 1;
    }
    else
    {
        tiles->times_set[p.y*tiles->w + p.x] = 1;
    }
    tiles->data[p.y*tiles->w + p.x] = value;
}

#endif /* DUNGEONS_WORLDGEN_HPP */
