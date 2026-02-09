/**
 * @file widget_nav_bar.c
 * @brief Bottom navigation bar widget implementation
 *
 * Layout:
 *   ┌────────────────────────────────────────────────────────────────────┐
 *   │  Pt: No Patient       [Vitals] [Trends] [Alarms] [Patient] [Settings]  │
 *   └────────────────────────────────────────────────────────────────────┘
 */

#include "widget_nav_bar.h"
#include "theme_vitals.h"
#include "phosphor_icons.h"
#include <string.h>
#include <stdio.h>

/* Static pool */
#define MAX_NAV_BARS 2

/* Number of visible navigation buttons (excludes non-navigable screens like LOGIN) */
#define NAV_BTN_COUNT 5

struct widget_nav_bar {
    bool      in_use;
    lv_obj_t *container;
    lv_obj_t *patient_lbl;
    lv_obj_t *buttons[NAV_BTN_COUNT];
    lv_obj_t *btn_icons[NAV_BTN_COUNT];
    lv_obj_t *btn_labels[NAV_BTN_COUNT];
    screen_id_t active_screen;
};

static widget_nav_bar_t nav_pool[MAX_NAV_BARS];

/* Button labels and their corresponding screen IDs */
static const char *btn_texts[NAV_BTN_COUNT] = {
    "Vitals", "Trends", "Alarms", "Patient", "Settings"
};

/* Phosphor icon descriptors for each nav button */
static const lv_image_dsc_t *btn_icons[NAV_BTN_COUNT] = {
    &icon_heartbeat_dsc,    /* Vitals */
    &icon_chart_line_dsc,   /* Trends */
    &icon_bell_dsc,         /* Alarms */
    &icon_user_dsc,         /* Patient */
    &icon_gear_dsc          /* Settings */
};

/* ── Forward declarations ──────────────────────────────────── */

static void nav_btn_event_cb(lv_event_t *e);
static void update_active_highlight(widget_nav_bar_t *w);

/* ── Pool ──────────────────────────────────────────────────── */

static widget_nav_bar_t * pool_alloc(void) {
    for (int i = 0; i < MAX_NAV_BARS; i++) {
        if (!nav_pool[i].in_use) {
            memset(&nav_pool[i], 0, sizeof(nav_pool[i]));
            nav_pool[i].in_use = true;
            return &nav_pool[i];
        }
    }
    return NULL;
}

/* ── Public API ────────────────────────────────────────────── */

widget_nav_bar_t * widget_nav_bar_create(lv_obj_t *parent) {
    widget_nav_bar_t *w = pool_alloc();
    if (!w) return NULL;

    w->active_screen = SCREEN_ID_MAIN_VITALS;

    /* Container — full width, fixed height at bottom */
    w->container = lv_obj_create(parent);
    lv_obj_remove_flag(w->container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(w->container, VM_SCREEN_WIDTH, VM_NAV_BAR_HEIGHT);
    lv_obj_align(w->container, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_radius(w->container, 0, 0);
    lv_obj_set_style_bg_color(w->container, VM_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(w->container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(w->container, VM_COLOR_BG_PANEL_BORDER, 0);
    lv_obj_set_style_border_width(w->container, 1, 0);
    lv_obj_set_style_border_side(w->container, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_pad_hor(w->container, VM_PAD_NORMAL, 0);
    lv_obj_set_style_pad_ver(w->container, VM_PAD_SMALL, 0);

    /* Row layout */
    lv_obj_set_flex_flow(w->container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(w->container, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Patient label (left side, ~180px) */
    w->patient_lbl = lv_label_create(w->container);
    lv_label_set_text(w->patient_lbl, "No Patient");
    lv_obj_set_style_text_font(w->patient_lbl, VM_FONT_CAPTION, 0);
    lv_obj_set_style_text_color(w->patient_lbl, VM_COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_width(w->patient_lbl, 160);
    lv_label_set_long_mode(w->patient_lbl, LV_LABEL_LONG_CLIP);

    /* Navigation buttons */
    for (int i = 0; i < NAV_BTN_COUNT; i++) {
        w->buttons[i] = lv_button_create(w->container);
        lv_obj_set_height(w->buttons[i], VM_NAV_BAR_HEIGHT - VM_PAD_SMALL * 2);
        lv_obj_set_flex_grow(w->buttons[i], 1);
        lv_obj_set_style_radius(w->buttons[i], 4, 0);
        lv_obj_set_style_pad_all(w->buttons[i], VM_PAD_TINY, 0);
        lv_obj_set_style_pad_gap(w->buttons[i], VM_PAD_TINY, 0);

        /* Default (inactive) style */
        lv_obj_set_style_bg_color(w->buttons[i], VM_COLOR_BG, 0);
        lv_obj_set_style_bg_opa(w->buttons[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(w->buttons[i], 0, 0);

        /* Column layout: icon on top, text below */
        lv_obj_set_flex_flow(w->buttons[i], LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(w->buttons[i], LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        /* Icon (16x16 Phosphor A8) */
        w->btn_icons[i] = lv_image_create(w->buttons[i]);
        lv_image_set_src(w->btn_icons[i], btn_icons[i]);
        lv_obj_set_style_image_recolor(w->btn_icons[i], VM_COLOR_TEXT_SECONDARY, 0);
        lv_obj_set_style_image_recolor_opa(w->btn_icons[i], LV_OPA_COVER, 0);

        /* Button label */
        w->btn_labels[i] = lv_label_create(w->buttons[i]);
        lv_label_set_text(w->btn_labels[i], btn_texts[i]);
        lv_obj_set_style_text_font(w->btn_labels[i], VM_FONT_SMALL, 0);
        lv_obj_set_style_text_color(w->btn_labels[i], VM_COLOR_TEXT_SECONDARY, 0);

        /* Event handler — store screen ID in user data */
        lv_obj_add_event_cb(w->buttons[i], nav_btn_event_cb,
                            LV_EVENT_CLICKED, (void *)(intptr_t)i);
    }

    update_active_highlight(w);
    return w;
}

void widget_nav_bar_set_active(widget_nav_bar_t *w, screen_id_t active_screen) {
    if (!w || !w->in_use) return;
    w->active_screen = active_screen;
    update_active_highlight(w);
}

void widget_nav_bar_set_patient(widget_nav_bar_t *w, const char *patient_str) {
    if (!w || !w->in_use || !w->patient_lbl) return;
    lv_label_set_text(w->patient_lbl, patient_str);
}

lv_obj_t * widget_nav_bar_get_obj(widget_nav_bar_t *w) {
    if (!w || !w->in_use) return NULL;
    return w->container;
}

void widget_nav_bar_free(widget_nav_bar_t *w) {
    if (!w) return;
    w->in_use = false;
    w->container = NULL;
    w->patient_lbl = NULL;
    for (int i = 0; i < NAV_BTN_COUNT; i++) {
        w->buttons[i] = NULL;
        w->btn_icons[i] = NULL;
        w->btn_labels[i] = NULL;
    }
}

/* ── Private helpers ───────────────────────────────────────── */

static void nav_btn_event_cb(lv_event_t *e) {
    int screen_idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (screen_idx < 0 || screen_idx >= NAV_BTN_COUNT) return;

    screen_id_t target = (screen_id_t)screen_idx;
    screen_id_t current = screen_manager_get_active();

    if (target == current) return;  /* Already on this screen */

    if (target == SCREEN_ID_MAIN_VITALS) {
        screen_manager_go_home();
    } else {
        /* Go home first then push target, so back goes to home */
        screen_manager_go_home();
        screen_manager_push(target);
    }
}

static void update_active_highlight(widget_nav_bar_t *w) {
    for (int i = 0; i < NAV_BTN_COUNT; i++) {
        if ((int)w->active_screen == i) {
            /* Active button — highlighted */
            lv_obj_set_style_bg_color(w->buttons[i], VM_COLOR_BG_PANEL_BORDER, 0);
            lv_obj_set_style_text_color(w->btn_labels[i], VM_COLOR_TEXT_PRIMARY, 0);
            lv_obj_set_style_image_recolor(w->btn_icons[i], VM_COLOR_TEXT_PRIMARY, 0);
        } else {
            /* Inactive button */
            lv_obj_set_style_bg_color(w->buttons[i], VM_COLOR_BG, 0);
            lv_obj_set_style_text_color(w->btn_labels[i], VM_COLOR_TEXT_SECONDARY, 0);
            lv_obj_set_style_image_recolor(w->btn_icons[i], VM_COLOR_TEXT_SECONDARY, 0);
        }
    }
}
