/**
 * @file screen_trends.h
 * @brief Trends screen — historical vital sign charts
 */

#ifndef SCREEN_TRENDS_H
#define SCREEN_TRENDS_H

#include "lvgl.h"

/** Create the trends screen (called by screen manager). */
lv_obj_t * screen_trends_create(void);

/** Destroy / release widgets (called by screen manager). */
void screen_trends_destroy(void);

#endif /* SCREEN_TRENDS_H */
