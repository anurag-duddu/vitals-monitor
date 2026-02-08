/**
 * @file screen_manager.c
 * @brief Stack-based screen navigation implementation
 */

#include "screen_manager.h"
#include <stdio.h>
#include <string.h>

/* Navigation stack */
static screen_id_t nav_stack[SCREEN_STACK_MAX_DEPTH];
static int32_t     nav_stack_top = -1;

/* Screen registry */
static screen_reg_t screen_registry[SCREEN_ID_COUNT];
static bool         screen_registered[SCREEN_ID_COUNT];

/* Auto-return timer */
static lv_timer_t *auto_return_timer = NULL;
static uint32_t    auto_return_timeout_ms = 0;

/* ── Forward declarations ──────────────────────────────────── */

static void load_screen(screen_id_t id);
static void auto_return_cb(lv_timer_t *timer);
static void reset_auto_return(void);

/* ── Public API ────────────────────────────────────────────── */

void screen_manager_init(void) {
    nav_stack_top = -1;
    memset(screen_registered, 0, sizeof(screen_registered));
    auto_return_timer = NULL;
    auto_return_timeout_ms = 0;
}

void screen_manager_register(const screen_reg_t *reg) {
    if (!reg || reg->id >= SCREEN_ID_COUNT) return;

    screen_registry[reg->id] = *reg;
    screen_registered[reg->id] = true;
    printf("[screen_mgr] Registered screen: %s (id=%d)\n", reg->name, reg->id);
}

void screen_manager_push(screen_id_t id) {
    if (id >= SCREEN_ID_COUNT || !screen_registered[id]) {
        printf("[screen_mgr] Cannot push unregistered screen %d\n", id);
        return;
    }

    if (nav_stack_top >= SCREEN_STACK_MAX_DEPTH - 1) {
        printf("[screen_mgr] Navigation stack full, cannot push\n");
        return;
    }

    /* Call destroy on current screen if there is one */
    if (nav_stack_top >= 0) {
        screen_id_t current = nav_stack[nav_stack_top];
        if (screen_registry[current].destroy) {
            screen_registry[current].destroy();
        }
    }

    nav_stack_top++;
    nav_stack[nav_stack_top] = id;

    printf("[screen_mgr] Push -> %s (depth=%d)\n",
           screen_registry[id].name, nav_stack_top + 1);

    load_screen(id);
    reset_auto_return();
}

void screen_manager_pop(void) {
    if (nav_stack_top <= 0) {
        /* Already at root or empty — do nothing */
        return;
    }

    /* Call destroy on current screen */
    screen_id_t current = nav_stack[nav_stack_top];
    if (screen_registry[current].destroy) {
        screen_registry[current].destroy();
    }

    nav_stack_top--;
    screen_id_t prev = nav_stack[nav_stack_top];

    printf("[screen_mgr] Pop -> %s (depth=%d)\n",
           screen_registry[prev].name, nav_stack_top + 1);

    load_screen(prev);
    reset_auto_return();
}

void screen_manager_go_home(void) {
    if (nav_stack_top >= 0) {
        screen_id_t current = nav_stack[nav_stack_top];
        if (screen_registry[current].destroy) {
            screen_registry[current].destroy();
        }
    }

    /* Reset stack to just home */
    nav_stack_top = 0;
    nav_stack[0] = SCREEN_ID_MAIN_VITALS;

    printf("[screen_mgr] Go home -> Main Vitals\n");

    load_screen(SCREEN_ID_MAIN_VITALS);

    /* Stop auto-return timer (we're already home) */
    if (auto_return_timer) {
        lv_timer_pause(auto_return_timer);
    }
}

screen_id_t screen_manager_get_active(void) {
    if (nav_stack_top < 0) return SCREEN_ID_MAIN_VITALS;
    return nav_stack[nav_stack_top];
}

void screen_manager_set_auto_return(uint32_t timeout_ms) {
    auto_return_timeout_ms = timeout_ms;

    if (timeout_ms == 0) {
        /* Disable auto-return */
        if (auto_return_timer) {
            lv_timer_delete(auto_return_timer);
            auto_return_timer = NULL;
        }
        return;
    }

    if (!auto_return_timer) {
        auto_return_timer = lv_timer_create(auto_return_cb, timeout_ms, NULL);
        lv_timer_pause(auto_return_timer);
    } else {
        lv_timer_set_period(auto_return_timer, timeout_ms);
    }
}

/* ── Private helpers ───────────────────────────────────────── */

static void load_screen(screen_id_t id) {
    if (!screen_registered[id] || !screen_registry[id].create) return;

    lv_obj_t *new_scr = screen_registry[id].create();
    if (!new_scr) {
        printf("[screen_mgr] Screen create returned NULL for %s\n",
               screen_registry[id].name);
        return;
    }

    /* Load with fade animation; auto-delete old screen */
    lv_screen_load_anim(new_scr, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, true);
}

static void auto_return_cb(lv_timer_t *timer) {
    (void)timer;

    if (screen_manager_get_active() != SCREEN_ID_MAIN_VITALS) {
        printf("[screen_mgr] Auto-return timeout, going home\n");
        screen_manager_go_home();
    }
}

static void reset_auto_return(void) {
    if (!auto_return_timer || auto_return_timeout_ms == 0) return;

    if (screen_manager_get_active() == SCREEN_ID_MAIN_VITALS) {
        /* No need for auto-return when we're already home */
        lv_timer_pause(auto_return_timer);
    } else {
        lv_timer_reset(auto_return_timer);
        lv_timer_resume(auto_return_timer);
    }
}
