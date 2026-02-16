# SOUP List (Software of Unknown Provenance)

| Field          | Value                                      |
|----------------|--------------------------------------------|
| Document ID    | SOUP-001                                   |
| Version        | 1.0                                        |
| Date           | 2026-02-15                                 |
| Author         | Engineering Team                           |
| Reviewer       | Quality Assurance                          |
| Approval       | Regulatory Affairs                         |
| Classification | IEC 62304 Software Safety Class B          |

---

## 1. Purpose

This document identifies and evaluates all Software of Unknown Provenance (SOUP) used in the Bedside Vitals Monitor, per IEC 62304 clause 8 (Software Configuration Management) and clause 5.3.3 (SOUP identification). SOUP items are third-party software components not developed under the manufacturer's quality management system.

For each SOUP item, this document records the component name, version, license, intended purpose, known anomalies relevant to the device, and a risk assessment.

---

## 2. SOUP Classification

| Classification | Definition |
|----------------|------------|
| Production     | Included in the production device image deployed to clinical environments |
| Development    | Used only during development, testing, or simulation; not present on production hardware |

---

## 3. SOUP Items

### 3.1 LVGL (Light and Versatile Graphics Library)

| Attribute              | Value                                                          |
|------------------------|----------------------------------------------------------------|
| Component Name         | LVGL                                                           |
| Version                | 9.3                                                            |
| License                | MIT                                                            |
| Classification         | Production                                                     |
| Source                  | https://github.com/lvgl/lvgl                                   |
| Purpose                | Embedded graphics UI framework providing touchscreen widgets, waveform rendering, screen management, and animation for the 800x480 display. Core of the ui-app process. |
| Integration Points     | `src/ui/screens/`, `src/ui/widgets/`, `simulator/main.c`       |
| Known Anomalies        | No known anomalies affecting intended use. LVGL 9.3 is a stable release with active community support and regular security reviews. |
| Anomaly Monitoring     | LVGL GitHub issues and release notes reviewed at each project milestone. |
| Risk Assessment        | **Low.** LVGL is a widely deployed embedded graphics library used in medical, automotive, and industrial applications. The MIT license has no copyleft obligations. The project uses static allocation patterns (`LV_MEM_CUSTOM`) to avoid dynamic allocation in critical paths. Display-layer defects would be visible to the user and detectable during system testing. |
| Verification           | Functional verification through simulator-based system tests. Waveform rendering verified at >= 30 FPS. Screen transitions verified for memory safety. |

### 3.2 SQLite

| Attribute              | Value                                                          |
|------------------------|----------------------------------------------------------------|
| Component Name         | SQLite (amalgamation build)                                    |
| Version                | 3.45                                                           |
| License                | Public Domain                                                  |
| Classification         | Production                                                     |
| Source                  | https://sqlite.org/                                            |
| Purpose                | Embedded relational database for on-device storage of vital sign trends (vitals_raw, vitals_1min), NIBP measurements, alarm events, patient demographics, user credentials, audit log entries, settings, and offline sync queue. |
| Integration Points     | `src/core/trend_db.c`, `src/core/patient_data.c`, `src/core/settings_store.c`, `src/core/auth_manager.c`, `src/core/audit_log.c`, `src/core/sync_queue.c` |
| Known Anomalies        | No known anomalies affecting intended use. SQLite is the most widely deployed database engine in the world, used in aviation, automotive, and medical device applications. WAL journaling mode provides crash resilience. |
| Anomaly Monitoring     | SQLite changelog reviewed at each project milestone. CVE monitoring via NVD and sqlite.org announcements. |
| Risk Assessment        | **Low.** SQLite has an extensive test suite (100% branch coverage in the proprietary test harness). The public domain license imposes no restrictions. The amalgamation build ensures a single, verified source file. Data integrity is protected by WAL journaling and application-level retention/purge policies verified by 18 unit tests and 7 integration tests. |
| Verification           | 18 unit tests (test_trend_db.c) verify CRUD, aggregation, purge, and edge cases. 11 unit tests (test_settings_store.c) verify settings persistence. 12 unit tests (test_patient_data.c) verify patient data CRUD. 14 unit tests (test_audit_log.c) verify audit logging. 10 unit tests (test_sync_queue.c) verify sync queue operations. All use in-memory SQLite databases. |

### 3.3 SDL2 (Simple DirectMedia Layer)

| Attribute              | Value                                                          |
|------------------------|----------------------------------------------------------------|
| Component Name         | SDL2                                                           |
| Version                | 2.x (host system version)                                     |
| License                | zlib                                                           |
| Classification         | **Development only** -- not included in production image       |
| Source                  | https://www.libsdl.org/                                        |
| Purpose                | Provides display, input, and timing abstractions for the host simulator build (`./simulator`). Used for development and testing on macOS/Linux workstations. Replaced by framebuffer/DRM drivers on the production STM32MP1 target. |
| Integration Points     | `simulator/CMakeLists.txt`, LVGL SDL2 driver backend           |
| Known Anomalies        | Not applicable -- SDL2 is not present on production hardware.  |
| Anomaly Monitoring     | Not applicable for production risk assessment.                 |
| Risk Assessment        | **None (development only).** SDL2 is not deployed to clinical environments. It is used solely for host-based simulation during development and testing. The zlib license has no copyleft obligations. |
| Verification           | N/A for production. Simulator functionality verified by successful execution of 563 automated tests via the SDL2-backed simulator build. |

### 3.4 nanomsg

| Attribute              | Value                                                          |
|------------------------|----------------------------------------------------------------|
| Component Name         | nanomsg                                                        |
| Version                | 1.2                                                            |
| License                | MIT                                                            |
| Classification         | Production                                                     |
| Source                  | https://github.com/nanomsg/nanomsg                             |
| Purpose                | Lightweight messaging library providing publish/subscribe IPC transport for high-frequency vitals data (1 Hz numeric, 128 Hz waveform) and alarm state changes between sensor-service, alarm-service, and ui-app processes. |
| Integration Points     | `src/common/ipc/ipc_transport.c`, `src/common/ipc/ipc_messages.h` |
| Known Anomalies        | No known anomalies affecting intended use. nanomsg is a stable messaging library with well-defined socket semantics. In the simulator build, nanomsg calls are stubbed out for single-process testing. |
| Anomaly Monitoring     | nanomsg GitHub issues and release notes reviewed at each project milestone. |
| Risk Assessment        | **Low.** nanomsg is a mature, widely used IPC library. The multi-process architecture uses pub/sub patterns with defined message structures (ipc_messages.h). Message loss is handled by the application layer (stale-data indicators, alarm-service independence). The MIT license has no copyleft obligations. |
| Verification           | IPC message structures verified by integration tests. In the simulator build, nanomsg is stubbed via `SIMULATOR_BUILD=1` conditional compilation, with the vitals_provider abstraction providing mock data for testing. |

### 3.5 Linux Kernel

| Attribute              | Value                                                          |
|------------------------|----------------------------------------------------------------|
| Component Name         | Linux Kernel                                                   |
| Version                | 5.15 LTS (Buildroot 2024.02 default for STM32MP1)             |
| License                | GPLv2                                                          |
| Classification         | Production                                                     |
| Source                  | https://kernel.org/ (via Buildroot)                            |
| Purpose                | Operating system kernel providing process management, device drivers (I2C, SPI, framebuffer, USB, WiFi/Ethernet), filesystem (ext4, squashfs), security (AppArmor, dm-verity), and systemd service management for all application processes. |
| Integration Points     | `buildroot-external/` (kernel configuration, device tree, systemd overlay) |
| Known Anomalies        | LTS kernels receive regular security patches. CVEs are monitored and patches applied via Buildroot configuration updates. No known unpatched anomalies affecting the STM32MP1 platform at time of this assessment. |
| Anomaly Monitoring     | Linux kernel security mailing list and CVE databases monitored. Buildroot version updates reviewed quarterly. OTA update mechanism enables field patching. |
| Risk Assessment        | **Low.** Linux 5.15 LTS is widely deployed in medical and industrial embedded devices. The GPLv2 license applies only to the kernel; application-layer code (MIT/public domain) is not affected. dm-verity ensures rootfs integrity. AppArmor confines application processes. Secure boot prevents unauthorized kernel modifications. |
| Verification           | System-level tests on target hardware verify kernel functionality (boot time, driver operation, network connectivity, watchdog behavior). Buildroot build reproducibility verified by configuration management. |

---

## 4. SOUP Management Process

1. **Identification:** All third-party components are identified and recorded in this document prior to integration.
2. **Evaluation:** Each SOUP item is evaluated for functional suitability, license compatibility, known anomalies, and risk level.
3. **Version control:** SOUP versions are locked in `buildroot-external/` configuration and `simulator/CMakeLists.txt`. Updates require re-evaluation.
4. **Anomaly monitoring:** Known anomaly databases (CVE, vendor advisories, GitHub issues) are reviewed at each project milestone and prior to each release.
5. **Update procedure:** SOUP updates follow the change control process defined in the Configuration Management Plan (CMP-001). Updates are regression tested prior to integration.
6. **Verification:** SOUP functionality is verified through the project's automated test suite (563 tests) and system-level testing on target hardware.

---

## Revision History

| Version | Date       | Author           | Changes                                          |
|---------|------------|------------------|--------------------------------------------------|
| 1.0     | 2026-02-15 | Engineering Team | Initial SOUP list with all identified components |
