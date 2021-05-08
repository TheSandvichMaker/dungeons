static inline EntityNode *
NewEntityNode(Entity *entity)
{
    EntityNode *result = nullptr;
    if (entity)
    {
        if (!entity_manager->first_free_entity_node)
        {
            entity_manager->first_free_entity_node = PushStructNoClear(&entity_manager->arena, EntityNode);
        }
        result = SllStackPop(entity_manager->first_free_entity_node);
        result->next = nullptr;
        result->handle = entity->handle;
    }
    else
    {
        INVALID_CODE_PATH;
    }
    return result;
}

static inline void
FreeEntityNode(EntityNode *node)
{
    SllStackPush(entity_manager->first_free_entity_node, node);
}

static inline bool
IsInWorld(V2i p)
{
    return ((p.x >= 0) &&
            (p.y >= 0) &&
            (p.x < WORLD_SIZE_X) &&
            (p.y < WORLD_SIZE_Y));
}

static inline Entity *
EntityFromHandle(EntityHandle handle)
{
    Entity *slot = &entity_manager->entities[handle.index];
    if (slot->handle == handle)
    {
        return slot;
    }
    return nullptr;
}

static inline EntityHandle
HandleFromEntity(Entity *entity)
{
    return entity->handle;
}

static inline bool
RemoveEntityFromGrid(Entity *e)
{
    Assert(IsInWorld(e->p));
    for (Entity **it_at = &entity_manager->entity_grid[e->p.x][e->p.y];
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
    Assert(IsInWorld(e->p));
    Assert(IsInWorld(p));
    for (Entity *it = entity_manager->entity_grid[p.x][p.y];
         it;
         it = it->next_on_tile)
    {
        if (HasProperty(it, EntityProperty_Blocking))
        {
            return false;
        }
    }

    RemoveEntityFromGrid(e);

    e->next_on_tile = entity_manager->entity_grid[p.x][p.y];
    entity_manager->entity_grid[p.x][p.y] = e;

    e->p = p;

    return true;
}

static inline void
SetContactTrigger(Entity *e, TriggerKind kind)
{
    e->contact_trigger = kind;
}

static inline Entity *
AddEntity(String name, V2i p, Sprite sprite, EntityPropertySet initial_properties = {})
{
    Entity *result = nullptr;
    if (entity_manager->first_free_entity)
    {
        result = entity_manager->first_free_entity;
        entity_manager->first_free_entity = result->next_free;
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
        SetProperty(result, EntityProperty_InWorld);

        result->handle = { (uint32_t)(result - entity_manager->entities), gen };
        result->name = name;
        result->p = p;
        result->health = 2;
        result->sprites[result->sprite_count++] = sprite;
        result->speed = 100;
        result->amount = 1;

        MoveEntity(result, p);
    }

    return result;
}

static inline Entity *
AddWall(V2i p)
{
    Entity *e = AddEntity(StringLiteral("Wall"), p, MakeSprite(Glyph_Tone50, MakeColor(110, 165, 100)));
    SetProperties(e, EntityProperty_Invulnerable|EntityProperty_Blocking);
    return e;
}

static inline Entity *
AddDoor(V2i p)
{
    Entity *e = AddEntity(StringLiteral("Door"), p, MakeSprite('#', MakeColor(255, 127, 0)));
    SetProperties(e, EntityProperty_Invulnerable|EntityProperty_Door|EntityProperty_Unlockable|EntityProperty_Blocking);
    SetContactTrigger(e, Trigger_Unblock);
    return e;
}

static inline Entity *
AddPlayer(V2i p)
{
    Entity *e = AddEntity(StringLiteral("Player"), p, MakeSprite('@', MakeColor(255, 255, 0)));
    SetProperties(e, EntityProperty_PlayerControlled|EntityProperty_Blocking);

    Assert(!entity_manager->player);
    entity_manager->player = e;

    return e;
}

static inline Entity *
AddGold(V2i p, int amount)
{
    Entity *e = AddEntity(StringLiteral("Gold"), p, MakeSprite('g', MakeColor(255, 255, 0)));
    e->amount = amount;
    e->sprite_anim_rate = 0.25f;
    e->sprite_anim_pause_time = 1.75f;
    e->sprites[e->sprite_count++] = MakeSprite('g', MakeColor(255, 255, 0));
    e->sprites[e->sprite_count++] = MakeSprite('g', MakeColor(255, 255, 127));
    e->sprites[e->sprite_count++] = MakeSprite('g', MakeColor(255, 255, 255));
    SetContactTrigger(e, Trigger_PickUp);
    return e;
}

static inline Entity *
AddChest(V2i p)
{
    Entity *e = AddEntity(StringLiteral("Chest"), p, MakeSprite('M', MakeColor(127, 255, 0)));
    SetProperties(e, EntityProperty_Unlockable|EntityProperty_Invulnerable|EntityProperty_Blocking);
    SetContactTrigger(e, Trigger_Container);
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
    e->next_free = entity_manager->first_free_entity;
    entity_manager->first_free_entity = e;
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
IterateAllEntities(EntityPropertySet filter = {})
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
IterateAllEntities(EntityPropertyKind prop)
{
    return IterateAllEntities(MakeSet(prop));
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
    EntityIter result = {};
    if (IsInWorld(p))
    {
        Entity *list = entity_manager->entity_grid[p.x][p.y];
        result = IterateEntityList(list, next_on_tile, filter);
    }
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
    for (EntityIter iter = IterateAllEntities(); IsValid(iter); Next(&iter))
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
                PushTile(Layer_World, p, sprite);
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
    // NOTE: No protection against collisions. Spicy!!!
    bool node_hash[1024];
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

static inline EntityArray
Linearize(EntityList *list)
{
    int count = 0;
    for (EntityNode *node = list->first; node; node = node->next)
    {
        count += 1;
    }

    EntityArray result = PushArrayContainer<Entity *>(GetTempArena(), count);

    for (EntityNode *node = list->first; node; node = node->next)
    {
        Entity *e = EntityFromHandle(node->handle);
        if (e)
        {
            Push(&result, e);
        }
    }

    return result;
}

static inline EntityArray
Pull(EntityList *list)
{
    int count = 0;
    for (EntityNode *node = list->first; node; node = node->next)
    {
        count += 1;
    }

    EntityArray result = PushArrayContainer<Entity *>(GetTempArena(), count);

    while (list->first)
    {
        EntityNode *node = SllQueuePop(list->first, list->last);

        Entity *e = EntityFromHandle(node->handle);
        if (e && HasProperty(e, EntityProperty_Alive))
        {
            Push(&result, e);
        }

        FreeEntityNode(node);
    }
    list->first = list->last = nullptr;

    return result;
}

static inline void
Place(EntityList *list, EntityArray array)
{
    for (size_t i = 0; i < array.count; ++i)
    {
        EntityNode *node = NewEntityNode(array[i]);
        SllQueuePush(list->first, list->last, node);
    }
}

static inline void
CleanStaleReferences(EntityList *list)
{
    // NOTE: Because pulling the list already filters out invalid handles,
    // we can just rely on that. Cheeky. Performant? Maybe not. Whatever. @Speed
    EntityArray array = Pull(list);
    Place(list, array);
}

static inline void
AddToInventory(Entity *e, Entity *item)
{
    RemoveEntityFromGrid(item);
    UnsetProperty(item, EntityProperty_InWorld);

    EntityNode *node = NewEntityNode(item);
    SllQueuePush(e->inventory.first, e->inventory.last, node);
}

static inline void
LockWithKey(Entity *e, Entity *key)
{
    Assert(e->required_key == NullEntityHandle());
    e->required_key = key->handle;
    e->open = false;
    SetProperty(e, EntityProperty_Unlockable);
}

static inline bool
TryOpen(Entity *e, Entity *other)
{
    bool open = false;
    if (e->required_key == NullEntityHandle())
    {
        open = true;
    }
    else
    {
        ForSll (item, other->inventory.first)
        {
            if (item->handle == e->required_key)
            {
                Entity *key = EntityFromHandle(item->handle);
                KillEntity(key);
                open = true;
            }
        }
    }

    if (open)
    {
        e->open = true;
        if (HasProperty(e, EntityProperty_Door))
        {
            UnsetProperty(e, EntityProperty_Blocking);
        }
    }

    return open;
}

static inline void
InteractWithContainer(Entity *player, Entity *container)
{
    EntityArray items = Pull(&container->inventory);

    if (Triggered(input->north)) entity_manager->container_selection_index -= 1;
    if (Triggered(input->south)) entity_manager->container_selection_index += 1;
    entity_manager->container_selection_index %= items.count;

    if (Triggered(input->east))
    {
        Entity *taken_item = RemoveOrdered(&items, entity_manager->container_selection_index);
        AddToInventory(player, taken_item);
    }

    Place(&container->inventory, items);
}

static inline void
DrawEntityList(EntityList *list, Rect2i rect, int highlight_index = -1)
{
    EntityArray items = Linearize(list);
    rect.max.y = Min(rect.max.y, rect.min.y + (int)items.count + 2);
    rect.max.y = Max(rect.min.y + 3, rect.max.y);

    PushRectOutline(Layer_Ui, rect, COLOR_WHITE, COLOR_BLACK);

    V2i at_p = MakeV2i(rect.min.x + 3, rect.max.y - 2);

    int item_index = 0;
    for (size_t i = 0; i < items.count; ++i)
    {
        Entity *item = items[i];

        Color text_color = MakeColor(127, 127, 127);
        if (highlight_index == -1 || item_index == highlight_index)
        {
            text_color = COLOR_WHITE;
        }

        PushTile(Layer_Ui, at_p - MakeV2i(1, 0), item->sprites[item->sprite_index]);

        String text = FormatTempString(" %4d %.*s ", item->amount, StringExpand(item->name));
        PushText(Layer_Ui, at_p, text, text_color, COLOR_BLACK);

        at_p.y -= 1;
        item_index += 1;
    }

    if (!items.count)
    {
        Color text_color = MakeColor(127, 127, 127);
        String text = FormatTempString("  Nothing here...");
        PushText(Layer_Ui, at_p, text, text_color, COLOR_BLACK);
    }
}

static inline void
ProcessTrigger(Entity *e, Entity *other)
{
    if (HasProperty(e, EntityProperty_Unlockable))
    {
        TryOpen(e, other);

        if (!e->open)
        {
            // Must be open to interact with it!
            return;
        }
    }

    switch (e->contact_trigger)
    {
        case Trigger_Unblock:
        {
            UnsetProperty(e, EntityProperty_Blocking);
        } break;

        case Trigger_Container:
        {
            entity_manager->looking_at_container = e;
        } break;

        case Trigger_PickUp:
        {
            AddToInventory(other, e);
        } break;
    }
}

static inline bool
PlayerAct(void)
{
    Entity *player = entity_manager->player;
    if (!player)
    {
        return true;
    }

    player->energy += player->speed;
    if (player->energy < 100)
    {
        return false;
    }

    if (Triggered(input->here))
    {
        entity_manager->looking_at_container = nullptr;
    }

    if (entity_manager->looking_at_container)
    {
        if (!entity_manager->looking_at_container->inventory.first)
        {
            entity_manager->looking_at_container = nullptr;
        }
    }

    if (entity_manager->looking_at_container)
    {
        InteractWithContainer(player, entity_manager->looking_at_container);
        return false;
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

    bool result = false;
    if (!AreEqual(move, MakeV2i(0, 0)))
    {
        V2i move_p = player->p + move;

        entity_manager->looking_at_container = nullptr;
        
        bool blocked = false;
        for (EntityIter at_move_p = GetEntitiesAt(move_p); IsValid(at_move_p); Next(&at_move_p))
        {
            Entity *e = at_move_p.entity;

            if (e->contact_trigger)
            {
                ProcessTrigger(e, player);
            }

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

    if (Triggered(input->here))
    {
        return true;
    }

    return result;
}

static inline VisibilityGrid
PushVisibilityGrid(Arena *arena, Rect2i bounds)
{
    VisibilityGrid result = {};
    result.bounds = bounds;

    int width = GetWidth(bounds);
    int height = GetHeight(bounds);
    result.tiles = PushArray(arena, width*height, bool);

    return result;
}

static inline void
CalculateVisibility(VisibilityGrid *grid, Entity *e)
{
    int w = GetWidth(grid->bounds);
    int h = GetHeight(grid->bounds);
    for (int y = 0; y <= h; y += 1)
    for (int x = 0; x <= w; x += 1)
    {
        V2i p = MakeV2i(x + grid->bounds.min.x, y + grid->bounds.min.y);
        EntityIter hit_entities = TraceLine(e->p, p);
        if (IsValid(hit_entities))
        {
            grid->tiles[y*w + x] = true;
            for (; IsValid(hit_entities); Next(&hit_entities))
            {
                Entity *seen = hit_entities.entity;
                seen->seen_by_player = true;
            }
        }
        else
        {
            grid->tiles[y*w + x] = false;
            SetSeenByPlayer(game_state->gen_tiles, p, true);
            for (EntityIter on_tile = GetEntitiesAt(p);
                 IsValid(on_tile);
                 Next(&on_tile))
            {
                Entity *seen = on_tile.entity;
                seen->seen_by_player = true;
            }
        }
    }
}

static inline void
UpdateAndRenderEntities(void)
{
    if (entity_manager->turn_timer <= 0.0f)
    {
        if (entity_manager->player)
        {
            int player_view_radius = 16;
            entity_manager->player_visibility = PushVisibilityGrid(GetTempArena(), MakeRect2iCenterHalfDim(entity_manager->player->p, MakeV2i(player_view_radius)));
            CalculateVisibility(&entity_manager->player_visibility, entity_manager->player);
        }

        if (PlayerAct())
        {
            // entity_manager->turn_timer += 0.10f;
            for (EntityIter it = IterateAllEntities(EntityProperty_Martins); IsValid(it); Next(&it))
            {
                Entity *e = it.entity;

                e->energy += e->speed;
                if (e->energy < 100)
                {
                    continue;
                }

                while (e->energy >= 100)
                {
                    Clear(&entity_manager->turn_arena);

                    Path best_path = {};
                    best_path.length = UINT32_MAX;

                    Entity *closest_target = nullptr;

                    EntityIter target_it = IterateAllEntities(EntityProperty_C);
                    if (!IsValid(target_it))
                    {
                        target_it = IterateAllEntities(EntityProperty_PlayerControlled);
                    }

                    for (; IsValid(target_it); Next(&target_it))
                    {
                        Entity *test_target = target_it.entity;

                        Path path = FindPath(&entity_manager->turn_arena, e->p, test_target->p);
                        if (path.length > 0 && path.length < best_path.length)
                        {
                            closest_target = test_target;
                            best_path = path;
                        }
                    }

                    if (closest_target)
                    {
                        V2i new_p = best_path.positions[0];
                        if (AreEqual(new_p, closest_target->p))
                        {
                            if (DamageEntity(closest_target, 999))
                            {
                                platform->LogPrint(PlatformLogLevel_Info, "Martins eviscerated a %.*s", StringExpand(closest_target->name));
                            }
                        }
                        else
                        {
                            MoveEntity(e, new_p);
                        }
                    }

                    e->energy -= 100;
                }
            }
        }
    }
    else
    {
        entity_manager->turn_timer -= platform->dt;
    }

    for (EntityIter it = IterateAllEntities(); IsValid(it); Next(&it))
    {
        Entity *e = it.entity;

        Sprite sprite = e->sprites[e->sprite_index];
        if (e->open)
        {
            sprite.foreground = MakeColor(sprite.foreground.r / 2,
                                          sprite.foreground.g / 2,
                                          sprite.foreground.b / 2);
        }

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

        if (HasProperty(e, EntityProperty_InWorld) && e->seen_by_player)
        {
            PushTile(Layer_World, e->p, sprite);
        }

        if (HasProperty(e, EntityProperty_PlayerControlled))
        {
            int y = 0; (void)y;
        }
        CleanStaleReferences(&e->inventory);
    }

    if (entity_manager->looking_at_container)
    {
        Entity *container = entity_manager->looking_at_container;

        if (container->inventory.first)
        {
            DrawEntityList(&container->inventory, MakeRect2iMinDim(2, 2, 24, 16), entity_manager->container_selection_index);
        }
    }
}
