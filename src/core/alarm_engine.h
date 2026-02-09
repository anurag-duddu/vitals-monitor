/**
 * @file alarm_engine.h
 * @brief Alarm threshold engine with per-parameter state machine
 *
 * Evaluates vital sign data against configurable thresholds and manages
 * alarm state transitions (inactive -> active -> acknowledged -> silenced).
 *
 * Design principles:
 *   - Pure logic module: NO LVGL headers, no UI dependencies
 *   - Static allocation: no malloc, all state at file scope
 *   - Single-threaded: called from LVGL main loop only
 *   - Temperature stored as x10 integer to avoid float comparison issues
 *
 * USAGE:
 *   1. Call alarm_engine_init() once at startup
 *   2. Call alarm_engine_evaluate() on each vitals_data_t update (1 Hz)
 *   3. Read alarm_engine_get_state() to drive UI banner / audio
 *   4. Call acknowledge/silence from UI button handlers
 *   5. Call alarm_engine_deinit() at shutdown
 */

#ifndef ALARM_ENGINE_H
#define ALARM_ENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include "vitals_provider.h"    /* vitals_data_t (no LVGL dependency) */

#ifdef __cplusplus
extern "C" {
#endif

/* ── Alarm severity (mirrors vm_alarm_severity_t) ────────── */

/* We redefine severity here to keep the header free of LVGL/theme
 * dependencies.  Numeric values match vm_alarm_severity_t exactly
 * so a simple cast is safe at the call site. */
typedef enum {
    ALARM_SEV_NONE = 0,
    ALARM_SEV_LOW,       /* Advisory - cyan */
    ALARM_SEV_MEDIUM,    /* Warning - yellow */
    ALARM_SEV_HIGH       /* Critical - red */
} alarm_severity_t;

/* ── Per-parameter alarm thresholds ──────────────────────── */

typedef struct {
    int  critical_high;   /* > triggers HIGH alarm (e.g., HR > 150)    */
    int  critical_low;    /* < triggers HIGH alarm (e.g., HR < 40)     */
    int  warning_high;    /* > triggers MEDIUM alarm (e.g., HR > 120)  */
    int  warning_low;     /* < triggers MEDIUM alarm (e.g., HR < 50)   */
    bool enabled;         /* false = alarms disabled for this param    */
} alarm_limits_t;

/* ── Parameter identifiers ───────────────────────────────── */

typedef enum {
    ALARM_PARAM_HR = 0,
    ALARM_PARAM_SPO2,
    ALARM_PARAM_RR,
    ALARM_PARAM_TEMP,
    ALARM_PARAM_NIBP_SYS,
    ALARM_PARAM_NIBP_DIA,
    ALARM_PARAM_COUNT
} alarm_param_t;

/* ── Alarm state machine ─────────────────────────────────── */

typedef enum {
    ALARM_STATE_INACTIVE = 0,    /* No alarm condition                          */
    ALARM_STATE_ACTIVE,          /* Alarm triggered, not acknowledged           */
    ALARM_STATE_ACKNOWLEDGED,    /* Acknowledged by user, condition still true  */
    ALARM_STATE_SILENCED,        /* Temporarily silenced (default 2-min)        */
} alarm_state_t;

/* ── Per-parameter alarm status ──────────────────────────── */

typedef struct {
    alarm_state_t    state;
    alarm_severity_t severity;       /* Current severity (NONE/LOW/MEDIUM/HIGH) */
    char             message[48];    /* Human-readable alarm message            */
    uint32_t         trigger_time_s; /* When alarm was first triggered          */
    uint32_t         ack_time_s;     /* When acknowledged (0 if not)            */
    uint32_t         silence_until_s;/* When silence expires                    */
} alarm_status_t;

/* ── Overall alarm engine state ──────────────────────────── */

typedef struct {
    alarm_status_t   params[ALARM_PARAM_COUNT];
    alarm_severity_t highest_active;      /* Highest unacknowledged alarm     */
    alarm_severity_t highest_any;         /* Highest alarm of any state       */
    const char      *highest_message;     /* Message for highest alarm        */
    bool             audio_paused;        /* Global audio pause active        */
    uint32_t         audio_pause_until_s; /* When audio pause expires         */
} alarm_engine_state_t;

/* ── Lifecycle ───────────────────────────────────────────── */

/** Initialize the alarm engine with default thresholds. */
void alarm_engine_init(void);

/** Shutdown the alarm engine and reset all state. */
void alarm_engine_deinit(void);

/* ── Evaluation ──────────────────────────────────────────── */

/**
 * Evaluate vitals against current thresholds and update alarm states.
 * Should be called once per vitals update (typically 1 Hz).
 *
 * @param data           Current vital signs snapshot
 * @param current_time_s Monotonic time in seconds (for silence timeouts)
 */
void alarm_engine_evaluate(const vitals_data_t *data, uint32_t current_time_s);

/* ── State query ─────────────────────────────────────────── */

/** Get read-only pointer to current alarm engine state. */
const alarm_engine_state_t *alarm_engine_get_state(void);

/* ── Alarm management ────────────────────────────────────── */

/**
 * Acknowledge a single parameter's alarm (ACTIVE -> ACKNOWLEDGED).
 * @return true if the alarm was active and is now acknowledged
 */
bool alarm_engine_acknowledge(alarm_param_t param);

/**
 * Acknowledge all active alarms.
 * @return true if at least one alarm was acknowledged
 */
bool alarm_engine_acknowledge_all(void);

/**
 * Silence a single parameter's alarm for duration_s seconds.
 * @param param      Parameter to silence
 * @param duration_s Silence duration (use 120 for default 2-minute)
 * @return true if the alarm was active/acknowledged and is now silenced
 */
bool alarm_engine_silence(alarm_param_t param, uint32_t duration_s);

/**
 * Silence all active/acknowledged alarms for duration_s seconds.
 * @param duration_s Silence duration (use 120 for default 2-minute)
 * @return true if at least one alarm was silenced
 */
bool alarm_engine_silence_all(uint32_t duration_s);

/* ── Threshold configuration ─────────────────────────────── */

/**
 * Set alarm limits for a parameter.
 * For ALARM_PARAM_TEMP, values are x10 integer (e.g., 390 = 39.0 C).
 */
void alarm_engine_set_limits(alarm_param_t param, const alarm_limits_t *limits);

/** Get current alarm limits for a parameter (read-only). */
const alarm_limits_t *alarm_engine_get_limits(alarm_param_t param);

/** Reset all thresholds to factory defaults. */
void alarm_engine_reset_defaults(void);

/* ── Audio control ───────────────────────────────────────── */

/**
 * Globally pause alarm audio for duration_s seconds.
 * Visual alarms remain visible; only audio is suppressed.
 */
void alarm_engine_pause_audio(uint32_t duration_s);

/** Check if alarm audio is currently paused. */
bool alarm_engine_is_audio_paused(void);

#ifdef __cplusplus
}
#endif

#endif /* ALARM_ENGINE_H */
