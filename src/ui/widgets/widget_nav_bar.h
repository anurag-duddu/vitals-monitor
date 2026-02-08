/**
 * @file widget_nav_bar.h
 * @brief Bottom navigation bar widget
 *
 * Fixed 48px bar at screen bottom. Shows patient identifier on the left
 * and tab-style navigation buttons (Vitals, Trends, Alarms, Patient, Settings).
 */

#ifndef WIDGET_NAV_BAR_H
#define WIDGET_NAV_BAR_H

#include "lvgl.h"
#include "screen_manager.h"

/* Opaque handle */
typedef struct widget_nav_bar widget_nav_bar_t;

/** Create the navigation bar as a child of parent. */
widget_nav_bar_t * widget_nav_bar_create(lv_obj_t *parent);

/** Highlight the currently active screen's button. */
void widget_nav_bar_set_active(widget_nav_bar_t *w, screen_id_t active_screen);

/** Update patient info text (left section). */
void widget_nav_bar_set_patient(widget_nav_bar_t *w, const char *patient_str);

/** Get the underlying LVGL object. */
lv_obj_t * widget_nav_bar_get_obj(widget_nav_bar_t *w);

/** Release back to pool on screen destroy. */
void widget_nav_bar_free(widget_nav_bar_t *w);

#endif /* WIDGET_NAV_BAR_H */
