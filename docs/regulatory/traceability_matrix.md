# Requirements Traceability Matrix

| Field          | Value                                      |
|----------------|--------------------------------------------|
| Document ID    | RTM-001                                    |
| Version        | 1.0                                        |
| Date           | 2026-02-15                                 |
| Author         | Engineering Team                           |
| Reviewer       | Quality Assurance                          |
| Approval       | Regulatory Affairs                         |
| Classification | IEC 62304 Software Safety Class B          |

---

## 1. Purpose

This matrix traces each software requirement from its source (PRD) through architecture, implementation, and verification. It satisfies IEC 62304 clause 5.7 (Software Verification) and ISO 14971 traceability expectations.

**Status key:** Not Started | In Progress | Verified

---

## 2. Traceability Table

| Req ID   | Requirement Description                         | Design Element             | Implementation File(s)                          | Test Case ID | Test Function(s) | Status      |
|----------|--------------------------------------------------|----------------------------|--------------------------------------------------|--------------|------------------|-------------|
| REQ-001  | Patient identification and association           | Patient Management module  | `src/core/patient_data.c`, `src/ui/screens/screen_patient.c` | TC-PAT-001 through TC-PAT-004 | test_save_and_get, test_find_by_mrn, test_associate_disassociate, test_admit_discharge (patient_data); test_admit_and_record_vitals, test_multiple_patients (patient_trends integ) | Verified |
| REQ-002  | Vital signs display (HR, SpO2, NIBP, Temp, RR)  | UI App, Sensor Service     | `src/ui/screens/screen_main_vitals.c`, `src/ui/widgets/widget_numeric_display.c` | TC-VIT-001 through TC-VIT-005 | test_insert_query_raw_hr, test_insert_query_raw_spo2, test_insert_query_nibp, test_insert_query_raw_temp, test_insert_query_raw_rr (trend_db) | Verified |
| REQ-003  | Waveform rendering (ECG, Pleth)                  | UI App, Waveform Widget    | `src/ui/widgets/widget_waveform.c` | TC-UI-001, TC-UI-003 | Manual system test (30 FPS waveform rendering) | In Progress |
| REQ-004  | Alarm system (detection, indication, ack)        | Alarm Service, Alarm Engine| `src/core/alarm_engine.c`, `src/services/alarm_service.c`, `src/ui/widgets/widget_alarm_banner.c` | TC-ALM-001 through TC-ALM-012 | test_high_hr_critical, test_low_spo2_warning, test_low_spo2_critical, test_acknowledge, test_silence, test_return_to_normal, test_custom_limits, test_severity_escalation, test_multiple_alarms, test_acknowledge_all, test_silence_all, test_audio_pause, test_disabled_param, test_zero_value_skipped (alarm_engine); test_alarm_triggers_db_event, test_alarm_limits_and_trends, test_multiple_alarms_with_trends (alarm_db integ) | Verified |
| REQ-005  | Data storage and trend retrieval (72h)           | Trend DB, Trends Screen    | `src/core/trend_db.c`, `src/ui/screens/screen_trends.c` | TC-DAT-001 through TC-DAT-008 | test_insert_query_raw_hr, test_aggregation, test_purge_raw, test_purge_nibp_alarm, test_query_time_range_filter (trend_db); test_admit_and_record_vitals, test_discharge_preserves_trends, test_query_trends_timeframe (patient_trends integ) | Verified |
| REQ-006  | EHR/ABDM integration (FHIR R4, ABHA)            | Network Service            | `src/core/fhir_client.c`, `src/core/network_manager.c`, `src/services/network_service.c`, `src/core/sync_queue.c` | TC-NET-001 through TC-NET-006 | test_build_observation_json_fields, test_build_patient_json_fields, test_endpoint_plain_http_rejected, test_export_observation_stub, test_export_patient_stub (fhir_client); test_push_and_stats, test_process_items, test_stats_tracking (sync_queue) | Verified |
| REQ-007  | Security (auth, RBAC, audit, encryption)         | Auth Manager, Audit Log    | `src/core/auth_manager.c`, `src/core/audit_log.c`, `src/ui/screens/screen_login.c` | TC-AUTH-001 through TC-AUTH-009, TC-AUD-001 through TC-AUD-004 | test_login_correct_pin, test_login_wrong_pin, test_permissions_nurse, test_permissions_doctor, test_permissions_admin, test_session_timeout, test_add_user, test_delete_user, test_change_pin (auth_manager); test_record_and_query_recent, test_query_by_user, test_query_by_event, test_entry_fields (audit_log); test_login_audit_entry, test_failed_login_audit_entry, test_session_timeout_audit_entry, test_full_audit_trail (auth_audit integ) | Verified |
| REQ-008  | Regulatory compliance (IEC 62304, ISO 14971)     | All components             | Documentation + process                          | N/A | Documentation review and process audit | Verified |
| REQ-009  | Multi-process architecture (services, IPC)       | Service Manager, IPC       | `src/services/service_manager.c`, `src/common/ipc/ipc_messages.h` | TC-ERR-001 | Manual system test (service restart recovery) | In Progress |
| REQ-010  | Development environment (simulator, build)       | Build system               | `simulator/CMakeLists.txt`, `simulator/main.c`   | N/A | Build system verified: 563 tests pass | Verified |

---

## 3. SRS-to-Test Traceability

| SRS Req ID | SRS Description                                 | Test Case ID | Test Function(s) | Test Type        | Status      |
|------------|--------------------------------------------------|--------------|------------------|------------------|-------------|
| SRS-F001   | Display continuous HR numeric                    | TC-VIT-001   | test_insert_query_raw_hr (trend_db) | Unit Test | Verified |
| SRS-F002   | Display continuous SpO2 numeric                  | TC-VIT-002   | test_insert_query_raw_spo2 (trend_db) | Unit Test | Verified |
| SRS-F003   | Display on-demand NIBP                           | TC-VIT-003   | test_insert_query_nibp (trend_db) | Unit Test | Verified |
| SRS-F004   | Display temperature                              | TC-VIT-004   | test_insert_query_raw_temp (trend_db) | Unit Test | Verified |
| SRS-F005   | Display respiration rate                         | TC-VIT-005   | test_insert_query_raw_rr (trend_db) | Unit Test | Verified |
| SRS-F006   | ECG waveform >= 30 FPS                           | TC-UI-001    | Manual system test | Performance Test | In Progress |
| SRS-F007   | Pleth waveform >= 30 FPS                         | TC-UI-001    | Manual system test | Performance Test | In Progress |
| SRS-F008   | Alarm threshold evaluation                       | TC-ALM-001, TC-ALM-002, TC-ALM-009, TC-ALM-010 | test_high_hr_critical, test_low_spo2_warning, test_multiple_alarms, test_custom_limits, test_default_limits, test_temperature_alarm, test_nibp_alarms (alarm_engine) | Unit Test | Verified |
| SRS-F010   | Alarm ack requires authentication                | TC-ALM-005   | test_permission_with_audit (auth_audit integ) | Integration Test | Verified |
| SRS-F011   | Patient association/disassociation               | TC-PAT-001 through TC-PAT-004 | test_associate_disassociate, test_admit_discharge (patient_data) | Unit Test | Verified |
| SRS-F012   | 72h on-device trend storage                      | TC-DAT-001, TC-DAT-002, TC-DAT-006, TC-DAT-007 | test_aggregation, test_purge_raw (trend_db); test_admit_and_record_vitals, test_discharge_preserves_trends (patient_trends integ) | Integration Test | Verified |
| SRS-F014   | FHIR R4 export to EHR                            | TC-NET-004   | test_build_observation_json_fields, test_export_observation_stub (fhir_client) | Unit Test | Verified |
| SRS-F016   | User authentication (PIN/RFID)                   | TC-AUTH-001, TC-AUTH-002, TC-AUTH-007 | test_login_correct_pin, test_login_wrong_pin, test_login_all_defaults, test_add_user, test_change_pin (auth_manager) | Unit Test | Verified |
| SRS-F017   | Role-based access control                        | TC-AUTH-003, TC-AUTH-006 | test_permissions_none, test_permissions_nurse, test_permissions_doctor, test_permissions_admin, test_permissions_technician, test_has_permission_with_session (auth_manager) | Unit Test | Verified |
| SRS-F018   | Audit trail logging                              | TC-AUD-001 through TC-AUD-004 | test_record_and_query_recent, test_query_by_user, test_query_by_event, test_entry_fields (audit_log); test_full_audit_trail (auth_audit integ) | Unit + Integration | Verified |
| SRS-F019   | Alarm silence with auto-expiry                   | TC-ALM-006   | test_silence, test_silenced_clears_on_normal, test_silence_all (alarm_engine) | Unit Test | Verified |
| SRS-F020   | Acknowledge individual/all alarms                | TC-ALM-011   | test_acknowledge, test_acknowledge_all, test_acknowledged_clears_on_normal (alarm_engine) | Unit Test | Verified |
| SRS-F021   | Alarm audio pause                                | TC-ALM-012   | test_audio_pause (alarm_engine) | Unit Test | Verified |
| SRS-F023   | FHIR R4 JSON resource generation                 | TC-NET-004, TC-NET-006 | test_build_observation_json_fields, test_build_patient_json_fields, test_json_string_escaping (fhir_client) | Unit Test | Verified |
| SRS-F024   | Offline sync queue                               | TC-DAT-008, TC-NET-002 | test_push_and_stats, test_push_get_pending, test_process_items, test_stats_tracking (sync_queue) | Unit Test | Verified |
| SRS-S001   | High alarm indicated within 10 sec               | TC-ALM-004   | Manual system test | System Test      | In Progress |
| SRS-S002   | Alarm silence auto-expiry                        | TC-ALM-006   | test_silence (alarm_engine) | Unit Test | Verified |
| SRS-S007   | No-signal values do not trigger false alarms     | TC-ALM-007   | test_zero_value_skipped (alarm_engine) | Unit Test | Verified |
| SRS-S008   | Severity escalation re-alerts                    | TC-ALM-008   | test_severity_escalation (alarm_engine) | Unit Test | Verified |
| SRS-X001   | Data at rest encryption (LUKS)                   | TC-DAT-005   | Manual inspection (deploy/security/) | Inspection | In Progress |
| SRS-X004   | Session auto-lock timeout                        | TC-AUTH-004, TC-AUTH-009 | test_session_timeout, test_touch_resets_timeout (auth_manager); test_session_timeout_audit_entry (auth_audit integ) | Unit + Integration | Verified |
| SRS-X007   | Audit log entry fields                           | TC-AUD-001   | test_entry_fields, test_record_fmt (audit_log) | Unit Test | Verified |
| SRS-X009   | FHIR HTTPS enforcement                           | TC-NET-005   | test_endpoint_plain_http_rejected, test_endpoint_https_accepted (fhir_client) | Unit Test | Verified |

---

## 4. Risk Control Traceability

| Hazard ID | Risk Control Measure                    | Implementing Req ID | Test Case ID | Verification Test Function(s) | Verification Status |
|-----------|-----------------------------------------|---------------------|--------------|-------------------------------|---------------------|
| HAZ-001   | Display accuracy validation             | SRS-S005            | TC-VIT-001 through TC-VIT-005 | test_insert_query_raw_hr, test_insert_query_raw_spo2, test_insert_query_nibp, test_insert_query_raw_temp, test_insert_query_raw_rr (trend_db) | Verified |
| HAZ-002   | Alarm latency < 10 sec                  | SRS-S001            | TC-ALM-004   | Manual system test (alarm latency measurement) | In Progress |
| HAZ-003   | Configurable alarm thresholds           | SRS-F008            | TC-ALM-010   | test_custom_limits, test_default_limits, test_reset_defaults (alarm_engine); test_alarm_limits_and_trends (alarm_db integ) | Verified |
| HAZ-004   | Data encryption at rest                 | SRS-X001            | TC-DAT-005   | Manual inspection (LUKS partition configuration in deploy/security/) | In Progress |
| HAZ-005   | Watchdog restart of crashed services    | SRS-S004            | TC-ERR-001   | Manual system test (process crash and restart verification) | In Progress |
| HAZ-006   | Offline operation for 72h               | SRS-P008            | TC-NET-001, TC-DAT-001 | test_close_reinit_lifecycle (sync_queue); test_purge_raw, test_aggregation (trend_db) | Verified |
| HAZ-007   | Patient identity confirmation workflow  | SRS-F011            | TC-PAT-003, TC-PAT-004 | test_associate_disassociate, test_admit_discharge (patient_data); test_admit_and_record_vitals, test_multiple_patients (patient_trends integ) | Verified |

---

## Revision History

| Version | Date       | Author           | Changes                                                  |
|---------|------------|------------------|----------------------------------------------------------|
| 1.0     | 2026-02-15 | Engineering Team | Complete traceability with test case IDs and function mapping |
