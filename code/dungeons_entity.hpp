#ifndef DUNGEONS_ENTITY_HPP
#define DUNGEONS_ENTITY_HPP

#define MAX_ENTITY_COUNT (1 << 16)
#define WORLD_EXTENT_X 1024
#define WORLD_EXTENT_Y 1024

enum EntityPropertyKind
{
    EntityProperty_Alive,
    EntityProperty_Dying,
    EntityProperty_AngryDude,
    EntityProperty_COUNT,
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

    Sprite sprite;
    uint64_t properties[(EntityProperty_COUNT + 63) / 64];
};

struct EntityManager
{
    float turn_timer;
    uint32_t entity_count;
    Entity entities[MAX_ENTITY_COUNT];
    EntityHandle entity_grid[WORLD_EXTENT_X][WORLD_EXTENT_X];
};
GLOBAL_STATE(EntityManager, entity_manager);

#endif /* DUNGEONS_ENTITY_HPP */
