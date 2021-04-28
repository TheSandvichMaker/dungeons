#ifndef DUNGEONS_CONTROLLER_HPP
#define DUNGEONS_CONTROLLER_HPP

struct Button
{
    const char *name;
    uint32_t half_transition_count;
    uint32_t repeat_count;
    bool ended_down;
};

static inline bool
Pressed(Button button)
{
    return (button.half_transition_count >= 2 ||
            (button.half_transition_count == 1 && button.ended_down));
}

static inline bool
Repeated(Button button)
{
    return (button.repeat_count > 0);
}

static inline bool
Triggered(Button button)
{
    return Pressed(button) || Repeated(button);
}

static inline bool
Released(Button button)
{
    return (button.half_transition_count >= 2 ||
            (button.half_transition_count == 1 && !button.ended_down));
}

struct Input
{
    V2i mouse_p;
    V2i ui_mouse_p;
    V2i world_mouse_p;

    Button north        = { "north" };
    Button northeast    = { "northeast" };
    Button east         = { "east" };
    Button southeast    = { "southeast" };
    Button south        = { "south" };
    Button southwest    = { "southwest" };
    Button west         = { "west" };
    Button northwest    = { "northwest" };
    Button here         = { "here" };

    Button interact     = { "interact" };
    Button alt_interact = { "alternative interact" };

    Button *binding_map[PlatformInputCode_COUNT];
};
GLOBAL_STATE(Input, input);

static inline Button *
FirstButton(void)
{
    return &input->north;
}

static inline Button *
OnePastLastButton(void)
{
    return &input->alt_interact + 1;
}

#endif /* DUNGEONS_CONTROLLER_HPP */
