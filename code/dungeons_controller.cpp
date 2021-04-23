static inline void
HandleButton(Button *button, bool pressed)
{
    if (button->ended_down != pressed)
    {
        button->half_transition_count += 1;
        button->ended_down = pressed;
    }
}

static inline void
HandleController(void)
{
    controller->mouse_p = MakeV2i(platform->mouse_x, platform->mouse_y);
    controller->ui_mouse_p = ScreenToUi(controller->mouse_p);
    controller->world_mouse_p = ScreenToWorld(controller->mouse_p);

    for (Button *button = FirstButton(); button < LastButton(); ++button)
    {
        button->half_transition_count = 0;
    }

    for (PlatformEvent *event = nullptr; NextEvent(&event, PlatformEventFilter_Mouse|PlatformEventFilter_Keyboard);)
    {
        if (event->mouse_button == PlatformMouseButton_Left)
        {
            HandleButton(&controller->interact, event->pressed);
        }
        else if (event->mouse_button == PlatformMouseButton_Right)
        {
            HandleButton(&controller->alt_interact, event->pressed);
        }
        else
        {
            switch (event->key_code)
            {
                case 'W': case PlatformKeyCode_Up:    { HandleButton(&controller->up, event->pressed); } break;
                case 'A': case PlatformKeyCode_Left:  { HandleButton(&controller->left, event->pressed); } break;
                case 'S': case PlatformKeyCode_Down:  { HandleButton(&controller->down, event->pressed); } break;
                case 'D': case PlatformKeyCode_Right: { HandleButton(&controller->right, event->pressed); } break;
                case PlatformKeyCode_Return:          { HandleButton(&controller->interact, event->pressed); } break;
                case PlatformKeyCode_Back:            { HandleButton(&controller->alt_interact, event->pressed); } break;
                default:
                {
                    LeaveUnhandled(event);
                } break;
            }
        }
    }
}
