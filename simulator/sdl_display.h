/**
 * @file sdl_display.h
 * @brief SDL2 display driver for LVGL 9
 */

#ifndef SDL_DISPLAY_H
#define SDL_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <SDL2/SDL.h>
#include <stdbool.h>

/**
 * @brief Initialize SDL2 display
 * @param width Display width in pixels
 * @param height Display height in pixels
 * @return true on success, false on failure
 */
bool sdl_display_init(uint32_t width, uint32_t height);

/**
 * @brief LVGL flush callback
 */
void sdl_display_flush(lv_display_t *disp_drv, const lv_area_t *area, uint8_t *color_p);

/**
 * @brief Get the display object
 * @return Pointer to LVGL display
 */
lv_display_t *sdl_display_get(void);

/**
 * @brief Clean up SDL2 resources
 */
void sdl_display_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* SDL_DISPLAY_H */
