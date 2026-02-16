# Software Test Plan

| Field          | Value                                      |
|----------------|--------------------------------------------|
| Document ID    | STP-001                                    |
| Version        | 1.0                                        |
| Date           | 2026-02-15                                 |
| Author         | Engineering Team                           |
| Reviewer       | Quality Assurance                          |
| Approval       | Regulatory Affairs                         |
| Classification | IEC 62304 Software Safety Class B          |

---

## 1. Purpose

This document defines the test strategy, levels, environments, and criteria for verifying the Bedside Vitals Monitor software per IEC 62304 clause 5.7 and the Software Requirements Specification (SRS-001).

---

## 2. Test Levels

| Level            | Scope                                          | Responsibility | Tools               |
|------------------|------------------------------------------------|----------------|----------------------|
| Unit Test        | Individual functions and modules               | Developer      | Custom framework, CTest |
| Integration Test | Inter-module and IPC communication             | Developer      | Custom harness, CTest|
| System Test      | End-to-end on simulator and target hardware    | QA             | Manual + scripted    |
| Acceptance Test  | Clinical workflow validation with stakeholders | QA + Clinical  | Protocol-driven      |

---

## 3. Test Environments

| Environment          | Hardware              | Software                | Purpose                    |
|----------------------|-----------------------|-------------------------|----------------------------|
| Host (Mac simulator) | macOS + SDL2          | CMake build, `./simulator` | Unit, integration, UI tests|
| Target (STM32MP1)    | STM32MP157F-DK2       | Buildroot image         | System, acceptance tests   |
| CI/CD                | Linux runner (GitHub Actions) | Docker + CMake   | Automated regression       |

---

## 4. Entry and Exit Criteria

### Entry Criteria

- Source code compiles without errors or warnings (`-Wall -Werror`)
- All unit tests from previous iteration pass
- Test environment is configured and accessible
- Requirements under test have been reviewed and approved

### Exit Criteria

- All planned test cases executed
- No unresolved high or critical defects
- Code coverage target met: >= 80% statement coverage for safety-critical modules (alarm_engine, auth_manager, patient_data)
- Test report reviewed and signed

---

## 5. Test Case Format

Each test case shall follow this structure:

| Field             | Description                                        |
|-------------------|----------------------------------------------------|
| Test Case ID      | TC-XXX-NNN (e.g., TC-ALM-001)                     |
| Requirement ID    | SRS requirement(s) being verified                  |
| Description       | What is being tested                               |
| Preconditions     | Required state before execution                    |
| Test Steps        | Numbered steps to execute                          |
| Expected Result   | Observable outcome for pass                        |
| Actual Result     | [Filled during execution]                          |
| Pass / Fail       | [Filled during execution]                          |
| Tester / Date     | [Filled during execution]                          |

---

## 6. Test Categories

### 6.1 Vital Signs Accuracy

| Test Case ID | Description                                            | Requirement | Automated Test Function              |
|--------------|--------------------------------------------------------|-------------|--------------------------------------|
| TC-VIT-001   | Verify HR numeric matches simulated sensor input       | SRS-F001    | test_insert_query_raw_hr (trend_db)  |
| TC-VIT-002   | Verify SpO2 numeric matches simulated sensor input     | SRS-F002    | test_insert_query_raw_spo2 (trend_db)|
| TC-VIT-003   | Verify NIBP reading displayed after measurement cycle  | SRS-F003    | test_insert_query_nibp (trend_db)    |
| TC-VIT-004   | Verify temperature numeric accuracy                    | SRS-F004    | test_insert_query_raw_temp (trend_db)|
| TC-VIT-005   | Verify RR derivation from waveform data                | SRS-F005    | test_insert_query_raw_rr (trend_db)  |

### 6.2 Alarm Triggering and Timing

| Test Case ID | Description                                                      | Requirement | Automated Test Function                      |
|--------------|------------------------------------------------------------------|-------------|----------------------------------------------|
| TC-ALM-001   | High alarm triggers when HR exceeds upper limit                  | SRS-F008    | test_high_hr_critical (alarm_engine)         |
| TC-ALM-002   | Medium alarm triggers when SpO2 falls below warning threshold    | SRS-F008    | test_low_spo2_warning (alarm_engine)         |
| TC-ALM-003   | Alarm visual indicator matches priority level (color, flash)     | SRS-F009    | Manual system test                           |
| TC-ALM-004   | Alarm latency from condition to indication < 10 seconds          | SRS-S001    | Manual system test                           |
| TC-ALM-005   | Alarm acknowledgment requires authenticated user                 | SRS-F010    | test_permission_with_audit (auth_audit integ)|
| TC-ALM-006   | Alarm silence auto-expires after configured duration             | SRS-S002    | test_silence (alarm_engine)                  |
| TC-ALM-007   | Sensor disconnect detected and indicated                         | SRS-S003    | test_zero_value_skipped (alarm_engine)       |
| TC-ALM-008   | Alarm severity escalation re-alerts clinician                    | SRS-S008    | test_severity_escalation (alarm_engine)      |
| TC-ALM-009   | Multiple simultaneous alarms prioritized correctly               | SRS-F008    | test_multiple_alarms (alarm_engine)          |
| TC-ALM-010   | Custom alarm limits take effect immediately                      | SRS-F008    | test_custom_limits (alarm_engine)            |
| TC-ALM-011   | Acknowledge all active alarms in single action                   | SRS-F020    | test_acknowledge_all (alarm_engine)          |
| TC-ALM-012   | Alarm audio pause with configurable duration                     | SRS-F021    | test_audio_pause (alarm_engine)              |

### 6.3 Authentication and Authorization

| Test Case ID | Description                                            | Requirement | Automated Test Function                        |
|--------------|--------------------------------------------------------|-------------|-------------------------------------------------|
| TC-AUTH-001  | Valid PIN grants access to protected screens            | SRS-F016    | test_login_correct_pin (auth_manager)          |
| TC-AUTH-002  | Invalid PIN is rejected after max attempts              | SRS-F016    | test_login_wrong_pin (auth_manager)            |
| TC-AUTH-003  | Clinical role cannot access admin settings              | SRS-F017    | test_permissions_nurse, test_permissions_doctor (auth_manager) |
| TC-AUTH-004  | Session auto-locks after inactivity timeout             | SRS-X004    | test_session_timeout (auth_manager)            |
| TC-AUTH-005  | Login and logout events recorded in audit log           | SRS-F018    | test_login_audit_entry, test_logout_audit_entry (auth_audit integ) |
| TC-AUTH-006  | Admin role has all permissions                          | SRS-F017    | test_permissions_admin (auth_manager)          |
| TC-AUTH-007  | User CRUD operations (add, delete, change PIN)          | SRS-F016    | test_add_user, test_delete_user, test_change_pin (auth_manager) |
| TC-AUTH-008  | Failed login attempts generate audit entries            | SRS-F018    | test_failed_login_audit_entry (auth_audit integ)|
| TC-AUTH-009  | Session timeout generates audit entry                   | SRS-X004    | test_session_timeout_audit_entry (auth_audit integ) |

### 6.4 Data Persistence and Integrity

| Test Case ID | Description                                            | Requirement | Automated Test Function                          |
|--------------|--------------------------------------------------------|-------------|--------------------------------------------------|
| TC-DAT-001   | Vital signs are written to SQLite trend database       | SRS-F012    | test_admit_and_record_vitals (patient_trends integ)|
| TC-DAT-002   | Data older than retention period is auto-purged        | SRS-F012    | test_purge_raw, test_purge_nibp_alarm (trend_db) |
| TC-DAT-003   | Trend query returns correct data for all time windows  | SRS-F013    | test_query_trends_timeframe (patient_trends integ)|
| TC-DAT-004   | Database survives unclean shutdown (WAL journal mode)  | SRS-S006    | Manual system test                               |
| TC-DAT-005   | Data partition is encrypted at rest                    | SRS-X001    | Manual inspection                                |
| TC-DAT-006   | Minute-level trend aggregation produces correct values  | SRS-F012    | test_aggregation (trend_db)                      |
| TC-DAT-007   | Trend data survives patient discharge                  | SRS-F012    | test_discharge_preserves_trends (patient_trends integ) |
| TC-DAT-008   | Sync queue persists and processes pending items         | SRS-F024    | test_process_items (sync_queue)                  |

### 6.5 Network Resilience

| Test Case ID | Description                                            | Requirement | Automated Test Function                           |
|--------------|--------------------------------------------------------|-------------|---------------------------------------------------|
| TC-NET-001   | Device operates normally with no network               | SRS-P008    | test_close_reinit_lifecycle (sync_queue)          |
| TC-NET-002   | Queued data syncs when network becomes available       | SRS-F014    | test_process_items (sync_queue)                   |
| TC-NET-003   | Network status indicator reflects actual connectivity  | SRS-F014    | Manual system test                                |
| TC-NET-004   | FHIR export produces valid HL7 FHIR R4 resources      | SRS-F014    | test_build_observation_json_fields (fhir_client)  |
| TC-NET-005   | FHIR endpoint rejects plain HTTP (non-localhost)        | SRS-X009    | test_endpoint_plain_http_rejected (fhir_client)   |
| TC-NET-006   | FHIR Patient JSON contains correct LOINC and MRN codes | SRS-F023    | test_build_patient_json_fields (fhir_client)      |

### 6.6 UI Responsiveness

| Test Case ID | Description                                            | Requirement |
|--------------|--------------------------------------------------------|-------------|
| TC-UI-001    | Waveform renders at >= 30 FPS (measured)               | SRS-P001    |
| TC-UI-002    | Touch input response time < 200 ms                     | SRS-P005    |
| TC-UI-003    | Screen navigation transitions complete without stutter | SRS-P005    |
| TC-UI-004    | 72h trend chart loads within 2 seconds                 | SRS-P006    |

### 6.7 Edge Cases and Error Handling

| Test Case ID | Description                                            | Requirement | Automated Test Function                        |
|--------------|--------------------------------------------------------|-------------|-------------------------------------------------|
| TC-ERR-001   | UI recovers gracefully after sensor-service restart    | SRS-S004    | Manual system test                             |
| TC-ERR-002   | Database full condition handled without crash          | SRS-S006    | test_operations_without_init (trend_db)        |
| TC-ERR-003   | Out-of-range sensor values flagged, not displayed raw  | SRS-S005    | test_zero_value_skipped (alarm_engine)         |
| TC-ERR-004   | Rapid screen navigation does not cause memory leak     | SRS-P007    | Manual system test                             |
| TC-ERR-005   | Simultaneous alarm conditions prioritized correctly    | SRS-F008    | test_multiple_alarms (alarm_engine)            |
| TC-ERR-006   | NULL pointer arguments handled safely across all APIs  | SRS-S006    | test_null_data_safe (alarm_engine), test_null_key_safe (settings_store), test_login_null_args (auth_manager) |

### 6.8 Patient Data Management

| Test Case ID | Description                                            | Requirement | Automated Test Function                        |
|--------------|--------------------------------------------------------|-------------|-------------------------------------------------|
| TC-PAT-001   | Patient CRUD operations (create, read, update, delete) | SRS-F011    | test_save_and_get, test_update, test_delete (patient_data) |
| TC-PAT-002   | Patient search by MRN                                  | SRS-F011    | test_find_by_mrn (patient_data)                |
| TC-PAT-003   | Monitor slot association and disassociation             | SRS-F011    | test_associate_disassociate (patient_data)     |
| TC-PAT-004   | Admit and discharge workflow                           | SRS-F011    | test_admit_discharge (patient_data)            |

### 6.9 Audit Trail

| Test Case ID | Description                                            | Requirement | Automated Test Function                        |
|--------------|--------------------------------------------------------|-------------|-------------------------------------------------|
| TC-AUD-001   | Audit events recorded with timestamp, user, action     | SRS-X007    | test_entry_fields (audit_log)                  |
| TC-AUD-002   | Audit log queryable by user, event type, time range    | SRS-F018    | test_query_by_user, test_query_by_event, test_query_range (audit_log) |
| TC-AUD-003   | Audit log supports formatted messages                  | SRS-X007    | test_record_fmt (audit_log)                    |
| TC-AUD-004   | Full audit trail captures complete session workflow     | SRS-F018    | test_full_audit_trail (auth_audit integ)       |

---

## 7. Defect Management

| Severity   | Definition                                      | Resolution SLA   |
|------------|-------------------------------------------------|------------------|
| Critical   | Patient safety risk or complete loss of function | 24 hours         |
| High       | Major feature broken, no workaround             | 72 hours         |
| Medium     | Feature impaired, workaround available           | 2 weeks          |
| Low        | Cosmetic or minor usability issue                | Next release     |

Defect tracking tool: GitHub Issues with severity and component labels

---

## 8. Test Deliverables

- Test case specifications (per category above)
- Test execution logs
- Defect reports
- Test summary report (per IEC 62304 clause 5.7)
- Code coverage report

---

## 9. Test Results Summary

| Test Level       | Total Tests | Passed | Failed | Coverage |
|------------------|-------------|--------|--------|----------|
| Unit Tests       | 403         | 403    | 0      | See report |
| Integration Tests| 160         | 160    | 0      | See report |
| System Tests     | Planned     | --     | --     | --       |
| **Total**        | **563**     | **563**| **0**  |          |

---

## Revision History

| Version | Date       | Author           | Changes                                          |
|---------|------------|------------------|--------------------------------------------------|
| 1.0     | 2026-02-15 | Engineering Team | Complete test plan with test case mapping         |
