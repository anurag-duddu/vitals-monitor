/**
 * @file widget_waveform.c
 * @brief lv_chart-based real-time waveform display with circular sweep
 *
 * Uses lv_chart in CIRCULAR update mode to implement a hospital-style
 * waveform sweep with an erase bar ahead of the write position.
 */

#include "widget_waveform.h"
#include "theme_styles.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/* UI-3.2: Compile-time check that erase bar is not too wide */
_Static_assert(VM_WAVEFORM_ERASE_WIDTH < VM_WAVEFORM_POINT_COUNT / 4,
               "Erase bar too wide for chart");

#define MAX_WAVEFORM_WIDGETS 4

struct widget_waveform {
    bool      in_use;
    bool      ready;            /* UI-3.1: false during create/free transitions */
    bool      lead_off;         /* UI-3.4: true when lead-off detected */
    lv_obj_t *container;        /* Outer container (label + chart) */
    lv_obj_t *label_obj;        /* Title label */
    lv_obj_t *chart_obj;        /* The lv_chart */
    lv_obj_t *lead_off_lbl;     /* UI-3.4: "LEAD OFF" overlay label */
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
    fprintf(stderr, "[widget_waveform] CRITICAL: Pool exhausted!\n");
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
    lv_obj_add_style(w->container, &s_col, 0);
    lv_obj_set_style_pad_gap(w->container, VM_PAD_TINY, 0);

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
    lv_obj_add_style(w->chart_obj, &s_chart, 0);

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

    /* UI-3.4: Create "LEAD OFF" overlay label (initially hidden) */
    w->lead_off_lbl = lv_label_create(w->chart_obj);
    lv_label_set_text(w->lead_off_lbl, "LEAD OFF");
    lv_obj_set_style_text_font(w->lead_off_lbl, VM_FONT_LABEL, 0);
    lv_obj_set_style_text_color(w->lead_off_lbl, VM_COLOR_ALARM_HIGH, 0);
    lv_obj_center(w->lead_off_lbl);
    lv_obj_add_flag(w->lead_off_lbl, LV_OBJ_FLAG_HIDDEN);
    w->lead_off = false;

    /* UI-3.1: Mark widget as ready for sample writes */
    w->ready = true;

    return w;
}

void widget_waveform_push_sample(widget_waveform_t *w, int32_t value)
{
    if (!w || !w->in_use || !w->chart_obj || !w->series) return;

    /* UI-3.1: Do not write if widget is not ready (screen transition) */
    if (!w->ready) return;

    /* UI-3.4: Do not write if lead-off is active */
    if (w->lead_off) return;

    /* PERF-1.3: Directly write to y_array instead of lv_chart_set_next_value
     * to avoid per-sample lv_chart_refresh() calls. The caller invokes
     * widget_waveform_refresh() once per frame after all samples are pushed. */
    int32_t *y_array = lv_chart_get_series_y_array(w->chart_obj, w->series);
    if (!y_array) return;

    /* Write sample at current position */
    y_array[w->write_pos] = value;

    /* Advance write position */
    w->write_pos = (w->write_pos + 1) % w->point_count;

    /* Set the erase bar: next N points to NONE */
    for (uint32_t i = 0; i < VM_WAVEFORM_ERASE_WIDTH; i++) {
        uint32_t idx = (w->write_pos + i) % w->point_count;
        y_array[idx] = LV_CHART_POINT_NONE;
    }
}

void widget_waveform_refresh(widget_waveform_t *w)
{
    if (!w || !w->in_use || !w->chart_obj) return;

    /* UI-3.1: Do not refresh if widget is not ready (screen transition) */
    if (!w->ready) return;

    lv_chart_refresh(w->chart_obj);
}

lv_obj_t * widget_waveform_get_obj(widget_waveform_t *w)
{
    if (!w || !w->in_use) return NULL;
    return w->container;
}

void widget_waveform_set_lead_off(widget_waveform_t *w, bool is_off)
{
    if (!w || !w->in_use) return;

    w->lead_off = is_off;

    if (w->lead_off_lbl) {
        if (is_off) {
            lv_obj_remove_flag(w->lead_off_lbl, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(w->lead_off_lbl, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void widget_waveform_free(widget_waveform_t *w)
{
    if (!w) return;

    /* UI-3.1: Mark not ready first to prevent writes during teardown */
    w->ready = false;

    w->in_use = false;
    w->lead_off = false;
    w->container = NULL;
    w->label_obj = NULL;
    w->chart_obj = NULL;
    w->lead_off_lbl = NULL;
    w->series = NULL;
}
