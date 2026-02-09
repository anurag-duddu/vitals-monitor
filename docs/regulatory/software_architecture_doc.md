# Software Architecture Document (SAD)

| Field          | Value                                      |
|----------------|--------------------------------------------|
| Document ID    | SAD-001                                    |
| Version        | [TODO]                                     |
| Date           | [TODO]                                     |
| Author         | [TODO]                                     |
| Reviewer       | [TODO]                                     |
| Approval       | [TODO]                                     |
| Classification | IEC 62304 Software Safety Class B          |

---

## 1. Purpose

This document describes the software architecture of the Bedside Vitals Monitor per IEC 62304 clause 5.3. It covers the decomposition into software items, interfaces, data flows, and design rationale.

---

## 2. System Context

```
                     +-----------+
                     |  Hospital |
                     |  Network  |
                     +-----+-----+
                           |  WiFi / Ethernet
                           |  (FHIR R4, ABDM)
+----------+         +----+----+         +---------+
| Bedside  |--I2C/-->| Vitals  |<--USB-->| Barcode |
| Sensors  |  SPI    | Monitor |         | Scanner |
+----------+         +----+----+         +---------+
                           |
                      +----+----+
                      | 800x480 |
                      | Touch   |
                      | Display |
                      +---------+
```

---

## 3. Component Diagram

```
+----------------------------------------------------------------+
|                        LVGL UI App (ui-app)                     |
|  src/ui/screens/    src/ui/widgets/    src/ui/themes/           |
|  screen_main_vitals  widget_waveform    theme_vitals            |
|  screen_trends       widget_numeric     phosphor_icons          |
|  screen_alarms       widget_alarm_banner                        |
|  screen_patient      widget_nav_bar                             |
|  screen_settings                                                |
|  screen_login                                                   |
|  screen_manager                                                 |
+-------+----------------------------+---------------------------+
        | nanomsg (pub/sub)          | D-Bus (control)
+-------+--------+    +-------------+------+    +---------------+
| Sensor Service  |    |  Alarm Service     |    | Audit Service |
| src/services/   |    |  src/services/     |    | src/core/     |
| sensor_service  |    |  alarm_service     |    | audit_log     |
| sensor_hal      |    |  alarm_engine      |    +---------------+
+-----------------+    +--------------------+
        |                                         +---------------+
        | I2C / SPI                               | Network Svc   |
+-------+--------+                                | src/services/ |
| Sensor Drivers  |                                | network_svc   |
| src/drivers/    |                                | fhir_client   |
| sensor_hal.h    |                                | sync_queue    |
+-----------------+                                +---------------+
```

---

## 4. Data Flow

```
Sensors --> sensor-service --[nanomsg pub]--> ui-app (display)
                           --[nanomsg pub]--> alarm-service (evaluate)
                           --[SQLite write]--> trend_db (vitals_raw)

alarm-service --[nanomsg pub]--> ui-app (alarm banner)
              --[SQLite write]--> trend_db (alarm_events)

ui-app --[SQLite read]--> trend_db (trend queries)

network-service --[SQLite read]--> sync_queue --> EHR (FHIR)
                                              --> ABDM
```

---

## 5. Technology Stack

| Layer            | Technology       | Version   | License   |
|------------------|------------------|-----------|-----------|
| UI Framework     | LVGL             | 9.3       | MIT       |
| Language         | C (C99)          | --        | --        |
| OS               | Buildroot Linux  | [TODO]    | GPL (OS)  |
| Database         | SQLite           | 3.x       | Public Domain |
| IPC              | nanomsg          | [TODO]    | MIT       |
| Control Bus      | D-Bus            | [TODO]    | AFL/GPL   |
| TLS              | mbedTLS          | [TODO]    | Apache 2  |
| Build (host)     | CMake + SDL2     | [TODO]    | BSD / zlib|
| Build (target)   | Buildroot        | [TODO]    | GPL       |

---

## 6. Database Schema Summary

| Table            | Purpose                              | Retention |
|------------------|--------------------------------------|-----------|
| `vitals_raw`     | 1-second vital sign samples          | 4 hours   |
| `vitals_1min`    | 1-minute aggregated samples          | 72 hours  |
| `nibp_measurements` | Discrete NIBP readings            | 72 hours  |
| `alarm_events`   | Alarm activation/acknowledgment log  | 72 hours  |
| `audit_log`      | Security and user action events      | [TODO]    |
| `patients`       | Patient demographics and association | [TODO]    |
| `users`          | Credentials and roles                | [TODO]    |

Schema implementation: `src/core/trend_db.c`, `src/core/audit_log.c`, `src/core/patient_data.c`

---

## 7. IPC Architecture

| Channel               | Transport      | Pattern  | Data                     |
|-----------------------|----------------|----------|--------------------------|
| `/tmp/vitals.ipc`     | nanomsg        | Pub/Sub  | vitals_data_t (1 Hz)     |
| `/tmp/waveform.ipc`   | nanomsg        | Pub/Sub  | Waveform samples (128 Hz)|
| `/tmp/alarms.ipc`     | nanomsg        | Pub/Sub  | Alarm state changes       |
| D-Bus system bus      | D-Bus          | RPC      | Config, control commands  |

Message definitions: `src/common/ipc/ipc_messages.h`

---

## 8. Security Architecture

| Layer              | Mechanism                                           |
|--------------------|-----------------------------------------------------|
| Boot               | Secure boot, signed bootloader/kernel               |
| Rootfs integrity   | dm-verity (read-only squashfs)                      |
| Data at rest       | LUKS encrypted data partition                       |
| Data in transit    | TLS 1.2+ (mbedTLS)                                 |
| Authentication     | PIN (hashed with Argon2) + RFID badge               |
| Authorization      | Role-based access (Clinical, Biomed, Admin, Service)|
| Audit              | Append-only audit log with integrity checksums       |
| Update             | Signed SWUpdate packages, A/B partition scheme       |

Implementation: `src/core/auth_manager.c`, `src/core/audit_log.c`

---

## 9. Thread / Process Model

| Process            | Criticality | Restart Policy       | Watchdog |
|--------------------|-------------|----------------------|----------|
| sensor-service     | Critical    | Always restart       | Yes      |
| alarm-service      | Critical    | Always restart       | Yes      |
| ui-app             | High        | Always restart       | Yes      |
| network-service    | Medium      | Restart with backoff | No       |
| audit-service      | High        | Always restart       | Yes      |
| watchdog-service   | Critical    | Kernel-level         | N/A      |

All processes managed by systemd. The LVGL event loop runs single-threaded within ui-app; IPC data is received on a background thread and dispatched to the UI thread via a message queue (`src/core/vitals_provider.h` abstraction).

---

## 10. Error Handling Strategy

| Error Category        | Strategy                                              |
|-----------------------|-------------------------------------------------------|
| Sensor read failure   | Retry 3x, then indicate "sensor disconnected" alarm   |
| IPC timeout           | UI shows stale-data indicator after [TODO] seconds     |
| Database write fail   | Retry with exponential backoff; log to stderr          |
| Network failure       | Queue data locally (offline-first); retry on reconnect |
| UI crash              | Watchdog restarts ui-app; alarm-service continues independently |
| Critical assertion    | Log, restart process, increment failure counter        |

---

## 11. Coding Standards

- Language: C99
- Style: [TODO -- define or reference coding standard document]
- No dynamic memory allocation in critical real-time paths
- All user-facing strings externalized for localization readiness
- No GPL-licensed code in application layer
- Timestamps stored as UTC; converted for display only

---

## Revision History

| Version | Date   | Author | Changes         |
|---------|--------|--------|-----------------|
| [TODO]  | [TODO] | [TODO] | Initial draft   |
