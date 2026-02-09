# Requirements Traceability Matrix

| Field          | Value                                      |
|----------------|--------------------------------------------|
| Document ID    | RTM-001                                    |
| Version        | [TODO]                                     |
| Date           | [TODO]                                     |
| Author         | [TODO]                                     |
| Classification | IEC 62304 Software Safety Class B          |

---

## 1. Purpose

This matrix traces each software requirement from its source (PRD) through architecture, implementation, and verification. It satisfies IEC 62304 clause 5.7 (Software Verification) and ISO 14971 traceability expectations.

**Status key:** Not Started | In Progress | Verified

---

## 2. Traceability Table

| Req ID   | Requirement Description                         | Design Element             | Implementation File(s)                          | Test Case ID | Status      |
|----------|--------------------------------------------------|----------------------------|--------------------------------------------------|--------------|-------------|
| REQ-001  | Patient identification and association           | Patient Management module  | `src/core/patient_data.c`, `src/ui/screens/screen_patient.c` | [TODO]       | Not Started |
| REQ-002  | Vital signs display (HR, SpO2, NIBP, Temp, RR)  | UI App, Sensor Service     | `src/ui/screens/screen_main_vitals.c`, `src/ui/widgets/widget_numeric_display.c` | [TODO]       | In Progress |
| REQ-003  | Waveform rendering (ECG, Pleth)                  | UI App, Waveform Widget    | `src/ui/widgets/widget_waveform.c`, `src/core/waveform_gen.c` | [TODO]       | In Progress |
| REQ-004  | Alarm system (detection, indication, ack)        | Alarm Service, Alarm Engine| `src/core/alarm_engine.c`, `src/services/alarm_service.c`, `src/ui/widgets/widget_alarm_banner.c` | [TODO]       | Not Started |
| REQ-005  | Data storage and trend retrieval (72h)           | Trend DB, Trends Screen    | `src/core/trend_db.c`, `src/ui/screens/screen_trends.c` | [TODO]       | In Progress |
| REQ-006  | EHR/ABDM integration (FHIR R4, ABHA)            | Network Service            | `src/core/fhir_client.c`, `src/core/network_manager.c`, `src/services/network_service.c` | [TODO]       | Not Started |
| REQ-007  | Security (auth, RBAC, audit, encryption)         | Auth Manager, Audit Log    | `src/core/auth_manager.c`, `src/core/audit_log.c`, `src/ui/screens/screen_login.c` | [TODO]       | Not Started |
| REQ-008  | Regulatory compliance (IEC 62304, ISO 14971)     | All components             | Documentation + process                          | [TODO]       | Not Started |
| REQ-009  | Multi-process architecture (services, IPC)       | Service Manager, IPC       | `src/services/service_manager.c`, `src/common/ipc/ipc_messages.h` | [TODO]       | Not Started |
| REQ-010  | Development environment (simulator, build)       | Build system               | `simulator/CMakeLists.txt`, `simulator/main.c`   | [TODO]       | Verified    |

---

## 3. SRS-to-Test Traceability

| SRS Req ID | SRS Description                                 | Test Case ID | Test Type        | Status      |
|------------|--------------------------------------------------|--------------|------------------|-------------|
| SRS-F001   | Display continuous HR numeric                    | [TODO]       | System Test      | Not Started |
| SRS-F002   | Display continuous SpO2 numeric                  | [TODO]       | System Test      | Not Started |
| SRS-F003   | Display on-demand NIBP                           | [TODO]       | System Test      | Not Started |
| SRS-F006   | ECG waveform >= 30 FPS                           | [TODO]       | Performance Test | Not Started |
| SRS-F007   | Pleth waveform >= 30 FPS                         | [TODO]       | Performance Test | Not Started |
| SRS-F008   | Alarm threshold evaluation                       | [TODO]       | Unit Test        | Not Started |
| SRS-F010   | Alarm ack requires authentication                | [TODO]       | System Test      | Not Started |
| SRS-F012   | 72h on-device trend storage                      | [TODO]       | Integration Test | Not Started |
| SRS-F016   | User authentication (PIN/RFID)                   | [TODO]       | System Test      | Not Started |
| SRS-S001   | High alarm indicated within 10 sec               | [TODO]       | System Test      | Not Started |
| SRS-X001   | Data at rest encryption (LUKS)                   | [TODO]       | Inspection       | Not Started |
| [TODO]     | [Add rows as SRS requirements are finalized]     | [TODO]       | [TODO]           | Not Started |

---

## 4. Risk Control Traceability

| Hazard ID | Risk Control Measure                    | Implementing Req ID | Verification Status |
|-----------|-----------------------------------------|---------------------|---------------------|
| HAZ-001   | Display accuracy validation             | SRS-S005            | Not Started         |
| HAZ-002   | Alarm latency < 10 sec                  | SRS-S001            | Not Started         |
| HAZ-003   | Configurable alarm thresholds           | SRS-F008            | Not Started         |
| HAZ-004   | Data encryption at rest                 | SRS-X001            | Not Started         |
| HAZ-005   | Watchdog restart of crashed services    | SRS-S004            | Not Started         |
| HAZ-006   | Offline operation for 72h               | SRS-P008            | Not Started         |
| HAZ-007   | Patient identity confirmation workflow  | SRS-F011            | Not Started         |
| [TODO]    | [Add rows as risk analysis progresses]  | [TODO]              | Not Started         |

---

## Revision History

| Version | Date   | Author | Changes         |
|---------|--------|--------|-----------------|
| [TODO]  | [TODO] | [TODO] | Initial draft   |
