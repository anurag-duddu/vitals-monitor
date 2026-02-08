/**
 * @file screen_alarms.c
 * @brief Alarms screen — alarm history log and current limit settings
 *
 * Layout (800x480):
 *   ┌──────────────────────────────────────────────────────────────┐
 *   │ ALARM BAR (32px)                                            │
 *   ├────────────────────────────────┬─────────────────────────────┤
 *   │  Alarm History (scrollable)   │  Alarm Limits (table)       │
 *   │  ┌──────────────────────────┐ │  ┌────────────────────────┐ │
 *   │  │ 14:32  HR Very High  RED │ │  │ Param  High   Low     │ │
 *   │  │ 14:30  SpO2 Low    YEL  │ │  │ HR     >150   <40     │ │
 *   │  │ ...                      │ │  │ SpO2          <85     │ │
 *   │  └──────────────────────────┘ │  │ ...                   │ │
 *   │                                │  └────────────────────────┘ │
 *   ├────────────────────────────────┴─────────────────────────────┤
 *   │ NAV BAR (48px)                                              │
 *   └──────────────────────────────────────────────────────────────┘
 */

#include "screen_alarms.h"
#include "screen_manager.h"
#include "widget_alarm_banner.h"
#include "widget_nav_bar.h"
#include "theme_vitals.h"
#include "mock_data.h"
#include <stdio.h>

/* ── Module state ──────────────────────────────────────────── */

static widget_alarm_banner_t *alarm_banner;
static widget_nav_bar_t      *nav_bar;
static lv_obj_t              *log_list;
static lv_timer_t            *refresh_timer;
static int                    last_log_count;

/* ── Forward declarations ──────────────────────────────────── */

static void build_alarm_log_list(lv_obj_t *parent);
static void build_alarm_limits_table(lv_obj_t *parent);
static void refresh_timer_cb(lv_timer_t *timer);
static const char * severity_str(vm_alarm_severity_t s);
static lv_color_t severity_color(vm_alarm_severity_t s);

/* ── Public API ────────────────────────────────────────────── */

lv_obj_t * screen_alarms_create(void) {
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
    lv_obj_set_style_pad_gap(content, VM_PAD_NORMAL, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW);

    /* Left panel: Alarm history log (60%) */
    lv_obj_t *left_panel = lv_obj_create(content);
    lv_obj_remove_flag(left_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(left_panel, lv_pct(58), lv_pct(100));
    lv_obj_set_style_bg_color(left_panel, VM_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(left_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(left_panel, VM_COLOR_BG_PANEL_BORDER, 0);
    lv_obj_set_style_border_width(left_panel, 1, 0);
    lv_obj_set_style_radius(left_panel, 4, 0);
    lv_obj_set_style_pad_all(left_panel, VM_PAD_NORMAL, 0);
    lv_obj_set_style_pad_gap(left_panel, VM_PAD_SMALL, 0);
    lv_obj_set_flex_flow(left_panel, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *log_title = lv_label_create(left_panel);
    lv_label_set_text(log_title, "Alarm History");
    lv_obj_set_style_text_font(log_title, VM_FONT_BODY, 0);
    lv_obj_set_style_text_color(log_title, VM_COLOR_TEXT_PRIMARY, 0);

    build_alarm_log_list(left_panel);

    /* Right panel: Alarm limits (40%) */
    lv_obj_t *right_panel = lv_obj_create(content);
    lv_obj_remove_flag(right_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_grow(right_panel, 1);
    lv_obj_set_height(right_panel, lv_pct(100));
    lv_obj_set_style_bg_color(right_panel, VM_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(right_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(right_panel, VM_COLOR_BG_PANEL_BORDER, 0);
    lv_obj_set_style_border_width(right_panel, 1, 0);
    lv_obj_set_style_radius(right_panel, 4, 0);
    lv_obj_set_style_pad_all(right_panel, VM_PAD_NORMAL, 0);
    lv_obj_set_style_pad_gap(right_panel, VM_PAD_SMALL, 0);
    lv_obj_set_flex_flow(right_panel, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *limits_title = lv_label_create(right_panel);
    lv_label_set_text(limits_title, "Alarm Limits");
    lv_obj_set_style_text_font(limits_title, VM_FONT_BODY, 0);
    lv_obj_set_style_text_color(limits_title, VM_COLOR_TEXT_PRIMARY, 0);

    build_alarm_limits_table(right_panel);

    /* Refresh timer to check for new alarms */
    const alarm_log_t *alog = mock_data_get_alarm_log();
    last_log_count = alog->count;
    refresh_timer = lv_timer_create(refresh_timer_cb, 2000, NULL);

    /* ── Nav bar (bottom) ────────────────────────────────── */
    nav_bar = widget_nav_bar_create(scr);
    widget_nav_bar_set_active(nav_bar, SCREEN_ID_ALARMS);
    widget_nav_bar_set_patient(nav_bar, "No Patient");

    printf("[alarms] Screen created\n");
    return scr;
}

void screen_alarms_destroy(void) {
    if (refresh_timer) {
        lv_timer_delete(refresh_timer);
        refresh_timer = NULL;
    }
    widget_alarm_banner_free(alarm_banner);
    widget_nav_bar_free(nav_bar);
    alarm_banner = NULL;
    nav_bar = NULL;
    log_list = NULL;
    printf("[alarms] Screen destroyed\n");
}

/* ── Private helpers ───────────────────────────────────────── */

static const char * severity_str(vm_alarm_severity_t s) {
    switch (s) {
        case VM_ALARM_HIGH:   return "HIGH";
        case VM_ALARM_MEDIUM: return "MED";
        case VM_ALARM_LOW:    return "LOW";
        default:              return "---";
    }
}

static lv_color_t severity_color(vm_alarm_severity_t s) {
    return theme_vitals_alarm_color(s);
}

static void add_log_entry_to_list(const alarm_log_entry_t *entry) {
    if (!log_list) return;

    lv_obj_t *row = lv_obj_create(log_list);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 28);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(row, VM_PAD_NORMAL, 0);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Time */
    lv_obj_t *time_lbl = lv_label_create(row);
    lv_label_set_text(time_lbl, entry->time_str);
    lv_obj_set_style_text_font(time_lbl, VM_FONT_SMALL, 0);
    lv_obj_set_style_text_color(time_lbl, VM_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_width(time_lbl, 44);

    /* Severity badge */
    lv_obj_t *sev_lbl = lv_label_create(row);
    lv_label_set_text(sev_lbl, severity_str(entry->severity));
    lv_obj_set_style_text_font(sev_lbl, VM_FONT_SMALL, 0);
    lv_obj_set_style_text_color(sev_lbl, severity_color(entry->severity), 0);
    lv_obj_set_width(sev_lbl, 40);

    /* Message */
    lv_obj_t *msg_lbl = lv_label_create(row);
    lv_label_set_text(msg_lbl, entry->message);
    lv_obj_set_style_text_font(msg_lbl, VM_FONT_SMALL, 0);
    lv_obj_set_style_text_color(msg_lbl, VM_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_flex_grow(msg_lbl, 1);
}

static void build_alarm_log_list(lv_obj_t *parent) {
    /* Scrollable list container */
    log_list = lv_obj_create(parent);
    lv_obj_set_width(log_list, lv_pct(100));
    lv_obj_set_flex_grow(log_list, 1);
    lv_obj_set_style_bg_opa(log_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(log_list, 0, 0);
    lv_obj_set_style_pad_all(log_list, 0, 0);
    lv_obj_set_style_pad_gap(log_list, 2, 0);
    lv_obj_set_flex_flow(log_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_flag(log_list, LV_OBJ_FLAG_SCROLLABLE);

    const alarm_log_t *alog = mock_data_get_alarm_log();

    if (alog->count == 0) {
        lv_obj_t *empty = lv_label_create(log_list);
        lv_label_set_text(empty, "No alarms recorded");
        lv_obj_set_style_text_font(empty, VM_FONT_CAPTION, 0);
        lv_obj_set_style_text_color(empty, VM_COLOR_TEXT_DISABLED, 0);
        return;
    }

    /* Show entries newest-first */
    int total = alog->count < ALARM_LOG_MAX ? alog->count : ALARM_LOG_MAX;
    for (int i = total - 1; i >= 0; i--) {
        int idx = (alog->write_idx - 1 - (total - 1 - i) + ALARM_LOG_MAX) % ALARM_LOG_MAX;
        add_log_entry_to_list(&alog->entries[idx]);
    }
}

static void build_alarm_limits_table(lv_obj_t *parent) {
    lv_obj_t *table = lv_table_create(parent);
    lv_obj_set_width(table, lv_pct(100));
    lv_obj_set_flex_grow(table, 1);
    lv_obj_set_style_bg_color(table, VM_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(table, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(table, 0, 0);
    lv_obj_set_style_pad_all(table, 0, 0);

    /* Cell styling */
    lv_obj_set_style_border_color(table, VM_COLOR_BG_PANEL_BORDER, LV_PART_ITEMS);
    lv_obj_set_style_border_width(table, 1, LV_PART_ITEMS);
    lv_obj_set_style_text_font(table, VM_FONT_SMALL, LV_PART_ITEMS);
    lv_obj_set_style_text_color(table, VM_COLOR_TEXT_PRIMARY, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(table, VM_COLOR_BG, LV_PART_ITEMS);
    lv_obj_set_style_pad_top(table, 4, LV_PART_ITEMS);
    lv_obj_set_style_pad_bottom(table, 4, LV_PART_ITEMS);
    lv_obj_set_style_pad_left(table, 6, LV_PART_ITEMS);
    lv_obj_set_style_pad_right(table, 6, LV_PART_ITEMS);

    lv_table_set_column_count(table, 3);
    lv_table_set_column_width(table, 0, 65);
    lv_table_set_column_width(table, 1, 75);
    lv_table_set_column_width(table, 2, 75);

    /* Header row */
    lv_table_set_cell_value(table, 0, 0, "Param");
    lv_table_set_cell_value(table, 0, 1, "High");
    lv_table_set_cell_value(table, 0, 2, "Low");

    /* HR limits */
    lv_table_set_cell_value(table, 1, 0, "HR");
    lv_table_set_cell_value(table, 1, 1, "> 150");
    lv_table_set_cell_value(table, 1, 2, "< 40");

    /* SpO2 limits */
    lv_table_set_cell_value(table, 2, 0, "SpO2");
    lv_table_set_cell_value(table, 2, 1, "---");
    lv_table_set_cell_value(table, 2, 2, "< 85");

    /* RR limits */
    lv_table_set_cell_value(table, 3, 0, "RR");
    lv_table_set_cell_value(table, 3, 1, "> 30");
    lv_table_set_cell_value(table, 3, 2, "< 8");

    /* Temp limits */
    lv_table_set_cell_value(table, 4, 0, "Temp");
    lv_table_set_cell_value(table, 4, 1, "> 39.0");
    lv_table_set_cell_value(table, 4, 2, "< 35.0");

    /* Medium alarm thresholds subtitle */
    lv_obj_t *med_title = lv_label_create(parent);
    lv_label_set_text(med_title, "Warning Limits");
    lv_obj_set_style_text_font(med_title, VM_FONT_CAPTION, 0);
    lv_obj_set_style_text_color(med_title, VM_COLOR_ALARM_MEDIUM, 0);
    lv_obj_set_style_pad_top(med_title, VM_PAD_SMALL, 0);

    lv_obj_t *table2 = lv_table_create(parent);
    lv_obj_set_width(table2, lv_pct(100));
    lv_obj_set_flex_grow(table2, 1);
    lv_obj_set_style_bg_color(table2, VM_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(table2, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(table2, 0, 0);
    lv_obj_set_style_pad_all(table2, 0, 0);

    lv_obj_set_style_border_color(table2, VM_COLOR_BG_PANEL_BORDER, LV_PART_ITEMS);
    lv_obj_set_style_border_width(table2, 1, LV_PART_ITEMS);
    lv_obj_set_style_text_font(table2, VM_FONT_SMALL, LV_PART_ITEMS);
    lv_obj_set_style_text_color(table2, VM_COLOR_TEXT_SECONDARY, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(table2, VM_COLOR_BG, LV_PART_ITEMS);
    lv_obj_set_style_pad_top(table2, 4, LV_PART_ITEMS);
    lv_obj_set_style_pad_bottom(table2, 4, LV_PART_ITEMS);
    lv_obj_set_style_pad_left(table2, 6, LV_PART_ITEMS);
    lv_obj_set_style_pad_right(table2, 6, LV_PART_ITEMS);

    lv_table_set_column_count(table2, 3);
    lv_table_set_column_width(table2, 0, 65);
    lv_table_set_column_width(table2, 1, 75);
    lv_table_set_column_width(table2, 2, 75);

    lv_table_set_cell_value(table2, 0, 0, "HR");
    lv_table_set_cell_value(table2, 0, 1, "> 120");
    lv_table_set_cell_value(table2, 0, 2, "< 50");

    lv_table_set_cell_value(table2, 1, 0, "SpO2");
    lv_table_set_cell_value(table2, 1, 1, "---");
    lv_table_set_cell_value(table2, 1, 2, "< 90");

    lv_table_set_cell_value(table2, 2, 0, "RR");
    lv_table_set_cell_value(table2, 2, 1, "> 24");
    lv_table_set_cell_value(table2, 2, 2, "< 10");

    lv_table_set_cell_value(table2, 3, 0, "Temp");
    lv_table_set_cell_value(table2, 3, 1, "> 38.0");
    lv_table_set_cell_value(table2, 3, 2, "< 36.0");
}

static void refresh_timer_cb(lv_timer_t *timer) {
    (void)timer;
    const alarm_log_t *alog = mock_data_get_alarm_log();
    if (alog->count > last_log_count && log_list) {
        /* New alarm entries — add them to the list */
        int new_count = alog->count - last_log_count;
        if (new_count > ALARM_LOG_MAX) new_count = ALARM_LOG_MAX;
        for (int i = 0; i < new_count; i++) {
            int idx = (alog->write_idx - new_count + i + ALARM_LOG_MAX) % ALARM_LOG_MAX;
            add_log_entry_to_list(&alog->entries[idx]);
        }
        last_log_count = alog->count;
    }
}
