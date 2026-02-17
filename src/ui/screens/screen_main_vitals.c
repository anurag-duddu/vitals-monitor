/**
 * @file screen_main_vitals.c
 * @brief Main vitals monitoring screen implementation
 *
 * Layout (800x480):
 *   ┌──────────────────────────────────────────────────────────────┐
 *   │ ALARM BAR (32px)                                            │
 *   ├────────────────────────────────────┬─────────────────────────┤
 *   │ WAVEFORM AREA (~60%)              │ VITALS PANEL (~40%)     │
 *   │  ECG  Lead II  25mm/s             │   HR    72  bpm         │
 *   │  ═══════════════════              │   SpO2  98  %           │
 *   │  Pleth                            │   NIBP  120/80 mmHg     │
 *   │  ───────────────────              │   Temp 36.8 | RR 16     │
 *   ├────────────────────────────────────┴─────────────────────────┤
 *   │ NAV BAR (48px)                                              │
 *   └──────────────────────────────────────────────────────────────┘
 */

#include "screen_main_vitals.h"
#include "screen_manager.h"
#include "widget_numeric_display.h"
#include "widget_alarm_banner.h"
#include "widget_nav_bar.h"
#include "widget_waveform.h"
#include "theme_styles.h"
#include "phosphor_icons.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ── Module state (valid while screen is active) ───────────── */

static widget_numeric_display_t *hr_display;
static widget_numeric_display_t *spo2_display;
static widget_numeric_display_t *nibp_display;
static widget_numeric_display_t *temp_display;
static widget_numeric_display_t *rr_display;
static widget_alarm_banner_t    *alarm_banner;
static widget_nav_bar_t         *nav_bar;
static widget_waveform_t        *ecg_waveform;
static widget_waveform_t        *pleth_waveform;

/* ── PERF-1.4: Cached previous values to avoid redundant display updates ── */
#define CACHE_INVALID_INT (-999999)
#define CACHE_INVALID_FLT (-999999.0f)

static int   prev_hr   = CACHE_INVALID_INT;
static int   prev_spo2 = CACHE_INVALID_INT;
static int   prev_rr   = CACHE_INVALID_INT;
static float prev_temp = CACHE_INVALID_FLT;
static int   prev_nibp_sys = CACHE_INVALID_INT;
static int   prev_nibp_dia = CACHE_INVALID_INT;
static int   prev_nibp_map = CACHE_INVALID_INT;

/* ── Forward declarations ──────────────────────────────────── */
void screen_main_vitals_reset_cache(void);

/* ── Public API ────────────────────────────────────────────── */

lv_obj_t * screen_main_vitals_create(void) {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_add_style(scr, &s_screen, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* ── 1. Alarm banner (top, 32px) ──────────────────────── */
    alarm_banner = widget_alarm_banner_create(scr);
    if (!alarm_banner) {
        fprintf(stderr, "[main_vitals] CRITICAL: Alarm banner pool exhausted!\n");
    }

    /* ── 2. Main content area (between alarm bar and nav bar) ─ */
    lv_obj_t *content = lv_obj_create(scr);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(content, 0, VM_ALARM_BAR_HEIGHT);
    lv_obj_set_size(content, VM_SCREEN_WIDTH, VM_CONTENT_HEIGHT);
    lv_obj_add_style(content, &s_row, 0);
    lv_obj_set_style_pad_gap(content, 0, 0);

    /* ── 2a. Waveform area (left, ~60%) ─────────────────────── */
    lv_obj_t *wave_area = lv_obj_create(content);
    lv_obj_remove_flag(wave_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(wave_area, lv_pct(VM_WAVEFORM_WIDTH_PCT), lv_pct(100));
    lv_obj_add_style(wave_area, &s_col, 0);
    lv_obj_set_style_bg_color(wave_area, lv_color_hex(vm_active_scheme->background), 0);
    lv_obj_set_style_bg_opa(wave_area, VM_OPA_COVER, 0);
    lv_obj_set_style_border_color(wave_area, lv_color_hex(vm_active_scheme->outline), 0);
    lv_obj_set_style_border_width(wave_area, VM_BORDER_THIN, 0);
    lv_obj_set_style_border_side(wave_area, LV_BORDER_SIDE_RIGHT, 0);
    lv_obj_set_style_pad_all(wave_area, VM_PAD_SMALL, 0);
    lv_obj_set_style_pad_gap(wave_area, VM_PAD_SMALL, 0);

    /* ECG waveform (top, ~55% of waveform area) */
    lv_obj_t *ecg_container = lv_obj_create(wave_area);
    lv_obj_remove_flag(ecg_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(ecg_container, lv_pct(100));
    lv_obj_set_flex_grow(ecg_container, VM_WAVEFORM_ECG_HEIGHT_PCT);
    lv_obj_add_style(ecg_container, &s_base, 0);

    ecg_waveform = widget_waveform_create(
        ecg_container, "ECG  Lead II  25mm/s", VM_COLOR_HR,
        VM_WAVEFORM_POINT_COUNT, 0, 400);
    if (!ecg_waveform) {
        fprintf(stderr, "[main_vitals] CRITICAL: ECG waveform pool exhausted!\n");
    }

    /* Pleth waveform (bottom, ~45% of waveform area) */
    lv_obj_t *pleth_container = lv_obj_create(wave_area);
    lv_obj_remove_flag(pleth_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(pleth_container, lv_pct(100));
    lv_obj_set_flex_grow(pleth_container, VM_WAVEFORM_PLETH_HEIGHT_PCT);
    lv_obj_add_style(pleth_container, &s_base, 0);

    pleth_waveform = widget_waveform_create(
        pleth_container, "Pleth", VM_COLOR_SPO2,
        VM_WAVEFORM_POINT_COUNT, 0, 400);
    if (!pleth_waveform) {
        fprintf(stderr, "[main_vitals] CRITICAL: Pleth waveform pool exhausted!\n");
    }

    /* ── 2b. Vitals panel (right, ~40%) — column of numerics ── */
    lv_obj_t *vitals_panel = lv_obj_create(content);
    lv_obj_remove_flag(vitals_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(vitals_panel, lv_pct(VM_VITALS_WIDTH_PCT), lv_pct(100));
    lv_obj_add_style(vitals_panel, &s_col, 0);
    lv_obj_set_style_pad_all(vitals_panel, VM_PAD_SMALL, 0);
    lv_obj_set_style_pad_gap(vitals_panel, VM_PAD_SMALL, 0);

    /* HR — large */
    hr_display = widget_numeric_display_create(
        vitals_panel, "HR", "bpm", VM_COLOR_HR, NUMERIC_SIZE_LARGE,
        &icon_heartbeat_dsc);

    /* SpO2 — large */
    spo2_display = widget_numeric_display_create(
        vitals_panel, "SpO2", "%", VM_COLOR_SPO2, NUMERIC_SIZE_LARGE,
        &icon_drop_dsc);

    /* NIBP — medium */
    nibp_display = widget_numeric_display_create(
        vitals_panel, "NIBP", "mmHg", VM_COLOR_NIBP, NUMERIC_SIZE_MEDIUM,
        &icon_gauge_dsc);

    /* Bottom row: Temp + RR side by side */
    lv_obj_t *bottom_row = lv_obj_create(vitals_panel);
    lv_obj_remove_flag(bottom_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(bottom_row, lv_pct(100));
    lv_obj_set_flex_grow(bottom_row, 2);
    lv_obj_add_style(bottom_row, &s_row, 0);
    lv_obj_set_style_pad_gap(bottom_row, VM_PAD_SMALL, 0);

    /* Temp — small */
    temp_display = widget_numeric_display_create(
        bottom_row, "Temp", "\xc2\xb0""C", VM_COLOR_TEMP, NUMERIC_SIZE_SMALL,
        &icon_thermometer_dsc);

    /* RR — small */
    rr_display = widget_numeric_display_create(
        bottom_row, "RR", "/min", VM_COLOR_RR, NUMERIC_SIZE_SMALL,
        &icon_wind_dsc);

    /* ── 3. Navigation bar (bottom, 48px) ─────────────────── */
    nav_bar = widget_nav_bar_create(scr);
    widget_nav_bar_set_active(nav_bar, SCREEN_ID_MAIN_VITALS);
    widget_nav_bar_set_patient(nav_bar, "No Patient");

    printf("[main_vitals] Screen created\n");
    return scr;
}

void screen_main_vitals_destroy(void) {
    /* Release widget pool slots — LVGL auto-deletes the actual objects */
    widget_numeric_display_free(hr_display);
    widget_numeric_display_free(spo2_display);
    widget_numeric_display_free(nibp_display);
    widget_numeric_display_free(temp_display);
    widget_numeric_display_free(rr_display);
    widget_alarm_banner_free(alarm_banner);
    widget_nav_bar_free(nav_bar);
    widget_waveform_free(ecg_waveform);
    widget_waveform_free(pleth_waveform);

    hr_display = NULL;
    spo2_display = NULL;
    nibp_display = NULL;
    temp_display = NULL;
    rr_display = NULL;
    alarm_banner = NULL;
    nav_bar = NULL;
    ecg_waveform = NULL;
    pleth_waveform = NULL;

    /* PERF-1.4: Reset cached values so next create gets fresh display */
    screen_main_vitals_reset_cache();

    printf("[main_vitals] Screen destroyed\n");
}

/* ── Value update functions ────────────────────────────────── */
/* UI-5.1: Display "---" when value is 0/invalid (not being measured)
 * UI-5.2: Clamp to physiological ranges; display "ERR" if out of range
 * UI-5.3: isnan()/isinf() check for temperature
 * PERF-1.4: Cache previous values, only update display when changed */

void screen_main_vitals_update_hr(int value) {
    if (!hr_display) return;
    if (value == prev_hr) return;           /* PERF-1.4: skip if unchanged */
    prev_hr = value;

    char buf[16];
    if (value == 0) {                       /* UI-5.1: not measured */
        snprintf(buf, sizeof(buf), "---");
    } else if (value < 20 || value > 300) { /* UI-5.2: out of range */
        snprintf(buf, sizeof(buf), "ERR");
    } else {
        snprintf(buf, sizeof(buf), "%d", value);
    }
    widget_numeric_display_set_value(hr_display, buf);
}

void screen_main_vitals_update_spo2(int value) {
    if (!spo2_display) return;
    if (value == prev_spo2) return;         /* PERF-1.4 */
    prev_spo2 = value;

    char buf[16];
    if (value == 0) {                       /* UI-5.1 */
        snprintf(buf, sizeof(buf), "---");
    } else if (value < 1 || value > 100) {  /* UI-5.2 */
        snprintf(buf, sizeof(buf), "ERR");
    } else {
        snprintf(buf, sizeof(buf), "%d", value);
    }
    widget_numeric_display_set_value(spo2_display, buf);
}

void screen_main_vitals_update_nibp(int sys, int dia, int map) {
    if (!nibp_display) return;
    if (sys == prev_nibp_sys && dia == prev_nibp_dia && map == prev_nibp_map) return; /* PERF-1.4 */
    prev_nibp_sys = sys;
    prev_nibp_dia = dia;
    prev_nibp_map = map;

    char buf[32];
    snprintf(buf, sizeof(buf), "%d/%d (%d)", sys, dia, map);
    widget_numeric_display_set_value(nibp_display, buf);
}

void screen_main_vitals_update_temp(float value) {
    if (!temp_display) return;

    /* UI-5.3: guard against NaN/Inf before any comparison */
    if (isnan(value) || isinf(value)) {
        prev_temp = CACHE_INVALID_FLT;
        widget_numeric_display_set_value(temp_display, "---");
        return;
    }

    if (value == prev_temp) return;         /* PERF-1.4 */
    prev_temp = value;

    char buf[16];
    if (value <= 0.0f) {                    /* UI-5.1: not measured */
        snprintf(buf, sizeof(buf), "---");
    } else if (value < 20.0f || value > 50.0f) { /* UI-5.2: out of range */
        snprintf(buf, sizeof(buf), "ERR");
    } else {
        snprintf(buf, sizeof(buf), "%.1f", (double)value);
    }
    widget_numeric_display_set_value(temp_display, buf);
}

void screen_main_vitals_update_rr(int value) {
    if (!rr_display) return;
    if (value == prev_rr) return;           /* PERF-1.4 */
    prev_rr = value;

    char buf[16];
    if (value == 0) {                       /* UI-5.1 */
        snprintf(buf, sizeof(buf), "---");
    } else if (value < 1 || value > 100) {  /* UI-5.2 */
        snprintf(buf, sizeof(buf), "ERR");
    } else {
        snprintf(buf, sizeof(buf), "%d", value);
    }
    widget_numeric_display_set_value(rr_display, buf);
}

void screen_main_vitals_set_alarm(vm_alarm_severity_t severity, const char *message) {
    if (!alarm_banner) return;

    if (severity == VM_ALARM_NONE) {
        widget_alarm_banner_clear(alarm_banner);
    } else {
        widget_alarm_banner_set(alarm_banner, severity, message);
    }
}

void screen_main_vitals_set_alarm_count(int count) {
    if (!alarm_banner) return;
    widget_alarm_banner_set_count(alarm_banner, count, 1);
}

void screen_main_vitals_set_ack_callback(void (*cb)(void)) {
    if (!alarm_banner) return;
    widget_alarm_banner_set_ack_cb(alarm_banner, cb);
}

void screen_main_vitals_update_time(const char *time_str) {
    if (!alarm_banner) return;
    widget_alarm_banner_set_time(alarm_banner, time_str);
}

/* ── Cache reset (PERF-1.4) ────────────────────────────────── */

void screen_main_vitals_reset_cache(void) {
    prev_hr       = CACHE_INVALID_INT;
    prev_spo2     = CACHE_INVALID_INT;
    prev_rr       = CACHE_INVALID_INT;
    prev_temp     = CACHE_INVALID_FLT;
    prev_nibp_sys = CACHE_INVALID_INT;
    prev_nibp_dia = CACHE_INVALID_INT;
    prev_nibp_map = CACHE_INVALID_INT;
}

/* ── Waveform functions ────────────────────────────────────── */

void screen_main_vitals_push_ecg_sample(int32_t value) {
    if (!ecg_waveform) return;
    widget_waveform_push_sample(ecg_waveform, value);
}

void screen_main_vitals_push_pleth_sample(int32_t value) {
    if (!pleth_waveform) return;
    widget_waveform_push_sample(pleth_waveform, value);
}

void screen_main_vitals_refresh_waveforms(void) {
    if (ecg_waveform) widget_waveform_refresh(ecg_waveform);
    if (pleth_waveform) widget_waveform_refresh(pleth_waveform);
}
