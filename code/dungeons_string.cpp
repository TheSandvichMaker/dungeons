static inline size_t
CStringLength(const char *str)
{
    size_t result = 0;
    while (*str++)
    {
        result += 1;
    }
    return result;
}

static inline bool
AreEqual(const char *a, const char *b)
{
    bool result = (*a == *b);

    while (*a && *b)
    {
        char c1 = *a++;
        char c2 = *b++;

        if (c1 != c2)
        {
            result = false;
            break;
        }
    }

    return result;
}

static inline bool
AreEqual(const String &a, const String &b)
{
    bool result = false;

    if (a.size == b.size)
    {
        result = true;
        for (size_t i = 0; i < a.size; ++i)
        {
            char c1 = a.data[i];
            char c2 = b.data[i];

            if (c1 != c2)
            {
                result = false;
                break;
            }
        }
    }

    return result;
}

static inline String
PushStringFV(Arena *arena, const char *fmt, va_list args_init)
{
    va_list args_size;
    va_copy(args_size, args_init);

    va_list args_fmt;
    va_copy(args_fmt, args_init);

    int chars_required = vsnprintf(nullptr, 0, fmt, args_size) + 1;
    va_end(args_size);

    String result = {};
    result.size = chars_required - 1;
    result.data = PushArrayNoClear(arena, chars_required, uint8_t);
    vsnprintf((char *)result.data, chars_required, fmt, args_fmt);
    va_end(args_fmt);

    return result;
}

static inline String
PushStringF(Arena *arena, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    String result = PushStringFV(arena, fmt, args);
    va_end(args);
    return result;
}

static inline String
PushTempStringF(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    String result = PushStringFV(platform->GetTempArena(), fmt, args);
    va_end(args);
    return result;
}

static inline bool
IsEmpty(StringList *list)
{
    bool result = !list->first;
    return result;
}

static inline StringNode
MakeStringNode(String string)
{
    StringNode result = {};
    result.string = string;
    return result;
}

static inline void
AddNode(StringList *list, StringNode *node)
{
    SllQueuePush(list->first, list->last, node);
    node->foreground = list->foreground;
    node->background = list->background;

    list->node_count += 1;
    list->total_size += node->string.size;
}

static inline void
PushString(StringList *list, Arena *arena, String string)
{
    StringNode *node = PushStruct(arena, StringNode);
    node->string = PushString(arena, string);
    AddNode(list, node);
}

static inline void
PushStringFV(StringList *list, Arena *arena, const char *fmt, va_list args)
{
    StringNode *node = PushStruct(arena, StringNode);
    node->string = PushStringFV(arena, fmt, args);
    AddNode(list, node);
}

static inline void
PushStringF(StringList *list, Arena *arena, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    PushStringFV(list, arena, fmt, args);
    va_end(args);
}

static inline void
PushTempStringF(StringList *list, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    PushStringFV(list, platform->GetTempArena(), fmt, args);
    va_end(args);
}

static inline void
SetForeground(StringList *list, Color color)
{
    list->foreground = color;
}

static inline void
SetBackground(StringList *list, Color color)
{
    list->background = color;
}

static inline void
InsertSeparators(StringList *list, Arena *arena, String separator, StringSeparatorFlags flags = 0)
{
    // I hate inserting in the middle of linked lists with tail pointers,
    // maybe that's just because I've never thought about how to really do
    // it properly, but whatever. We'll just make a new list with the same
    // nodes. Inefficient? Yeah!!!! Whatever!!!!!

    bool separator_before_first = !!(flags & StringSeparator_BeforeFirst);
    bool separator_after_last = !!(flags & StringSeparator_AfterLast);

    StringList new_list = {};

    if (separator_before_first)
    {
        PushString(&new_list, arena, separator);
    }

    for (StringNode *node = list->first;
         node;
         )
    {
        StringNode *next_node = node->next;

        new_list.foreground = node->foreground;
        new_list.background = node->background;

        AddNode(&new_list, node);
        if (next_node)
        {
            PushString(&new_list, arena, separator);
        }

        node = next_node;
    }

    if (separator_after_last)
    {
        PushString(&new_list, arena, separator);
    }

    *list = new_list;
}

static inline String
PushFlattenedString(StringList *list, Arena *arena, String separator = {}, StringSeparatorFlags flags = 0)
{
    size_t total_size = list->total_size;

    bool separator_before_first = !!(flags & StringSeparator_BeforeFirst);
    bool separator_after_last = !!(flags & StringSeparator_AfterLast);

    size_t separator_count = 0;
    if (list->node_count > 0)
    {
        separator_count = list->node_count - 1;
        if (separator_before_first) separator_count += 1;
        if (separator_after_last) separator_count += 1;
    }

    if (separator_count > 0)
    {
        total_size += separator_count*separator.size;
    }

    String result = {};
    result.size = total_size;
    result.data = PushArray(arena, result.size + 1, uint8_t);

    auto Append = [](uint8_t **dest, String string)
    {
        CopySize(string.size, string.data, *dest);
        *dest += string.size;
    };

    uint8_t *dest = result.data;
    if (separator_before_first)
    {
        Append(&dest, separator);
    }
    
    for (StringNode *node = list->first;
         node;
         node = node->next)
    {
        Append(&dest, node->string);
        if (node->next)
        {
            Append(&dest, separator);
        }
    }

    if (separator_after_last)
    {
        Append(&dest, separator);
    }

    result.data[result.size] = 0;

    Assert(dest == (result.data + result.size));
    return result;
}

static inline String
PushFlattenedTempString(StringList *list, String separator = {}, StringSeparatorFlags flags = 0)
{
    return PushFlattenedString(list, platform->GetTempArena(), separator, flags);
}
