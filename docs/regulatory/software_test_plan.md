# Software Test Plan

| Field          | Value                                      |
|----------------|--------------------------------------------|
| Document ID    | STP-001                                    |
| Version        | [TODO]                                     |
| Date           | [TODO]                                     |
| Author         | [TODO]                                     |
| Reviewer       | [TODO]                                     |
| Approval       | [TODO]                                     |
| Classification | IEC 62304 Software Safety Class B          |

---

## 1. Purpose

This document defines the test strategy, levels, environments, and criteria for verifying the Bedside Vitals Monitor software per IEC 62304 clause 5.7 and the Software Requirements Specification (SRS-001).

---

## 2. Test Levels

| Level            | Scope                                          | Responsibility | Tools               |
|------------------|------------------------------------------------|----------------|----------------------|
| Unit Test        | Individual functions and modules               | Developer      | Unity / CMocka, CTest|
| Integration Test | Inter-module and IPC communication             | Developer      | Custom harness, CTest|
| System Test      | End-to-end on simulator and target hardware    | QA             | Manual + scripted    |
| Acceptance Test  | Clinical workflow validation with stakeholders | QA + Clinical  | Protocol-driven      |

---

## 3. Test Environments

| Environment          | Hardware              | Software                | Purpose                    |
|----------------------|-----------------------|-------------------------|----------------------------|
| Host (Mac simulator) | macOS + SDL2          | CMake build, `./simulator` | Unit, integration, UI tests|
| Target (STM32MP1)    | STM32MP157F-DK2       | Buildroot image         | System, acceptance tests   |
| CI/CD                | [TODO - Linux runner] | Docker + CMake          | Automated regression       |

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
- Code coverage target met: [TODO -- target >= 80% statement coverage for safety-critical modules]
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

| Test Case ID | Description                                            | Requirement |
|--------------|--------------------------------------------------------|-------------|
| TC-VIT-001   | Verify HR numeric matches simulated sensor input       | SRS-F001    |
| TC-VIT-002   | Verify SpO2 numeric matches simulated sensor input     | SRS-F002    |
| TC-VIT-003   | Verify NIBP reading displayed after measurement cycle  | SRS-F003    |
| TC-VIT-004   | Verify temperature numeric accuracy                    | SRS-F004    |
| TC-VIT-005   | Verify RR derivation from waveform data                | SRS-F005    |
| [TODO]       | [Add sensor-specific accuracy tests for target HW]     | [TODO]      |

### 6.2 Alarm Triggering and Timing

| Test Case ID | Description                                                      | Requirement |
|--------------|------------------------------------------------------------------|-------------|
| TC-ALM-001   | High alarm triggers when HR exceeds upper limit                  | SRS-F008    |
| TC-ALM-002   | Medium alarm triggers when SpO2 falls below warning threshold    | SRS-F008    |
| TC-ALM-003   | Alarm visual indicator matches priority level (color, flash)     | SRS-F009    |
| TC-ALM-004   | Alarm latency from condition to indication < 10 seconds          | SRS-S001    |
| TC-ALM-005   | Alarm acknowledgment requires authenticated user                 | SRS-F010    |
| TC-ALM-006   | Alarm silence auto-expires after configured duration             | SRS-S002    |
| TC-ALM-007   | Sensor disconnect detected and indicated                         | SRS-S003    |
| [TODO]       | [Add escalation and alarm history tests]                         | [TODO]      |

### 6.3 Authentication and Authorization

| Test Case ID | Description                                            | Requirement |
|--------------|--------------------------------------------------------|-------------|
| TC-AUTH-001  | Valid PIN grants access to protected screens            | SRS-F016    |
| TC-AUTH-002  | Invalid PIN is rejected after max attempts              | SRS-F016    |
| TC-AUTH-003  | Clinical role cannot access admin settings              | SRS-F017    |
| TC-AUTH-004  | Session auto-locks after inactivity timeout             | SRS-X004    |
| TC-AUTH-005  | Login and logout events recorded in audit log           | SRS-F018    |
| [TODO]       | [Add RFID badge tests for target hardware]              | [TODO]      |

### 6.4 Data Persistence and Integrity

| Test Case ID | Description                                            | Requirement |
|--------------|--------------------------------------------------------|-------------|
| TC-DAT-001   | Vital signs are written to SQLite trend database       | SRS-F012    |
| TC-DAT-002   | Data older than retention period is auto-purged        | SRS-F012    |
| TC-DAT-003   | Trend query returns correct data for all time windows  | SRS-F013    |
| TC-DAT-004   | Database survives unclean shutdown (WAL journal mode)  | SRS-S006    |
| TC-DAT-005   | Data partition is encrypted at rest                    | SRS-X001    |
| [TODO]       | [Add sync queue persistence tests]                     | [TODO]      |

### 6.5 Network Resilience

| Test Case ID | Description                                            | Requirement |
|--------------|--------------------------------------------------------|-------------|
| TC-NET-001   | Device operates normally with no network               | SRS-P008    |
| TC-NET-002   | Queued data syncs when network becomes available       | SRS-F014    |
| TC-NET-003   | Network status indicator reflects actual connectivity  | SRS-F014    |
| TC-NET-004   | FHIR export produces valid HL7 FHIR R4 resources      | SRS-F014    |
| [TODO]       | [Add ABDM integration tests]                           | [TODO]      |

### 6.6 UI Responsiveness

| Test Case ID | Description                                            | Requirement |
|--------------|--------------------------------------------------------|-------------|
| TC-UI-001    | Waveform renders at >= 30 FPS (measured)               | SRS-P001    |
| TC-UI-002    | Touch input response time < 200 ms                     | SRS-P005    |
| TC-UI-003    | Screen navigation transitions complete without stutter | SRS-P005    |
| TC-UI-004    | 72h trend chart loads within 2 seconds                 | SRS-P006    |
| [TODO]       | [Add multi-patient split-screen tests]                 | [TODO]      |

### 6.7 Edge Cases and Error Handling

| Test Case ID | Description                                            | Requirement |
|--------------|--------------------------------------------------------|-------------|
| TC-ERR-001   | UI recovers gracefully after sensor-service restart    | SRS-S004    |
| TC-ERR-002   | Database full condition handled without crash          | SRS-S006    |
| TC-ERR-003   | Out-of-range sensor values flagged, not displayed raw  | SRS-S005    |
| TC-ERR-004   | Rapid screen navigation does not cause memory leak     | SRS-P007    |
| TC-ERR-005   | Simultaneous alarm conditions prioritized correctly    | SRS-F008    |
| [TODO]       | [Add power-cycle and boot-time tests]                  | [TODO]      |

---

## 7. Defect Management

| Severity   | Definition                                      | Resolution SLA |
|------------|-------------------------------------------------|----------------|
| Critical   | Patient safety risk or complete loss of function | [TODO]         |
| High       | Major feature broken, no workaround             | [TODO]         |
| Medium     | Feature impaired, workaround available           | [TODO]         |
| Low        | Cosmetic or minor usability issue                | [TODO]         |

Defect tracking tool: [TODO -- specify tool, e.g., GitHub Issues, Jira]

---

## 8. Test Deliverables

- Test case specifications (per category above)
- Test execution logs
- Defect reports
- Test summary report (per IEC 62304 clause 5.7)
- Code coverage report

---

## Revision History

| Version | Date   | Author | Changes         |
|---------|--------|--------|-----------------|
| [TODO]  | [TODO] | [TODO] | Initial draft   |
