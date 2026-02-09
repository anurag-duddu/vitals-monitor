/**
 * @file screen_login.c
 * @brief PIN-entry login screen — full-screen authentication gate
 *
 * Layout (800x480, no nav bar or alarm banner):
 *   ┌──────────────────────────────────────────────────────────────┐
 *   │                                                              │
 *   │              Bedside Vitals Monitor                          │
 *   │                VM-800 Series                                 │
 *   │                                                              │
 *   │           ┌────────────────────────┐                         │
 *   │           │  Username: [dropdown]  │                         │
 *   │           │  PIN: [● ● ● ●    ]   │                         │
 *   │           │                        │                         │
 *   │           │  [1] [2] [3]           │                         │
 *   │           │  [4] [5] [6]           │                         │
 *   │           │  [7] [8] [9]           │                         │
 *   │           │  [C] [0] [OK]          │                         │
 *   │           │                        │                         │
 *   │           │  Status: Ready         │                         │
 *   │           └────────────────────────┘                         │
 *   │                                                              │
 *   └──────────────────────────────────────────────────────────────┘
 */

#include "screen_login.h"
#include "screen_manager.h"
#include "auth_manager.h"
#include "theme_vitals.h"
#include <stdio.h>
#include <string.h>

/* ── Constants ────────────────────────────────────────────── */

#define PIN_MAX_DIGITS      6
#define BTN_WIDTH           VM_SCALE_W(60)
#define BTN_HEIGHT          VM_SCALE_H(50)
#define BTN_GAP             VM_SCALE_W(8)
#define CARD_WIDTH          VM_SCALE_W(320)
#define ERROR_DISPLAY_MS    2000

/* ── Module state ─────────────────────────────────────────── */

static char         pin_buf[PIN_MAX_DIGITS + 1];  /* NUL-terminated */
static int          pin_cursor;
static lv_obj_t    *pin_label;
static lv_obj_t    *status_label;
static lv_obj_t    *dropdown;
static lv_timer_t  *error_timer;

/* ── Forward declarations ─────────────────────────────────── */

static void build_pin_display(void);
static void on_numpad_click(lv_event_t *e);
static void on_clear_click(lv_event_t *e);
static void on_ok_click(lv_event_t *e);
static void error_timer_cb(lv_timer_t *timer);
static void reset_pin(void);
static lv_obj_t * create_numpad_btn(lv_obj_t *parent, const char *text,
                                     lv_color_t bg_color,
                                     lv_event_cb_t cb);

/* ── Public API ───────────────────────────────────────────── */

lv_obj_t * screen_login_create(void) {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, VM_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* Reset state */
    reset_pin();
    error_timer = NULL;

    /* ── Title area (top of screen) ───────────────────────── */
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Bedside Vitals Monitor");
    lv_obj_set_style_text_font(title, VM_FONT_VALUE_MEDIUM, 0);
    lv_obj_set_style_text_color(title, VM_COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, VM_SCALE_H(24));

    lv_obj_t *subtitle = lv_label_create(scr);
    lv_label_set_text(subtitle, "VM-800 Series");
    lv_obj_set_style_text_font(subtitle, VM_FONT_BODY, 0);
    lv_obj_set_style_text_color(subtitle, VM_COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, VM_SCALE_H(60));

    /* ── Login card (centered) ────────────────────────────── */
    lv_obj_t *card = lv_obj_create(scr);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(card, CARD_WIDTH, VM_SCALE_H(370));
    lv_obj_align(card, LV_ALIGN_CENTER, 0, VM_SCALE_H(20));
    lv_obj_set_style_bg_color(card, VM_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, VM_COLOR_BG_PANEL_BORDER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 8, 0);
    lv_obj_set_style_pad_all(card, VM_PAD_NORMAL, 0);
    lv_obj_set_style_pad_gap(card, VM_PAD_SMALL, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_flex_cross_place(card, LV_FLEX_ALIGN_CENTER, 0);

    /* ── Username row ─────────────────────────────────────── */
    lv_obj_t *user_row = lv_obj_create(card);
    lv_obj_remove_flag(user_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(user_row, lv_pct(100));
    lv_obj_set_height(user_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(user_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(user_row, 0, 0);
    lv_obj_set_style_pad_all(user_row, 0, 0);
    lv_obj_set_flex_flow(user_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_flex_cross_place(user_row, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_gap(user_row, VM_PAD_NORMAL, 0);

    lv_obj_t *user_lbl = lv_label_create(user_row);
    lv_label_set_text(user_lbl, "Username:");
    lv_obj_set_style_text_font(user_lbl, VM_FONT_BODY, 0);
    lv_obj_set_style_text_color(user_lbl, VM_COLOR_TEXT_SECONDARY, 0);

    dropdown = lv_dropdown_create(user_row);
    lv_dropdown_set_options(dropdown, "admin\ndoctor\nnurse\ntech");
    lv_obj_set_flex_grow(dropdown, 1);
    lv_obj_set_style_text_font(dropdown, VM_FONT_BODY, 0);
    lv_obj_set_style_bg_color(dropdown, VM_COLOR_BG, 0);
    lv_obj_set_style_text_color(dropdown, VM_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_border_color(dropdown, VM_COLOR_BG_PANEL_BORDER, 0);

    /* Style the dropdown list when opened */
    lv_obj_set_style_bg_color(dropdown, VM_COLOR_BG, LV_PART_INDICATOR);
    lv_obj_t *dd_list = lv_dropdown_get_list(dropdown);
    if(dd_list) {
        lv_obj_set_style_bg_color(dd_list, VM_COLOR_BG_PANEL, 0);
        lv_obj_set_style_text_color(dd_list, VM_COLOR_TEXT_PRIMARY, 0);
        lv_obj_set_style_text_font(dd_list, VM_FONT_BODY, 0);
    }

    /* ── PIN display row ──────────────────────────────────── */
    lv_obj_t *pin_row = lv_obj_create(card);
    lv_obj_remove_flag(pin_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(pin_row, lv_pct(100));
    lv_obj_set_height(pin_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(pin_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(pin_row, 0, 0);
    lv_obj_set_style_pad_all(pin_row, 0, 0);
    lv_obj_set_flex_flow(pin_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_flex_cross_place(pin_row, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_gap(pin_row, VM_PAD_NORMAL, 0);

    lv_obj_t *pin_lbl = lv_label_create(pin_row);
    lv_label_set_text(pin_lbl, "PIN:");
    lv_obj_set_style_text_font(pin_lbl, VM_FONT_BODY, 0);
    lv_obj_set_style_text_color(pin_lbl, VM_COLOR_TEXT_SECONDARY, 0);

    pin_label = lv_label_create(pin_row);
    lv_obj_set_style_text_font(pin_label, VM_FONT_LABEL, 0);
    lv_obj_set_style_text_color(pin_label, VM_COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_flex_grow(pin_label, 1);
    build_pin_display();

    /* ── Number pad ───────────────────────────────────────── */
    lv_obj_t *pad = lv_obj_create(card);
    lv_obj_remove_flag(pad, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(pad, 3 * BTN_WIDTH + 2 * BTN_GAP + 2 * VM_PAD_SMALL);
    lv_obj_set_height(pad, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(pad, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(pad, 0, 0);
    lv_obj_set_style_pad_all(pad, VM_PAD_SMALL, 0);
    lv_obj_set_style_pad_gap(pad, BTN_GAP, 0);
    lv_obj_set_style_pad_row(pad, BTN_GAP, 0);
    lv_obj_set_style_pad_column(pad, BTN_GAP, 0);
    lv_obj_set_flex_flow(pad, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_flex_main_place(pad, LV_FLEX_ALIGN_CENTER, 0);

    /* Rows 1-3: digits 1-9 */
    static const char *digits[] = {
        "1", "2", "3",
        "4", "5", "6",
        "7", "8", "9"
    };
    for(int i = 0; i < 9; i++) {
        create_numpad_btn(pad, digits[i], VM_COLOR_BG_PANEL, on_numpad_click);
    }

    /* Bottom row: C, 0, OK */
    create_numpad_btn(pad, "C",  VM_COLOR_ALARM_MEDIUM, on_clear_click);
    create_numpad_btn(pad, "0",  VM_COLOR_BG_PANEL,     on_numpad_click);
    create_numpad_btn(pad, "OK", VM_COLOR_ALARM_NONE,   on_ok_click);

    /* ── Status label ─────────────────────────────────────── */
    status_label = lv_label_create(card);
    lv_label_set_text(status_label, "Enter PIN");
    lv_obj_set_style_text_font(status_label, VM_FONT_CAPTION, 0);
    lv_obj_set_style_text_color(status_label, VM_COLOR_TEXT_SECONDARY, 0);

    return scr;
}

void screen_login_destroy(void) {
    if(error_timer) {
        lv_timer_delete(error_timer);
        error_timer = NULL;
    }
    pin_label    = NULL;
    status_label = NULL;
    dropdown     = NULL;
    pin_cursor   = 0;
    memset(pin_buf, 0, sizeof(pin_buf));
}

/* ── Private helpers ──────────────────────────────────────── */

static void reset_pin(void) {
    memset(pin_buf, 0, sizeof(pin_buf));
    pin_cursor = 0;
}

/**
 * Build the visual PIN display string.
 * Shows a filled dot for each entered digit and an underscore placeholder
 * for remaining positions up to PIN_MAX_DIGITS.
 */
static void build_pin_display(void) {
    if(!pin_label) return;

    /* Each dot is the UTF-8 sequence for U+25CF (3 bytes) plus a space,
     * each underscore is 1 byte plus a space.  Generous buffer. */
    static char display[PIN_MAX_DIGITS * 4 + 1];
    int pos = 0;

    for(int i = 0; i < PIN_MAX_DIGITS; i++) {
        if(i < pin_cursor) {
            /* ● (U+25CF) — 3 UTF-8 bytes */
            display[pos++] = (char)0xE2;
            display[pos++] = (char)0x97;
            display[pos++] = (char)0x8F;
        } else {
            display[pos++] = '_';
        }
        if(i < PIN_MAX_DIGITS - 1) {
            display[pos++] = ' ';
        }
    }
    display[pos] = '\0';

    lv_label_set_text(pin_label, display);
}

/** Create a single number pad button with consistent styling. */
static lv_obj_t * create_numpad_btn(lv_obj_t *parent, const char *text,
                                     lv_color_t bg_color,
                                     lv_event_cb_t cb) {
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, BTN_WIDTH, BTN_HEIGHT);
    lv_obj_set_style_bg_color(btn, bg_color, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(btn, VM_COLOR_BG_PANEL_BORDER, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn, 4, 0);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, VM_FONT_VALUE_MEDIUM, 0);
    lv_obj_set_style_text_color(lbl, VM_COLOR_TEXT_PRIMARY, 0);
    lv_obj_center(lbl);

    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    return btn;
}

/** Handler for digit buttons 0-9. */
static void on_numpad_click(lv_event_t *e) {
    if(pin_cursor >= PIN_MAX_DIGITS) return;

    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *lbl = lv_obj_get_child(btn, 0);
    const char *digit = lv_label_get_text(lbl);

    pin_buf[pin_cursor] = digit[0];
    pin_cursor++;
    pin_buf[pin_cursor] = '\0';

    build_pin_display();

    /* Reset status if it was showing an error */
    lv_obj_set_style_text_color(status_label, VM_COLOR_TEXT_SECONDARY, 0);
    lv_label_set_text(status_label, "Enter PIN");
}

/** Handler for the C (clear) button. */
static void on_clear_click(lv_event_t *e) {
    (void)e;
    reset_pin();
    build_pin_display();
    lv_obj_set_style_text_color(status_label, VM_COLOR_TEXT_SECONDARY, 0);
    lv_label_set_text(status_label, "Enter PIN");
}

/** Timer callback: clears error message and resets PIN display. */
static void error_timer_cb(lv_timer_t *timer) {
    (void)timer;
    reset_pin();
    build_pin_display();
    lv_obj_set_style_text_color(status_label, VM_COLOR_TEXT_SECONDARY, 0);
    lv_label_set_text(status_label, "Enter PIN");
    error_timer = NULL;
}

/** Handler for the OK (login) button. */
static void on_ok_click(lv_event_t *e) {
    (void)e;

    if(pin_cursor == 0) return;

    /* Get selected username from dropdown */
    char username[32];
    lv_dropdown_get_selected_str(dropdown, username, sizeof(username));

    /* Attempt authentication */
    if(auth_manager_login(username, pin_buf)) {
        /* Success */
        lv_obj_set_style_text_color(status_label, VM_COLOR_ALARM_NONE, 0);
        lv_label_set_text(status_label, "Login successful!");
        screen_manager_push(SCREEN_ID_MAIN_VITALS);
    } else {
        /* Failure — show error for ERROR_DISPLAY_MS, then reset */
        lv_obj_set_style_text_color(status_label, VM_COLOR_ALARM_HIGH, 0);
        lv_label_set_text(status_label, "Invalid PIN");

        /* Cancel any existing error timer before starting a new one */
        if(error_timer) {
            lv_timer_delete(error_timer);
        }
        error_timer = lv_timer_create(error_timer_cb, ERROR_DISPLAY_MS, NULL);
        lv_timer_set_repeat_count(error_timer, 1);
    }
}
