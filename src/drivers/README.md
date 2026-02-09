# Sensor Drivers -- Hardware Integration Guide

This directory contains the **hardware abstraction layer (HAL)** for all sensors
used by the vitals monitor. The remote hardware team implements the driver
callbacks defined in `sensor_hal.h`; the rest of the application interacts only
with the abstract interface.

---

## Directory Layout

```
src/drivers/
  sensor_hal.h          # HAL interface (DO NOT MODIFY without coordination)
  sensor_hal.c          # Registry implementation (registration + init/deinit)
  ota_update.h          # OTA update interface (Phase 11)
  ota_update.c          # OTA stub (simulator); replaced on target
  ecg/                  # ECG driver implementations
    hal_ecg_ads1292r.c  # (you create this)
  spo2/                 # SpO2 driver implementations
    hal_spo2_max30102.c # (you create this)
  nibp/                 # NIBP driver implementations
    hal_nibp_<vendor>.c # (you create this)
  temp/                 # Temperature driver implementations
    hal_temp_tmp117.c   # (you create this)
```

---

## How to Implement a Sensor Driver

### Step 1: Create the Driver File

Place your implementation in the correct subdirectory. Example for the
ADS1292R ECG front-end:

```
src/drivers/ecg/hal_ecg_ads1292r.c
```

### Step 2: Implement the sensor_hal_t Callbacks

Each sensor driver fills in a `sensor_hal_t` struct with function pointers.
Not all callbacks are relevant to every sensor -- set unused ones to `NULL`.

**Continuous sensors (ECG, SpO2)** use:
- `init` / `deinit`
- `start_sampling` / `stop_sampling`
- `read_samples`
- `calibrate`, `self_test`

**One-shot sensors (Temp, NIBP)** use:
- `init` / `deinit`
- `trigger_measurement` / `is_measurement_ready`
- `read_samples` (to fetch the result once ready)
- `calibrate`, `self_test`

### Step 3: Register at Startup

At the bottom of your driver file, provide a registration function that the
sensor-service main() calls during initialization:

```c
#include "sensor_hal.h"

/* --- callback implementations --- */

static bool ecg_init(void) {
    /* Configure SPI bus, GPIO pins, ADS1292R registers */
    return true;
}

static void ecg_deinit(void) {
    /* Release SPI, power down chip */
}

static sensor_state_t ecg_get_state(void) {
    return SENSOR_STATE_READY;
}

static bool ecg_start_sampling(uint32_t sample_rate_hz) {
    /* Configure ADS1292R for continuous conversion at sample_rate_hz */
    /* Typical: 500 Hz */
    return true;
}

static void ecg_stop_sampling(void) {
    /* Send SDATAC command to ADS1292R */
}

static int ecg_read_samples(void *buffer, int max_samples) {
    ecg_raw_sample_t *out = (ecg_raw_sample_t *)buffer;
    /* Read from SPI FIFO, fill out[0..n-1] */
    /* Return number of samples actually read */
    return 0;
}

static bool ecg_calibrate(void) {
    /* Run internal offset calibration */
    return true;
}

static bool ecg_self_test(void) {
    /* Read chip ID register, verify SPI communication */
    return true;
}

/* --- HAL struct --- */

static const sensor_hal_t ecg_hal = {
    .type             = SENSOR_ECG,
    .name             = "ADS1292R ECG",
    .init             = ecg_init,
    .deinit           = ecg_deinit,
    .get_state        = ecg_get_state,
    .start_sampling   = ecg_start_sampling,
    .stop_sampling    = ecg_stop_sampling,
    .trigger_measurement  = NULL,   /* not a one-shot sensor */
    .is_measurement_ready = NULL,
    .read_samples     = ecg_read_samples,
    .calibrate        = ecg_calibrate,
    .self_test        = ecg_self_test,
};

/* Called from sensor-service main() */
void hal_ecg_ads1292r_register(void) {
    sensor_hal_register(SENSOR_ECG, &ecg_hal);
}
```

### Step 4: Call from sensor-service

In `src/services/sensor-service/main.c`:

```c
extern void hal_ecg_ads1292r_register(void);
extern void hal_spo2_max30102_register(void);
extern void hal_temp_tmp117_register(void);
extern void hal_nibp_register(void);

int main(void) {
    /* Register all sensor drivers */
    hal_ecg_ads1292r_register();
    hal_spo2_max30102_register();
    hal_temp_tmp117_register();
    hal_nibp_register();

    /* Initialize all at once */
    if (!sensor_hal_init_all()) {
        fprintf(stderr, "Sensor init failed\n");
        return 1;
    }

    /* Start continuous sensors */
    const sensor_hal_t *ecg = sensor_hal_get(SENSOR_ECG);
    if (ecg) ecg->start_sampling(500);

    const sensor_hal_t *spo2 = sensor_hal_get(SENSOR_SPO2);
    if (spo2) spo2->start_sampling(100);

    /* Main loop: read and publish */
    /* ... */

    sensor_hal_deinit_all();
    return 0;
}
```

---

## Expected Sample Rates

| Sensor | Chip        | Bus  | Sample Rate     | Notes                          |
|--------|-------------|------|-----------------|--------------------------------|
| ECG    | ADS1292R    | SPI  | 500 Hz          | Continuous; 2 channels (I, II) |
| SpO2   | MAX30102    | I2C  | 100 Hz          | Continuous; red + IR channels  |
| Temp   | TMP117      | I2C  | 1 Hz (on-demand)| One-shot; trigger + poll       |
| NIBP   | Transducer  | ADC  | On-demand       | One-shot; inflate/measure/deflate cycle |

---

## I2C / SPI Bus Assignments (STM32MP157F-DK2)

These are the default bus assignments. Adjust if your hardware revision differs.

| Sensor   | Bus    | Device Path          | Address / CS   | Notes                    |
|----------|--------|----------------------|----------------|--------------------------|
| ADS1292R | SPI1   | `/dev/spidev0.0`    | CS0 (GPIO PA4) | DRDY on GPIO PG1         |
| MAX30102 | I2C1   | `/dev/i2c-1`        | 0x57           | INT on GPIO PB3          |
| TMP117   | I2C1   | `/dev/i2c-1`        | 0x48           | ALERT on GPIO PB4        |
| NIBP ADC | SPI2   | `/dev/spidev1.0`    | CS0 (GPIO PB9) | External pressure sensor  |

### GPIO Pin Summary

| Function         | Pin   | Direction | Notes                              |
|------------------|-------|-----------|------------------------------------|
| ADS1292R DRDY    | PG1   | Input     | Active-low, data-ready interrupt   |
| ADS1292R RESET   | PG2   | Output    | Active-low, hold high in normal op |
| ADS1292R START   | PG3   | Output    | High = start conversion            |
| MAX30102 INT     | PB3   | Input     | Active-low, FIFO almost full       |
| TMP117 ALERT     | PB4   | Input     | Active-low, temperature threshold  |
| NIBP Valve Ctrl  | PD0   | Output    | High = close solenoid valve        |
| NIBP Pump Ctrl   | PD1   | Output    | PWM for inflation pump             |

---

## Data Flow: HAL to UI

```
sensor_hal_t (your code)
      |
      v
sensor-service (reads HAL, builds IPC messages)
      |
      | nanomsg pub/sub (ipc:///tmp/vitals-monitor/vitals.ipc)
      v
vitals_provider_ipc.c (in UI process, subscribes to IPC)
      |
      v
vitals_provider.h API (abstract interface)
      |
      v
screen_patient.c / screen_trends.c (UI renders values)
```

The UI code never calls sensor_hal functions directly. It only uses the
`vitals_provider.h` API, which in the target build subscribes to IPC messages
published by the sensor-service.

---

## Testing Procedure

### 1. Unit Test Each Driver

Create test files in `tests/unit/`:

```
tests/unit/test_hal_ecg_ads1292r.c
tests/unit/test_hal_spo2_max30102.c
tests/unit/test_hal_temp_tmp117.c
tests/unit/test_hal_nibp.c
```

Each test should verify:
- `init()` returns true (hardware present and communicating)
- `self_test()` passes (chip ID matches expected value)
- `calibrate()` completes without error
- `start_sampling()` / `read_samples()` returns valid data
- `deinit()` releases resources cleanly

### 2. Test with sensor-service Standalone

Run the sensor-service on the target and verify IPC output:

```bash
# Terminal 1: start sensor-service
/opt/vitals-monitor/bin/sensor-service

# Terminal 2: subscribe to IPC and print messages
nanocat --sub --connect ipc:///tmp/vitals-monitor/vitals.ipc --raw
```

Verify:
- Messages arrive at 1 Hz for vitals
- Waveform packets arrive at 10-20 Hz
- Values are within physiological range

### 3. Integration Test with UI

Build the full system and boot on the target:

```bash
# All 5 services start via systemd
systemctl status sensor-service alarm-service vitals-ui network-service watchdog
```

Verify:
- Heart rate, SpO2, temperature display on the patient screen
- ECG and pleth waveforms render smoothly at 30 FPS
- Alarm triggers when finger is removed from SpO2 probe
- Trends screen shows historical data after a few minutes

### 4. Stress Test

- Run for 24 hours continuously and check for memory leaks
- Disconnect and reconnect sensors -- verify recovery
- Kill sensor-service and verify watchdog restarts it
- Verify no data loss across service restarts

---

## Common Pitfalls

1. **SPI clock too fast** -- ADS1292R max is 4 MHz for SCLK. Start at 1 MHz
   and increase only after verifying stable communication.

2. **I2C clock stretching** -- MAX30102 can stretch SCL. Ensure the I2C
   controller has clock stretching support enabled in the device tree.

3. **FIFO overflow** -- Both ADS1292R and MAX30102 have internal FIFOs. If
   `read_samples()` is not called frequently enough, data will be lost.
   The sensor-service polls at the configured sample rate.

4. **Thread safety** -- The sensor-service calls HAL functions from a single
   thread. You do NOT need to make your driver thread-safe.

5. **Power sequencing** -- Some sensors require specific power-on timing.
   Handle this in `init()` and document any required delays.

---

## Contact

If you encounter issues with the HAL interface, IPC message formats, or
data flow architecture, file an issue or contact the UI development team
before modifying any shared interfaces (`sensor_hal.h`, `vitals_provider.h`,
`ipc_messages.h`).
