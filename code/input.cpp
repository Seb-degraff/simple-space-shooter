
bool keys_state[KB_KEY_LAST+1];
bool keys_state_prev[KB_KEY_LAST+1];
bool mouse_button_state[MOUSE_BTN_7+1];
bool mouse_button_state_prev[MOUSE_BTN_7+1];

int mouse_x;
int mouse_y;

void tick_input()
{
    memcpy(keys_state_prev, keys_state, sizeof(keys_state));
    memcpy(mouse_button_state_prev, mouse_button_state, sizeof(mouse_button_state));
}

bool key_is_down(mfb_key key)
{
    return keys_state[key];
}

bool key_was_just_pressed(mfb_key key)
{
    return !keys_state_prev[key] && keys_state[key];
}

bool key_was_just_released(mfb_key key)
{
    return keys_state_prev[key] && !keys_state[key];
}

bool mouse_is_down(mfb_mouse_button button)
{
    return mouse_button_state[button];
}

bool mouse_was_just_pressed(mfb_mouse_button button)
{
    return !mouse_button_state_prev[button] && mouse_button_state[button];
}

bool mouse_was_just_released(mfb_mouse_button button)
{
    return mouse_button_state_prev[button] && !mouse_button_state[button];
}

#define UNUSED(x) ((void) x)

void on_keyboard_event(struct mfb_window *window, mfb_key key, mfb_key_mod mod, bool is_pressed)
{
    UNUSED(window);
    UNUSED(mod);

    keys_state[key] = is_pressed;
}

void on_mouse_button(struct mfb_window *window, mfb_mouse_button button, mfb_key_mod mod, bool is_pressed)
{
    UNUSED(window);
    UNUSED(mod);

    mouse_button_state[button] = is_pressed;
}


void on_mouse_move(struct mfb_window *window, int x, int y)
{
    UNUSED(window);
    mouse_x = x / WINDOW_FAC;
    mouse_y = y / WINDOW_FAC; 
}

