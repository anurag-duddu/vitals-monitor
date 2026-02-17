/**
 * @file theme_vitals.c
 * @brief Vitals monitor theme initialization and helpers
 */

#include "theme_vitals.h"

/* ================================================================
 *  Color scheme instances (extern declared in design_tokens.h)
 * ================================================================ */

const vm_color_scheme_t vm_scheme_dark = {
    /* Surface hierarchy */
    .background             = VM_GRAY_950,   /* 0x0A0A0A */
    .surface                = VM_GRAY_800,   /* 0x1A1A1A */
    .surface_container      = VM_GRAY_800,   /* 0x1A1A1A */
    .surface_container_high = VM_GRAY_700,   /* 0x222222 */
    .surface_container_low  = VM_GRAY_900,   /* 0x141414 */
    .on_surface             = VM_WHITE,      /* 0xFFFFFF */
    .on_surface_secondary   = VM_GRAY_300,   /* 0xAAAAAA */
    .on_surface_disabled    = VM_GRAY_500,   /* 0x666666 */
    .outline                = VM_GRAY_600,   /* 0x333333 */
    .outline_variant        = VM_GRAY_700,   /* 0x222222 */
    .primary                = VM_BLUE,       /* 0x4488FF */
    .on_primary             = VM_WHITE,      /* 0xFFFFFF */
    .alarm_high             = VM_RED,            /* 0xFF0000 */
    .alarm_medium           = VM_YELLOW_AMBER,   /* 0xFFCC00 */
    .alarm_low              = VM_CYAN,           /* 0x00CCFF */
    .alarm_none             = VM_GREEN,          /* 0x00CC00 */
    .param_hr               = VM_GREEN,          /* 0x00CC00 */
    .param_spo2             = VM_CYAN,           /* 0x00CCFF */
    .param_nibp             = VM_WHITE,          /* 0xFFFFFF */
    .param_temp             = VM_ORANGE,         /* 0xFF8800 */
    .param_rr               = VM_BRIGHT_YELLOW,  /* 0xFFFF00 */
};

const vm_color_scheme_t vm_scheme_high_contrast = {
    .background             = VM_BLACK,      /* 0x000000 */
    .surface                = 0x111111,
    .surface_container      = 0x111111,
    .surface_container_high = 0x1A1A1A,
    .surface_container_low  = 0x080808,
    .on_surface             = VM_WHITE,
    .on_surface_secondary   = VM_GRAY_200,   /* 0xCCCCCC */
    .on_surface_disabled    = VM_GRAY_400,   /* 0x888888 */
    .outline                = VM_GRAY_300,   /* 0xAAAAAA */
    .outline_variant        = VM_GRAY_500,   /* 0x666666 */
    .primary                = 0x66AAFF,
    .on_primary             = VM_BLACK,
    .alarm_high             = VM_RED,
    .alarm_medium           = VM_YELLOW_AMBER,
    .alarm_low              = VM_CYAN,
    .alarm_none             = VM_GREEN,
    .param_hr               = VM_GREEN,
    .param_spo2             = VM_CYAN,
    .param_nibp             = VM_WHITE,
    .param_temp             = VM_ORANGE,
    .param_rr               = VM_BRIGHT_YELLOW,
};

const vm_color_scheme_t vm_scheme_dimmed = {
    .background             = 0x050505,
    .surface                = 0x101010,
    .surface_container      = 0x101010,
    .surface_container_high = 0x181818,
    .surface_container_low  = 0x0A0A0A,
    .on_surface             = VM_GRAY_300,   /* 0xAAAAAA */
    .on_surface_secondary   = VM_GRAY_500,   /* 0x666666 */
    .on_surface_disabled    = VM_GRAY_600,   /* 0x333333 */
    .outline                = 0x222222,
    .outline_variant        = 0x1A1A1A,
    .primary                = 0x3366CC,
    .on_primary             = VM_WHITE,
    .alarm_high             = VM_RED,
    .alarm_medium           = VM_YELLOW_AMBER,
    .alarm_low              = VM_CYAN,
    .alarm_none             = VM_GREEN,
    .param_hr               = VM_GREEN,
    .param_spo2             = VM_CYAN,
    .param_nibp             = VM_WHITE,
    .param_temp             = VM_ORANGE,
    .param_rr               = VM_BRIGHT_YELLOW,
};

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
