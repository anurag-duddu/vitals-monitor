/**
 * @file theme_vitals.h
 * @brief Backward-compatible wrapper over design_tokens.h
 *
 * This header re-exports every VM_COLOR_*, layout, font, and spacing
 * macro the codebase already uses, but resolves them through the
 * active color-scheme pointer (vm_active_scheme) defined in
 * design_tokens.h.  Existing source files continue to compile
 * unchanged.
 *
 * Color assignments follow IEC 60601-1-8 and medical device
 * industry conventions.
 */

#ifndef THEME_VITALS_H
#define THEME_VITALS_H

#include "design_tokens.h"   /* full token hierarchy + vm_active_scheme */

/* ================================================================
 *  Backward-compatible VM_COLOR_* macros
 *
 *  Each resolves through the active color scheme pointer so that
 *  switching themes at run-time changes all colors automatically.
 * ================================================================ */

/* ---- Backgrounds / surfaces ----------------------------------- */
#define VM_COLOR_BG               lv_color_hex(vm_active_scheme->background)
#define VM_COLOR_BG_PANEL         lv_color_hex(vm_active_scheme->surface)
#define VM_COLOR_BG_PANEL_BORDER  lv_color_hex(vm_active_scheme->outline)

/* ---- Alarm severity colors (IEC 60601-1-8) -------------------- */
#define VM_COLOR_ALARM_HIGH       lv_color_hex(vm_active_scheme->alarm_high)
#define VM_COLOR_ALARM_MEDIUM     lv_color_hex(vm_active_scheme->alarm_medium)
#define VM_COLOR_ALARM_LOW        lv_color_hex(vm_active_scheme->alarm_low)
#define VM_COLOR_ALARM_NONE       lv_color_hex(vm_active_scheme->alarm_none)

/* ---- Vital-sign parameter colors ------------------------------ */
#define VM_COLOR_HR               lv_color_hex(vm_active_scheme->param_hr)
#define VM_COLOR_SPO2             lv_color_hex(vm_active_scheme->param_spo2)
#define VM_COLOR_NIBP             lv_color_hex(vm_active_scheme->param_nibp)
#define VM_COLOR_TEMP             lv_color_hex(vm_active_scheme->param_temp)
#define VM_COLOR_RR               lv_color_hex(vm_active_scheme->param_rr)

/* ---- Text colors ---------------------------------------------- */
#define VM_COLOR_TEXT_PRIMARY     lv_color_hex(vm_active_scheme->on_surface)
#define VM_COLOR_TEXT_SECONDARY   lv_color_hex(vm_active_scheme->on_surface_secondary)
#define VM_COLOR_TEXT_DISABLED    lv_color_hex(vm_active_scheme->on_surface_disabled)

/* ================================================================
 *  Alarm Severity Enum
 * ================================================================ */

typedef enum {
    VM_ALARM_NONE = 0,
    VM_ALARM_LOW,       /* Advisory - Cyan */
    VM_ALARM_MEDIUM,    /* Warning - Yellow */
    VM_ALARM_HIGH       /* Critical - Red */
} vm_alarm_severity_t;

/* ================================================================
 *  API
 * ================================================================ */

/** Initialize the vitals monitor dark theme on the active display. */
void theme_vitals_init(void);

/** Switch the active color scheme at run-time. */
void theme_vitals_set_mode(vm_theme_mode_t mode);

/** Get the current theme mode. */
vm_theme_mode_t theme_vitals_get_mode(void);

/** Get the LVGL color for a given alarm severity. */
lv_color_t theme_vitals_alarm_color(vm_alarm_severity_t severity);

#endif /* THEME_VITALS_H */
