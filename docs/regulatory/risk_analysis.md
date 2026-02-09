# Risk Analysis (ISO 14971)

| Field          | Value                                      |
|----------------|--------------------------------------------|
| Document ID    | RA-001                                     |
| Version        | [TODO]                                     |
| Date           | [TODO]                                     |
| Author         | [TODO]                                     |
| Reviewer       | [TODO]                                     |
| Approval       | [TODO]                                     |
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
| Verification            | [TODO - test case IDs]                                                |

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
| Verification            | [TODO - test case IDs]                                                |

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
| Verification            | [TODO - test case IDs]                                                |

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
| Risk Control Measures   | LUKS partition encryption; TLS for network; PIN+RFID authentication; session timeout; kiosk mode |
| Probability (post)      | 1 - Improbable                                                        |
| Residual Risk Level     | **Low**                                                               |
| Verification            | [TODO - test case IDs]                                                |

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
| Verification            | [TODO - test case IDs]                                                |

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
| Verification            | [TODO - test case IDs]                                                |

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
| Verification            | [TODO - test case IDs]                                                |

---

## 4. Residual Risk Summary

| Hazard ID | Residual Risk | Acceptable? | Notes                                     |
|-----------|:-------------:|:-----------:|-------------------------------------------|
| HAZ-001   | Low           | Yes         |                                            |
| HAZ-002   | Medium        | ALARP       | [TODO - document additional mitigations]   |
| HAZ-003   | Medium        | ALARP       | [TODO - usability validation needed]       |
| HAZ-004   | Low           | Yes         |                                            |
| HAZ-005   | Low           | Yes         |                                            |
| HAZ-006   | Low           | Yes         |                                            |
| HAZ-007   | Low           | Yes         |                                            |
| [TODO]    | [TODO]        | [TODO]      | [Add hazards as design progresses]         |

---

## 5. Overall Residual Risk Evaluation

[TODO -- After all individual hazards are assessed, provide an overall benefit-risk evaluation statement here, confirming that the overall residual risk is acceptable when weighed against the clinical benefits of continuous bedside vital sign monitoring.]

---

## Revision History

| Version | Date   | Author | Changes         |
|---------|--------|--------|-----------------|
| [TODO]  | [TODO] | [TODO] | Initial draft   |
