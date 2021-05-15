static inline EntityNode *
NewEntityNode(EntityHandle handle)
{
    if (!entity_manager->first_free_entity_node)
    {
        entity_manager->first_free_entity_node = PushStructNoClear(&entity_manager->arena, EntityNode);
    }
    EntityNode *result = SllStackPop(entity_manager->first_free_entity_node);
    result->next = nullptr;
    result->handle = handle;
    return result;
}

static inline void
FreeEntityNode(EntityNode *node)
{
    SllStackPush(entity_manager->first_free_entity_node, node);
}

static inline EntityArray
Linearize(EntityList *list)
{
    int count = 0;
    for (EntityNode *node = list->first; node; node = node->next)
    {
        count += 1;
    }

    EntityArray result = PushArrayContainer<Entity *>(platform->GetTempArena(), count);

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

    EntityArray result = PushArrayContainer<Entity *>(platform->GetTempArena(), count);

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
        if (array[i])
        {
            EntityNode *node = NewEntityNode(array[i]->handle);
            SllQueuePush(list->first, list->last, node);
        }
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
PushToList(Arena *arena, EntityList *list, EntityHandle handle)
{
    EntityNode *node = PushStruct(arena, EntityNode);
    node->handle = handle;
    SllQueuePush(list->first, list->last, node);
}

static inline void
AddToList(EntityList *list, EntityHandle handle)
{
    EntityNode *node = NewEntityNode(handle);
    SllQueuePush(list->first, list->last, node);
}

static inline void
AddToListUnique(EntityList *list, EntityHandle handle)
{
    bool is_unique = true;
    for (EntityNode *node = list->first;
         node;
         node = node->next)
    {
        if (node->handle == handle)
        {
            is_unique = false;
            break;
        }
    }
    if (is_unique)
    {
        AddToList(list, handle);
    }
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
        if (HasProperty(it, EntityProperty_BlockMovement))
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
        result->view_radius = 24.0f;

        MoveEntity(result, p);
    }

    return result;
}

static inline Entity *
AddWall(V2i p)
{
    Entity *e = AddEntity(StringLiteral("Wall"), p, MakeSprite(Glyph_Tone50, MakeColor(110, 165, 100)));
    SetProperties(e, EntityProperty_Invulnerable|EntityProperty_BlockMovement|EntityProperty_BlockSight);
    return e;
}

static inline Entity *
AddDoor(V2i p)
{
    Entity *e = AddEntity(StringLiteral("Door"), p, MakeSprite('#', MakeColor(255, 127, 0)));
    SetProperties(e, EntityProperty_Invulnerable|EntityProperty_Door|EntityProperty_Unlockable|EntityProperty_BlockMovement|EntityProperty_BlockSight);
    SetContactTrigger(e, Trigger_Unblock);
    return e;
}

static inline Entity *
AddPlayer(V2i p)
{
    Entity *e = AddEntity(StringLiteral("Player"), p, MakeSprite('@', MakeColor(255, 255, 0)));
    SetProperties(e, EntityProperty_PlayerControlled|EntityProperty_BlockMovement|EntityProperty_BlockSight|EntityProperty_HasVisibilityGrid);

    e->health = 100;
    e->faction = Faction_Human;

    Assert(!entity_manager->player);
    entity_manager->player = e;

    return e;
}

static inline Entity *
AddOrc(V2i p)
{
    Entity *e = AddEntity(StringLiteral("Orc"), p, MakeSprite('O', MakeColor(126, 192, 95)));
    e->health = 3;
    e->speed = 125;
    e->ai = Ai_StandardHumanoid;
    e->faction = Faction_Monster;
    SetProperty(e, EntityProperty_BlockMovement);
    SetProperty(e, EntityProperty_HasVisibilityGrid);
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
    SetProperties(e, EntityProperty_Unlockable|EntityProperty_Invulnerable|EntityProperty_BlockMovement);
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

static inline EntityArray
GetEntitiesAt(V2i p, EntityPropertySet filter = {})
{
    EntityArray result = {};
    if (IsInWorld(p))
    {
        Entity *list = entity_manager->entity_grid[p.x][p.y];

        size_t count = 0;
        for (Entity *e = list; e; e = e->next_on_tile)
        {
            if (HasProperties(e, filter))
            {
                count += 1;
            }
        }

        result = PushArrayContainer<Entity *>(platform->GetTempArena(), count);

        for (Entity *e = list; e; e = e->next_on_tile)
        {
            if (HasProperties(e, filter))
            {
                Push(&result, e);
            }
        }
    }
    return result;
}

static inline bool
TileBlocked(V2i p)
{
    EntityArray arr = GetEntitiesAt(p, MakeSet(EntityProperty_BlockMovement));
    return arr.count > 0;
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

static inline DamageDescriptor
BasicDamage(EntityHandle aggressor, int32_t amount)
{
    DamageDescriptor result = {};
    result.aggressor = aggressor;
    result.amount = amount;
    return result;
}

static inline bool
TakeDamage(Entity *e, const DamageDescriptor &desc)
{
    if (!HasProperty(e, EntityProperty_Dying))
    {
        e->health -= desc.amount;
        e->flash_timer = 0.2f;
        e->flash_color = MakeColor(255, 0, 0);
        if (e->health <= 0)
        {
            SetProperty(e, EntityProperty_Dying);
        }
        if (e->ai)
        {
            AddToListUnique(&e->forced_hostile_entities, desc.aggressor);
        }
        return true;
    }
    return false;
}

typedef uint32_t RaycastFlags;
enum
{
    Raycast_TestMovement = 0x1,
    Raycast_TestSight = 0x2,
};

static inline EntityArray
Raycast(V2i start, V2i end, RaycastFlags flags = Raycast_TestMovement)
{
    EntityArray result = {};

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
            EntityPropertySet filter = {};
            if (flags & Raycast_TestMovement) filter |= EntityProperty_BlockMovement;
            if (flags & Raycast_TestSight   ) filter |= EntityProperty_BlockSight;
            EntityArray on_tile = GetEntitiesAt(p, filter);
            if (on_tile.count)
            {
                result = on_tile;
                break;
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
    PathNode *next_in_hash;
    PathNode *next, *prev;
    float cost;
    V2i p;
};

struct PathfindingState
{
    PathNode *node_hash[1024];
};

static inline Path
FindPath(Arena *arena, V2i start, V2i target)
{
    Arena *temp_arena = platform->GetTempArena();
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
            PathNode *on_tile = state->node_hash[slot];
            for (; on_tile && !AreEqual(on_tile->p, p); on_tile = on_tile->next_in_hash);
            if (on_tile)
            {
                continue;
            }

            if (!TileBlocked(p) || AreEqual(p, target))
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

                node->next_in_hash = state->node_hash[slot];
                state->node_hash[slot] = node;

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

static inline void
AddToInventory(Entity *e, Entity *item)
{
    RemoveEntityFromGrid(item);
    UnsetProperty(item, EntityProperty_InWorld);

    EntityNode *node = NewEntityNode(item->handle);
    SllQueuePush(e->inventory.first, e->inventory.last, node);
}

static inline void
LockWithKey(Entity *e, Entity *key)
{
    Assert(e->required_key == NullEntityHandle());
    e->required_key = key->handle;
    e->open = false;
    SetProperty(e, EntityProperty_Unlockable);

    key->uses += 1;
}

static inline bool
TryOpen(Entity *e, Entity *other)
{
    if (e->open)
    {
        return true;
    }

    if (e->required_key == NullEntityHandle())
    {
        e->open = true;
    }
    else
    {
        ForSll (item, other->inventory.first)
        {
            if (item->handle == e->required_key)
            {
                Entity *key = EntityFromHandle(item->handle);
                if (key->uses > 0)
                {
                    key->uses -= 1;
                    if (key->uses <= 0)
                    {
                        KillEntity(key);
                    }
                }
                e->open = true;
            }
        }
    }

    return e->open;
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

    DrawRectOutline(Layer_Ui, rect, COLOR_WHITE, COLOR_BLACK);

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

        DrawTile(Layer_Ui, at_p - MakeV2i(1, 0), item->sprites[item->sprite_index]);

        String text = PushTStringF(" %4d %.*s ", item->amount, StringExpand(item->name));
        DrawText(Layer_Ui, at_p, text, text_color, COLOR_BLACK);

        at_p.y -= 1;
        item_index += 1;
    }

    if (!items.count)
    {
        Color text_color = MakeColor(127, 127, 127);
        String text = PushTStringF("  Nothing here...");
        DrawText(Layer_Ui, at_p, text, text_color, COLOR_BLACK);
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
            UnsetProperty(e, EntityProperty_BlockMovement);
            UnsetProperty(e, EntityProperty_BlockSight);
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
        CleanStaleReferences(&player->inventory);
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
        for (Entity *e: GetEntitiesAt(move_p))
        {
            if (e->contact_trigger)
            {
                ProcessTrigger(e, player);
            }

            if (HasProperty(e, EntityProperty_BlockMovement))
            {
                if (HasProperty(e, EntityProperty_Invulnerable))
                {
                    // blocked with no recourse
                    blocked = true;
                }
                else
                {
                    TakeDamage(e, BasicDamage(player->handle, 1));
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

static inline VisibilityGrid *
PushVisibilityGrid(Arena *arena, Rect2i bounds)
{
    VisibilityGrid *result = PushStruct(arena, VisibilityGrid);
    result->bounds = bounds;

    int width = GetWidth(bounds);
    int height = GetHeight(bounds);
    result->tiles = PushArray(arena, width*height, bool);

    return result;
}

static inline void
MarkAsSeen(Entity *e)
{
    e->seen_by_player = true;
    e->seen_p = e->p;
}

static inline void
CalculateVisibilityMassRay(VisibilityGrid *grid, Entity *e, float radius)
{
    int w = GetWidth(grid->bounds);
    int h = GetHeight(grid->bounds);
    for (int y = 0; y <= h; y += 1)
    for (int x = 0; x <= w; x += 1)
    {
        V2i p = MakeV2i(x + grid->bounds.min.x, y + grid->bounds.min.y);
        if (AreEqual(p, e->p))
        {
            grid->tiles[y*w + x] = true;
        }
        else if (Length(e->p - p) <= radius)
        {
            EntityArray hit_entities = Raycast(e->p, p, Raycast_TestSight);
            if (hit_entities.count)
            {
                for (Entity *seen: hit_entities)
                {
                    if (IsInRect(grid->bounds, seen->p))
                    {
                        grid->tiles[y*w + x] = true;
                        SetSeenByPlayer(game_state->gen_tiles, seen->p, true);
                    }
                    MarkAsSeen(seen);
                }
            }
            else
            {
                grid->tiles[y*w + x] = true;
                SetSeenByPlayer(game_state->gen_tiles, p, true);
                for (Entity *seen: GetEntitiesAt(p))
                {
                    MarkAsSeen(seen);
                }
            }
        }
    }
}

static inline V2i
TransformForQuadrant(int quadrant, V2i p)
{
    switch (quadrant)
    {
        case 0: return MakeV2i( p.x,  p.y);
        case 1: return MakeV2i( p.x, -p.y);
        case 2: return MakeV2i( p.y,  p.x);
        case 3: return MakeV2i(-p.y,  p.x);
    }
    return p;
}

static inline int
RoundDown(float f)
{
    return (int)ceilf(f - 0.5f);
}

static inline int
RoundUp(float f)
{
    return (int)floorf(f + 0.5f);
}

static inline void
SetVisible(VisibilityGrid *grid, V2i p)
{
    if (IsInRect(grid->bounds, p))
    {
        int w = GetWidth(grid->bounds);
        int rel_x = p.x - grid->bounds.min.x;
        int rel_y = p.y - grid->bounds.min.y;
        grid->tiles[rel_y*w + rel_x] = true;
    }
}

static inline bool
IsVisible(VisibilityGrid *grid, V2i p)
{
    bool result = false;
    if (grid && IsInRect(grid->bounds, p))
    {
        int w = GetWidth(grid->bounds);
        int rel_x = p.x - grid->bounds.min.x;
        int rel_y = p.y - grid->bounds.min.y;
        result = grid->tiles[rel_y*w + rel_x];
    }
    return result;
}

static inline float
Slope(V2i p)
{
    return (float)(2*p.x - 1) / (float)(2*p.y);
}

static inline void
CalculateVisibilityRecursiveShadowcastInternal(VisibilityGrid *grid, bool is_player, int quadrant, V2i origin, int row, float start_slope, float end_slope)
{
    int row_limit = grid->bounds.max.x - origin.x; // alert! alert! assuming square bounds alert!!!
    if (row >= row_limit)
    {
        return;
    }

    bool prev_tile_set = false;
    bool prev_tile_was_wall = false;
    int min_col = RoundUp((float)row*start_slope);
    int max_col = RoundDown((float)row*end_slope);
    for (int col = min_col; col <= max_col; col += 1)
    {
        bool is_wall = false;
        bool is_symmetric = (((float)col >= (float)row*start_slope) &&
                             ((float)col <= (float)row*end_slope));
        V2i p = origin + TransformForQuadrant(quadrant, MakeV2i(col, row));
        V2i p_rel = MakeV2i(col, row);
        for (Entity *e: GetEntitiesAt(p))
        {
            MarkAsSeen(e);
            if (HasProperty(e, EntityProperty_BlockSight))
            {
                SetVisible(grid, p);
                if (is_player) SetSeenByPlayer(game_state->gen_tiles, p, true);

                is_wall = true;
            }
        }
        if (is_symmetric)
        {
            SetVisible(grid, p);
            if (is_player) SetSeenByPlayer(game_state->gen_tiles, p, true);
        }
        if (prev_tile_set && prev_tile_was_wall && !is_wall)
        {
            start_slope = Slope(p_rel);
        }
        if (prev_tile_set && !prev_tile_was_wall && is_wall)
        {
            int next_row = row + 1;
            float next_end_slope = Slope(p_rel);
            CalculateVisibilityRecursiveShadowcastInternal(grid, is_player, quadrant, origin, next_row, start_slope, next_end_slope);
        }
        prev_tile_was_wall = is_wall;
        prev_tile_set = true;
    }
    if (!prev_tile_was_wall)
    {
        CalculateVisibilityRecursiveShadowcastInternal(grid, is_player, quadrant, origin, row + 1, start_slope, end_slope);
    }
}

static inline void
CalculateVisibilityRecursiveShadowcast(VisibilityGrid *grid, Entity *e)
{
    bool is_player = (entity_manager->player && (e == entity_manager->player));
    for (int i = 0; i < 4; i += 1)
    {
        CalculateVisibilityRecursiveShadowcastInternal(grid, is_player, i, e->p, 1, -1, 1);
    }

    SetVisible(grid, e->p);
    MarkAsSeen(e);
    if (is_player) SetSeenByPlayer(game_state->gen_tiles, e->p, true);
}

static inline VisibilityGrid *
PushAndCalculateVisibility(Arena *arena, Entity *e)
{
    int radius = RoundUp(e->view_radius);
    Rect2i bounds = MakeRect2iCenterHalfDim(e->p, MakeV2i(radius));

    VisibilityGrid *result = PushVisibilityGrid(arena, bounds);
    CalculateVisibilityRecursiveShadowcast(result, e);

    return result;
}

static inline bool
IsVisibleTo(Entity *e, V2i p)
{
    if (!HasProperty(e, EntityProperty_HasVisibilityGrid))
    {
        return false;
    }

    if (Length(p - e->p) > e->view_radius)
    {
        return false;
    }

    return IsVisible(e->visibility_grid, p);
}

static inline bool
EntityAct(Entity *e)
{
    if (e->ai != Ai_None)
    {
        e->energy += e->speed;
        while (e->energy > 100)
        {
            e->energy -= 100;

            int32_t best_dist_sq = INT32_MAX;
            Entity *target = nullptr;
            for (EntityIter iter_other = IterateAllEntities(); IsValid(iter_other); Next(&iter_other))
            {
                Entity *other = iter_other.entity;

                if (FactionIsHostileTo(e->faction, other->faction))
                {
                    V2i delta = e->p - other->p;
                    int32_t dist_sq = LengthSq(delta);
                    if (dist_sq < best_dist_sq)
                    {
                        best_dist_sq = dist_sq;
                        target = other;
                    }
                }
            }

            for (Entity *other: Linearize(&e->forced_hostile_entities))
            {
                V2i delta = e->p - other->p;
                int32_t dist_sq = LengthSq(delta);
                if (dist_sq < best_dist_sq)
                {
                    best_dist_sq = dist_sq;
                    target = other;
                }
            }

            if (target)
            {
                V2i delta = e->p - target->p;
                int32_t dist_sq = LengthSq(delta);
                if (dist_sq < 12*12)
                {
                    for (Entity *seen: Raycast(e->p, target->p, Raycast_TestSight))
                    {
                        if (seen == target)
                        {
                            if ((Abs(delta.x) <= 1) &&
                                (Abs(delta.y) <= 1))
                            {
                                TakeDamage(target, BasicDamage(e->handle, 1));
                                return true;
                            }
                            else
                            {
                                ScopedMemory crap(&entity_manager->arena);
                                Path path = FindPath(&entity_manager->arena, e->p, target->p);
                                if (path.length > 0)
                                {
                                    MoveEntity(e, path.positions[0]);
                                    return true;
                                }
                            }

                            break;
                        }
                    }
                }
            }
        }
    }

    return false;
}

static inline void
NextTurn(void)
{
    entity_manager->turn_index += 1;
    Clear(&entity_manager->turn_arena);
}

static inline void
WarmUpEntityVisibilityGrids(void)
{
    for (EntityIter iter = IterateAllEntities(EntityProperty_HasVisibilityGrid);
         IsValid(iter);
         Next(&iter))
    {
        Entity *e = iter.entity;
        e->visibility_grid = PushAndCalculateVisibility(&entity_manager->turn_arena, e);
    }
}

static inline void
UpdateAndRenderEntities(void)
{
    if (!entity_manager->block_simulation && entity_manager->turn_timer <= 0.0f)
    {
        if (PlayerAct())
        {
            NextTurn();

            Entity *player = entity_manager->player;
            player->visibility_grid = PushAndCalculateVisibility(&entity_manager->turn_arena, player);

            for (EntityIter iter = IterateAllEntities(); IsValid(iter); Next(&iter))
            {
                Entity *e = iter.entity;

                if (e == player)
                {
                    continue;
                }

                // see if the player lost track of us
                if (!AreEqual(e->p, e->seen_p) || HasProperty(e, EntityProperty_Volatile))
                {
                    e->seen_by_player = false;
                }

                bool did_something = EntityAct(e);
                if (did_something)
                {
                    NextTurn();
                    e->visibility_grid = PushAndCalculateVisibility(&entity_manager->turn_arena, e);
                }
            }
        }
    }
    else
    {
        entity_manager->turn_timer -= platform->dt;
    }

    entity_manager->block_simulation = false;

    Font *world_font = render_state->world_font;

    Entity *player = entity_manager->player;

    V2i render_tile_dim = MakeV2i(platform->render_w / world_font->glyph_w, platform->render_h / world_font->glyph_h);
    render_state->camera_bottom_left = player->p - render_tile_dim / 2;

    int viewport_w = (platform->render_w + world_font->glyph_w - 1) / world_font->glyph_w;
    int viewport_h = (platform->render_h + world_font->glyph_h - 1) / world_font->glyph_h;

    Rect2i viewport = MakeRect2iMinDim(render_state->camera_bottom_left, MakeV2i(viewport_w, viewport_h));
    render_state->viewport = viewport;

    bool done_animations = true;
    for (int y = viewport.min.y; y <= viewport.max.y; y += 1)
    for (int x = viewport.min.x; x <= viewport.max.x; x += 1)
    {
        V2i p = MakeV2i(x, y);
        for (Entity *e: GetEntitiesAt(p))
        {
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
                done_animations = false;
            }

            if (e->flash_timer <= 0.0f && HasProperty(e, EntityProperty_Dying))
            {
                KillEntity(e);
            }

            if (HasProperty(e, EntityProperty_PlayerControlled))
            {
                e->seen_by_player = true;
            }

            bool draw = (HasProperty(e, EntityProperty_InWorld) && e->seen_by_player);
            draw |= game_state->debug_fullbright;
            if (draw)
            {
                bool visible = IsVisible(player->visibility_grid, e->p);
                visible |= game_state->debug_fullbright;
                if (!visible)
                {
                    sprite.foreground.r = sprite.foreground.r / 2;
                    sprite.foreground.g = sprite.foreground.g / 2;
                    sprite.foreground.b = sprite.foreground.b / 2;
                    sprite.background.r = sprite.background.r / 2;
                    sprite.background.g = sprite.background.g / 2;
                    sprite.background.b = sprite.background.b / 2;
                }
                RenderLayer layer = Layer_World;
                if (!HasProperty(e, EntityProperty_BlockMovement))
                {
                    layer = Layer_Floor;
                }
                DrawTile(layer, e->p, sprite);
            }

            CleanStaleReferences(&e->inventory);
            CleanStaleReferences(&e->forced_hostile_entities);
        }
    }

    if (!done_animations)
    {
        entity_manager->block_simulation = true;
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
