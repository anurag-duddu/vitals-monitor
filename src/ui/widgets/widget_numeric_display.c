/**
 * @file widget_numeric_display.c
 * @brief Vital sign numeric display widget implementation
 *
 * Compact 2-row layout:
 *   ┌──────────────────────┐
 *   │  HR            bpm   │  ← top row: label (left) + unit (right)
 *   │         72           │  ← value (centered, large font)
 *   └──────────────────────┘
 */

#include "widget_numeric_display.h"
#include <string.h>
#include <stdio.h>

/* Static pool — no dynamic allocation in critical paths */
#define MAX_NUMERIC_DISPLAYS 10

struct widget_numeric_display {
    bool     in_use;
    lv_obj_t *container;
    lv_obj_t *top_row;
    lv_obj_t *icon_obj;
    lv_obj_t *label_obj;
    lv_obj_t *value_obj;
    lv_obj_t *unit_obj;
    lv_color_t param_color;
    vm_alarm_severity_t alarm_state;
};

static widget_numeric_display_t pool[MAX_NUMERIC_DISPLAYS];

/* ── Pool management ───────────────────────────────────────── */

static widget_numeric_display_t * pool_alloc(void) {
    for (int i = 0; i < MAX_NUMERIC_DISPLAYS; i++) {
        if (!pool[i].in_use) {
            memset(&pool[i], 0, sizeof(pool[i]));
            pool[i].in_use = true;
            return &pool[i];
        }
    }
    printf("[widget_numeric] Pool exhausted!\n");
    return NULL;
}

/* ── Public API ────────────────────────────────────────────── */

widget_numeric_display_t * widget_numeric_display_create(
    lv_obj_t *parent,
    const char *label_text,
    const char *unit_text,
    lv_color_t color,
    numeric_display_size_t size,
    const lv_image_dsc_t *icon)
{
    widget_numeric_display_t *w = pool_alloc();
    if (!w) return NULL;

    w->param_color = color;
    w->alarm_state = VM_ALARM_NONE;

    /* Select fonts based on size variant */
    const lv_font_t *value_font;
    const lv_font_t *label_font;

    switch (size) {
        case NUMERIC_SIZE_LARGE:
            value_font = VM_FONT_VALUE_LARGE;   /* 48pt */
            label_font = VM_FONT_LABEL;          /* 20pt */
            break;
        case NUMERIC_SIZE_MEDIUM:
            value_font = VM_FONT_VALUE_MEDIUM;   /* 32pt */
            label_font = VM_FONT_BODY;           /* 16pt */
            break;
        case NUMERIC_SIZE_SMALL:
        default:
            value_font = VM_FONT_LABEL;          /* 20pt */
            label_font = VM_FONT_CAPTION;        /* 14pt */
            break;
    }

    /* ── Container ────────────────────────────────────────── */
    w->container = lv_obj_create(parent);
    lv_obj_remove_flag(w->container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(w->container, VM_COLOR_BG_PANEL, 0);
    lv_obj_set_style_bg_opa(w->container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(w->container, VM_COLOR_BG_PANEL_BORDER, 0);
    lv_obj_set_style_border_width(w->container, 2, 0);
    lv_obj_set_style_radius(w->container, 4, 0);
    lv_obj_set_style_pad_all(w->container, VM_PAD_SMALL, 0);
    lv_obj_set_style_pad_gap(w->container, VM_PAD_TINY, 0);

    /* Size depends on variant */
    lv_obj_set_width(w->container, lv_pct(100));
    switch (size) {
        case NUMERIC_SIZE_LARGE:
            lv_obj_set_flex_grow(w->container, 3);
            break;
        case NUMERIC_SIZE_MEDIUM:
            lv_obj_set_flex_grow(w->container, 2);
            break;
        case NUMERIC_SIZE_SMALL:
            lv_obj_set_flex_grow(w->container, 1);
            lv_obj_set_width(w->container, lv_pct(50));
            break;
    }

    /* Column flex layout: top_row + value */
    lv_obj_set_flex_flow(w->container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(w->container, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* ── Top row: [icon] label (left) + unit (right) ──── */
    w->top_row = lv_obj_create(w->container);
    lv_obj_remove_flag(w->top_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_width(w->top_row, lv_pct(100));
    lv_obj_set_height(w->top_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(w->top_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(w->top_row, 0, 0);
    lv_obj_set_style_pad_all(w->top_row, 0, 0);
    lv_obj_set_style_pad_gap(w->top_row, VM_PAD_TINY, 0);
    lv_obj_set_flex_flow(w->top_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(w->top_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Left group: icon + label (grouped so SPACE_BETWEEN pushes unit right) */
    lv_obj_t *left_group = lv_obj_create(w->top_row);
    lv_obj_remove_flag(left_group, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(left_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(left_group, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(left_group, 0, 0);
    lv_obj_set_style_pad_all(left_group, 0, 0);
    lv_obj_set_style_pad_gap(left_group, VM_PAD_TINY, 0);
    lv_obj_set_flex_flow(left_group, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(left_group, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Icon (optional, 16x16 Phosphor A8) */
    if (icon) {
        w->icon_obj = lv_image_create(left_group);
        lv_image_set_src(w->icon_obj, icon);
        lv_obj_set_style_image_recolor(w->icon_obj, color, 0);
        lv_obj_set_style_image_recolor_opa(w->icon_obj, LV_OPA_COVER, 0);
    }

    /* Label */
    w->label_obj = lv_label_create(left_group);
    lv_label_set_text(w->label_obj, label_text);
    lv_obj_set_style_text_font(w->label_obj, label_font, 0);
    lv_obj_set_style_text_color(w->label_obj, color, 0);

    /* Unit (right side, smaller, secondary color) */
    w->unit_obj = lv_label_create(w->top_row);
    lv_label_set_text(w->unit_obj, unit_text);
    lv_obj_set_style_text_font(w->unit_obj, VM_FONT_UNIT, 0);
    lv_obj_set_style_text_color(w->unit_obj, VM_COLOR_TEXT_SECONDARY, 0);

    /* ── Value (large centered number, fills remaining space) ── */
    w->value_obj = lv_label_create(w->container);
    lv_label_set_text(w->value_obj, "---");
    lv_obj_set_style_text_font(w->value_obj, value_font, 0);
    lv_obj_set_style_text_color(w->value_obj, color, 0);
    lv_obj_set_style_text_align(w->value_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(w->value_obj, lv_pct(100));
    lv_obj_set_flex_grow(w->value_obj, 1);

    return w;
}

void widget_numeric_display_set_value(widget_numeric_display_t *w, const char *value_str) {
    if (!w || !w->in_use || !w->value_obj) return;
    lv_label_set_text(w->value_obj, value_str);
}

void widget_numeric_display_set_alarm(widget_numeric_display_t *w, vm_alarm_severity_t severity) {
    if (!w || !w->in_use || !w->container) return;

    w->alarm_state = severity;

    if (severity == VM_ALARM_NONE) {
        lv_obj_set_style_border_color(w->container, VM_COLOR_BG_PANEL_BORDER, 0);
        lv_obj_set_style_border_width(w->container, 2, 0);
    } else {
        lv_color_t alarm_color = theme_vitals_alarm_color(severity);
        lv_obj_set_style_border_color(w->container, alarm_color, 0);
        lv_obj_set_style_border_width(w->container, 3, 0);
    }
}

lv_obj_t * widget_numeric_display_get_obj(widget_numeric_display_t *w) {
    if (!w || !w->in_use) return NULL;
    return w->container;
}

void widget_numeric_display_free(widget_numeric_display_t *w) {
    if (!w) return;
    w->in_use = false;
    w->container = NULL;
    w->top_row = NULL;
    w->icon_obj = NULL;
    w->label_obj = NULL;
    w->value_obj = NULL;
    w->unit_obj = NULL;
}
