#include "dungeons.hpp"

static inline void
DrawTestPattern(PlatformOffscreenBuffer *buffer)
{
    static int frame_counter = 0;
    static float accumulator = 0.0f;
    for (int y = 0; y < buffer->h; ++y)
    for (int x = 0; x < buffer->w; ++x)
    {
        Color *pixel = &buffer->data[y*buffer->w + x];
        pixel->r = (x + frame_counter) & 255;
        pixel->g = (y + frame_counter) & 255;
        pixel->b = 0;
        pixel->a = 255;
    }
    accumulator += platform->dt;
    if (accumulator >= 1.0f / 60.0f)
    {
        ++frame_counter;
        accumulator -= 1.0 / 60.0f;
    }
}

extern "C" void
App_UpdateAndRender(Platform *platform_)
{
    platform = platform_;
    DrawTestPattern(&platform->backbuffer);
}
