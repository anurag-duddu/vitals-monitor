# Development Environment Setup: Bedside Vitals Monitor

## Document Information

| Field | Value |
|-------|-------|
| Document ID | REQ-010 |
| Version | 1.0 |
| Status | Draft |
| Parent Document | REQ-001 Product Overview |

---

## 1. Overview

This document provides setup instructions for developing the bedside vitals monitor application. The development environment supports macOS as the primary development platform, with Linux builds handled via Docker containers.

### 1.1 Development Philosophy

The development workflow is designed around these principles:

1. **Fast iteration on Mac** — UI and application logic developed locally with instant feedback
2. **Reproducible builds** — Docker containers ensure consistent Linux build environment
3. **Hardware testing when needed** — Real target hardware for integration and validation
4. **CI/CD ready** — Same Docker environment runs in CI pipelines

### 1.2 Environment Overview

| Environment | Purpose | Platform |
|-------------|---------|----------|
| Simulator | UI development, unit tests, fast iteration | Mac native |
| Docker Build | Cross-compilation, system images | Linux container |
| Target Hardware | Integration testing, validation | ARM board |

---

## 2. Prerequisites

### 2.1 Hardware Requirements

| Component | Minimum | Recommended |
|-----------|---------|-------------|
| Mac | Apple Silicon or Intel | Apple Silicon (M1/M2/M3) |
| RAM | 8 GB | 16 GB |
| Storage | 50 GB free | 100 GB free (Buildroot cache) |
| Target Board | STM32MP1 Discovery Kit | STM32MP157F-DK2 |

### 2.2 Software Requirements

| Software | Version | Purpose |
|----------|---------|---------|
| macOS | 12+ (Monterey) | Host OS |
| Xcode Command Line Tools | Latest | Compiler, build tools |
| Homebrew | Latest | Package manager |
| Docker Desktop | Latest | Linux container environment |
| Git | 2.x | Version control |
| VS Code or CLion | Latest | IDE (optional) |

---

## 3. Mac Development Setup

### 3.1 Install Core Tools

```bash
# Install Xcode command line tools
xcode-select --install

# Install Homebrew (if not present)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install development dependencies
brew install \
    cmake \
    ninja \
    sdl2 \
    pkg-config \
    git \
    python3 \
    node
```

### 3.2 Install Docker Desktop

1. Download Docker Desktop from https://docker.com/products/docker-desktop
2. Install and launch
3. Allocate resources in Preferences:
   - CPUs: 4+ recommended
   - Memory: 8 GB+ recommended
   - Disk: 64 GB+ recommended

### 3.3 Clone Project Repository

```bash
# Clone the project
git clone <repository-url> vitals-monitor
cd vitals-monitor

# Initialize submodules (LVGL, etc.)
git submodule update --init --recursive
```

---

## 4. LVGL Simulator Setup (Mac Native)

### 4.1 Overview

The LVGL simulator uses SDL2 to render the UI in a window on your Mac. This provides:

- Instant compilation and preview
- Mouse as touch input
- Keyboard input simulation
- Same LVGL code runs on target

### 4.2 Project Structure for Simulator

```
vitals-monitor/
├── simulator/                 # Mac simulator build
│   ├── CMakeLists.txt
│   ├── main.c                 # Simulator entry point
│   └── lv_conf.h              # LVGL config for simulator
├── src/
│   ├── ui/                    # Shared UI code
│   │   ├── screens/
│   │   ├── widgets/
│   │   └── themes/
│   ├── core/                  # Application logic
│   └── common/                # Shared utilities
├── lvgl/                      # LVGL submodule
├── lv_drivers/                # LVGL drivers submodule
└── target/                    # Target-specific code
```

### 4.3 Simulator CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(vitals_simulator C)

set(CMAKE_C_STANDARD 99)

# Find SDL2
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

# LVGL configuration
set(LV_CONF_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h)
add_definitions(-DLV_CONF_INCLUDE_SIMPLE)

# Include LVGL
add_subdirectory(../lvgl lvgl)
add_subdirectory(../lv_drivers lv_drivers)

# Simulator executable
add_executable(simulator
    main.c
    ../src/ui/screens/main_vitals.c
    ../src/ui/screens/trends.c
    ../src/ui/screens/settings.c
    ../src/ui/widgets/waveform.c
    ../src/ui/widgets/numeric_display.c
    ../src/ui/widgets/alarm_banner.c
    ../src/core/vital_processor.c
    ../src/core/alarm_engine.c
    ../src/core/mock_sensors.c      # Simulated sensor data
)

target_include_directories(simulator PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ../src
    ../lvgl
    .
)

target_link_libraries(simulator
    ${SDL2_LIBRARIES}
    lvgl
    lv_drivers
    m
    pthread
)
```

### 4.4 Building and Running Simulator

```bash
# Create build directory
mkdir -p simulator/build
cd simulator/build

# Configure
cmake ..

# Build
cmake --build . -j$(sysctl -n hw.ncpu)

# Run
./simulator
```

### 4.5 Simulator Features

| Feature | Implementation |
|---------|----------------|
| Touch input | Mouse click and drag |
| Screen resolution | Configurable in lv_conf.h |
| Frame rate | Matches target (30 FPS default) |
| Sensor data | Mock data generator |
| Alarms | Simulated triggers |

### 4.6 Mock Sensor Data

For simulator builds, sensor data is generated by mock implementations:

```c
// src/core/mock_sensors.c

#include "sensors.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>

static float base_hr = 72.0f;
static float base_spo2 = 98.0f;

void mock_sensors_init(void) {
    srand(time(NULL));
}

vitals_data_t mock_sensors_read(void) {
    vitals_data_t data = {0};
    
    // Simulate realistic variation
    data.heart_rate = base_hr + ((rand() % 100) - 50) / 50.0f;
    data.spo2 = base_spo2 + ((rand() % 100) - 50) / 100.0f;
    data.resp_rate = 16.0f + ((rand() % 100) - 50) / 50.0f;
    data.temperature = 36.8f + ((rand() % 100) - 50) / 200.0f;
    data.timestamp = get_timestamp_ms();
    
    return data;
}

// Generate synthetic ECG waveform
void mock_ecg_samples(float *buffer, size_t count, uint32_t sample_rate) {
    static float phase = 0.0f;
    float hr_hz = base_hr / 60.0f;
    
    for (size_t i = 0; i < count; i++) {
        float t = phase / sample_rate;
        // Simplified ECG shape
        buffer[i] = generate_ecg_shape(t * hr_hz);
        phase += 1.0f;
    }
}
```

---

## 5. Docker Build Environment

### 5.1 Docker Image Setup

Create a Dockerfile for the build environment:

```dockerfile
# Dockerfile.buildroot
FROM ubuntu:22.04

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install Buildroot dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    g++ \
    make \
    cmake \
    ninja-build \
    git \
    wget \
    curl \
    unzip \
    bc \
    bison \
    flex \
    libssl-dev \
    libncurses5-dev \
    libelf-dev \
    cpio \
    rsync \
    python3 \
    python3-pip \
    file \
    locales \
    && rm -rf /var/lib/apt/lists/*

# Set locale
RUN locale-gen en_US.UTF-8
ENV LANG=en_US.UTF-8

# Create non-root user (Buildroot requirement)
RUN useradd -m builder
USER builder
WORKDIR /home/builder

# Download Buildroot
ARG BUILDROOT_VERSION=2024.02.1
RUN wget https://buildroot.org/downloads/buildroot-${BUILDROOT_VERSION}.tar.gz \
    && tar xzf buildroot-${BUILDROOT_VERSION}.tar.gz \
    && rm buildroot-${BUILDROOT_VERSION}.tar.gz \
    && mv buildroot-${BUILDROOT_VERSION} buildroot

WORKDIR /home/builder/buildroot

CMD ["/bin/bash"]
```

### 5.2 Build the Docker Image

```bash
# Build the image
docker build -t vitals-buildroot -f Dockerfile.buildroot .

# Verify
docker images | grep vitals-buildroot
```

### 5.3 Running the Build Container

```bash
# Run with project mounted
docker run -it \
    -v $(pwd):/home/builder/project \
    -v vitals-buildroot-cache:/home/builder/buildroot/dl \
    vitals-buildroot

# Inside container:
cd /home/builder/project
```

### 5.4 Build Script

Create a convenience script for builds:

```bash
#!/bin/bash
# scripts/docker-build.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

docker run --rm \
    -v "$PROJECT_DIR":/home/builder/project \
    -v vitals-buildroot-cache:/home/builder/buildroot/dl \
    -v vitals-buildroot-output:/home/builder/buildroot/output \
    vitals-buildroot \
    /bin/bash -c "cd /home/builder/project && make $*"
```

Usage:

```bash
# Full build
./scripts/docker-build.sh all

# Clean build
./scripts/docker-build.sh clean

# Just the application
./scripts/docker-build.sh app
```

---

## 6. Buildroot Configuration

### 6.1 External Tree Structure

```
buildroot-external/
├── Config.in
├── external.mk
├── external.desc
├── board/
│   └── vitals-monitor/
│       ├── linux.config          # Kernel config
│       ├── uboot.config          # U-Boot config
│       ├── genimage.cfg          # Image layout
│       ├── post_build.sh         # Post-build hook
│       ├── post_image.sh         # Post-image hook
│       └── overlay/              # Filesystem overlay
│           ├── etc/
│           │   ├── init.d/
│           │   └── inittab
│           └── usr/
│               └── share/
├── configs/
│   └── vitals_monitor_defconfig  # Board defconfig
└── package/
    ├── sensor-service/
    │   ├── Config.in
    │   └── sensor-service.mk
    ├── alarm-service/
    │   ├── Config.in
    │   └── alarm-service.mk
    ├── ui-app/
    │   ├── Config.in
    │   └── ui-app.mk
    └── ...
```

### 6.2 Sample Defconfig

```makefile
# configs/vitals_monitor_defconfig

# Architecture
BR2_arm=y
BR2_cortex_a7=y
BR2_ARM_FPU_NEON_VFPV4=y

# Toolchain
BR2_TOOLCHAIN_BUILDROOT_GLIBC=y
BR2_TOOLCHAIN_BUILDROOT_CXX=y

# System
BR2_SYSTEM_DHCP="eth0"
BR2_TARGET_GENERIC_HOSTNAME="vitals-monitor"
BR2_TARGET_GENERIC_ISSUE="Vitals Monitor"
BR2_ROOTFS_OVERLAY="$(BR2_EXTERNAL_VITALS_PATH)/board/vitals-monitor/overlay"

# Kernel
BR2_LINUX_KERNEL=y
BR2_LINUX_KERNEL_USE_CUSTOM_CONFIG=y
BR2_LINUX_KERNEL_CUSTOM_CONFIG_FILE="$(BR2_EXTERNAL_VITALS_PATH)/board/vitals-monitor/linux.config"

# Filesystem
BR2_TARGET_ROOTFS_SQUASHFS=y
BR2_TARGET_ROOTFS_SQUASHFS4_LZ4=y

# Packages - System
BR2_PACKAGE_BUSYBOX=y
BR2_PACKAGE_DROPBEAR=y

# Packages - Display
BR2_PACKAGE_LIBDRM=y
BR2_PACKAGE_MESA3D=y

# Packages - Our applications
BR2_PACKAGE_SENSOR_SERVICE=y
BR2_PACKAGE_ALARM_SERVICE=y
BR2_PACKAGE_UI_APP=y
BR2_PACKAGE_NETWORK_SERVICE=y
BR2_PACKAGE_AUDIT_SERVICE=y

# Security
BR2_PACKAGE_LIBSECCOMP=y
```

### 6.3 Building with Buildroot

```bash
# Inside Docker container

# Set external tree
cd /home/builder/buildroot
export BR2_EXTERNAL=/home/builder/project/buildroot-external

# Load defconfig
make vitals_monitor_defconfig

# Build (takes 30-60 min first time)
make -j$(nproc)

# Output images in output/images/
ls output/images/
# rootfs.squashfs  zImage  stm32mp157f-dk2.dtb
```

---

## 7. Target Hardware Setup

### 7.1 Recommended Development Board

**STM32MP157F-DK2** (Discovery Kit)

| Feature | Specification |
|---------|---------------|
| SoC | STM32MP157 (Cortex-A7 + M4) |
| RAM | 512 MB DDR3 |
| Storage | microSD slot |
| Display | 4.3" MIPI-DSI touchscreen |
| Connectivity | Ethernet, WiFi, Bluetooth |
| Debug | ST-LINK/V2-1 integrated |

This is ST's reference board for the MP1 series — well-documented and supported.

### 7.2 Flashing the Target

```bash
# Insert microSD card into Mac

# Find the device
diskutil list

# Unmount (replace diskN with actual)
diskutil unmountDisk /dev/diskN

# Flash (use rdisk for faster transfer)
sudo dd if=output/images/sdcard.img of=/dev/rdiskN bs=1m status=progress

# Eject
diskutil eject /dev/diskN
```

### 7.3 Serial Console Access

```bash
# Install serial terminal
brew install minicom

# Find serial device (usually /dev/tty.usbmodemXXXX)
ls /dev/tty.usb*

# Connect
minicom -D /dev/tty.usbmodem14203 -b 115200
```

### 7.4 Network Access

```bash
# Find board IP (from serial console or router)
# Then SSH in
ssh root@<board-ip>
```

---

## 8. Debugging

### 8.1 Simulator Debugging (Mac)

Use LLDB or GDB on Mac:

```bash
# Build with debug symbols
cd simulator/build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .

# Run with LLDB
lldb ./simulator

# Or in VS Code with CodeLLDB extension
```

**VS Code launch.json:**

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug Simulator",
            "type": "lldb",
            "request": "launch",
            "program": "${workspaceFolder}/simulator/build/simulator",
            "args": [],
            "cwd": "${workspaceFolder}/simulator/build"
        }
    ]
}
```

### 8.2 Remote Debugging (Target)

```bash
# On target: start gdbserver
gdbserver :2345 /usr/bin/ui-app

# On Mac: connect with GDB (ARM)
# Use GDB from ARM toolchain in Docker, or
# Use VS Code with remote debugging

# In Docker:
arm-linux-gnueabihf-gdb
(gdb) target remote <board-ip>:2345
(gdb) continue
```

### 8.3 Logging

```bash
# On target: view logs
journalctl -f                    # All logs
journalctl -u ui-app -f          # Specific service
tail -f /var/log/audit.log       # Audit logs

# Copy logs to Mac
scp root@<board-ip>:/var/log/*.log ./logs/
```

---

## 9. Development Workflow

### 9.1 Daily Development Cycle

```
┌────────────────────────────────────────────────────────────────┐
│ 1. Write code on Mac                                           │
│    - UI screens, widgets, application logic                    │
│    - Use IDE with LVGL intellisense                            │
└──────────────────────────┬─────────────────────────────────────┘
                           │
                           ▼
┌────────────────────────────────────────────────────────────────┐
│ 2. Build and test in simulator                                 │
│    $ cd simulator/build && cmake --build . && ./simulator      │
│    - Fast iteration (seconds)                                  │
│    - Visual verification                                       │
└──────────────────────────┬─────────────────────────────────────┘
                           │
                           ▼
┌────────────────────────────────────────────────────────────────┐
│ 3. Run unit tests                                              │
│    $ cd tests && ctest                                         │
│    - Verify logic correctness                                  │
└──────────────────────────┬─────────────────────────────────────┘
                           │
                           ▼
┌────────────────────────────────────────────────────────────────┐
│ 4. Commit and push                                             │
│    $ git commit -m "Add feature X"                             │
│    $ git push                                                  │
│    - CI runs Docker build + tests                              │
└────────────────────────────────────────────────────────────────┘
```

### 9.2 Hardware Integration Cycle

```
┌────────────────────────────────────────────────────────────────┐
│ 1. Build target image                                          │
│    $ ./scripts/docker-build.sh all                             │
│    - Cross-compile for ARM                                     │
│    - Generate rootfs image                                     │
└──────────────────────────┬─────────────────────────────────────┘
                           │
                           ▼
┌────────────────────────────────────────────────────────────────┐
│ 2. Flash to SD card                                            │
│    $ sudo dd if=sdcard.img of=/dev/rdiskN bs=1m                │
└──────────────────────────┬─────────────────────────────────────┘
                           │
                           ▼
┌────────────────────────────────────────────────────────────────┐
│ 3. Boot and test on target                                     │
│    - Serial console for boot logs                              │
│    - SSH for runtime access                                    │
│    - Test with real sensors                                    │
└──────────────────────────┬─────────────────────────────────────┘
                           │
                           ▼
┌────────────────────────────────────────────────────────────────┐
│ 4. Debug if needed                                             │
│    - gdbserver for remote debugging                            │
│    - journalctl for logs                                       │
└────────────────────────────────────────────────────────────────┘
```

### 9.3 Quick Deploy (Development)

For faster iteration during hardware integration, deploy just the application without reflashing:

```bash
# Build only the app package
./scripts/docker-build.sh ui-app

# Copy to target over SSH
scp output/target/usr/bin/ui-app root@<board-ip>:/usr/bin/

# Restart the service on target
ssh root@<board-ip> "systemctl restart ui-app"
```

---

## 10. Testing

### 10.1 Test Structure

```
tests/
├── unit/
│   ├── test_vital_processor.c
│   ├── test_alarm_engine.c
│   ├── test_trend_storage.c
│   └── CMakeLists.txt
├── integration/
│   ├── test_sensor_to_ui.c
│   ├── test_alarm_flow.c
│   └── CMakeLists.txt
└── mocks/
    ├── mock_sensors.c
    ├── mock_display.c
    └── mock_network.c
```

### 10.2 Running Unit Tests

```bash
# Build tests
cd tests/unit/build
cmake ..
cmake --build .

# Run
ctest --output-on-failure

# Or run specific test
./test_alarm_engine
```

### 10.3 Test Framework

Use **Unity** (C) or **Criterion** (C) for unit tests:

```c
// tests/unit/test_alarm_engine.c

#include "unity.h"
#include "alarm_engine.h"

void setUp(void) {
    alarm_engine_init();
}

void tearDown(void) {
    alarm_engine_deinit();
}

void test_high_hr_triggers_alarm(void) {
    alarm_config_t config = {
        .hr_high = 120,
        .hr_low = 40
    };
    alarm_engine_configure(&config);
    
    vitals_data_t vitals = {
        .heart_rate = 130  // Above threshold
    };
    
    alarm_result_t result = alarm_engine_evaluate(&vitals);
    
    TEST_ASSERT_TRUE(result.alarm_active);
    TEST_ASSERT_EQUAL(ALARM_PRIORITY_HIGH, result.priority);
    TEST_ASSERT_EQUAL(ALARM_PARAM_HR, result.parameter);
}

void test_normal_hr_no_alarm(void) {
    vitals_data_t vitals = {
        .heart_rate = 72  // Normal
    };
    
    alarm_result_t result = alarm_engine_evaluate(&vitals);
    
    TEST_ASSERT_FALSE(result.alarm_active);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_high_hr_triggers_alarm);
    RUN_TEST(test_normal_hr_no_alarm);
    return UNITY_END();
}
```

---

## 11. CI/CD Integration

### 11.1 GitHub Actions Example

```yaml
# .github/workflows/build.yml

name: Build and Test

on:
  push:
    branches: [main, develop]
  pull_request:
    branches: [main]

jobs:
  simulator:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive
      
      - name: Install dependencies
        run: brew install sdl2 cmake ninja
      
      - name: Build simulator
        run: |
          cd simulator
          mkdir build && cd build
          cmake ..
          cmake --build .
      
      - name: Run unit tests
        run: |
          cd tests/unit/build
          cmake ..
          cmake --build .
          ctest --output-on-failure

  target-build:
    runs-on: ubuntu-latest
    container:
      image: ghcr.io/your-org/vitals-buildroot:latest
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive
      
      - name: Build target image
        run: |
          cd buildroot
          make BR2_EXTERNAL=../buildroot-external vitals_monitor_defconfig
          make -j$(nproc)
      
      - name: Upload artifacts
        uses: actions/upload-artifact@v4
        with:
          name: target-images
          path: buildroot/output/images/
```

### 11.2 Pre-commit Hooks

```bash
# .pre-commit-config.yaml

repos:
  - repo: local
    hooks:
      - id: clang-format
        name: clang-format
        entry: clang-format -i
        language: system
        types: [c]
      
      - id: build-simulator
        name: build simulator
        entry: bash -c 'cd simulator/build && cmake --build .'
        language: system
        pass_filenames: false
      
      - id: run-tests
        name: run tests
        entry: bash -c 'cd tests/unit/build && ctest'
        language: system
        pass_filenames: false
```

---

## 12. IDE Setup

### 12.1 VS Code

**Extensions:**
- C/C++ (Microsoft)
- CodeLLDB (for debugging)
- CMake Tools
- clangd (better intellisense)

**Settings (.vscode/settings.json):**

```json
{
    "C_Cpp.default.includePath": [
        "${workspaceFolder}/src",
        "${workspaceFolder}/lvgl",
        "${workspaceFolder}/lv_drivers",
        "${workspaceFolder}/simulator"
    ],
    "C_Cpp.default.defines": [
        "LV_CONF_INCLUDE_SIMPLE",
        "SIMULATOR=1"
    ],
    "cmake.buildDirectory": "${workspaceFolder}/simulator/build",
    "files.associations": {
        "lv_conf.h": "c"
    }
}
```

### 12.2 CLion

CLion works well with CMake projects. Open the `simulator/CMakeLists.txt` as project root.

**Toolchain:** Default system (Apple Clang)

**CMake Options:** `-DCMAKE_BUILD_TYPE=Debug`

---

## 13. Troubleshooting

### 13.1 Common Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| SDL window doesn't open | SDL2 not found | `brew install sdl2` and reconfigure CMake |
| Buildroot fails with permission error | Running as root | Use non-root user in Docker |
| Cross-compile fails | Wrong toolchain | Check BR2_TOOLCHAIN settings |
| Serial console garbled | Wrong baud rate | Ensure 115200 baud |
| Can't SSH to target | Network not up | Check Ethernet cable, DHCP |
| Display blank on target | Wrong framebuffer | Check device tree, display driver |

### 13.2 Useful Commands

```bash
# Check LVGL version
grep LV_VERSION lvgl/lv_version.h

# Clean simulator build
cd simulator/build && rm -rf * && cmake .. && cmake --build .

# Clean Buildroot (full)
cd buildroot && make clean

# Clean Buildroot (keep downloads)
cd buildroot && make distclean

# Check target disk space
ssh root@<board-ip> df -h

# Check target memory
ssh root@<board-ip> free -m

# Check running services
ssh root@<board-ip> systemctl status
```

---

## 14. Quick Reference

### 14.1 Build Commands

| Task | Command |
|------|---------|
| Build simulator | `cd simulator/build && cmake --build .` |
| Run simulator | `./simulator` |
| Run tests | `cd tests && ctest` |
| Docker build shell | `./scripts/docker-build.sh shell` |
| Full target build | `./scripts/docker-build.sh all` |
| Flash SD card | `sudo dd if=sdcard.img of=/dev/rdiskN bs=1m` |

### 14.2 Target Commands

| Task | Command |
|------|---------|
| Serial console | `minicom -D /dev/tty.usbmodemXXX -b 115200` |
| SSH access | `ssh root@<board-ip>` |
| View logs | `journalctl -f` |
| Restart UI | `systemctl restart ui-app` |
| Check sensors | `cat /sys/bus/i2c/devices/*/status` |

### 14.3 Environment Variables

| Variable | Purpose | Example |
|----------|---------|---------|
| `BR2_EXTERNAL` | Buildroot external tree | `/path/to/buildroot-external` |
| `SIMULATOR` | Build for simulator | `1` |
| `DEBUG` | Enable debug output | `1` |
| `TARGET_IP` | Board IP for scripts | `192.168.1.100` |

---

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2024 | — | Initial draft |
