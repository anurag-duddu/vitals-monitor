# Hardware Integration Guide

## Overview

This guide is for the **remote hardware team** integrating the vitals monitor software with real sensors on the STM32MP157F-DK2 target hardware.

The codebase is designed so that:
1. **UI code is identical** between simulator and target
2. **Data flows through abstraction layers** that you implement
3. **IPC message formats are pre-defined** for compatibility

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         UI Application                           │
│                    (screens, widgets, themes)                    │
│                      SAME CODE, NO CHANGES                       │
├─────────────────────────────────────────────────────────────────┤
│                     vitals_provider.h                            │
│              (abstract interface for vital signs)                │
├───────────────────────┬─────────────────────────────────────────┤
│   SIMULATOR BUILD     │          TARGET BUILD                    │
│ vitals_provider_mock.c│     vitals_provider_ipc.c               │
│  (generates fake data)│   (receives from sensor-service)        │
└───────────────────────┴──────────────┬──────────────────────────┘
                                       │ nanomsg IPC
                        ┌──────────────┴──────────────┐
                        │       sensor-service         │
                        │   (YOUR IMPLEMENTATION)      │
                        ├──────────────────────────────┤
                        │        sensor_hal.h          │
                        │    (hardware abstraction)    │
                        ├───────┬───────┬───────┬──────┤
                        │ SpO2  │  ECG  │ NIBP  │ Temp │
                        │ HAL   │  HAL  │  HAL  │ HAL  │
                        │(impl) │(impl) │(impl) │(impl)│
                        └───────┴───────┴───────┴──────┘
                              ↓       ↓       ↓      ↓
                        ┌─────────────────────────────────┐
                        │     OEM Sensor Modules          │
                        │  (I2C/SPI/UART interfaces)      │
                        └─────────────────────────────────┘
```

---

## What You Need to Implement

### 1. Sensor HAL Implementations

Location: `src/drivers/<sensor>/hal_<sensor>_<vendor>.c`

**Files to create:**
- `src/drivers/spo2/hal_spo2_<your_vendor>.c`
- `src/drivers/ecg/hal_ecg_<your_vendor>.c`
- `src/drivers/nibp/hal_nibp_<your_vendor>.c`
- `src/drivers/temp/hal_temp_<your_vendor>.c`

**Interface:** Implement the ops structures from `src/drivers/sensor_hal.h`

Example skeleton for SpO2:

```c
// src/drivers/spo2/hal_spo2_example.c

#include "sensor_hal.h"
#include <stdio.h>

static int spo2_init(void) {
    // Initialize I2C/SPI communication with your OEM module
    // Return 0 on success
    return 0;
}

static void spo2_deinit(void) {
    // Cleanup
}

static sensor_state_t spo2_get_state(void) {
    return SENSOR_STATE_READY;  // Or appropriate state
}

static sensor_error_t spo2_get_error(void) {
    return SENSOR_ERROR_NONE;
}

static bool spo2_is_ready(void) {
    return true;
}

static int spo2_get_reading(spo2_reading_t *reading) {
    // Read from your OEM module
    reading->spo2 = 98;          // Example
    reading->pulse_rate = 72;
    reading->signal_quality = 95;
    reading->finger_detected = true;
    return 0;
}

static spo2_waveform_cb_t g_waveform_cb = NULL;
static void *g_waveform_user_data = NULL;

static void spo2_set_waveform_callback(spo2_waveform_cb_t cb, void *user_data) {
    g_waveform_cb = cb;
    g_waveform_user_data = user_data;
}

// Register this implementation
static const sensor_ops_spo2_t spo2_ops = {
    .base = {
        .name = "Example SpO2",
        .vendor = "Example Vendor",
        .init = spo2_init,
        .deinit = spo2_deinit,
        .get_state = spo2_get_state,
        .get_error = spo2_get_error,
        .is_ready = spo2_is_ready,
    },
    .get_reading = spo2_get_reading,
    .set_waveform_callback = spo2_set_waveform_callback,
};

// Called at startup
void spo2_hal_register(void) {
    sensor_hal_register_spo2(&spo2_ops);
}
```

### 2. sensor-service Daemon

Location: `src/services/sensor-service/`

**Responsibilities:**
- Initialize all sensor HALs
- Read sensors at appropriate intervals
- Publish data via nanomsg

**Key files to create:**
- `src/services/sensor-service/main.c`
- `src/services/sensor-service/sensor_service.c`
- `src/services/sensor-service/CMakeLists.txt`

**Example main loop:**

```c
#include "sensor_hal.h"
#include "ipc_messages.h"
#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>

int main(void) {
    // Initialize sensors
    sensor_hal_init_all();

    // Create nanomsg publisher
    int sock = nn_socket(AF_SP, NN_PUB);
    nn_bind(sock, IPC_SOCKET_VITALS);

    while (running) {
        // Read all sensors
        spo2_reading_t spo2;
        ecg_reading_t ecg;
        // ...

        // Build IPC message
        ipc_msg_vitals_t msg = {
            .header = {
                .msg_type = IPC_MSG_VITALS,
                .payload_len = sizeof(msg) - sizeof(msg.header),
                .version = IPC_PROTOCOL_VERSION,
            },
            .hr = ecg.heart_rate,
            .spo2 = spo2.spo2,
            // ...
        };

        // Publish
        nn_send(sock, &msg, sizeof(msg), 0);

        usleep(1000000);  // 1 second
    }

    nn_close(sock);
    sensor_hal_deinit_all();
    return 0;
}
```

### 3. Cortex-M4 Firmware (ECG Sampling)

The M4 must handle time-critical ECG sampling at 500 Hz with <1ms jitter.

**Location:** `firmware/m4/`

**Communication:** Use RPMsg or shared memory to send samples to the A7.

**Responsibilities:**
- Configure ADC for ECG analog front-end
- Sample at 500 Hz
- Perform basic signal conditioning
- Send samples to A7 via RPMsg

---

## IPC Message Format

All messages are defined in: `src/common/ipc/ipc_messages.h`

**Socket paths:**
- `ipc:///tmp/vitals-monitor/vitals.ipc` - Vital signs (1 Hz)
- `ipc:///tmp/vitals-monitor/waveforms.ipc` - Waveforms (10-20 Hz)
- `ipc:///tmp/vitals-monitor/alarms.ipc` - Alarm events

**Message structure:**
```c
typedef struct {
    uint8_t  msg_type;      // See ipc_msg_type_t
    uint16_t payload_len;
    uint8_t  version;       // Currently 1
} ipc_msg_header_t;
```

**DO NOT MODIFY** the message structures without coordinating. The UI depends on this format.

---

## Build Configuration

### Simulator Build (your reference)
```bash
cd simulator
mkdir -p build && cd build
cmake -DVITALS_PROVIDER=mock ..
cmake --build .
./simulator
```

### Target Build with IPC
```bash
# In Buildroot or cross-compile environment
cmake -DVITALS_PROVIDER=ipc ..
```

### Target Build for Testing (mock data, no IPC)
```bash
cmake -DVITALS_PROVIDER=direct ..
```

---

## Testing Strategy

### 1. Unit Test Your HAL

Create unit tests for each sensor HAL:
```
tests/unit/test_hal_spo2.c
tests/unit/test_hal_ecg.c
...
```

### 2. Test sensor-service Standalone

Run sensor-service and verify it publishes valid IPC messages:
```bash
# Terminal 1: Run sensor-service
./sensor-service

# Terminal 2: Monitor IPC
nn_cat --subscribe "ipc:///tmp/vitals-monitor/vitals.ipc"
```

### 3. Integration Test with UI

Build the UI with IPC provider and verify data flows:
```bash
# Terminal 1
./sensor-service

# Terminal 2
./ui-app
```

---

## Checklist Before Pushing

- [ ] Sensor HAL implementations compile without warnings
- [ ] sensor-service publishes valid IPC messages
- [ ] IPC message format matches `ipc_messages.h` exactly
- [ ] UI receives and displays data correctly
- [ ] Waveform data renders smoothly (30 FPS)
- [ ] All unit tests pass
- [ ] No memory leaks (run with valgrind)

---

## Contact

If you encounter issues with the abstraction layer or message formats, file an issue or contact the UI development team before making changes to shared interfaces.
