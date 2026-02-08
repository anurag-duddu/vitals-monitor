/**
 * @file theme_vitals.c
 * @brief Vitals monitor theme initialization and helpers
 */

#include "theme_vitals.h"

void theme_vitals_init(void) {
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, VM_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
}

lv_color_t theme_vitals_alarm_color(vm_alarm_severity_t severity) {
    switch (severity) {
        case VM_ALARM_HIGH:   return VM_COLOR_ALARM_HIGH;
        case VM_ALARM_MEDIUM: return VM_COLOR_ALARM_MEDIUM;
        case VM_ALARM_LOW:    return VM_COLOR_ALARM_LOW;
        default:              return VM_COLOR_ALARM_NONE;
    }
}
