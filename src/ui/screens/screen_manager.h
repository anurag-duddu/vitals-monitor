/**
 * @file screen_manager.h
 * @brief Stack-based screen navigation manager
 *
 * Provides push/pop navigation between screens with auto-return to the
 * main vitals screen after a configurable timeout. Screens are created
 * fresh on each navigation and auto-deleted by LVGL on transition.
 */

#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include "lvgl.h"
#include <stdbool.h>

/* Maximum navigation stack depth (PRD allows max 2 levels; 8 is generous) */
#define SCREEN_STACK_MAX_DEPTH  8

/* Screen identifiers */
typedef enum {
    SCREEN_ID_MAIN_VITALS = 0,
    SCREEN_ID_TRENDS,
    SCREEN_ID_ALARMS,
    SCREEN_ID_PATIENT,
    SCREEN_ID_SETTINGS,
    SCREEN_ID_COUNT
} screen_id_t;

/* Callback: create a screen, return the lv_obj_t* created via lv_obj_create(NULL) */
typedef lv_obj_t * (*screen_create_fn_t)(void);

/* Callback: optional cleanup before screen is deleted */
typedef void (*screen_destroy_fn_t)(void);

/* Screen registration entry */
typedef struct {
    screen_id_t         id;
    screen_create_fn_t  create;
    screen_destroy_fn_t destroy;   /* NULL if no cleanup needed */
    const char         *name;      /* For logging/debug */
} screen_reg_t;

/** Initialize the screen manager. Call once at startup. */
void screen_manager_init(void);

/** Register a screen. Must be called before navigating to that screen. */
void screen_manager_register(const screen_reg_t *reg);

/** Navigate to a screen by ID (pushes onto stack). */
void screen_manager_push(screen_id_t id);

/** Go back to the previous screen (pops stack). No-op if at root. */
void screen_manager_pop(void);

/** Go directly to main vitals screen (clears stack). */
void screen_manager_go_home(void);

/** Get the currently active screen ID. */
screen_id_t screen_manager_get_active(void);

/**
 * Enable auto-return timer. If user stays on a non-home screen for
 * timeout_ms milliseconds, automatically navigate home.
 * Pass 0 to disable.
 */
void screen_manager_set_auto_return(uint32_t timeout_ms);

#endif /* SCREEN_MANAGER_H */
