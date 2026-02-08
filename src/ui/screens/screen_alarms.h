/**
 * @file screen_alarms.h
 * @brief Alarms screen — alarm history log and limit settings
 */

#ifndef SCREEN_ALARMS_H
#define SCREEN_ALARMS_H

#include "lvgl.h"

/** Create the alarms screen (called by screen manager). */
lv_obj_t * screen_alarms_create(void);

/** Destroy / release widgets (called by screen manager). */
void screen_alarms_destroy(void);

#endif /* SCREEN_ALARMS_H */
