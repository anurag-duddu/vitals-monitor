/**
 * @file theme_styles.c
 * @brief Implementation of reusable lv_style_t objects
 *
 * Each style is initialized from the active vm_color_scheme_t so that
 * switching themes only requires calling vm_styles_init() again.
 * A safe_init() helper ensures styles are properly reset before
 * reinitialization.
 */

#include "theme_styles.h"

/* ================================================================
 *  Style object definitions
 * ================================================================ */

/* Base */
lv_style_t s_base;
lv_style_t s_screen;

/* Surface / Card */
lv_style_t s_card;
lv_style_t s_card_elevated;
lv_style_t s_divider;

/* Row / Column */
lv_style_t s_row;
lv_style_t s_col;

/* Button */
lv_style_t s_btn;
lv_style_t s_btn_pressed;
lv_style_t s_btn_focused;
lv_style_t s_btn_disabled;
lv_style_t s_btn_primary;
lv_style_t s_btn_danger;

/* Label */
lv_style_t s_label;
lv_style_t s_label_secondary;
lv_style_t s_label_title;
lv_style_t s_label_display;

/* Input */
lv_style_t s_input;
lv_style_t s_dropdown;
lv_style_t s_slider_main;
lv_style_t s_slider_knob;

/* Table */
lv_style_t s_table;
lv_style_t s_table_cell;

/* Chart */
lv_style_t s_chart;

/* ================================================================
 *  Internal helpers
 * ================================================================ */

/** Track whether styles have been initialized at least once */
static bool styles_initialized = false;

/**
 * Reset and reinitialize a style safely.
 * On first call lv_style_init() is sufficient; on subsequent calls
 * lv_style_reset() clears any previous properties first.
 */
static void safe_init(lv_style_t *s)
{
    if (styles_initialized) {
        lv_style_reset(s);
    }
    lv_style_init(s);
}

/* ================================================================
 *  Style initialization from scheme
 * ================================================================ */

void vm_styles_init(const vm_color_scheme_t *scheme)
{
    if (scheme == NULL) return;

    /* ---- s_base: transparent container, no border/pad ---------- */
    safe_init(&s_base);
    lv_style_set_bg_opa(&s_base, VM_OPA_TRANSPARENT);
    lv_style_set_border_width(&s_base, VM_BORDER_NONE);
    lv_style_set_pad_all(&s_base, 0);

    /* ---- s_screen: screen background -------------------------- */
    safe_init(&s_screen);
    lv_style_set_bg_color(&s_screen, lv_color_hex(scheme->background));
    lv_style_set_bg_opa(&s_screen, VM_OPA_COVER);

    /* ---- s_card: standard panel/card -------------------------- */
    safe_init(&s_card);
    lv_style_set_bg_color(&s_card, lv_color_hex(scheme->surface_container));
    lv_style_set_bg_opa(&s_card, VM_OPA_COVER);
    lv_style_set_radius(&s_card, VM_RADIUS_SM);
    lv_style_set_border_width(&s_card, VM_BORDER_THIN);
    lv_style_set_border_color(&s_card, lv_color_hex(scheme->outline));
    lv_style_set_border_opa(&s_card, VM_OPA_COVER);
    lv_style_set_pad_all(&s_card, VM_INSET_DEFAULT);
    lv_style_set_pad_gap(&s_card, VM_STACK_DEFAULT);

    /* ---- s_card_elevated: higher elevation card --------------- */
    safe_init(&s_card_elevated);
    lv_style_set_bg_color(&s_card_elevated, lv_color_hex(scheme->surface_container_high));
    lv_style_set_bg_opa(&s_card_elevated, VM_OPA_COVER);
    lv_style_set_radius(&s_card_elevated, VM_RADIUS_MD);
    lv_style_set_border_width(&s_card_elevated, VM_BORDER_THIN);
    lv_style_set_border_color(&s_card_elevated, lv_color_hex(scheme->outline));
    lv_style_set_border_opa(&s_card_elevated, VM_OPA_COVER);
    lv_style_set_pad_all(&s_card_elevated, VM_INSET_DEFAULT);
    lv_style_set_pad_gap(&s_card_elevated, VM_STACK_DEFAULT);
#if VM_FEATURE_SHADOWS
    lv_style_set_shadow_width(&s_card_elevated, VM_SHADOW_MD.width);
    lv_style_set_shadow_ofs_x(&s_card_elevated, VM_SHADOW_MD.ofs_x);
    lv_style_set_shadow_ofs_y(&s_card_elevated, VM_SHADOW_MD.ofs_y);
    lv_style_set_shadow_opa(&s_card_elevated, VM_SHADOW_MD.opa);
    lv_style_set_shadow_color(&s_card_elevated, lv_color_hex(scheme->background));
#endif

    /* ---- s_divider: horizontal separator ---------------------- */
    safe_init(&s_divider);
    lv_style_set_bg_color(&s_divider, lv_color_hex(scheme->outline));
    lv_style_set_bg_opa(&s_divider, VM_OPA_COVER);
    lv_style_set_border_width(&s_divider, VM_BORDER_NONE);
    lv_style_set_pad_all(&s_divider, 0);
    lv_style_set_radius(&s_divider, VM_RADIUS_NONE);

    /* ---- s_row: flex row, transparent ------------------------- */
    safe_init(&s_row);
    lv_style_set_bg_opa(&s_row, VM_OPA_TRANSPARENT);
    lv_style_set_border_width(&s_row, VM_BORDER_NONE);
    lv_style_set_pad_all(&s_row, 0);
    lv_style_set_layout(&s_row, LV_LAYOUT_FLEX);
    lv_style_set_flex_flow(&s_row, LV_FLEX_FLOW_ROW);

    /* ---- s_col: flex column, transparent ---------------------- */
    safe_init(&s_col);
    lv_style_set_bg_opa(&s_col, VM_OPA_TRANSPARENT);
    lv_style_set_border_width(&s_col, VM_BORDER_NONE);
    lv_style_set_pad_all(&s_col, 0);
    lv_style_set_layout(&s_col, LV_LAYOUT_FLEX);
    lv_style_set_flex_flow(&s_col, LV_FLEX_FLOW_COLUMN);

    /* ---- s_btn: standard button default ----------------------- */
    safe_init(&s_btn);
    lv_style_set_bg_color(&s_btn, lv_color_hex(scheme->surface_container_high));
    lv_style_set_bg_opa(&s_btn, VM_OPA_COVER);
    lv_style_set_radius(&s_btn, VM_RADIUS_SM);
    lv_style_set_text_color(&s_btn, lv_color_hex(scheme->on_surface));
    lv_style_set_text_font(&s_btn, VM_FONT_BODY);
    lv_style_set_pad_all(&s_btn, VM_SPACE_8);
    lv_style_set_min_height(&s_btn, VM_TOUCH_MIN);
    lv_style_set_border_width(&s_btn, VM_BORDER_THIN);
    lv_style_set_border_color(&s_btn, lv_color_hex(scheme->outline));
    lv_style_set_border_opa(&s_btn, VM_OPA_COVER);

    /* ---- s_btn_pressed: pressed state ------------------------- */
    safe_init(&s_btn_pressed);
    lv_style_set_bg_color(&s_btn_pressed, lv_color_hex(scheme->outline));
    lv_style_set_bg_opa(&s_btn_pressed, VM_OPA_COVER);
    lv_style_set_transform_width(&s_btn_pressed, -2);
    lv_style_set_transform_height(&s_btn_pressed, -2);

    /* ---- s_btn_focused: focused state ------------------------- */
    safe_init(&s_btn_focused);
    lv_style_set_outline_width(&s_btn_focused, VM_BORDER_MEDIUM);
    lv_style_set_outline_color(&s_btn_focused, lv_color_hex(scheme->primary));
    lv_style_set_outline_opa(&s_btn_focused, VM_OPA_COVER);
    lv_style_set_outline_pad(&s_btn_focused, 2);

    /* ---- s_btn_disabled: disabled state ----------------------- */
    safe_init(&s_btn_disabled);
    lv_style_set_bg_opa(&s_btn_disabled, VM_OPA_50);
    lv_style_set_text_opa(&s_btn_disabled, VM_OPA_50);

    /* ---- s_btn_primary: primary action button ----------------- */
    safe_init(&s_btn_primary);
    lv_style_set_bg_color(&s_btn_primary, lv_color_hex(scheme->primary));
    lv_style_set_bg_opa(&s_btn_primary, VM_OPA_COVER);
    lv_style_set_text_color(&s_btn_primary, lv_color_hex(scheme->on_primary));

    /* ---- s_btn_danger: destructive action --------------------- */
    safe_init(&s_btn_danger);
    lv_style_set_bg_color(&s_btn_danger, lv_color_hex(scheme->alarm_high));
    lv_style_set_bg_opa(&s_btn_danger, VM_OPA_COVER);
    lv_style_set_text_color(&s_btn_danger, lv_color_hex(scheme->on_surface));

    /* ---- s_label: default body text --------------------------- */
    safe_init(&s_label);
    lv_style_set_text_color(&s_label, lv_color_hex(scheme->on_surface));
    lv_style_set_text_font(&s_label, VM_FONT_BODY);

    /* ---- s_label_secondary: muted text ------------------------ */
    safe_init(&s_label_secondary);
    lv_style_set_text_color(&s_label_secondary, lv_color_hex(scheme->on_surface_secondary));
    lv_style_set_text_font(&s_label_secondary, VM_FONT_CAPTION);

    /* ---- s_label_title: section title ------------------------- */
    safe_init(&s_label_title);
    lv_style_set_text_color(&s_label_title, lv_color_hex(scheme->on_surface));
    lv_style_set_text_font(&s_label_title, VM_FONT_LABEL);

    /* ---- s_label_display: large value display ----------------- */
    safe_init(&s_label_display);
    lv_style_set_text_color(&s_label_display, lv_color_hex(scheme->on_surface));
    lv_style_set_text_font(&s_label_display, VM_FONT_VALUE_LARGE);

    /* ---- s_input: text/number input field ---------------------- */
    safe_init(&s_input);
    lv_style_set_bg_color(&s_input, lv_color_hex(scheme->surface_container_low));
    lv_style_set_bg_opa(&s_input, VM_OPA_COVER);
    lv_style_set_radius(&s_input, VM_RADIUS_SM);
    lv_style_set_border_width(&s_input, VM_BORDER_THIN);
    lv_style_set_border_color(&s_input, lv_color_hex(scheme->outline));
    lv_style_set_border_opa(&s_input, VM_OPA_COVER);
    lv_style_set_text_color(&s_input, lv_color_hex(scheme->on_surface));
    lv_style_set_pad_all(&s_input, VM_SPACE_8);

    /* ---- s_dropdown: dropdown selector ------------------------ */
    safe_init(&s_dropdown);
    lv_style_set_bg_color(&s_dropdown, lv_color_hex(scheme->surface_container));
    lv_style_set_bg_opa(&s_dropdown, VM_OPA_COVER);
    lv_style_set_radius(&s_dropdown, VM_RADIUS_SM);
    lv_style_set_border_width(&s_dropdown, VM_BORDER_THIN);
    lv_style_set_border_color(&s_dropdown, lv_color_hex(scheme->outline));
    lv_style_set_border_opa(&s_dropdown, VM_OPA_COVER);
    lv_style_set_text_color(&s_dropdown, lv_color_hex(scheme->on_surface));
    lv_style_set_pad_all(&s_dropdown, VM_SPACE_8);

    /* ---- s_slider_main: slider track -------------------------- */
    safe_init(&s_slider_main);
    lv_style_set_bg_color(&s_slider_main, lv_color_hex(scheme->surface_container_high));
    lv_style_set_bg_opa(&s_slider_main, VM_OPA_COVER);
    lv_style_set_radius(&s_slider_main, VM_RADIUS_FULL);

    /* ---- s_slider_knob: slider handle ------------------------- */
    safe_init(&s_slider_knob);
    lv_style_set_bg_color(&s_slider_knob, lv_color_hex(scheme->primary));
    lv_style_set_bg_opa(&s_slider_knob, VM_OPA_COVER);
    lv_style_set_pad_all(&s_slider_knob, VM_SPACE_4);

    /* ---- s_table: table container ----------------------------- */
    safe_init(&s_table);
    lv_style_set_bg_color(&s_table, lv_color_hex(scheme->surface));
    lv_style_set_bg_opa(&s_table, VM_OPA_COVER);
    lv_style_set_border_width(&s_table, VM_BORDER_THIN);
    lv_style_set_border_color(&s_table, lv_color_hex(scheme->outline));
    lv_style_set_border_opa(&s_table, VM_OPA_COVER);

    /* ---- s_table_cell: table cell ----------------------------- */
    safe_init(&s_table_cell);
    lv_style_set_border_width(&s_table_cell, VM_BORDER_THIN);
    lv_style_set_border_color(&s_table_cell, lv_color_hex(scheme->outline));
    lv_style_set_border_opa(&s_table_cell, VM_OPA_COVER);
    lv_style_set_border_side(&s_table_cell, LV_BORDER_SIDE_BOTTOM);
    lv_style_set_pad_all(&s_table_cell, VM_SPACE_4);

    /* ---- s_chart: chart area ---------------------------------- */
    safe_init(&s_chart);
    lv_style_set_bg_color(&s_chart, lv_color_hex(scheme->background));
    lv_style_set_bg_opa(&s_chart, VM_OPA_COVER);
    lv_style_set_border_width(&s_chart, VM_BORDER_NONE);
    lv_style_set_pad_all(&s_chart, 0);

    /* Mark initialization complete for subsequent reinit calls */
    styles_initialized = true;
}
