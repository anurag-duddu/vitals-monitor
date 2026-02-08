/**
 * @file widget_waveform.h
 * @brief lv_chart-based real-time waveform display widget
 *
 * Wraps an lv_chart in CIRCULAR update mode with a hospital-style
 * sweep and erase bar.  Includes a small label for the waveform name.
 */

#ifndef WIDGET_WAVEFORM_H
#define WIDGET_WAVEFORM_H

#include "lvgl.h"
#include "theme_vitals.h"

/* Opaque handle */
typedef struct widget_waveform widget_waveform_t;

/**
 * Create a waveform chart widget.
 *
 * @param parent      LVGL parent object
 * @param label_text  Title (e.g. "ECG  Lead II  25mm/s")
 * @param color       Trace color
 * @param point_count Number of data points (chart width)
 * @param y_min       Chart Y-axis minimum
 * @param y_max       Chart Y-axis maximum
 * @return Handle, or NULL if pool exhausted
 */
widget_waveform_t * widget_waveform_create(
    lv_obj_t *parent,
    const char *label_text,
    lv_color_t color,
    uint32_t point_count,
    int32_t y_min,
    int32_t y_max
);

/**
 * Push a new sample value into the circular chart.
 * Manages the erase bar automatically.
 * Call lv_chart_refresh() is deferred — use widget_waveform_refresh()
 * after pushing all samples for the frame.
 */
void widget_waveform_push_sample(widget_waveform_t *w, int32_t value);

/**
 * Refresh the chart display. Call once per frame after all samples pushed.
 */
void widget_waveform_refresh(widget_waveform_t *w);

/** Get the underlying LVGL container (for sizing by parent). */
lv_obj_t * widget_waveform_get_obj(widget_waveform_t *w);

/** Release back to pool on screen destroy. */
void widget_waveform_free(widget_waveform_t *w);

#endif /* WIDGET_WAVEFORM_H */
