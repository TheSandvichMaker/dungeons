#ifndef DUNGEONS_CONTROLLER_HPP
#define DUNGEONS_CONTROLLER_HPP

struct Button
{
    const char *name;
    uint32_t half_transition_count;
    bool ended_down;
};

static inline bool
Pressed(Button button)
{
    return (button.half_transition_count >= 2 ||
            (button.half_transition_count == 1 && button.ended_down));
}

static inline bool
Released(Button button)
{
    return (button.half_transition_count >= 2 ||
            (button.half_transition_count == 1 && !button.ended_down));
}

struct Input
{
};

struct Controller
{
    V2i mouse_p;
    V2i ui_mouse_p;
    V2i world_mouse_p;

    Button left         = { "left" };
    Button right        = { "right" };
    Button up           = { "up" };
    Button down         = { "down" };
    Button interact     = { "interact" };
    Button alt_interact = { "alternative interact" };
};
GLOBAL_STATE(Controller, controller);

static inline Button *
FirstButton(void)
{
    return &controller->left;
}

static inline Button *
LastButton(void)
{
    return (Button *)(controller + 1);
}

#endif /* DUNGEONS_CONTROLLER_HPP */
