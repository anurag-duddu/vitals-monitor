/**
 * @file screen_trends.c
 * @brief Trends screen — shows HR, SpO2, and RR history as line charts
 *
 * Layout (800x480):
 *   ┌──────────────────────────────────────────────────────────────┐
 *   │ ALARM BAR (32px)                                            │
 *   ├──────────────────────────────────────────────────────────────┤
 *   │  Trends - Last 2 Minutes                                    │
 *   │ ┌─────────────────────────────────────────────────────────┐ │
 *   │ │  HR chart (green line)                                  │ │
 *   │ ├─────────────────────────────────────────────────────────┤ │
 *   │ │  SpO2 chart (cyan line)                                 │ │
 *   │ ├─────────────────────────────────────────────────────────┤ │
 *   │ │  RR chart (yellow line)                                 │ │
 *   │ └─────────────────────────────────────────────────────────┘ │
 *   ├──────────────────────────────────────────────────────────────┤
 *   │ NAV BAR (48px)                                              │
 *   └──────────────────────────────────────────────────────────────┘
 */

#include "screen_trends.h"
#include "screen_manager.h"
#include "widget_alarm_banner.h"
#include "widget_nav_bar.h"
#include "theme_vitals.h"
#include "vitals_provider.h"
#include <stdio.h>

/* ── Module state ──────────────────────────────────────────── */

static widget_alarm_banner_t *alarm_banner;
static widget_nav_bar_t      *nav_bar;
static lv_obj_t              *hr_chart;
static lv_obj_t              *spo2_chart;
static lv_obj_t              *rr_chart;
static lv_chart_series_t     *hr_series;
static lv_chart_series_t     *spo2_series;
static lv_chart_series_t     *rr_series;
static lv_timer_t            *refresh_timer;

/* ── Forward declarations ──────────────────────────────────── */

static lv_obj_t * create_trend_chart(lv_obj_t *parent, const char *title,
                                      lv_color_t color, int y_min, int y_max,
                                      lv_chart_series_t **series_out);
static void populate_chart_from_history(lv_obj_t *chart, lv_chart_series_t *series,
                                         const int *ring_buf, int count, int write_idx);
static void refresh_timer_cb(lv_timer_t *timer);

/* ── Public API ────────────────────────────────────────────── */

lv_obj_t * screen_trends_create(void) {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, VM_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* ── Alarm banner (top) ──────────────────────────────── */
    alarm_banner = widget_alarm_banner_create(scr);

    /* ── Content area ────────────────────────────────────── */
    lv_obj_t *content = lv_obj_create(scr);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(content, 0, VM_ALARM_BAR_HEIGHT);
    lv_obj_set_size(content, VM_SCREEN_WIDTH, VM_CONTENT_HEIGHT);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, VM_PAD_NORMAL, 0);
    lv_obj_set_style_pad_gap(content, VM_PAD_SMALL, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);

    /* Title */
    lv_obj_t *title = lv_label_create(content);
    lv_label_set_text(title, "Trends  \xE2\x80\x94  Last 2 Minutes");
    lv_obj_set_style_text_font(title, VM_FONT_BODY, 0);
    lv_obj_set_style_text_color(title, VM_COLOR_TEXT_PRIMARY, 0);

    /* Three charts stacked vertically */
    hr_chart   = create_trend_chart(content, "HR (bpm)",   VM_COLOR_HR,   40, 160, &hr_series);
    spo2_chart = create_trend_chart(content, "SpO2 (%)",   VM_COLOR_SPO2, 85, 100, &spo2_series);
    rr_chart   = create_trend_chart(content, "RR (/min)",  VM_COLOR_RR,    5,  35, &rr_series);

    /* Populate with existing history data */
    const vitals_history_t *hist = vitals_provider_get_history(0);
    populate_chart_from_history(hr_chart,   hr_series,   hist->hr,   hist->count, hist->write_idx);
    populate_chart_from_history(spo2_chart, spo2_series, hist->spo2, hist->count, hist->write_idx);
    populate_chart_from_history(rr_chart,   rr_series,   hist->rr,   hist->count, hist->write_idx);

    /* Refresh charts every second */
    refresh_timer = lv_timer_create(refresh_timer_cb, 1000, NULL);

    /* ── Nav bar (bottom) ────────────────────────────────── */
    nav_bar = widget_nav_bar_create(scr);
    widget_nav_bar_set_active(nav_bar, SCREEN_ID_TRENDS);
    widget_nav_bar_set_patient(nav_bar, "No Patient");

    printf("[trends] Screen created\n");
    return scr;
}

void screen_trends_destroy(void) {
    if (refresh_timer) {
        lv_timer_delete(refresh_timer);
        refresh_timer = NULL;
    }
    widget_alarm_banner_free(alarm_banner);
    widget_nav_bar_free(nav_bar);
    alarm_banner = NULL;
    nav_bar = NULL;
    hr_chart = NULL;
    spo2_chart = NULL;
    rr_chart = NULL;
    hr_series = NULL;
    spo2_series = NULL;
    rr_series = NULL;
    printf("[trends] Screen destroyed\n");
}

/* ── Private helpers ───────────────────────────────────────── */

static lv_obj_t * create_trend_chart(lv_obj_t *parent, const char *title,
                                      lv_color_t color, int y_min, int y_max,
                                      lv_chart_series_t **series_out) {
    /* Row container: label on left, chart fills rest */
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_flex_grow(row, 1);
    lv_obj_set_style_bg_color(row, VM_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(row, VM_COLOR_BG_PANEL_BORDER, 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_radius(row, 4, 0);
    lv_obj_set_style_pad_all(row, VM_PAD_SMALL, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(row, VM_PAD_SMALL, 0);

    /* Label column (fixed width) */
    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_font(lbl, VM_FONT_CAPTION, 0);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_width(lbl, 80);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_pad_top(lbl, VM_PAD_NORMAL, 0);

    /* Chart */
    lv_obj_t *chart = lv_chart_create(row);
    lv_obj_set_flex_grow(chart, 1);
    lv_obj_set_height(chart, lv_pct(100));
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, VITALS_HISTORY_LEN);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);
    lv_chart_set_div_line_count(chart, 3, 0);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);

    /* Chart styling */
    lv_obj_set_style_bg_color(chart, VM_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(chart, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(chart, 0, 0);
    lv_obj_set_style_line_color(chart, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_pad_all(chart, VM_PAD_TINY, 0);

    /* Series */
    lv_chart_series_t *ser = lv_chart_add_series(chart, color, LV_CHART_AXIS_PRIMARY_Y);
    lv_obj_set_style_line_width(chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(chart, 0, 0, LV_PART_INDICATOR);  /* Hide point dots */

    *series_out = ser;
    return chart;
}

static void populate_chart_from_history(lv_obj_t *chart, lv_chart_series_t *series,
                                         const int *ring_buf, int count, int write_idx) {
    int len = VITALS_HISTORY_LEN;
    int available = count < len ? count : len;

    /* Start from oldest data point */
    int start = (write_idx - available + len) % len;
    for (int i = 0; i < available; i++) {
        int idx = (start + i) % len;
        lv_chart_set_next_value(chart, series, ring_buf[idx]);
    }
    lv_chart_refresh(chart);
}

static void refresh_timer_cb(lv_timer_t *timer) {
    (void)timer;
    if (!hr_chart || !spo2_chart || !rr_chart) return;

    const vitals_data_t *data = vitals_provider_get_current(0);

    /* Append latest value to each chart (shift mode auto-scrolls) */
    lv_chart_set_next_value(hr_chart,   hr_series,   data->hr);
    lv_chart_set_next_value(spo2_chart, spo2_series, data->spo2);
    lv_chart_set_next_value(rr_chart,   rr_series,   data->rr);
}
