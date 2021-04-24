static inline void
SetProperty(Entity *e, EntityPropertyKind property)
{
    if (e)
    {
        e->properties[property / 64] |= 1ull << (property % 64);
    }
}

static inline void
UnsetProperty(Entity *e, EntityPropertyKind property)
{
    if (e)
    {
        e->properties[property / 64] &= ~(1ull << (property % 64));
    }
}

static inline bool
HasProperty(Entity *e, EntityPropertyKind property)
{
    bool result = false;
    if (e)
    {
        result = !!(e->properties[property / 64] & (1ull << (property % 64)));
    }
    return result;
}

static inline Entity *
GetEntity(EntityHandle handle)
{
    Entity *slot = &entity_manager->entities[handle.index];
    if (slot->handle.generation == handle.generation)
    {
        return slot;
    }
    return nullptr;
}

static inline bool
RemoveEntityFromGrid(Entity *e)
{
    bool result = true;

    if ((e->p.x >= 0) &&
        (e->p.y >= 0) &&
        (e->p.x < WORLD_EXTENT_X) &&
        (e->p.y < WORLD_EXTENT_Y))
    {
        EntityHandle grid_slot = entity_manager->entity_grid[e->p.x][e->p.y];
        if (grid_slot == e->handle)
        {
            entity_manager->entity_grid[e->p.x][e->p.y] = {};
        }
        else
        {
            result = false;
        }
    }

    return result;
}

static inline bool
MoveEntity(Entity *e, V2i p)
{
    bool result = true;

    RemoveEntityFromGrid(e);

    if ((p.x >= 0) &&
        (p.y >= 0) &&
        (p.x < WORLD_EXTENT_X) &&
        (p.y < WORLD_EXTENT_Y))
    {
        if (entity_manager->entity_grid[p.x][p.y] == NullEntityHandle())
        {
            entity_manager->entity_grid[p.x][p.y] = e->handle;
            e->p = p;
        }
        else
        {
            result = false;
        }
    }

    return result;
}

static inline Entity *
AddEntity(V2i p, Sprite sprite)
{
    Entity *result = nullptr;
    for (size_t i = 0; i < entity_manager->entity_count; ++i)
    {
        Entity *e = &entity_manager->entities[i];
        if (!HasProperty(e, EntityProperty_Alive))
        {
            result = e;
        }
    }

    if (!result && (entity_manager->entity_count < MAX_ENTITY_COUNT))
    {
        result = &entity_manager->entities[entity_manager->entity_count++];
    }

    if (result)
    {
        uint32_t gen = result->handle.generation + 1;

        ZeroStruct(result);
        SetProperty(result, EntityProperty_Alive);
        result->handle = { (uint32_t)(result - entity_manager->entities), gen };
        result->p = p;
        result->health = 2;
        result->sprite = sprite;

        MoveEntity(result, p);
    }

    return result;
}

static inline Entity *
AddPlayer(V2i p)
{
    Entity *e = AddEntity(p, MakeSprite(Glyph_Dwarf2));
    SetProperty(e, EntityProperty_PlayerControlled);

    Assert(!entity_manager->player);
    entity_manager->player = e;

    return e;
}

static inline void
KillEntity(Entity *e)
{
    UnsetProperty(e, EntityProperty_Alive);
    RemoveEntityFromGrid(e);
    if (HasProperty(e, EntityProperty_PlayerControlled))
    {
        Assert(e == entity_manager->player);
        entity_manager->player = nullptr;
    }
}

static inline bool
NextEntity(Entity **entity_at)
{
    Entity *e = *entity_at;
    if (!e)
    {
        e = entity_manager->entities - 1;
    }

    Entity *end = entity_manager->entities + entity_manager->entity_count;
    while (e < end)
    {
        e += 1;
        if (HasProperty(e, EntityProperty_Alive))
        {
            *entity_at = e;
            return true;
        }
    }
    return false;
}

static inline Entity *
GetEntityAt(V2i p)
{
    Entity *result = nullptr;
    if (p.x > 0 && p.y > 0 && p.x < WORLD_EXTENT_X && p.y < WORLD_EXTENT_Y)
    {
        result = GetEntity(entity_manager->entity_grid[p.x][p.y]);
    }
    return result;
}

static inline Entity *
FindClosestEntity(V2i p, Entity *filter = nullptr)
{
    uint32_t best_dist = UINT32_MAX;
    Entity *result = nullptr;
    for (Entity *e = nullptr; NextEntity(&e);)
    {
        if (e == filter)
        {
            continue;
        }

        V2i delta = p - e->p;
        uint32_t dist = LengthSq(delta);
        if (best_dist > dist)
        {
            best_dist = dist;
            result = e;
        }
    }
    return result;
}

static inline bool
DamageEntity(Entity *e, int amount)
{
    if (!HasProperty(e, EntityProperty_Dying))
    {
        e->health -= amount;
        e->flash_timer = 0.2f;
        e->flash_color = MakeColor(255, 0, 0);
        if (e->health <= 0)
        {
            SetProperty(e, EntityProperty_Dying);
        }
        return true;
    }
    return false;
}

static inline Entity *
TraceLine(V2i start, V2i end, Sprite sprite = MakeSprite(0))
{
    Entity *result = nullptr;

    int x0 = start.x;
    int y0 = start.y;
    int x1 = end.x;
    int y1 = end.y;

    int dx =  Abs(x1 - x0);
    int dy = -Abs(y1 - y0);
    int err = dx + dy;

    int xi = (x1 > x0 ? 1 : -1);
    int yi = (y1 > y0 ? 1 : -1);

    int x = x0;
    int y = y0;

    for (;;)
    {
        V2i p = MakeV2i(x, y);

        if (!AreEqual(p, start))
        {
            Entity *entity_at = GetEntityAt(p);
            if (entity_at)
            {
                result = entity_at;
                break;
            }

            if (sprite.glyph)
            {
                DrawTile(p, sprite);
            }
        }

        if (x == x1 && y == y1)
        {
            break;
        }

        int err2 = 2*err;

        if (err2 >= dy)
        {
            err += dy;
            x += xi;
        }

        if (err2 <= dx)
        {
            err += dx;
            y += yi;
        }
    }

    return result;
}

struct PathNode
{
    PathNode *next, *prev;
    float cost;
    V2i p;
};

struct PathfindingState
{
    bool node_grid[WORLD_EXTENT_X][WORLD_EXTENT_Y];
};

static inline Path
FindPath(Arena *arena, V2i start, V2i target)
{
    Arena *temp_arena = GetTempArena();
    ScopedMemory temp(temp_arena);

    //
    // NOTE: This is Dijkstra, for now. A* is annoying, if this is fast enough let's keep things simple.
    //

    Path result = {};
    if (GetEntityAt(target))
    {
        return result;
    }

    PathfindingState *state = PushStruct(temp_arena, PathfindingState);

    static const V2i possible_moves[] =
    {
        MakeV2i(-1, -1), MakeV2i( 1, -1), MakeV2i(-1, 1), MakeV2i( 1,  1),
        MakeV2i(-1,  0), MakeV2i( 1,  0), MakeV2i( 0, 1), MakeV2i( 0, -1),
    };

    PathNode *first = PushStruct(temp_arena, PathNode);
    first->p = start;

    PathNode *queue = first;

    size_t node_count = 0;

    PathNode *target_node = nullptr;
    while (queue)
    {
        PathNode *top_node = queue;
        queue = queue->next;

        if (AreEqual(top_node->p, target))
        {
            target_node = top_node;
            break;
        }

        for (size_t i = 0; i < ArrayCount(possible_moves); ++i)
        {
            V2i p = top_node->p + possible_moves[i];
            if (state->node_grid[p.x][p.y])
            {
                continue;
            }
            state->node_grid[p.x][p.y] = true;

            if (!GetEntityAt(p))
            {
                PathNode *node = PushStruct(temp_arena, PathNode);
                node->prev = top_node;
                node->p = p;
                node->cost = top_node->cost + Length(possible_moves[i]); /* + DiagonalDistance(p, target); */

                // DrawTile(p, MakeSprite(Glyph_Tone25, MakeColor(0, 255, 255), MakeColor(0, 127, 127)));

                PathNode **insert_at = &queue;
                for (; *insert_at; insert_at = &(*insert_at)->next)
                {
                    PathNode *insert = *insert_at;
                    if (insert->cost > node->cost)
                    {
                        break;
                    }
                }

                node->next = *insert_at;
                *insert_at = node;

                node_count += 1;

                if (AreEqual(p, target))
                {
                    target_node = node;
                }
            }
        }
    }

    if (target_node)
    {
        uint32_t path_length = 0;
        for (PathNode *node = target_node; node; node = node->prev)
        {
            path_length += 1;
        }

        result.length = path_length;
        result.positions = PushArrayNoClear(arena, result.length, V2i);
        size_t i = path_length - 1;
        for (PathNode *node = target_node; node; node = node->prev)
        {
            result.positions[i--] = node->p;
        }
    }

    return result;
}

static inline bool
PlayerAct(void)
{
    Entity *player = entity_manager->player;
    if (!player)
    {
        return true;
    }

    V2i move = MakeV2i(0, 0);
    if (Triggered(controller->left))  move.x -= 1;
    if (Triggered(controller->right)) move.x += 1;
    if (Triggered(controller->up))    move.y += 1;
    if (Triggered(controller->down))  move.y -= 1;

    if (!AreEqual(move, MakeV2i(0, 0)))
    {
        V2i move_p = player->p + move;
        Entity *e_at_move_p = GetEntityAt(move_p);
        if (e_at_move_p)
        {
            if (HasProperty(e_at_move_p, EntityProperty_Invulnerable))
            {
                // blocked
            }
            else
            {
                DamageEntity(e_at_move_p, 1);
                return true;
            }
        }
        else
        {
            MoveEntity(player, move_p);
            return true;
        }
    }

    return false;
}

static inline void
UpdateAndRenderEntities(void)
{
    if (entity_manager->turn_timer <= 0.0f)
    {
        if (PlayerAct())
        {
            entity_manager->turn_timer += 0.05f;
            // simulate everybody else
        }
    }
    else
    {
        entity_manager->turn_timer -= platform->dt;
    }

    for (Entity *e = nullptr; NextEntity(&e);)
    {
        Sprite sprite = e->sprite;
        if (e->flash_timer >= 0.0f)
        {
            sprite.foreground = e->flash_color;
            e->flash_timer -= platform->dt;
        }

        if (e->flash_timer <= 0.0f && HasProperty(e, EntityProperty_Dying))
        {
            KillEntity(e);
        }

        DrawTile(e->p, sprite);
    }
}

static inline void
AddRoom(Rect2i rect)
{
    Sprite sprite = MakeSprite(Glyph_Tone50);
    for (int x = rect.min.x; x <= rect.max.x; ++x)
    {
        AddEntity(MakeV2i(x, rect.min.y), sprite);
        AddEntity(MakeV2i(x, rect.max.y), sprite);
    }
    for (int y = rect.min.y + 1; y <= rect.max.y - 1; ++y)
    {
        AddEntity(MakeV2i(rect.min.x, y), sprite);
        AddEntity(MakeV2i(rect.max.x, y), sprite);
    }
}
