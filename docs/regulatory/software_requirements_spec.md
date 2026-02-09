# Software Requirements Specification (SRS)

| Field          | Value                                      |
|----------------|--------------------------------------------|
| Document ID    | SRS-001                                    |
| Version        | [TODO]                                     |
| Date           | [TODO]                                     |
| Author         | [TODO]                                     |
| Reviewer       | [TODO]                                     |
| Approval       | [TODO]                                     |
| Classification | IEC 62304 Software Safety Class B          |

---

## 1. Purpose and Scope

This document defines the software requirements for the Bedside Vitals Monitor, a CDSCO Class B medical device targeting Indian hospital general wards. It covers all software functions running on the STM32MP157F-DK2 platform under Buildroot Linux with an LVGL-based user interface.

This SRS satisfies IEC 62304 clause 5.2 (Software Requirements Analysis) and provides the basis for architecture design, implementation, and verification activities.

**Referenced documents:** `files/PRD.md`, `files/ROADMAP.md`, `files/CLAUDE.md`

---

## 2. System Overview

The system consists of an embedded Linux device that continuously acquires vital signs from bedside sensors, displays real-time waveforms and numerics on a touchscreen, evaluates alarm conditions, stores trend data locally, and synchronizes with hospital EHR/ABDM infrastructure when network is available.

**Process architecture:** sensor-service, alarm-service, ui-app, network-service, audit-service, watchdog-service (see `src/services/`).

**IPC:** nanomsg pub/sub for high-frequency vitals data; D-Bus for control messages.

---

## 3. Functional Requirements

| Req ID   | Description                                                                 | Priority | Verification Method | PRD Ref  |
|----------|-----------------------------------------------------------------------------|----------|---------------------|----------|
| SRS-F001 | Display continuous HR numeric derived from ECG/SpO2 source                  | High     | System Test         | REQ-002  |
| SRS-F002 | Display continuous SpO2 numeric from pulse oximeter                         | High     | System Test         | REQ-002  |
| SRS-F003 | Display on-demand NIBP (systolic, diastolic, MAP)                           | High     | System Test         | REQ-002  |
| SRS-F004 | Display temperature (spot or continuous)                                    | Medium   | System Test         | REQ-002  |
| SRS-F005 | Display derived respiration rate                                            | Medium   | System Test         | REQ-002  |
| SRS-F006 | Render real-time ECG waveform at >= 30 FPS                                  | High     | Performance Test    | REQ-003  |
| SRS-F007 | Render real-time plethysmograph waveform at >= 30 FPS                       | High     | Performance Test    | REQ-003  |
| SRS-F008 | Evaluate alarm conditions (high, medium, low priority) per configurable thresholds | High | Unit / System Test | REQ-004  |
| SRS-F009 | Generate visual and audible alarm indicators matching IEC 60601-1-8         | High     | System Test         | REQ-004  |
| SRS-F010 | Provide alarm acknowledgment requiring authenticated user action            | High     | System Test         | REQ-004  |
| SRS-F011 | Associate / disassociate patient identity with monitored bed                | High     | System Test         | REQ-001  |
| SRS-F012 | Store vital sign trends for minimum 72 hours on-device (SQLite)             | High     | Integration Test    | REQ-005  |
| SRS-F013 | Display historical trends with selectable time windows (1h-72h)             | Medium   | System Test         | REQ-005  |
| SRS-F014 | Export vitals and alarms to EHR via HL7 FHIR R4                             | Medium   | Integration Test    | REQ-006  |
| SRS-F015 | Support ABDM ABHA ID lookup and health record push                          | Medium   | Integration Test    | REQ-006  |
| SRS-F016 | Authenticate users via PIN and/or RFID badge                                | High     | System Test         | REQ-007  |
| SRS-F017 | Enforce role-based access control (Clinical, Biomedical, Admin, Service)     | High     | System Test         | REQ-007  |
| SRS-F018 | Log all security-relevant events to tamper-evident audit trail              | High     | Inspection          | REQ-007  |
| [TODO]   | [Add additional requirements as design progresses]                          | [TODO]   | [TODO]              | [TODO]   |

---

## 4. Performance Requirements

| Req ID   | Description                                              | Target           |
|----------|----------------------------------------------------------|------------------|
| SRS-P001 | UI frame rate during waveform rendering                  | >= 30 FPS        |
| SRS-P002 | Time from power-on to first vital sign display           | < 30 seconds     |
| SRS-P003 | Alarm condition detection to visual/audio indicator      | < 10 seconds     |
| SRS-P004 | Waveform sample-to-display latency                       | [TODO] ms        |
| SRS-P005 | Touchscreen input response time                          | < 200 ms         |
| SRS-P006 | Database query time for 72h trend retrieval              | < 2 seconds      |
| SRS-P007 | System uptime                                            | > 99.9%          |
| SRS-P008 | Offline continuous operation without data loss            | >= 72 hours      |

---

## 5. Safety Requirements

| Req ID   | Description                                                                  |
|----------|------------------------------------------------------------------------------|
| SRS-S001 | High-priority alarms shall be visually and audibly indicated within 10 sec   |
| SRS-S002 | Alarm silence shall auto-expire after a configurable maximum duration        |
| SRS-S003 | Sensor disconnection shall be detected and indicated within [TODO] seconds   |
| SRS-S004 | Watchdog shall restart any crashed critical service within [TODO] seconds    |
| SRS-S005 | Displayed vital sign values shall match sensor readings within specified accuracy tolerances (IEC 60601-2-49) |
| SRS-S006 | Data corruption during storage or retrieval shall be detectable via integrity checks |
| [TODO]   | [Add safety requirements identified during risk analysis]                    |

---

## 6. Security Requirements

| Req ID   | Description                                                        |
|----------|--------------------------------------------------------------------|
| SRS-X001 | Data at rest encrypted via LUKS partition encryption               |
| SRS-X002 | Data in transit encrypted via TLS 1.2+                             |
| SRS-X003 | User PINs stored as salted hashes (Argon2 or bcrypt)              |
| SRS-X004 | Session auto-lock after configurable inactivity timeout            |
| SRS-X005 | Secure boot chain with dm-verity for rootfs integrity             |
| SRS-X006 | OTA update packages cryptographically signed                       |
| SRS-X007 | Audit log entries include timestamp, user ID, action, and outcome  |
| [TODO]   | [Add security requirements per threat modeling results]            |

---

## 7. Regulatory Requirements

| Req ID   | Description                                                        |
|----------|--------------------------------------------------------------------|
| SRS-R001 | Software developed per IEC 62304 Software Safety Class B           |
| SRS-R002 | Risk management per ISO 14971                                      |
| SRS-R003 | Alarm behavior per IEC 60601-1-8                                   |
| SRS-R004 | Device classification: CDSCO Class B                               |
| SRS-R005 | Usability engineering per IEC 62366-1                              |
| SRS-R006 | Quality management per ISO 13485                                   |
| [TODO]   | [Add ABDM/NHA certification requirements]                         |

---

## 8. Traceability

Each requirement in this document shall trace to:
- **Upstream:** PRD sections (REQ-001 through REQ-010) in `files/PRD.md`
- **Downstream:** Software Architecture Document, implementation source files, and test cases

See `docs/regulatory/traceability_matrix.md` for the complete mapping.

---

## Revision History

| Version | Date   | Author | Changes         |
|---------|--------|--------|-----------------|
| [TODO]  | [TODO] | [TODO] | Initial draft   |
