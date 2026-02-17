# UI Modernization Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Replace 489 inline `lv_obj_set_style_*()` calls with a proper design system: token hierarchy, reusable `lv_style_t` objects, LVGL `lv_theme_t` engine, and visual enhancements — all without breaking the existing 958 tests.

**Architecture:** Three-layer token system (primitive → semantic → component) feeding ~25 static `lv_style_t` objects initialized via `vm_styles_init()`. An `lv_theme_t` with `apply_cb` auto-applies base styles. Screens/widgets consume styles via `lv_obj_add_style()` instead of inline calls. Three theme modes (dark, high-contrast, dimmed) switchable at runtime.

**Tech Stack:** C99, LVGL v9.3, SDL2 simulator, SQLite, CMake

**Reference:** Full design rationale in `docs/UI_MODERNIZATION_PLAN.md`

---

## Context Rot Prevention

This plan is designed for subagent-driven execution. After each wave:
1. Update `docs/plans/2026-02-16-ui-modernization-progress.md` with completed tasks
2. Run full test suite to confirm no regressions
3. Build simulator to confirm compilation

**If session breaks:** Resume from the progress doc. Each wave is self-contained.

---

## Wave 1: Design Tokens (Phase 1)

### Task 1.1: Create design_tokens.h — Primitive tokens

**Files:**
- Create: `src/ui/themes/design_tokens.h`

**Step 1: Create the token file with primitive color palette**

```c
/**
 * @file design_tokens.h
 * @brief Design system tokens for the vitals monitor UI
 *
 * Three-layer token hierarchy:
 *   Primitive  → Raw values (colors, px, ms)
 *   Semantic   → Roles (surface, on-surface, primary)
 *   Component  → Widget-specific overrides
 *
 * All screens and widgets consume semantic/component tokens.
 * Only this file references raw hex values.
 */

#ifndef DESIGN_TOKENS_H
#define DESIGN_TOKENS_H

#include "lvgl.h"

/* ============================================================
 *  Scaling helpers (from theme_vitals.h, shared)
 * ============================================================ */
#ifndef VM_SCREEN_WIDTH
#define VM_SCREEN_WIDTH  800
#define VM_SCREEN_HEIGHT 480
#endif

#define VM_SCALE_W(px) ((int32_t)((px) * VM_SCREEN_WIDTH  / 800))
#define VM_SCALE_H(px) ((int32_t)((px) * VM_SCREEN_HEIGHT / 480))

/* ============================================================
 *  1. PRIMITIVE TOKENS — Raw values, never used directly by UI
 * ============================================================ */

/* ── 1.1 Color palette (hex values) ──────────────────────── */

/* Neutrals (dark theme base) */
#define VM_PRIM_GRAY_950    0x0A0A0A
#define VM_PRIM_GRAY_900    0x141414
#define VM_PRIM_GRAY_850    0x1A1A1A
#define VM_PRIM_GRAY_800    0x222222
#define VM_PRIM_GRAY_700    0x333333
#define VM_PRIM_GRAY_600    0x444444
#define VM_PRIM_GRAY_500    0x666666
#define VM_PRIM_GRAY_400    0x888888
#define VM_PRIM_GRAY_300    0xAAAAAA
#define VM_PRIM_GRAY_200    0xBBBBBB
#define VM_PRIM_GRAY_100    0xDDDDDD
#define VM_PRIM_GRAY_50     0xF0F0F0
#define VM_PRIM_WHITE       0xFFFFFF
#define VM_PRIM_BLACK       0x000000

/* Clinical colors (IEC 60601-1-8 compliant alarm colors) */
#define VM_PRIM_RED         0xFF0000
#define VM_PRIM_YELLOW      0xFFCC00
#define VM_PRIM_CYAN        0x00CCFF
#define VM_PRIM_GREEN       0x00CC00
#define VM_PRIM_ORANGE      0xFF8800
#define VM_PRIM_BRIGHT_YLW  0xFFFF00

/* Accent */
#define VM_PRIM_BLUE        0x4488FF
#define VM_PRIM_BLUE_DARK   0x2266CC

/* ── 1.2 Spacing scale (11 stops, 4px base) ─────────────── */
#define VM_SPACE_0      0
#define VM_SPACE_25     VM_SCALE_H(1)     /*  1px */
#define VM_SPACE_50     VM_SCALE_H(2)     /*  2px */
#define VM_SPACE_100    VM_SCALE_H(4)     /*  4px */
#define VM_SPACE_200    VM_SCALE_H(8)     /*  8px */
#define VM_SPACE_300    VM_SCALE_H(12)    /* 12px */
#define VM_SPACE_400    VM_SCALE_H(16)    /* 16px */
#define VM_SPACE_500    VM_SCALE_H(20)    /* 20px */
#define VM_SPACE_600    VM_SCALE_H(24)    /* 24px */
#define VM_SPACE_800    VM_SCALE_H(32)    /* 32px */
#define VM_SPACE_1000   VM_SCALE_H(40)    /* 40px */

/* ── 1.3 Radius scale ───────────────────────────────────── */
#define VM_RADIUS_NONE  0
#define VM_RADIUS_XS    VM_SCALE_H(2)     /*  2px */
#define VM_RADIUS_SM    VM_SCALE_H(4)     /*  4px */
#define VM_RADIUS_MD    VM_SCALE_H(8)     /*  8px */
#define VM_RADIUS_LG    VM_SCALE_H(12)    /* 12px */
#define VM_RADIUS_XL    VM_SCALE_H(16)    /* 16px */
#define VM_RADIUS_FULL  LV_RADIUS_CIRCLE

/* ── 1.4 Border width scale ─────────────────────────────── */
#define VM_BORDER_NONE   0
#define VM_BORDER_THIN   1
#define VM_BORDER_MEDIUM 2
#define VM_BORDER_THICK  3

/* ── 1.5 Opacity scale ──────────────────────────────────── */
#define VM_OPA_TRANSPARENT LV_OPA_TRANSP
#define VM_OPA_SUBTLE      LV_OPA_30
#define VM_OPA_HALF        LV_OPA_50
#define VM_OPA_VISIBLE     LV_OPA_70
#define VM_OPA_MOSTLY      LV_OPA_90
#define VM_OPA_FULL        LV_OPA_COVER

/* ── 1.6 Motion tokens ──────────────────────────────────── */
#define VM_MOTION_INSTANT  0
#define VM_MOTION_FAST     100     /* ms */
#define VM_MOTION_NORMAL   200     /* ms */
#define VM_MOTION_SLOW     350     /* ms */
#define VM_MOTION_GLACIAL  500     /* ms */

/* Easing function pointers (LVGL path callbacks) */
#define VM_EASE_STANDARD   lv_anim_path_ease_in_out
#define VM_EASE_DECEL      lv_anim_path_ease_out
#define VM_EASE_ACCEL      lv_anim_path_ease_in
#define VM_EASE_LINEAR     lv_anim_path_linear

/* ── 1.7 Elevation / Shadow tokens ──────────────────────── */
/* Shadows are behind a feature flag for performance on low-end targets */
#ifndef VM_FEATURE_SHADOWS
#define VM_FEATURE_SHADOWS 0
#endif

typedef struct {
    int32_t width;
    int32_t ofs_x;
    int32_t ofs_y;
    lv_opa_t opa;
} vm_shadow_t;

#define VM_SHADOW_NONE  ((vm_shadow_t){0, 0, 0, LV_OPA_TRANSP})
#define VM_SHADOW_LOW   ((vm_shadow_t){8,  0, 2, LV_OPA_20})
#define VM_SHADOW_MID   ((vm_shadow_t){16, 0, 4, LV_OPA_30})
#define VM_SHADOW_HIGH  ((vm_shadow_t){24, 0, 8, LV_OPA_40})

/* ── 1.8 Touch target ───────────────────────────────────── */
#define VM_TOUCH_MIN    VM_SCALE_H(44)    /* 10mm at 130 DPI */

/* ============================================================
 *  2. SEMANTIC TOKENS — Color roles (what the color MEANS)
 * ============================================================ */

/**
 * Color scheme structure — one per theme mode.
 * All UI code references the active scheme via vm_active_scheme pointer.
 */
typedef struct {
    /* Surface hierarchy */
    uint32_t background;               /* Screen background */
    uint32_t surface;                  /* Primary surface (cards) */
    uint32_t surface_container;        /* Container surface */
    uint32_t surface_container_high;   /* Elevated container */
    uint32_t surface_container_low;    /* Recessed area */

    /* Content on surface */
    uint32_t on_surface;               /* Primary text */
    uint32_t on_surface_secondary;     /* Secondary text */
    uint32_t on_surface_disabled;      /* Disabled text */

    /* Outline / border */
    uint32_t outline;                  /* Default border */
    uint32_t outline_variant;          /* Subtle border */

    /* Primary accent */
    uint32_t primary;                  /* Primary action / accent */
    uint32_t on_primary;               /* Text on primary */

    /* Clinical — NEVER change across theme modes (IEC 60601-1-8) */
    uint32_t alarm_high;               /* Red — critical */
    uint32_t alarm_medium;             /* Yellow — warning */
    uint32_t alarm_low;                /* Cyan — advisory */
    uint32_t alarm_none;               /* Green — normal */

    /* Vital parameter colors — NEVER change across modes */
    uint32_t param_hr;                 /* ECG / Heart Rate */
    uint32_t param_spo2;               /* SpO2 / Pleth */
    uint32_t param_nibp;               /* Blood Pressure */
    uint32_t param_temp;               /* Temperature */
    uint32_t param_rr;                 /* Respiration Rate */
} vm_color_scheme_t;

/* ── Scheme instances ────────────────────────────────────── */

static const vm_color_scheme_t vm_scheme_dark = {
    .background              = VM_PRIM_GRAY_950,
    .surface                 = VM_PRIM_GRAY_850,
    .surface_container       = VM_PRIM_GRAY_850,
    .surface_container_high  = VM_PRIM_GRAY_800,
    .surface_container_low   = VM_PRIM_GRAY_900,
    .on_surface              = VM_PRIM_WHITE,
    .on_surface_secondary    = VM_PRIM_GRAY_300,
    .on_surface_disabled     = VM_PRIM_GRAY_500,
    .outline                 = VM_PRIM_GRAY_700,
    .outline_variant         = VM_PRIM_GRAY_800,
    .primary                 = VM_PRIM_BLUE,
    .on_primary              = VM_PRIM_WHITE,
    /* Clinical — invariant across all modes */
    .alarm_high              = VM_PRIM_RED,
    .alarm_medium            = VM_PRIM_YELLOW,
    .alarm_low               = VM_PRIM_CYAN,
    .alarm_none              = VM_PRIM_GREEN,
    .param_hr                = VM_PRIM_GREEN,
    .param_spo2              = VM_PRIM_CYAN,
    .param_nibp              = VM_PRIM_WHITE,
    .param_temp              = VM_PRIM_ORANGE,
    .param_rr                = VM_PRIM_BRIGHT_YLW,
};

static const vm_color_scheme_t vm_scheme_high_contrast = {
    .background              = VM_PRIM_BLACK,
    .surface                 = VM_PRIM_GRAY_950,
    .surface_container       = VM_PRIM_GRAY_950,
    .surface_container_high  = VM_PRIM_GRAY_900,
    .surface_container_low   = VM_PRIM_BLACK,
    .on_surface              = VM_PRIM_WHITE,
    .on_surface_secondary    = VM_PRIM_GRAY_100,
    .on_surface_disabled     = VM_PRIM_GRAY_400,
    .outline                 = VM_PRIM_GRAY_500,
    .outline_variant         = VM_PRIM_GRAY_600,
    .primary                 = VM_PRIM_BLUE,
    .on_primary              = VM_PRIM_WHITE,
    /* Clinical — invariant */
    .alarm_high              = VM_PRIM_RED,
    .alarm_medium            = VM_PRIM_YELLOW,
    .alarm_low               = VM_PRIM_CYAN,
    .alarm_none              = VM_PRIM_GREEN,
    .param_hr                = VM_PRIM_GREEN,
    .param_spo2              = VM_PRIM_CYAN,
    .param_nibp              = VM_PRIM_WHITE,
    .param_temp              = VM_PRIM_ORANGE,
    .param_rr                = VM_PRIM_BRIGHT_YLW,
};

static const vm_color_scheme_t vm_scheme_dimmed = {
    .background              = VM_PRIM_GRAY_950,
    .surface                 = VM_PRIM_GRAY_900,
    .surface_container       = VM_PRIM_GRAY_900,
    .surface_container_high  = VM_PRIM_GRAY_850,
    .surface_container_low   = VM_PRIM_GRAY_950,
    .on_surface              = VM_PRIM_GRAY_300,
    .on_surface_secondary    = VM_PRIM_GRAY_400,
    .on_surface_disabled     = VM_PRIM_GRAY_600,
    .outline                 = VM_PRIM_GRAY_800,
    .outline_variant         = VM_PRIM_GRAY_850,
    .primary                 = VM_PRIM_BLUE_DARK,
    .on_primary              = VM_PRIM_GRAY_100,
    /* Clinical — invariant */
    .alarm_high              = VM_PRIM_RED,
    .alarm_medium            = VM_PRIM_YELLOW,
    .alarm_low               = VM_PRIM_CYAN,
    .alarm_none              = VM_PRIM_GREEN,
    .param_hr                = VM_PRIM_GREEN,
    .param_spo2              = VM_PRIM_CYAN,
    .param_nibp              = VM_PRIM_WHITE,
    .param_temp              = VM_PRIM_ORANGE,
    .param_rr                = VM_PRIM_BRIGHT_YLW,
};

/* ── Active scheme pointer (set by theme engine) ─────────── */
extern const vm_color_scheme_t *vm_active_scheme;

/* ── Theme mode enum ─────────────────────────────────────── */
typedef enum {
    VM_THEME_DARK = 0,
    VM_THEME_HIGH_CONTRAST,
    VM_THEME_DIMMED,
    VM_THEME_COUNT
} vm_theme_mode_t;

/* ============================================================
 *  3. COMPONENT TOKENS — Shorthand for common patterns
 * ============================================================ */

/* Layout constants (scaled from 800x480 reference) */
#define VM_ALARM_BAR_HEIGHT      VM_SCALE_H(32)
#define VM_NAV_BAR_HEIGHT        VM_SCALE_H(48)
#define VM_CONTENT_HEIGHT        (VM_SCREEN_HEIGHT - VM_ALARM_BAR_HEIGHT - VM_NAV_BAR_HEIGHT)

#define VM_WAVEFORM_WIDTH_PCT    60
#define VM_VITALS_WIDTH_PCT      40

/* Waveform display */
#define VM_WAVEFORM_POINT_COUNT      450
#define VM_WAVEFORM_ERASE_WIDTH      8
#define VM_WAVEFORM_ECG_HEIGHT_PCT   55
#define VM_WAVEFORM_PLETH_HEIGHT_PCT 45

/* Spacing aliases (backward compat + semantic naming) */
#define VM_PAD_TINY     VM_SPACE_50     /*  2px */
#define VM_PAD_SMALL    VM_SPACE_100    /*  4px */
#define VM_PAD_NORMAL   VM_SPACE_200    /*  8px */
#define VM_PAD_LARGE    VM_SPACE_400    /* 16px */

/* Inset defaults for cards/containers */
#define VM_INSET_DEFAULT VM_SPACE_200   /* 8px internal padding */
#define VM_STACK_DEFAULT VM_SPACE_100   /* 4px gap between children */

/* ============================================================
 *  4. TYPOGRAPHY TOKENS
 * ============================================================ */

typedef struct {
    const lv_font_t *font;
    int32_t          letter_space;
    int32_t          line_space;
} vm_type_token_t;

/* Font references (tier-selected by screen height) */
#if (VM_SCREEN_HEIGHT >= 480)
    #define VM_FONT_VALUE_LARGE   &lv_font_montserrat_48
    #define VM_FONT_VALUE_MEDIUM  &lv_font_montserrat_32
    #define VM_FONT_LABEL         &lv_font_montserrat_20
    #define VM_FONT_BODY          &lv_font_montserrat_16
    #define VM_FONT_CAPTION       &lv_font_montserrat_14
    #define VM_FONT_SMALL         &lv_font_montserrat_12
    #define VM_FONT_UNIT          &lv_font_montserrat_12
#elif (VM_SCREEN_HEIGHT >= 320)
    #define VM_FONT_VALUE_LARGE   &lv_font_montserrat_32
    #define VM_FONT_VALUE_MEDIUM  &lv_font_montserrat_24
    #define VM_FONT_LABEL         &lv_font_montserrat_16
    #define VM_FONT_BODY          &lv_font_montserrat_14
    #define VM_FONT_CAPTION       &lv_font_montserrat_12
    #define VM_FONT_SMALL         &lv_font_montserrat_12
    #define VM_FONT_UNIT          &lv_font_montserrat_12
#else
    #define VM_FONT_VALUE_LARGE   &lv_font_montserrat_24
    #define VM_FONT_VALUE_MEDIUM  &lv_font_montserrat_20
    #define VM_FONT_LABEL         &lv_font_montserrat_14
    #define VM_FONT_BODY          &lv_font_montserrat_12
    #define VM_FONT_CAPTION       &lv_font_montserrat_12
    #define VM_FONT_SMALL         &lv_font_montserrat_12
    #define VM_FONT_UNIT          &lv_font_montserrat_12
#endif

/* Type role tokens */
#define VM_TYPE_DISPLAY_LG ((vm_type_token_t){ VM_FONT_VALUE_LARGE,  0, 0 })
#define VM_TYPE_DISPLAY_MD ((vm_type_token_t){ VM_FONT_VALUE_MEDIUM, 0, 0 })
#define VM_TYPE_TITLE      ((vm_type_token_t){ VM_FONT_LABEL,        0, 2 })
#define VM_TYPE_BODY_MD    ((vm_type_token_t){ VM_FONT_BODY,         0, 2 })
#define VM_TYPE_BODY_SM    ((vm_type_token_t){ VM_FONT_CAPTION,      0, 1 })
#define VM_TYPE_CAPTION    ((vm_type_token_t){ VM_FONT_SMALL,        0, 1 })
#define VM_TYPE_UNIT       ((vm_type_token_t){ VM_FONT_UNIT,         1, 0 })

/* ============================================================
 *  5. IEC 60601-1-8 Alarm timing constants
 * ============================================================ */
#define VM_ALARM_HIGH_PERIOD_MS   500    /* 2.0 Hz flash (250ms on/off) */
#define VM_ALARM_MEDIUM_PERIOD_MS 2000   /* 0.5 Hz flash (1000ms on/off) */
#define VM_ALARM_HIGH_TOGGLE_MS   250
#define VM_ALARM_MEDIUM_TOGGLE_MS 1000

#endif /* DESIGN_TOKENS_H */
```

**Step 2: Verify compilation**

Run: `cd /Users/anuragduddu/code-projects/vitals-monitor/simulator/build && cmake .. && cmake --build . -j8 2>&1 | tail -5`
Expected: Build succeeds (design_tokens.h is not yet included by anything)

**Step 3: Commit**

```bash
git add src/ui/themes/design_tokens.h
git commit -m "feat(tokens): add design system token hierarchy

Three-layer token architecture: primitive → semantic → component.
Includes color palette, spacing scale, radius, border, opacity,
motion, shadow, and typography tokens. Three color schemes
(dark, high-contrast, dimmed) with IEC 60601-1-8 invariant
alarm colors."
```

---

### Task 1.2: Update theme_vitals.h to include design_tokens.h

**Files:**
- Modify: `src/ui/themes/theme_vitals.h`

**Step 1: Replace inline definitions with token references**

The current `theme_vitals.h` (150 lines) defines colors, spacing, fonts, and layout constants inline. Replace all of those with includes from `design_tokens.h`, keeping backward-compatible aliases where names differ.

Replace the entire file with:

```c
/**
 * @file theme_vitals.h
 * @brief Vitals monitor theme — public API and backward-compatible aliases
 *
 * This file includes design_tokens.h for all token definitions and provides
 * the theme engine API. Existing VM_COLOR_* macros are preserved as aliases
 * to the semantic token system for backward compatibility during migration.
 */

#ifndef THEME_VITALS_H
#define THEME_VITALS_H

#include "lvgl.h"
#include "design_tokens.h"

/* ============================================================
 *  Backward-compatible color macros
 *
 *  These resolve to lv_color_hex() calls using the ACTIVE scheme.
 *  During migration, screens/widgets can use either the old
 *  VM_COLOR_* macros or the new vm_active_scheme-> fields.
 *  After Phase 3, these aliases can be removed.
 * ============================================================ */

/* Background colors — resolve via active scheme */
#define VM_COLOR_BG              lv_color_hex(vm_active_scheme->background)
#define VM_COLOR_BG_PANEL        lv_color_hex(vm_active_scheme->surface_container)
#define VM_COLOR_BG_PANEL_BORDER lv_color_hex(vm_active_scheme->outline)

/* Alarm severity colors (IEC 60601-1-8) — invariant across modes */
#define VM_COLOR_ALARM_HIGH      lv_color_hex(vm_active_scheme->alarm_high)
#define VM_COLOR_ALARM_MEDIUM    lv_color_hex(vm_active_scheme->alarm_medium)
#define VM_COLOR_ALARM_LOW       lv_color_hex(vm_active_scheme->alarm_low)
#define VM_COLOR_ALARM_NONE      lv_color_hex(vm_active_scheme->alarm_none)

/* Vital sign parameter colors — invariant across modes */
#define VM_COLOR_HR              lv_color_hex(vm_active_scheme->param_hr)
#define VM_COLOR_SPO2            lv_color_hex(vm_active_scheme->param_spo2)
#define VM_COLOR_NIBP            lv_color_hex(vm_active_scheme->param_nibp)
#define VM_COLOR_TEMP            lv_color_hex(vm_active_scheme->param_temp)
#define VM_COLOR_RR              lv_color_hex(vm_active_scheme->param_rr)

/* Text colors — resolve via active scheme */
#define VM_COLOR_TEXT_PRIMARY    lv_color_hex(vm_active_scheme->on_surface)
#define VM_COLOR_TEXT_SECONDARY  lv_color_hex(vm_active_scheme->on_surface_secondary)
#define VM_COLOR_TEXT_DISABLED   lv_color_hex(vm_active_scheme->on_surface_disabled)

/* ============================================================
 *  Alarm Severity Enum
 * ============================================================ */

typedef enum {
    VM_ALARM_NONE = 0,
    VM_ALARM_LOW,       /* Advisory - Cyan */
    VM_ALARM_MEDIUM,    /* Warning - Yellow */
    VM_ALARM_HIGH       /* Critical - Red */
} vm_alarm_severity_t;

/* ============================================================
 *  Theme API
 * ============================================================ */

/** Initialize the vitals monitor theme on the default display. */
void theme_vitals_init(void);

/** Switch theme mode at runtime. Reinitializes all styles. */
void theme_vitals_set_mode(vm_theme_mode_t mode);

/** Get current theme mode. */
vm_theme_mode_t theme_vitals_get_mode(void);

/** Get the LVGL color for a given alarm severity. */
lv_color_t theme_vitals_alarm_color(vm_alarm_severity_t severity);

#endif /* THEME_VITALS_H */
```

**Step 2: Verify compilation**

Run: `cd /Users/anuragduddu/code-projects/vitals-monitor/simulator/build && cmake .. && cmake --build . -j8 2>&1 | tail -20`
Expected: Compilation errors — `vm_active_scheme` is declared extern but not defined yet. This is expected and will be resolved in Task 2.1.

**Step 3: Add temporary stub for vm_active_scheme in theme_vitals.c**

Add to the top of `src/ui/themes/theme_vitals.c`:

```c
/* Active scheme pointer — default to dark */
const vm_color_scheme_t *vm_active_scheme = &vm_scheme_dark;
```

**Step 4: Verify full compilation + tests**

Run: `cd /Users/anuragduddu/code-projects/vitals-monitor/simulator/build && cmake .. && cmake --build . -j8 2>&1 | tail -5`
Expected: Build succeeds

Run: `cd /Users/anuragduddu/code-projects/vitals-monitor/tests/unit/build && cmake .. && cmake --build . -j8 && ./test_runner 2>&1 | tail -10`
Expected: All 572 unit tests pass

**Step 5: Commit**

```bash
git add src/ui/themes/theme_vitals.h src/ui/themes/theme_vitals.c
git commit -m "feat(tokens): wire theme_vitals.h to design token system

VM_COLOR_* macros now resolve through vm_active_scheme pointer,
enabling runtime theme mode switching. All backward-compatible
aliases preserved. Clinical/alarm colors remain invariant."
```

---

## Wave 2: Style Objects & Theme Engine (Phase 2)

### Task 2.1: Create theme_styles.h — Style object declarations

**Files:**
- Create: `src/ui/themes/theme_styles.h`

**Step 1: Create the header with extern declarations for all reusable styles**

```c
/**
 * @file theme_styles.h
 * @brief Reusable lv_style_t objects for the vitals monitor design system
 *
 * ~25 static lv_style_t objects replace ~333 inline lv_obj_set_style_*() calls.
 * Initialized by vm_styles_init() during theme startup.
 * Screens/widgets apply via lv_obj_add_style(&s_card, 0).
 */

#ifndef THEME_STYLES_H
#define THEME_STYLES_H

#include "lvgl.h"

/* ── Base ──────────────────────────────────────── */
extern lv_style_t s_base;            /* Transparent container, no border/pad */
extern lv_style_t s_screen;          /* Screen background */

/* ── Surface / Card ────────────────────────────── */
extern lv_style_t s_card;            /* Standard panel/card */
extern lv_style_t s_card_elevated;   /* Higher elevation card */
extern lv_style_t s_divider;         /* Horizontal/vertical separator */

/* ── Row / Column layout helpers ───────────────── */
extern lv_style_t s_row;             /* Flex row, transparent, no pad */
extern lv_style_t s_col;             /* Flex column, transparent, no pad */

/* ── Button ────────────────────────────────────── */
extern lv_style_t s_btn;             /* Standard button default */
extern lv_style_t s_btn_pressed;     /* Button pressed state */
extern lv_style_t s_btn_focused;     /* Button focused state */
extern lv_style_t s_btn_disabled;    /* Button disabled state */
extern lv_style_t s_btn_primary;     /* Primary action button */
extern lv_style_t s_btn_danger;      /* Destructive action */

/* ── Label ─────────────────────────────────────── */
extern lv_style_t s_label;           /* Default body text */
extern lv_style_t s_label_secondary; /* Secondary / muted text */
extern lv_style_t s_label_title;     /* Section title */
extern lv_style_t s_label_display;   /* Large value display */

/* ── Input ─────────────────────────────────────── */
extern lv_style_t s_input;           /* Text/number input field */
extern lv_style_t s_dropdown;        /* Dropdown selector */
extern lv_style_t s_slider_main;     /* Slider track */
extern lv_style_t s_slider_knob;     /* Slider handle */

/* ── Table ─────────────────────────────────────── */
extern lv_style_t s_table;           /* Table container */
extern lv_style_t s_table_cell;      /* Table cell */

/* ── Chart ─────────────────────────────────────── */
extern lv_style_t s_chart;           /* Chart area */

/* ── Initialize all styles from the given color scheme ──── */
void vm_styles_init(const vm_color_scheme_t *scheme);

#endif /* THEME_STYLES_H */
```

**Step 2: Commit**

```bash
git add src/ui/themes/theme_styles.h
git commit -m "feat(styles): add theme_styles.h with ~25 reusable style declarations"
```

---

### Task 2.2: Create theme_styles.c — Style initialization

**Files:**
- Create: `src/ui/themes/theme_styles.c`

**Step 1: Create the implementation file**

```c
/**
 * @file theme_styles.c
 * @brief Style object initialization for the vitals monitor design system
 */

#include "theme_styles.h"
#include "design_tokens.h"

/* ── Style object definitions ────────────────────────────── */

lv_style_t s_base;
lv_style_t s_screen;
lv_style_t s_card;
lv_style_t s_card_elevated;
lv_style_t s_divider;
lv_style_t s_row;
lv_style_t s_col;
lv_style_t s_btn;
lv_style_t s_btn_pressed;
lv_style_t s_btn_focused;
lv_style_t s_btn_disabled;
lv_style_t s_btn_primary;
lv_style_t s_btn_danger;
lv_style_t s_label;
lv_style_t s_label_secondary;
lv_style_t s_label_title;
lv_style_t s_label_display;
lv_style_t s_input;
lv_style_t s_dropdown;
lv_style_t s_slider_main;
lv_style_t s_slider_knob;
lv_style_t s_table;
lv_style_t s_table_cell;
lv_style_t s_chart;

/* Track initialization for safe reinit on mode switch */
static bool styles_initialized = false;

/* Helper: reset then init a style (safe for reinit) */
static void safe_init(lv_style_t *s) {
    if (styles_initialized) {
        lv_style_reset(s);
    }
    lv_style_init(s);
}

void vm_styles_init(const vm_color_scheme_t *scheme) {
    /* ── Base (transparent container) ────────────────────── */
    safe_init(&s_base);
    lv_style_set_bg_opa(&s_base, VM_OPA_TRANSPARENT);
    lv_style_set_border_width(&s_base, VM_BORDER_NONE);
    lv_style_set_pad_all(&s_base, VM_SPACE_0);

    /* ── Screen ──────────────────────────────────────────── */
    safe_init(&s_screen);
    lv_style_set_bg_color(&s_screen, lv_color_hex(scheme->background));
    lv_style_set_bg_opa(&s_screen, VM_OPA_FULL);

    /* ── Card ────────────────────────────────────────────── */
    safe_init(&s_card);
    lv_style_set_bg_color(&s_card, lv_color_hex(scheme->surface_container));
    lv_style_set_bg_opa(&s_card, VM_OPA_FULL);
    lv_style_set_radius(&s_card, VM_RADIUS_SM);
    lv_style_set_border_width(&s_card, VM_BORDER_THIN);
    lv_style_set_border_color(&s_card, lv_color_hex(scheme->outline));
    lv_style_set_pad_all(&s_card, VM_INSET_DEFAULT);
    lv_style_set_pad_gap(&s_card, VM_STACK_DEFAULT);

    /* ── Card elevated ───────────────────────────────────── */
    safe_init(&s_card_elevated);
    lv_style_set_bg_color(&s_card_elevated, lv_color_hex(scheme->surface_container_high));
    lv_style_set_bg_opa(&s_card_elevated, VM_OPA_FULL);
    lv_style_set_radius(&s_card_elevated, VM_RADIUS_MD);
    lv_style_set_border_width(&s_card_elevated, VM_BORDER_THIN);
    lv_style_set_border_color(&s_card_elevated, lv_color_hex(scheme->outline));
    lv_style_set_pad_all(&s_card_elevated, VM_INSET_DEFAULT);
    lv_style_set_pad_gap(&s_card_elevated, VM_STACK_DEFAULT);
#if VM_FEATURE_SHADOWS
    lv_style_set_shadow_width(&s_card_elevated, VM_SHADOW_LOW.width);
    lv_style_set_shadow_ofs_y(&s_card_elevated, VM_SHADOW_LOW.ofs_y);
    lv_style_set_shadow_opa(&s_card_elevated, VM_SHADOW_LOW.opa);
    lv_style_set_shadow_color(&s_card_elevated, lv_color_hex(VM_PRIM_BLACK));
#endif

    /* ── Divider ─────────────────────────────────────────── */
    safe_init(&s_divider);
    lv_style_set_bg_color(&s_divider, lv_color_hex(scheme->outline_variant));
    lv_style_set_bg_opa(&s_divider, VM_OPA_FULL);

    /* ── Row (flex horizontal) ───────────────────────────── */
    safe_init(&s_row);
    lv_style_set_bg_opa(&s_row, VM_OPA_TRANSPARENT);
    lv_style_set_border_width(&s_row, VM_BORDER_NONE);
    lv_style_set_pad_all(&s_row, VM_SPACE_0);
    lv_style_set_layout(&s_row, LV_LAYOUT_FLEX);
    lv_style_set_flex_flow(&s_row, LV_FLEX_FLOW_ROW);

    /* ── Column (flex vertical) ──────────────────────────── */
    safe_init(&s_col);
    lv_style_set_bg_opa(&s_col, VM_OPA_TRANSPARENT);
    lv_style_set_border_width(&s_col, VM_BORDER_NONE);
    lv_style_set_pad_all(&s_col, VM_SPACE_0);
    lv_style_set_layout(&s_col, LV_LAYOUT_FLEX);
    lv_style_set_flex_flow(&s_col, LV_FLEX_FLOW_COLUMN);

    /* ── Button default ──────────────────────────────────── */
    safe_init(&s_btn);
    lv_style_set_bg_color(&s_btn, lv_color_hex(scheme->surface_container_high));
    lv_style_set_bg_opa(&s_btn, VM_OPA_FULL);
    lv_style_set_radius(&s_btn, VM_RADIUS_SM);
    lv_style_set_text_color(&s_btn, lv_color_hex(scheme->on_surface));
    lv_style_set_text_font(&s_btn, VM_TYPE_BODY_MD.font);
    lv_style_set_pad_all(&s_btn, VM_SPACE_200);
    lv_style_set_min_height(&s_btn, VM_TOUCH_MIN);
    lv_style_set_border_width(&s_btn, VM_BORDER_THIN);
    lv_style_set_border_color(&s_btn, lv_color_hex(scheme->outline));

    /* ── Button pressed ──────────────────────────────────── */
    safe_init(&s_btn_pressed);
    lv_style_set_bg_color(&s_btn_pressed, lv_color_hex(scheme->outline));
    lv_style_set_transform_width(&s_btn_pressed, -2);
    lv_style_set_transform_height(&s_btn_pressed, -2);

    /* ── Button focused ──────────────────────────────────── */
    safe_init(&s_btn_focused);
    lv_style_set_outline_width(&s_btn_focused, 2);
    lv_style_set_outline_color(&s_btn_focused, lv_color_hex(scheme->primary));
    lv_style_set_outline_pad(&s_btn_focused, 2);

    /* ── Button disabled ─────────────────────────────────── */
    safe_init(&s_btn_disabled);
    lv_style_set_bg_opa(&s_btn_disabled, VM_OPA_HALF);
    lv_style_set_text_opa(&s_btn_disabled, VM_OPA_HALF);

    /* ── Button primary ──────────────────────────────────── */
    safe_init(&s_btn_primary);
    lv_style_set_bg_color(&s_btn_primary, lv_color_hex(scheme->primary));
    lv_style_set_text_color(&s_btn_primary, lv_color_hex(scheme->on_primary));

    /* ── Button danger ───────────────────────────────────── */
    safe_init(&s_btn_danger);
    lv_style_set_bg_color(&s_btn_danger, lv_color_hex(scheme->alarm_high));
    lv_style_set_text_color(&s_btn_danger, lv_color_hex(VM_PRIM_WHITE));

    /* ── Label default ───────────────────────────────────── */
    safe_init(&s_label);
    lv_style_set_text_color(&s_label, lv_color_hex(scheme->on_surface));
    lv_style_set_text_font(&s_label, VM_TYPE_BODY_MD.font);

    /* ── Label secondary ─────────────────────────────────── */
    safe_init(&s_label_secondary);
    lv_style_set_text_color(&s_label_secondary, lv_color_hex(scheme->on_surface_secondary));
    lv_style_set_text_font(&s_label_secondary, VM_TYPE_BODY_SM.font);

    /* ── Label title ─────────────────────────────────────── */
    safe_init(&s_label_title);
    lv_style_set_text_color(&s_label_title, lv_color_hex(scheme->on_surface));
    lv_style_set_text_font(&s_label_title, VM_TYPE_TITLE.font);

    /* ── Label display (large value) ─────────────────────── */
    safe_init(&s_label_display);
    lv_style_set_text_color(&s_label_display, lv_color_hex(scheme->on_surface));
    lv_style_set_text_font(&s_label_display, VM_TYPE_DISPLAY_LG.font);

    /* ── Input ───────────────────────────────────────────── */
    safe_init(&s_input);
    lv_style_set_bg_color(&s_input, lv_color_hex(scheme->surface_container_low));
    lv_style_set_bg_opa(&s_input, VM_OPA_FULL);
    lv_style_set_radius(&s_input, VM_RADIUS_SM);
    lv_style_set_border_width(&s_input, VM_BORDER_THIN);
    lv_style_set_border_color(&s_input, lv_color_hex(scheme->outline));
    lv_style_set_text_color(&s_input, lv_color_hex(scheme->on_surface));
    lv_style_set_pad_all(&s_input, VM_SPACE_200);

    /* ── Dropdown ────────────────────────────────────────── */
    safe_init(&s_dropdown);
    lv_style_set_bg_color(&s_dropdown, lv_color_hex(scheme->surface_container));
    lv_style_set_bg_opa(&s_dropdown, VM_OPA_FULL);
    lv_style_set_radius(&s_dropdown, VM_RADIUS_SM);
    lv_style_set_border_width(&s_dropdown, VM_BORDER_THIN);
    lv_style_set_border_color(&s_dropdown, lv_color_hex(scheme->outline));
    lv_style_set_text_color(&s_dropdown, lv_color_hex(scheme->on_surface));
    lv_style_set_pad_all(&s_dropdown, VM_SPACE_200);

    /* ── Slider main (track) ─────────────────────────────── */
    safe_init(&s_slider_main);
    lv_style_set_bg_color(&s_slider_main, lv_color_hex(scheme->surface_container_high));
    lv_style_set_bg_opa(&s_slider_main, VM_OPA_FULL);
    lv_style_set_radius(&s_slider_main, VM_RADIUS_FULL);

    /* ── Slider knob ─────────────────────────────────────── */
    safe_init(&s_slider_knob);
    lv_style_set_bg_color(&s_slider_knob, lv_color_hex(scheme->primary));
    lv_style_set_bg_opa(&s_slider_knob, VM_OPA_FULL);
    lv_style_set_pad_all(&s_slider_knob, VM_SPACE_100);

    /* ── Table ───────────────────────────────────────────── */
    safe_init(&s_table);
    lv_style_set_bg_color(&s_table, lv_color_hex(scheme->surface));
    lv_style_set_bg_opa(&s_table, VM_OPA_FULL);
    lv_style_set_border_width(&s_table, VM_BORDER_THIN);
    lv_style_set_border_color(&s_table, lv_color_hex(scheme->outline));
    lv_style_set_pad_all(&s_table, VM_SPACE_100);

    /* ── Table cell ──────────────────────────────────────── */
    safe_init(&s_table_cell);
    lv_style_set_border_width(&s_table_cell, VM_BORDER_THIN);
    lv_style_set_border_color(&s_table_cell, lv_color_hex(scheme->outline_variant));
    lv_style_set_border_side(&s_table_cell, LV_BORDER_SIDE_BOTTOM);
    lv_style_set_pad_all(&s_table_cell, VM_SPACE_100);

    /* ── Chart ───────────────────────────────────────────── */
    safe_init(&s_chart);
    lv_style_set_bg_color(&s_chart, lv_color_hex(scheme->background));
    lv_style_set_bg_opa(&s_chart, VM_OPA_FULL);
    lv_style_set_border_width(&s_chart, VM_BORDER_NONE);
    lv_style_set_pad_all(&s_chart, VM_SPACE_0);

    styles_initialized = true;
}
```

**Step 2: Add theme_styles.c to CMakeLists.txt**

In `simulator/CMakeLists.txt`, add to UI_SOURCES (after line 140):
```
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/ui/themes/theme_styles.c
```

**Step 3: Verify compilation**

Run: `cd /Users/anuragduddu/code-projects/vitals-monitor/simulator/build && cmake .. && cmake --build . -j8 2>&1 | tail -5`
Expected: Build succeeds

**Step 4: Commit**

```bash
git add src/ui/themes/theme_styles.c simulator/CMakeLists.txt
git commit -m "feat(styles): implement ~25 reusable lv_style_t objects

vm_styles_init() initializes all style objects from a color scheme.
Supports safe reinit on theme mode switch via lv_style_reset().
Shadow support behind VM_FEATURE_SHADOWS flag."
```

---

### Task 2.3: Rewrite theme_vitals.c — Full theme engine

**Files:**
- Modify: `src/ui/themes/theme_vitals.c`

**Step 1: Rewrite with lv_theme_t engine**

Replace entire file:

```c
/**
 * @file theme_vitals.c
 * @brief LVGL theme engine for the vitals monitor
 *
 * Implements lv_theme_t with apply_cb that auto-applies base styles
 * to all newly created widgets. Supports runtime mode switching.
 */

#include "theme_vitals.h"
#include "theme_styles.h"
#include "design_tokens.h"

/* Required for lv_theme_t struct access in LVGL v9 */
#include "lvgl_private.h"

/* ── Active scheme pointer (extern in design_tokens.h) ───── */
const vm_color_scheme_t *vm_active_scheme = &vm_scheme_dark;

/* ── Theme state ─────────────────────────────────────────── */
static lv_theme_t theme_vitals;
static vm_theme_mode_t current_mode = VM_THEME_DARK;
static bool theme_initialized = false;

/* ── Theme apply callback ────────────────────────────────── */
static void apply_cb(lv_theme_t *th, lv_obj_t *obj) {
    (void)th;

    /* Apply screen background style */
    if (lv_obj_get_parent(obj) == NULL) {
        lv_obj_add_style(obj, &s_screen, 0);
        return;
    }

    /* Apply base transparent style to all containers by default */
    /* Individual screens override with s_card, s_btn, etc. */
}

/* ── Public API ──────────────────────────────────────────── */

void theme_vitals_init(void) {
    /* Initialize styles from default scheme */
    vm_styles_init(vm_active_scheme);

    /* Set up lv_theme_t */
    lv_theme_t *parent = lv_theme_default_init(
        lv_display_get_default(),
        lv_color_hex(vm_active_scheme->primary),
        lv_color_hex(vm_active_scheme->primary),
        true,  /* dark mode */
        VM_FONT_BODY
    );

    theme_vitals.apply_cb = apply_cb;
    theme_vitals.parent = parent;
    theme_vitals.user_data = NULL;

    lv_display_set_theme(lv_display_get_default(), &theme_vitals);

    theme_initialized = true;
}

void theme_vitals_set_mode(vm_theme_mode_t mode) {
    switch (mode) {
        case VM_THEME_DARK:           vm_active_scheme = &vm_scheme_dark; break;
        case VM_THEME_HIGH_CONTRAST:  vm_active_scheme = &vm_scheme_high_contrast; break;
        case VM_THEME_DIMMED:         vm_active_scheme = &vm_scheme_dimmed; break;
        default:                      vm_active_scheme = &vm_scheme_dark; break;
    }
    current_mode = mode;

    /* Reinitialize all style objects with new color values */
    vm_styles_init(vm_active_scheme);

    /* Force full screen redraw */
    lv_obj_invalidate(lv_screen_active());
}

vm_theme_mode_t theme_vitals_get_mode(void) {
    return current_mode;
}

lv_color_t theme_vitals_alarm_color(vm_alarm_severity_t severity) {
    switch (severity) {
        case VM_ALARM_HIGH:   return VM_COLOR_ALARM_HIGH;
        case VM_ALARM_MEDIUM: return VM_COLOR_ALARM_MEDIUM;
        case VM_ALARM_LOW:    return VM_COLOR_ALARM_LOW;
        default:              return VM_COLOR_ALARM_NONE;
    }
}
```

**Step 2: Verify compilation**

Run: `cd /Users/anuragduddu/code-projects/vitals-monitor/simulator/build && cmake .. && cmake --build . -j8 2>&1 | tail -20`
Expected: Build succeeds. Note: `lvgl_private.h` is in the LVGL source tree already included by CMake.

**Step 3: Run unit tests**

Run: `cd /Users/anuragduddu/code-projects/vitals-monitor/tests/unit/build && cmake .. && cmake --build . -j8 && ./test_runner 2>&1 | tail -10`
Expected: All 572 unit tests pass

**Step 4: Commit**

```bash
git add src/ui/themes/theme_vitals.c
git commit -m "feat(theme): implement LVGL lv_theme_t engine with mode switching

apply_cb auto-styles screens. Three modes: dark, high-contrast,
dimmed. Runtime switching via theme_vitals_set_mode().
Chains as child of lv_theme_default."
```

---

## Wave 3: Widget Refactor (Phase 3, Part 1)

Refactor widgets first since they're shared across all screens.

### Task 3.1: Refactor widget_waveform.c

**Files:**
- Modify: `src/ui/widgets/widget_waveform.c`

**Step 1: Add theme_styles.h include and replace inline calls**

Add `#include "theme_styles.h"` after existing includes.

Replace lines 67-74 (container styling) with:
```c
    /* Container: transparent flex column */
    w->container = lv_obj_create(parent);
    lv_obj_remove_flag(w->container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(w->container, lv_pct(100), lv_pct(100));
    lv_obj_add_style(w->container, &s_col, 0);
    lv_obj_set_style_pad_gap(w->container, VM_PAD_TINY, 0);
```

Replace lines 94-97 (chart bg/border/pad) with:
```c
    lv_obj_add_style(w->chart_obj, &s_chart, 0);
```

Replace line 117 `lv_color_hex(0xFF0000)` with `VM_COLOR_ALARM_HIGH` (lead-off label).

**Step 2: Verify compilation + tests**

Run: `cd /Users/anuragduddu/code-projects/vitals-monitor/simulator/build && cmake .. && cmake --build . -j8 2>&1 | tail -5`
Expected: Build succeeds

**Step 3: Commit**

```bash
git add src/ui/widgets/widget_waveform.c
git commit -m "refactor(waveform): replace inline styles with design system objects

Removed ~10 inline lv_obj_set_style_* calls, replaced with
s_col and s_chart style objects from theme_styles."
```

---

### Task 3.2: Refactor widget_numeric_display.c

**Files:**
- Modify: `src/ui/widgets/widget_numeric_display.c`

**Step 1: Add include and replace container/row patterns**

Add `#include "theme_styles.h"` after existing includes.

Replace the container styling block (~lines 86-92) with:
```c
    lv_obj_add_style(w->container, &s_card, 0);
    lv_obj_set_style_pad_all(w->container, VM_PAD_SMALL, 0);
    lv_obj_set_style_pad_gap(w->container, VM_PAD_TINY, 0);
    lv_obj_set_style_border_color(w->container, color, 0);
    lv_obj_set_style_border_width(w->container, VM_BORDER_MEDIUM, 0);
```

Replace the top_row styling block (~lines 119-125) with:
```c
    lv_obj_add_style(top_row, &s_row, 0);
    lv_obj_set_style_pad_gap(top_row, VM_PAD_TINY, 0);
```

Replace the left_group styling block (~lines 131-137) with:
```c
    lv_obj_add_style(left_group, &s_row, 0);
    lv_obj_set_style_pad_gap(left_group, VM_PAD_TINY, 0);
```

Keep the alarm state styling (lines 182-187) as local overrides — this is correct usage.

**Step 2: Verify compilation**

Run: `cd /Users/anuragduddu/code-projects/vitals-monitor/simulator/build && cmake .. && cmake --build . -j8 2>&1 | tail -5`
Expected: Build succeeds

**Step 3: Commit**

```bash
git add src/ui/widgets/widget_numeric_display.c
git commit -m "refactor(numeric-display): replace inline styles with design system

Container uses s_card, rows use s_row. Parameter color and alarm
border remain as intentional local overrides."
```

---

### Task 3.3: Refactor widget_alarm_banner.c

**Files:**
- Modify: `src/ui/widgets/widget_alarm_banner.c`

**Step 1: Add include and replace inline calls**

Add `#include "theme_styles.h"` after existing includes.

Replace hardcoded `0x444444` (ACK button bg, ~line 116) with `lv_color_hex(vm_active_scheme->surface_container_high)`.

Replace `0xFFFFFF` and `0x000000` in flash update (~lines 293, 297) with:
```c
lv_color_hex(vm_active_scheme->on_surface)    /* white text */
lv_color_hex(VM_PRIM_BLACK)                   /* black text on yellow */
```

Keep alarm-state flash styling as local overrides (correct for highest priority).

**Step 2: Verify compilation**

**Step 3: Commit**

```bash
git add src/ui/widgets/widget_alarm_banner.c
git commit -m "refactor(alarm-banner): replace hardcoded hex with design tokens

ACK button and flash colors now reference scheme/primitives.
Alarm state styling kept as local overrides per design."
```

---

### Task 3.4: Refactor widget_nav_bar.c

**Files:**
- Modify: `src/ui/widgets/widget_nav_bar.c`

**Step 1: Add include and replace inline calls**

Add `#include "theme_styles.h"` after existing includes.

Replace container styling (~lines 80-92) with:
```c
    lv_obj_add_style(w->container, &s_row, 0);
    lv_obj_set_style_bg_color(w->container, lv_color_hex(vm_active_scheme->surface_container), 0);
    lv_obj_set_style_bg_opa(w->container, VM_OPA_FULL, 0);
    lv_obj_set_style_border_color(w->container, lv_color_hex(vm_active_scheme->outline), 0);
    lv_obj_set_style_border_width(w->container, VM_BORDER_THIN, 0);
    lv_obj_set_style_border_side(w->container, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_pad_hor(w->container, VM_PAD_NORMAL, 0);
    lv_obj_set_style_pad_ver(w->container, VM_PAD_SMALL, 0);
```

Replace button styling (~lines 107-119): apply `&s_btn` base + override the specific differences.

**Step 2: Verify compilation**

**Step 3: Commit**

```bash
git add src/ui/widgets/widget_nav_bar.c
git commit -m "refactor(nav-bar): replace inline styles with design system

Container uses s_row base with explicit surface/border overrides.
Buttons use s_btn. Active state highlight uses scheme colors."
```

---

## Wave 4: Screen Refactor (Phase 3, Part 2)

Each screen follows the same pattern: add `#include "theme_styles.h"`, replace inline style blocks with `lv_obj_add_style()` calls, replace hardcoded hex values with token references.

### Task 4.1: Refactor screen_main_vitals.c

**Files:**
- Modify: `src/ui/screens/screen_main_vitals.c`

**Step 1: Add include and replace all inline style patterns**

Add `#include "theme_styles.h"` after existing includes.

Replace screen bg (~lines 61-62) → `lv_obj_add_style(scr, &s_screen, 0);`

Replace content container (~lines 76-80) → `lv_obj_add_style(content, &s_row, 0);`

Replace wave_area (~lines 86-93):
```c
    lv_obj_add_style(wave_area, &s_col, 0);
    lv_obj_set_style_bg_color(wave_area, lv_color_hex(vm_active_scheme->background), 0);
    lv_obj_set_style_bg_opa(wave_area, VM_OPA_FULL, 0);
    lv_obj_set_style_border_color(wave_area, lv_color_hex(vm_active_scheme->outline), 0);
    lv_obj_set_style_border_width(wave_area, VM_BORDER_THIN, 0);
    lv_obj_set_style_border_side(wave_area, LV_BORDER_SIDE_RIGHT, 0);
    lv_obj_set_style_pad_all(wave_area, VM_PAD_SMALL, 0);
    lv_obj_set_style_pad_gap(wave_area, VM_PAD_SMALL, 0);
```

Replace transparent containers (ecg_container, pleth_container, vitals_panel, bottom_row) → `lv_obj_add_style(container, &s_col, 0);` or `&s_row` as appropriate, with pad overrides where needed.

**Step 2: Verify compilation + build**

**Step 3: Commit**

```bash
git add src/ui/screens/screen_main_vitals.c
git commit -m "refactor(main-vitals): replace inline styles with design system

Screen uses s_screen, containers use s_row/s_col. Wave area
and vitals panel use explicit scheme color overrides."
```

---

### Task 4.2–4.7: Refactor remaining screens

Each follows the identical pattern. Listed concisely:

**Task 4.2: screen_trends.c**
- Replace `0xBBBBBB` (line ~216) with `lv_color_hex(VM_PRIM_GRAY_200)`
- Replace `0x222222` (line ~329) with `lv_color_hex(VM_PRIM_GRAY_800)`
- Replace inline container patterns with s_col/s_row/s_card

**Task 4.3: screen_alarms.c**
- Replace `lv_color_black()` (lines ~159, 171) with `lv_color_hex(VM_PRIM_BLACK)`
- Replace panel patterns with s_card
- Replace button patterns with s_btn

**Task 4.4: screen_settings.c**
- Replace all panel/section patterns with s_card
- Replace slider/switch styling references
- Replace button patterns with s_btn

**Task 4.5: screen_patient.c**
- Replace info-row patterns with s_row
- Replace panel patterns with s_card

**Task 4.6: screen_login.c**
- Replace login card with s_card_elevated
- Replace numpad button patterns with s_btn

**Task 4.7: screen_audit_log.c**
- Replace `0x000000` (line ~533) with `lv_color_hex(VM_PRIM_BLACK)`
- Replace table-like row patterns with s_row
- Replace filter button patterns with s_btn

**After each task:** Build simulator, run tests, commit.

---

## Wave 5: Visual Enhancements (Phase 4)

### Task 5.1: Screen transitions in screen_manager.c

**Files:**
- Modify: `src/ui/screens/screen_manager.c`

**Step 1: Add fade transitions**

In the screen loading function, replace `lv_screen_load()` with:
```c
if (alarm_engine_has_active_alarm()) {
    lv_screen_load(new_screen);  /* Instant — alarm takes priority */
} else {
    lv_screen_load_anim(new_screen, LV_SCR_LOAD_ANIM_FADE_IN,
                        VM_MOTION_SLOW, 0, true);
}
```

**Step 2: Verify and commit**

---

### Task 5.2: Smooth alarm flash animation

**Files:**
- Modify: `src/ui/widgets/widget_alarm_banner.c`

Replace timer-based binary flash with `lv_anim_t` fade while preserving IEC 60601-1-8 timing constraints.

---

### Task 5.3: Nav bar active indicator animation

**Files:**
- Modify: `src/ui/widgets/widget_nav_bar.c`

Add sliding accent bar that follows active tab using `lv_anim_t`.

---

## Wave 6: Verification & Cleanup

### Task 6.1: Grep audit — zero hardcoded hex in UI code

Run:
```bash
grep -rn '0x[0-9A-Fa-f]\{6\}' src/ui/screens/ src/ui/widgets/ --include='*.c' | grep -v 'VM_PRIM_\|vm_active_scheme\|design_tokens\|theme_'
```
Expected: Zero results (all hex values resolved through tokens).

### Task 6.2: Full test suite verification

Run:
```bash
cd tests/unit/build && cmake .. && cmake --build . -j8 && ./test_runner
cd tests/integration/build && cmake .. && cmake --build . -j8 && ./integration_test_runner
```
Expected: 572 unit + 386 integration = 958 tests pass.

### Task 6.3: Simulator build and visual check

Run: `cd simulator/build && cmake .. && cmake --build . -j8`
Expected: Clean build, zero warnings from project code.

### Task 6.4: Update progress documentation

Create/update `docs/plans/2026-02-16-ui-modernization-progress.md` with final status.

---

## Session Recovery Protocol

If the session breaks mid-wave:

1. Read `docs/plans/2026-02-16-ui-modernization-progress.md` for last completed task
2. Run `git log --oneline -10` to see what's committed
3. Run the build to check current state
4. Resume from the next incomplete task

Each wave is independently verifiable:
- **After Wave 1:** `design_tokens.h` exists, `theme_vitals.h` includes it, builds pass
- **After Wave 2:** `theme_styles.h/c` exist, `theme_vitals.c` has lv_theme_t, builds pass
- **After Wave 3:** All 4 widgets refactored, builds pass
- **After Wave 4:** All 7 screens refactored, builds pass, zero hardcoded hex
- **After Wave 5:** Visual enhancements (transitions, animations), builds pass
- **After Wave 6:** Full verification, all 958 tests pass
