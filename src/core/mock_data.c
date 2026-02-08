/**
 * @file mock_data.c
 * @brief Mock sensor data generator implementation
 *
 * Each parameter uses a random walk with mean-reversion bias:
 *   new_value = current + random_drift - reversion_toward_base
 *
 * NIBP simulates periodic (non-continuous) measurements.
 */

#include "mock_data.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* ── Simulation parameters ─────────────────────────────────── */

typedef struct {
    int base;
    int current;
    int min;
    int max;
    int drift_max;  /* Max change per tick */
} int_sim_t;

typedef struct {
    float base;
    float current;
    float min;
    float max;
    float drift_max;
} float_sim_t;

/* Parameter simulators */
static int_sim_t   hr_sim   = { .base = 72,  .current = 72,  .min = 45,  .max = 160, .drift_max = 3 };
static int_sim_t   spo2_sim = { .base = 97,  .current = 97,  .min = 85,  .max = 100, .drift_max = 1 };
static int_sim_t   rr_sim   = { .base = 16,  .current = 16,  .min = 8,   .max = 35,  .drift_max = 1 };
static float_sim_t temp_sim = { .base = 36.8f, .current = 36.8f, .min = 35.0f, .max = 40.0f, .drift_max = 0.1f };

/* NIBP state */
static int_sim_t   nibp_sys_sim = { .base = 120, .current = 120, .min = 80,  .max = 200, .drift_max = 5 };
static int_sim_t   nibp_dia_sim = { .base = 80,  .current = 80,  .min = 40,  .max = 120, .drift_max = 3 };
static int          nibp_tick_counter = 0;
#define NIBP_INTERVAL_TICKS 60  /* Measure every ~60 seconds at 1s tick rate */

/* Current data snapshot */
static vitals_data_t current_data;

/* History ring buffer for trends */
static vitals_history_t history;

/* Alarm log */
static alarm_log_t alarm_log;
static uint32_t tick_counter_s = 0;  /* Seconds since mock_data_init */

/* LVGL timer and callback */
static lv_timer_t   *data_timer = NULL;
static mock_data_cb_t user_callback = NULL;

/* ── Forward declarations ──────────────────────────────────── */

static void timer_cb(lv_timer_t *timer);
static int  step_int(int_sim_t *sim);
static float step_float(float_sim_t *sim);
static int  clamp_int(int val, int lo, int hi);
static float clamp_float(float val, float lo, float hi);

/* ── Public API ────────────────────────────────────────────── */

void mock_data_init(void) {
    srand((unsigned int)time(NULL));

    /* Initialize current data with base values */
    current_data.hr       = hr_sim.base;
    current_data.spo2     = spo2_sim.base;
    current_data.nibp_sys = nibp_sys_sim.base;
    current_data.nibp_dia = nibp_dia_sim.base;
    current_data.nibp_map = (nibp_sys_sim.base + 2 * nibp_dia_sim.base) / 3;
    current_data.temp     = temp_sim.base;
    current_data.rr       = rr_sim.base;
    current_data.nibp_fresh = true;

    nibp_tick_counter = 0;

    /* Clear history and alarm log */
    memset(&history, 0, sizeof(history));
    memset(&alarm_log, 0, sizeof(alarm_log));
    tick_counter_s = 0;

    printf("[mock_data] Initialized (HR=%d, SpO2=%d, Temp=%.1f, RR=%d, NIBP=%d/%d)\n",
           current_data.hr, current_data.spo2, (double)current_data.temp,
           current_data.rr, current_data.nibp_sys, current_data.nibp_dia);
}

void mock_data_start(uint32_t update_interval_ms) {
    if (data_timer) {
        lv_timer_delete(data_timer);
    }
    data_timer = lv_timer_create(timer_cb, update_interval_ms, NULL);
    printf("[mock_data] Started (interval=%u ms)\n", update_interval_ms);
}

void mock_data_stop(void) {
    if (data_timer) {
        lv_timer_delete(data_timer);
        data_timer = NULL;
    }
    printf("[mock_data] Stopped\n");
}

const vitals_data_t * mock_data_get_current(void) {
    return &current_data;
}

void mock_data_set_callback(mock_data_cb_t cb) {
    user_callback = cb;
}

/* ── Private helpers ───────────────────────────────────────── */

static void timer_cb(lv_timer_t *timer) {
    (void)timer;

    /* Step continuous parameters */
    current_data.hr   = step_int(&hr_sim);
    current_data.spo2 = step_int(&spo2_sim);
    current_data.rr   = step_int(&rr_sim);
    current_data.temp = step_float(&temp_sim);

    /* NIBP — periodic measurement */
    nibp_tick_counter++;
    if (nibp_tick_counter >= NIBP_INTERVAL_TICKS) {
        nibp_tick_counter = 0;
        current_data.nibp_sys = step_int(&nibp_sys_sim);
        current_data.nibp_dia = step_int(&nibp_dia_sim);

        /* MAP = (SYS + 2 * DIA) / 3 */
        current_data.nibp_map = (current_data.nibp_sys + 2 * current_data.nibp_dia) / 3;
        current_data.nibp_fresh = true;
    } else {
        current_data.nibp_fresh = false;
    }

    /* Record into history ring buffer */
    int idx = history.write_idx;
    history.hr[idx]   = current_data.hr;
    history.spo2[idx] = current_data.spo2;
    history.rr[idx]   = current_data.rr;
    history.write_idx = (idx + 1) % MOCK_DATA_HISTORY_LEN;
    history.count++;

    tick_counter_s++;

    /* Notify callback */
    if (user_callback) {
        user_callback(&current_data);
    }
}

/**
 * Random walk with mean-reversion for integer parameters.
 * The bias toward base prevents values from drifting to extremes.
 */
static int step_int(int_sim_t *sim) {
    /* Random drift: [-drift_max, +drift_max] */
    int drift = (rand() % (2 * sim->drift_max + 1)) - sim->drift_max;

    /* Mean-reversion bias: pull 20% toward base */
    int reversion = (sim->base - sim->current) / 5;

    sim->current = clamp_int(sim->current + drift + reversion, sim->min, sim->max);
    return sim->current;
}

static float step_float(float_sim_t *sim) {
    /* Random drift: [-drift_max, +drift_max] */
    float drift = ((float)(rand() % 201) / 100.0f - 1.0f) * sim->drift_max;

    /* Mean-reversion bias */
    float reversion = (sim->base - sim->current) * 0.2f;

    sim->current = clamp_float(sim->current + drift + reversion, sim->min, sim->max);

    /* Round to 0.1 */
    sim->current = ((int)(sim->current * 10.0f + 0.5f)) / 10.0f;
    return sim->current;
}

static int clamp_int(int val, int lo, int hi) {
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

static float clamp_float(float val, float lo, float hi) {
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

/* ── History & Alarm Log API ──────────────────────────────── */

const vitals_history_t * mock_data_get_history(void) {
    return &history;
}

const alarm_log_t * mock_data_get_alarm_log(void) {
    return &alarm_log;
}

void mock_data_log_alarm(vm_alarm_severity_t severity, const char *message,
                          const char *time_str) {
    int idx = alarm_log.write_idx;
    alarm_log.entries[idx].severity = severity;
    strncpy(alarm_log.entries[idx].message, message,
            sizeof(alarm_log.entries[idx].message) - 1);
    alarm_log.entries[idx].message[sizeof(alarm_log.entries[idx].message) - 1] = '\0';
    strncpy(alarm_log.entries[idx].time_str, time_str,
            sizeof(alarm_log.entries[idx].time_str) - 1);
    alarm_log.entries[idx].time_str[sizeof(alarm_log.entries[idx].time_str) - 1] = '\0';
    alarm_log.entries[idx].timestamp_s = tick_counter_s;

    alarm_log.write_idx = (idx + 1) % ALARM_LOG_MAX;
    if (alarm_log.count < ALARM_LOG_MAX) {
        alarm_log.count++;
    }
}
