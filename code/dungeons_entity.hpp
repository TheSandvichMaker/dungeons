#ifndef DUNGEONS_ENTITY_HPP
#define DUNGEONS_ENTITY_HPP

#define MAX_ENTITY_COUNT (1 << 16)

enum EntityPropertyKind
{
    EntityProperty_Alive,
    EntityProperty_COUNT,
};

struct EntityHandle
{
    uint32_t index;
    uint32_t generation;
};

struct Entity
{
    EntityHandle handle;

    V2i p;
    Sprite sprite;
    uint64_t properties[(EntityProperty_COUNT + 63) / 64];
};

struct EntityManager
{
    uint32_t entity_count;
    Entity entities[MAX_ENTITY_COUNT];
};
GLOBAL_STATE(EntityManager, entity_manager);

#endif /* DUNGEONS_ENTITY_HPP */
