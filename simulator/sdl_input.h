/**
 * @file sdl_input.h
 * @brief SDL2 input driver for LVGL 9
 */

#ifndef SDL_INPUT_H
#define SDL_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <SDL2/SDL.h>
#include <stdbool.h>

/**
 * @brief Initialize SDL input devices
 * @return true on success, false on failure
 */
bool sdl_input_init(void);

/**
 * @brief Process SDL events (call this in main loop)
 * @return true to continue, false to quit
 */
bool sdl_input_process_events(void);

/**
 * @brief Get mouse input device
 * @return Pointer to mouse input device
 */
lv_indev_t *sdl_input_get_mouse(void);

/**
 * @brief Get keyboard input device
 * @return Pointer to keyboard input device
 */
lv_indev_t *sdl_input_get_keyboard(void);

#ifdef __cplusplus
}
#endif

#endif /* SDL_INPUT_H */
