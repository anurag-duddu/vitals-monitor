/**
 * @file sensor_hal.h
 * @brief Hardware Abstraction Layer for vital signs sensors
 *
 * This interface defines the contract that all sensor drivers must implement.
 * The sensor-service uses this HAL to interact with sensors regardless of
 * whether they are:
 *   - OEM medical-grade modules (production)
 *   - Development evaluation boards (prototyping)
 *   - Mock implementations (testing)
 *
 * IMPLEMENTATION NOTES FOR REMOTE TEAM:
 *
 * 1. Each sensor type has its own implementation file:
 *    - src/drivers/spo2/hal_spo2_<vendor>.c  (e.g., hal_spo2_masimo.c)
 *    - src/drivers/ecg/hal_ecg_<vendor>.c    (e.g., hal_ecg_analog_devices.c)
 *    - src/drivers/nibp/hal_nibp_<vendor>.c  (e.g., hal_nibp_suntech.c)
 *    - src/drivers/temp/hal_temp_<vendor>.c
 *
 * 2. Implement the ops structure for your sensor and register it:
 *    sensor_hal_register(SENSOR_TYPE_SPO2, &my_spo2_ops);
 *
 * 3. The sensor-service will call these functions from its main loop.
 *
 * 4. For waveform data, use the provided callback mechanism.
 */

#ifndef SENSOR_HAL_H
#define SENSOR_HAL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 *  Sensor Types
 * ============================================================ */

typedef enum {
    SENSOR_TYPE_SPO2 = 0,
    SENSOR_TYPE_ECG,
    SENSOR_TYPE_NIBP,
    SENSOR_TYPE_TEMP,
    SENSOR_TYPE_COUNT
} sensor_type_t;

/* ============================================================
 *  Sensor State
 * ============================================================ */

typedef enum {
    SENSOR_STATE_DISCONNECTED = 0,
    SENSOR_STATE_INITIALIZING,
    SENSOR_STATE_READY,
    SENSOR_STATE_MEASURING,
    SENSOR_STATE_ERROR,
} sensor_state_t;

typedef enum {
    SENSOR_ERROR_NONE = 0,
    SENSOR_ERROR_NOT_FOUND,
    SENSOR_ERROR_INIT_FAILED,
    SENSOR_ERROR_COMM_FAILED,
    SENSOR_ERROR_CALIBRATION,
    SENSOR_ERROR_PATIENT_OFF,
    SENSOR_ERROR_INTERNAL,
} sensor_error_t;

/* ============================================================
 *  SpO2 Sensor Data
 * ============================================================ */

typedef struct {
    int16_t  spo2;              /* SpO2 percentage (0-100), -1 = invalid */
    int16_t  pulse_rate;        /* Pulse rate from pleth (bpm), -1 = invalid */
    uint8_t  signal_quality;    /* 0-100, 0 = no signal */
    uint8_t  perfusion_index;   /* PI * 10 (e.g., 35 = 3.5%), 0 = invalid */
    bool     finger_detected;   /* True if finger/probe connected */
} spo2_reading_t;

typedef struct {
    int16_t  samples[100];      /* Raw IR or Red samples, normalized to 0-4095 */
    uint16_t sample_count;
    uint16_t sample_rate_hz;    /* Typically 100 Hz */
} spo2_waveform_t;

/* ============================================================
 *  ECG Sensor Data
 * ============================================================ */

typedef struct {
    int16_t  heart_rate;        /* Heart rate (bpm), -1 = invalid */
    int16_t  resp_rate;         /* Resp rate from impedance (breaths/min), -1 = invalid */
    uint8_t  lead_off_status;   /* Bitmask: bit0=LA, bit1=RA, bit2=LL off */
    uint8_t  signal_quality;    /* 0-100 */
    bool     pacemaker_detected;
    bool     arrhythmia_flag;   /* Generic arrhythmia indicator */
} ecg_reading_t;

typedef struct {
    int16_t  samples[250];      /* Raw ECG samples in microvolts */
    uint16_t sample_count;
    uint16_t sample_rate_hz;    /* Typically 500 Hz */
    uint8_t  lead;              /* Which lead (0 = Lead I, 1 = Lead II, etc.) */
} ecg_waveform_t;

/* ============================================================
 *  NIBP Sensor Data
 * ============================================================ */

typedef enum {
    NIBP_MODE_MANUAL = 0,       /* Single measurement on demand */
    NIBP_MODE_AUTO,             /* Periodic automatic measurements */
    NIBP_MODE_STAT,             /* Rapid repeated measurements */
} nibp_mode_t;

typedef enum {
    NIBP_STATUS_IDLE = 0,
    NIBP_STATUS_INFLATING,
    NIBP_STATUS_MEASURING,
    NIBP_STATUS_DEFLATING,
    NIBP_STATUS_COMPLETE,
    NIBP_STATUS_ERROR,
} nibp_status_t;

typedef struct {
    int16_t  systolic;          /* mmHg, -1 = invalid */
    int16_t  diastolic;         /* mmHg, -1 = invalid */
    int16_t  map;               /* Mean arterial pressure, mmHg */
    int16_t  pulse_rate;        /* Pulse rate from cuff, bpm */
    uint8_t  cuff_pressure;     /* Current cuff pressure during measurement */
    nibp_status_t status;
    sensor_error_t error;
} nibp_reading_t;

/* ============================================================
 *  Temperature Sensor Data
 * ============================================================ */

typedef enum {
    TEMP_SITE_UNKNOWN = 0,
    TEMP_SITE_ORAL,
    TEMP_SITE_AXILLARY,
    TEMP_SITE_RECTAL,
    TEMP_SITE_TYMPANIC,
    TEMP_SITE_SKIN,
} temp_site_t;

typedef struct {
    int16_t  temp_x10;          /* Temperature * 10 (e.g., 368 = 36.8C), -1 = invalid */
    temp_site_t site;
    bool     probe_connected;
} temp_reading_t;

/* ============================================================
 *  Waveform Callback Type
 *
 *  Called by sensor drivers when new waveform data is available.
 *  The callback must copy the data - the buffer may be reused.
 * ============================================================ */

typedef void (*spo2_waveform_cb_t)(const spo2_waveform_t *waveform, void *user_data);
typedef void (*ecg_waveform_cb_t)(const ecg_waveform_t *waveform, void *user_data);

/* ============================================================
 *  Sensor Operations Interface
 *
 *  Each sensor implementation provides this structure.
 * ============================================================ */

typedef struct {
    const char *name;           /* e.g., "Masimo SET" */
    const char *vendor;         /* e.g., "Masimo" */

    /**
     * Initialize the sensor hardware.
     * @return 0 on success, negative error code on failure
     */
    int (*init)(void);

    /**
     * Shutdown the sensor hardware.
     */
    void (*deinit)(void);

    /**
     * Get current sensor state.
     */
    sensor_state_t (*get_state)(void);

    /**
     * Get last error (if state == SENSOR_STATE_ERROR).
     */
    sensor_error_t (*get_error)(void);

    /**
     * Check if sensor is ready to provide readings.
     */
    bool (*is_ready)(void);

} sensor_ops_base_t;

/* SpO2-specific operations */
typedef struct {
    sensor_ops_base_t base;

    /**
     * Get the latest SpO2 reading.
     * @param reading Output structure
     * @return 0 on success, -1 if no valid reading
     */
    int (*get_reading)(spo2_reading_t *reading);

    /**
     * Set callback for waveform data.
     */
    void (*set_waveform_callback)(spo2_waveform_cb_t callback, void *user_data);

} sensor_ops_spo2_t;

/* ECG-specific operations */
typedef struct {
    sensor_ops_base_t base;

    int (*get_reading)(ecg_reading_t *reading);
    void (*set_waveform_callback)(ecg_waveform_cb_t callback, void *user_data);

    /**
     * For M4 co-processor: start/stop high-speed sampling.
     * The M4 firmware will push data via shared memory.
     */
    int (*start_acquisition)(void);
    void (*stop_acquisition)(void);

} sensor_ops_ecg_t;

/* NIBP-specific operations */
typedef struct {
    sensor_ops_base_t base;

    int (*get_reading)(nibp_reading_t *reading);

    /**
     * Start a blood pressure measurement.
     * @param mode Measurement mode
     * @return 0 if started, -1 on error
     */
    int (*start_measurement)(nibp_mode_t mode);

    /**
     * Abort an in-progress measurement.
     */
    void (*abort_measurement)(void);

    /**
     * Set auto-measurement interval (for NIBP_MODE_AUTO).
     * @param interval_sec Seconds between measurements (e.g., 300 for 5 min)
     */
    void (*set_auto_interval)(uint32_t interval_sec);

} sensor_ops_nibp_t;

/* Temperature-specific operations */
typedef struct {
    sensor_ops_base_t base;

    int (*get_reading)(temp_reading_t *reading);
    void (*set_measurement_site)(temp_site_t site);

} sensor_ops_temp_t;

/* ============================================================
 *  HAL Registration API
 *
 *  Called by sensor drivers at startup to register their ops.
 * ============================================================ */

/**
 * Register a sensor implementation.
 * @param type Sensor type
 * @param ops Pointer to ops structure (must remain valid)
 * @return 0 on success
 */
int sensor_hal_register_spo2(const sensor_ops_spo2_t *ops);
int sensor_hal_register_ecg(const sensor_ops_ecg_t *ops);
int sensor_hal_register_nibp(const sensor_ops_nibp_t *ops);
int sensor_hal_register_temp(const sensor_ops_temp_t *ops);

/**
 * Get registered sensor ops.
 * @return Pointer to ops, or NULL if not registered
 */
const sensor_ops_spo2_t *sensor_hal_get_spo2(void);
const sensor_ops_ecg_t *sensor_hal_get_ecg(void);
const sensor_ops_nibp_t *sensor_hal_get_nibp(void);
const sensor_ops_temp_t *sensor_hal_get_temp(void);

/**
 * Initialize all registered sensors.
 * @return 0 if all succeeded, negative if any failed
 */
int sensor_hal_init_all(void);

/**
 * Shutdown all sensors.
 */
void sensor_hal_deinit_all(void);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_HAL_H */
