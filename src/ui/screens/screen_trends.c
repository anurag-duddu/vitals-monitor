/**
 * @file screen_trends.c
 * @brief Trends screen — SQLite-backed historical vital sign charts
 *
 * Layout (800x480):
 *   ┌──────────────────────────────────────────────────────────────┐
 *   │ ALARM BAR (32px)                                            │
 *   ├──────────────────────────────────────────────────────────────┤
 *   │  Trends  [1h] [4h] [8h] [12h] [24h] [72h]                  │
 *   │ ┌────┬──────────────────────────────────────────────────────┐│
 *   │ │ HR │  green line + red/yellow threshold lines             ││
 *   │ ├────┼──────────────────────────────────────────────────────┤│
 *   │ │SpO2│  cyan line + threshold lines                        ││
 *   │ ├────┼──────────────────────────────────────────────────────┤│
 *   │ │ RR │  yellow line + threshold lines                      ││
 *   │ ├────┼──────────────────────────────────────────────────────┤│
 *   │ │BP/T│  white NIBP scatter + orange Temp overlay           ││
 *   │ └────┴──────────────────────────────────────────────────────┘│
 *   ├──────────────────────────────────────────────────────────────┤
 *   │ NAV BAR (48px)                                              │
 *   └──────────────────────────────────────────────────────────────┘
 *
 * Data source:
 *   - ≤2h ranges: vitals_raw table (1-sec resolution, downsampled)
 *   - >2h ranges: vitals_1min table (1-min aggregates, downsampled)
 *   - NIBP: nibp_measurements table (discrete events)
 *   - Alarms: alarm_events table (vertical markers on HR chart)
 */

#include "screen_trends.h"
#include "screen_manager.h"
#include "widget_alarm_banner.h"
#include "widget_nav_bar.h"
#include "theme_vitals.h"
#include "vitals_provider.h"
#include "trend_db.h"
#include <stdio.h>
#include <string.h>

/* ── Constants ────────────────────────────────────────────── */

#define CHART_POINTS        TREND_DB_MAX_POINTS  /* 480 points per chart */
#define RANGE_COUNT         6
#define MAX_ALARM_MARKERS   20
#define REFRESH_INTERVAL_MS 10000  /* 10 seconds */

/* Alarm threshold values (must match main.c evaluate_alarms) */
#define THRESH_HR_CRIT_HI   150
#define THRESH_HR_WARN_HI   120
#define THRESH_HR_WARN_LO    50
#define THRESH_HR_CRIT_LO    40

#define THRESH_SPO2_CRIT_LO  85
#define THRESH_SPO2_WARN_LO  90

#define THRESH_RR_CRIT_HI    30
#define THRESH_RR_WARN_HI    24
#define THRESH_RR_WARN_LO    10
#define THRESH_RR_CRIT_LO     8

/* Time range presets */
static const trend_range_t range_values[RANGE_COUNT] = {
    TREND_RANGE_1H, TREND_RANGE_4H, TREND_RANGE_8H,
    TREND_RANGE_12H, TREND_RANGE_24H, TREND_RANGE_72H
};
static const char *range_texts[RANGE_COUNT] = {
    "1h", "4h", "8h", "12h", "24h", "72h"
};

/* ── Module state ──────────────────────────────────────────── */

static widget_alarm_banner_t *alarm_banner;
static widget_nav_bar_t      *nav_bar;

/* Time range selector */
static lv_obj_t *range_btns[RANGE_COUNT];
static lv_obj_t *range_btn_labels[RANGE_COUNT];
static int       active_range_idx = 0;

/* Charts */
static lv_obj_t *hr_chart, *spo2_chart, *rr_chart, *nibp_temp_chart;

/* Data series (added last so they draw on top of thresholds) */
static lv_chart_series_t *hr_series;
static lv_chart_series_t *spo2_series;
static lv_chart_series_t *rr_series;
static lv_chart_series_t *nibp_sys_series, *nibp_dia_series;
static lv_chart_series_t *temp_series;

/* Threshold series (added first so they draw behind data) */
static lv_chart_series_t *hr_th[4];    /* hi-crit, hi-warn, lo-warn, lo-crit */
static lv_chart_series_t *spo2_th[2];  /* lo-crit, lo-warn */
static lv_chart_series_t *rr_th[4];    /* hi-crit, hi-warn, lo-warn, lo-crit */

/* Alarm event markers (thin colored bars on HR chart) */
static lv_obj_t *alarm_markers[MAX_ALARM_MARKERS];
static int       alarm_marker_count = 0;

/* Refresh timer */
static lv_timer_t *refresh_timer;

/* Static query result buffers (avoids heap allocation) */
static trend_query_result_t query_buf;
static trend_nibp_result_t  nibp_buf;
static trend_alarm_result_t alarm_buf;

/* ── Forward declarations ──────────────────────────────────── */

static lv_obj_t * create_trend_chart(lv_obj_t *parent, const char *title,
                                      lv_color_t color, int y_min, int y_max);
static lv_chart_series_t * add_threshold_series(lv_obj_t *chart, int value,
                                                 lv_color_t color);
static void populate_chart_from_db(lv_obj_t *chart, lv_chart_series_t *series,
                                    trend_param_t param,
                                    uint32_t start_ts, uint32_t end_ts);
static void populate_nibp_from_db(uint32_t start_ts, uint32_t end_ts);
static void populate_temp_from_db(uint32_t start_ts, uint32_t end_ts);
static void update_alarm_markers(uint32_t start_ts, uint32_t end_ts);
static void clear_alarm_markers(void);
static void refresh_all_charts(void);
static void refresh_timer_cb(lv_timer_t *timer);
static void range_btn_cb(lv_event_t *e);
static void update_range_highlight(void);
static uint32_t get_current_ts(void);

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
    lv_obj_set_style_pad_all(content, VM_PAD_SMALL, 0);
    lv_obj_set_style_pad_gap(content, VM_PAD_TINY, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);

    /* ── Title row with time range buttons ───────────────── */
    lv_obj_t *title_row = lv_obj_create(content);
    lv_obj_remove_flag(title_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(title_row, lv_pct(100));
    lv_obj_set_height(title_row, 28);
    lv_obj_set_style_bg_opa(title_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(title_row, 0, 0);
    lv_obj_set_style_pad_all(title_row, 0, 0);
    lv_obj_set_style_pad_gap(title_row, VM_PAD_SMALL, 0);
    lv_obj_set_flex_flow(title_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(title_row, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *title = lv_label_create(title_row);
    lv_label_set_text(title, "Trends");
    lv_obj_set_style_text_font(title, VM_FONT_BODY, 0);
    lv_obj_set_style_text_color(title, VM_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_width(title, 65);

    for (int i = 0; i < RANGE_COUNT; i++) {
        range_btns[i] = lv_button_create(title_row);
        lv_obj_set_size(range_btns[i], 48, 24);
        lv_obj_set_style_radius(range_btns[i], 4, 0);
        lv_obj_set_style_pad_all(range_btns[i], 2, 0);
        lv_obj_set_style_bg_opa(range_btns[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(range_btns[i], 1, 0);
        lv_obj_set_style_border_color(range_btns[i], VM_COLOR_BG_PANEL_BORDER, 0);

        range_btn_labels[i] = lv_label_create(range_btns[i]);
        lv_label_set_text(range_btn_labels[i], range_texts[i]);
        lv_obj_set_style_text_font(range_btn_labels[i], VM_FONT_SMALL, 0);
        lv_obj_center(range_btn_labels[i]);

        lv_obj_add_event_cb(range_btns[i], range_btn_cb,
                            LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }

    active_range_idx = 0;
    update_range_highlight();

    /* ── HR chart ────────────────────────────────────────── */
    hr_chart = create_trend_chart(content, "HR", VM_COLOR_HR, 40, 160);
    hr_th[0] = add_threshold_series(hr_chart, THRESH_HR_CRIT_HI, VM_COLOR_ALARM_HIGH);
    hr_th[1] = add_threshold_series(hr_chart, THRESH_HR_WARN_HI, VM_COLOR_ALARM_MEDIUM);
    hr_th[2] = add_threshold_series(hr_chart, THRESH_HR_WARN_LO, VM_COLOR_ALARM_MEDIUM);
    hr_th[3] = add_threshold_series(hr_chart, THRESH_HR_CRIT_LO, VM_COLOR_ALARM_HIGH);
    hr_series = lv_chart_add_series(hr_chart, VM_COLOR_HR, LV_CHART_AXIS_PRIMARY_Y);

    /* ── SpO2 chart ──────────────────────────────────────── */
    spo2_chart = create_trend_chart(content, "SpO2", VM_COLOR_SPO2, 80, 100);
    spo2_th[0] = add_threshold_series(spo2_chart, THRESH_SPO2_CRIT_LO, VM_COLOR_ALARM_HIGH);
    spo2_th[1] = add_threshold_series(spo2_chart, THRESH_SPO2_WARN_LO, VM_COLOR_ALARM_MEDIUM);
    spo2_series = lv_chart_add_series(spo2_chart, VM_COLOR_SPO2, LV_CHART_AXIS_PRIMARY_Y);

    /* ── RR chart ────────────────────────────────────────── */
    rr_chart = create_trend_chart(content, "RR", VM_COLOR_RR, 5, 35);
    rr_th[0] = add_threshold_series(rr_chart, THRESH_RR_CRIT_HI, VM_COLOR_ALARM_HIGH);
    rr_th[1] = add_threshold_series(rr_chart, THRESH_RR_WARN_HI, VM_COLOR_ALARM_MEDIUM);
    rr_th[2] = add_threshold_series(rr_chart, THRESH_RR_WARN_LO, VM_COLOR_ALARM_MEDIUM);
    rr_th[3] = add_threshold_series(rr_chart, THRESH_RR_CRIT_LO, VM_COLOR_ALARM_HIGH);
    rr_series = lv_chart_add_series(rr_chart, VM_COLOR_RR, LV_CHART_AXIS_PRIMARY_Y);

    /* ── NIBP + Temperature chart ────────────────────────── */
    nibp_temp_chart = create_trend_chart(content, "BP/T", VM_COLOR_NIBP, 40, 200);
    lv_chart_set_range(nibp_temp_chart, LV_CHART_AXIS_SECONDARY_Y, 350, 400);
    nibp_sys_series = lv_chart_add_series(nibp_temp_chart, VM_COLOR_NIBP,
                                           LV_CHART_AXIS_PRIMARY_Y);
    nibp_dia_series = lv_chart_add_series(nibp_temp_chart,
                                           lv_color_hex(0xBBBBBB),
                                           LV_CHART_AXIS_PRIMARY_Y);
    temp_series = lv_chart_add_series(nibp_temp_chart, VM_COLOR_TEMP,
                                       LV_CHART_AXIS_SECONDARY_Y);

    /* Initial data load */
    refresh_all_charts();

    /* Auto-refresh every 10 seconds */
    refresh_timer = lv_timer_create(refresh_timer_cb, REFRESH_INTERVAL_MS, NULL);

    /* ── Nav bar (bottom) ────────────────────────────────── */
    nav_bar = widget_nav_bar_create(scr);
    widget_nav_bar_set_active(nav_bar, SCREEN_ID_TRENDS);
    widget_nav_bar_set_patient(nav_bar, "No Patient");

    printf("[trends] Screen created (range=%s)\n", range_texts[active_range_idx]);
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
    hr_chart = spo2_chart = rr_chart = nibp_temp_chart = NULL;
    hr_series = spo2_series = rr_series = NULL;
    nibp_sys_series = nibp_dia_series = temp_series = NULL;
    memset(hr_th, 0, sizeof(hr_th));
    memset(spo2_th, 0, sizeof(spo2_th));
    memset(rr_th, 0, sizeof(rr_th));
    alarm_marker_count = 0;
    memset(alarm_markers, 0, sizeof(alarm_markers));

    printf("[trends] Screen destroyed\n");
}

/* ── Private helpers ───────────────────────────────────────── */

static uint32_t get_current_ts(void) {
    const vitals_data_t *d = vitals_provider_get_current(0);
    return d ? (uint32_t)(d->timestamp_ms / 1000) : 0;
}

/* ── Time range selector ──────────────────────────────────── */

static void update_range_highlight(void) {
    for (int i = 0; i < RANGE_COUNT; i++) {
        if (i == active_range_idx) {
            lv_obj_set_style_bg_color(range_btns[i], VM_COLOR_BG_PANEL_BORDER, 0);
            lv_obj_set_style_text_color(range_btn_labels[i], VM_COLOR_TEXT_PRIMARY, 0);
        } else {
            lv_obj_set_style_bg_color(range_btns[i], VM_COLOR_BG, 0);
            lv_obj_set_style_text_color(range_btn_labels[i], VM_COLOR_TEXT_SECONDARY, 0);
        }
    }
}

static void range_btn_cb(lv_event_t *e) {
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx >= RANGE_COUNT || idx == active_range_idx) return;

    active_range_idx = idx;
    update_range_highlight();
    refresh_all_charts();
    printf("[trends] Range changed to %s\n", range_texts[idx]);
}

/* ── Chart creation ───────────────────────────────────────── */

static lv_obj_t * create_trend_chart(lv_obj_t *parent, const char *title,
                                      lv_color_t color, int y_min, int y_max) {
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

    /* Label column */
    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_font(lbl, VM_FONT_CAPTION, 0);
    lv_obj_set_style_text_color(lbl, color, 0);
    lv_obj_set_width(lbl, 38);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_pad_top(lbl, VM_PAD_NORMAL, 0);

    /* Chart */
    lv_obj_t *chart = lv_chart_create(row);
    lv_obj_set_flex_grow(chart, 1);
    lv_obj_set_height(chart, lv_pct(100));
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, CHART_POINTS);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, y_min, y_max);
    lv_chart_set_div_line_count(chart, 3, 0);

    /* Chart styling */
    lv_obj_set_style_bg_color(chart, VM_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(chart, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(chart, 0, 0);
    lv_obj_set_style_line_color(chart, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_obj_set_style_pad_all(chart, VM_PAD_TINY, 0);
    lv_obj_set_style_line_width(chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(chart, 0, 0, LV_PART_INDICATOR);

    return chart;
}

static lv_chart_series_t * add_threshold_series(lv_obj_t *chart, int value,
                                                 lv_color_t color) {
    lv_chart_series_t *ser = lv_chart_add_series(chart, color,
                                                  LV_CHART_AXIS_PRIMARY_Y);
    int32_t *y = lv_chart_get_series_y_array(chart, ser);
    for (int i = 0; i < CHART_POINTS; i++) {
        y[i] = value;
    }
    return ser;
}

/* ── Data population from SQLite ──────────────────────────── */

static void populate_chart_from_db(lv_obj_t *chart, lv_chart_series_t *series,
                                    trend_param_t param,
                                    uint32_t start_ts, uint32_t end_ts) {
    if (!chart || !series) return;

    trend_db_query_param(param, start_ts, end_ts, CHART_POINTS, &query_buf);

    int32_t *y = lv_chart_get_series_y_array(chart, series);

    /* Leading gap filled with NONE (no data yet for that time window) */
    int gap = CHART_POINTS - query_buf.count;
    for (int i = 0; i < gap; i++) {
        y[i] = LV_CHART_POINT_NONE;
    }
    for (int i = 0; i < query_buf.count; i++) {
        y[gap + i] = query_buf.value[i];
    }
    lv_chart_refresh(chart);
}

static void populate_nibp_from_db(uint32_t start_ts, uint32_t end_ts) {
    if (!nibp_temp_chart || !nibp_sys_series || !nibp_dia_series) return;

    trend_db_query_nibp(start_ts, end_ts, &nibp_buf);

    int32_t *sys_y = lv_chart_get_series_y_array(nibp_temp_chart, nibp_sys_series);
    int32_t *dia_y = lv_chart_get_series_y_array(nibp_temp_chart, nibp_dia_series);

    /* All points start as NONE (NIBP is sparse) */
    for (int i = 0; i < CHART_POINTS; i++) {
        sys_y[i] = LV_CHART_POINT_NONE;
        dia_y[i] = LV_CHART_POINT_NONE;
    }

    uint32_t range = end_ts - start_ts;
    if (range == 0) return;

    for (int i = 0; i < nibp_buf.count; i++) {
        int pos = (int)((uint64_t)(nibp_buf.timestamp_s[i] - start_ts)
                        * CHART_POINTS / range);
        if (pos >= 0 && pos < CHART_POINTS) {
            sys_y[pos] = nibp_buf.sys[i];
            dia_y[pos] = nibp_buf.dia[i];
        }
    }
    lv_chart_refresh(nibp_temp_chart);
}

static void populate_temp_from_db(uint32_t start_ts, uint32_t end_ts) {
    if (!nibp_temp_chart || !temp_series) return;

    trend_db_query_param(TREND_PARAM_TEMP, start_ts, end_ts,
                          CHART_POINTS, &query_buf);

    int32_t *y = lv_chart_get_series_y_array(nibp_temp_chart, temp_series);

    int gap = CHART_POINTS - query_buf.count;
    for (int i = 0; i < gap; i++) {
        y[i] = LV_CHART_POINT_NONE;
    }
    for (int i = 0; i < query_buf.count; i++) {
        y[gap + i] = query_buf.value[i];  /* Already x10 from DB */
    }
    lv_chart_refresh(nibp_temp_chart);
}

/* ── Alarm event markers ──────────────────────────────────── */

static void clear_alarm_markers(void) {
    for (int i = 0; i < alarm_marker_count; i++) {
        if (alarm_markers[i]) {
            lv_obj_delete(alarm_markers[i]);
            alarm_markers[i] = NULL;
        }
    }
    alarm_marker_count = 0;
}

static void update_alarm_markers(uint32_t start_ts, uint32_t end_ts) {
    clear_alarm_markers();
    if (!hr_chart) return;

    trend_db_query_alarms(start_ts, end_ts, &alarm_buf);
    if (alarm_buf.count == 0) return;

    lv_obj_update_layout(hr_chart);
    int32_t chart_w = lv_obj_get_content_width(hr_chart);
    int32_t chart_h = lv_obj_get_content_height(hr_chart);
    uint32_t range = end_ts - start_ts;
    if (range == 0 || chart_w <= 0 || chart_h <= 0) return;

    int limit = alarm_buf.count < MAX_ALARM_MARKERS
              ? alarm_buf.count : MAX_ALARM_MARKERS;

    for (int i = 0; i < limit; i++) {
        if (alarm_buf.timestamp_s[i] < start_ts) continue;
        int x = (int)((uint64_t)(alarm_buf.timestamp_s[i] - start_ts)
                       * chart_w / range);
        if (x < 0 || x >= chart_w) continue;

        lv_obj_t *m = lv_obj_create(hr_chart);
        lv_obj_remove_flag(m, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_size(m, 2, chart_h);
        lv_obj_set_pos(m, x, 0);
        lv_obj_set_style_border_width(m, 0, 0);
        lv_obj_set_style_radius(m, 0, 0);
        lv_obj_set_style_bg_opa(m, LV_OPA_50, 0);

        lv_color_t color = (alarm_buf.severity[i] >= VM_ALARM_HIGH)
                            ? VM_COLOR_ALARM_HIGH : VM_COLOR_ALARM_MEDIUM;
        lv_obj_set_style_bg_color(m, color, 0);

        alarm_markers[alarm_marker_count++] = m;
    }
}

/* ── Refresh orchestration ────────────────────────────────── */

static void refresh_all_charts(void) {
    uint32_t now = get_current_ts();
    uint32_t range_s = (uint32_t)range_values[active_range_idx];
    uint32_t start_ts = (now > range_s) ? now - range_s : 0;

    populate_chart_from_db(hr_chart,   hr_series,   TREND_PARAM_HR,   start_ts, now);
    populate_chart_from_db(spo2_chart, spo2_series, TREND_PARAM_SPO2, start_ts, now);
    populate_chart_from_db(rr_chart,   rr_series,   TREND_PARAM_RR,   start_ts, now);
    populate_nibp_from_db(start_ts, now);
    populate_temp_from_db(start_ts, now);
    update_alarm_markers(start_ts, now);
}

static void refresh_timer_cb(lv_timer_t *timer) {
    (void)timer;
    if (!hr_chart) return;
    refresh_all_charts();
}
