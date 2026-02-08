/**
 * @file mock_data.h
 * @brief Mock sensor data generator for simulator development
 *
 * Produces realistic vital sign values using random walk with mean-reversion.
 * Uses LVGL timer for periodic updates. In production (Phase 6+), this module
 * is replaced by IPC subscriptions, but the callback interface is identical.
 */

#ifndef MOCK_DATA_H
#define MOCK_DATA_H

#include "lvgl.h"
#include "theme_vitals.h"
#include <stdbool.h>

/* Complete vital signs snapshot */
typedef struct {
    int     hr;         /* Heart rate (bpm) */
    int     spo2;       /* SpO2 (%) */
    int     nibp_sys;   /* Systolic BP (mmHg) */
    int     nibp_dia;   /* Diastolic BP (mmHg) */
    int     nibp_map;   /* Mean arterial pressure (mmHg) */
    float   temp;       /* Temperature (Celsius) */
    int     rr;         /* Respiratory rate (breaths/min) */
    bool    nibp_fresh; /* True when NIBP just measured this tick */
} vitals_data_t;

/** Callback type — called each tick with new data. */
typedef void (*mock_data_cb_t)(const vitals_data_t *data);

/** Initialize the mock data generator (seeds RNG). */
void mock_data_init(void);

/** Start generating data at update_interval_ms (creates LVGL timer). */
void mock_data_start(uint32_t update_interval_ms);

/** Stop generating data (deletes LVGL timer). */
void mock_data_stop(void);

/** Get the latest vitals snapshot. */
const vitals_data_t * mock_data_get_current(void);

/** Register a callback for data updates. */
void mock_data_set_callback(mock_data_cb_t cb);

/* ============================================================
 *  History Buffer (for Trends screen)
 * ============================================================ */

#define MOCK_DATA_HISTORY_LEN 120   /* 2 minutes at 1Hz */

typedef struct {
    int hr[MOCK_DATA_HISTORY_LEN];
    int spo2[MOCK_DATA_HISTORY_LEN];
    int rr[MOCK_DATA_HISTORY_LEN];
    int count;          /* Total samples recorded (may exceed HISTORY_LEN) */
    int write_idx;      /* Next write position (circular) */
} vitals_history_t;

/** Get pointer to the history ring buffer (read-only). */
const vitals_history_t * mock_data_get_history(void);

/* ============================================================
 *  Alarm Log (for Alarms screen)
 * ============================================================ */

#define ALARM_LOG_MAX 32

typedef struct {
    vm_alarm_severity_t severity;
    char message[48];
    char time_str[8];       /* "HH:MM" */
    uint32_t timestamp_s;   /* seconds since app start */
} alarm_log_entry_t;

typedef struct {
    alarm_log_entry_t entries[ALARM_LOG_MAX];
    int count;              /* Total entries (capped at ALARM_LOG_MAX) */
    int write_idx;          /* Circular write index */
} alarm_log_t;

/** Get pointer to the alarm log (read-only). */
const alarm_log_t * mock_data_get_alarm_log(void);

/** Record an alarm event into the log. */
void mock_data_log_alarm(vm_alarm_severity_t severity, const char *message,
                          const char *time_str);

#endif /* MOCK_DATA_H */
