/**
 * @file screen_audit_log.c
 * @brief Audit log viewer screen -- regulatory compliance audit trail
 *
 * Layout (800x480):
 *   +--------------------------------------------------------------+
 *   | ALARM BAR (32px)                                             |
 *   +--------------------------------------------------------------+
 *   | [< Back]   Audit Log                                        |
 *   | [All] [Login] [Alarms] [Patient] [System]                   |
 *   +--------------------------------------------------------------+
 *   | Timestamp   Event              User      Message             |
 *   | ----------------------------------------------------------- |
 *   | 14:32:05    LOGIN              admin     Admin logged in     |
 *   | 14:30:12    ALARM_ACK          nurse     HR alarm acked      |
 *   | 14:28:00    PATIENT_ADMIT      doctor    MRN-001 admitted    |
 *   | ...                          (scrollable)                    |
 *   +--------------------------------------------------------------+
 *   | NAV BAR (48px)                                               |
 *   +--------------------------------------------------------------+
 */

#include "screen_audit_log.h"
#include "screen_manager.h"
#include "widget_alarm_banner.h"
#include "widget_nav_bar.h"
#include "theme_vitals.h"
#include "audit_log.h"
#include "patient_data.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

/* -- Filter categories -------------------------------------------- */

typedef enum {
    FILTER_ALL = 0,
    FILTER_LOGIN_LOGOUT,
    FILTER_ALARMS,
    FILTER_PATIENT,
    FILTER_SYSTEM,
    FILTER_COUNT
} audit_filter_t;

/* -- Module state -------------------------------------------------- */

static widget_alarm_banner_t *alarm_banner;
static widget_nav_bar_t      *nav_bar;
static lv_obj_t              *entry_list;
static lv_obj_t              *filter_buttons[FILTER_COUNT];
static audit_filter_t         active_filter;
static lv_timer_t            *refresh_timer;

/* Static query result buffer (avoid malloc) */
static audit_query_result_t   query_result;

/* -- Forward declarations ------------------------------------------ */

static void build_header(lv_obj_t *parent);
static void build_filter_bar(lv_obj_t *parent);
static void build_entry_list(lv_obj_t *parent);
static void populate_entries(void);
static void add_entry_row(const audit_entry_t *entry);
static void filter_btn_cb(lv_event_t *e);
static void back_btn_cb(lv_event_t *e);
static void refresh_timer_cb(lv_timer_t *timer);
static void update_filter_btn_styles(void);
static void format_timestamp(uint32_t ts, char *buf, size_t buf_size);

/* -- Filter label strings ------------------------------------------ */

static const char *filter_labels[FILTER_COUNT] = {
    "All", "Login/Logout", "Alarms", "Patient", "System"
};

/* -- Public API ---------------------------------------------------- */

lv_obj_t *screen_audit_log_create(void) {
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
    lv_obj_set_style_pad_gap(content, VM_PAD_SMALL, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);

    /* Header with title and back button */
    build_header(content);

    /* Filter button bar */
    active_filter = FILTER_ALL;
    build_filter_bar(content);

    /* Entry list (scrollable) */
    build_entry_list(content);

    /* Populate with initial data */
    populate_entries();

    /* Refresh timer -- check for new entries every 5 seconds */
    refresh_timer = lv_timer_create(refresh_timer_cb, 5000, NULL);

    /* -- Nav bar (bottom) ------------------------------------------ */
    nav_bar = widget_nav_bar_create(scr);

    /* Show active patient name */
    const patient_t *pt = patient_data_get_active(0);
    widget_nav_bar_set_patient(nav_bar, pt ? pt->name : "No Patient");

    printf("[audit_log_screen] Screen created\n");
    return scr;
}

void screen_audit_log_destroy(void) {
    if (refresh_timer) {
        lv_timer_delete(refresh_timer);
        refresh_timer = NULL;
    }
    widget_alarm_banner_free(alarm_banner);
    widget_nav_bar_free(nav_bar);
    alarm_banner = NULL;
    nav_bar      = NULL;
    entry_list   = NULL;
    for (int i = 0; i < FILTER_COUNT; i++) {
        filter_buttons[i] = NULL;
    }
    printf("[audit_log_screen] Screen destroyed\n");
}

/* -- Header: back button + title ----------------------------------- */

static void build_header(lv_obj_t *parent) {
    lv_obj_t *header = lv_obj_create(parent);
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(header, lv_pct(100));
    lv_obj_set_height(header, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_set_style_pad_gap(header, VM_PAD_NORMAL, 0);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Back button */
    lv_obj_t *btn_back = lv_button_create(header);
    lv_obj_set_size(btn_back, VM_SCALE_W(70), VM_SCALE_H(32));
    lv_obj_set_style_bg_color(btn_back, VM_COLOR_BG_PANEL, 0);
    lv_obj_set_style_border_color(btn_back, VM_COLOR_BG_PANEL_BORDER, 0);
    lv_obj_set_style_border_width(btn_back, 1, 0);
    lv_obj_set_style_radius(btn_back, 4, 0);
    lv_obj_add_event_cb(btn_back, back_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_back = lv_label_create(btn_back);
    lv_label_set_text(lbl_back, LV_SYMBOL_LEFT " Back");
    lv_obj_center(lbl_back);
    lv_obj_set_style_text_font(lbl_back, VM_FONT_CAPTION, 0);
    lv_obj_set_style_text_color(lbl_back, VM_COLOR_TEXT_PRIMARY, 0);

    /* Title */
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Audit Log");
    lv_obj_set_style_text_font(title, VM_FONT_LABEL, 0);
    lv_obj_set_style_text_color(title, VM_COLOR_TEXT_PRIMARY, 0);
}

/* -- Filter button bar --------------------------------------------- */

static void build_filter_bar(lv_obj_t *parent) {
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(bar, lv_pct(100));
    lv_obj_set_height(bar, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_pad_all(bar, 0, 0);
    lv_obj_set_style_pad_gap(bar, VM_PAD_SMALL, 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);

    for (int i = 0; i < FILTER_COUNT; i++) {
        lv_obj_t *btn = lv_button_create(bar);
        lv_obj_set_size(btn, LV_SIZE_CONTENT, VM_SCALE_H(28));
        lv_obj_set_style_pad_left(btn, VM_PAD_NORMAL, 0);
        lv_obj_set_style_pad_right(btn, VM_PAD_NORMAL, 0);
        lv_obj_set_style_pad_top(btn, VM_PAD_SMALL, 0);
        lv_obj_set_style_pad_bottom(btn, VM_PAD_SMALL, 0);
        lv_obj_set_style_radius(btn, 4, 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_border_color(btn, VM_COLOR_BG_PANEL_BORDER, 0);
        lv_obj_add_event_cb(btn, filter_btn_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)i);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, filter_labels[i]);
        lv_obj_center(lbl);
        lv_obj_set_style_text_font(lbl, VM_FONT_SMALL, 0);

        filter_buttons[i] = btn;
    }

    update_filter_btn_styles();
}

/* -- Entry list (scrollable table-like layout) --------------------- */

static void build_entry_list(lv_obj_t *parent) {
    /* Column header row */
    lv_obj_t *hdr_row = lv_obj_create(parent);
    lv_obj_remove_flag(hdr_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(hdr_row, lv_pct(100));
    lv_obj_set_height(hdr_row, VM_SCALE_H(24));
    lv_obj_set_style_bg_color(hdr_row, VM_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(hdr_row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hdr_row, 0, 0);
    lv_obj_set_style_pad_all(hdr_row, 0, 0);
    lv_obj_set_style_pad_gap(hdr_row, VM_PAD_NORMAL, 0);
    lv_obj_set_flex_flow(hdr_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hdr_row, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Column: Timestamp */
    lv_obj_t *h_ts = lv_label_create(hdr_row);
    lv_label_set_text(h_ts, "Timestamp");
    lv_obj_set_style_text_font(h_ts, VM_FONT_SMALL, 0);
    lv_obj_set_style_text_color(h_ts, VM_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_width(h_ts, VM_SCALE_W(100));

    /* Column: Event */
    lv_obj_t *h_ev = lv_label_create(hdr_row);
    lv_label_set_text(h_ev, "Event");
    lv_obj_set_style_text_font(h_ev, VM_FONT_SMALL, 0);
    lv_obj_set_style_text_color(h_ev, VM_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_width(h_ev, VM_SCALE_W(180));

    /* Column: User */
    lv_obj_t *h_user = lv_label_create(hdr_row);
    lv_label_set_text(h_user, "User");
    lv_obj_set_style_text_font(h_user, VM_FONT_SMALL, 0);
    lv_obj_set_style_text_color(h_user, VM_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_width(h_user, VM_SCALE_W(100));

    /* Column: Message */
    lv_obj_t *h_msg = lv_label_create(hdr_row);
    lv_label_set_text(h_msg, "Message");
    lv_obj_set_style_text_font(h_msg, VM_FONT_SMALL, 0);
    lv_obj_set_style_text_color(h_msg, VM_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_flex_grow(h_msg, 1);

    /* Scrollable list container */
    entry_list = lv_obj_create(parent);
    lv_obj_set_width(entry_list, lv_pct(100));
    lv_obj_set_flex_grow(entry_list, 1);
    lv_obj_set_style_bg_opa(entry_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(entry_list, 0, 0);
    lv_obj_set_style_pad_all(entry_list, 0, 0);
    lv_obj_set_style_pad_gap(entry_list, 1, 0);
    lv_obj_set_flex_flow(entry_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_flag(entry_list, LV_OBJ_FLAG_SCROLLABLE);
}

/* -- Populate entries from audit_log query ----------------------- */

static void populate_entries(void) {
    if (!entry_list) return;

    /* Clear existing children */
    lv_obj_clean(entry_list);

    int count = 0;

    switch (active_filter) {
        case FILTER_ALL:
            count = audit_log_query_recent(AUDIT_LOG_QUERY_MAX, &query_result);
            break;

        case FILTER_LOGIN_LOGOUT:
            /* Combine login-related events by querying each type */
            {
                audit_query_result_t tmp;
                count = 0;
                int n;

                n = audit_log_query_by_event(AUDIT_EVENT_LOGIN, AUDIT_LOG_QUERY_MAX, &query_result);
                count = n;

                n = audit_log_query_by_event(AUDIT_EVENT_LOGOUT, AUDIT_LOG_QUERY_MAX, &tmp);
                for (int i = 0; i < n && count < AUDIT_LOG_QUERY_MAX; i++) {
                    query_result.entries[count++] = tmp.entries[i];
                }

                n = audit_log_query_by_event(AUDIT_EVENT_LOGIN_FAILED, AUDIT_LOG_QUERY_MAX, &tmp);
                for (int i = 0; i < n && count < AUDIT_LOG_QUERY_MAX; i++) {
                    query_result.entries[count++] = tmp.entries[i];
                }

                n = audit_log_query_by_event(AUDIT_EVENT_SESSION_TIMEOUT, AUDIT_LOG_QUERY_MAX, &tmp);
                for (int i = 0; i < n && count < AUDIT_LOG_QUERY_MAX; i++) {
                    query_result.entries[count++] = tmp.entries[i];
                }

                query_result.count = count;
            }
            break;

        case FILTER_ALARMS:
            {
                audit_query_result_t tmp;
                count = 0;
                int n;

                n = audit_log_query_by_event(AUDIT_EVENT_ALARM_ACK, AUDIT_LOG_QUERY_MAX, &query_result);
                count = n;

                n = audit_log_query_by_event(AUDIT_EVENT_ALARM_SILENCE, AUDIT_LOG_QUERY_MAX, &tmp);
                for (int i = 0; i < n && count < AUDIT_LOG_QUERY_MAX; i++) {
                    query_result.entries[count++] = tmp.entries[i];
                }

                n = audit_log_query_by_event(AUDIT_EVENT_ALARM_LIMITS_CHANGED, AUDIT_LOG_QUERY_MAX, &tmp);
                for (int i = 0; i < n && count < AUDIT_LOG_QUERY_MAX; i++) {
                    query_result.entries[count++] = tmp.entries[i];
                }

                query_result.count = count;
            }
            break;

        case FILTER_PATIENT:
            {
                audit_query_result_t tmp;
                count = 0;
                int n;

                n = audit_log_query_by_event(AUDIT_EVENT_PATIENT_ADMIT, AUDIT_LOG_QUERY_MAX, &query_result);
                count = n;

                n = audit_log_query_by_event(AUDIT_EVENT_PATIENT_DISCHARGE, AUDIT_LOG_QUERY_MAX, &tmp);
                for (int i = 0; i < n && count < AUDIT_LOG_QUERY_MAX; i++) {
                    query_result.entries[count++] = tmp.entries[i];
                }

                n = audit_log_query_by_event(AUDIT_EVENT_PATIENT_MODIFIED, AUDIT_LOG_QUERY_MAX, &tmp);
                for (int i = 0; i < n && count < AUDIT_LOG_QUERY_MAX; i++) {
                    query_result.entries[count++] = tmp.entries[i];
                }

                query_result.count = count;
            }
            break;

        case FILTER_SYSTEM:
            {
                audit_query_result_t tmp;
                count = 0;
                int n;

                n = audit_log_query_by_event(AUDIT_EVENT_SYSTEM_START, AUDIT_LOG_QUERY_MAX, &query_result);
                count = n;

                n = audit_log_query_by_event(AUDIT_EVENT_SYSTEM_SHUTDOWN, AUDIT_LOG_QUERY_MAX, &tmp);
                for (int i = 0; i < n && count < AUDIT_LOG_QUERY_MAX; i++) {
                    query_result.entries[count++] = tmp.entries[i];
                }

                n = audit_log_query_by_event(AUDIT_EVENT_SETTINGS_CHANGED, AUDIT_LOG_QUERY_MAX, &tmp);
                for (int i = 0; i < n && count < AUDIT_LOG_QUERY_MAX; i++) {
                    query_result.entries[count++] = tmp.entries[i];
                }

                n = audit_log_query_by_event(AUDIT_EVENT_USER_CREATED, AUDIT_LOG_QUERY_MAX, &tmp);
                for (int i = 0; i < n && count < AUDIT_LOG_QUERY_MAX; i++) {
                    query_result.entries[count++] = tmp.entries[i];
                }

                n = audit_log_query_by_event(AUDIT_EVENT_USER_DELETED, AUDIT_LOG_QUERY_MAX, &tmp);
                for (int i = 0; i < n && count < AUDIT_LOG_QUERY_MAX; i++) {
                    query_result.entries[count++] = tmp.entries[i];
                }

                query_result.count = count;
            }
            break;

        default:
            count = audit_log_query_recent(AUDIT_LOG_QUERY_MAX, &query_result);
            break;
    }

    if (count == 0) {
        lv_obj_t *empty = lv_label_create(entry_list);
        lv_label_set_text(empty, "No audit entries found");
        lv_obj_set_style_text_font(empty, VM_FONT_CAPTION, 0);
        lv_obj_set_style_text_color(empty, VM_COLOR_TEXT_DISABLED, 0);
        lv_obj_set_style_pad_top(empty, VM_PAD_LARGE, 0);
        return;
    }

    /* Add entries to the list */
    for (int i = 0; i < count; i++) {
        add_entry_row(&query_result.entries[i]);
    }
}

/* -- Add a single entry row to the list --------------------------- */

static void add_entry_row(const audit_entry_t *entry) {
    if (!entry_list) return;

    lv_obj_t *row = lv_obj_create(entry_list);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, VM_SCALE_H(24));
    lv_obj_set_style_bg_color(row, VM_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_50, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_style_pad_gap(row, VM_PAD_NORMAL, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Timestamp */
    static char ts_buf[20];
    format_timestamp(entry->timestamp_s, ts_buf, sizeof(ts_buf));

    lv_obj_t *ts_lbl = lv_label_create(row);
    lv_label_set_text(ts_lbl, ts_buf);
    lv_obj_set_style_text_font(ts_lbl, VM_FONT_SMALL, 0);
    lv_obj_set_style_text_color(ts_lbl, VM_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_width(ts_lbl, VM_SCALE_W(100));

    /* Event type */
    lv_obj_t *ev_lbl = lv_label_create(row);
    lv_label_set_text(ev_lbl, audit_event_name(entry->event));
    lv_obj_set_style_text_font(ev_lbl, VM_FONT_SMALL, 0);
    lv_obj_set_width(ev_lbl, VM_SCALE_W(180));

    /* Color-code event type label */
    switch (entry->event) {
        case AUDIT_EVENT_LOGIN:
        case AUDIT_EVENT_LOGOUT:
            lv_obj_set_style_text_color(ev_lbl, VM_COLOR_ALARM_NONE, 0);
            break;
        case AUDIT_EVENT_LOGIN_FAILED:
        case AUDIT_EVENT_SESSION_TIMEOUT:
            lv_obj_set_style_text_color(ev_lbl, VM_COLOR_ALARM_MEDIUM, 0);
            break;
        case AUDIT_EVENT_ALARM_ACK:
        case AUDIT_EVENT_ALARM_SILENCE:
        case AUDIT_EVENT_ALARM_LIMITS_CHANGED:
            lv_obj_set_style_text_color(ev_lbl, VM_COLOR_ALARM_HIGH, 0);
            break;
        case AUDIT_EVENT_PATIENT_ADMIT:
        case AUDIT_EVENT_PATIENT_DISCHARGE:
        case AUDIT_EVENT_PATIENT_MODIFIED:
            lv_obj_set_style_text_color(ev_lbl, VM_COLOR_SPO2, 0);
            break;
        default:
            lv_obj_set_style_text_color(ev_lbl, VM_COLOR_TEXT_PRIMARY, 0);
            break;
    }

    /* Username */
    lv_obj_t *user_lbl = lv_label_create(row);
    lv_label_set_text(user_lbl, entry->username);
    lv_obj_set_style_text_font(user_lbl, VM_FONT_SMALL, 0);
    lv_obj_set_style_text_color(user_lbl, VM_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_width(user_lbl, VM_SCALE_W(100));

    /* Message */
    lv_obj_t *msg_lbl = lv_label_create(row);
    lv_label_set_text(msg_lbl, entry->message);
    lv_obj_set_style_text_font(msg_lbl, VM_FONT_SMALL, 0);
    lv_obj_set_style_text_color(msg_lbl, VM_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_flex_grow(msg_lbl, 1);
    lv_label_set_long_mode(msg_lbl, LV_LABEL_LONG_CLIP);
}

/* -- Filter button callback ---------------------------------------- */

static void filter_btn_cb(lv_event_t *e) {
    audit_filter_t f = (audit_filter_t)(intptr_t)lv_event_get_user_data(e);
    if (f >= FILTER_COUNT) return;

    active_filter = f;
    update_filter_btn_styles();
    populate_entries();

    printf("[audit_log_screen] Filter changed to: %s\n", filter_labels[f]);
}

/* -- Back button callback ------------------------------------------ */

static void back_btn_cb(lv_event_t *e) {
    (void)e;
    screen_manager_pop();
}

/* -- Refresh timer ------------------------------------------------- */

static void refresh_timer_cb(lv_timer_t *timer) {
    (void)timer;
    /* Re-populate the list with current filter to pick up new entries */
    populate_entries();
}

/* -- Update filter button visual state ----------------------------- */

static void update_filter_btn_styles(void) {
    for (int i = 0; i < FILTER_COUNT; i++) {
        if (!filter_buttons[i]) continue;

        if ((int)i == (int)active_filter) {
            /* Active filter: highlighted */
            lv_obj_set_style_bg_color(filter_buttons[i],
                                       VM_COLOR_ALARM_LOW, 0);
            lv_obj_set_style_bg_opa(filter_buttons[i], LV_OPA_COVER, 0);

            /* Set child label text color to dark */
            lv_obj_t *lbl = lv_obj_get_child(filter_buttons[i], 0);
            if (lbl) {
                lv_obj_set_style_text_color(lbl, lv_color_hex(0x000000), 0);
            }
        } else {
            /* Inactive filter: dimmed */
            lv_obj_set_style_bg_color(filter_buttons[i],
                                       VM_COLOR_BG_PANEL, 0);
            lv_obj_set_style_bg_opa(filter_buttons[i], LV_OPA_COVER, 0);

            /* Set child label text color to light */
            lv_obj_t *lbl = lv_obj_get_child(filter_buttons[i], 0);
            if (lbl) {
                lv_obj_set_style_text_color(lbl, VM_COLOR_TEXT_SECONDARY, 0);
            }
        }
    }
}

/* -- Format unix timestamp to "HH:MM:SS" string ------------------- */

static void format_timestamp(uint32_t ts, char *buf, size_t buf_size) {
    if (ts == 0) {
        snprintf(buf, buf_size, "--:--:--");
        return;
    }

    time_t t = (time_t)ts;
    struct tm *tm_info = localtime(&t);
    if (tm_info) {
        snprintf(buf, buf_size, "%02d:%02d:%02d",
                 tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    } else {
        snprintf(buf, buf_size, "--:--:--");
    }
}
