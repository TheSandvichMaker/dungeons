#ifndef DUNGEONS_HPP
#define DUNGEONS_HPP

#include "dungeons_platform.hpp"
#include "dungeons_intrinsics.hpp"
#include "dungeons_shared.hpp"
#include "dungeons_memory.hpp"
#include "dungeons_math.hpp"
#include "dungeons_image.hpp"
#include "dungeons_render.hpp"

struct GameState
{
    Arena permanent_arena;
    Arena transient_arena;

    RenderState render_state;

    Font world_font;
    Font ui_font;
};

#endif /* DUNGEONS_HPP */
