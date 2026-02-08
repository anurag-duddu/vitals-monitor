# Alarm Requirements: Bedside Vitals Monitor

## Document Information

| Attribute | Value |
|-----------|-------|
| Document ID | PRD-004 |
| Version | 1.0 |
| Status | Draft |
| Parent Document | PRD-001 Product Overview |
| Applicable Standard | IEC 60601-1-8:2020 |

---

## 1. Introduction

### 1.1 Purpose

This document defines the alarm system requirements for the bedside vitals monitor. Alarms are the primary safety mechanism for notifying clinical staff of patient deterioration or equipment issues. The design must balance sensitivity (catching real events) against alarm fatigue (excessive false/nuisance alarms).

### 1.2 Scope

This document covers:
- Alarm philosophy and principles
- Alarm categorization and prioritization
- Alarm generation conditions
- Alarm presentation (visual and audible)
- Alarm interaction (acknowledgment, pause, configuration)
- Alarm logging and history
- IEC 60601-1-8 compliance considerations

### 1.3 Definitions

| Term | Definition |
|------|------------|
| **Alarm Condition** | State of the equipment in which the alarm system has determined that a potential or actual hazard exists |
| **Alarm Signal** | Visual and/or auditory indication of an alarm condition |
| **Physiological Alarm** | Alarm triggered by patient vital sign exceeding threshold |
| **Technical Alarm** | Alarm triggered by equipment/sensor issue |
| **Latching Alarm** | Alarm that remains active until manually acknowledged, even if condition resolves |
| **Non-latching Alarm** | Alarm that automatically clears when condition resolves |
| **Alarm Fatigue** | Desensitization to alarms due to excessive frequency, leading to missed critical alarms |

### 1.4 Requirement Notation

| Prefix | Category |
|--------|----------|
| ALM-PHI | Alarm Philosophy |
| ALM-GEN | Alarm Generation |
| ALM-PRI | Alarm Prioritization |
| ALM-VIS | Visual Presentation |
| ALM-AUD | Audible Presentation |
| ALM-INT | Alarm Interaction |
| ALM-CFG | Alarm Configuration |
| ALM-LOG | Alarm Logging |
| ALM-IEC | IEC 60601-1-8 Compliance |

---

## 2. Alarm Philosophy

### 2.1 Guiding Principles

#### ALM-PHI-001: Purpose of Alarms
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Alarms exist to protect patient safety by alerting clinical staff to conditions requiring attention |

**Core Objectives:**
1. Alert staff to patient deterioration requiring intervention
2. Alert staff to equipment issues affecting monitoring capability
3. Minimize unnecessary alarms that contribute to alarm fatigue
4. Ensure alarms are perceptible in the clinical environment

#### ALM-PHI-002: Alarm Prioritization Philosophy
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Alarms shall be prioritized based on clinical urgency |

**Prioritization Framework:**

| Priority | Clinical Meaning | Response Expectation |
|----------|------------------|---------------------|
| **High** | Immediate patient danger; requires urgent intervention | Immediate response (seconds) |
| **Medium** | Clinically significant; requires prompt attention | Prompt response (minutes) |
| **Low** | Advisory; awareness needed but not urgent | Timely response (reasonable time) |

#### ALM-PHI-003: Alarm Fatigue Mitigation
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | The alarm system shall incorporate features to reduce alarm fatigue |

**Mitigation Strategies:**
- Appropriate default alarm limits (not overly sensitive)
- Short alarm delays to filter transient events
- Intelligent artifact rejection
- Patient-specific alarm limit customization
- Clear distinction between priority levels
- Alarm statistics for identifying problem areas

#### ALM-PHI-004: Fail-Safe Behavior
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | The alarm system shall fail safely |

**Fail-Safe Principles:**
- Alarm capability cannot be completely disabled
- System failures generate technical alarms
- Loss of monitoring capability is alarmed
- Audio cannot be permanently muted without indication
- Default to more conservative (alarming) state when uncertain

---

## 3. Alarm Generation

### 3.1 Physiological Alarms

#### ALM-GEN-001: Heart Rate Alarms
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Heart rate alarm conditions |

| Condition | Default Threshold | Priority | Message |
|-----------|-------------------|----------|---------|
| HR Very Low | < 40 bpm | High | "HR Very Low" |
| HR Low | < 50 bpm | Medium | "HR Low" |
| HR High | > 120 bpm | Medium | "HR High" |
| HR Very High | > 150 bpm | High | "HR Very High" |
| Asystole | No beat > 4 seconds | High | "Asystole" |

**Alarm Delay:** 10 seconds (configurable 5-30 seconds)
**Latching:** No (clears when condition resolves)

#### ALM-GEN-002: SpO2 Alarms
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Oxygen saturation alarm conditions |

| Condition | Default Threshold | Priority | Message |
|-----------|-------------------|----------|---------|
| SpO2 Very Low | < 85% | High | "SpO2 Very Low" |
| SpO2 Low | < 90% | Medium | "SpO2 Low" |
| Desaturation | > 4% drop in 60 seconds | Medium | "SpO2 Desaturation" |

**Alarm Delay:** 10 seconds (configurable 5-30 seconds)
**Latching:** No

#### ALM-GEN-003: Blood Pressure Alarms
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | NIBP alarm conditions |

| Condition | Default Threshold | Priority | Message |
|-----------|-------------------|----------|---------|
| Systolic Very Low | < 80 mmHg | High | "SYS Very Low" |
| Systolic Low | < 90 mmHg | Medium | "SYS Low" |
| Systolic High | > 160 mmHg | Medium | "SYS High" |
| Systolic Very High | > 180 mmHg | High | "SYS Very High" |
| Diastolic Very Low | < 40 mmHg | High | "DIA Very Low" |
| Diastolic Low | < 50 mmHg | Medium | "DIA Low" |
| Diastolic High | > 90 mmHg | Medium | "DIA High" |
| Diastolic Very High | > 100 mmHg | High | "DIA Very High" |
| MAP Very Low | < 60 mmHg | High | "MAP Very Low" |
| MAP Very High | > 110 mmHg | High | "MAP Very High" |

**Alarm Delay:** Immediate on measurement completion
**Latching:** No

#### ALM-GEN-004: Temperature Alarms
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Body temperature alarm conditions |

| Condition | Default Threshold | Priority | Message |
|-----------|-------------------|----------|---------|
| Temp Very Low (Hypothermia) | < 35.0°C | High | "Temp Very Low" |
| Temp Low | < 36.0°C | Low | "Temp Low" |
| Temp High (Fever) | > 38.0°C | Low | "Temp High" |
| Temp Very High (Hyperthermia) | > 39.0°C | Medium | "Temp Very High" |
| Temp Critical | > 40.5°C | High | "Temp Critical" |

**Alarm Delay:** 30 seconds (to filter probe adjustment)
**Latching:** No

#### ALM-GEN-005: Respiratory Rate Alarms
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Respiratory rate alarm conditions |

| Condition | Default Threshold | Priority | Message |
|-----------|-------------------|----------|---------|
| Apnea | No breath > 30 seconds | High | "Apnea" |
| RR Very Low | < 8 /min | High | "RR Very Low" |
| RR Low | < 10 /min | Medium | "RR Low" |
| RR High | > 24 /min | Medium | "RR High" |
| RR Very High | > 30 /min | High | "RR Very High" |

**Alarm Delay:** 15 seconds (configurable)
**Latching:** Apnea alarm latches until acknowledged

### 3.2 Technical Alarms

#### ALM-GEN-010: Sensor Disconnection Alarms
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Sensor disconnection detection |

| Condition | Detection Time | Priority | Message |
|-----------|----------------|----------|---------|
| SpO2 sensor off | < 3 seconds | Medium | "SpO2 Sensor Off" |
| ECG leads off | < 3 seconds | Medium | "ECG Leads Off" |
| NIBP cuff disconnected | On measurement attempt | Low | "NIBP Cuff Off" |
| Temperature probe off | < 5 seconds | Low | "Temp Probe Off" |

#### ALM-GEN-011: Signal Quality Alarms
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Poor signal quality detection |

| Condition | Priority | Message |
|-----------|----------|---------|
| SpO2 low perfusion (PI < 0.4) | Low | "SpO2 Low Perfusion" |
| ECG artifact (sustained) | Low | "ECG Artifact" |
| Unable to obtain NIBP | Low | "NIBP Failed" |

#### ALM-GEN-012: System Alarms
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | System-level alarm conditions |

| Condition | Priority | Message |
|-----------|----------|---------|
| Battery low (< 30%) | Medium | "Battery Low" |
| Battery critical (< 10%) | High | "Battery Critical" |
| Storage nearly full (> 90%) | Low | "Storage Nearly Full" |
| Network disconnected | Low | "Network Disconnected" |
| System error | High | "System Error - Contact Support" |

### 3.3 Alarm Delay and Filtering

#### ALM-GEN-020: Alarm Delay Implementation
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Delays to reduce nuisance alarms |

**Delay Mechanism:**
- Condition must persist for delay duration before alarm activates
- Delay timer resets if condition clears
- Delay configurable within defined range
- Display "Pre-alarm" indicator during delay period (optional)

**Delay Limits by Parameter:**

| Parameter | Minimum Delay | Maximum Delay | Default |
|-----------|---------------|---------------|---------|
| HR | 5 seconds | 30 seconds | 10 seconds |
| SpO2 | 5 seconds | 30 seconds | 10 seconds |
| NIBP | 0 (immediate) | 0 (immediate) | 0 |
| Temp | 10 seconds | 60 seconds | 30 seconds |
| RR | 10 seconds | 60 seconds | 15 seconds |

#### ALM-GEN-021: Artifact Rejection
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Suppress alarms during artifact |

**Behavior:**
- Motion artifact detection suspends physiological alarms
- Maximum artifact suspension: 30 seconds
- Technical alarm generated if artifact persists beyond threshold
- Artifact rejection configurable (on/off per parameter)
- Artifact events logged even if alarm suppressed

---

## 4. Alarm Prioritization

### 4.1 Priority Levels

#### ALM-PRI-001: Three-Tier Priority System
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | All alarms classified into three priority levels |

**High Priority:**
- Life-threatening conditions
- Require immediate response
- Examples: asystole, severe hypoxia, critical hypotension

**Medium Priority:**
- Clinically significant conditions
- Require prompt attention
- Examples: moderate limit violations, sensor disconnection affecting monitoring

**Low Priority:**
- Advisory conditions
- Awareness needed, response at convenience
- Examples: minor limit violations, low battery, network issues

#### ALM-PRI-002: Priority Escalation
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Alarm priority may escalate based on condition persistence or severity |

**Escalation Rules:**
- Medium alarm unacknowledged > 2 minutes → Audio escalation (louder)
- Battery Low unaddressed + battery depleting → Battery Critical (High)
- Multiple medium alarms from different parameters → May indicate critical deterioration

#### ALM-PRI-003: Priority Override
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Clinicians may adjust alarm priority for specific patients |

**Constraints:**
- Cannot reduce High priority to less than Medium
- Override requires authentication
- Override documented in audit log
- Override expires at session end (configurable)

---

## 5. Visual Alarm Presentation

### 5.1 Visual Indicators

#### ALM-VIS-001: Color Coding (per IEC 60601-1-8)
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Visual alarm colors per international standard |

| Priority | Background Color | Text Color |
|----------|------------------|------------|
| High | Red | White/Black |
| Medium | Yellow | Black |
| Low | Cyan/Light Blue | Black |

**Note:** These colors shall be consistent across all alarm indications.

#### ALM-VIS-002: Flashing Rates (per IEC 60601-1-8)
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Visual flashing frequencies per standard |

| Priority | Flash Rate | Duty Cycle |
|----------|------------|------------|
| High | 1.4 - 2.8 Hz | 20-60% |
| Medium | 0.4 - 0.8 Hz | 20-60% |
| Low | Steady or slow flash (< 0.4 Hz) | N/A |

#### ALM-VIS-003: Alarm Bar Display
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Primary alarm display area |

**Alarm Bar Requirements:**
- Fixed position at top of screen
- Always visible on all screens
- Shows highest priority active alarm
- Background color indicates priority
- Flashes at appropriate rate
- Displays alarm message text
- Shows parameter value triggering alarm
- Includes acknowledge button

**Multiple Alarms:**
- Highest priority displayed prominently
- Alarm count indicator ("2 more alarms")
- Scrollable or expandable to see all active alarms

#### ALM-VIS-004: Parameter Box Indication
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Alarm indication in parameter display |

**Requirements:**
- Alarming parameter box border flashes with priority color
- Alarming value displayed in priority color
- Alarm limit exceeded indicated (↑ for high, ↓ for low)
- Persists until alarm clears or is acknowledged

#### ALM-VIS-005: Screen Border Indication
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | High priority alarm screen border indication |

**Requirements:**
- High priority alarms trigger screen border flash
- Ensures visibility even if looking at other screen area
- Red border for high priority only

### 5.2 Information Display

#### ALM-VIS-010: Alarm Message Content
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Information displayed with alarm |

**Required Elements:**
- Parameter name (e.g., "HR", "SpO2")
- Alarm condition (e.g., "High", "Low", "Sensor Off")
- Current value (e.g., "156 bpm")
- Threshold violated (e.g., "> 150")
- Time of alarm onset

**Optional Elements:**
- Trend direction (rising/falling)
- Patient identifier (in multi-patient mode)

#### ALM-VIS-011: Silenced Alarm Indication
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Visual indication when audio is silenced |

**Requirements:**
- Alarm silenced icon displayed prominently
- Remaining silence duration shown (countdown)
- Visual indication continues even when audio silenced
- Silenced status visible from distance

---

## 6. Audible Alarm Presentation

### 6.1 Alarm Sounds

#### ALM-AUD-001: Sound Characteristics (per IEC 60601-1-8)
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Audible alarm characteristics per standard |

**High Priority Sound:**
- Pattern: Burst of 5 pulses, repeated
- Pitch: Higher frequency (≥ 500 Hz fundamental)
- Rhythm: Rapid, urgent pattern
- Recognizability: Distinct, cannot be confused with other sounds

**Medium Priority Sound:**
- Pattern: 3 pulses, repeated
- Pitch: Medium frequency
- Rhythm: Moderate tempo
- Recognizability: Distinct from high priority

**Low Priority Sound:**
- Pattern: 1-2 pulses, repeated slowly
- Pitch: Lower frequency
- Rhythm: Slow, less urgent
- Recognizability: Distinct from medium priority

#### ALM-AUD-002: Volume Levels
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Audible alarm volume requirements |

**Volume Requirements:**
- Audible at 1 meter in typical ward environment
- Adjustable within defined range
- Minimum volume for high-priority alarms enforced
- Default volume appropriate for clinical environment

**Volume Range (at 1 meter):**

| Priority | Minimum | Default | Maximum |
|----------|---------|---------|---------|
| High | 55 dB | 70 dB | 85 dB |
| Medium | 45 dB | 60 dB | 80 dB |
| Low | 40 dB | 50 dB | 75 dB |

#### ALM-AUD-003: Sound Escalation
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Audio escalation for unacknowledged alarms |

**Behavior:**
- If alarm unacknowledged for configurable period (default 2 minutes)
- Volume increases incrementally
- Pattern may intensify
- Maximum volume reached after escalation period
- Escalation resets on acknowledgment

### 6.2 Muting and Pausing

#### ALM-AUD-010: Audio Pause Function
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Temporary audio silence |

**Pause Durations:**
- 1 minute, 2 minutes, 3 minutes (selectable)
- Single button press for default duration (e.g., 2 min)
- Extended pause requires additional confirmation

**Pause Behavior:**
- Silences audio only; visual alarms continue
- Countdown displayed on screen
- Auto-resumes at timeout
- New high-priority alarms may override pause (configurable)
- Pause event logged

#### ALM-AUD-011: Audio Disable Restrictions
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Restrictions on disabling audio |

**Requirements:**
- Audio cannot be permanently disabled
- Minimum volume for high-priority alarms cannot be set to zero
- Extended silence (> 3 minutes) requires re-confirmation
- Continuous silence not possible; auto-resumes
- Audio disabled state prominently indicated

---

## 7. Alarm Interaction

### 7.1 Acknowledgment

#### ALM-INT-001: Alarm Acknowledgment
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | User acknowledgment of alarms |

**Acknowledgment Behavior:**
- Single tap on acknowledge button
- Silences audio for current alarm
- Visual indication continues until condition resolves
- Re-arms if condition worsens or new alarm occurs
- Acknowledgment logged with user ID (if authenticated), timestamp

**Access Control:**
- Acknowledgment may require user authentication (configurable)
- If not authenticated, alarm still acknowledged but logged as "anonymous"

#### ALM-INT-002: Alarm Reset
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Alarm clearing behavior |

**Non-Latching Alarms:**
- Automatically clear when condition returns to normal
- Visual and audio indication stop
- Logged as resolved with timestamp

**Latching Alarms (e.g., Apnea):**
- Require explicit acknowledgment to clear
- Persist until manual reset even if condition resolves
- Prevents missing transient critical events

### 7.2 Alarm Pause

#### ALM-INT-010: Alarm Pause During Procedures
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Extended alarm suppression during procedures |

**Use Case:** Patient movement, position change, sensor reapplication

**Behavior:**
- "Procedure Pause" mode available
- Duration: Up to 5 minutes
- All physiological alarms paused
- Technical alarms continue
- Clear visual indication of paused state
- Auto-resume at timeout
- Requires authentication
- Logged with user ID and reason (optional)

### 7.3 Alarm Limits Adjustment

#### ALM-INT-020: Bedside Limit Adjustment
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Adjusting alarm limits at bedside |

**Access:**
- Available from Main Vitals screen (parameter tap → adjust limits)
- Alternatively from Alarms screen → Alarm Limits

**Interface:**
- Current limit displayed
- Slider and/or +/- buttons for adjustment
- Preview of new limit against current value
- Apply and Cancel buttons
- Limits bounded within safe range

**Constraints:**

| Parameter | Minimum Lower Limit | Maximum Upper Limit |
|-----------|--------------------|--------------------|
| HR | 20 bpm | 250 bpm |
| SpO2 | 70% | 99% |
| Systolic | 60 mmHg | 250 mmHg |
| Diastolic | 30 mmHg | 150 mmHg |
| Temp | 30°C | 42°C |
| RR | 4 /min | 60 /min |

#### ALM-INT-021: Alarm Presets
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Pre-defined alarm limit sets |

**Available Presets:**
- Default Adult
- Post-Surgical
- COPD Patient (lower SpO2 threshold)
- Elderly (adjusted limits)
- Custom (hospital-defined)

**Preset Application:**
- Select preset from Alarm Limits screen
- Preview limits before applying
- Apply to current patient session
- Logged as limit change with preset name

---

## 8. Alarm Configuration

### 8.1 Default Configuration

#### ALM-CFG-001: Factory Default Limits
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Out-of-box alarm configuration |

**Factory Defaults:**

| Parameter | Low-Low | Low | High | High-High |
|-----------|---------|-----|------|-----------|
| HR | 40 | 50 | 120 | 150 |
| SpO2 | 85 | 90 | OFF | OFF |
| SYS | 80 | 90 | 160 | 180 |
| DIA | 40 | 50 | 90 | 100 |
| Temp | 35.0 | 36.0 | 38.0 | 39.0 |
| RR | 8 | 10 | 24 | 30 |

#### ALM-CFG-002: Hospital Default Configuration
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Administrator-defined default limits |

**Capabilities:**
- Administrator can modify default limits
- Defaults apply to new patient sessions
- Per-parameter enable/disable
- Delay configuration
- Volume configuration
- Stored persistently

### 8.2 Individual Alarm Control

#### ALM-CFG-010: Per-Parameter Alarm Enable
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Enable/disable alarms per parameter |

**Constraints:**
- High-priority thresholds cannot be disabled
- Disabling requires authentication
- Disabled state indicated on parameter display
- Logged in audit trail

#### ALM-CFG-011: Per-Limit Enable
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Enable/disable individual limits |

**Example:** Disable HR Low alarm but keep HR High active

**Constraints:**
- At least one limit must remain active per parameter
- Changes logged

---

## 9. Alarm Logging

### 9.1 Event Logging

#### ALM-LOG-001: Alarm Event Record
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Information logged for each alarm event |

**Logged Data:**
- Timestamp (onset)
- Parameter
- Alarm condition
- Priority level
- Triggering value
- Alarm limit at time of alarm
- Patient identifier
- Alarm outcome (acknowledged, resolved, escalated)
- Acknowledgment timestamp (if acknowledged)
- Acknowledging user ID (if authenticated)
- Resolution timestamp

#### ALM-LOG-002: Alarm Configuration Change Log
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Logging alarm configuration changes |

**Logged Data:**
- Timestamp
- User ID
- Parameter affected
- Previous value
- New value
- Reason (if provided)

### 9.2 Alarm History

#### ALM-LOG-010: Alarm History Display
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Viewing alarm history |

**Features:**
- Chronological list of alarms
- Filter by parameter, priority, time range
- Sort by time, priority, parameter
- 72 hours minimum history
- Export capability (to EHR)

#### ALM-LOG-011: Alarm Statistics
| Attribute | Specification |
|-----------|---------------|
| Priority | P3 |
| Description | Alarm frequency statistics |

**Displayed Statistics:**
- Total alarms by priority (last 24h, session)
- Alarms by parameter
- Average time to acknowledgment
- Repeated alarms (same condition recurring)

**Purpose:** Identify alarm fatigue risk, inappropriate limits

---

## 10. IEC 60601-1-8 Compliance

### 10.1 Standard Requirements

#### ALM-IEC-001: Alarm System Documentation
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Documentation per IEC 60601-1-8 |

**Required Documentation:**
- Alarm condition list with priorities
- Alarm signal characteristics (visual and audible)
- Alarm generation method and rationale
- Alarm system testing procedures
- Instructions for use regarding alarms

#### ALM-IEC-002: Alarm Signal Compliance
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Alarm signals per IEC 60601-1-8 |

**Requirements:**
- Three-priority system implemented
- Visual characteristics per standard (colors, flash rates)
- Audible characteristics per standard (patterns, frequencies)
- Information signals distinguished from alarm signals

#### ALM-IEC-003: Alarm System Integrity
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Alarm system reliability |

**Requirements:**
- Alarm generation shall not be bypassed by single fault
- Alarm indication shall not be bypassed by single fault
- Self-test of alarm capability
- Failure of alarm system generates alarm

#### ALM-IEC-004: Distributed Alarm System
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Requirements if alarms transmitted remotely |

**If Remote Notification Implemented:**
- Transmission integrity verified
- Delay in transmission alarmed if exceeds threshold
- Loss of connection alarmed
- Remote acknowledgment traceable

---

## 11. Risk Considerations

### 11.1 Alarm-Related Hazards

| Hazard | Mitigation |
|--------|------------|
| Missed alarm due to audio disabled | Minimum volume enforced, visual always present |
| Alarm fatigue leading to ignored alarms | Appropriate defaults, customization, artifact rejection |
| Incorrect alarm limits | Bounded ranges, preset options, clinical validation |
| Alarm system failure | Self-test, watchdog, failure alarms |
| Alarm not heard in noisy environment | Volume adjustment, escalation, remote notification |
| Alarm condition persists after acknowledgment | Visual indication continues, re-alarm if worsens |

### 11.2 Safety Classification

Per IEC 62304, alarm-related software functions are likely **Safety Class B or C** depending on risk analysis outcome. Design and verification shall comply with appropriate safety class requirements.

---

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2024 | — | Initial draft |
