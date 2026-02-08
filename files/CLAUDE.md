# Bedside Vitals Monitor - Claude Code Project

## What This Project Is

A bedside vitals monitor for Indian hospital general wards. It's a **real medical device** — not a prototype — targeting CDSCO Class B regulatory approval.

**Hardware:** STM32MP157F-DK2 (Cortex-A7 + M4, 512MB RAM, 4" touchscreen)
**Software:** Embedded Linux (Buildroot) + LVGL UI framework
**License constraint:** Open source only, no GPL in application code (MIT/LGPL OK)

## Project Structure

```
vitals-monitor/
├── CLAUDE.md                 # This file
├── PRD.md                    # Master product requirements
├── docs/
│   └── requirements/         # Detailed requirements (REQ-001 to REQ-010)
├── src/
│   ├── ui/                   # LVGL UI code
│   │   ├── screens/          # Screen implementations
│   │   ├── widgets/          # Custom widgets (waveform, numeric display)
│   │   └── themes/           # Styling
│   ├── core/                 # Application logic
│   │   ├── vital_processor.c
│   │   ├── alarm_engine.c
│   │   ├── trend_storage.c
│   │   └── patient_manager.c
│   ├── services/             # Background services
│   │   ├── sensor-service/   # Sensor reading daemon
│   │   ├── alarm-service/    # Alarm evaluation daemon
│   │   ├── network-service/  # EHR/ABDM sync daemon
│   │   └── audit-service/    # Logging daemon
│   ├── drivers/              # Hardware abstraction
│   │   ├── spo2/
│   │   ├── ecg/
│   │   ├── nibp/
│   │   └── temp/
│   └── common/               # Shared utilities
│       ├── ipc/              # nanomsg/D-Bus wrappers
│       ├── config/           # Configuration management
│       └── crypto/           # Encryption utilities
├── simulator/                # Mac SDL simulator
│   ├── CMakeLists.txt
│   ├── main.c
│   └── lv_conf.h
├── buildroot-external/       # Buildroot customization
│   ├── board/vitals-monitor/
│   ├── configs/
│   └── package/
├── tests/
│   ├── unit/
│   └── integration/
└── scripts/
    ├── docker-build.sh
    └── flash-sd.sh
```

## Technology Stack

| Layer | Technology | Notes |
|-------|------------|-------|
| UI | LVGL 9.x | MIT license, C API |
| Language | C (C99) | LVGL native, embedded standard |
| Build (Mac) | CMake + SDL2 | Simulator for development |
| Build (Target) | Buildroot | Docker container |
| Database | SQLite 3 | Trends, config, audit logs |
| IPC | nanomsg | Sensor data pub/sub |
| IPC | D-Bus | Control messages |
| Encryption | mbedTLS | TLS, data at rest |
| Target OS | Linux 5.x/6.x | Buildroot-generated |

## Key Design Decisions

### 1. Multi-Process Architecture

Each major function runs as a separate Linux process:
- **sensor-service**: Reads I2C/SPI sensors, publishes via nanomsg
- **alarm-service**: Subscribes to vitals, evaluates alarms
- **ui-app**: LVGL application, subscribes to vitals/alarms
- **network-service**: EHR/ABDM sync, export queue
- **audit-service**: Security event logging

Why: Isolation, independent restart on crash, cleaner code.

### 2. Cortex-M4 Co-Processor

The STM32MP1 has a Cortex-M4 alongside the A7. Use it for:
- Time-critical ECG sampling (500 Hz, <1ms jitter)
- SpO2 signal acquisition
- Pass processed data to Linux via shared memory or RPMsg

This is optional for initial development — can run everything on A7 first.

### 3. Offline-First

The device must function fully without network. All data stored locally, sync when available. Export queue persists across reboots.

### 4. Security Layers

- Secure boot (verified boot chain)
- dm-verity (read-only rootfs integrity)
- LUKS (encrypted data partition)
- SELinux/AppArmor (runtime confinement)
- No root login, kiosk mode

## Development Workflow

### Daily Development (Mac)

```bash
# Build simulator
cd simulator/build
cmake ..
cmake --build .

# Run
./simulator
```

The simulator uses SDL2 to render LVGL in a window. Mouse = touch.

### Target Build (Docker)

```bash
# Build system image
./scripts/docker-build.sh all

# Output: buildroot/output/images/sdcard.img
```

### Flash to Hardware

```bash
# Insert SD card
sudo dd if=sdcard.img of=/dev/rdiskN bs=1m

# Connect serial console
minicom -D /dev/tty.usbmodem* -b 115200
```

## Coding Guidelines

### LVGL Patterns

```c
// Screen creation pattern
lv_obj_t * create_main_screen(void) {
    lv_obj_t * scr = lv_obj_create(NULL);
    
    // Create widgets
    lv_obj_t * hr_label = lv_label_create(scr);
    lv_label_set_text(hr_label, "72");
    lv_obj_align(hr_label, LV_ALIGN_TOP_RIGHT, -20, 50);
    
    return scr;
}

// Load screen
lv_scr_load_anim(main_screen, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, true);
```

### IPC Patterns

```c
// Publisher (sensor-service)
nn_socket(AF_SP, NN_PUB);
nn_bind(sock, "ipc:///tmp/vitals.ipc");
nn_send(sock, &vitals_msg, sizeof(vitals_msg), 0);

// Subscriber (ui-app)
nn_socket(AF_SP, NN_SUB);
nn_connect(sock, "ipc:///tmp/vitals.ipc");
nn_setsockopt(sock, NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
nn_recv(sock, &vitals_msg, sizeof(vitals_msg), 0);
```

### Thread Safety (UI Updates)

LVGL is not thread-safe. Use message queue pattern:

```c
// From IPC thread
msg_queue_push(&ui_msg_queue, &vitals_update);

// In LVGL timer callback (main thread)
while (msg_queue_pop(&ui_msg_queue, &msg)) {
    update_vitals_display(&msg);
}
```

## File Naming Conventions

| Type | Convention | Example |
|------|------------|---------|
| Screen | `screen_<name>.c` | `screen_main_vitals.c` |
| Widget | `widget_<name>.c` | `widget_waveform.c` |
| Service | `<name>_service.c` | `sensor_service.c` |
| Driver | `drv_<sensor>.c` | `drv_max30102.c` |
| Test | `test_<module>.c` | `test_alarm_engine.c` |

## Common Tasks

### Add a New Screen

1. Create `src/ui/screens/screen_<name>.c`
2. Implement `lv_obj_t * screen_<name>_create(void)`
3. Add navigation in screen manager
4. Add to CMakeLists.txt

### Add a New Widget

1. Create `src/ui/widgets/widget_<name>.c`
2. Define creation function and update API
3. Add to CMakeLists.txt

### Add a New Sensor Driver

1. Create `src/drivers/<sensor>/drv_<chip>.c`
2. Implement standard interface (init, read, deinit)
3. Add to sensor-service
4. Create mock for simulator

### Add a New Service

1. Create `src/services/<name>-service/`
2. Implement main loop with IPC
3. Add systemd unit file in buildroot overlay
4. Add Buildroot package

## Testing

### Unit Tests

```bash
cd tests/unit/build
cmake ..
cmake --build .
ctest --output-on-failure
```

### Simulator Testing

1. Build and run simulator
2. Mock sensor data auto-generates
3. Test UI interactions with mouse
4. Trigger alarms via test controls

### Hardware Testing

1. Flash image to SD card
2. Boot target board
3. Connect real sensors
4. SSH for logs: `journalctl -f`

## Important Constraints

1. **No dynamic memory in critical paths** — Pre-allocate buffers
2. **No blocking in UI thread** — Async everything
3. **No GPL code in application** — Check license of every dependency
4. **Timestamps always UTC** — Convert for display only
5. **All user actions logged** — Audit trail requirement

## Requirements Reference

Detailed requirements are in `docs/requirements/`:

| File | Content |
|------|---------|
| REQ-001 | Product overview, personas, scope |
| REQ-002 | Functional requirements |
| REQ-003 | UI/UX requirements |
| REQ-004 | Alarm requirements |
| REQ-005 | Data storage requirements |
| REQ-006 | Integration requirements (EHR, ABDM) |
| REQ-007 | Security requirements |
| REQ-008 | Regulatory requirements |
| REQ-009 | Architecture requirements |
| REQ-010 | Development environment setup |

## Getting Started

### First Session Goals

1. Set up Mac development environment (Homebrew, SDL2, CMake)
2. Create project structure
3. Get LVGL simulator running with blank screen
4. Display "Hello, Vitals Monitor" text

### Second Session Goals

1. Implement main vitals screen layout
2. Create numeric display widget
3. Create mock sensor data generator
4. Display fake vitals updating every second

### Third Session Goals

1. Implement waveform widget
2. Display synthetic ECG waveform
3. Add basic alarm banner

Build incrementally. Get something on screen fast, then iterate.
