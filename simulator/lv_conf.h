/**
 * @file lv_conf.h
 * Configuration file for LVGL
 * For LVGL v9.3
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/

/* Color depth: 1 (1 byte per pixel), 8 (RGB332), 16 (RGB565), 32 (ARGB8888) */
#define LV_COLOR_DEPTH 32

/*=========================
   MEMORY SETTINGS
 *=========================*/

/* Size of the memory available for LVGL's internal management purposes */
#define LV_MEM_SIZE (256 * 1024U)  /* 256KB - increased for trend charts + overlays */

/* Set an address for the memory pool instead of allocating it as a normal array */
#define LV_MEM_ADR 0     /* 0: unused */

/* Use the standard `memcpy` and `memset` instead of LVGL's own functions */
#define LV_MEMCPY_MEMSET_STL 1

/*====================
   HAL SETTINGS
 *====================*/

/* Default display refresh period in milliseconds. Can be changed in the display driver */
#define LV_DEF_REFR_PERIOD 33    /* 30 FPS */

/* Dot Per Inch: used to initialize default sizes such as widgets sized, style paddings, etc. */
#define LV_DPI_DEF 130     /* Common value for 4" displays */

/*=================
   FONT SETTINGS
 *=================*/

/* Montserrat fonts with various sizes */
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 0
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 1

/* Set a default font */
#define LV_FONT_DEFAULT &lv_font_montserrat_16

/*=================
   THEME SETTINGS
 *=================*/

/* Enable built-in theme(s) */
#define LV_USE_THEME_DEFAULT 1

/*=====================
   WIDGET SETTINGS
 *=====================*/

#define LV_USE_ANIMIMG          1
#define LV_USE_ARC              1
#define LV_USE_BAR              1
#define LV_USE_BTN              1
#define LV_USE_BTNMATRIX        1
#define LV_USE_CALENDAR         0
#define LV_USE_CANVAS           1  /* Needed for waveforms */
#define LV_USE_CHART            1  /* Needed for trends */
#define LV_USE_CHECKBOX         1
#define LV_USE_DROPDOWN         1
#define LV_USE_IMG              1
#define LV_USE_IMGBTN           1
#define LV_USE_KEYBOARD         1
#define LV_USE_LABEL            1
#define LV_USE_LED              1
#define LV_USE_LINE             1
#define LV_USE_LIST             1
#define LV_USE_MENU             1
#define LV_USE_MSGBOX           1
#define LV_USE_ROLLER           1
#define LV_USE_SCALE            1
#define LV_USE_SLIDER           1
#define LV_USE_SPAN             1
#define LV_USE_SPINBOX          1
#define LV_USE_SPINNER          1
#define LV_USE_SWITCH           1
#define LV_USE_TABLE            1
#define LV_USE_TABVIEW          1
#define LV_USE_TEXTAREA         1
#define LV_USE_WIN              1

/*==================
   LAYOUT ENGINES
 *==================*/

#define LV_USE_FLEX 1
#define LV_USE_GRID 1

/*==================
   OTHERS
 *==================*/

/* Enable filesystem support */
#define LV_USE_FS_STDIO 0

/* Enable logging */
#define LV_USE_LOG 1
#if LV_USE_LOG
  #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN  /* LV_LOG_LEVEL_TRACE, LV_LOG_LEVEL_INFO, LV_LOG_LEVEL_WARN, LV_LOG_LEVEL_ERROR, LV_LOG_LEVEL_USER, LV_LOG_LEVEL_NONE */
  #define LV_LOG_PRINTF 1  /* Use printf for logging */
#endif  /*LV_USE_LOG*/

/* Performance monitor (LVGL v9 uses LV_USE_SYSMON) */
#define LV_USE_SYSMON 0
#define LV_USE_PERF_MONITOR 0

#endif /*LV_CONF_H*/
