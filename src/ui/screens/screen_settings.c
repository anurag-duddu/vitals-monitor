/**
 * @file screen_settings.c
 * @brief Settings screen — device configuration display with interactive controls
 *
 * Layout (800x480):
 *   ┌──────────────────────────────────────────────────────────────┐
 *   │ ALARM BAR (32px)                                            │
 *   ├─────────────────────────────┬────────────────────────────────┤
 *   │  Display Settings           │  Device Info                  │
 *   │  Brightness: [====----] 60% │  Model: VM-800               │
 *   │  Auto-dim:   [ON/OFF]       │  S/N:   SIM-00001            │
 *   │  Screen lock: 2 min         │  FW:    1.0.0-sim            │
 *   │                              │  LVGL:  9.3.0               │
 *   │  Audio Settings             │                               │
 *   │  Alarm Vol:  [=======] 80%  │  Network                     │
 *   │  Key click:  [ON/OFF]       │  Status: Disconnected        │
 *   │  Alarm mute: OFF            │  HL7: Not configured         │
 *   ├─────────────────────────────┴────────────────────────────────┤
 *   │ NAV BAR (48px)                                              │
 *   └──────────────────────────────────────────────────────────────┘
 */

#include "screen_settings.h"
#include "screen_manager.h"
#include "widget_alarm_banner.h"
#include "widget_nav_bar.h"
#include "theme_vitals.h"
#include "mock_data.h"
#include <stdio.h>

/* ── Module state ──────────────────────────────────────────── */

static widget_alarm_banner_t *alarm_banner;
static widget_nav_bar_t      *nav_bar;
static lv_obj_t              *brightness_slider;
static lv_obj_t              *brightness_label;
static lv_obj_t              *volume_slider;
static lv_obj_t              *volume_label;

/* ── Forward declarations ──────────────────────────────────── */

static lv_obj_t * create_section_panel(lv_obj_t *parent, const char *title);
static void add_setting_row(lv_obj_t *parent, const char *label, const char *value);
static void add_switch_row(lv_obj_t *parent, const char *label, bool initial_state);
static void brightness_cb(lv_event_t *e);
static void volume_cb(lv_event_t *e);

/* ── Public API ────────────────────────────────────────────── */

lv_obj_t * screen_settings_create(void) {
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

    /* ── Left column ─────────────────────────────────────── */
    lv_obj_t *left_col = lv_obj_create(content);
    lv_obj_remove_flag(left_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(left_col, lv_pct(50), lv_pct(100));
    lv_obj_set_style_bg_opa(left_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(left_col, 0, 0);
    lv_obj_set_style_pad_all(left_col, 0, 0);
    lv_obj_set_style_pad_gap(left_col, VM_PAD_NORMAL, 0);
    lv_obj_set_flex_flow(left_col, LV_FLEX_FLOW_COLUMN);

    /* Display Settings */
    lv_obj_t *display_section = create_section_panel(left_col, "Display Settings");

    /* Brightness slider */
    {
        lv_obj_t *row = lv_obj_create(display_section);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_width(row, lv_pct(100));
        lv_obj_set_height(row, 36);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_style_pad_gap(row, VM_PAD_NORMAL, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t *lbl = lv_label_create(row);
        lv_label_set_text(lbl, "Brightness");
        lv_obj_set_style_text_font(lbl, VM_FONT_CAPTION, 0);
        lv_obj_set_style_text_color(lbl, VM_COLOR_TEXT_SECONDARY, 0);
        lv_obj_set_width(lbl, 80);

        brightness_slider = lv_slider_create(row);
        lv_obj_set_flex_grow(brightness_slider, 1);
        lv_obj_set_height(brightness_slider, 10);
        lv_slider_set_range(brightness_slider, 10, 100);
        lv_slider_set_value(brightness_slider, 70, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(brightness_slider, VM_COLOR_BG_PANEL_BORDER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(brightness_slider, VM_COLOR_SPO2, LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(brightness_slider, VM_COLOR_TEXT_PRIMARY, LV_PART_KNOB);
        lv_obj_set_style_pad_all(brightness_slider, 4, LV_PART_KNOB);
        lv_obj_add_event_cb(brightness_slider, brightness_cb, LV_EVENT_VALUE_CHANGED, NULL);

        brightness_label = lv_label_create(row);
        lv_label_set_text(brightness_label, "70%");
        lv_obj_set_style_text_font(brightness_label, VM_FONT_CAPTION, 0);
        lv_obj_set_style_text_color(brightness_label, VM_COLOR_TEXT_PRIMARY, 0);
        lv_obj_set_width(brightness_label, 40);
    }

    add_switch_row(display_section, "Auto-dim",     true);
    add_setting_row(display_section, "Screen lock",  "2 minutes");
    add_setting_row(display_section, "Color scheme",  "Dark (Medical)");

    /* Audio Settings */
    lv_obj_t *audio_section = create_section_panel(left_col, "Audio Settings");

    /* Volume slider */
    {
        lv_obj_t *row = lv_obj_create(audio_section);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_width(row, lv_pct(100));
        lv_obj_set_height(row, 36);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_style_pad_all(row, 0, 0);
        lv_obj_set_style_pad_gap(row, VM_PAD_NORMAL, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t *lbl = lv_label_create(row);
        lv_label_set_text(lbl, "Alarm Vol.");
        lv_obj_set_style_text_font(lbl, VM_FONT_CAPTION, 0);
        lv_obj_set_style_text_color(lbl, VM_COLOR_TEXT_SECONDARY, 0);
        lv_obj_set_width(lbl, 80);

        volume_slider = lv_slider_create(row);
        lv_obj_set_flex_grow(volume_slider, 1);
        lv_obj_set_height(volume_slider, 10);
        lv_slider_set_range(volume_slider, 0, 100);
        lv_slider_set_value(volume_slider, 80, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(volume_slider, VM_COLOR_BG_PANEL_BORDER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(volume_slider, VM_COLOR_ALARM_MEDIUM, LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(volume_slider, VM_COLOR_TEXT_PRIMARY, LV_PART_KNOB);
        lv_obj_set_style_pad_all(volume_slider, 4, LV_PART_KNOB);
        lv_obj_add_event_cb(volume_slider, volume_cb, LV_EVENT_VALUE_CHANGED, NULL);

        volume_label = lv_label_create(row);
        lv_label_set_text(volume_label, "80%");
        lv_obj_set_style_text_font(volume_label, VM_FONT_CAPTION, 0);
        lv_obj_set_style_text_color(volume_label, VM_COLOR_TEXT_PRIMARY, 0);
        lv_obj_set_width(volume_label, 40);
    }

    add_switch_row(audio_section, "Key click",     true);
    add_switch_row(audio_section, "Alarm mute",    false);

    /* ── Right column ────────────────────────────────────── */
    lv_obj_t *right_col = lv_obj_create(content);
    lv_obj_remove_flag(right_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_grow(right_col, 1);
    lv_obj_set_height(right_col, lv_pct(100));
    lv_obj_set_style_bg_opa(right_col, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(right_col, 0, 0);
    lv_obj_set_style_pad_all(right_col, 0, 0);
    lv_obj_set_style_pad_gap(right_col, VM_PAD_NORMAL, 0);
    lv_obj_set_flex_flow(right_col, LV_FLEX_FLOW_COLUMN);

    /* Device Info */
    lv_obj_t *info_section = create_section_panel(right_col, "Device Info");
    add_setting_row(info_section, "Model",    "VM-800 Bedside");
    add_setting_row(info_section, "S/N",      "SIM-00001");
    add_setting_row(info_section, "Firmware",  "1.0.0-sim");

    char lvgl_ver[32];
    snprintf(lvgl_ver, sizeof(lvgl_ver), "%d.%d.%d",
             lv_version_major(), lv_version_minor(), lv_version_patch());
    add_setting_row(info_section, "LVGL",     lvgl_ver);
    add_setting_row(info_section, "Display",  "800x480 TFT");
    add_setting_row(info_section, "Uptime",   "Simulator");

    /* Network */
    lv_obj_t *net_section = create_section_panel(right_col, "Network");
    add_setting_row(net_section, "Status",    "Disconnected");
    add_setting_row(net_section, "HL7 FHIR",  "Not configured");
    add_setting_row(net_section, "WiFi",       "Not available");
    add_setting_row(net_section, "Ethernet",   "Not connected");

    /* ── Nav bar (bottom) ────────────────────────────────── */
    nav_bar = widget_nav_bar_create(scr);
    widget_nav_bar_set_active(nav_bar, SCREEN_ID_SETTINGS);
    widget_nav_bar_set_patient(nav_bar, "No Patient");

    printf("[settings] Screen created\n");
    return scr;
}

void screen_settings_destroy(void) {
    widget_alarm_banner_free(alarm_banner);
    widget_nav_bar_free(nav_bar);
    alarm_banner = NULL;
    nav_bar = NULL;
    brightness_slider = NULL;
    brightness_label = NULL;
    volume_slider = NULL;
    volume_label = NULL;
    printf("[settings] Screen destroyed\n");
}

/* ── Private helpers ───────────────────────────────────────── */

static lv_obj_t * create_section_panel(lv_obj_t *parent, const char *title) {
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(panel, lv_pct(100));
    lv_obj_set_flex_grow(panel, 1);
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

static void add_setting_row(lv_obj_t *parent, const char *label, const char *value) {
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(row, VM_PAD_NORMAL, 0);

    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_font(lbl, VM_FONT_CAPTION, 0);
    lv_obj_set_style_text_color(lbl, VM_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_width(lbl, 80);

    lv_obj_t *val = lv_label_create(row);
    lv_label_set_text(val, value);
    lv_obj_set_style_text_font(val, VM_FONT_CAPTION, 0);
    lv_obj_set_style_text_color(val, VM_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_flex_grow(val, 1);
}

static void add_switch_row(lv_obj_t *parent, const char *label, bool initial_state) {
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 30);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_gap(row, VM_PAD_NORMAL, 0);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *lbl = lv_label_create(row);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_font(lbl, VM_FONT_CAPTION, 0);
    lv_obj_set_style_text_color(lbl, VM_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_width(lbl, 80);

    lv_obj_t *sw = lv_switch_create(row);
    lv_obj_set_size(sw, 44, 22);
    lv_obj_set_style_bg_color(sw, VM_COLOR_BG_PANEL_BORDER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw, VM_COLOR_ALARM_NONE, LV_PART_INDICATOR | LV_STATE_CHECKED);
    if (initial_state) {
        lv_obj_add_state(sw, LV_STATE_CHECKED);
    }
}

static void brightness_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    int val = lv_slider_get_value(slider);
    if (brightness_label) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d%%", val);
        lv_label_set_text(brightness_label, buf);
    }
}

static void volume_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    int val = lv_slider_get_value(slider);
    if (volume_label) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%d%%", val);
        lv_label_set_text(volume_label, buf);
    }
}
