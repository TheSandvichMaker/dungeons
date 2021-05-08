#ifndef DUNGEONS_CONTROLLER_HPP
#define DUNGEONS_CONTROLLER_HPP

struct Button
{
    String name;
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

    Button first_button__do_not_remove;

    Button north;
    Button northeast;
    Button east;
    Button southeast;
    Button south;
    Button southwest;
    Button west;
    Button northwest;
    Button here;

    Button interact;
    Button alt_interact;

    Button f_keys[25];

    Button last_button__do_not_remove;

    Button *binding_map[PlatformInputCode_COUNT];
};
GLOBAL_STATE(Input, input);

static inline Button *
FirstButton(void)
{
    return &input->first_button__do_not_remove + 1;
}

static inline Button *
OnePastLastButton(void)
{
    return &input->last_button__do_not_remove;
}

#endif /* DUNGEONS_CONTROLLER_HPP */
