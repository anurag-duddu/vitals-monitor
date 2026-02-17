/**
 * @file theme_vitals.c
 * @brief Vitals monitor theme initialization and helpers
 */

#include "theme_vitals.h"

/* ---- Active color scheme (defaults to dark) ------------------- */
const vm_color_scheme_t *vm_active_scheme = &vm_scheme_dark;

/* ---- Scheme table for mode switching -------------------------- */
static const vm_color_scheme_t * const s_scheme_table[VM_THEME_COUNT] = {
    [VM_THEME_DARK]           = &vm_scheme_dark,
    [VM_THEME_HIGH_CONTRAST]  = &vm_scheme_high_contrast,
    [VM_THEME_DIMMED]         = &vm_scheme_dimmed,
};

static vm_theme_mode_t s_current_mode = VM_THEME_DARK;

void theme_vitals_init(void) {
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, VM_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
}

void theme_vitals_set_mode(vm_theme_mode_t mode) {
    if (mode >= VM_THEME_COUNT) return;
    s_current_mode = mode;
    vm_active_scheme = s_scheme_table[mode];
}

vm_theme_mode_t theme_vitals_get_mode(void) {
    return s_current_mode;
}

lv_color_t theme_vitals_alarm_color(vm_alarm_severity_t severity) {
    switch (severity) {
        case VM_ALARM_HIGH:   return VM_COLOR_ALARM_HIGH;
        case VM_ALARM_MEDIUM: return VM_COLOR_ALARM_MEDIUM;
        case VM_ALARM_LOW:    return VM_COLOR_ALARM_LOW;
        default:              return VM_COLOR_ALARM_NONE;
    }
}
