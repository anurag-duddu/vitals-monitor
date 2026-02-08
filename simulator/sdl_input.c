/**
 * @file sdl_input.c
 * @brief SDL2 input driver for LVGL 9 (mouse and keyboard)
 */

#include "sdl_input.h"

/* Input device handles */
static lv_indev_t *indev_mouse = NULL;
static lv_indev_t *indev_keyboard = NULL;

/* Mouse state */
static int32_t mouse_x = 0;
static int32_t mouse_y = 0;
static bool mouse_pressed = false;

/**
 * @brief Process SDL events (mouse, keyboard, window)
 * @return true to continue, false to quit
 */
bool sdl_input_process_events(void) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                return false;

            case SDL_MOUSEMOTION:
                mouse_x = event.motion.x;
                mouse_y = event.motion.y;
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    mouse_pressed = true;
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    mouse_pressed = false;
                }
                break;

            default:
                break;
        }
    }

    return true;
}

/**
 * @brief Mouse input read callback for LVGL
 */
static void sdl_mouse_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    (void)indev; /* Unused */

    /* Get mouse state */
    data->point.x = mouse_x;
    data->point.y = mouse_y;
    data->state = mouse_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

/**
 * @brief Initialize SDL input devices
 */
bool sdl_input_init(void) {
    /* Create mouse input device */
    indev_mouse = lv_indev_create();
    if (!indev_mouse) {
        SDL_Log("Failed to create mouse input device");
        return false;
    }

    lv_indev_set_type(indev_mouse, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_mouse, sdl_mouse_read_cb);

    SDL_Log("SDL input devices initialized (mouse)");
    return true;
}

/**
 * @brief Get mouse input device
 */
lv_indev_t *sdl_input_get_mouse(void) {
    return indev_mouse;
}

/**
 * @brief Get keyboard input device
 */
lv_indev_t *sdl_input_get_keyboard(void) {
    return indev_keyboard;
}
