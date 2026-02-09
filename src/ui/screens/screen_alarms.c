/**
 * @file screen_alarms.c
 * @brief Alarms screen -- alarm history log, live engine state, and editable limits
 *
 * Layout (800x480):
 *   +--------------------------------------------------------------+
 *   | ALARM BAR (32px)                                             |
 *   +------------------------------+-------------------------------+
 *   |  Alarm History (scrollable) |  Alarm Limits (table)          |
 *   |  [Status: HR HIGH ACTIVE]   |  +-------------------------+  |
 *   |  [Ack All] [Silence 2min]   |  | Param   High    Low     |  |
 *   |  +------------------------+ |  | HR      >150    <40     |  |
 *   |  | 14:32  HR Very High RED| |  | SpO2    ---     <85     |  |
 *   |  | 14:30  SpO2 Low    YEL | |  | ...                     |  |
 *   |  | ...                    | |  +-------------------------+  |
 *   |  +------------------------+ |  Warning Limits               |
 *   |                              |  +-------------------------+  |
 *   |                              |  | HR      >120    <50     |  |
 *   |                              |  | ...                     |  |
 *   |                              |  +-------------------------+  |
 *   +------------------------------+-------------------------------+
 *   | NAV BAR (48px)                                               |
 *   +--------------------------------------------------------------+
 */

#include "screen_alarms.h"
#include "screen_manager.h"
#include "widget_alarm_banner.h"
#include "widget_nav_bar.h"
#include "theme_vitals.h"
#include "vitals_provider.h"
#include "alarm_engine.h"
#include "patient_data.h"
#include <stdio.h>

/* -- Module state -------------------------------------------------- */

static widget_alarm_banner_t *alarm_banner;
static widget_nav_bar_t      *nav_bar;
static lv_obj_t              *log_list;
static lv_obj_t              *status_label;
static lv_timer_t            *refresh_timer;
static int                    last_log_count;

/* -- Forward declarations ------------------------------------------ */

static void build_alarm_log_list(lv_obj_t *parent);
static void build_alarm_limits_table(lv_obj_t *parent);
static void refresh_timer_cb(lv_timer_t *timer);
static const char *severity_str(vm_alarm_severity_t s);
static lv_color_t severity_color(vm_alarm_severity_t s);
static void ack_all_cb(lv_event_t *e);
static void silence_all_cb(lv_event_t *e);
static void update_status_label(void);

/* -- Helpers for alarm state strings ------------------------------- */

static const char *alarm_state_str(alarm_state_t s) {
    switch (s) {
        case ALARM_STATE_ACTIVE:       return "ACTIVE";
        case ALARM_STATE_ACKNOWLEDGED: return "ACK";
        case ALARM_STATE_SILENCED:     return "SILENCED";
        default:                       return "OK";
    }
}

static const char *alarm_param_name(alarm_param_t p) {
    switch (p) {
        case ALARM_PARAM_HR:       return "HR";
        case ALARM_PARAM_SPO2:     return "SpO2";
        case ALARM_PARAM_RR:       return "RR";
        case ALARM_PARAM_TEMP:     return "Temp";
        case ALARM_PARAM_NIBP_SYS: return "SYS";
        case ALARM_PARAM_NIBP_DIA: return "DIA";
        default:                   return "?";
    }
}

/* -- Helper: format a limit value into a buffer -------------------- */

static void format_limit(char *buf, size_t len, alarm_param_t param,
                         int value, const char *prefix) {
    if (value == 0) {
        snprintf(buf, len, "---");
    } else if (param == ALARM_PARAM_TEMP) {
        snprintf(buf, len, "%s %.1f", prefix, value / 10.0);
    } else {
        snprintf(buf, len, "%s %d", prefix, value);
    }
}

/* -- Public API ---------------------------------------------------- */

lv_obj_t *screen_alarms_create(void) {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, VM_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* -- Alarm banner (top) ---------------------------------------- */
    alarm_banner = widget_alarm_banner_create(scr);

    /* -- Content area ---------------------------------------------- */
    lv_obj_t *content = lv_obj_create(scr);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(content, 0, VM_ALARM_BAR_HEIGHT);
    lv_obj_set_size(content, VM_SCREEN_WIDTH, VM_CONTENT_HEIGHT);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, VM_PAD_NORMAL, 0);
    lv_obj_set_style_pad_gap(content, VM_PAD_NORMAL, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW);

    /* Left panel: Alarm history log (58%) */
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

    /* Title */
    lv_obj_t *log_title = lv_label_create(left_panel);
    lv_label_set_text(log_title, "Alarm History");
    lv_obj_set_style_text_font(log_title, VM_FONT_BODY, 0);
    lv_obj_set_style_text_color(log_title, VM_COLOR_TEXT_PRIMARY, 0);

    /* Status row: shows highest active alarm from alarm_engine */
    status_label = lv_label_create(left_panel);
    lv_obj_set_style_text_font(status_label, VM_FONT_CAPTION, 0);
    update_status_label();

    /* Button row: Ack All + Silence 2min */
    lv_obj_t *btn_row = lv_obj_create(left_panel);
    lv_obj_remove_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(btn_row, lv_pct(100));
    lv_obj_set_height(btn_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(btn_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_row, 0, 0);
    lv_obj_set_style_pad_all(btn_row, 0, 0);
    lv_obj_set_style_pad_gap(btn_row, VM_PAD_NORMAL, 0);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);

    /* Ack All button */
    lv_obj_t *btn_ack = lv_button_create(btn_row);
    lv_obj_set_size(btn_ack, 100, 32);
    lv_obj_set_style_bg_color(btn_ack, VM_COLOR_ALARM_NONE, 0);
    lv_obj_add_event_cb(btn_ack, ack_all_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_ack = lv_label_create(btn_ack);
    lv_label_set_text(lbl_ack, "Ack All");
    lv_obj_center(lbl_ack);
    lv_obj_set_style_text_font(lbl_ack, VM_FONT_CAPTION, 0);
    lv_obj_set_style_text_color(lbl_ack, lv_color_black(), 0);

    /* Silence 2min button */
    lv_obj_t *btn_silence = lv_button_create(btn_row);
    lv_obj_set_size(btn_silence, 100, 32);
    lv_obj_set_style_bg_color(btn_silence, VM_COLOR_ALARM_MEDIUM, 0);
    lv_obj_add_event_cb(btn_silence, silence_all_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_silence = lv_label_create(btn_silence);
    lv_label_set_text(lbl_silence, "Silence 2min");
    lv_obj_center(lbl_silence);
    lv_obj_set_style_text_font(lbl_silence, VM_FONT_CAPTION, 0);
    lv_obj_set_style_text_color(lbl_silence, lv_color_black(), 0);

    /* Alarm log list */
    build_alarm_log_list(left_panel);

    /* Right panel: Alarm limits (flex-grow fills remaining 42%) */
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

    /* Refresh timer to check for new alarms and update status */
    const vitals_alarm_log_t *alog = vitals_provider_get_alarm_log();
    last_log_count = alog->count;
    refresh_timer = lv_timer_create(refresh_timer_cb, 2000, NULL);

    /* -- Nav bar (bottom) ------------------------------------------ */
    nav_bar = widget_nav_bar_create(scr);
    widget_nav_bar_set_active(nav_bar, SCREEN_ID_ALARMS);

    /* Show active patient name */
    const patient_t *pt = patient_data_get_active(0);
    widget_nav_bar_set_patient(nav_bar, pt ? pt->name : "No Patient");

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
    nav_bar      = NULL;
    log_list     = NULL;
    status_label = NULL;
    printf("[alarms] Screen destroyed\n");
}

/* -- Private helpers ----------------------------------------------- */

static const char *severity_str(vm_alarm_severity_t s) {
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

/* -- Button callbacks ---------------------------------------------- */

static void ack_all_cb(lv_event_t *e) {
    (void)e;
    bool acked = alarm_engine_acknowledge_all();
    printf("[alarms] Ack All pressed, result=%d\n", acked);
    update_status_label();
}

static void silence_all_cb(lv_event_t *e) {
    (void)e;
    bool silenced = alarm_engine_silence_all(120);
    printf("[alarms] Silence 2min pressed, result=%d\n", silenced);
    update_status_label();
}

/* -- Status label update ------------------------------------------- */

static void update_status_label(void) {
    if (!status_label) return;

    const alarm_engine_state_t *state = alarm_engine_get_state();

    if (state->highest_active == ALARM_SEV_NONE &&
        state->highest_any == ALARM_SEV_NONE) {
        lv_label_set_text(status_label, "Status: All clear");
        lv_obj_set_style_text_color(status_label, VM_COLOR_ALARM_NONE, 0);
        return;
    }

    /* Find the parameter driving the highest alarm */
    static char buf[128];
    buf[0] = '\0';
    int off = 0;

    for (int i = 0; i < ALARM_PARAM_COUNT; i++) {
        if (state->params[i].state != ALARM_STATE_INACTIVE) {
            const char *sev_name = "---";
            lv_color_t  sev_col  = VM_COLOR_TEXT_SECONDARY;

            switch (state->params[i].severity) {
                case ALARM_SEV_HIGH:
                    sev_name = "HIGH";
                    sev_col  = VM_COLOR_ALARM_HIGH;
                    break;
                case ALARM_SEV_MEDIUM:
                    sev_name = "MED";
                    sev_col  = VM_COLOR_ALARM_MEDIUM;
                    break;
                case ALARM_SEV_LOW:
                    sev_name = "LOW";
                    sev_col  = VM_COLOR_ALARM_LOW;
                    break;
                default:
                    break;
            }

            /* Only show first (highest prio) alarm in the label */
            if (off == 0) {
                off = snprintf(buf, sizeof(buf), "Status: %s %s [%s]",
                               alarm_param_name((alarm_param_t)i),
                               sev_name,
                               alarm_state_str(state->params[i].state));
                lv_obj_set_style_text_color(status_label, sev_col, 0);
            }
        }
    }

    /* Fallback: use highest_message if no per-param match found */
    if (off == 0 && state->highest_message) {
        snprintf(buf, sizeof(buf), "Status: %s", state->highest_message);
        lv_obj_set_style_text_color(status_label, VM_COLOR_ALARM_MEDIUM, 0);
    } else if (off == 0) {
        snprintf(buf, sizeof(buf), "Status: All clear");
        lv_obj_set_style_text_color(status_label, VM_COLOR_ALARM_NONE, 0);
    }

    lv_label_set_text(status_label, buf);
}

/* -- Alarm log list ------------------------------------------------ */

static void add_log_entry_to_list(const vitals_alarm_entry_t *entry) {
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
    lv_label_set_text(sev_lbl, severity_str((vm_alarm_severity_t)entry->severity));
    lv_obj_set_style_text_font(sev_lbl, VM_FONT_SMALL, 0);
    lv_obj_set_style_text_color(sev_lbl, severity_color((vm_alarm_severity_t)entry->severity), 0);
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

    const vitals_alarm_log_t *alog = vitals_provider_get_alarm_log();

    if (alog->count == 0) {
        lv_obj_t *empty = lv_label_create(log_list);
        lv_label_set_text(empty, "No alarms recorded");
        lv_obj_set_style_text_font(empty, VM_FONT_CAPTION, 0);
        lv_obj_set_style_text_color(empty, VM_COLOR_TEXT_DISABLED, 0);
        return;
    }

    /* Show entries newest-first */
    int total = alog->count < VITALS_ALARM_LOG_MAX ? alog->count : VITALS_ALARM_LOG_MAX;
    for (int i = total - 1; i >= 0; i--) {
        int idx = (alog->write_idx - 1 - (total - 1 - i) + VITALS_ALARM_LOG_MAX) % VITALS_ALARM_LOG_MAX;
        add_log_entry_to_list(&alog->entries[idx]);
    }
}

/* -- Alarm limits table (dynamic from alarm_engine) ---------------- */

/** Shared table styling helper */
static void style_limits_table(lv_obj_t *table) {
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
    lv_obj_set_style_bg_color(table, VM_COLOR_BG, LV_PART_ITEMS);
    lv_obj_set_style_pad_top(table, 4, LV_PART_ITEMS);
    lv_obj_set_style_pad_bottom(table, 4, LV_PART_ITEMS);
    lv_obj_set_style_pad_left(table, 6, LV_PART_ITEMS);
    lv_obj_set_style_pad_right(table, 6, LV_PART_ITEMS);
}

/** Populate one limits table (critical or warning) for all params. */
static void fill_limits_table(lv_obj_t *table, bool critical) {
    static const struct {
        alarm_param_t param;
        const char   *name;
    } params[] = {
        { ALARM_PARAM_HR,       "HR"   },
        { ALARM_PARAM_SPO2,     "SpO2" },
        { ALARM_PARAM_RR,       "RR"   },
        { ALARM_PARAM_TEMP,     "Temp" },
        { ALARM_PARAM_NIBP_SYS, "SYS"  },
        { ALARM_PARAM_NIBP_DIA, "DIA"  },
    };
    static const int param_count = sizeof(params) / sizeof(params[0]);

    lv_table_set_column_count(table, 3);
    lv_table_set_column_width(table, 0, 65);
    lv_table_set_column_width(table, 1, 75);
    lv_table_set_column_width(table, 2, 75);

    /* Header row */
    lv_table_set_cell_value(table, 0, 0, "Param");
    lv_table_set_cell_value(table, 0, 1, "High");
    lv_table_set_cell_value(table, 0, 2, "Low");

    for (int i = 0; i < param_count; i++) {
        const alarm_limits_t *lim = alarm_engine_get_limits(params[i].param);
        int row = i + 1;
        char buf_high[24];
        char buf_low[24];

        if (!lim || !lim->enabled) {
            snprintf(buf_high, sizeof(buf_high), "OFF");
            snprintf(buf_low,  sizeof(buf_low),  "OFF");
        } else if (critical) {
            format_limit(buf_high, sizeof(buf_high), params[i].param,
                         lim->critical_high, ">");
            format_limit(buf_low,  sizeof(buf_low),  params[i].param,
                         lim->critical_low, "<");
        } else {
            format_limit(buf_high, sizeof(buf_high), params[i].param,
                         lim->warning_high, ">");
            format_limit(buf_low,  sizeof(buf_low),  params[i].param,
                         lim->warning_low, "<");
        }

        lv_table_set_cell_value(table, row, 0, params[i].name);
        lv_table_set_cell_value(table, row, 1, buf_high);
        lv_table_set_cell_value(table, row, 2, buf_low);
    }
}

static void build_alarm_limits_table(lv_obj_t *parent) {
    /* Critical limits table */
    lv_obj_t *table = lv_table_create(parent);
    style_limits_table(table);
    lv_obj_set_style_text_color(table, VM_COLOR_TEXT_PRIMARY, LV_PART_ITEMS);
    fill_limits_table(table, true);

    /* Warning limits subtitle */
    lv_obj_t *med_title = lv_label_create(parent);
    lv_label_set_text(med_title, "Warning Limits");
    lv_obj_set_style_text_font(med_title, VM_FONT_CAPTION, 0);
    lv_obj_set_style_text_color(med_title, VM_COLOR_ALARM_MEDIUM, 0);
    lv_obj_set_style_pad_top(med_title, VM_PAD_SMALL, 0);

    /* Warning limits table */
    lv_obj_t *table2 = lv_table_create(parent);
    style_limits_table(table2);
    lv_obj_set_style_text_color(table2, VM_COLOR_TEXT_SECONDARY, LV_PART_ITEMS);
    fill_limits_table(table2, false);
}

/* -- Refresh timer ------------------------------------------------- */

static void refresh_timer_cb(lv_timer_t *timer) {
    (void)timer;

    /* Update status label with current alarm engine state */
    update_status_label();

    /* Check for new alarm log entries */
    const vitals_alarm_log_t *alog = vitals_provider_get_alarm_log();
    if (alog->count > last_log_count && log_list) {
        /* New alarm entries -- add them to the list */
        int new_count = alog->count - last_log_count;
        if (new_count > VITALS_ALARM_LOG_MAX) new_count = VITALS_ALARM_LOG_MAX;
        for (int i = 0; i < new_count; i++) {
            int idx = (alog->write_idx - new_count + i + VITALS_ALARM_LOG_MAX) % VITALS_ALARM_LOG_MAX;
            add_log_entry_to_list(&alog->entries[idx]);
        }
        last_log_count = alog->count;
    }
}
