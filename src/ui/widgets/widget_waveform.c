/**
 * @file widget_waveform.c
 * @brief lv_chart-based real-time waveform display with circular sweep
 *
 * Uses lv_chart in CIRCULAR update mode to implement a hospital-style
 * waveform sweep with an erase bar ahead of the write position.
 */

#include "widget_waveform.h"
#include <string.h>
#include <stdio.h>

#define MAX_WAVEFORM_WIDGETS 4

struct widget_waveform {
    bool      in_use;
    lv_obj_t *container;        /* Outer container (label + chart) */
    lv_obj_t *label_obj;        /* Title label */
    lv_obj_t *chart_obj;        /* The lv_chart */
    lv_chart_series_t *series;
    uint32_t  point_count;
    uint32_t  write_pos;        /* Tracks write position for erase bar */
};

static widget_waveform_t wf_pool[MAX_WAVEFORM_WIDGETS];

/* ── Pool management ───────────────────────────────────────── */

static widget_waveform_t * pool_alloc(void)
{
    for (int i = 0; i < MAX_WAVEFORM_WIDGETS; i++) {
        if (!wf_pool[i].in_use) {
            memset(&wf_pool[i], 0, sizeof(wf_pool[i]));
            wf_pool[i].in_use = true;
            return &wf_pool[i];
        }
    }
    printf("[widget_waveform] Pool exhausted!\n");
    return NULL;
}

/* ── Public API ────────────────────────────────────────────── */

widget_waveform_t * widget_waveform_create(
    lv_obj_t *parent,
    const char *label_text,
    lv_color_t color,
    uint32_t point_count,
    int32_t y_min,
    int32_t y_max)
{
    widget_waveform_t *w = pool_alloc();
    if (!w) return NULL;

    w->point_count = point_count;
    w->write_pos = 0;

    /* ── Container: flex column for label + chart ───────── */
    w->container = lv_obj_create(parent);
    lv_obj_remove_flag(w->container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(w->container, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(w->container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(w->container, 0, 0);
    lv_obj_set_style_pad_all(w->container, 0, 0);
    lv_obj_set_style_pad_gap(w->container, VM_PAD_TINY, 0);
    lv_obj_set_flex_flow(w->container, LV_FLEX_FLOW_COLUMN);

    /* ── Title label ───────────────────────────────────── */
    w->label_obj = lv_label_create(w->container);
    lv_label_set_text(w->label_obj, label_text);
    lv_obj_set_style_text_font(w->label_obj, VM_FONT_SMALL, 0);
    lv_obj_set_style_text_color(w->label_obj, color, 0);

    /* ── Chart ─────────────────────────────────────────── */
    w->chart_obj = lv_chart_create(w->container);
    lv_obj_set_width(w->chart_obj, lv_pct(100));
    lv_obj_set_flex_grow(w->chart_obj, 1);

    lv_chart_set_type(w->chart_obj, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(w->chart_obj, point_count);
    lv_chart_set_update_mode(w->chart_obj, LV_CHART_UPDATE_MODE_CIRCULAR);
    lv_chart_set_axis_range(w->chart_obj, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);
    lv_chart_set_div_line_count(w->chart_obj, 0, 0);

    /* Chart styling: dark background, no border, no padding */
    lv_obj_set_style_bg_color(w->chart_obj, VM_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(w->chart_obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(w->chart_obj, 0, 0);
    lv_obj_set_style_pad_all(w->chart_obj, 0, 0);

    /* Trace line: 2px width, colored */
    lv_obj_set_style_line_width(w->chart_obj, 2, LV_PART_ITEMS);
    lv_obj_set_style_line_color(w->chart_obj, color, LV_PART_ITEMS);

    /* Hide point indicator dots */
    lv_obj_set_style_size(w->chart_obj, 0, 0, LV_PART_INDICATOR);

    /* Add data series */
    w->series = lv_chart_add_series(w->chart_obj, color,
                                     LV_CHART_AXIS_PRIMARY_Y);

    /* Initialize all points to NONE (blank chart) */
    lv_chart_set_all_values(w->chart_obj, w->series, LV_CHART_POINT_NONE);

    return w;
}

void widget_waveform_push_sample(widget_waveform_t *w, int32_t value)
{
    if (!w || !w->in_use || !w->chart_obj || !w->series) return;

    /* Write the new value (LVGL circular mode advances internally) */
    lv_chart_set_next_value(w->chart_obj, w->series, value);

    /* Advance our write position tracker */
    w->write_pos = (w->write_pos + 1) % w->point_count;

    /* Set the erase bar: next N points to NONE via direct array access */
    int32_t *y_array = lv_chart_get_series_y_array(w->chart_obj, w->series);
    if (y_array) {
        for (uint32_t i = 1; i <= VM_WAVEFORM_ERASE_WIDTH; i++) {
            uint32_t idx = (w->write_pos + i) % w->point_count;
            y_array[idx] = LV_CHART_POINT_NONE;
        }
    }
}

void widget_waveform_refresh(widget_waveform_t *w)
{
    if (!w || !w->in_use || !w->chart_obj) return;
    lv_chart_refresh(w->chart_obj);
}

lv_obj_t * widget_waveform_get_obj(widget_waveform_t *w)
{
    if (!w || !w->in_use) return NULL;
    return w->container;
}

void widget_waveform_free(widget_waveform_t *w)
{
    if (!w) return;
    w->in_use = false;
    w->container = NULL;
    w->label_obj = NULL;
    w->chart_obj = NULL;
    w->series = NULL;
}
