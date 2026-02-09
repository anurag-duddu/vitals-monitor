/**
 * @file vitals_provider.h
 * @brief Abstract data provider interface for vital signs
 *
 * This abstraction allows the UI to receive vital signs data without knowing
 * whether it comes from:
 *   - Mock data generator (simulator builds)
 *   - IPC subscription (target builds, multi-process)
 *   - Direct sensor reads (target builds, single-process testing)
 *
 * USAGE:
 *   1. Call vitals_provider_init() once at startup
 *   2. Register callback with vitals_provider_set_callback()
 *   3. Call vitals_provider_start() to begin receiving data
 *   4. Your callback fires with new vitals_data_t on each update
 *
 * BUILD CONFIGURATION:
 *   - VITALS_PROVIDER_MOCK: Use mock data generator (default for simulator)
 *   - VITALS_PROVIDER_IPC:  Use nanomsg IPC subscriber (default for target)
 *   - VITALS_PROVIDER_DIRECT: Direct HAL calls (single-process target test)
 */

#ifndef VITALS_PROVIDER_H
#define VITALS_PROVIDER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 *  Vital Signs Data Structure
 *
 *  This is the CANONICAL data format shared between:
 *  - Mock data generator
 *  - IPC messages (sensor-service → ui-app)
 *  - Sensor HAL implementations
 * ============================================================ */

typedef struct {
    /* Continuous parameters */
    int     hr;             /* Heart rate (bpm), 0 = invalid */
    int     spo2;           /* SpO2 (%), 0 = invalid */
    int     rr;             /* Respiratory rate (breaths/min), 0 = invalid */
    float   temp;           /* Temperature (Celsius), 0.0 = invalid */

    /* Periodic NIBP */
    int     nibp_sys;       /* Systolic BP (mmHg), 0 = invalid */
    int     nibp_dia;       /* Diastolic BP (mmHg), 0 = invalid */
    int     nibp_map;       /* Mean arterial pressure (mmHg), 0 = invalid */
    bool    nibp_fresh;     /* True when NIBP was just measured */

    /* Metadata */
    uint64_t timestamp_ms;  /* Milliseconds since epoch (UTC) */
    uint8_t  patient_slot;  /* 0 or 1 for dual-patient mode */

    /* Signal quality indicators */
    uint8_t  hr_quality;    /* 0-100, 0 = no signal */
    uint8_t  spo2_quality;  /* 0-100, 0 = no signal */
    uint8_t  ecg_lead_off;  /* Bitmask: bit0=LA, bit1=RA, bit2=LL */
} vitals_data_t;

/* ============================================================
 *  Waveform Data Structure
 *
 *  Separate from vitals for high-frequency updates
 * ============================================================ */

#define WAVEFORM_SAMPLES_PER_PACKET 50  /* ~100ms at 500Hz ECG */

typedef enum {
    WAVEFORM_ECG = 0,
    WAVEFORM_PLETH,
    WAVEFORM_RESP,
    WAVEFORM_TYPE_COUNT
} waveform_type_t;

typedef struct {
    waveform_type_t type;
    uint8_t         patient_slot;
    uint16_t        sample_rate_hz;
    uint16_t        sample_count;
    int16_t         samples[WAVEFORM_SAMPLES_PER_PACKET];
    uint64_t        timestamp_ms;   /* Timestamp of first sample */
} waveform_packet_t;

/* ============================================================
 *  Callback Types
 * ============================================================ */

/** Called when new vital signs data arrives (typically 1 Hz) */
typedef void (*vitals_callback_t)(const vitals_data_t *data, void *user_data);

/** Called when new waveform samples arrive (typically 10-20 Hz) */
typedef void (*waveform_callback_t)(const waveform_packet_t *packet, void *user_data);

/* ============================================================
 *  Provider API
 * ============================================================ */

/**
 * Initialize the vitals provider.
 * Must be called before any other provider functions.
 *
 * @return 0 on success, negative error code on failure
 */
int vitals_provider_init(void);

/**
 * Start the vitals provider.
 * After this call, registered callbacks will start receiving data.
 *
 * @param vitals_interval_ms  Interval for vitals updates (typically 1000)
 * @return 0 on success, negative error code on failure
 */
int vitals_provider_start(uint32_t vitals_interval_ms);

/**
 * Stop the vitals provider.
 * Callbacks will no longer be called after this returns.
 */
void vitals_provider_stop(void);

/**
 * Shutdown and cleanup the vitals provider.
 */
void vitals_provider_deinit(void);

/**
 * Register a callback for vital signs updates.
 *
 * @param callback  Function to call with new data
 * @param user_data Opaque pointer passed to callback
 */
void vitals_provider_set_vitals_callback(vitals_callback_t callback, void *user_data);

/**
 * Register a callback for waveform updates.
 *
 * @param callback  Function to call with new waveform data
 * @param user_data Opaque pointer passed to callback
 */
void vitals_provider_set_waveform_callback(waveform_callback_t callback, void *user_data);

/**
 * Get the most recent vitals snapshot.
 * Useful for getting current values without waiting for callback.
 *
 * @param slot  Patient slot (0 or 1)
 * @return Pointer to current data (valid until next update), NULL if slot invalid
 */
const vitals_data_t *vitals_provider_get_current(uint8_t slot);

/**
 * Check if the provider is currently running.
 *
 * @return true if started and receiving data
 */
bool vitals_provider_is_running(void);

/**
 * Get provider type string for debugging.
 *
 * @return "mock", "ipc", or "direct"
 */
const char *vitals_provider_get_type(void);

/* ============================================================
 *  History API (for trends screen)
 * ============================================================ */

#define VITALS_HISTORY_LEN 4320  /* 72 hours at 1-minute resolution */

typedef struct {
    int hr[VITALS_HISTORY_LEN];
    int spo2[VITALS_HISTORY_LEN];
    int rr[VITALS_HISTORY_LEN];
    int nibp_sys[VITALS_HISTORY_LEN];
    int nibp_dia[VITALS_HISTORY_LEN];
    uint64_t timestamps[VITALS_HISTORY_LEN];
    int count;
    int write_idx;
} vitals_history_t;

/**
 * Get pointer to the history buffer for a patient slot.
 *
 * @param slot  Patient slot (0 or 1)
 * @return Pointer to history (read-only), NULL if slot invalid
 */
const vitals_history_t *vitals_provider_get_history(uint8_t slot);

/* ============================================================
 *  Alarm Log API (for alarms screen)
 * ============================================================ */

/* Alarm severity levels (matches theme_vitals.h vm_alarm_severity_t) */
typedef enum {
    VITALS_ALARM_NONE = 0,
    VITALS_ALARM_LOW,       /* Technical/advisory - cyan */
    VITALS_ALARM_MEDIUM,    /* Warning - yellow */
    VITALS_ALARM_HIGH,      /* Critical - red */
} vitals_alarm_severity_t;

#define VITALS_ALARM_LOG_MAX 32

typedef struct {
    vitals_alarm_severity_t severity;
    char message[48];
    char time_str[8];       /* "HH:MM" */
    uint32_t timestamp_s;   /* Seconds since provider start */
} vitals_alarm_entry_t;

typedef struct {
    vitals_alarm_entry_t entries[VITALS_ALARM_LOG_MAX];
    int count;              /* Total entries (capped at max) */
    int write_idx;          /* Circular write index */
} vitals_alarm_log_t;

/**
 * Get pointer to the alarm log.
 *
 * @return Pointer to alarm log (read-only)
 */
const vitals_alarm_log_t *vitals_provider_get_alarm_log(void);

/**
 * Record an alarm event into the log.
 *
 * @param severity  Alarm severity level
 * @param message   Human-readable alarm message
 * @param time_str  Time string (e.g., "14:32")
 */
void vitals_provider_log_alarm(vitals_alarm_severity_t severity,
                               const char *message,
                               const char *time_str);

#ifdef __cplusplus
}
#endif

#endif /* VITALS_PROVIDER_H */
