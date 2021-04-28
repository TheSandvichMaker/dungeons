#ifndef DUNGEONS_HPP
#define DUNGEONS_HPP

#include <stdarg.h>
#include <stdio.h>

#include "dungeons_platform.hpp"
#include "dungeons_intrinsics.hpp"
#include "dungeons_shared.hpp"
#include "dungeons_memory.hpp"
#include "dungeons_global_state.hpp"
#include "dungeons_math.hpp"
#include "dungeons_random.hpp"
#include "dungeons_image.hpp"
#include "dungeons_render.hpp"
#include "dungeons_controller.hpp"
#include "dungeons_entity.hpp"

struct GameState
{
    GlobalState global_state;

    Arena permanent_arena;
    Arena transient_arena;

    Font world_font;
    Font ui_font;
};

#endif /* DUNGEONS_HPP */
