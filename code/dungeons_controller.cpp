static inline void
HandleButton(Button *button, bool pressed)
{
    if (button->ended_down != pressed)
    {
        button->half_transition_count += 1;
        button->ended_down = pressed;
    }
    else
    {
        button->repeat_count += 1;
    }
}

static inline void
InitializeInputBindings(Arena *string_arena)
{
    input->north.name        = "north"_str;
    input->northeast.name    = "northeast"_str;
    input->east.name         = "east"_str;
    input->southeast.name    = "southeast"_str;
    input->south.name        = "south"_str;
    input->southwest.name    = "southwest"_str;
    input->west.name         = "west"_str;
    input->northwest.name    = "northwest"_str;
    input->here.name         = "here"_str;
    input->interact.name     = "interact"_str;
    input->alt_interact.name = "alt_interact"_str;

    input->f_keys[0].name = "F0? That's a problem."_str;
    for (size_t i = 1; i < ArrayCount(input->f_keys); ++i)
    {
        input->f_keys[i].name = FormatString(string_arena, "F%zu", i);
    }

    input->binding_map['W']                          = &input->north;
    input->binding_map[PlatformInputCode_Up]         = &input->north;
    input->binding_map[PlatformInputCode_Numpad8]    = &input->north;

    input->binding_map['E']                          = &input->northeast;
    input->binding_map[PlatformInputCode_PageUp]     = &input->northeast;
    input->binding_map[PlatformInputCode_Numpad9]    = &input->northeast;

    input->binding_map['D']                          = &input->east;
    input->binding_map[PlatformInputCode_Right]      = &input->east;
    input->binding_map[PlatformInputCode_Numpad6]    = &input->east;

    input->binding_map['C']                          = &input->southeast;
    input->binding_map[PlatformInputCode_PageDown]   = &input->southeast;
    input->binding_map[PlatformInputCode_Numpad3]    = &input->southeast;

    input->binding_map['X']                          = &input->south;
    input->binding_map[PlatformInputCode_Down]       = &input->south;
    input->binding_map[PlatformInputCode_Numpad2]    = &input->south;

    input->binding_map['Z']                          = &input->southwest;
    input->binding_map[PlatformInputCode_End]        = &input->southwest;
    input->binding_map[PlatformInputCode_Numpad1]    = &input->southwest;

    input->binding_map['A']                          = &input->west;
    input->binding_map[PlatformInputCode_Left]       = &input->west;
    input->binding_map[PlatformInputCode_Numpad4]    = &input->west;

    input->binding_map['Q']                          = &input->northwest;
    input->binding_map[PlatformInputCode_Home]       = &input->northwest;
    input->binding_map[PlatformInputCode_Numpad7]    = &input->northwest;

    input->binding_map['S']                          = &input->here;
    input->binding_map[PlatformInputCode_Clear]      = &input->here;
    input->binding_map[PlatformInputCode_Numpad5]    = &input->here;

    input->binding_map[PlatformInputCode_Return]     = &input->interact;
    input->binding_map[PlatformInputCode_Back]       = &input->alt_interact;

    input->binding_map[PlatformInputCode_LButton] = &input->interact;
    input->binding_map[PlatformInputCode_RButton] = &input->alt_interact;

    for (size_t i = 1; i < ArrayCount(input->f_keys); ++i)
    {
        input->binding_map[PlatformInputCode_F1 + i - 1] = &input->f_keys[i];
    }
}

static inline void
HandleInput(void)
{
    input->mouse_p = MakeV2i(platform->mouse_x, platform->mouse_y);
    input->ui_mouse_p = ScreenToUi(input->mouse_p);
    input->world_mouse_p = ScreenToWorld(input->mouse_p);

    for (Button *button = FirstButton(); button < OnePastLastButton(); ++button)
    {
        button->half_transition_count = 0;
        button->repeat_count = 0;
    }

    for (PlatformEvent *event = nullptr; NextEvent(&event, PlatformEventFilter_Mouse|PlatformEventFilter_Keyboard);)
    {
        Button *button = input->binding_map[event->input_code];
        if (button)
        {
            HandleButton(button, event->pressed);
        }
        else
        {
            LeaveUnhandled(event);
        }
    }
}
