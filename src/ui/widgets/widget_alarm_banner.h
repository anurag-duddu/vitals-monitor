/**
 * @file widget_alarm_banner.h
 * @brief Top alarm bar widget
 *
 * Fixed 32px bar at screen top. Displays alarm severity color, message,
 * acknowledge button, and current time. Flashes per IEC 60601-1-8.
 */

#ifndef WIDGET_ALARM_BANNER_H
#define WIDGET_ALARM_BANNER_H

#include "lvgl.h"
#include "theme_vitals.h"

/* Opaque handle */
typedef struct widget_alarm_banner widget_alarm_banner_t;

/** Create the alarm banner as a child of parent (one per screen). */
widget_alarm_banner_t * widget_alarm_banner_create(lv_obj_t *parent);

/** Set an active alarm with severity and message text. */
void widget_alarm_banner_set(widget_alarm_banner_t *w,
                              vm_alarm_severity_t severity,
                              const char *message);

/** Clear alarm (return to green "No Alarms" state). */
void widget_alarm_banner_clear(widget_alarm_banner_t *w);

/** Update the clock display string (right side). */
void widget_alarm_banner_set_time(widget_alarm_banner_t *w, const char *time_str);

/** Get the underlying LVGL object (for positioning). */
lv_obj_t * widget_alarm_banner_get_obj(widget_alarm_banner_t *w);

/** Release back to pool on screen destroy. */
void widget_alarm_banner_free(widget_alarm_banner_t *w);

#endif /* WIDGET_ALARM_BANNER_H */
