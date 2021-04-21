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
        result->health = 2;
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

static inline void
UpdateAndRenderEntities(void)
{
    if (entity_manager->turn_timer <= 0.0f)
    {
        entity_manager->turn_timer += 1.0f;
        for (Entity *e = nullptr; NextEntity(&e);)
        {
            if (HasProperty(e, EntityProperty_AngryDude))
            {
                uint32_t best_dist = UINT32_MAX;
                Entity *target = nullptr;
                for (Entity *other = nullptr; NextEntity(&other);)
                {
                    if (other != e)
                    {
                        V2i delta = other->p - e->p;
                        uint32_t dist = LengthSq(delta);
                        if (best_dist > dist)
                        {
                            best_dist = dist;
                            target = other;
                        }
                    }
                }

                if (target)
                {
                    V2i delta = target->p - e->p;
                    delta = Clamp(delta, MakeV2i(-1, -1), MakeV2i(1, 1));
                    V2i new_p = e->p + delta;
                    if (new_p == target->p)
                    {
                        target->health -= 1;
                        target->flash_timer = 0.2f;
                        target->flash_color = MakeColor(255, 0, 0);
                        if (target->health <= 0)
                        {
                            SetProperty(target, EntityProperty_Dying);
                        }
                    }
                    else
                    {
                        e->p = new_p;
                    }
                }
            }
        }
    }
    entity_manager->turn_timer -= platform->dt;

    for (Entity *e = nullptr; NextEntity(&e);)
    {
        Sprite sprite = e->sprite;
        if (e->flash_timer > 0.0f)
        {
            sprite.foreground = e->flash_color;
            e->flash_timer -= platform->dt;
        }
        else
        {
            if (HasProperty(e, EntityProperty_Dying))
            {
                UnsetProperty(e, EntityProperty_Dying);
                UnsetProperty(e, EntityProperty_Alive);
            }
        }
        DrawTile(Draw_World, e->p, sprite);
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
