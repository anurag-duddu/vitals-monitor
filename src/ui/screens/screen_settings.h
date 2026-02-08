/**
 * @file screen_settings.h
 * @brief Settings screen — device configuration display
 */

#ifndef SCREEN_SETTINGS_H
#define SCREEN_SETTINGS_H

#include "lvgl.h"

/** Create the settings screen (called by screen manager). */
lv_obj_t * screen_settings_create(void);

/** Destroy / release widgets (called by screen manager). */
void screen_settings_destroy(void);

#endif /* SCREEN_SETTINGS_H */
