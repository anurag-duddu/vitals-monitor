/**
 * @file theme_styles.h
 * @brief Reusable lv_style_t objects for the vitals monitor UI
 *
 * All styles are initialized from the active vm_color_scheme_t via
 * vm_styles_init().  When the scheme changes (e.g. dark -> high-contrast),
 * call vm_styles_init() again to reinitialize every style in-place.
 *
 * Widgets apply these styles with lv_obj_add_style() instead of
 * setting properties directly, ensuring visual consistency and
 * enabling run-time theme switching with zero widget changes.
 */

#ifndef THEME_STYLES_H
#define THEME_STYLES_H

#include "lvgl.h"
#include "design_tokens.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 *  Base
 * ================================================================ */
extern lv_style_t s_base;            /**< Transparent container, no border/pad */
extern lv_style_t s_screen;          /**< Screen background */

/* ================================================================
 *  Surface / Card
 * ================================================================ */
extern lv_style_t s_card;            /**< Standard panel/card */
extern lv_style_t s_card_elevated;   /**< Higher elevation card */
extern lv_style_t s_divider;         /**< Horizontal separator */

/* ================================================================
 *  Row / Column layout helpers
 * ================================================================ */
extern lv_style_t s_row;             /**< Flex row, transparent */
extern lv_style_t s_col;             /**< Flex column, transparent */

/* ================================================================
 *  Button
 * ================================================================ */
extern lv_style_t s_btn;             /**< Standard button default */
extern lv_style_t s_btn_pressed;     /**< Button pressed state */
extern lv_style_t s_btn_focused;     /**< Button focused state */
extern lv_style_t s_btn_disabled;    /**< Button disabled state */
extern lv_style_t s_btn_primary;     /**< Primary action button */
extern lv_style_t s_btn_danger;      /**< Destructive action */

/* ================================================================
 *  Label
 * ================================================================ */
extern lv_style_t s_label;           /**< Default body text */
extern lv_style_t s_label_secondary; /**< Secondary / muted text */
extern lv_style_t s_label_title;     /**< Section title */
extern lv_style_t s_label_display;   /**< Large value display */

/* ================================================================
 *  Input
 * ================================================================ */
extern lv_style_t s_input;           /**< Text/number input field */
extern lv_style_t s_dropdown;        /**< Dropdown selector */
extern lv_style_t s_slider_main;     /**< Slider track */
extern lv_style_t s_slider_knob;     /**< Slider handle */

/* ================================================================
 *  Table
 * ================================================================ */
extern lv_style_t s_table;           /**< Table container */
extern lv_style_t s_table_cell;      /**< Table cell */

/* ================================================================
 *  Chart
 * ================================================================ */
extern lv_style_t s_chart;           /**< Chart area */

/* ================================================================
 *  API
 * ================================================================ */

/**
 * Initialize (or reinitialize) all style objects from the given scheme.
 *
 * Safe to call repeatedly -- existing style properties are reset before
 * new values are applied, so widgets that reference these styles see
 * updated values immediately.
 *
 * @param scheme  pointer to the active color scheme
 */
void vm_styles_init(const vm_color_scheme_t *scheme);

#ifdef __cplusplus
}
#endif

#endif /* THEME_STYLES_H */
