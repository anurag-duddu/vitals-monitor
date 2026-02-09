/**
 * @file sensor_hal.h
 * @brief Hardware Abstraction Layer for vital signs sensors
 *
 * This interface defines the contract that all sensor drivers must implement.
 * The remote hardware team fills in the sensor_hal_t callbacks for each
 * sensor type. The sensor-service uses this HAL to interact with sensors
 * regardless of whether they are:
 *   - OEM medical-grade modules (production)
 *   - Development evaluation boards (prototyping)
 *   - Mock implementations (testing)
 *
 * IMPLEMENTATION NOTES FOR REMOTE TEAM:
 *
 * 1. Each sensor type has its own implementation file:
 *    - src/drivers/ecg/hal_ecg_<vendor>.c
 *    - src/drivers/spo2/hal_spo2_<vendor>.c
 *    - src/drivers/nibp/hal_nibp_<vendor>.c
 *    - src/drivers/temp/hal_temp_<vendor>.c
 *
 * 2. Implement the sensor_hal_t callbacks for your sensor and register it:
 *    sensor_hal_register(SENSOR_ECG, &my_ecg_hal);
 *
 * 3. The sensor-service will call these functions from its main loop.
 *
 * 4. For waveform data, use read_samples() with the appropriate buffer type.
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
    SENSOR_ECG = 0,      /* ADS1292R via SPI */
    SENSOR_SPO2,         /* MAX30102 via I2C */
    SENSOR_TEMP,         /* TMP117 via I2C */
    SENSOR_NIBP,         /* Pressure transducer via ADC */
    SENSOR_COUNT
} sensor_type_t;

/* ============================================================
 *  Sensor State
 * ============================================================ */

typedef enum {
    SENSOR_STATE_UNINITIALIZED = 0,
    SENSOR_STATE_READY,
    SENSOR_STATE_SAMPLING,
    SENSOR_STATE_ERROR,
    SENSOR_STATE_CALIBRATING
} sensor_state_t;

/* ============================================================
 *  Raw Sample Types
 * ============================================================ */

/**
 * Raw ECG sample from ADS1292R (24-bit ADC).
 * The ADS1292R provides two channels: Lead I and Lead II.
 * Values are 24-bit, sign-extended to int32_t.
 */
typedef struct {
    int32_t lead_I;          /* Lead I value (24-bit, sign-extended) */
    int32_t lead_II;         /* Lead II value */
    uint32_t timestamp_us;   /* Microsecond timestamp from sensor clock */
} ecg_raw_sample_t;

/**
 * Raw SpO2 sample from MAX30102.
 * Contains raw photodiode counts for both red and infrared LEDs.
 */
typedef struct {
    uint32_t red;            /* Red LED photodiode count */
    uint32_t ir;             /* IR LED photodiode count */
    uint32_t timestamp_us;   /* Microsecond timestamp from sensor clock */
} spo2_raw_sample_t;

/* ============================================================
 *  HAL Interface
 *
 *  One sensor_hal_t struct per sensor type. Implement all
 *  relevant callbacks and register via sensor_hal_register().
 * ============================================================ */

typedef struct {
    sensor_type_t type;
    const char *name;

    /**
     * Initialize the sensor hardware.
     * Called once at startup. Configure GPIO, I2C/SPI buses, etc.
     * @return true on success, false on failure
     */
    bool (*init)(void);

    /**
     * Deinitialize the sensor hardware.
     * Release any hardware resources. Called at shutdown.
     */
    void (*deinit)(void);

    /**
     * Get the current sensor state.
     * @return Current state of this sensor
     */
    sensor_state_t (*get_state)(void);

    /**
     * Start continuous sampling (for ECG and SpO2).
     * After this call, read_samples() will return new data.
     * @param sample_rate_hz Requested sample rate in Hz
     * @return true if sampling started successfully
     */
    bool (*start_sampling)(uint32_t sample_rate_hz);

    /**
     * Stop continuous sampling.
     */
    void (*stop_sampling)(void);

    /**
     * Trigger a one-shot measurement (for Temp and NIBP).
     * Poll is_measurement_ready() to check completion.
     * @return true if measurement triggered successfully
     */
    bool (*trigger_measurement)(void);

    /**
     * Check if a triggered measurement is complete.
     * @return true if result is ready to read
     */
    bool (*is_measurement_ready)(void);

    /**
     * Read available samples from the sensor.
     *
     * For ECG:  buffer is ecg_raw_sample_t[], max_samples is array length
     * For SpO2: buffer is spo2_raw_sample_t[], max_samples is array length
     * For Temp/NIBP: buffer is type-specific, max_samples typically 1
     *
     * @param buffer      Output buffer (caller allocates, type depends on sensor)
     * @param max_samples Maximum number of samples to read
     * @return Number of samples actually read, or -1 on error
     */
    int (*read_samples)(void *buffer, int max_samples);

    /**
     * Run sensor calibration routine.
     * May block for several seconds. Call before first measurement.
     * @return true if calibration succeeded
     */
    bool (*calibrate)(void);

    /**
     * Run sensor self-test.
     * Verifies communication and basic sensor function.
     * @return true if self-test passed
     */
    bool (*self_test)(void);
} sensor_hal_t;

/* ============================================================
 *  HAL Registration API
 * ============================================================ */

/**
 * Register a sensor HAL implementation.
 * Called by each driver at startup to make itself available.
 *
 * @param type Sensor type (must be < SENSOR_COUNT)
 * @param hal  Pointer to HAL struct (must remain valid for program lifetime)
 */
void sensor_hal_register(sensor_type_t type, const sensor_hal_t *hal);

/**
 * Get the registered HAL for a sensor type.
 *
 * @param type Sensor type to look up
 * @return Pointer to HAL struct, or NULL if not registered
 */
const sensor_hal_t *sensor_hal_get(sensor_type_t type);

/**
 * Initialize all registered sensors.
 * Calls init() on each registered HAL in order.
 *
 * @return true if all sensors initialized successfully
 */
bool sensor_hal_init_all(void);

/**
 * Deinitialize all registered sensors.
 * Calls deinit() on each registered HAL in reverse order.
 */
void sensor_hal_deinit_all(void);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_HAL_H */
