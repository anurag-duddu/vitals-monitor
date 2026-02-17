/**
 * @file theme_vitals.c
 * @brief Vitals monitor theme engine, scheme definitions, and helpers
 *
 * Implements a proper lv_theme_t engine that:
 *   1. Initializes all reusable style objects via vm_styles_init()
 *   2. Creates an LVGL theme with an apply_cb that auto-styles screens
 *   3. Registers the theme on the default display
 *   4. Supports run-time scheme switching (dark / high-contrast / dimmed)
 */

#include "theme_vitals.h"
#include "theme_styles.h"

/*
 * lv_theme_t is an opaque struct in LVGL v9.  We need the private
 * header to allocate and populate the struct fields directly.
 */
#include "src/themes/lv_theme_private.h"

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

/* ================================================================
 *  LVGL Theme Engine
 * ================================================================ */

/** Static theme instance -- lives for the entire program lifetime */
static lv_theme_t theme_vitals;

/**
 * Theme apply callback.
 *
 * Called by LVGL whenever a new object is created.  We auto-apply
 * the s_screen style to screen objects (objects whose parent is NULL)
 * so that every screen automatically gets the correct background.
 */
static void apply_cb(lv_theme_t *th, lv_obj_t *obj)
{
    (void)th;  /* unused */

    if (lv_obj_get_parent(obj) == NULL) {
        /* This is a screen object -- apply screen background style */
        lv_obj_add_style(obj, &s_screen, LV_PART_MAIN);
    }
}

/* ================================================================
 *  Public API
 * ================================================================ */

void theme_vitals_init(void) {
    /* 1. Initialize all reusable style objects from the active scheme */
    vm_styles_init(vm_active_scheme);

    /* 2. Get the default display */
    lv_display_t *disp = lv_display_get_default();
    if (disp == NULL) return;

    /* 3. Create the LVGL default theme as the parent */
    lv_theme_t *parent = lv_theme_default_init(
        disp,
        lv_color_hex(vm_active_scheme->primary),
        lv_color_hex(vm_active_scheme->primary),   /* secondary = primary */
        true,                                       /* dark mode */
        VM_FONT_BODY
    );

    /* 4. Set up the vitals theme */
    lv_theme_set_parent(&theme_vitals, parent);
    lv_theme_set_apply_cb(&theme_vitals, apply_cb);
    theme_vitals.user_data = (void *)vm_active_scheme;
    theme_vitals.color_primary = lv_color_hex(vm_active_scheme->primary);
    theme_vitals.color_secondary = lv_color_hex(vm_active_scheme->primary);
    theme_vitals.font_small = VM_FONT_SMALL;
    theme_vitals.font_normal = VM_FONT_BODY;
    theme_vitals.font_large = VM_FONT_LABEL;

    /* 5. Register the theme on the default display */
    lv_display_set_theme(disp, &theme_vitals);
}

void theme_vitals_set_mode(vm_theme_mode_t mode) {
    if (mode >= VM_THEME_COUNT) return;
    s_current_mode = mode;
    vm_active_scheme = s_scheme_table[mode];

    /* Reinitialize all styles with the new scheme colors */
    vm_styles_init(vm_active_scheme);

    /* Update theme metadata to reflect new scheme */
    theme_vitals.user_data = (void *)vm_active_scheme;
    theme_vitals.color_primary = lv_color_hex(vm_active_scheme->primary);
    theme_vitals.color_secondary = lv_color_hex(vm_active_scheme->primary);

    /* Force a full redraw of the active screen */
    lv_obj_t *scr = lv_screen_active();
    if (scr != NULL) {
        lv_obj_invalidate(scr);
    }
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
