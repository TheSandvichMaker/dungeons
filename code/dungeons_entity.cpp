static inline void
SetProperty(Entity *e, EntityPropertyKind property)
{
    e->properties[property / 64] |= 1ull << (property % 64);
}

static inline void
UnsetProperty(Entity *e, EntityPropertyKind property)
{
    e->properties[property / 64] &= ~(1ull << (property % 64));
}

static inline bool
HasProperty(Entity *e, EntityPropertyKind property)
{
    bool result = !!(e->properties[property / 64] & (1ull << (property % 64)));
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
        result->sprite = sprite;
    }

    return result;
}

static inline bool
NextEntity(Entity **entity_at)
{
    Entity *e = *entity_at;
    if (!e)
    {
        *entity_at = entity_manager->entities;
        return true;
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

static inline void
UpdateAndRenderEntities(void)
{
    for (Entity *e = nullptr; NextEntity(&e);)
    {
        DrawTile(Draw_World, e->p, e->sprite);
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
