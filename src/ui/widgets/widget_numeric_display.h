/**
 * @file widget_numeric_display.h
 * @brief Vital sign numeric display widget
 *
 * Displays a single vital parameter with label, large numeric value, and unit.
 * Supports three size variants and alarm-state border coloring.
 */

#ifndef WIDGET_NUMERIC_DISPLAY_H
#define WIDGET_NUMERIC_DISPLAY_H

#include "lvgl.h"
#include "theme_vitals.h"

/* Size variants for different vital parameter importance */
typedef enum {
    NUMERIC_SIZE_LARGE,   /* HR, SpO2 — 48pt value */
    NUMERIC_SIZE_MEDIUM,  /* NIBP — 32pt value */
    NUMERIC_SIZE_SMALL    /* Temp, RR — 24pt value (montserrat_24) */
} numeric_display_size_t;

/* Opaque handle */
typedef struct widget_numeric_display widget_numeric_display_t;

/**
 * Create a numeric display widget as a child of parent.
 *
 * @param parent       LVGL parent object
 * @param label_text   Parameter name (e.g., "HR")
 * @param unit_text    Unit string (e.g., "bpm")
 * @param color        Parameter color (e.g., VM_COLOR_HR)
 * @param size         Size variant
 * @return Handle, or NULL if pool exhausted
 */
widget_numeric_display_t * widget_numeric_display_create(
    lv_obj_t *parent,
    const char *label_text,
    const char *unit_text,
    lv_color_t color,
    numeric_display_size_t size
);

/** Update the displayed value string (e.g., "72", "120/80"). */
void widget_numeric_display_set_value(widget_numeric_display_t *w, const char *value_str);

/** Update alarm state — changes border color of the container. */
void widget_numeric_display_set_alarm(widget_numeric_display_t *w, vm_alarm_severity_t severity);

/** Get the underlying LVGL container object (for sizing/positioning). */
lv_obj_t * widget_numeric_display_get_obj(widget_numeric_display_t *w);

/** Release a widget back to the pool (called on screen destroy). */
void widget_numeric_display_free(widget_numeric_display_t *w);

#endif /* WIDGET_NUMERIC_DISPLAY_H */
