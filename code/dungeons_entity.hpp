#ifndef DUNGEONS_ENTITY_HPP
#define DUNGEONS_ENTITY_HPP

#define MAX_ENTITY_COUNT (1 << 16)
#define ENTITY_HASH_SIZE 8192

#define WORLD_SIZE_X 1024
#define WORLD_SIZE_Y 1024

struct Path
{
    uint32_t length;
    V2i *positions;
};

enum EntityPropertyKind
{
    EntityProperty_None,
    EntityProperty_Alive,
    EntityProperty_InWorld,
    EntityProperty_Dying,
    EntityProperty_Blocking,
    EntityProperty_PlayerControlled,
    EntityProperty_Invulnerable,
    EntityProperty_Unlockable,
    EntityProperty_Door,
    EntityProperty_Martins,
    EntityProperty_C,
    EntityProperty_AngryDude,
    EntityProperty_COUNT,
    EntityProperty_PAGECOUNT = (EntityProperty_COUNT + 63) / 64,
};

struct EntityPropertySet
{
    uint64_t properties[(EntityProperty_COUNT + 63) / 64];
};

struct EntityHandle
{
    uint32_t index;
    uint32_t generation;
};

static inline bool
operator == (EntityHandle a, EntityHandle b)
{
    return (a.index == b.index && a.generation == b.generation);
}

static inline bool
operator != (EntityHandle a, EntityHandle b)
{
    return !(a == b);
}

static inline EntityHandle
NullEntityHandle(void)
{
    EntityHandle result = {};
    return result;
}

struct Entity;

struct EntityNode
{
    EntityNode *next;
    EntityHandle handle;
};

struct EntityList
{
    EntityNode *first;
    EntityNode *last;
};

typedef Array<Entity *> EntityArray;

enum TriggerKind
{
    Trigger_None,
    Trigger_Unblock,
    Trigger_Container,
    Trigger_PickUp,
    Trigger_COUNT,
};

struct Entity
{
    EntityHandle handle;

    union
    {
        Entity *next_on_tile;
        Entity *next_free;
    };

    EntityList inventory;

    String name;

    TriggerKind contact_trigger;

    EntityHandle required_key;
    bool open;
    bool seen_by_player;

    int32_t amount;

    V2i p;
    int32_t health;

    float flash_timer;
    Color flash_color;

    int32_t speed;
    int32_t energy;

    float sprite_anim_rate;
    float sprite_anim_timer;
    float sprite_anim_pause_time;
    int16_t sprite_count;
    int16_t sprite_index;
    Sprite sprites[4];

    uint64_t properties[(EntityProperty_COUNT + 63) / 64];
};

struct VisibilityGrid
{
    Rect2i bounds;
    bool *tiles;
};

struct EntityManager
{
    Arena arena;

    Arena turn_arena;
    float turn_timer;

    uint32_t entity_count;

    Entity *player;
    Entity *looking_at_container;
    VisibilityGrid player_visibility;

    int container_selection_index;

    EntityNode *first_free_entity_node;

    Entity *first_free_entity;
    Entity entities[MAX_ENTITY_COUNT];
    Entity *entity_grid[WORLD_SIZE_X][WORLD_SIZE_Y];
};
GLOBAL_STATE(EntityManager, entity_manager);

static inline EntityNode *NewEntityNode(Entity *entity);
static inline void FreeEntityNode(EntityNode *node);
static inline Entity *EntityFromHandle(EntityHandle handle);
static inline EntityHandle HandleFromEntity(Entity *entity);

static inline void
SetProperty(Entity *e, EntityPropertyKind property)
{
    if (e)
    {
        e->properties[property / 64] |= 1ull << (property % 64);
    }
}

static inline void
SetProperty(Entity *e, EntityPropertySet set)
{
    if (e)
    {
        for (size_t i = 0; i < EntityProperty_PAGECOUNT; ++i)
        {
            e->properties[i] |= set.properties[i];
        }
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

static inline bool
HasProperties(Entity *e, EntityPropertySet set)
{
    bool result = false;
    if (e)
    {
        result = true;
        for (size_t i = 0; i < EntityProperty_PAGECOUNT; ++i)
        {
            if ((e->properties[i] & set.properties[i]) != set.properties[i])
            {
                result = false;
                break;
            }
        }
    }
    return result;
}

static inline void
SetProperties(Entity *e, EntityPropertySet set)
{
    if (e)
    {
        for (size_t i = 0; i < EntityProperty_PAGECOUNT; ++i)
        {
            e->properties[i] |= set.properties[i];
        }
    }
}

static inline EntityPropertySet
SetProperty(EntityPropertySet set, EntityPropertyKind property)
{
    set.properties[property / 64] |= 1ull << (property % 64);
    return set;
}

static inline void
UnsetProperty(EntityPropertySet *set, EntityPropertyKind property)
{
    set->properties[property / 64] &= ~(1ull << (property % 64));
}

static inline bool
HasProperty(EntityPropertySet *set, EntityPropertyKind property)
{
    bool result = !!(set->properties[property / 64] & (1ull << (property % 64)));
    return result;
}

static inline EntityPropertySet
AnyProperty(void)
{
    EntityPropertySet result = {};
    for (size_t i = 0; i = EntityProperty_COUNT / 64; ++i)
    {
        result.properties[i] = (uint64_t)-1;
    }
    return result;
}

#define MakeEntityPropertySet(...) MakeEntityPropertySet_(__VA_ARGS__, EntityProperty_None)
static inline EntityPropertySet
MakeEntityPropertySet_(EntityPropertyKind first, ...)
{
    EntityPropertySet result = {};
    result = SetProperty(result, first);

    va_list args;
    va_start(args, first);
    for (;;)
    {
        EntityPropertyKind prop = va_arg(args, EntityPropertyKind);
        if (prop == EntityProperty_None)
        {
            break;
        }

        result = SetProperty(result, prop);
    }

    return result;
}

static inline EntityPropertySet
CombineSet(EntityPropertySet a, EntityPropertySet b)
{
    EntityPropertySet result = a;
    for (size_t i = 0; i < EntityProperty_PAGECOUNT; ++i)
    {
        result.properties[i] |= b.properties[i];
    }
    return result;
}

static inline EntityPropertySet
operator | (EntityPropertyKind a, EntityPropertyKind b)
{
    EntityPropertySet result = {};
    result = SetProperty(result, a);
    result = SetProperty(result, b);
    return result;
}

static inline EntityPropertySet
operator | (EntityPropertySet set, EntityPropertyKind prop)
{
    set = SetProperty(set, prop);
    return set;
}

static inline EntityPropertySet
operator | (EntityPropertySet a, EntityPropertySet b)
{
    EntityPropertySet result = CombineSet(a, b);
    return result;
}

static inline EntityPropertySet &
operator |= (EntityPropertySet &set, EntityPropertyKind prop)
{
    set = set|prop;
    return set;
}

static inline EntityPropertySet &
operator |= (EntityPropertySet &a, EntityPropertySet b)
{
    a = a|b;
    return a;
}

static inline EntityPropertySet
MakeSet(EntityPropertyKind prop)
{
    EntityPropertySet set = {};
    set |= prop;
    return set;
}

#endif /* DUNGEONS_ENTITY_HPP */
