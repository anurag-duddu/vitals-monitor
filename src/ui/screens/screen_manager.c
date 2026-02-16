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

    /* UI-1.3: Duplicate screen push guard */
    if (nav_stack_top >= 0 && nav_stack[nav_stack_top] == id) {
        printf("[screen_mgr] WARNING: Screen %s already on top, ignoring duplicate push\n",
               screen_registry[id].name);
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

void screen_manager_replace_current(screen_id_t id) {
    if (id >= SCREEN_ID_COUNT || !screen_registered[id]) {
        printf("[screen_mgr] Cannot replace with unregistered screen %d\n", id);
        return;
    }

    /* Destroy the current screen if there is one */
    if (nav_stack_top >= 0) {
        screen_id_t current = nav_stack[nav_stack_top];
        if (screen_registry[current].destroy) {
            screen_registry[current].destroy();
        }
        /* Replace top of stack atomically */
        nav_stack[nav_stack_top] = id;
    } else {
        /* Stack is empty, just push */
        nav_stack_top = 0;
        nav_stack[0] = id;
    }

    printf("[screen_mgr] Replace -> %s (depth=%d)\n",
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
    /* REL-4.2: Validate home screen registration before navigating */
    if (!screen_registered[SCREEN_ID_MAIN_VITALS]) {
        printf("[screen_mgr] CRITICAL: Home screen (MAIN_VITALS) not registered, cannot go home\n");
        return;
    }

    /* UI-1.5: Destroy ALL intermediate screens from top down to index 1
     * (index 0 is the home screen entry, which we replace) */
    for (int32_t i = nav_stack_top; i > 0; i--) {
        screen_id_t scr = nav_stack[i];
        if (screen_registered[scr] && screen_registry[scr].destroy) {
            screen_registry[scr].destroy();
        }
    }
    /* Also destroy screen at index 0 if it's not already home */
    if (nav_stack_top >= 0) {
        screen_id_t bottom = nav_stack[0];
        if (bottom != SCREEN_ID_MAIN_VITALS && screen_registered[bottom] && screen_registry[bottom].destroy) {
            screen_registry[bottom].destroy();
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

void screen_manager_go_home_then_push(screen_id_t target) {
    if (target >= SCREEN_ID_COUNT || !screen_registered[target]) {
        printf("[screen_mgr] Cannot go_home_then_push unregistered screen %d\n", target);
        return;
    }
    if (!screen_registered[SCREEN_ID_MAIN_VITALS]) {
        printf("[screen_mgr] CRITICAL: Home screen not registered\n");
        return;
    }

    /* Destroy all current screens from top down */
    for (int32_t i = nav_stack_top; i >= 0; i--) {
        screen_id_t scr = nav_stack[i];
        if (screen_registered[scr] && screen_registry[scr].destroy) {
            screen_registry[scr].destroy();
        }
    }

    /* Set stack to [home, target] — home is conceptual base, only target is loaded */
    nav_stack[0] = SCREEN_ID_MAIN_VITALS;
    nav_stack[1] = target;
    nav_stack_top = 1;

    printf("[screen_mgr] GoHomeThenPush -> %s (depth=2)\n",
           screen_registry[target].name);

    load_screen(target);
    reset_auto_return();
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

    /* UI-1.4: Create the new screen BEFORE any transition */
    lv_obj_t *new_scr = screen_registry[id].create();
    if (!new_scr) {
        printf("[screen_mgr] Screen create returned NULL for %s\n",
               screen_registry[id].name);
        return;
    }

    /* UI-1.4: Disable clickability on old screen during transition for safety */
    lv_obj_t *old_scr = lv_screen_active();
    if (old_scr) {
        lv_obj_remove_flag(old_scr, LV_OBJ_FLAG_CLICKABLE);
    }

    /* PERF-7.2: Use no animation for initial screen load (no previous screen) */
    if (nav_stack_top == 0 && old_scr == NULL) {
        lv_screen_load_anim(new_scr, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
    } else {
        /* Load with fade animation; auto-delete old screen */
        lv_screen_load_anim(new_scr, LV_SCR_LOAD_ANIM_FADE_IN, 200, 0, true);
    }
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
