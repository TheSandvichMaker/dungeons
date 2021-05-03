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
#include "dungeons_worldgen.hpp"

struct GameState
{
    GlobalState global_state;

    Arena permanent_arena;
    Arena transient_arena;

    bool world_generated;
    GenTiles *gen_tiles;

    Font tileset;

    Font world_font;
    Font ui_font;

    int debug_delay;
    int debug_delay_frame_count;
};
static GameState *game_state;

static inline void SetDebugDelay(int milliseconds, int frame_count);

#endif /* DUNGEONS_HPP */
