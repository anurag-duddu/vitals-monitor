# Software Development Plan

| Field          | Value                                      |
|----------------|--------------------------------------------|
| Document ID    | SDP-001                                    |
| Version        | 1.0                                        |
| Date           | 2026-02-15                                 |
| Author         | Engineering Team                           |
| Reviewer       | Quality Assurance                          |
| Approval       | Regulatory Affairs                         |
| Classification | IEC 62304 Software Safety Class B          |

---

## 1. Purpose and Scope

This document defines the software development lifecycle, activities, deliverables, and tools for the Bedside Vitals Monitor, per IEC 62304 clause 5.1 (Software Development Planning). It applies to all software running on the production device and the host simulator used for development and verification.

---

## 2. Software Safety Classification

The Bedside Vitals Monitor software is classified as **IEC 62304 Software Safety Class B** based on the following rationale:

- The device is a non-life-sustaining monitoring device classified as **CDSCO Class B**.
- Software failure can contribute to a hazardous situation (e.g., missed alarm, incorrect vital sign display) but the probability of resulting harm can be reduced by external risk control measures (clinical workflows, central monitoring, nurse rounds).
- The system is not the sole source of alarm or diagnostic information.

---

## 3. Development Lifecycle Model

The project follows an **iterative incremental** development lifecycle with phased milestones:

| Phase   | Description                                        | IEC 62304 Activities |
|---------|----------------------------------------------------|----------------------|
| Phase 0 | Development environment, simulator, build system   | 5.1 (Planning)       |
| Phase 1 | UI framework, screen manager, navigation           | 5.3 (Architecture)   |
| Phase 2 | Waveform rendering, numeric display widgets        | 5.4 (Detailed Design)|
| Phase 3 | Alarm engine, alarm banner, alarm screen           | 5.4, 5.5 (Implementation) |
| Phase 4 | Patient data, settings store, trends database      | 5.5 (Implementation) |
| Phase 5 | Trend display, trend queries, data aggregation     | 5.5 (Implementation) |
| Phase 6 | Multi-process services, IPC, service manager       | 5.3, 5.5             |
| Phase 7 | Authentication, RBAC, audit logging                | 5.5 (Implementation) |
| Phase 8 | Buildroot BSP, target image, systemd integration   | 5.5, 5.8 (Release)   |
| Phase 9 | Network manager, FHIR client, ABDM client          | 5.5 (Implementation) |
| Phase 10| Security hardening (AppArmor, dm-verity, LUKS)     | 5.5 (Implementation) |
| Phase 11| OTA update, A/B partitioning                       | 5.5 (Implementation) |
| Phase 12| Sensor HAL interface for hardware team              | 5.5 (Implementation) |
| Phase 13| Test completion, regulatory documentation           | 5.7 (Verification), 5.8 (Release) |

---

## 4. Development Activities per IEC 62304

### 4.1 Software Requirements Analysis (Clause 5.2)

- Requirements captured in Software Requirements Specification (SRS-001).
- Requirements derived from PRD (REQ-001 through REQ-010) and risk analysis (RA-001).
- Each requirement has a unique identifier, priority, and verification method.
- Requirements reviewed and approved by Quality Assurance.

### 4.2 Software Architectural Design (Clause 5.3)

- Architecture documented in Software Architecture Document (SAD-001).
- Decomposition into software items: ui-app, sensor-service, alarm-service, network-service, audit-service, watchdog-service.
- Interfaces defined: nanomsg pub/sub for vitals data, D-Bus for control, SQLite for persistence.
- SOUP components identified and documented in SOUP List (SOUP-001).

### 4.3 Software Detailed Design (Clause 5.4)

- Module-level design documented via doxygen-style comments in source code headers.
- Key design patterns: static allocation, stack-based screen management, pure-logic alarm engine, vitals_provider abstraction layer.
- Database schema defined in source code: `trend_db.c`, `patient_data.c`, `audit_log.c`, `settings_store.c`, `auth_manager.c`.

### 4.4 Software Unit Implementation (Clause 5.5)

- Language: C99 with `-Wall -Wextra -Werror` compiler flags.
- No dynamic memory allocation in critical real-time paths.
- Coding standards: Linux kernel style with 4-space indentation, snake_case identifiers.
- All source code under version control (Git).

### 4.5 Software Integration and Integration Testing (Clause 5.6)

- Integration tests verify cross-module interactions:
  - alarm_engine + trend_db (alarm persistence)
  - auth_manager + audit_log (authentication audit trail)
  - patient_data + trend_db (patient vitals association)
- 160 integration tests executed via CTest.

### 4.6 Software System Testing (Clause 5.7)

- Verification activities defined in Software Test Plan (STP-001).
- Test case-to-requirement traceability maintained in Traceability Matrix (RTM-001).
- 403 unit tests + 160 integration tests = 563 total automated tests, all passing.
- System tests executed on simulator and target hardware.

### 4.7 Software Release (Clause 5.8)

- Release build produced by Buildroot with locked SOUP versions.
- Release artifacts: Buildroot image, release notes, test summary report.
- Release reviewed and approved prior to deployment.

---

## 5. Development Tools

| Tool             | Purpose                              | Version     |
|------------------|--------------------------------------|-------------|
| GCC              | C compiler (host and cross)          | 12.x        |
| CMake            | Build system generator               | 3.x         |
| Git              | Version control                      | 2.x         |
| Buildroot        | Embedded Linux build system          | 2024.02     |
| SDL2             | Host simulator display/input         | 2.x         |
| SQLite           | Embedded database (amalgamation)     | 3.45        |
| CTest            | Test execution framework             | 3.x         |
| GitHub           | Source hosting and issue tracking     | N/A         |
| Custom test_framework.h | Unit/integration test assertions | In-house |

---

## 6. Roles and Responsibilities

| Role                | Responsibilities                                              |
|---------------------|---------------------------------------------------------------|
| Engineering Team    | Requirements analysis, design, implementation, unit testing   |
| Quality Assurance   | Review of deliverables, test oversight, process compliance     |
| Regulatory Affairs  | Document approval, regulatory submissions, classification     |
| Clinical Advisors   | Requirements validation, usability review, acceptance testing |

---

## 7. Software Development Standards and Procedures

- **Coding standard:** C99, Linux kernel style, snake_case, doxygen comments.
- **Code review:** All changes reviewed prior to merge to main branch.
- **Build verification:** All 563 automated tests must pass before merge.
- **Configuration management:** Per Configuration Management Plan (CMP-001).
- **Risk management:** Per ISO 14971, documented in Risk Analysis (RA-001).
- **Change control:** Changes tracked via Git commits and GitHub Issues.

---

## 8. Referenced Documents

| Document ID | Title                               |
|-------------|-------------------------------------|
| SRS-001     | Software Requirements Specification |
| SAD-001     | Software Architecture Document      |
| STP-001     | Software Test Plan                  |
| RTM-001     | Requirements Traceability Matrix    |
| RA-001      | Risk Analysis                       |
| SOUP-001    | SOUP List                           |
| CMP-001     | Configuration Management Plan       |
| MTP-001     | Maintenance Plan                    |

---

## Revision History

| Version | Date       | Author           | Changes                                     |
|---------|------------|------------------|---------------------------------------------|
| 1.0     | 2026-02-15 | Engineering Team | Initial software development plan            |
