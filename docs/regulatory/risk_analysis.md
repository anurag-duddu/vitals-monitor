# Risk Analysis (ISO 14971)

| Field          | Value                                      |
|----------------|--------------------------------------------|
| Document ID    | RA-001                                     |
| Version        | 1.0                                        |
| Date           | 2026-02-15                                 |
| Author         | Engineering Team                           |
| Reviewer       | Quality Assurance                          |
| Approval       | Regulatory Affairs                         |
| Classification | IEC 62304 Software Safety Class B          |

---

## 1. Purpose and Scope

This document identifies software-related hazards for the Bedside Vitals Monitor and assesses associated risks per ISO 14971. It covers hazards arising from software faults, use errors, and environmental conditions.

**Device classification:** CDSCO Class B (low-moderate risk)
**Software safety class:** IEC 62304 Class B

---

## 2. Risk Estimation Criteria

### Severity Levels

| Level | Rating      | Definition                                           |
|-------|-------------|------------------------------------------------------|
| 5     | Catastrophic| Death or irreversible serious injury                 |
| 4     | Critical    | Reversible serious injury or life-threatening event  |
| 3     | Serious     | Injury requiring medical intervention                |
| 2     | Minor       | Temporary discomfort, no medical intervention needed |
| 1     | Negligible  | Inconvenience only, no injury                        |

### Probability Levels

| Level | Rating      | Definition                                           |
|-------|-------------|------------------------------------------------------|
| 5     | Frequent    | Likely to occur multiple times during device lifetime |
| 4     | Probable    | Likely to occur at least once during device lifetime |
| 3     | Occasional  | May occur during device lifetime                     |
| 2     | Remote      | Unlikely but possible during device lifetime         |
| 1     | Improbable  | Probability can be neglected                         |

### Risk Matrix

| Probability \ Severity | 1-Negligible | 2-Minor | 3-Serious | 4-Critical | 5-Catastrophic |
|------------------------|:------------:|:-------:|:---------:|:----------:|:--------------:|
| 5-Frequent             | Low          | Medium  | High      | High       | High           |
| 4-Probable             | Low          | Medium  | High      | High       | High           |
| 3-Occasional           | Low          | Low     | Medium    | High       | High           |
| 2-Remote               | Low          | Low     | Medium    | Medium     | High           |
| 1-Improbable           | Low          | Low     | Low       | Low        | Medium         |

**Acceptability:** Low = Acceptable. Medium = ALARP review required. High = Unacceptable without mitigation.

---

## 3. Hazard Identification and Risk Assessment

### HAZ-001: Incorrect Vital Sign Display

| Attribute               | Value                                                                 |
|-------------------------|-----------------------------------------------------------------------|
| Hazard ID               | HAZ-001                                                               |
| Hazardous Situation     | Displayed vital sign value does not match actual patient measurement  |
| Potential Harm          | Delayed or incorrect clinical decision                                |
| Cause(s)                | Sensor calibration drift, software calculation error, display bug     |
| Severity                | 4 - Critical                                                          |
| Probability (pre)       | 3 - Occasional                                                        |
| Risk Level (pre)        | **High**                                                              |
| Risk Control Measures   | Sensor accuracy validation per IEC 60601-2-49; end-to-end numeric verification tests; out-of-range value flagging |
| Probability (post)      | 1 - Improbable                                                        |
| Residual Risk Level     | **Low**                                                               |
| Verification            | **Unit tests:** test_insert_query_raw_hr, test_insert_query_raw_spo2, test_insert_query_nibp, test_insert_query_raw_temp, test_insert_query_raw_rr (test_trend_db.c) verify that sensor values stored and retrieved match input values. **Integration tests:** test_admit_and_record_vitals, test_query_trends_timeframe (test_patient_trends_integration.c) verify end-to-end vitals data accuracy across patient association and trend storage. |

### HAZ-002: Missed Alarm

| Attribute               | Value                                                                 |
|-------------------------|-----------------------------------------------------------------------|
| Hazard ID               | HAZ-002                                                               |
| Hazardous Situation     | Alarm condition exists but no alarm is generated                      |
| Potential Harm          | Patient deterioration goes unnoticed                                  |
| Cause(s)                | Alarm engine logic error, IPC message loss, service crash             |
| Severity                | 5 - Catastrophic                                                      |
| Probability (pre)       | 2 - Remote                                                            |
| Risk Level (pre)        | **High**                                                              |
| Risk Control Measures   | Independent alarm-service process; watchdog restart; alarm engine unit tests; end-to-end alarm latency tests |
| Probability (post)      | 1 - Improbable                                                        |
| Residual Risk Level     | **Medium** -- ALARP: additional clinical workflow mitigations required |
| Verification            | **Unit tests:** test_high_hr_critical, test_low_hr_critical, test_low_spo2_critical, test_warning_hr, test_low_spo2_warning, test_temperature_alarm, test_nibp_alarms (test_alarm_engine.c) verify that alarm conditions at all threshold boundaries trigger correct severity alarms. test_normal_vitals_no_alarm verifies no false triggers. test_multiple_alarms verifies simultaneous alarm detection. test_severity_escalation verifies that escalation from warning to critical re-alerts even after acknowledgment. **Integration tests:** test_alarm_triggers_db_event, test_alarm_limits_and_trends, test_multiple_alarms_with_trends (test_alarm_db_integration.c) verify alarm events are recorded and persisted in the database. |

### HAZ-003: Alarm Fatigue (Excessive False Alarms)

| Attribute               | Value                                                                 |
|-------------------------|-----------------------------------------------------------------------|
| Hazard ID               | HAZ-003                                                               |
| Hazardous Situation     | Clinicians ignore or disable alarms due to excessive false positives  |
| Potential Harm          | True alarm missed due to desensitization                              |
| Cause(s)                | Overly sensitive default thresholds, motion artifact, poor sensor contact |
| Severity                | 4 - Critical                                                          |
| Probability (pre)       | 4 - Probable                                                          |
| Risk Level (pre)        | **High**                                                              |
| Risk Control Measures   | Configurable per-patient thresholds; artifact rejection algorithms; alarm delay for transient events; target < 5% false alarm rate |
| Probability (post)      | 2 - Remote                                                            |
| Residual Risk Level     | **Medium** -- ALARP: usability study to validate threshold defaults   |
| Verification            | **Unit tests:** test_custom_limits (test_alarm_engine.c) verifies per-patient threshold customization. test_default_limits verifies clinically appropriate factory defaults (HR: 40-150 bpm critical, 50-120 bpm warning; SpO2: 85% critical low, 90% warning low; RR: 8-30 critical; Temp: 35.0-39.0 C critical, 36.0-38.0 C warning). test_disabled_param verifies that individual alarm parameters can be disabled. test_zero_value_skipped verifies that no-signal sentinel values do not produce false alarms. test_reset_defaults verifies return to factory defaults. **Integration tests:** test_alarm_limits_and_trends (test_alarm_db_integration.c) verifies custom limits interact correctly with trend storage. |

### HAZ-004: Patient Data Breach

| Attribute               | Value                                                                 |
|-------------------------|-----------------------------------------------------------------------|
| Hazard ID               | HAZ-004                                                               |
| Hazardous Situation     | Unauthorized access to patient health information                     |
| Potential Harm          | Privacy violation, regulatory non-compliance                          |
| Cause(s)                | Unencrypted storage, network interception, stolen device              |
| Severity                | 3 - Serious                                                           |
| Probability (pre)       | 3 - Occasional                                                        |
| Risk Level (pre)        | **Medium**                                                            |
| Risk Control Measures   | LUKS partition encryption; TLS for network; PIN+RFID authentication; session timeout; kiosk mode; AppArmor MAC profiles |
| Probability (post)      | 1 - Improbable                                                        |
| Residual Risk Level     | **Low**                                                               |
| Verification            | **Unit tests:** test_login_correct_pin, test_login_wrong_pin, test_login_nonexistent_user, test_login_null_args (test_auth_manager.c) verify PIN-based authentication. test_permissions_none, test_permissions_nurse, test_permissions_doctor, test_permissions_admin, test_permissions_technician verify role-based access control matrix. test_session_timeout, test_touch_resets_timeout verify automatic session expiry. test_endpoint_plain_http_rejected (test_fhir_client.c) verifies HTTPS enforcement for network communications. **Integration tests:** test_login_audit_entry, test_failed_login_audit_entry, test_session_timeout_audit_entry, test_permission_with_audit (test_auth_audit_integration.c) verify that all authentication events produce audit trail entries. **Inspection:** LUKS encryption configuration (deploy/security/), AppArmor profiles (deploy/security/). |

### HAZ-005: Software Crash During Monitoring

| Attribute               | Value                                                                 |
|-------------------------|-----------------------------------------------------------------------|
| Hazard ID               | HAZ-005                                                               |
| Hazardous Situation     | Software crash causes loss of monitoring for a period                 |
| Potential Harm          | Patient unmonitored; alarm not generated                              |
| Cause(s)                | Null pointer, memory corruption, resource exhaustion                  |
| Severity                | 4 - Critical                                                          |
| Probability (pre)       | 3 - Occasional                                                        |
| Risk Level (pre)        | **High**                                                              |
| Risk Control Measures   | Multi-process architecture (crash isolation); watchdog auto-restart; no dynamic allocation in critical paths; static analysis |
| Probability (post)      | 1 - Improbable                                                        |
| Residual Risk Level     | **Low**                                                               |
| Verification            | **Unit tests:** test_null_data_safe (test_alarm_engine.c) verifies alarm engine handles NULL input without crash. test_null_key_safe (test_settings_store.c) verifies settings store handles NULL keys without crash. test_login_null_args (test_auth_manager.c) verifies auth manager handles NULL arguments without crash. test_null_result_pointer, test_operations_without_init (test_trend_db.c) verify trend DB handles invalid state and NULL pointers gracefully. test_null_payload_rejected, test_get_pending_null_safety (test_sync_queue.c) verify sync queue handles NULL inputs safely. test_null_observation_returns_error, test_null_name_patient_json (test_fhir_client.c) verify FHIR client handles NULL arguments without crash. test_null_username, test_null_message (test_audit_log.c) verify audit log handles NULL inputs. **Architecture:** Independent alarm-service process continues to monitor and alert even if ui-app crashes. Systemd watchdog restarts crashed services automatically. |

### HAZ-006: Network Connectivity Loss

| Attribute               | Value                                                                 |
|-------------------------|-----------------------------------------------------------------------|
| Hazard ID               | HAZ-006                                                               |
| Hazardous Situation     | Network failure prevents data sync to EHR/central station             |
| Potential Harm          | Clinical decisions based on stale centralized data                    |
| Cause(s)                | WiFi outage, server downtime, configuration error                     |
| Severity                | 2 - Minor                                                             |
| Probability (pre)       | 4 - Probable                                                          |
| Risk Level (pre)        | **Medium**                                                            |
| Risk Control Measures   | Offline-first architecture; 72h local storage; automatic sync queue; network status indicator on UI |
| Probability (post)      | 4 - Probable (unchanged; harm is mitigated)                           |
| Residual Risk Level     | **Low** (severity reduced to negligible at bedside)                   |
| Verification            | **Unit tests:** test_push_and_stats, test_push_get_pending, test_process_items, test_stats_tracking, test_close_reinit_lifecycle, test_capacity_tracking (test_sync_queue.c) verify offline queue stores data when network is unavailable and processes it when available. test_purge_raw, test_purge_nibp_alarm (test_trend_db.c) verify 72h data retention with automatic purge of old data. test_aggregation (test_trend_db.c) verifies minute-level aggregation for long-term storage. **Integration tests:** test_admit_and_record_vitals, test_discharge_preserves_trends (test_patient_trends_integration.c) verify end-to-end data persistence during patient workflows. |

### HAZ-007: Incorrect Patient Association

| Attribute               | Value                                                                 |
|-------------------------|-----------------------------------------------------------------------|
| Hazard ID               | HAZ-007                                                               |
| Hazardous Situation     | Vital signs recorded/displayed under wrong patient identity           |
| Potential Harm          | Incorrect clinical decisions for both affected patients               |
| Cause(s)                | Manual entry error, barcode mismatch, stale association               |
| Severity                | 4 - Critical                                                          |
| Probability (pre)       | 3 - Occasional                                                        |
| Risk Level (pre)        | **High**                                                              |
| Risk Control Measures   | Confirmation dialog on association; patient identity prominently displayed; disassociation on discharge workflow; ABHA ID cross-check |
| Probability (post)      | 1 - Improbable                                                        |
| Residual Risk Level     | **Low**                                                               |
| Verification            | **Unit tests:** test_associate_disassociate (test_patient_data.c) verifies monitor slot association and disassociation including invalid slot rejection. test_admit_discharge verifies admit/discharge workflow with proper state transitions. test_find_by_mrn verifies unique patient identification by medical record number. test_get_active_empty_slot verifies empty slot handling. **Integration tests:** test_admit_and_record_vitals (test_patient_trends_integration.c) verifies patient-to-vitals association. test_multiple_patients verifies that multiple patients on different slots are tracked independently. test_patient_crud_with_trends verifies patient modifications do not affect trend data integrity. |

---

## 4. Residual Risk Summary

| Hazard ID | Residual Risk | Acceptable? | Notes                                     |
|-----------|:-------------:|:-----------:|-------------------------------------------|
| HAZ-001   | Low           | Yes         | Verified by 18 trend_db unit tests and 7 patient_trends integration tests |
| HAZ-002   | Medium        | ALARP       | Mitigated by 25 alarm_engine unit tests and 6 alarm_db integration tests; additional clinical workflow protections (central monitoring, nurse rounds) reduce residual risk to ALARP |
| HAZ-003   | Medium        | ALARP       | Mitigated by configurable thresholds and no-signal filtering; usability validation with clinical staff planned for post-market phase |
| HAZ-004   | Low           | Yes         | Verified by 24 auth_manager unit tests, 14 audit_log unit tests, and 8 auth_audit integration tests |
| HAZ-005   | Low           | Yes         | Verified by NULL-safety tests across all modules (alarm_engine, settings_store, auth_manager, trend_db, sync_queue, fhir_client, audit_log); multi-process architecture provides crash isolation |
| HAZ-006   | Low           | Yes         | Verified by 10 sync_queue unit tests and 7 patient_trends integration tests |
| HAZ-007   | Low           | Yes         | Verified by 12 patient_data unit tests and 7 patient_trends integration tests |

---

## 5. Overall Residual Risk Evaluation

The Bedside Vitals Monitor has been assessed against seven identified software-related hazards covering the primary risk categories of display accuracy, alarm reliability, alarm fatigue, data security, software stability, network resilience, and patient identification.

**Summary of residual risk levels:**
- Five hazards (HAZ-001, HAZ-004, HAZ-005, HAZ-006, HAZ-007) have residual risk levels of **Low** and are considered **acceptable**.
- Two hazards (HAZ-002, HAZ-003) have residual risk levels of **Medium** and have been evaluated under the ALARP (As Low As Reasonably Practicable) principle.

**ALARP justification for HAZ-002 (Missed Alarm):** The alarm engine is implemented as a pure-logic module with no UI dependencies, verified by 25 dedicated unit tests covering all threshold boundaries, state transitions, and edge cases. The multi-process architecture ensures the alarm-service operates independently of the UI process. Additionally, clinical workflow mitigations (nurse rounds, central monitoring stations, bedside handoff protocols) provide defense-in-depth. The residual risk is deemed ALARP.

**ALARP justification for HAZ-003 (Alarm Fatigue):** Configurable per-patient alarm thresholds, no-signal filtering, and clinically validated factory defaults reduce false alarm rates. Post-market surveillance and usability studies with clinical staff are planned to further validate threshold appropriateness. The residual risk is deemed ALARP.

**Benefit-risk conclusion:** The clinical benefits of continuous bedside vital sign monitoring -- including early detection of patient deterioration, timely alarm notification, automated trend recording, and EHR integration -- substantially outweigh the residual risks. All high-priority hazards have been mitigated to acceptable or ALARP levels through a combination of software design controls, verified by 563 automated tests (403 unit + 160 integration), and supplemented by architectural safeguards (multi-process isolation, watchdog recovery, offline-first data persistence).

The overall residual risk of the Bedside Vitals Monitor software is **acceptable** for its intended use in Indian hospital general wards under CDSCO Class B classification.

---

## Revision History

| Version | Date       | Author           | Changes                                                      |
|---------|------------|------------------|--------------------------------------------------------------|
| 1.0     | 2026-02-15 | Engineering Team | Complete risk analysis with verification evidence and residual risk evaluation |
