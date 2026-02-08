/**
 * @file widget_alarm_banner.c
 * @brief Top alarm bar widget implementation
 *
 * Layout:
 *   ┌──────────────────────────────────────────────────────────────┐
 *   │ ▲ HIGH: HR > 120 bpm (132)        [ACK]           14:32    │
 *   └──────────────────────────────────────────────────────────────┘
 *
 * Flash rates (IEC 60601-1-8):
 *   High:   ~2 Hz   (250ms toggle period)
 *   Medium: ~0.5 Hz (1000ms toggle period)
 *   Low:    steady color (no flash)
 */

#include "widget_alarm_banner.h"
#include <string.h>
#include <stdio.h>

/* Static pool (typically only 1 per screen, allow 4 for dual-patient) */
#define MAX_ALARM_BANNERS 4

struct widget_alarm_banner {
    bool      in_use;
    lv_obj_t *container;
    lv_obj_t *icon_lbl;       /* Alarm icon/indicator */
    lv_obj_t *message_lbl;    /* Alarm message text */
    lv_obj_t *time_lbl;       /* Clock (right-aligned) */
    vm_alarm_severity_t severity;
    lv_timer_t *flash_timer;
    bool flash_state;         /* Toggle for flashing */
};

static widget_alarm_banner_t banner_pool[MAX_ALARM_BANNERS];

/* ── Forward declarations ──────────────────────────────────── */

static void flash_timer_cb(lv_timer_t *timer);
static void update_visual(widget_alarm_banner_t *w);

/* ── Pool ──────────────────────────────────────────────────── */

static widget_alarm_banner_t * pool_alloc(void) {
    for (int i = 0; i < MAX_ALARM_BANNERS; i++) {
        if (!banner_pool[i].in_use) {
            memset(&banner_pool[i], 0, sizeof(banner_pool[i]));
            banner_pool[i].in_use = true;
            return &banner_pool[i];
        }
    }
    return NULL;
}

/* ── Public API ────────────────────────────────────────────── */

widget_alarm_banner_t * widget_alarm_banner_create(lv_obj_t *parent) {
    widget_alarm_banner_t *w = pool_alloc();
    if (!w) return NULL;

    w->severity = VM_ALARM_NONE;
    w->flash_state = false;

    /* Container — full width, fixed height at top */
    w->container = lv_obj_create(parent);
    lv_obj_remove_flag(w->container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(w->container, VM_SCREEN_WIDTH, VM_ALARM_BAR_HEIGHT);
    lv_obj_set_pos(w->container, 0, 0);
    lv_obj_set_style_radius(w->container, 0, 0);
    lv_obj_set_style_border_width(w->container, 0, 0);
    lv_obj_set_style_pad_hor(w->container, VM_PAD_NORMAL, 0);
    lv_obj_set_style_pad_ver(w->container, VM_PAD_TINY, 0);

    /* Row layout */
    lv_obj_set_flex_flow(w->container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(w->container, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Alarm icon */
    w->icon_lbl = lv_label_create(w->container);
    lv_label_set_text(w->icon_lbl, LV_SYMBOL_OK);
    lv_obj_set_style_text_font(w->icon_lbl, VM_FONT_BODY, 0);
    lv_obj_set_style_text_color(w->icon_lbl, VM_COLOR_ALARM_NONE, 0);

    /* Message label — grows to fill space */
    w->message_lbl = lv_label_create(w->container);
    lv_label_set_text(w->message_lbl, "  No Alarms");
    lv_obj_set_style_text_font(w->message_lbl, VM_FONT_CAPTION, 0);
    lv_obj_set_style_text_color(w->message_lbl, VM_COLOR_ALARM_NONE, 0);
    lv_obj_set_flex_grow(w->message_lbl, 1);

    /* Time label — right-aligned */
    w->time_lbl = lv_label_create(w->container);
    lv_label_set_text(w->time_lbl, "--:--");
    lv_obj_set_style_text_font(w->time_lbl, VM_FONT_CAPTION, 0);
    lv_obj_set_style_text_color(w->time_lbl, VM_COLOR_TEXT_SECONDARY, 0);

    /* Set initial visual state */
    update_visual(w);

    return w;
}

void widget_alarm_banner_set(widget_alarm_banner_t *w,
                              vm_alarm_severity_t severity,
                              const char *message) {
    if (!w || !w->in_use) return;

    w->severity = severity;
    w->flash_state = false;

    /* Update icon */
    if (severity == VM_ALARM_HIGH) {
        lv_label_set_text(w->icon_lbl, LV_SYMBOL_WARNING);
    } else if (severity == VM_ALARM_MEDIUM) {
        lv_label_set_text(w->icon_lbl, LV_SYMBOL_WARNING);
    } else if (severity == VM_ALARM_LOW) {
        lv_label_set_text(w->icon_lbl, LV_SYMBOL_BELL);
    } else {
        lv_label_set_text(w->icon_lbl, LV_SYMBOL_OK);
    }

    /* Update message */
    if (message) {
        char buf[128];
        snprintf(buf, sizeof(buf), "  %s", message);
        lv_label_set_text(w->message_lbl, buf);
    }

    /* Manage flash timer */
    if (w->flash_timer) {
        lv_timer_delete(w->flash_timer);
        w->flash_timer = NULL;
    }

    if (severity == VM_ALARM_HIGH) {
        /* ~2 Hz flash (250ms toggle) */
        w->flash_timer = lv_timer_create(flash_timer_cb, 250, w);
    } else if (severity == VM_ALARM_MEDIUM) {
        /* ~0.5 Hz flash (1000ms toggle) */
        w->flash_timer = lv_timer_create(flash_timer_cb, 1000, w);
    }

    update_visual(w);
}

void widget_alarm_banner_clear(widget_alarm_banner_t *w) {
    if (!w || !w->in_use) return;

    w->severity = VM_ALARM_NONE;
    w->flash_state = false;

    if (w->flash_timer) {
        lv_timer_delete(w->flash_timer);
        w->flash_timer = NULL;
    }

    lv_label_set_text(w->icon_lbl, LV_SYMBOL_OK);
    lv_label_set_text(w->message_lbl, "  No Alarms");

    update_visual(w);
}

void widget_alarm_banner_set_time(widget_alarm_banner_t *w, const char *time_str) {
    if (!w || !w->in_use || !w->time_lbl) return;
    lv_label_set_text(w->time_lbl, time_str);
}

lv_obj_t * widget_alarm_banner_get_obj(widget_alarm_banner_t *w) {
    if (!w || !w->in_use) return NULL;
    return w->container;
}

void widget_alarm_banner_free(widget_alarm_banner_t *w) {
    if (!w) return;
    if (w->flash_timer) {
        lv_timer_delete(w->flash_timer);
        w->flash_timer = NULL;
    }
    w->in_use = false;
    w->container = NULL;
}

/* ── Private helpers ───────────────────────────────────────── */

static void flash_timer_cb(lv_timer_t *timer) {
    widget_alarm_banner_t *w = (widget_alarm_banner_t *)lv_timer_get_user_data(timer);
    if (!w || !w->in_use) return;

    w->flash_state = !w->flash_state;
    update_visual(w);
}

static void update_visual(widget_alarm_banner_t *w) {
    lv_color_t bg_color;
    lv_color_t text_color;

    switch (w->severity) {
        case VM_ALARM_HIGH:
            bg_color = w->flash_state ? VM_COLOR_BG : VM_COLOR_ALARM_HIGH;
            text_color = w->flash_state ? VM_COLOR_ALARM_HIGH : lv_color_hex(0xFFFFFF);
            break;
        case VM_ALARM_MEDIUM:
            bg_color = w->flash_state ? VM_COLOR_BG : VM_COLOR_ALARM_MEDIUM;
            text_color = w->flash_state ? VM_COLOR_ALARM_MEDIUM : lv_color_hex(0x000000);
            break;
        case VM_ALARM_LOW:
            bg_color = VM_COLOR_BG_PANEL;
            text_color = VM_COLOR_ALARM_LOW;
            break;
        default:
            bg_color = VM_COLOR_BG_PANEL;
            text_color = VM_COLOR_ALARM_NONE;
            break;
    }

    lv_obj_set_style_bg_color(w->container, bg_color, 0);
    lv_obj_set_style_bg_opa(w->container, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(w->icon_lbl, text_color, 0);
    lv_obj_set_style_text_color(w->message_lbl, text_color, 0);
}
