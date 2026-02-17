/**
 * @file design_tokens.h
 * @brief Complete design-token hierarchy for the vitals monitor UI
 *
 * Tokens are organized in three layers:
 *   1. Primitive tokens   -- raw values (colors, spacing, radius, etc.)
 *   2. Semantic tokens     -- role-based aliases via vm_color_scheme_t
 *   3. Component tokens    -- layout / typography / timing constants
 *
 * Clinical color assignments follow IEC 60601-1-8:
 *   Red = HIGH (critical), Yellow = MEDIUM (warning),
 *   Cyan = LOW (advisory), Green = NONE (normal).
 *
 * All spatial values scale through VM_SCALE_W / VM_SCALE_H so the
 * design adapts when VM_SCREEN_WIDTH / VM_SCREEN_HEIGHT change.
 */

#ifndef DESIGN_TOKENS_H
#define DESIGN_TOKENS_H

#include "lvgl.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 *  0. Screen dimensions & responsive helpers
 * ================================================================ */

#define VM_SCREEN_WIDTH   800
#define VM_SCREEN_HEIGHT  480

#define VM_SCALE_W(px)  ((int32_t)((px) * VM_SCREEN_WIDTH  / 800))
#define VM_SCALE_H(px)  ((int32_t)((px) * VM_SCREEN_HEIGHT / 480))

/* ================================================================
 *  1. PRIMITIVE TOKENS -- raw palette values
 * ================================================================ */

/* ---- Neutrals ------------------------------------------------- */
#define VM_GRAY_950   0x0A0A0A
#define VM_GRAY_900   0x141414
#define VM_GRAY_800   0x1A1A1A
#define VM_GRAY_700   0x222222
#define VM_GRAY_600   0x333333
#define VM_GRAY_500   0x666666
#define VM_GRAY_400   0x888888
#define VM_GRAY_300   0xAAAAAA
#define VM_GRAY_200   0xCCCCCC
#define VM_GRAY_100   0xE5E5E5
#define VM_GRAY_50    0xF5F5F5
#define VM_WHITE      0xFFFFFF
#define VM_BLACK      0x000000

/* ---- Clinical colors (IEC 60601-1-8 alarm palette) ------------ */
#define VM_RED            0xFF0000
#define VM_YELLOW_AMBER   0xFFCC00
#define VM_CYAN           0x00CCFF
#define VM_GREEN          0x00CC00
#define VM_ORANGE         0xFF8800
#define VM_BRIGHT_YELLOW  0xFFFF00

/* ---- Accent --------------------------------------------------- */
#define VM_BLUE           0x4488FF
#define VM_BLUE_DARK      0x2255AA

/* ---- Spacing scale (11 stops, via VM_SCALE_H) ----------------- */
#define VM_SPACE_0    VM_SCALE_H(0)
#define VM_SPACE_1    VM_SCALE_H(1)
#define VM_SPACE_2    VM_SCALE_H(2)
#define VM_SPACE_4    VM_SCALE_H(4)
#define VM_SPACE_8    VM_SCALE_H(8)
#define VM_SPACE_12   VM_SCALE_H(12)
#define VM_SPACE_16   VM_SCALE_H(16)
#define VM_SPACE_20   VM_SCALE_H(20)
#define VM_SPACE_24   VM_SCALE_H(24)
#define VM_SPACE_32   VM_SCALE_H(32)
#define VM_SPACE_40   VM_SCALE_H(40)

/* ---- Radius scale (7 stops) ----------------------------------- */
#define VM_RADIUS_NONE  0
#define VM_RADIUS_XS    2
#define VM_RADIUS_SM    4
#define VM_RADIUS_MD    8
#define VM_RADIUS_LG    12
#define VM_RADIUS_XL    16
#define VM_RADIUS_FULL  LV_RADIUS_CIRCLE

/* ---- Border width (4 stops) ----------------------------------- */
#define VM_BORDER_NONE    0
#define VM_BORDER_THIN    1
#define VM_BORDER_MEDIUM  2
#define VM_BORDER_THICK   3

/* ---- Opacity (6 stops, mapped to LV_OPA_*) -------------------- */
#define VM_OPA_TRANSPARENT  LV_OPA_TRANSP    /* 0   */
#define VM_OPA_10           LV_OPA_10        /* 25  */
#define VM_OPA_30           LV_OPA_30        /* 76  */
#define VM_OPA_50           LV_OPA_50        /* 127 */
#define VM_OPA_80           LV_OPA_80        /* 204 */
#define VM_OPA_COVER        LV_OPA_COVER     /* 255 */

/* ---- Motion durations (ms) ------------------------------------ */
#define VM_MOTION_INSTANT   0
#define VM_MOTION_FAST      100
#define VM_MOTION_NORMAL    200
#define VM_MOTION_SLOW      350
#define VM_MOTION_GLACIAL   500

/* ---- Easing function pointers --------------------------------- */
#define VM_EASE_DEFAULT     lv_anim_path_ease_in_out
#define VM_EASE_IN          lv_anim_path_ease_in
#define VM_EASE_OUT         lv_anim_path_ease_out
#define VM_EASE_LINEAR      lv_anim_path_linear

/* ---- Shadow tokens (behind feature flag) ---------------------- */
#ifndef VM_FEATURE_SHADOWS
#define VM_FEATURE_SHADOWS  0
#endif

#if VM_FEATURE_SHADOWS
typedef struct {
    int32_t width;
    int32_t ofs_x;
    int32_t ofs_y;
    lv_opa_t opa;
} vm_shadow_t;

static const vm_shadow_t VM_SHADOW_SM  = { .width = 4,  .ofs_x = 0, .ofs_y = 2,  .opa = LV_OPA_30 };
static const vm_shadow_t VM_SHADOW_MD  = { .width = 8,  .ofs_x = 0, .ofs_y = 4,  .opa = LV_OPA_30 };
static const vm_shadow_t VM_SHADOW_LG  = { .width = 16, .ofs_x = 0, .ofs_y = 8,  .opa = LV_OPA_30 };
#endif /* VM_FEATURE_SHADOWS */

/* ---- Touch target --------------------------------------------- */
#define VM_TOUCH_MIN  VM_SCALE_H(44)

/* ================================================================
 *  2. SEMANTIC TOKENS -- role-based color scheme
 * ================================================================ */

/**
 * Color scheme structure.
 *
 * All fields are raw 24-bit hex values (not lv_color_t) so the
 * struct can be a static const aggregate initializer.  Convert
 * to LVGL color via lv_color_hex() at the call site.
 */
typedef struct {
    /* ---- Surface hierarchy ---- */
    uint32_t background;
    uint32_t surface;
    uint32_t surface_container;
    uint32_t surface_container_high;
    uint32_t surface_container_low;

    /* ---- Content ---- */
    uint32_t on_surface;
    uint32_t on_surface_secondary;
    uint32_t on_surface_disabled;

    /* ---- Outline ---- */
    uint32_t outline;
    uint32_t outline_variant;

    /* ---- Primary accent ---- */
    uint32_t primary;
    uint32_t on_primary;

    /* ---- Clinical alarm invariants (IEC 60601-1-8) ---- */
    uint32_t alarm_high;     /* Critical - Red   */
    uint32_t alarm_medium;   /* Warning  - Yellow */
    uint32_t alarm_low;      /* Advisory - Cyan   */
    uint32_t alarm_none;     /* Normal   - Green  */

    /* ---- Vital-sign parameter colors ---- */
    uint32_t param_hr;       /* ECG / Heart rate  (Green)  */
    uint32_t param_spo2;     /* SpO2 / Pleth      (Cyan)   */
    uint32_t param_nibp;     /* Blood pressure    (White)  */
    uint32_t param_temp;     /* Temperature       (Orange) */
    uint32_t param_rr;       /* Respiration rate  (Yellow) */
} vm_color_scheme_t;

/* ---- Scheme instances (defined in theme_vitals.c) ------------- */
extern const vm_color_scheme_t vm_scheme_dark;
extern const vm_color_scheme_t vm_scheme_high_contrast;
extern const vm_color_scheme_t vm_scheme_dimmed;

/* ---- Active scheme pointer (defined in theme_vitals.c) -------- */
extern const vm_color_scheme_t *vm_active_scheme;

/* ---- Theme mode enum ------------------------------------------ */
typedef enum {
    VM_THEME_DARK = 0,
    VM_THEME_HIGH_CONTRAST,
    VM_THEME_DIMMED,
    VM_THEME_COUNT
} vm_theme_mode_t;

/* ================================================================
 *  3. COMPONENT TOKENS -- layout, spacing aliases, insets
 * ================================================================ */

/* ---- Key layout dimensions ------------------------------------ */
#define VM_ALARM_BAR_HEIGHT   VM_SCALE_H(32)
#define VM_NAV_BAR_HEIGHT     VM_SCALE_H(48)
#define VM_CONTENT_HEIGHT     (VM_SCREEN_HEIGHT - VM_ALARM_BAR_HEIGHT - VM_NAV_BAR_HEIGHT)

/* Waveform / vitals split */
#define VM_WAVEFORM_WIDTH_PCT    60   /* % of content width for waveforms */
#define VM_VITALS_WIDTH_PCT      40   /* % of content width for vitals panel */

/* Waveform display */
#define VM_WAVEFORM_POINT_COUNT      450   /* Data points per chart */
#define VM_WAVEFORM_ERASE_WIDTH      8     /* Gap points ahead of sweep head */
#define VM_WAVEFORM_ECG_HEIGHT_PCT   55    /* % of waveform area for ECG */
#define VM_WAVEFORM_PLETH_HEIGHT_PCT 45    /* % of waveform area for Pleth */

/* ---- Spacing aliases (backward-compatible) -------------------- */
#define VM_PAD_TINY    VM_SCALE_H(2)
#define VM_PAD_SMALL   VM_SCALE_H(4)
#define VM_PAD_NORMAL  VM_SCALE_H(8)
#define VM_PAD_LARGE   VM_SCALE_H(16)

/* ---- Inset defaults ------------------------------------------- */
#define VM_INSET_DEFAULT   VM_SCALE_H(8)
#define VM_STACK_DEFAULT   VM_SCALE_H(4)

/* ================================================================
 *  4. TYPOGRAPHY TOKENS
 * ================================================================ */

/* ---- Type token struct ---------------------------------------- */
typedef struct {
    const lv_font_t *font;
    int32_t          letter_space;
    int32_t          line_space;
} vm_type_token_t;

/* ---- Font macros (3-tier selection by screen height) ---------- */
#if (VM_SCREEN_HEIGHT >= 480)
    /* Full-size tier (800x480 target) */
    #define VM_FONT_VALUE_LARGE   &lv_font_montserrat_48
    #define VM_FONT_VALUE_MEDIUM  &lv_font_montserrat_32
    #define VM_FONT_LABEL         &lv_font_montserrat_20
    #define VM_FONT_BODY          &lv_font_montserrat_16
    #define VM_FONT_CAPTION       &lv_font_montserrat_14
    #define VM_FONT_SMALL         &lv_font_montserrat_12
    #define VM_FONT_UNIT          &lv_font_montserrat_12
#elif (VM_SCREEN_HEIGHT >= 320)
    /* Medium tier (e.g. 480x320) */
    #define VM_FONT_VALUE_LARGE   &lv_font_montserrat_32
    #define VM_FONT_VALUE_MEDIUM  &lv_font_montserrat_24
    #define VM_FONT_LABEL         &lv_font_montserrat_16
    #define VM_FONT_BODY          &lv_font_montserrat_14
    #define VM_FONT_CAPTION       &lv_font_montserrat_12
    #define VM_FONT_SMALL         &lv_font_montserrat_12
    #define VM_FONT_UNIT          &lv_font_montserrat_12
#else
    /* Small tier (<320) */
    #define VM_FONT_VALUE_LARGE   &lv_font_montserrat_24
    #define VM_FONT_VALUE_MEDIUM  &lv_font_montserrat_20
    #define VM_FONT_LABEL         &lv_font_montserrat_14
    #define VM_FONT_BODY          &lv_font_montserrat_12
    #define VM_FONT_CAPTION       &lv_font_montserrat_12
    #define VM_FONT_SMALL         &lv_font_montserrat_12
    #define VM_FONT_UNIT          &lv_font_montserrat_12
#endif

/* ---- Type role tokens ----------------------------------------- */
#define VM_TYPE_DISPLAY_LG  ((vm_type_token_t){ VM_FONT_VALUE_LARGE,   0, 0 })
#define VM_TYPE_DISPLAY_MD  ((vm_type_token_t){ VM_FONT_VALUE_MEDIUM,  0, 0 })
#define VM_TYPE_TITLE       ((vm_type_token_t){ VM_FONT_LABEL,         0, 2 })
#define VM_TYPE_BODY_MD     ((vm_type_token_t){ VM_FONT_BODY,          0, 2 })
#define VM_TYPE_BODY_SM     ((vm_type_token_t){ VM_FONT_CAPTION,       0, 2 })
#define VM_TYPE_CAPTION     ((vm_type_token_t){ VM_FONT_SMALL,         0, 1 })
#define VM_TYPE_UNIT        ((vm_type_token_t){ VM_FONT_UNIT,          1, 0 })

/* ================================================================
 *  5. ALARM TIMING TOKENS
 * ================================================================ */

#define VM_ALARM_HIGH_PERIOD_MS    500
#define VM_ALARM_MEDIUM_PERIOD_MS  2000
#define VM_ALARM_HIGH_TOGGLE_MS    250    /* half period */
#define VM_ALARM_MEDIUM_TOGGLE_MS  1000   /* half period */

#ifdef __cplusplus
}
#endif

#endif /* DESIGN_TOKENS_H */
