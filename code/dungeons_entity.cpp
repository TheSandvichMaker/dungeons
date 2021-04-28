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
    uint64_t slot = HashCoordinate(e->p) % ENTITY_HASH_SIZE;
    for (Entity **it_at = &entity_manager->entity_hash[slot];
         *it_at;
         it_at = &(*it_at)->next_on_tile)
    {
        Entity *it = *it_at;
        if (it->handle == e->handle)
        {
            Assert(it == e);

            *it_at = it->next_on_tile;
            it->next_on_tile = nullptr;

            return true;
        }
    }

    return false;
}

static inline bool
MoveEntity(Entity *e, V2i p)
{
    uint64_t slot = HashCoordinate(p) % ENTITY_HASH_SIZE;
    for (Entity *it = entity_manager->entity_hash[slot];
         it;
         it = it->next_on_tile)
    {
        if (HasProperty(it, EntityProperty_Blocking))
        {
            return false;
        }
    }

    RemoveEntityFromGrid(e);

    e->next_on_tile = entity_manager->entity_hash[slot];
    entity_manager->entity_hash[slot] = e;

    e->p = p;

    return true;
}

static inline Entity *
AddEntity(V2i p, Sprite sprite, EntityPropertySet initial_properties = {})
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

        SetProperties(result, initial_properties);
        SetProperty(result, EntityProperty_Alive);

        result->handle = { (uint32_t)(result - entity_manager->entities), gen };
        result->p = p;
        result->health = 2;
        result->sprites[result->sprite_count++] = sprite;

        MoveEntity(result, p);
    }

    return result;
}

static inline Entity *
AddWall(V2i p)
{
    Entity *e = AddEntity(p, MakeSprite(Glyph_Tone50));
    SetProperties(e, EntityProperty_Invulnerable|EntityProperty_Blocking);
    return e;
}

static inline Entity *
AddPlayer(V2i p)
{
    Entity *e = AddEntity(p, MakeSprite(Glyph_Dwarf2, MakeColor(255, 255, 0)));
    SetProperties(e, EntityProperty_PlayerControlled|EntityProperty_Blocking);

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

struct EntityIter
{
    EntityPropertySet filter;

    Entity *list;
    size_t next_offset;

    Entity *entity;
};

static inline void Next(EntityIter *iter);

static inline EntityIter
Reset(EntityIter iter)
{
    if (iter.list)
    {
        iter.entity = iter.list;
    }
    else
    {
        iter.entity = entity_manager->entities;
    }
    return iter;
}

static inline EntityIter
IterateEntities(EntityPropertySet filter = {})
{
    EntityPropertySet must_filter = {};

    EntityIter result = {};
    result.filter = SetProperty(result.filter, EntityProperty_Alive);
    result.filter = CombineSet(result.filter, filter);
    result.entity = entity_manager->entities;

    if (!HasProperties(result.entity, result.filter))
    {
        Next(&result);
    }

    return result;
}

static inline EntityIter
IterateEntities(EntityPropertyKind prop)
{
    return IterateEntities(MakeSet(prop));
}

#define IterateEntityList(list, next_pointer, ...) \
    IterateEntityList_(list, offsetof(Entity, next_pointer), ##__VA_ARGS__)
static inline EntityIter
IterateEntityList_(Entity *list, size_t next_offset, EntityPropertySet filter = {})
{
    EntityPropertySet must_filter = {};

    EntityIter result = {};
    result.filter = SetProperty(result.filter, EntityProperty_Alive);
    result.filter = CombineSet(result.filter, filter);

    result.list = list;
    result.next_offset = next_offset;

    result.entity = list;

    // NOTE: Let's warm up the iterator so if there are no entities with the
    // required properties, the iterator will start invalid.
    if (!HasProperties(result.entity, result.filter))
    {
        Next(&result);
    }

    return result;
}

static inline EntityIter
IterateEntityList_(Entity *list, size_t next_offset, EntityPropertyKind prop)
{
    return IterateEntityList_(list, next_offset, MakeSet(prop));
}

static inline bool
IsValid(const EntityIter &iter)
{
    return !!iter.entity;
}

static inline void
Next(EntityIter *iter)
{
    Entity *end = entity_manager->entities + entity_manager->entity_count;

    while (iter->entity)
    {
        if (iter->list)
        {
            iter->entity = *(Entity **)((char *)iter->entity + iter->next_offset);
        }
        else
        {
            iter->entity += 1;
            if (iter->entity >= end)
            {
                iter->entity = nullptr;
            }
        }

        if (HasProperties(iter->entity, iter->filter))
        {
            break;
        }
    }
}

static inline EntityIter
GetEntitiesAt(V2i p, EntityPropertySet filter = {})
{
    Entity *list = entity_manager->entity_hash[HashCoordinate(p) % ENTITY_HASH_SIZE];
    EntityIter result = IterateEntityList(list, next_on_tile, filter);
    return result;
}

static inline EntityIter
GetEntitiesAt(V2i p, EntityPropertyKind prop)
{
    return GetEntitiesAt(p, MakeSet(prop));
}

static inline bool
TileBlocked(V2i p)
{
    EntityIter iter = GetEntitiesAt(p, MakeSet(EntityProperty_Blocking));
    return IsValid(iter);
}

static inline Entity *
FindClosestEntity(V2i p, EntityPropertyKind required_property, Entity *filter = nullptr)
{
    uint32_t best_dist = UINT32_MAX;
    Entity *result = nullptr;
    for (EntityIter iter = IterateEntities(); IsValid(iter); Next(&iter))
    {
        Entity *e = iter.entity;

        if (e == filter)
        {
            continue;
        }

        if (required_property != EntityProperty_None &&
            !HasProperty(e, required_property))
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

static inline EntityIter
TraceLine(V2i start, V2i end, Sprite sprite = MakeSprite(0))
{
    EntityIter result = {};

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
            EntityIter on_tile = GetEntitiesAt(p, EntityProperty_Blocking);
            if (IsValid(on_tile))
            {
                result = on_tile;
                break;
            }

            if (sprite.glyph)
            {
                DrawTile(Draw_World, p, sprite);
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
    bool node_hash[512];
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

            uint64_t slot = HashCoordinate(p) % ArrayCount(state->node_hash);
            if (state->node_hash[slot])
            {
                continue;
            }
            state->node_hash[slot] = true;

            EntityIter blocking_entities = GetEntitiesAt(p, EntityProperty_Blocking);
            if (!IsValid(blocking_entities) || AreEqual(p, target))
            {
                PathNode *node = PushStruct(temp_arena, PathNode);
                node->prev = top_node;
                node->p = p;
                node->cost = top_node->cost + Length(possible_moves[i]); /* + DiagonalDistance(p, target); */

                // DrawTile(Draw_World, p, MakeSprite(Glyph_Tone25, MakeColor(0, 255, 255), MakeColor(0, 127, 127)));

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
            }
        }
    }

    if (target_node)
    {
        uint32_t path_length = 0;
        for (PathNode *node = target_node; node && node != first; node = node->prev)
        {
            path_length += 1;
        }

        result.length = path_length;
        result.positions = PushArrayNoClear(arena, result.length, V2i);
        size_t i = path_length - 1;
        for (PathNode *node = target_node; node && node != first; node = node->prev)
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
    if (Triggered(input->north))     move += MakeV2i( 0,  1);
    if (Triggered(input->northeast)) move += MakeV2i( 1,  1);
    if (Triggered(input->east))      move += MakeV2i( 1,  0);
    if (Triggered(input->southeast)) move += MakeV2i( 1, -1);
    if (Triggered(input->south))     move += MakeV2i( 0, -1);
    if (Triggered(input->southwest)) move += MakeV2i(-1, -1);
    if (Triggered(input->west))      move += MakeV2i(-1,  0);
    if (Triggered(input->northwest)) move += MakeV2i(-1,  1);

    if (!AreEqual(move, MakeV2i(0, 0)))
    {
        V2i move_p = player->p + move;
        
        bool blocked = false;
        for (EntityIter at_move_p = GetEntitiesAt(move_p); IsValid(at_move_p); Next(&at_move_p))
        {
            Entity *e = at_move_p.entity;
            if (HasProperty(e, EntityProperty_Blocking))
            {
                if (HasProperty(e, EntityProperty_Invulnerable))
                {
                    // blocked with no recourse
                    blocked = true;
                }
                else
                {
                    DamageEntity(e, 1);
                    return true;
                }
            }
        }

        if (!blocked)
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
            // entity_manager->turn_timer += 0.10f;
            for (EntityIter it = IterateEntities(EntityProperty_Martins); IsValid(it); Next(&it))
            {
                Entity *e = it.entity;

                Clear(&entity_manager->turn_arena);

                Path best_path = {};
                best_path.length = UINT32_MAX;

                Entity *best_c = nullptr;

                for (EntityIter c_it = IterateEntities(EntityProperty_C); IsValid(c_it); Next(&c_it))
                {
                    Entity *c = c_it.entity;

                    Path path = FindPath(&entity_manager->turn_arena, e->p, c->p);
                    if (path.length > 0 && path.length < best_path.length)
                    {
                        best_c = c;
                        best_path = path;
                    }
                }

                if (best_c)
                {
                    V2i new_p = best_path.positions[0];
                    if (AreEqual(new_p, best_c->p))
                    {
                        DamageEntity(best_c, 999);
                    }
                    else
                    {
                        MoveEntity(e, new_p);
                    }
                }
            }
        }
    }
    else
    {
        entity_manager->turn_timer -= platform->dt;
    }

    for (EntityIter it = IterateEntities(); IsValid(it); Next(&it))
    {
        Entity *e = it.entity;

        Sprite sprite = e->sprites[e->sprite_index];
        if (e->sprite_anim_timer >= e->sprite_anim_rate)
        {
            e->sprite_anim_timer -= e->sprite_anim_rate;
            e->sprite_index = e->sprite_index + 1;
            if (e->sprite_index >= e->sprite_count)
            {
                e->sprite_index = 0;
                e->sprite_anim_timer -= e->sprite_anim_pause_time;
            }
        }
        e->sprite_anim_timer += platform->dt;

        if (e->flash_timer >= 0.0f)
        {
            sprite.foreground = e->flash_color;
            e->flash_timer -= platform->dt;
        }

        if (e->flash_timer <= 0.0f && HasProperty(e, EntityProperty_Dying))
        {
            KillEntity(e);
        }

        DrawTile(Draw_World, e->p, sprite);
    }
}

static inline void
AddRoom(Rect2i rect)
{
    for (int x = rect.min.x; x <= rect.max.x; ++x)
    {
        AddWall(MakeV2i(x, rect.min.y));
        AddWall(MakeV2i(x, rect.max.y));
    }
    for (int y = rect.min.y + 1; y <= rect.max.y - 1; ++y)
    {
        AddWall(MakeV2i(rect.min.x, y));
        AddWall(MakeV2i(rect.max.x, y));
    }
}
