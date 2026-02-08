/**
 * @file lv_drv_conf.h
 * Configuration file for LVGL drivers
 */

#ifndef LV_DRV_CONF_H
#define LV_DRV_CONF_H

#include "lv_conf.h"

/*==================
 * SDL CONFIGURATION
 *==================*/

#define USE_SDL 1
#if USE_SDL
#  define SDL_HOR_RES     800
#  define SDL_VER_RES     480
#  define SDL_ZOOM        1    /* Zoom level (1, 2, etc.) */
#  define SDL_DOUBLE_BUFFERED 1
#endif

/*==================
 * MONITOR CONFIGURATION
 *==================*/

#define USE_MONITOR 1
#if USE_MONITOR
#  define MONITOR_HOR_RES LV_HOR_RES_MAX
#  define MONITOR_VER_RES LV_VER_RES_MAX
#  define MONITOR_ZOOM    1
#  define MONITOR_DOUBLE_BUFFERED 1
#endif

/*==================
 * MOUSE CONFIGURATION
 *==================*/

#define USE_MOUSE 1
#if USE_MOUSE
#  define MOUSE_CURSOR_PATH ""  /* Empty = no cursor image */
#endif

/*==================
 * MOUSEWHEEL CONFIGURATION
 *==================*/

#define USE_MOUSEWHEEL 1

/*==================
 * KEYBOARD CONFIGURATION
 *==================*/

#define USE_KEYBOARD 1

#endif  /*LV_DRV_CONF_H*/
