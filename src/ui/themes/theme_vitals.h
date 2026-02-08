/**
 * @file theme_vitals.h
 * @brief Color palette, layout constants, font references, and alarm severity
 *
 * Central definition file for the vitals monitor visual design system.
 * All colors follow IEC 60601-1-8 and medical device industry conventions.
 *
 * Layout constants are derived from VM_SCREEN_WIDTH / VM_SCREEN_HEIGHT
 * via scale macros, so changing the target resolution automatically adapts
 * padding, bar heights, and font selections.
 */

#ifndef THEME_VITALS_H
#define THEME_VITALS_H

#include "lvgl.h"

/* ============================================================
 *  Screen Dimensions (configure for target hardware)
 * ============================================================ */

#define VM_SCREEN_WIDTH   800
#define VM_SCREEN_HEIGHT  480

/* ============================================================
 *  Responsive Scale Macros
 *
 *  VM_SCALE_W(px) / VM_SCALE_H(px) express a design-time pixel
 *  value relative to the 800×480 reference resolution.  At the
 *  reference size the macros evaluate to the same value; at other
 *  sizes they scale proportionally.  All arithmetic is integer and
 *  resolved at compile time via constant folding.
 * ============================================================ */

#define VM_SCALE_W(px)  ((int32_t)((px) * VM_SCREEN_WIDTH  / 800))
#define VM_SCALE_H(px)  ((int32_t)((px) * VM_SCREEN_HEIGHT / 480))

/* ============================================================
 *  Functional Colors
 * ============================================================ */

/* Background colors */
#define VM_COLOR_BG              lv_color_hex(0x0A0A0A)  /* Near-black screen bg */
#define VM_COLOR_BG_PANEL        lv_color_hex(0x1A1A1A)  /* Slightly lighter panel bg */
#define VM_COLOR_BG_PANEL_BORDER lv_color_hex(0x333333)  /* Panel border */

/* Alarm severity colors (IEC 60601-1-8) */
#define VM_COLOR_ALARM_HIGH      lv_color_hex(0xFF0000)  /* Red - critical */
#define VM_COLOR_ALARM_MEDIUM    lv_color_hex(0xFFCC00)  /* Yellow - warning */
#define VM_COLOR_ALARM_LOW       lv_color_hex(0x00CCFF)  /* Cyan - advisory */
#define VM_COLOR_ALARM_NONE      lv_color_hex(0x00CC00)  /* Green - normal */

/* Vital sign parameter colors (industry convention) */
#define VM_COLOR_HR              lv_color_hex(0x00CC00)  /* Green - ECG/HR */
#define VM_COLOR_SPO2            lv_color_hex(0x00CCFF)  /* Cyan - SpO2/Pleth */
#define VM_COLOR_NIBP            lv_color_hex(0xFFFFFF)  /* White - Blood pressure */
#define VM_COLOR_TEMP            lv_color_hex(0xFF8800)  /* Orange - Temperature */
#define VM_COLOR_RR              lv_color_hex(0xFFFF00)  /* Yellow - Respiration */

/* Text colors */
#define VM_COLOR_TEXT_PRIMARY    lv_color_hex(0xFFFFFF)
#define VM_COLOR_TEXT_SECONDARY  lv_color_hex(0xAAAAAA)
#define VM_COLOR_TEXT_DISABLED   lv_color_hex(0x666666)

/* ============================================================
 *  Layout Constants (scaled from 800×480 reference)
 * ============================================================ */

#define VM_ALARM_BAR_HEIGHT      VM_SCALE_H(32)
#define VM_NAV_BAR_HEIGHT        VM_SCALE_H(48)
#define VM_CONTENT_HEIGHT        (VM_SCREEN_HEIGHT - VM_ALARM_BAR_HEIGHT - VM_NAV_BAR_HEIGHT)

#define VM_WAVEFORM_WIDTH_PCT    60   /* % of content width for waveforms */
#define VM_VITALS_WIDTH_PCT      40   /* % of content width for vitals panel */

/* Padding and spacing (scaled) */
#define VM_PAD_TINY              VM_SCALE_H(2)
#define VM_PAD_SMALL             VM_SCALE_H(4)
#define VM_PAD_NORMAL            VM_SCALE_H(8)
#define VM_PAD_LARGE             VM_SCALE_H(16)

/* Touch target minimum (10mm physical, ~44px at 130 DPI) */
#define VM_TOUCH_MIN             VM_SCALE_H(44)

/* Waveform display */
#define VM_WAVEFORM_POINT_COUNT      450   /* Data points per chart (~pixel width) */
#define VM_WAVEFORM_ERASE_WIDTH      8     /* Gap points ahead of sweep head */
#define VM_WAVEFORM_ECG_HEIGHT_PCT   55    /* % of waveform area for ECG */
#define VM_WAVEFORM_PLETH_HEIGHT_PCT 45    /* % of waveform area for Pleth */

/* ============================================================
 *  Font References (tier-selected by screen height)
 *
 *  Each tier picks the best available Montserrat size for the
 *  semantic role.  Only the VM_SCREEN_HEIGHT define needs to
 *  change to adapt the entire font scale.
 * ============================================================ */

#if (VM_SCREEN_HEIGHT >= 480)
    /* Full-size tier (800×480 target) */
    #define VM_FONT_VALUE_LARGE   &lv_font_montserrat_48
    #define VM_FONT_VALUE_MEDIUM  &lv_font_montserrat_32
    #define VM_FONT_LABEL         &lv_font_montserrat_20
    #define VM_FONT_BODY          &lv_font_montserrat_16
    #define VM_FONT_CAPTION       &lv_font_montserrat_14
    #define VM_FONT_SMALL         &lv_font_montserrat_12
    #define VM_FONT_UNIT          &lv_font_montserrat_12
#elif (VM_SCREEN_HEIGHT >= 320)
    /* Medium tier (e.g. 480×320) */
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
 *  API
 * ============================================================ */

/** Initialize the vitals monitor dark theme on the active display. */
void theme_vitals_init(void);

/** Get the LVGL color for a given alarm severity. */
lv_color_t theme_vitals_alarm_color(vm_alarm_severity_t severity);

#endif /* THEME_VITALS_H */
