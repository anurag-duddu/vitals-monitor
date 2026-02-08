# Architecture Requirements: Bedside Vitals Monitor

## Document Information

| Field | Value |
|-------|-------|
| Document ID | REQ-009 |
| Version | 1.0 |
| Status | Draft |
| Parent Document | REQ-001 Product Overview |

---

## 1. Overview

This document specifies the software architecture requirements for the bedside vitals monitor. The architecture is designed for a production medical device running on embedded Linux with LVGL as the UI framework.

### 1.1 Architecture Goals

| Goal | Description |
|------|-------------|
| **Reliability** | System remains operational 24/7 with graceful degradation |
| **Security** | Tamper-resistant with defense in depth |
| **Maintainability** | Clear separation of concerns, modular design |
| **Testability** | Components independently testable |
| **Performance** | Real-time waveform display, responsive UI |
| **Updatability** | Safe, atomic updates with rollback |

### 1.2 Technology Stack Summary

| Layer | Technology | Rationale |
|-------|------------|-----------|
| Hardware | ARM Cortex-A (Snapdragon-class) | Sufficient for Linux + UI |
| OS | Embedded Linux (Buildroot) | Open source, medical device precedent |
| UI Framework | LVGL | MIT license, embedded-optimized, minimal footprint |
| Language (Core) | C or Rust | Performance, embedded suitability |
| Language (Services) | C/Rust or Python (non-critical) | Flexibility where appropriate |
| IPC | D-Bus or nanomsg | Standard Linux IPC |
| Data Storage | SQLite | Reliable, embedded-friendly |
| Security | dm-verity, LUKS, secure boot | Industry standard |

---

## 2. System Architecture

### 2.1 High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         PRESENTATION LAYER                          │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │                      LVGL UI Application                       │  │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────────┐  │  │
│  │  │ Vitals   │ │ Waveform │ │ Alarms   │ │ Settings/Admin   │  │  │
│  │  │ Display  │ │ Renderer │ │ Manager  │ │ Screens          │  │  │
│  │  └──────────┘ └──────────┘ └──────────┘ └──────────────────┘  │  │
│  └───────────────────────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────────────────────┤
│                         APPLICATION LAYER                           │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌──────────────┐  │
│  │   Vital     │ │   Alarm     │ │   Patient   │ │    Trend     │  │
│  │  Processor  │ │   Engine    │ │   Manager   │ │   Storage    │  │
│  └─────────────┘ └─────────────┘ └─────────────┘ └──────────────┘  │
├─────────────────────────────────────────────────────────────────────┤
│                          SERVICE LAYER                              │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌──────────────┐  │
│  │   Sensor    │ │   Network   │ │   Audit     │ │   Watchdog   │  │
│  │   Service   │ │   Service   │ │   Service   │ │   Service    │  │
│  └─────────────┘ └─────────────┘ └─────────────┘ └──────────────┘  │
├─────────────────────────────────────────────────────────────────────┤
│                          PLATFORM LAYER                             │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌──────────────┐  │
│  │   Sensor    │ │   Display   │ │   Storage   │ │   Network    │  │
│  │   Drivers   │ │   Driver    │ │   (SQLite)  │ │   Stack      │  │
│  └─────────────┘ └─────────────┘ └─────────────┘ └──────────────┘  │
├─────────────────────────────────────────────────────────────────────┤
│                         OPERATING SYSTEM                            │
│                    Linux Kernel (Buildroot image)                   │
├─────────────────────────────────────────────────────────────────────┤
│                            HARDWARE                                 │
│              ARM SoC │ Sensors │ Display │ Network                  │
└─────────────────────────────────────────────────────────────────────┘
```

### 2.2 Architecture Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| ARCH-001 | Layered architecture with clear separation | Must |
| ARCH-002 | Services communicate via defined IPC | Must |
| ARCH-003 | Sensor reading independent of UI | Must |
| ARCH-004 | UI crash does not stop sensor service | Must |
| ARCH-005 | Each service restartable independently | Must |
| ARCH-006 | Stateless components where possible | Should |
| ARCH-007 | Configuration externalized from code | Must |

---

## 3. Process Architecture

### 3.1 Process Model

The system runs as multiple independent processes for isolation and reliability:

| Process | Function | Criticality | Restart Policy |
|---------|----------|-------------|----------------|
| `sensor-service` | Read sensors, publish data | Critical | Auto-restart, max 5 attempts |
| `alarm-service` | Evaluate alarms, generate alerts | Critical | Auto-restart, max 5 attempts |
| `ui-app` | Display and user interaction | High | Auto-restart, unlimited |
| `network-service` | EHR/ABDM sync, connectivity | Medium | Auto-restart, unlimited |
| `audit-service` | Log events, manage audit trail | High | Auto-restart, unlimited |
| `watchdog-service` | Monitor health, trigger recovery | Critical | Managed by init |

### 3.2 Process Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| PROC-001 | Use systemd or init for process management | Must |
| PROC-002 | Define service dependencies | Must |
| PROC-003 | Automatic restart on crash | Must |
| PROC-004 | Configurable restart limits | Must |
| PROC-005 | Resource limits per process (memory, CPU) | Should |
| PROC-006 | Process isolation via namespaces or cgroups | Should |
| PROC-007 | Non-root execution for all application processes | Must |

### 3.3 Process Startup Order

```
1. kernel + init
2. watchdog-service (earliest, monitors system)
3. audit-service (logging available for other services)
4. sensor-service (begin data acquisition)
5. alarm-service (after sensors ready)
6. ui-app (display ready)
7. network-service (last, non-critical for monitoring)
```

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| PROC-START-001 | Defined startup order | Must |
| PROC-START-002 | Services wait for dependencies | Must |
| PROC-START-003 | Boot to UI in < 30 seconds | Must |
| PROC-START-004 | Sensor data available in < 15 seconds | Must |

---

## 4. Inter-Process Communication

### 4.1 IPC Mechanism Selection

| Option | Pros | Cons | Recommendation |
|--------|------|------|----------------|
| D-Bus | Standard Linux, well-supported | Heavier, XML overhead | For configuration/control |
| nanomsg/nng | Lightweight, pub/sub native | Less tooling | For real-time data |
| Unix sockets | Simple, low overhead | Manual protocol | For simple cases |
| Shared memory | Fastest | Complex synchronization | For waveform buffers |

**Recommended Approach**: 
- **nanomsg** for high-frequency sensor data (pub/sub pattern)
- **D-Bus** for control messages, configuration, RPC-style calls
- **Shared memory** for waveform ring buffers (optional optimization)

### 4.2 IPC Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| IPC-001 | Define message schemas | Must |
| IPC-002 | Use binary serialization for performance (protobuf, msgpack) | Should |
| IPC-003 | Pub/sub for sensor data distribution | Must |
| IPC-004 | Request/reply for control messages | Must |
| IPC-005 | Message versioning for compatibility | Must |
| IPC-006 | Timeout handling on all IPC calls | Must |
| IPC-007 | Local IPC only (no network IPC) | Must |

### 4.3 Message Flow

```
┌─────────────┐     vitals/raw      ┌─────────────┐
│   Sensor    │ ──────────────────► │    Alarm    │
│   Service   │                     │   Service   │
└──────┬──────┘                     └──────┬──────┘
       │                                    │
       │ vitals/processed                   │ alarms/active
       │                                    │
       ▼                                    ▼
┌─────────────────────────────────────────────────┐
│                    UI App                        │
│   (subscribes to vitals, alarms, patient data)   │
└─────────────────────────────────────────────────┘
       │
       │ export/queue
       ▼
┌─────────────┐
│   Network   │
│   Service   │
└─────────────┘
```

### 4.4 Message Definitions

#### 4.4.1 Vitals Message

| Field | Type | Description |
|-------|------|-------------|
| timestamp | uint64 | Milliseconds since epoch |
| patient_id | string | Associated patient (empty if none) |
| heart_rate | float32 | BPM |
| spo2 | float32 | Percentage |
| resp_rate | float32 | Breaths/min |
| temperature | float32 | Celsius |
| nibp_systolic | float32 | mmHg |
| nibp_diastolic | float32 | mmHg |
| nibp_mean | float32 | mmHg |
| nibp_timestamp | uint64 | Last NIBP measurement time |

#### 4.4.2 Waveform Message

| Field | Type | Description |
|-------|------|-------------|
| timestamp | uint64 | Milliseconds since epoch |
| type | enum | ECG, PLETH |
| sample_rate | uint16 | Samples per second |
| samples | float32[] | Waveform samples |

#### 4.4.3 Alarm Message

| Field | Type | Description |
|-------|------|-------------|
| alarm_id | string | Unique identifier |
| timestamp | uint64 | Trigger time |
| patient_id | string | Associated patient |
| parameter | string | HR, SPO2, etc. |
| priority | enum | HIGH, MEDIUM, LOW |
| value | float32 | Current value |
| limit | float32 | Threshold crossed |
| limit_type | enum | HIGH, LOW |
| acknowledged | bool | Ack status |
| ack_user | string | Who acknowledged |
| ack_time | uint64 | When acknowledged |

---

## 5. Data Architecture

### 5.1 Data Stores

| Store | Technology | Purpose | Persistence |
|-------|------------|---------|-------------|
| Configuration | SQLite or JSON files | Device and user settings | Permanent |
| Patient Data | SQLite | Active patient associations | Session |
| Trend Data | SQLite | Historical vital signs | 72 hours |
| Waveform Buffer | Ring buffer (memory) | Real-time waveform | None (volatile) |
| Audit Log | SQLite + log files | Security events | 90 days minimum |
| Export Queue | SQLite | Pending EHR exports | Until synced |

### 5.2 Data Architecture Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| DATA-001 | SQLite for structured data | Must |
| DATA-002 | Separate databases per concern | Should |
| DATA-003 | Database encryption at rest | Must |
| DATA-004 | Automatic database backup before updates | Must |
| DATA-005 | Database schema versioning and migration | Must |
| DATA-006 | WAL mode for concurrent access | Should |
| DATA-007 | Regular database integrity checks | Should |

### 5.3 Partition Layout

```
┌──────────────────────────────────────────────────────────────┐
│                        eMMC/SD Card                          │
├───────────┬───────────┬──────────────┬───────────────────────┤
│   boot    │  rootfs_a │   rootfs_b   │        data           │
│   (FAT)   │ (squashfs)│  (squashfs)  │       (ext4)          │
│   50MB    │   500MB   │    500MB     │       rest            │
├───────────┴───────────┴──────────────┴───────────────────────┤
│                                                              │
│  boot: U-Boot, kernel, device tree                           │
│  rootfs_a/b: Read-only root (A/B for updates)                │
│  data: Encrypted partition for all writable data             │
│                                                              │
└──────────────────────────────────────────────────────────────┘
```

### 5.4 Data Partition Structure

```
/data/
├── config/
│   ├── device.db        # Device configuration
│   ├── users.db         # User accounts (encrypted)
│   └── network.conf     # Network settings
├── patient/
│   ├── sessions.db      # Patient associations
│   └── trends.db        # Trend storage
├── audit/
│   ├── audit.db         # Audit events
│   └── logs/            # Log files
├── export/
│   └── queue.db         # Export queue
└── keys/
    └── (managed by secure element)
```

---

## 6. Sensor Service Architecture

### 6.1 Sensor Service Responsibilities

- Initialize and manage sensor hardware
- Read sensor data at appropriate rates
- Perform signal conditioning and filtering
- Publish processed data to other services
- Handle sensor errors and disconnections

### 6.2 Sensor Interface Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| SENS-001 | Abstract sensor interface for swappable implementations | Must |
| SENS-002 | Support I2C sensor communication | Must |
| SENS-003 | Support SPI sensor communication | Must |
| SENS-004 | Configurable sample rates | Must |
| SENS-005 | Sensor hot-plug detection | Should |
| SENS-006 | Sensor self-test on startup | Must |
| SENS-007 | Sensor error recovery | Must |

### 6.3 Sensor Timing

| Sensor | Sample Rate | Latency Budget |
|--------|-------------|----------------|
| SpO2/Pleth | 100 Hz | < 10ms |
| ECG | 250-500 Hz | < 10ms |
| Temperature | 0.5 Hz | < 100ms |
| NIBP | On-demand | < 60s cycle |

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| SENS-TIME-001 | ECG sampling jitter < 1ms | Must |
| SENS-TIME-002 | Vital derivation within 1 second of samples | Must |
| SENS-TIME-003 | Use high-priority thread for sampling | Must |
| SENS-TIME-004 | Consider RT-PREEMPT kernel if timing insufficient | Should |

---

## 7. UI Application Architecture

### 7.1 LVGL Architecture

```
┌───────────────────────────────────────────────────────────────┐
│                      LVGL Application                          │
├───────────────────────────────────────────────────────────────┤
│  ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌────────────┐  │
│  │   Screen   │ │   Screen   │ │   Screen   │ │   Screen   │  │
│  │   Manager  │ │   Stack    │ │  Factory   │ │   Loader   │  │
│  └────────────┘ └────────────┘ └────────────┘ └────────────┘  │
├───────────────────────────────────────────────────────────────┤
│  ┌────────────────────────────────────────────────────────┐   │
│  │                    Screens                              │   │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌───────────┐  │   │
│  │  │  Main    │ │  Trends  │ │ Settings │ │  Patient  │  │   │
│  │  │  Vitals  │ │  Screen  │ │  Screen  │ │  Screen   │  │   │
│  │  └──────────┘ └──────────┘ └──────────┘ └───────────┘  │   │
│  └────────────────────────────────────────────────────────┘   │
├───────────────────────────────────────────────────────────────┤
│  ┌────────────────────────────────────────────────────────┐   │
│  │                    Widgets                              │   │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌───────────┐  │   │
│  │  │ Waveform │ │ Numeric  │ │  Alarm   │ │ Keyboard  │  │   │
│  │  │ Widget   │ │ Display  │ │  Banner  │ │  Widget   │  │   │
│  │  └──────────┘ └──────────┘ └──────────┘ └───────────┘  │   │
│  └────────────────────────────────────────────────────────┘   │
├───────────────────────────────────────────────────────────────┤
│  ┌────────────────────────────────────────────────────────┐   │
│  │                 Data Bindings                           │   │
│  │         (Subscribe to IPC, update widgets)              │   │
│  └────────────────────────────────────────────────────────┘   │
├───────────────────────────────────────────────────────────────┤
│                      LVGL Core                                │
│           (Event loop, rendering, input handling)             │
└───────────────────────────────────────────────────────────────┘
```

### 7.2 UI Architecture Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| UI-ARCH-001 | Single main thread for LVGL | Must |
| UI-ARCH-002 | IPC handling in separate thread | Must |
| UI-ARCH-003 | Thread-safe UI update mechanism | Must |
| UI-ARCH-004 | Screen navigation stack | Must |
| UI-ARCH-005 | Reusable widget components | Must |
| UI-ARCH-006 | Theme/style separation from layout | Should |
| UI-ARCH-007 | Frame rate target: 30 FPS minimum | Must |
| UI-ARCH-008 | Input debouncing for touch | Must |

### 7.3 Display Driver Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| UI-DRV-001 | Use Linux framebuffer or DRM/KMS | Must |
| UI-DRV-002 | Support hardware acceleration if available | Should |
| UI-DRV-003 | Double buffering to prevent tearing | Must |
| UI-DRV-004 | Touch input via evdev | Must |
| UI-DRV-005 | Screen rotation support | Should |
| UI-DRV-006 | Brightness control | Should |

---

## 8. Network Service Architecture

### 8.1 Network Service Responsibilities

- Manage network connectivity (WiFi, Ethernet)
- Handle EHR integration (FHIR client)
- Handle ABDM integration
- Manage export queue
- Provide network status to UI

### 8.2 Network Architecture Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| NET-ARCH-001 | Async I/O for network operations | Must |
| NET-ARCH-002 | Connection pooling for efficiency | Should |
| NET-ARCH-003 | Automatic retry with backoff | Must |
| NET-ARCH-004 | Offline queue persistence | Must |
| NET-ARCH-005 | Network status change events | Must |
| NET-ARCH-006 | Certificate management | Must |

### 8.3 Integration Clients

| Client | Protocol | Library |
|--------|----------|---------|
| FHIR Client | HTTPS + JSON | libcurl + cJSON |
| ABDM Client | HTTPS + JSON | libcurl + cJSON |
| NTP Client | UDP | ntpd or systemd-timesyncd |

---

## 9. Build System

### 9.1 Buildroot Selection

Buildroot is selected over Yocto for the following reasons:

| Factor | Buildroot | Yocto |
|--------|-----------|-------|
| Complexity | Lower | Higher |
| Build time | Faster | Slower |
| Learning curve | Gentler | Steeper |
| Flexibility | Sufficient | More flexible |
| Package count | Sufficient | More |
| Documentation | Good | Extensive |

For this project's scope, Buildroot provides sufficient capability with lower overhead.

### 9.2 Build System Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| BUILD-001 | Use Buildroot for image generation | Must |
| BUILD-002 | Reproducible builds | Must |
| BUILD-003 | Version-controlled configuration | Must |
| BUILD-004 | Custom package for application | Must |
| BUILD-005 | Cross-compilation toolchain | Must |
| BUILD-006 | Debug and release build variants | Must |
| BUILD-007 | Build time < 30 minutes (incremental) | Should |
| BUILD-008 | CI/CD integration | Must |

### 9.3 Build Outputs

| Output | Description | Use |
|--------|-------------|-----|
| `rootfs.squashfs` | Read-only root filesystem | A/B slots |
| `zImage` / `Image` | Linux kernel | Boot |
| `*.dtb` | Device tree blob | Boot |
| `boot.scr` | U-Boot script | Boot |
| `update.swu` | Signed update package | OTA/USB update |

### 9.4 Build Directory Structure

```
project/
├── buildroot/
│   └── external/           # Custom external tree
│       ├── board/
│       │   └── vitals-monitor/
│       │       ├── overlay/      # Filesystem overlay
│       │       ├── post_build.sh
│       │       └── post_image.sh
│       ├── configs/
│       │   └── vitals_monitor_defconfig
│       └── package/
│           ├── sensor-service/
│           ├── alarm-service/
│           ├── ui-app/
│           ├── network-service/
│           ├── audit-service/
│           └── watchdog-service/
├── src/
│   ├── sensor-service/
│   ├── alarm-service/
│   ├── ui-app/
│   ├── network-service/
│   ├── audit-service/
│   ├── watchdog-service/
│   └── common/
├── tests/
├── docs/
└── tools/
```

---

## 10. Update System

### 10.1 Update Mechanism

The device uses A/B partition scheme with SWUpdate:

```
┌─────────────────────────────────────────────────────────────┐
│                     Update Process                          │
│                                                             │
│  1. Download/receive update package (.swu)                  │
│  2. Verify signature                                        │
│  3. Write to inactive partition (A→B or B→A)                │
│  4. Set boot flag to new partition                          │
│  5. Reboot                                                  │
│  6. Boot attempts new partition                             │
│  7. If success, mark as good                                │
│  8. If failure (3 attempts), rollback to previous           │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 10.2 Update Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| UPD-ARCH-001 | A/B partition scheme | Must |
| UPD-ARCH-002 | Use SWUpdate or RAUC | Must |
| UPD-ARCH-003 | Signed update packages (RSA or ECDSA) | Must |
| UPD-ARCH-004 | Version validation | Must |
| UPD-ARCH-005 | Automatic rollback on boot failure | Must |
| UPD-ARCH-006 | Update via USB storage | Must |
| UPD-ARCH-007 | Update via network (OTA) | Should |
| UPD-ARCH-008 | Monitoring continues during update | Must |
| UPD-ARCH-009 | Data partition preserved across updates | Must |
| UPD-ARCH-010 | Configuration migration if schema changes | Must |

### 10.3 Update Package Contents

```
update.swu
├── sw-description        # Metadata, version, checksums
├── sw-description.sig    # Signature
├── rootfs.squashfs       # New root filesystem
├── zImage                # New kernel (if changed)
├── *.dtb                 # New device tree (if changed)
└── scripts/
    ├── pre_install.sh
    └── post_install.sh
```

---

## 11. Security Architecture

### 11.1 Security Layers

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Security                      │
│         (Authentication, Authorization, Audit)               │
├─────────────────────────────────────────────────────────────┤
│                    Runtime Security                          │
│         (SELinux/AppArmor, sandboxing, ASLR)                 │
├─────────────────────────────────────────────────────────────┤
│                    Filesystem Security                       │
│         (dm-verity, read-only root, encrypted data)          │
├─────────────────────────────────────────────────────────────┤
│                    Boot Security                             │
│         (Secure boot, signed kernel, verified boot)          │
├─────────────────────────────────────────────────────────────┤
│                    Hardware Security                         │
│         (Secure element, fuses, disabled JTAG)               │
└─────────────────────────────────────────────────────────────┘
```

### 11.2 Security Architecture Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| SEC-ARCH-001 | Secure boot chain from ROM | Must |
| SEC-ARCH-002 | dm-verity for rootfs integrity | Must |
| SEC-ARCH-003 | LUKS encryption for data partition | Must |
| SEC-ARCH-004 | SELinux or AppArmor enabled | Must |
| SEC-ARCH-005 | No root login | Must |
| SEC-ARCH-006 | Disable JTAG/debug in production | Must |
| SEC-ARCH-007 | Secure key storage (TEE preferred) | Should |

---

## 12. Logging Architecture

### 12.1 Log Categories

| Category | Purpose | Retention |
|----------|---------|-----------|
| Audit | Security events | 90+ days |
| Application | Errors, warnings, info | 7 days |
| Sensor | Sensor diagnostics | 24 hours |
| Network | Connectivity events | 7 days |
| System | Kernel, services | 7 days |

### 12.2 Logging Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| LOG-001 | Structured logging (JSON preferred) | Must |
| LOG-002 | Log rotation with size limits | Must |
| LOG-003 | Audit logs tamper-evident | Must |
| LOG-004 | Timestamps with timezone | Must |
| LOG-005 | Log level configurable | Must |
| LOG-006 | Export logs to external system | Should |
| LOG-007 | No sensitive data in logs (PHI, credentials) | Must |

---

## 13. Testing Architecture

### 13.1 Test Levels

| Level | Scope | Tools |
|-------|-------|-------|
| Unit | Individual functions | Unity, CTest, criterion |
| Integration | Component interaction | Custom harness |
| System | Full device | Automated test rig |
| Hardware-in-Loop | With real sensors | Lab setup |

### 13.2 Testing Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| TEST-001 | Unit test coverage > 80% for critical components | Must |
| TEST-002 | Automated integration tests | Must |
| TEST-003 | CI pipeline runs tests on every commit | Must |
| TEST-004 | Simulated sensor mode for UI testing | Must |
| TEST-005 | Fault injection testing | Should |
| TEST-006 | Performance benchmarks | Should |

---

## 14. Development Environment

### 14.1 Development Tools

| Tool | Purpose |
|------|---------|
| Docker | Consistent build environment |
| GDB + gdbserver | Remote debugging |
| Valgrind | Memory analysis |
| strace/ltrace | System call tracing |
| perf | Performance profiling |
| QEMU | Emulated testing (limited) |

### 14.2 Development Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| DEV-001 | Containerized build environment | Must |
| DEV-002 | Remote debugging capability | Must |
| DEV-003 | Simulation mode for UI development | Must |
| DEV-004 | Hot reload for UI during development | Should |
| DEV-005 | Documentation generation (Doxygen) | Should |

---

## 15. Performance Requirements

### 15.1 Performance Targets

| Metric | Target |
|--------|--------|
| Boot to UI | < 30 seconds |
| Boot to monitoring | < 15 seconds |
| UI frame rate | ≥ 30 FPS |
| Touch response | < 100ms |
| Vital update latency | < 1 second |
| Waveform latency | < 100ms |
| Memory usage (total) | < 256 MB |
| CPU usage (idle) | < 20% |
| CPU usage (active) | < 60% |

### 15.2 Performance Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| PERF-001 | Profile and optimize boot sequence | Must |
| PERF-002 | Lazy loading for non-critical components | Should |
| PERF-003 | Memory budget per component | Should |
| PERF-004 | Frame rate monitoring in debug builds | Should |
| PERF-005 | Waveform rendering optimized for target | Must |

---

## 16. SOUP (Software of Unknown Provenance)

### 16.1 SOUP Inventory

| Component | Version | License | Function | Risk Level |
|-----------|---------|---------|----------|------------|
| Linux Kernel | 5.x/6.x | GPL-2.0 | Operating system | High |
| Buildroot | 2024.x | GPL-2.0+ | Build system | Medium |
| LVGL | 8.x/9.x | MIT | UI framework | High |
| SQLite | 3.x | Public Domain | Database | Medium |
| mbedTLS | 3.x | Apache-2.0 | Cryptography | High |
| nanomsg/nng | 1.x | MIT | IPC | Medium |
| libcurl | 8.x | curl license | HTTP client | Medium |
| cJSON | 1.x | MIT | JSON parsing | Low |
| SWUpdate | 2024.x | GPL-2.0 | Updates | Medium |

### 16.2 SOUP Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| SOUP-001 | Document all SOUP with versions | Must |
| SOUP-002 | Risk assessment for each SOUP | Must |
| SOUP-003 | Monitor SOUP for security advisories | Must |
| SOUP-004 | Process for SOUP updates | Must |
| SOUP-005 | License compliance verification | Must |
| SOUP-006 | No GPL in application code (use LGPL/MIT) | Must |

---

## 17. Traceability

### 17.1 Traceability Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| TRACE-001 | Trace architecture to system requirements | Must |
| TRACE-002 | Trace components to architecture | Must |
| TRACE-003 | Trace tests to requirements | Must |
| TRACE-004 | Trace risks to controls | Must |
| TRACE-005 | Maintain traceability matrix | Must |

---

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2024 | — | Initial draft |
