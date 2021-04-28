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
InitializeInputBindings(void)
{
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

    input->binding_map[PlatformInputCode_MouseLeft]  = &input->interact;
    input->binding_map[PlatformInputCode_MouseRight] = &input->alt_interact;
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
