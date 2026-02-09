/**
 * @file mock_data.h
 * @brief Mock sensor data generator for simulator development
 *
 * Produces realistic vital sign values using random walk with mean-reversion.
 * Uses LVGL timer for periodic updates. In production (Phase 6+), this module
 * is replaced by IPC subscriptions, but the callback interface is identical.
 *
 * NOTE: This header uses types from vitals_provider.h as the canonical source.
 */

#ifndef MOCK_DATA_H
#define MOCK_DATA_H

#include "lvgl.h"
#include "theme_vitals.h"
#include "vitals_provider.h"
#include <stdbool.h>

/* vitals_data_t is now defined in vitals_provider.h */

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
 *  NOTE: vitals_history_t is now defined in vitals_provider.h
 *  MOCK_DATA_HISTORY_LEN is used internally for the mock buffer
 * ============================================================ */

#define MOCK_DATA_HISTORY_LEN 120   /* 2 minutes at 1Hz */

/** Get pointer to the history ring buffer (read-only). */
const vitals_history_t * mock_data_get_history(void);

/* ============================================================
 *  Alarm Log (for Alarms screen)
 *  NOTE: Types are now defined in vitals_provider.h
 * ============================================================ */

/** Get pointer to the alarm log (read-only). */
const vitals_alarm_log_t * mock_data_get_alarm_log(void);

/** Record an alarm event into the log. */
void mock_data_log_alarm(vitals_alarm_severity_t severity, const char *message,
                          const char *time_str);

#endif /* MOCK_DATA_H */
