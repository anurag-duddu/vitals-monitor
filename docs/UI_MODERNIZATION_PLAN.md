# UI Modernization Plan — Vitals Monitor

> **Document version:** 2.0
> **Date:** 2026-02-16
> **Target:** LVGL v9.3 on Linux (800x480 primary, scalable)
> **Regulatory scope:** IEC 60601-1-8 (alarm signals), IEC 62366-1 (usability)
> **Approach:** Design system foundations first, visual features second

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Current State Audit](#2-current-state-audit)
3. [Regulatory Constraints](#3-regulatory-constraints)
4. [Design System Architecture](#4-design-system-architecture)
   - [4.1 Token Hierarchy](#41-token-hierarchy)
   - [4.2 Color System](#42-color-system)
   - [4.3 Typography System](#43-typography-system)
   - [4.4 Spacing System](#44-spacing-system)
   - [4.5 Elevation System](#45-elevation-system)
   - [4.6 Border & Radius System](#46-border--radius-system)
   - [4.7 Opacity System](#47-opacity-system)
   - [4.8 Motion System](#48-motion-system)
5. [Theme Engine](#5-theme-engine)
6. [Component Style Architecture](#6-component-style-architecture)
7. [Implementation Phases](#7-implementation-phases)
8. [Performance Budget](#8-performance-budget)
9. [Testing Strategy](#9-testing-strategy)
10. [Risk Register](#10-risk-register)
11. [File Impact Map](#11-file-impact-map)
12. [Reference Material](#12-reference-material)

---

## 1. Executive Summary

### The problem is architectural, not cosmetic

The vitals monitor UI is functionally complete (8 screens, 4 custom widgets, 958 tests) but visually basic. The root cause is **not** missing visual features — it's the **absence of a design system**. There is no token architecture, no reusable style objects, no theme engine, and no component style abstraction.

The current `theme_vitals.c` is **21 lines of code** that sets the background color. The entire codebase has **489 inline `lv_obj_set_style_*()` calls** and **zero reusable `lv_style_t` objects**. Adding shadows and gradients on top of this foundation would create a visually better but architecturally worse codebase.

### The correct order of operations

```
1. Design tokens       → Define the vocabulary (colors, spacing, radii, motion)
2. Theme engine        → Build the lv_theme_t that applies tokens automatically
3. Component styles    → Define reusable lv_style_t objects for each widget pattern
4. Screen refactor     → Replace 489 inline style calls with theme/style references
5. Visual enhancements → Add depth, motion, and typography (now trivial)
6. Custom widgets      → Build advanced visualizations on the solid foundation
```

This plan follows that order. Phases 1–4 build the system. Phases 5–6 use it.

### What this plan does NOT change

- Alarm engine logic (`alarm_engine.c`)
- Patient data model or settings store
- Screen navigation flow (stack-based push/pop)
- Auth/RBAC/audit system
- IPC transport or service architecture
- Test framework or test structure

---

## 2. Current State Audit

Hard data from a full codebase analysis.

### 2.1 What exists and works

| Area | Status | Detail |
|------|--------|--------|
| Color tokens | 16 defined, 87% adopted | 8 hardcoded hex escapes remain |
| Font tokens | 7 defined, 100% adopted | All font refs go through `VM_FONT_*` macros |
| Spacing tokens | 4 defined, partial adoption | Only `TINY(2)`, `SMALL(4)`, `NORMAL(8)`, `LARGE(16)` |
| Responsive scaling | Solid | `VM_SCALE_W/H()` macros everywhere |
| Alarm colors | IEC 60601-1-8 compliant | RED, YELLOW, CYAN correctly assigned |
| Flash timing | IEC 60601-1-8 compliant | HIGH=2.0 Hz, MEDIUM=0.5 Hz, correct duty cycles |

### 2.2 What's missing (by severity)

| Gap | Severity | Detail |
|-----|----------|--------|
| **Zero `lv_style_t` objects** | Critical | 489 inline style calls, 0 reusable styles |
| **No theme engine** | Critical | `theme_vitals_init()` = 2 lines (set bg color) |
| **No state-based styling** | High | No pressed/focused/disabled visual variants |
| **No radius tokens** | High | 19 hardcoded radius values (0, 4, 8) |
| **No border-width tokens** | High | 58 hardcoded border-width values |
| **No elevation/shadow system** | Medium | Completely flat, no depth |
| **No motion/animation system** | Medium | 0 `lv_anim_t` usage; timers only |
| **No opacity tokens** | Medium | Binary: fully opaque or fully transparent |
| **Incomplete spacing scale** | Medium | Only 4 stops; jumps 2x between each |
| **No letter-spacing/line-height** | Low | 0 calls anywhere |
| **No additional color tokens** | Low | 8 hardcoded hex values need tokens |

### 2.3 Hardcoded color escapes

These inline hex values bypass the theme system:

| File | Hex Value | Needed Token |
|------|-----------|--------------|
| `widget_alarm_banner.c:116` | `0x444444` | `VM_SEM_ON_SURFACE_MUTED` |
| `widget_alarm_banner.c:293` | `0xFFFFFF` | Already exists: `VM_COLOR_TEXT_PRIMARY` |
| `widget_alarm_banner.c:297` | `0x000000` | `VM_SEM_ON_WARNING` |
| `widget_waveform.c:117` | `0xFF0000` | Already exists: `VM_COLOR_ALARM_HIGH` |
| `screen_trends.c:216` | `0xBBBBBB` | `VM_SEM_ON_SURFACE_VARIANT` |
| `screen_trends.c:329` | `0x222222` | `VM_SEM_SURFACE_CONTAINER_HIGH` |
| `screen_audit_log.c:533` | `0x000000` | `VM_SEM_ON_WARNING` |
| `screen_alarms.c:159,171` | `lv_color_black()` | `VM_SEM_ON_WARNING` |

### 2.4 Inline style call distribution

| File | Inline `lv_obj_set_style_*()` calls |
|------|--------------------------------------|
| `screen_audit_log.c` | 73 |
| `screen_alarms.c` | 72 |
| `screen_settings.c` | 68 |
| `screen_login.c` | 59 |
| `screen_patient.c` | 52 |
| `screen_trends.c` | 44 |
| `widget_numeric_display.c` | 28 |
| `screen_main_vitals.c` | 27 |
| `widget_nav_bar.c` | 26 |
| `widget_alarm_banner.c` | 22 |
| `widget_waveform.c` | 15 |
| `theme_vitals.c` | 2 |
| **Total** | **489** |

---

## 3. Regulatory Constraints

All modernization work must preserve compliance with **IEC 60601-1-8:2006+AMD2:2020**.

### 3.1 Visual Alarm Signal Requirements (IEC 60601-1-8 Table 2)

| Priority | Required Color | Flash Frequency | Duty Cycle (ON) | Notes |
|----------|---------------|-----------------|------------------|-------|
| **HIGH** | **Red** | 1.4 Hz – 2.8 Hz | 20% – 60% | Must flash; immediate response |
| **MEDIUM** | **Yellow** | 0.4 Hz – 0.8 Hz | 20% – 60% | Must flash; prompt response |
| **LOW** | **Cyan** (or Yellow) | Constant ON | 100% | Steady; awareness |

Current implementation is **fully compliant** — HIGH at 2.0 Hz / 50% duty, MEDIUM at 0.5 Hz / 50% duty.

### 3.2 Immutable constraints for the design system

These rules shape the token architecture:

1. **Alarm colors are non-themeable** — RED (`0xFF0000`), YELLOW (`0xFFCC00`), CYAN (`0x00CCFF`) must be identical across all theme modes. They exist **outside** the themeable color system.
2. **Clinical parameter colors are convention-fixed** — HR=Green, SpO2=Cyan, NIBP=White, Temp=Orange, RR=Yellow. These are a separate "clinical palette," not part of the general color system.
3. **Flash rates are immutable** — Any motion design must not alter alarm flash timing. Animation of alarm indicators must preserve the exact frequency and duty cycle.
4. **Alarm indicators must be the most prominent element** — No decorative element may visually compete with an active alarm through brightness, saturation, size, or animation.
5. **Screen transitions must not mask alarms** — If an alarm fires during a transition, the transition must be cancelled and the alarm rendered immediately.
6. **Touch targets >= 44px** — IEC 62366-1 usability requirement.
7. **Non-alarm UI must avoid alarm hues** — Decorative use of red, yellow, or cyan as accent colors is prohibited to prevent confusion.

### 3.3 Implications for the design system

| Design System Component | Regulatory Impact |
|-------------------------|-------------------|
| Color tokens: alarm layer | **Locked** — defined as constants, not tokens |
| Color tokens: clinical palette | **Locked** — convention-fixed, rarely changed |
| Color tokens: surface/primary/accent | **Free** — fully themeable |
| Typography tokens | **Free** — no regulatory constraint (validate legibility) |
| Spacing tokens | **Free** — except touch target minimum |
| Elevation tokens | **Free** — must not obscure alarm indicators |
| Motion tokens | **Constrained** — alarm flash timing is immutable |
| Radius tokens | **Free** — purely cosmetic |

---

## 4. Design System Architecture

### Design philosophy

This design system follows the **three-layer token hierarchy** established by Material Design 3, IBM Carbon, and the W3C Design Tokens Community Group:

```
Layer 1: Primitive tokens    — Raw values with no meaning ("what options exist")
Layer 2: Semantic tokens     — Role-based mappings ("how options are used")
Layer 3: Component tokens    — Per-widget mappings ("where styles are applied")
```

In our C/LVGL context, tokens are implemented as `#define` macros (resolved at compile time, zero runtime overhead). Theme-switchable values use `const` structs initialized at startup.

### File organization

```
src/ui/themes/
    design_tokens.h         ← All token definitions (primitive + semantic)
    theme_vitals.h          ← Theme struct, API, alarm severity enum (refactored)
    theme_vitals.c          ← Theme init + lv_theme_t apply_cb + style objects
    theme_styles.h          ← Reusable lv_style_t declarations (extern)
    theme_styles.c          ← lv_style_t initialization
```

**Why this split:**
- `design_tokens.h` is the single source of truth for all design values. Only this file contains raw hex values or pixel numbers.
- `theme_vitals.h/c` owns the LVGL theme engine and theme mode switching.
- `theme_styles.h/c` owns the `lv_style_t` objects that screens and widgets reference. Separating these from the theme engine keeps both files focused and testable.

---

### 4.1 Token Hierarchy

#### Layer 1: Primitive Tokens

Raw values — a palette of options. **Never referenced directly by UI code.**

```c
/* ── Primitive Color Palette ──────────────────────────────── */

/* Neutral tonal scale (10-step) */
#define VM_PRIM_NEUTRAL_0       0x000000
#define VM_PRIM_NEUTRAL_5       0x050505
#define VM_PRIM_NEUTRAL_10      0x0A0A0A
#define VM_PRIM_NEUTRAL_15      0x111111
#define VM_PRIM_NEUTRAL_20      0x1A1A1A
#define VM_PRIM_NEUTRAL_25      0x222222
#define VM_PRIM_NEUTRAL_30      0x333333
#define VM_PRIM_NEUTRAL_40      0x444444
#define VM_PRIM_NEUTRAL_60      0x666666
#define VM_PRIM_NEUTRAL_70      0xAAAAAA
#define VM_PRIM_NEUTRAL_80      0xBBBBBB
#define VM_PRIM_NEUTRAL_100     0xFFFFFF

/* Blue accent scale (for interactive elements) */
#define VM_PRIM_BLUE_20         0x1A3366
#define VM_PRIM_BLUE_40         0x3366AA
#define VM_PRIM_BLUE_50         0x4488FF
#define VM_PRIM_BLUE_60         0x6699FF
#define VM_PRIM_BLUE_80         0xBBDDFF

/* Primitive spacing base (4px grid) */
#define VM_PRIM_SPACE_UNIT      4

/* Primitive radius base */
#define VM_PRIM_RADIUS_UNIT     4

/* Primitive font sizes (Major Third scale, 1.250 ratio, 12px base) */
/* 12 → 14 → 16 → 20 → 24 → 32 → 48 */
/* Maps to available LVGL Montserrat sizes */
```

#### Layer 2: Semantic Tokens

Role-based mappings that carry meaning. **This is what UI code references.**

```c
/* ── Surface hierarchy (from lowest to highest elevation) ─── */
#define VM_SEM_SURFACE                  VM_PRIM_NEUTRAL_10   /* 0x0A0A0A */
#define VM_SEM_SURFACE_DIM              VM_PRIM_NEUTRAL_5    /* 0x050505 */
#define VM_SEM_SURFACE_CONTAINER_LOW    VM_PRIM_NEUTRAL_15   /* 0x111111 */
#define VM_SEM_SURFACE_CONTAINER        VM_PRIM_NEUTRAL_20   /* 0x1A1A1A */
#define VM_SEM_SURFACE_CONTAINER_HIGH   VM_PRIM_NEUTRAL_25   /* 0x222222 */

/* ── Content on surfaces ──────────────────────────────────── */
#define VM_SEM_ON_SURFACE               VM_PRIM_NEUTRAL_100  /* 0xFFFFFF */
#define VM_SEM_ON_SURFACE_VARIANT       VM_PRIM_NEUTRAL_70   /* 0xAAAAAA */
#define VM_SEM_ON_SURFACE_MUTED         VM_PRIM_NEUTRAL_60   /* 0x666666 */

/* ── Outline / Borders ────────────────────────────────────── */
#define VM_SEM_OUTLINE                  VM_PRIM_NEUTRAL_30   /* 0x333333 */
#define VM_SEM_OUTLINE_VARIANT          VM_PRIM_NEUTRAL_40   /* 0x444444 */

/* ── Interactive accent (themeable) ───────────────────────── */
#define VM_SEM_PRIMARY                  VM_PRIM_BLUE_50      /* 0x4488FF */
#define VM_SEM_ON_PRIMARY               VM_PRIM_NEUTRAL_100  /* 0xFFFFFF */
#define VM_SEM_PRIMARY_CONTAINER        VM_PRIM_BLUE_20      /* 0x1A3366 */
#define VM_SEM_ON_PRIMARY_CONTAINER     VM_PRIM_BLUE_80      /* 0xBBDDFF */

/* ── Contrast pairs for text on alarm backgrounds ─────────── */
#define VM_SEM_ON_ALARM_HIGH            VM_PRIM_NEUTRAL_100  /* White on red */
#define VM_SEM_ON_ALARM_MEDIUM          VM_PRIM_NEUTRAL_0    /* Black on yellow */
#define VM_SEM_ON_ALARM_LOW             VM_PRIM_NEUTRAL_0    /* Black on cyan */
```

#### Layer 3: Component Tokens

Per-widget mappings — only defined where semantic tokens are insufficient.

```c
/* ── Numeric display widget ───────────────────────────────── */
#define VM_COMP_NUMERIC_BG              VM_SEM_SURFACE_CONTAINER
#define VM_COMP_NUMERIC_BORDER          VM_SEM_OUTLINE
#define VM_COMP_NUMERIC_VALUE_COLOR     /* Set per-parameter: VM_CLIN_HR, etc. */
#define VM_COMP_NUMERIC_LABEL_COLOR     VM_SEM_ON_SURFACE_VARIANT
#define VM_COMP_NUMERIC_UNIT_COLOR      VM_SEM_ON_SURFACE_MUTED

/* ── Navigation bar ───────────────────────────────────────── */
#define VM_COMP_NAV_BG                  VM_SEM_SURFACE_CONTAINER
#define VM_COMP_NAV_ICON_ACTIVE         VM_SEM_PRIMARY
#define VM_COMP_NAV_ICON_INACTIVE       VM_SEM_ON_SURFACE_MUTED
#define VM_COMP_NAV_INDICATOR           VM_SEM_PRIMARY

/* ── Alarm banner ─────────────────────────────────────────── */
/* NOTE: Alarm banner colors are NOT from the semantic layer.  */
/* They are regulatory-fixed constants from theme_vitals.h.    */
```

#### Token naming convention

| Prefix | Layer | Example | Referenced by |
|--------|-------|---------|---------------|
| `VM_PRIM_` | Primitive | `VM_PRIM_NEUTRAL_20` | Semantic tokens only |
| `VM_SEM_` | Semantic | `VM_SEM_SURFACE_CONTAINER` | Screens, widgets, component tokens |
| `VM_COMP_` | Component | `VM_COMP_NAV_BG` | Specific widget implementations |
| `VM_CLIN_` | Clinical | `VM_CLIN_HR` | Clinical parameter colors (convention-fixed) |
| `VM_ALARM_` | Alarm | `VM_ALARM_HIGH` | Alarm system only (regulatory-fixed) |

---

### 4.2 Color System

#### Architecture

```
                    ┌──────────────────────┐
                    │  Regulatory-Fixed    │  RED, YELLOW, CYAN
                    │  (IEC 60601-1-8)     │  Non-themeable
                    └──────────────────────┘
                              │
     ┌────────────────────────┼────────────────────────┐
     │                        │                        │
┌────▼─────┐          ┌──────▼──────┐          ┌──────▼──────┐
│ Alarm    │          │  Clinical   │          │  UI Color   │
│ Colors   │          │  Palette    │          │  System     │
│          │          │             │          │             │
│ HIGH=Red │          │ HR=Green    │          │ Primitive   │
│ MED=Yel  │          │ SpO2=Cyan   │          │   ↓         │
│ LOW=Cyan │          │ NIBP=White  │          │ Semantic    │
│ NONE=Grn │          │ Temp=Orange │          │   ↓         │
│          │          │ RR=Yellow   │          │ Component   │
│ LOCKED   │          │ LOCKED      │          │ THEMEABLE   │
└──────────┘          └─────────────┘          └─────────────┘
```

#### Alarm colors (non-themeable constants)

These are defined in `theme_vitals.h`, **not** in `design_tokens.h`, to make their regulatory status explicit:

```c
/* IEC 60601-1-8 FIXED — do not modify without regulatory review */
#define VM_COLOR_ALARM_HIGH      lv_color_hex(0xFF0000)  /* Red */
#define VM_COLOR_ALARM_MEDIUM    lv_color_hex(0xFFCC00)  /* Yellow */
#define VM_COLOR_ALARM_LOW       lv_color_hex(0x00CCFF)  /* Cyan */
#define VM_COLOR_ALARM_NONE      lv_color_hex(0x00CC00)  /* Green */
```

#### Clinical parameter colors (convention-fixed)

```c
/* Clinical convention — stable but not regulatory */
#define VM_CLIN_HR     lv_color_hex(0x00CC00)  /* Green — ECG/HR */
#define VM_CLIN_SPO2   lv_color_hex(0x00CCFF)  /* Cyan — SpO2/Pleth */
#define VM_CLIN_NIBP   lv_color_hex(0xFFFFFF)  /* White — Blood pressure */
#define VM_CLIN_TEMP   lv_color_hex(0xFF8800)  /* Orange — Temperature */
#define VM_CLIN_RR     lv_color_hex(0xFFFF00)  /* Yellow — Respiration */
```

#### Themeable color roles (semantic tokens)

The full set of semantic color roles for the dark theme:

| Role | Token | Value | Purpose |
|------|-------|-------|---------|
| `surface` | `VM_SEM_SURFACE` | `#0A0A0A` | Root background |
| `surface-dim` | `VM_SEM_SURFACE_DIM` | `#050505` | Dimmed night mode bg |
| `surface-container-low` | `VM_SEM_SURFACE_CONTAINER_LOW` | `#111111` | Recessed areas |
| `surface-container` | `VM_SEM_SURFACE_CONTAINER` | `#1A1A1A` | Cards, panels |
| `surface-container-high` | `VM_SEM_SURFACE_CONTAINER_HIGH` | `#222222` | Elevated cards, menus |
| `on-surface` | `VM_SEM_ON_SURFACE` | `#FFFFFF` | Primary text on surfaces |
| `on-surface-variant` | `VM_SEM_ON_SURFACE_VARIANT` | `#AAAAAA` | Secondary text |
| `on-surface-muted` | `VM_SEM_ON_SURFACE_MUTED` | `#666666` | Disabled/hint text |
| `outline` | `VM_SEM_OUTLINE` | `#333333` | Borders, dividers |
| `outline-variant` | `VM_SEM_OUTLINE_VARIANT` | `#444444` | Subtle borders |
| `primary` | `VM_SEM_PRIMARY` | `#4488FF` | Interactive accent |
| `on-primary` | `VM_SEM_ON_PRIMARY` | `#FFFFFF` | Text on primary |
| `primary-container` | `VM_SEM_PRIMARY_CONTAINER` | `#1A3366` | Accent backgrounds |
| `on-primary-container` | `VM_SEM_ON_PRIMARY_CONTAINER` | `#BBDDFF` | Text on accent bg |

#### Theme modes

Three modes, differing only in the non-alarm color roles:

| Token | Dark (default) | High Contrast | Dimmed (night) |
|-------|---------------|---------------|----------------|
| `surface` | `#0A0A0A` | `#000000` | `#050505` |
| `surface-container` | `#1A1A1A` | `#1A1A1A` | `#0F0F0F` |
| `on-surface` | `#FFFFFF` | `#FFFFFF` | `#CCCCCC` |
| `on-surface-variant` | `#AAAAAA` | `#DDDDDD` | `#888888` |
| `outline` | `#333333` | `#666666` | `#222222` |
| `primary` | `#4488FF` | `#FFFFFF` | `#2255AA` |
| **Alarm HIGH** | `#FF0000` | `#FF0000` | `#FF0000` |
| **Alarm MEDIUM** | `#FFCC00` | `#FFCC00` | `#FFCC00` |
| **Alarm LOW** | `#00CCFF` | `#00CCFF` | `#00CCFF` |

Alarm colors are **identical across all modes**. Only surface, text, and accent colors change.

#### Color contrast compliance

Target: **WCAG 2.1 AAA** (7:1 normal text, 4.5:1 large text).

| Pair | Ratio | Passes |
|------|-------|--------|
| `on-surface (#FFF)` on `surface (#0A0A0A)` | 19.3:1 | AAA |
| `on-surface-variant (#AAA)` on `surface (#0A0A0A)` | 10.1:1 | AAA |
| `on-surface-muted (#666)` on `surface (#0A0A0A)` | 4.2:1 | AA (large text only) |
| `on-primary (#FFF)` on `primary (#4488FF)` | 3.4:1 | AA (large text) |
| `on-alarm-high (#FFF)` on `alarm-high (#FF0000)` | 4.0:1 | AA (large text) |
| `on-alarm-medium (#000)` on `alarm-medium (#FFCC00)` | 14.5:1 | AAA |

#### Implementation in C

```c
/* Runtime-switchable theme struct (initialized at startup, not malloc'd) */
typedef struct {
    uint32_t surface;
    uint32_t surface_dim;
    uint32_t surface_container_low;
    uint32_t surface_container;
    uint32_t surface_container_high;
    uint32_t on_surface;
    uint32_t on_surface_variant;
    uint32_t on_surface_muted;
    uint32_t outline;
    uint32_t outline_variant;
    uint32_t primary;
    uint32_t on_primary;
    uint32_t primary_container;
    uint32_t on_primary_container;
} vm_color_scheme_t;

/* Pre-defined schemes (const, in ROM) */
extern const vm_color_scheme_t vm_scheme_dark;
extern const vm_color_scheme_t vm_scheme_high_contrast;
extern const vm_color_scheme_t vm_scheme_dimmed;

/* Active scheme pointer (switched at runtime) */
extern const vm_color_scheme_t *vm_active_scheme;
```

**Note:** Because `lv_color_hex()` is a function call in LVGL v9, color scheme values are stored as `uint32_t` hex values and converted to `lv_color_t` via `lv_color_hex()` at point of use. Alternatively, `LV_COLOR_MAKE(r, g, b)` can be used in static initializers.

---

### 4.3 Typography System

#### Type scale

Using a **Major Third (1.250)** ratio from a 12px base, mapped to available LVGL Montserrat sizes:

| Scale step | Calculated | Mapped to LVGL | Token |
|------------|-----------|-----------------|-------|
| 0 | 12px | `montserrat_12` | `VM_TYPE_CAPTION_SM` |
| 1 | 15px | `montserrat_14` | `VM_TYPE_CAPTION_MD` |
| 2 | 19px | `montserrat_16` | `VM_TYPE_BODY_MD` |
| 3 | 23px | `montserrat_20` | `VM_TYPE_TITLE_SM` |
| 4 | 29px | `montserrat_24` | `VM_TYPE_TITLE_MD` |
| 5 | 37px | `montserrat_32` | `VM_TYPE_DISPLAY_SM` |
| 6 | 47px | `montserrat_48` | `VM_TYPE_DISPLAY_LG` |

#### Type roles

Inspired by Material Design 3's 5-role system, adapted for medical monitors:

| Role | Size | Usage in vitals monitor | Token |
|------|------|------------------------|-------|
| **Display Large** | 48px | Primary vital values (HR: **72**) | `VM_TYPE_DISPLAY_LG` |
| **Display Small** | 32px | Secondary vital values, prominent numbers | `VM_TYPE_DISPLAY_SM` |
| **Title Medium** | 24px | Screen titles, section headers | `VM_TYPE_TITLE_MD` |
| **Title Small** | 20px | Panel titles, parameter labels | `VM_TYPE_TITLE_SM` |
| **Body Medium** | 16px | Settings text, list items, log entries | `VM_TYPE_BODY_MD` |
| **Caption Medium** | 14px | Timestamps, units, secondary labels | `VM_TYPE_CAPTION_MD` |
| **Caption Small** | 12px | Fine print, compact table cells | `VM_TYPE_CAPTION_SM` |

#### Typography token struct

Each type token bundles font pointer + letter-spacing + line-spacing:

```c
typedef struct {
    const lv_font_t *font;       /* LVGL font (family + size + weight) */
    int32_t          letter_space; /* px between characters */
    int32_t          line_space;   /* px between lines */
} vm_type_token_t;

/* Token definitions */
static const vm_type_token_t VM_TYPE_DISPLAY_LG = {
    .font = &lv_font_montserrat_48, .letter_space = -1, .line_space = 0
};
static const vm_type_token_t VM_TYPE_DISPLAY_SM = {
    .font = &lv_font_montserrat_32, .letter_space = 0, .line_space = 0
};
static const vm_type_token_t VM_TYPE_TITLE_MD = {
    .font = &lv_font_montserrat_24, .letter_space = 0, .line_space = 2
};
static const vm_type_token_t VM_TYPE_TITLE_SM = {
    .font = &lv_font_montserrat_20, .letter_space = 0, .line_space = 2
};
static const vm_type_token_t VM_TYPE_BODY_MD = {
    .font = &lv_font_montserrat_16, .letter_space = 0, .line_space = 4
};
static const vm_type_token_t VM_TYPE_CAPTION_MD = {
    .font = &lv_font_montserrat_14, .letter_space = 0, .line_space = 2
};
static const vm_type_token_t VM_TYPE_CAPTION_SM = {
    .font = &lv_font_montserrat_12, .letter_space = 0, .line_space = 2
};
```

#### Font weight variants (optional upgrade path)

Using LVGL's built-in Montserrat (Regular weight only) is the baseline. For weight variation:

| Option | Approach | ROM Cost | Visual Impact |
|--------|----------|----------|---------------|
| **A: Built-in only** | Montserrat Regular at 7 sizes | 0 KB extra | Low — size hierarchy only |
| **B: Add Bold** | Generate Montserrat Bold via `lv_font_conv` for 32px + 48px | ~65 KB | Medium — value emphasis |
| **C: Switch to Inter** | Generate Inter Regular + Bold + Light at all sizes | ~140 KB | High — full weight hierarchy |

Decision: Start with **Option A** (built-in Montserrat). The type token struct makes upgrading to B or C a single-file change in `design_tokens.h` — no screen code changes needed.

#### Backward compatibility

Map old `VM_FONT_*` macros to new tokens during migration:

```c
/* DEPRECATED — use vm_type_token_t instead */
#define VM_FONT_VALUE_LARGE   (VM_TYPE_DISPLAY_LG.font)
#define VM_FONT_VALUE_MEDIUM  (VM_TYPE_DISPLAY_SM.font)
#define VM_FONT_LABEL         (VM_TYPE_TITLE_SM.font)
#define VM_FONT_BODY          (VM_TYPE_BODY_MD.font)
#define VM_FONT_CAPTION       (VM_TYPE_CAPTION_MD.font)
#define VM_FONT_SMALL         (VM_TYPE_CAPTION_SM.font)
#define VM_FONT_UNIT          (VM_TYPE_CAPTION_SM.font)
```

---

### 4.4 Spacing System

#### Base unit: 4px

All spacing values are multiples of a 4px base unit, wrapped in responsive scale macros.

```c
/* Spacing scale (4px grid, resolution-adapted via VM_SCALE_H) */
#define VM_SPACE_0        0                     /*  0px — none */
#define VM_SPACE_025      VM_SCALE_H(1)         /*  1px — hairline */
#define VM_SPACE_050      VM_SCALE_H(2)         /*  2px — tight */
#define VM_SPACE_100      VM_SCALE_H(4)         /*  4px — base unit */
#define VM_SPACE_150      VM_SCALE_H(6)         /*  6px — compact */
#define VM_SPACE_200      VM_SCALE_H(8)         /*  8px — standard */
#define VM_SPACE_300      VM_SCALE_H(12)        /* 12px — comfortable */
#define VM_SPACE_400      VM_SCALE_H(16)        /* 16px — spacious */
#define VM_SPACE_500      VM_SCALE_H(20)        /* 20px — generous */
#define VM_SPACE_600      VM_SCALE_H(24)        /* 24px — section gap */
#define VM_SPACE_800      VM_SCALE_H(32)        /* 32px — region gap */
```

#### Semantic spacing aliases

```c
/* Component inset (internal padding) */
#define VM_INSET_COMPACT    VM_SPACE_100    /* 4px — dense list items */
#define VM_INSET_DEFAULT    VM_SPACE_200    /* 8px — standard panels */
#define VM_INSET_SPACIOUS   VM_SPACE_400    /* 16px — cards, dialog */

/* Stack gap (between stacked elements) */
#define VM_STACK_TIGHT      VM_SPACE_050    /* 2px — between related items */
#define VM_STACK_DEFAULT    VM_SPACE_100    /* 4px — standard gap */
#define VM_STACK_LOOSE      VM_SPACE_200    /* 8px — between sections */
#define VM_STACK_SECTION    VM_SPACE_400    /* 16px — between major sections */
```

#### Migration from old tokens

```c
/* Old → New mapping */
#define VM_PAD_TINY     VM_SPACE_050    /* 2px (was VM_SCALE_H(2)) */
#define VM_PAD_SMALL    VM_SPACE_100    /* 4px (was VM_SCALE_H(4)) */
#define VM_PAD_NORMAL   VM_SPACE_200    /* 8px (was VM_SCALE_H(8)) */
#define VM_PAD_LARGE    VM_SPACE_400    /* 16px (was VM_SCALE_H(16)) */
```

---

### 4.5 Elevation System

#### Approach: Tonal color, not shadows

Based on LVGL performance analysis (shadow rendering is CPU-expensive on embedded hardware), we express elevation through **tonal surface color differentiation** rather than drop shadows. This follows Material Design 3's dark theme approach where higher elevation = lighter surface tint.

```c
/* Elevation levels — expressed as surface colors */
#define VM_ELEVATION_0    VM_SEM_SURFACE                  /* #0A0A0A — base */
#define VM_ELEVATION_1    VM_SEM_SURFACE_CONTAINER        /* #1A1A1A — cards */
#define VM_ELEVATION_2    VM_SEM_SURFACE_CONTAINER_HIGH   /* #222222 — menus, dialogs */
```

#### Optional shadow tokens (for devices with GPU acceleration)

Shadows are available behind a compile-time flag for hardware with sufficient rendering power:

```c
#ifdef VM_FEATURE_SHADOWS

typedef struct {
    int32_t  width;     /* blur radius */
    int32_t  ofs_x;     /* horizontal offset */
    int32_t  ofs_y;     /* vertical offset */
    int32_t  spread;    /* expand/contract base */
    uint32_t color;     /* shadow color hex */
    lv_opa_t opa;       /* shadow opacity */
} vm_shadow_token_t;

static const vm_shadow_token_t VM_SHADOW_NONE  = { 0, 0, 0, 0, 0x000000, LV_OPA_TRANSP };
static const vm_shadow_token_t VM_SHADOW_LOW   = { 8,  0, 2, 1, 0x000000, LV_OPA_20 };
static const vm_shadow_token_t VM_SHADOW_MED   = { 16, 0, 4, 2, 0x000000, LV_OPA_30 };
static const vm_shadow_token_t VM_SHADOW_HIGH  = { 24, 0, 6, 3, 0x000000, LV_OPA_40 };

#endif /* VM_FEATURE_SHADOWS */
```

**Performance rule:** Maximum 3 visible shadowed elements at any time. Shadow blur radius capped at 24px.

---

### 4.6 Border & Radius System

#### Radius scale

```c
#define VM_RADIUS_NONE    0
#define VM_RADIUS_XS      VM_SCALE_H(2)    /*  2px — subtle rounding */
#define VM_RADIUS_SM      VM_SCALE_H(4)    /*  4px — buttons, inputs */
#define VM_RADIUS_MD      VM_SCALE_H(8)    /*  8px — cards, panels */
#define VM_RADIUS_LG      VM_SCALE_H(12)   /* 12px — prominent cards */
#define VM_RADIUS_XL      VM_SCALE_H(16)   /* 16px — modal dialogs */
#define VM_RADIUS_FULL    LV_RADIUS_CIRCLE /* Pill shape / circle */
```

#### Border width scale

```c
#define VM_BORDER_NONE    0
#define VM_BORDER_THIN    1    /* Subtle divider */
#define VM_BORDER_DEFAULT 2    /* Standard border */
#define VM_BORDER_THICK   3    /* Emphasis (alarm state) */
```

---

### 4.7 Opacity System

```c
#define VM_OPA_TRANSPARENT  LV_OPA_TRANSP    /*   0% — invisible */
#define VM_OPA_GHOST        LV_OPA_10        /*  10% — barely visible overlay */
#define VM_OPA_SUBTLE       LV_OPA_20        /*  20% — subtle background tint */
#define VM_OPA_HALF         LV_OPA_50        /*  50% — disabled state, overlays */
#define VM_OPA_VISIBLE      LV_OPA_70        /*  70% — secondary elements */
#define VM_OPA_FULL         LV_OPA_COVER     /* 100% — fully opaque */
```

---

### 4.8 Motion System

#### Duration tokens

```c
#define VM_MOTION_INSTANT     0      /* No animation */
#define VM_MOTION_FAST        100    /* Quick feedback (button press) */
#define VM_MOTION_NORMAL      200    /* Standard transition */
#define VM_MOTION_SLOW        350    /* Screen transition */
#define VM_MOTION_DELIBERATE  500    /* Emphasized entrance */
```

#### Easing tokens

```c
/* LVGL path function pointers */
#define VM_EASE_STANDARD      lv_anim_path_ease_in_out   /* Default motion */
#define VM_EASE_DECELERATE    lv_anim_path_ease_out      /* Enter animations */
#define VM_EASE_ACCELERATE    lv_anim_path_ease_in       /* Exit animations */
#define VM_EASE_LINEAR        lv_anim_path_linear        /* Progress bars */
```

#### Alarm-safe motion rules

```c
/* IEC 60601-1-8 TIMING — do not modify without regulatory review */
#define VM_ALARM_HIGH_PERIOD_MS    500    /* 2.0 Hz (range: 1.4–2.8 Hz) */
#define VM_ALARM_HIGH_DUTY_PCT     50     /* (range: 20–60%) */
#define VM_ALARM_MED_PERIOD_MS     2000   /* 0.5 Hz (range: 0.4–0.8 Hz) */
#define VM_ALARM_MED_DUTY_PCT      50     /* (range: 20–60%) */
```

#### Medical motion constraints (ISO 9241-391)

- No flashing above 3 Hz for elements covering > 25% of screen
- No alternating red/blue flashes at any frequency
- Screen transitions capped at 350ms to avoid clinical workflow delay
- All animations are **functional** (feedback, state change) — never decorative

---

## 5. Theme Engine

### 5.1 LVGL `lv_theme_t` implementation

Replace the current 21-line `theme_vitals.c` with a proper theme engine:

```c
#include "lvgl_private.h"  /* Required in LVGL v9 for lv_theme_t fields */

static lv_theme_t vm_theme;

static void vm_theme_apply_cb(lv_theme_t *th, lv_obj_t *obj) {
    (void)th;

    /* Base object — transparent container by default */
    lv_obj_add_style(obj, &s_base, 0);

    /* Type-specific auto-styling */
    if (lv_obj_check_type(obj, &lv_button_class)) {
        lv_obj_add_style(obj, &s_btn, 0);
        lv_obj_add_style(obj, &s_btn_pressed, LV_STATE_PRESSED);
        lv_obj_add_style(obj, &s_btn_focused, LV_STATE_FOCUSED);
        lv_obj_add_style(obj, &s_btn_disabled, LV_STATE_DISABLED);
    }
    else if (lv_obj_check_type(obj, &lv_label_class)) {
        lv_obj_add_style(obj, &s_label, 0);
    }
    else if (lv_obj_check_type(obj, &lv_slider_class)) {
        lv_obj_add_style(obj, &s_slider_main, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_slider_indicator, LV_PART_INDICATOR);
        lv_obj_add_style(obj, &s_slider_knob, LV_PART_KNOB);
    }
    else if (lv_obj_check_type(obj, &lv_switch_class)) {
        lv_obj_add_style(obj, &s_switch_main, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_switch_indicator, LV_PART_INDICATOR);
        lv_obj_add_style(obj, &s_switch_knob, LV_PART_KNOB);
    }
    else if (lv_obj_check_type(obj, &lv_dropdown_class)) {
        lv_obj_add_style(obj, &s_dropdown, 0);
    }
    else if (lv_obj_check_type(obj, &lv_table_class)) {
        lv_obj_add_style(obj, &s_table, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_table_cell, LV_PART_ITEMS);
    }
}
```

### 5.2 Theme mode switching

```c
void theme_vitals_set_mode(vm_theme_mode_t mode) {
    switch (mode) {
        case VM_THEME_DARK:           vm_active_scheme = &vm_scheme_dark; break;
        case VM_THEME_HIGH_CONTRAST:  vm_active_scheme = &vm_scheme_high_contrast; break;
        case VM_THEME_DIMMED:         vm_active_scheme = &vm_scheme_dimmed; break;
    }

    /* Reinitialize all style objects with new color values */
    vm_styles_init(vm_active_scheme);

    /* Force full screen redraw */
    lv_obj_invalidate(lv_screen_active());
}
```

### 5.3 Style priority (LVGL resolution order)

Understanding LVGL's style stack is critical for the theme architecture:

```
1. Transition styles    (highest priority — temporary animation states)
2. Local styles         (lv_obj_set_style_* — per-object overrides)
3. Added styles         (lv_obj_add_style — searched top-to-bottom)
4. Theme styles         (apply_cb — base layer)
5. Inherited styles     (from parent — lowest priority)
```

This means:
- The theme provides **base styling** for all widgets
- Screens can **override** the theme via `lv_obj_add_style()` for specific variants
- Alarm states use **local styles** (`lv_obj_set_style_*`) to override everything
- Widget-specific styling (parameter colors) uses added styles

---

## 6. Component Style Architecture

### 6.1 Reusable style objects

Replace 489 inline calls with ~25–30 reusable `lv_style_t` objects:

```c
/* In theme_styles.h — extern declarations */

/* ── Base ──────────────────────────────────────── */
extern lv_style_t s_base;            /* Transparent container default */
extern lv_style_t s_screen;          /* Screen background (with gradient) */

/* ── Surface / Card ────────────────────────────── */
extern lv_style_t s_card;            /* Standard panel/card */
extern lv_style_t s_card_elevated;   /* Higher elevation card */
extern lv_style_t s_divider;         /* Horizontal separator */

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
extern lv_style_t s_slider_indicator;/* Slider filled portion */
extern lv_style_t s_slider_knob;     /* Slider handle */
extern lv_style_t s_switch_main;     /* Switch track */
extern lv_style_t s_switch_indicator;/* Switch indicator */
extern lv_style_t s_switch_knob;     /* Switch handle */

/* ── Table ─────────────────────────────────────── */
extern lv_style_t s_table;           /* Table container */
extern lv_style_t s_table_cell;      /* Table cell */

/* ── Chart ─────────────────────────────────────── */
extern lv_style_t s_chart;           /* Chart area */
```

### 6.2 Style initialization (in `theme_styles.c`)

```c
void vm_styles_init(const vm_color_scheme_t *scheme) {
    /* Card */
    lv_style_init(&s_card);
    lv_style_set_bg_color(&s_card, lv_color_hex(scheme->surface_container));
    lv_style_set_bg_opa(&s_card, LV_OPA_COVER);
    lv_style_set_radius(&s_card, VM_RADIUS_MD);
    lv_style_set_border_width(&s_card, VM_BORDER_THIN);
    lv_style_set_border_color(&s_card, lv_color_hex(scheme->outline));
    lv_style_set_pad_all(&s_card, VM_INSET_DEFAULT);
    lv_style_set_pad_gap(&s_card, VM_STACK_DEFAULT);

    /* Button - default state */
    lv_style_init(&s_btn);
    lv_style_set_bg_color(&s_btn, lv_color_hex(scheme->surface_container_high));
    lv_style_set_bg_opa(&s_btn, LV_OPA_COVER);
    lv_style_set_radius(&s_btn, VM_RADIUS_SM);
    lv_style_set_text_color(&s_btn, lv_color_hex(scheme->on_surface));
    lv_style_set_text_font(&s_btn, VM_TYPE_BODY_MD.font);
    lv_style_set_pad_all(&s_btn, VM_SPACE_200);
    lv_style_set_min_height(&s_btn, VM_TOUCH_MIN);

    /* Button - pressed state */
    lv_style_init(&s_btn_pressed);
    lv_style_set_bg_color(&s_btn_pressed, lv_color_hex(scheme->outline));
    lv_style_set_transform_width(&s_btn_pressed, -2);
    lv_style_set_transform_height(&s_btn_pressed, -2);

    /* Button - focused state */
    lv_style_init(&s_btn_focused);
    lv_style_set_outline_width(&s_btn_focused, 2);
    lv_style_set_outline_color(&s_btn_focused, lv_color_hex(scheme->primary));
    lv_style_set_outline_pad(&s_btn_focused, 2);

    /* Button - disabled state */
    lv_style_init(&s_btn_disabled);
    lv_style_set_bg_opa(&s_btn_disabled, VM_OPA_HALF);
    lv_style_set_text_opa(&s_btn_disabled, VM_OPA_HALF);

    /* ... continue for all style objects ... */
}
```

### 6.3 Screen refactoring pattern

**Before** (current — inline calls):
```c
static void create_panel(lv_obj_t *parent) {
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_style_bg_color(panel, VM_COLOR_BG_PANEL, 0);       /* 1 */
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);              /* 2 */
    lv_obj_set_style_border_color(panel, VM_COLOR_BG_PANEL_BORDER, 0); /* 3 */
    lv_obj_set_style_border_width(panel, 1, 0);                    /* 4 */
    lv_obj_set_style_radius(panel, 4, 0);                          /* 5 */
    lv_obj_set_style_pad_all(panel, VM_PAD_NORMAL, 0);             /* 6 */
    lv_obj_set_style_pad_gap(panel, VM_PAD_SMALL, 0);              /* 7 */
    /* 7 calls per panel × ~30 panels = ~210 calls */
}
```

**After** (with design system):
```c
static void create_panel(lv_obj_t *parent) {
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_add_style(panel, &s_card, 0);
    /* 1 call. All properties come from the reusable style object. */
    /* Override only when this panel differs from the default: */
    /* lv_obj_add_style(panel, &s_card_elevated, 0); */
}
```

**Estimated reduction:** From ~489 inline calls to ~60–80 style additions + a handful of local overrides.

### 6.4 Alarm state overrides

Alarm states use **local styles** (highest priority except transitions) to ensure they always win over theme/added styles:

```c
/* When a parameter enters alarm state — set directly on the specific widget */
void widget_numeric_display_set_alarm_state(
    widget_numeric_display_t *w, vm_alarm_severity_t severity
) {
    lv_color_t border_color = theme_vitals_alarm_color(severity);

    if (severity != VM_ALARM_NONE) {
        /* Local style overrides — always wins over theme */
        lv_obj_set_style_border_color(w->container, border_color, 0);
        lv_obj_set_style_border_width(w->container, VM_BORDER_THICK, 0);
    } else {
        /* Remove local overrides — theme style takes effect */
        lv_obj_remove_local_style_prop(w->container, LV_STYLE_BORDER_COLOR, 0);
        lv_obj_remove_local_style_prop(w->container, LV_STYLE_BORDER_WIDTH, 0);
    }
}
```

---

## 7. Implementation Phases

Each phase is independently testable and builds on the previous.

---

### Phase 1: Design Tokens & File Structure (1 day)

**Goal:** Create the token architecture files and define all tokens.

**Deliverables:**
1. Create `src/ui/themes/design_tokens.h` with all token definitions:
   - Primitive color palette
   - Semantic color roles
   - Clinical and alarm color constants (moved from `theme_vitals.h`)
   - Spacing scale (11 stops)
   - Radius scale (7 stops)
   - Border width scale (4 stops)
   - Opacity scale (6 stops)
   - Typography token struct and definitions (7 type roles)
   - Motion tokens (5 durations + 4 easings)
   - Elevation tokens (3 levels)
   - Shadow tokens (optional, behind `VM_FEATURE_SHADOWS`)
2. Create `vm_color_scheme_t` struct and 3 scheme instances (dark, high-contrast, dimmed)
3. Update `theme_vitals.h` to include `design_tokens.h` and add backward-compatible aliases
4. Verify: All existing code compiles unchanged with new token files included

**Files created:** `design_tokens.h`
**Files modified:** `theme_vitals.h` (add include, deprecation aliases)
**Tests:** Compilation passes. All 958 existing tests still pass (no behavior change).

---

### Phase 2: Style Objects & Theme Engine (2–3 days)

**Goal:** Create reusable `lv_style_t` objects and the `lv_theme_t` engine.

**Deliverables:**
1. Create `src/ui/themes/theme_styles.h` — extern declarations for ~25 style objects
2. Create `src/ui/themes/theme_styles.c` — `vm_styles_init()` implementation
3. Rewrite `theme_vitals.c`:
   - Implement `lv_theme_t` with `apply_cb`
   - Call `vm_styles_init()` during initialization
   - Chain as child of `lv_theme_default_init()` parent
   - Add `theme_vitals_set_mode()` for runtime mode switching
4. Add theme mode selector to Settings screen
5. Verify: Theme auto-applies base styles to all newly created widgets

**Files created:** `theme_styles.h`, `theme_styles.c`
**Files modified:** `theme_vitals.c` (major rewrite), `screen_settings.c` (mode selector)
**Tests:** New unit tests for style property values. All existing tests pass.

---

### Phase 3: Screen & Widget Refactor (3–4 days)

**Goal:** Replace 489 inline style calls with theme-applied and added styles.

**Approach per file:**
1. Identify which style object (`s_card`, `s_btn`, `s_label`, etc.) matches the existing inline styling
2. Replace the group of inline calls with a single `lv_obj_add_style()` call
3. Keep any property that intentionally differs from the style object as a local override
4. Remove hardcoded hex values, replace with semantic tokens
5. Verify visual output matches pre-refactor appearance

**Order:** Refactor widgets first (shared across screens), then screens:

| Step | File | Estimated inline calls removed |
|------|------|-------------------------------|
| 3.1 | `widget_numeric_display.c` | ~20 of 28 |
| 3.2 | `widget_alarm_banner.c` | ~15 of 22 |
| 3.3 | `widget_nav_bar.c` | ~18 of 26 |
| 3.4 | `widget_waveform.c` | ~10 of 15 |
| 3.5 | `screen_main_vitals.c` | ~20 of 27 |
| 3.6 | `screen_trends.c` | ~30 of 44 |
| 3.7 | `screen_alarms.c` | ~50 of 72 |
| 3.8 | `screen_settings.c` | ~45 of 68 |
| 3.9 | `screen_patient.c` | ~35 of 52 |
| 3.10 | `screen_login.c` | ~40 of 59 |
| 3.11 | `screen_audit_log.c` | ~50 of 73 |
| **Total** | | **~333 of 489 calls removed** |

**Remaining ~156 calls:** Legitimate local overrides (parameter-specific colors, alarm states, dynamic values). These are correct usage of LVGL's style priority system.

**Tests:** Visual regression check (screenshot comparison). All 958 tests pass. Theme mode switching works across all screens.

---

### Phase 4: Visual Enhancements (2–3 days)

**Goal:** Now that the design system is in place, add visual depth and motion.

This is where the original plan's Tiers 1, 3, and partial 5 come in — but now they're trivial because they only require modifying token values and style objects, not touching 13 screen files.

#### 4.1 Elevation & depth

- Add subtle gradient to `s_screen` background style
- Enable `VM_FEATURE_SHADOWS` and apply `VM_SHADOW_LOW` to `s_card`
- Increase `VM_RADIUS_MD` from 8px to 12px (one token change → affects all cards)

```c
/* One change in design_tokens.h → every card gets rounded corners */
#define VM_RADIUS_MD    VM_SCALE_H(12)

/* One change in theme_styles.c → every card gets a shadow */
lv_style_set_shadow_width(&s_card, VM_SHADOW_LOW.width);
lv_style_set_shadow_ofs_y(&s_card, VM_SHADOW_LOW.ofs_y);
/* ... */
```

#### 4.2 Screen transitions

Add `lv_screen_load_anim()` in screen_manager.c with alarm-interrupt override:

```c
if (alarm_engine_has_active_alarm()) {
    lv_screen_load(new_screen);  /* Instant — alarm takes priority */
} else {
    lv_screen_load_anim(new_screen, LV_SCR_LOAD_ANIM_FADE_IN,
                        VM_MOTION_SLOW, 0, true);
}
```

#### 4.3 Button press feedback

Already defined in `s_btn_pressed` style — applied automatically by the theme engine for all buttons.

#### 4.4 Smooth alarm flash

Replace timer-based binary flash with `lv_anim_t` fade (preserving IEC 60601-1-8 timing):

```c
lv_anim_t anim;
lv_anim_init(&anim);
lv_anim_set_var(&anim, alarm_banner);
lv_anim_set_values(&anim, LV_OPA_COVER, VM_OPA_SUBTLE);  /* Never invisible */
lv_anim_set_duration(&anim, VM_ALARM_HIGH_PERIOD_MS / 2);
lv_anim_set_exec_cb(&anim, set_alarm_bg_opa);
lv_anim_set_path_cb(&anim, VM_EASE_STANDARD);
lv_anim_set_reverse_duration(&anim, VM_ALARM_HIGH_PERIOD_MS / 2);
lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
lv_anim_start(&anim);
```

#### 4.5 Value tweening

Animate numeric display value changes over `VM_MOTION_NORMAL` (200ms). Alarm engine always evaluates actual sensor value, not the interpolated display value.

#### 4.6 Nav bar indicator

Add sliding accent bar that follows active tab using `lv_anim_t` with `VM_EASE_STANDARD`.

#### 4.7 Status indicators

Pulsing dot for sensor connection status using `lv_anim_t` opacity cycle.

**Files modified:** `theme_styles.c` (shadow/gradient additions), `screen_manager.c` (transitions), `widget_alarm_banner.c` (smooth flash), `widget_numeric_display.c` (value tween), `widget_nav_bar.c` (slide indicator), `screen_main_vitals.c` (status dots).

---

### Phase 5: Typography Upgrade (2–3 days, optional)

**Goal:** Add font weight variation for visual hierarchy.

1. Choose Option B (Montserrat Bold for 32px + 48px) or Option C (full Inter family)
2. Generate fonts via `lv_font_conv`
3. Place generated `.c` files in `src/ui/fonts/`
4. Update type tokens in `design_tokens.h` (swap font pointers)
5. Update `CMakeLists.txt` to compile font files
6. Update `lv_conf.h` to disable unused built-in sizes

**Zero screen/widget code changes required** — the type token struct indirection means changing the font pointer in `design_tokens.h` propagates to all widgets via the style objects.

---

### Phase 6: Custom Widgets (5–10 days, optional)

**Goal:** Build advanced visualizations on the solid design system foundation.

- Radial gauge widget (`lv_arc`-based) for SpO2/HR
- Sparkline mini-trends inside numeric displays
- Alarm glow effect (supplementary shadow on alarming parameter cards)
- Rich alarm cards with waveform snapshots
- Enhanced waveform with gradient fill

These widgets consume design tokens and style objects. They don't define their own visual language — they inherit from the design system.

---

## 8. Performance Budget

| Metric | Current | Budget | Notes |
|--------|---------|--------|-------|
| Frame rate | 30 FPS | >= 25 FPS | LVGL timer period = 33ms |
| Frame render time | ~12ms | <= 33ms | Measured via `lv_refr_get_fps_avg()` |
| RAM (styles) | ~0 KB (inline) | <= 10 KB | ~25 static `lv_style_t` objects |
| RAM (animations) | ~0.5 KB | <= 5 KB | <= 8 concurrent `lv_anim_t` |
| ROM (fonts) | ~120 KB | <= 300 KB | Only if Phase 5 executed |
| Shadow blur radius | 0 | <= 16px | Behind `VM_FEATURE_SHADOWS` flag |
| Active animations | 2 | <= 8 | Flash + waveform + transitions |

### Profiling plan

1. Enable `LV_USE_PERF_MONITOR 1` in `lv_conf.h` before Phase 4
2. Measure baseline FPS and render time
3. After each sub-phase of Phase 4, re-measure
4. If FPS drops below 25: disable shadows (`VM_FEATURE_SHADOWS`), reduce animation count
5. Enable `LV_USE_MEM_MONITOR 1` to track style object memory

---

## 9. Testing Strategy

### 9.1 Phase 1–2 tests (token & theme integrity)

| Test | Type | Verification |
|------|------|-------------|
| Token compilation | Unit | `design_tokens.h` compiles with no warnings |
| Scheme values | Unit | Assert each field of `vm_scheme_dark` matches expected hex |
| Style property values | Unit | After `vm_styles_init()`, assert `lv_style_get_bg_color(&s_card)` matches scheme |
| Theme apply | Unit | Create `lv_btn`, verify `s_btn` auto-applied |
| Mode switching | Unit | Switch to high-contrast, verify `s_card` bg color updates |
| Alarm color invariance | Unit | Verify alarm colors identical in all 3 modes |
| Backward compat | Compile | Old `VM_COLOR_*` and `VM_FONT_*` macros still resolve |
| Full test suite | Integration | All 958 tests pass |

### 9.2 Phase 3 tests (refactor integrity)

| Test | Type | Verification |
|------|------|-------------|
| Visual regression | Manual | Screenshot comparison pre/post refactor for all 8 screens |
| Token coverage | Grep | Zero hardcoded hex values in screen/widget code |
| Style object usage | Grep | Zero `lv_obj_set_style_bg_color()` calls with hardcoded colors |
| Theme modes | Manual | All screens viewed in all 3 modes, no visual glitches |
| Full test suite | Integration | All 958 tests pass |

### 9.3 Phase 4 tests (visual enhancement)

| Test | Type | Verification |
|------|------|-------------|
| Alarm flash timing | Unit | Measure animation period: HIGH in 1.4–2.8 Hz, MEDIUM in 0.4–0.8 Hz |
| Alarm visibility | Manual | Alarm banner is most prominent element in all modes |
| Transition + alarm | Integration | Trigger alarm during screen fade — alarm renders immediately |
| Value tween accuracy | Unit | Displayed value matches actual value within 300ms |
| Touch targets | Manual | All buttons respond; none below 44px |
| Performance | Automated | FPS >= 25 with all animations active |
| Contrast ratios | Automated | All text/bg pairs meet WCAG AA minimum |

---

## 10. Risk Register

| # | Risk | Prob. | Impact | Mitigation |
|---|------|-------|--------|------------|
| R1 | Shadow rendering drops FPS < 25 | Medium | High | `VM_FEATURE_SHADOWS` compile flag; tonal elevation fallback |
| R2 | Smooth alarm flash fails IEC timing | Low | Critical | Timing validation test; binary flash fallback preserved |
| R3 | Theme mode switching causes flicker | Medium | Low | Full invalidation after style reinit; test all screens |
| R4 | Screen transition masks alarm onset | Medium | Critical | Alarm-interrupt cancels transition; integration test |
| R5 | Refactor changes visual appearance | Medium | Medium | Screenshot comparison before/after each file |
| R6 | Custom fonts exceed ROM budget | Low | Medium | ASCII subset only; `--bpp 2` fallback |
| R7 | Gradient banding on RGB565 | Medium | Low | `LV_DITHER_GRADIENT 1`; limit gradient range |
| R8 | Old code references removed `VM_COLOR_*` macros | High | Low | Keep deprecated aliases during migration |
| R9 | `lv_style_init()` called multiple times on mode switch | Medium | Medium | Track init state; `lv_style_reset()` before reinit |
| R10 | Regressions in existing 958 tests | Low | High | Full suite after each phase; CI gate |

---

## 11. File Impact Map

### New files

| File | Phase | Purpose |
|------|-------|---------|
| `src/ui/themes/design_tokens.h` | 1 | All token definitions |
| `src/ui/themes/theme_styles.h` | 2 | `lv_style_t` extern declarations |
| `src/ui/themes/theme_styles.c` | 2 | Style object initialization |
| `src/ui/fonts/*.c` | 5 | Generated custom font files (optional) |
| `src/ui/widgets/widget_gauge.h/.c` | 6 | Radial gauge widget (optional) |
| `src/ui/widgets/widget_sparkline.h/.c` | 6 | Inline mini-trend widget (optional) |

### Modified files

| File | Phase(s) | Changes |
|------|----------|---------|
| `src/ui/themes/theme_vitals.h` | 1, 2 | Include tokens, theme mode API, deprecation aliases |
| `src/ui/themes/theme_vitals.c` | 2 | Full rewrite — `lv_theme_t` engine |
| `src/ui/widgets/widget_numeric_display.c` | 3, 4, 6 | Style refactor → value tween → sparkline |
| `src/ui/widgets/widget_alarm_banner.c` | 3, 4 | Style refactor → smooth flash |
| `src/ui/widgets/widget_nav_bar.c` | 3, 4 | Style refactor → slide indicator |
| `src/ui/widgets/widget_waveform.c` | 3, 4 | Style refactor → gradient fill |
| `src/ui/screens/screen_main_vitals.c` | 3, 4 | Style refactor → status dots |
| `src/ui/screens/screen_trends.c` | 3 | Style refactor |
| `src/ui/screens/screen_alarms.c` | 3, 6 | Style refactor → rich cards |
| `src/ui/screens/screen_settings.c` | 2, 3 | Theme mode selector → style refactor |
| `src/ui/screens/screen_patient.c` | 3 | Style refactor |
| `src/ui/screens/screen_login.c` | 3 | Style refactor |
| `src/ui/screens/screen_audit_log.c` | 3 | Style refactor |
| `src/ui/screens/screen_manager.c` | 4 | Screen transitions |
| `simulator/lv_conf.h` | 2, 4, 5 | Gradient dithering, perf monitor, font config |
| `simulator/CMakeLists.txt` | 2, 5, 6 | New source files |
| `tests/unit/CMakeLists.txt` | 2 | Token/theme tests |

---

## 12. Reference Material

### Design System Theory

- [Material Design 3 — Design Tokens](https://m3.material.io/foundations/design-tokens)
- [Material Design 3 — Color Roles](https://m3.material.io/styles/color/roles)
- [Material Design 3 — Typography Tokens](https://m3.material.io/styles/typography/type-scale-tokens)
- [Material Design 3 — Elevation](https://m3.material.io/styles/elevation/applying-elevation)
- [Material Design 3 — Motion Tokens](https://m3.material.io/styles/motion/easing-and-duration/tokens-specs)
- [IBM Carbon — Color System](https://v10.carbondesignsystem.com/guidelines/color/overview/)
- [IBM Carbon — Typography](https://v10.carbondesignsystem.com/guidelines/typography/overview/)
- [IBM Carbon — Spacing](https://v9.carbondesignsystem.com/guidelines/spacing/)
- [Martin Fowler — Design Token Architecture](https://martinfowler.com/articles/design-token-based-ui-architecture.html)
- [W3C Design Tokens Community Group](https://goodpractices.design/articles/design-tokens)

### LVGL v9 Documentation

- [Themes](https://docs.lvgl.io/master/common-widget-features/styles/themes.html) — `lv_theme_t` API
- [Styles](https://docs.lvgl.io/master/common-widget-features/styles/style-properties.html) — All style properties
- [Parts & States](https://docs.lvgl.io/master/details/common-widget-features/parts_and_states.html) — Style selectors
- [Animation](https://docs.lvgl.io/master/main-modules/animation.html) — `lv_anim_t` API
- [Screens](https://docs.lvgl.io/master/common-widget-features/screens.html) — `lv_screen_load_anim()` options
- [Font Converter](https://github.com/lvgl/lv_font_conv) — `lv_font_conv` tool
- [Theme System Internals](https://deepwiki.com/lvgl/lvgl/3.3-style-and-theme-system) — DeepWiki analysis
- [Custom Theme Issue #6746](https://github.com/lvgl/lvgl/issues/6746) — `lvgl_private.h` requirement

### Regulatory

- [IEC 60601-1-8 Alarm Guidance (DigiKey)](https://www.digikey.com/en/articles/iec-60601-1-8-guidance-for-designing-medical-equipment-alarms)
- [Medical Alarm Systems Guide (Same Sky)](https://www.sameskydevices.com/blog/a-guide-to-iec-60601-1-8-and-medical-alarm-systems)
- [Demystifying Medical Alarms (TI)](https://www.ti.com/lit/pdf/sszt261)
- [IEC 60601-1-8 Conformity (Johner)](https://www.johner-institute.com/articles/product-development/and-more/iec-60601-1-8/)
- [WCAG 2.1 Contrast Requirements](https://www.w3.org/WAI/WCAG21/Understanding/contrast-minimum.html)
- [ISO 9241-391 Photosensitive Seizures](https://www.iso.org/standard/56350.html)

### LVGL v9 Implementation Notes

- `lv_theme_t` struct is **private** in v9 — requires `#include "lvgl_private.h"`
- `LV_GRADIENT_MAX_STOPS` defaults to 2 — increase if 3+ gradient stops needed
- `LV_USE_DRAW_SW_COMPLEX_GRADIENTS` must be enabled for radial/conical gradients
- `lv_style_reset()` before `lv_style_init()` when reinitializing on theme mode switch
- Screen transition animations disable input events during playback
- Only one animation per object+executor pair runs simultaneously
- `lv_color_hex()` is a function, not a macro — cannot be used in static initializers
- Use `LV_COLOR_MAKE(r, g, b)` for compile-time color constants where needed
