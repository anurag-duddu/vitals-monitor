/**
 * @file screen_patient.c
 * @brief Patient info screen — demographics and admission data
 *
 * Layout (800x480):
 *   +-----------------------------------------------------------------+
 *   | ALARM BAR (32px)                                                |
 *   +-------------------------------+---------------------------------+
 *   |  Patient Demographics         |  Current Vitals Summary         |
 *   |  Name: <from DB>              |  HR:    72 bpm                  |
 *   |  MRN:  <from DB>              |  SpO2:  97 %                   |
 *   |  DOB:  <from DB>              |  NIBP:  120/80 mmHg            |
 *   |  Gender: <from DB>            |  Temp:  36.8 C                  |
 *   |  Blood Type: <from DB>        |  RR:    16 /min                 |
 *   |  Ward: <from DB>              |                                 |
 *   |  Bed:  <from DB>              |  Clinical Notes                 |
 *   |  Attending: <from DB>         |  Allergies: <from DB>           |
 *   |  Weight: <from DB>            |  Diagnosis: <from DB>           |
 *   |  Height: <from DB>            |  Notes:     <from DB>           |
 *   |  [Discharge]                  |                                 |
 *   +-------------------------------+---------------------------------+
 *   | NAV BAR (48px)                                                  |
 *   +-----------------------------------------------------------------+
 */

#include "screen_patient.h"
#include "screen_manager.h"
#include "widget_alarm_banner.h"
#include "widget_nav_bar.h"
#include "theme_vitals.h"
#include "vitals_provider.h"
#include "patient_data.h"
#include <stdio.h>

/* -- Module state --------------------------------------------------- */

static widget_alarm_banner_t *alarm_banner;
static widget_nav_bar_t      *nav_bar;

/* -- Forward declarations ------------------------------------------- */

static void add_info_row(lv_obj_t *parent, const char *label, const char *value,
                          lv_color_t value_color);
static lv_obj_t * create_section(lv_obj_t *parent, const char *title, int width_pct);
static void discharge_cb(lv_event_t *e);

/* -- Public API ----------------------------------------------------- */

lv_obj_t * screen_patient_create(void) {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, VM_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* Fetch active patient for slot 0 */
    const patient_t *pt = patient_data_get_active(0);

    /* -- Alarm banner (top) ----------------------------------------- */
    alarm_banner = widget_alarm_banner_create(scr);

    /* -- Content area ----------------------------------------------- */
    lv_obj_t *content = lv_obj_create(scr);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(content, 0, VM_ALARM_BAR_HEIGHT);
    lv_obj_set_size(content, VM_SCREEN_WIDTH, VM_CONTENT_HEIGHT);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, VM_PAD_NORMAL, 0);
    lv_obj_set_style_pad_gap(content, VM_PAD_NORMAL, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_ROW);

    /* -- Left panel: Demographics ----------------------------------- */
    lv_obj_t *demo_panel = create_section(content, "Patient Demographics", 50);

    if (pt) {
        add_info_row(demo_panel, "Name",       pt->name,       VM_COLOR_TEXT_PRIMARY);
        add_info_row(demo_panel, "MRN",        pt->mrn,        VM_COLOR_TEXT_PRIMARY);
        add_info_row(demo_panel, "DOB",        pt->dob,        VM_COLOR_TEXT_PRIMARY);

        const char *gender_str = "Unknown";
        if (pt->gender == PATIENT_GENDER_MALE)        gender_str = "Male";
        else if (pt->gender == PATIENT_GENDER_FEMALE)  gender_str = "Female";
        else if (pt->gender == PATIENT_GENDER_OTHER)   gender_str = "Other";
        add_info_row(demo_panel, "Gender",     gender_str,     VM_COLOR_TEXT_PRIMARY);

        add_info_row(demo_panel, "Blood Type", pt->blood_type, VM_COLOR_ALARM_HIGH);
        add_info_row(demo_panel, "Ward",       pt->ward,       VM_COLOR_TEXT_PRIMARY);
        add_info_row(demo_panel, "Bed",        pt->bed,        VM_COLOR_TEXT_PRIMARY);
        add_info_row(demo_panel, "Attending",  pt->attending,  VM_COLOR_TEXT_PRIMARY);

        char wt_buf[16], ht_buf[16];
        if (pt->weight_kg > 0.0f)
            snprintf(wt_buf, sizeof(wt_buf), "%.0f kg", (double)pt->weight_kg);
        else
            snprintf(wt_buf, sizeof(wt_buf), "--");
        if (pt->height_cm > 0.0f)
            snprintf(ht_buf, sizeof(ht_buf), "%.0f cm", (double)pt->height_cm);
        else
            snprintf(ht_buf, sizeof(ht_buf), "--");
        add_info_row(demo_panel, "Weight",     wt_buf,         VM_COLOR_TEXT_PRIMARY);
        add_info_row(demo_panel, "Height",     ht_buf,         VM_COLOR_TEXT_PRIMARY);

        /* Discharge button */
        lv_obj_t *discharge_btn = lv_button_create(demo_panel);
        lv_obj_set_size(discharge_btn, VM_SCALE_W(120), VM_SCALE_H(36));
        lv_obj_set_style_bg_color(discharge_btn, VM_COLOR_ALARM_MEDIUM, 0);
        lv_obj_set_style_bg_opa(discharge_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(discharge_btn, 4, 0);
        lv_obj_add_event_cb(discharge_btn, discharge_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t *btn_lbl = lv_label_create(discharge_btn);
        lv_label_set_text(btn_lbl, "Discharge");
        lv_obj_set_style_text_font(btn_lbl, VM_FONT_CAPTION, 0);
        lv_obj_set_style_text_color(btn_lbl, VM_COLOR_BG, 0);
        lv_obj_center(btn_lbl);
    } else {
        /* No patient associated with this slot */
        lv_obj_t *empty_lbl = lv_label_create(demo_panel);
        lv_label_set_text(empty_lbl, "No Patient Associated");
        lv_obj_set_style_text_font(empty_lbl, VM_FONT_BODY, 0);
        lv_obj_set_style_text_color(empty_lbl, VM_COLOR_TEXT_DISABLED, 0);
    }

    /* -- Right column: Vitals summary + Clinical notes -------------- */
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

    const vitals_data_t *data = vitals_provider_get_current(0);
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

    if (pt) {
        add_info_row(notes_panel, "Allergies",
                     pt->allergies[0] ? pt->allergies : "None",
                     pt->allergies[0] ? VM_COLOR_ALARM_HIGH : VM_COLOR_TEXT_DISABLED);
        add_info_row(notes_panel, "Diagnosis",
                     pt->diagnosis[0] ? pt->diagnosis : "\xe2\x80\x94",
                     VM_COLOR_TEXT_PRIMARY);
        add_info_row(notes_panel, "Notes",
                     pt->notes[0] ? pt->notes : "\xe2\x80\x94",
                     VM_COLOR_TEXT_PRIMARY);
    } else {
        add_info_row(notes_panel, "Allergies",  "\xe2\x80\x94", VM_COLOR_TEXT_DISABLED);
        add_info_row(notes_panel, "Diagnosis",  "\xe2\x80\x94", VM_COLOR_TEXT_DISABLED);
        add_info_row(notes_panel, "Notes",      "\xe2\x80\x94", VM_COLOR_TEXT_DISABLED);
    }

    /* -- Nav bar (bottom) ------------------------------------------- */
    nav_bar = widget_nav_bar_create(scr);
    widget_nav_bar_set_active(nav_bar, SCREEN_ID_PATIENT);
    widget_nav_bar_set_patient(nav_bar, pt ? pt->name : "No Patient");

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

/* -- Private helpers ------------------------------------------------ */

static void discharge_cb(lv_event_t *e) {
    (void)e;
    const patient_t *pt = patient_data_get_active(0);
    if (pt) {
        printf("[patient] Discharging patient id=%d (%s)\n", (int)pt->id, pt->name);
        patient_data_discharge(pt->id);
        /* Refresh the screen to reflect the change */
        screen_manager_push(SCREEN_ID_PATIENT);
    }
}

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
