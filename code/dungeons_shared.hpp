#ifndef DUNGEONS_SHARED_HPP
#define DUNGEONS_SHARED_HPP

#define ArrayCount(x) (sizeof(x) / sizeof((x)[0]))

static inline void
ZeroSize(size_t size_init, void *data_init)
{
    size_t size = size_init;
    char *data = (char *)data_init;
    while (size--)
    {
        *data++ = 0;
    }
}

#define ZeroStruct(Struct) ZeroSize(sizeof(*(Struct)), Struct)
#define ZeroArray(Count, Data) ZeroSize(sizeof(*(Data))*(Count), Data)

#endif /* DUNGEONS_SHARED_HPP */
