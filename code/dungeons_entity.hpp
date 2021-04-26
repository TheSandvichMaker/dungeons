#ifndef DUNGEONS_ENTITY_HPP
#define DUNGEONS_ENTITY_HPP

#define MAX_ENTITY_COUNT (1 << 16)
#define WORLD_EXTENT_X 1024
#define WORLD_EXTENT_Y 1024

struct Path
{
    uint32_t length;
    V2i *positions;
};

enum EntityPropertyKind
{
    EntityProperty_None,
    EntityProperty_Alive,
    EntityProperty_Dying,
    EntityProperty_PlayerControlled,
    EntityProperty_Invulnerable,
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

static inline EntityHandle
NullEntityHandle(void)
{
    EntityHandle result = {};
    return result;
}

struct Entity
{
    EntityHandle handle;

    V2i p;
    int32_t health;

    float flash_timer;
    Color flash_color;

    float sprite_anim_rate;
    float sprite_anim_timer;
    float sprite_anim_pause_time;
    int16_t sprite_count;
    int16_t sprite_index;
    Sprite sprites[4];

    uint64_t properties[(EntityProperty_COUNT + 63) / 64];
};

enum ActionKind
{
    Action_None,

    Action_FollowPath,

    Action_COUNT,
};

struct Action
{
    ActionKind kind;
    Path path;
};

struct EntityManager
{
    Arena turn_arena;
    float turn_timer;

    uint32_t entity_count;

    Entity *player;

    Entity entities[MAX_ENTITY_COUNT];
    EntityHandle entity_grid[WORLD_EXTENT_X][WORLD_EXTENT_X];
};
GLOBAL_STATE(EntityManager, entity_manager);

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
    bool result = true;
    for (size_t i = 0; i < EntityProperty_PAGECOUNT; ++i)
    {
        if ((e->properties[i] & set.properties[i]) != set.properties[i])
        {
            result = false;
            break;
        }
    }
    return result;
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

#endif /* DUNGEONS_ENTITY_HPP */
