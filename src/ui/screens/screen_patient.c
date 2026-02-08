/**
 * @file screen_patient.c
 * @brief Patient info screen — demographics and admission data
 *
 * Layout (800x480):
 *   ┌──────────────────────────────────────────────────────────────┐
 *   │ ALARM BAR (32px)                                            │
 *   ├─────────────────────────────┬────────────────────────────────┤
 *   │  Patient Demographics       │  Current Vitals Summary       │
 *   │  Name: John Smith           │  HR:    72 bpm                │
 *   │  MRN:  MRN-2024-001234     │  SpO2:  97 %                  │
 *   │  DOB:  1958-03-15           │  NIBP:  120/80 mmHg           │
 *   │  Gender: Male               │  Temp:  36.8 C                │
 *   │  Room: ICU Bed 4            │  RR:    16 /min               │
 *   │  Attending: Dr. Patel       │                               │
 *   │  Admitted: 2024-01-15       │  Allergies:                   │
 *   │  Weight: 82 kg              │  Penicillin, Sulfa            │
 *   │  Height: 178 cm             │                               │
 *   ├─────────────────────────────┴────────────────────────────────┤
 *   │ NAV BAR (48px)                                              │
 *   └──────────────────────────────────────────────────────────────┘
 */

#include "screen_patient.h"
#include "screen_manager.h"
#include "widget_alarm_banner.h"
#include "widget_nav_bar.h"
#include "theme_vitals.h"
#include "mock_data.h"
#include <stdio.h>

/* ── Module state ──────────────────────────────────────────── */

static widget_alarm_banner_t *alarm_banner;
static widget_nav_bar_t      *nav_bar;

/* ── Forward declarations ──────────────────────────────────── */

static void add_info_row(lv_obj_t *parent, const char *label, const char *value,
                          lv_color_t value_color);
static lv_obj_t * create_section(lv_obj_t *parent, const char *title, int width_pct);

/* ── Public API ────────────────────────────────────────────── */

lv_obj_t * screen_patient_create(void) {
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

    /* ── Left panel: Demographics ────────────────────────── */
    lv_obj_t *demo_panel = create_section(content, "Patient Demographics", 50);

    add_info_row(demo_panel, "Name",      "John Smith",        VM_COLOR_TEXT_PRIMARY);
    add_info_row(demo_panel, "MRN",       "MRN-2024-001234",   VM_COLOR_TEXT_PRIMARY);
    add_info_row(demo_panel, "DOB",       "1958-03-15  (67y)", VM_COLOR_TEXT_PRIMARY);
    add_info_row(demo_panel, "Gender",    "Male",              VM_COLOR_TEXT_PRIMARY);
    add_info_row(demo_panel, "Blood Type","O+",                VM_COLOR_ALARM_HIGH);
    add_info_row(demo_panel, "Room",      "ICU Bed 4",         VM_COLOR_TEXT_PRIMARY);
    add_info_row(demo_panel, "Attending", "Dr. Priya Patel",   VM_COLOR_TEXT_PRIMARY);
    add_info_row(demo_panel, "Admitted",  "2024-01-15",        VM_COLOR_TEXT_PRIMARY);
    add_info_row(demo_panel, "Weight",    "82 kg",             VM_COLOR_TEXT_PRIMARY);
    add_info_row(demo_panel, "Height",    "178 cm",            VM_COLOR_TEXT_PRIMARY);

    /* ── Right panel: Current vitals summary + clinical ──── */
    lv_obj_t *right_col = lv_obj_create(content);
    lv_obj_remove_flag(right_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_grow(right_col, 1);
    lv_obj_set_height(right_col, lv_pct(100));
    lv_obj_set_style_bg_opa(right_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(right_col, 0, 0);
    lv_obj_set_style_pad_all(right_col, 0, 0);
    lv_obj_set_style_pad_gap(right_col, VM_PAD_NORMAL, 0);
    lv_obj_set_flex_flow(right_col, LV_FLEX_FLOW_COLUMN);

    /* Current vitals snapshot */
    lv_obj_t *vitals_panel = lv_obj_create(right_col);
    lv_obj_remove_flag(vitals_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(vitals_panel, lv_pct(100));
    lv_obj_set_flex_grow(vitals_panel, 1);
    lv_obj_set_style_bg_color(vitals_panel, VM_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(vitals_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(vitals_panel, VM_COLOR_BG_PANEL_BORDER, 0);
    lv_obj_set_style_border_width(vitals_panel, 1, 0);
    lv_obj_set_style_radius(vitals_panel, 4, 0);
    lv_obj_set_style_pad_all(vitals_panel, VM_PAD_NORMAL, 0);
    lv_obj_set_style_pad_gap(vitals_panel, VM_PAD_SMALL, 0);
    lv_obj_set_flex_flow(vitals_panel, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *vt = lv_label_create(vitals_panel);
    lv_label_set_text(vt, "Current Vitals");
    lv_obj_set_style_text_font(vt, VM_FONT_BODY, 0);
    lv_obj_set_style_text_color(vt, VM_COLOR_TEXT_PRIMARY, 0);

    const vitals_data_t *data = mock_data_get_current();
    char buf[64];
    snprintf(buf, sizeof(buf), "%d bpm", data->hr);
    add_info_row(vitals_panel, "HR",   buf, VM_COLOR_HR);
    snprintf(buf, sizeof(buf), "%d %%", data->spo2);
    add_info_row(vitals_panel, "SpO2", buf, VM_COLOR_SPO2);
    snprintf(buf, sizeof(buf), "%d/%d (%d) mmHg", data->nibp_sys, data->nibp_dia, data->nibp_map);
    add_info_row(vitals_panel, "NIBP", buf, VM_COLOR_NIBP);
    snprintf(buf, sizeof(buf), "%.1f \xc2\xb0""C", (double)data->temp);
    add_info_row(vitals_panel, "Temp", buf, VM_COLOR_TEMP);
    snprintf(buf, sizeof(buf), "%d /min", data->rr);
    add_info_row(vitals_panel, "RR",   buf, VM_COLOR_RR);

    /* Clinical notes */
    lv_obj_t *notes_panel = lv_obj_create(right_col);
    lv_obj_remove_flag(notes_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(notes_panel, lv_pct(100));
    lv_obj_set_flex_grow(notes_panel, 1);
    lv_obj_set_style_bg_color(notes_panel, VM_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(notes_panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(notes_panel, VM_COLOR_BG_PANEL_BORDER, 0);
    lv_obj_set_style_border_width(notes_panel, 1, 0);
    lv_obj_set_style_radius(notes_panel, 4, 0);
    lv_obj_set_style_pad_all(notes_panel, VM_PAD_NORMAL, 0);
    lv_obj_set_style_pad_gap(notes_panel, VM_PAD_SMALL, 0);
    lv_obj_set_flex_flow(notes_panel, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *nt = lv_label_create(notes_panel);
    lv_label_set_text(nt, "Clinical Notes");
    lv_obj_set_style_text_font(nt, VM_FONT_BODY, 0);
    lv_obj_set_style_text_color(nt, VM_COLOR_TEXT_PRIMARY, 0);

    add_info_row(notes_panel, "Allergies",  "Penicillin, Sulfa",     VM_COLOR_ALARM_HIGH);
    add_info_row(notes_panel, "Diagnosis",  "Post-op cardiac",       VM_COLOR_TEXT_PRIMARY);
    add_info_row(notes_panel, "DNR Status", "Full Code",             VM_COLOR_ALARM_NONE);
    add_info_row(notes_panel, "IV Access",  "Right arm, 18G",       VM_COLOR_TEXT_PRIMARY);

    /* ── Nav bar (bottom) ────────────────────────────────── */
    nav_bar = widget_nav_bar_create(scr);
    widget_nav_bar_set_active(nav_bar, SCREEN_ID_PATIENT);
    widget_nav_bar_set_patient(nav_bar, "Smith, John");

    printf("[patient] Screen created\n");
    return scr;
}

void screen_patient_destroy(void) {
    widget_alarm_banner_free(alarm_banner);
    widget_nav_bar_free(nav_bar);
    alarm_banner = NULL;
    nav_bar = NULL;
    printf("[patient] Screen destroyed\n");
}

/* ── Private helpers ───────────────────────────────────────── */

static lv_obj_t * create_section(lv_obj_t *parent, const char *title, int width_pct) {
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(panel, lv_pct(width_pct), lv_pct(100));
    lv_obj_set_style_bg_color(panel, VM_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(panel, VM_COLOR_BG_PANEL_BORDER, 0);
    lv_obj_set_style_border_width(panel, 1, 0);
    lv_obj_set_style_radius(panel, 4, 0);
    lv_obj_set_style_pad_all(panel, VM_PAD_NORMAL, 0);
    lv_obj_set_style_pad_gap(panel, VM_PAD_SMALL, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);

    lv_obj_t *lbl = lv_label_create(panel);
    lv_label_set_text(lbl, title);
    lv_obj_set_style_text_font(lbl, VM_FONT_BODY, 0);
    lv_obj_set_style_text_color(lbl, VM_COLOR_TEXT_PRIMARY, 0);

    return panel;
}

static void add_info_row(lv_obj_t *parent, const char *label, const char *value,
                          lv_color_t value_color) {
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(row, VM_PAD_NORMAL, 0);

    /* Label */
    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_font(lbl, VM_FONT_CAPTION, 0);
    lv_obj_set_style_text_color(lbl, VM_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_width(lbl, 80);

    /* Value */
    lv_obj_t *val = lv_label_create(row);
    lv_label_set_text(val, value);
    lv_obj_set_style_text_font(val, VM_FONT_CAPTION, 0);
    lv_obj_set_style_text_color(val, value_color, 0);
    lv_obj_set_flex_grow(val, 1);
}
