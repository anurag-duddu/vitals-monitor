# Requirements Clarifications: Bedside Vitals Monitor

## Document Information

| Field | Value |
|-------|-------|
| Document ID | REQ-000 |
| Version | 1.0 |
| Status | Approved |
| Date | 2024 |
| Purpose | Captures clarifications and decisions made during requirements review |

---

## 1. Hardware & Sensors

### 1.1 Sensor Module Strategy

| Decision | Details |
|----------|---------|
| **SpO2/Pulse Ox** | Medical-grade OEM module (not development boards like MAX30102) |
| **ECG** | Medical-grade OEM module with analog front-end |
| **NIBP** | OEM oscillometric module (e.g., SunTech, Welch Allyn OEM) |
| **Temperature** | To be determined (likely medical-grade IR or probe) |

**Rationale:** Using pre-certified OEM modules reduces regulatory burden and ensures medical-grade accuracy out of the box. Development modules like MAX30102 are not suitable for production medical devices.

**Action Items:**
- [ ] Identify and evaluate OEM SpO2 module vendors (Nonin, Masimo, etc.)
- [ ] Identify and evaluate OEM ECG module vendors
- [ ] Identify and evaluate OEM NIBP module vendors
- [ ] Create sensor module evaluation matrix

### 1.2 Target Hardware Platform

| Decision | Details |
|----------|---------|
| **Development Board** | STM32MP157F-DK2 (ST Discovery Kit) |
| **Production Board** | Same (DK2 is production target for V1.0) |
| **Display** | Adaptive design supporting multiple screen sizes |
| **Cortex-M4 Usage** | Required for V1.0 (ECG sampling with <1ms jitter) |

**Display Sizes to Support:**
- 4.3" 480x272 (DK2 native)
- 7" 800x480 (common for bedside monitors)
- 10" 1024x600 (optional, for larger installations)

**Additional Hardware Requirements Identified:**
- Ambient light sensor (for automatic night mode)
- Dry contact relay output (for nurse call system)
- 13.56 MHz NFC/RFID reader (for badge authentication)
- Impedance measurement circuit (for ECG-derived respiration)

---

## 2. V1.0 Feature Scope

### 2.1 In Scope for V1.0

| Feature | Status | Notes |
|---------|--------|-------|
| SpO2 monitoring | In scope | Via OEM module |
| ECG monitoring (3-lead) | In scope | Via OEM module, M4 sampling |
| Heart rate (ECG-derived) | In scope | |
| NIBP | In scope | Via OEM module |
| Temperature | In scope | |
| Respiration rate | In scope | ECG impedance + pleth fallback |
| Dual-patient mode | In scope | Split-screen UI |
| Alarm system | In scope | Configurable thresholds |
| Nurse call relay | In scope | Dry contact output |
| Trend storage (72h) | In scope | Per-patient + session |
| Waveform display | In scope | ECG + Pleth |
| PIN authentication | In scope | |
| RFID/badge authentication | In scope | Any 13.56 MHz card |
| Night mode (auto) | In scope | Ambient light sensor |
| Audit logging | In scope | |
| USB update | In scope | Signed firmware packages |

### 2.2 Deferred to V2.0+

| Feature | Reason for Deferral |
|---------|---------------------|
| EHR/FHIR integration | Simplify V1.0, offline-first |
| ABDM/ABHA integration | Simplify V1.0, India-specific |
| OTA network updates | USB sufficient for V1.0 |
| Remote monitoring | Offline-first approach |
| Multi-parameter trending (>5 parameters) | Prioritize core vitals |
| Advanced arrhythmia detection | Requires clinical validation |
| 12-lead ECG | Hardware limitation of DK2 |

---

## 3. Alarm System Clarifications

### 3.1 Alarm Acknowledgment Behavior

**Decision:** Configurable per-alarm type

| Alarm Category | Re-alarm Behavior |
|----------------|-------------------|
| Life-threatening (e.g., asystole, severe bradycardia) | Re-alarm after 2 minutes if condition persists |
| High priority (e.g., SpO2 <85%, HR >150) | Re-alarm after 3 minutes if condition persists |
| Medium priority | Stay acknowledged until condition resolves then recurs |
| Low priority (technical) | Stay acknowledged until condition resolves |

**Configuration:** Administrator can modify re-alarm timeouts per alarm type.

### 3.2 Nurse Call Integration

**Decision:** Dry contact relay output

| Specification | Value |
|---------------|-------|
| Output type | Form C relay (NO + NC contacts) |
| Voltage rating | 30V DC / 0.5A max |
| Activation | On any high-priority alarm |
| Deactivation | When all high-priority alarms cleared or acknowledged |

### 3.3 Respiration Rate Derivation

**Decision:** Dual-method with automatic fallback

| Priority | Method | Trigger |
|----------|--------|---------|
| Primary | ECG impedance (thoracic) | When ECG electrodes connected |
| Fallback | Plethysmograph amplitude modulation | When ECG unavailable or poor signal |

---

## 4. Data Storage Clarifications

### 4.1 Retention Policy

**Decision:** 72-hour retention + active session preservation

| Patient Status | Retention Rule |
|----------------|----------------|
| Active (currently monitored) | Keep full session regardless of duration |
| Recently discharged (<72h) | Keep full session |
| Discharged (>72h ago) | Purge oldest data to maintain 72h window |

### 4.2 Waveform Capture Configuration

**Decision:** Configurable per vital sign parameter

| Parameter | Default Capture Mode | Options |
|-----------|---------------------|---------|
| ECG | Continuous (rolling 30 min buffer) | Continuous, Event-only, Off |
| Pleth (SpO2) | Event-triggered (alarms + manual) | Continuous, Event-only, Off |

**Event Triggers:**
- Any alarm activation (capture 30s before + 60s after)
- Manual "Mark Event" button press
- Desaturation event (>4% drop in 60s)
- Arrhythmia detection

### 4.3 Storage Full Policy

**Decision:** Priority-based purging (non-alarm data first)

**Purge Priority (lowest to highest priority to keep):**
1. Trend data >48h old with no associated alarms
2. Trend data >24h old with no associated alarms
3. Waveform snapshots >24h old with low-priority alarms
4. Trend data with associated alarms (keep longest)
5. Active session data (never purge automatically)

---

## 5. UI/UX Clarifications

### 5.1 Display Adaptation

**Decision:** Responsive design supporting multiple screen sizes

| Screen Size | Layout Approach |
|-------------|-----------------|
| 4.3" (480x272) | Compact layout, tabbed sections, smaller fonts |
| 7" (800x480) | Standard layout, all vitals visible |
| 10" (1024x600) | Expanded layout, larger waveforms |

**Implementation:** Use LVGL flex containers with percentage-based sizing.

### 5.2 Dual-Patient Mode UI

**Decision:** Split-screen layout

```
┌─────────────────────────────────────────────────┐
│ [Patient A: Bed 12A - John Doe]                 │
│ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ │
│ │ HR: 72  │ │SpO2: 98%│ │ RR: 16  │ │ T: 37.1 │ │
│ └─────────┘ └─────────┘ └─────────┘ └─────────┘ │
│ [ECG Waveform ~~~~~~~~~~~~~~~~~~~~~~~~~~~~]     │
├─────────────────────────────────────────────────┤
│ [Patient B: Bed 12B - Jane Smith]               │
│ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ │
│ │ HR: 68  │ │SpO2: 99%│ │ RR: 14  │ │ T: 36.8 │ │
│ └─────────┘ └─────────┘ └─────────┘ └─────────┘ │
│ [Pleth Waveform ~~~~~~~~~~~~~~~~~~~~~~~~~~]     │
└─────────────────────────────────────────────────┘
```

**Touch Interaction:**
- Tap on patient section to expand to full screen
- Swipe up/down to scroll within section
- Long-press for patient menu (trends, settings, etc.)

### 5.3 Night Mode

**Decision:** Automatic via ambient light sensor

| Lighting Condition | Mode | Trigger Threshold |
|--------------------|------|-------------------|
| Normal (>100 lux) | Day mode | - |
| Low (<50 lux for >30s) | Night mode | Auto-enable |
| Very low (<10 lux for >30s) | Night mode + dimmed | Auto-enable |

**Override:** Manual toggle always available in quick settings.

---

## 6. Security Clarifications

### 6.1 RFID/Badge Authentication

**Decision:** Universal 13.56 MHz card passthrough

| Specification | Value |
|---------------|-------|
| Frequency | 13.56 MHz (ISO 14443A/B, ISO 15693) |
| Card types | MIFARE Classic, DESFire, hospital ID cards |
| Enrollment | Admin scans card, maps UID to user account |
| Security | Card UID only (no card data read) |

**Flow:**
1. User taps any 13.56 MHz card
2. Device reads card UID
3. Device looks up UID in local user database
4. If found: authenticate user
5. If not found: display "Unregistered card" error

### 6.2 Service Role Access

**Decision:** Time-limited PIN (TOTP-like)

| Specification | Value |
|---------------|-------|
| PIN format | 8 digits |
| Validity | 24 hours from generation |
| Generation | Manufacturer portal or phone app |
| Audit | All service actions logged with PIN ID |

**Flow:**
1. Service technician requests PIN from manufacturer portal
2. Portal generates PIN linked to device serial + time window
3. Technician enters PIN on device
4. Device validates using shared secret + time
5. Service mode enabled for 24 hours or until logout

### 6.3 Emergency Access

**Decision:** Configurable by hospital administrator

| Setting | Options |
|---------|---------|
| Emergency access enabled | Yes / No |
| Access level | View-only / Full access |
| Timeout | 2 min / 5 min / 10 min |
| Justification required | Yes (post-access) / No |

**When enabled:**
1. Long-press emergency button (3 seconds)
2. Confirm emergency access prompt
3. Full/limited access granted for configured timeout
4. Heavy audit trail created
5. If justification required: prompt for reason after timeout

---

## 7. Regulatory Clarifications

### 7.1 Timeline

**Decision:** 6-12 month target for CDSCO submission

| Milestone | Target |
|-----------|--------|
| MVP feature complete | Month 4 |
| Internal testing complete | Month 6 |
| IEC 60601 testing start | Month 7 |
| Clinical validation (TBD) | Month 8-10 |
| CDSCO submission | Month 10-12 |

**Risk:** Aggressive timeline requires:
- Clear scope boundaries (no scope creep)
- Parallel regulatory documentation
- Pre-identified test labs
- Early clinical site engagement

### 7.2 Software Safety Classification

**Decision:** IEC 62304 Class B confirmed

**Rationale:**
- General ward use (not ICU/life-critical)
- Human oversight always present (nurses)
- Device is adjunct, not sole basis for treatment
- Failure modes could delay intervention but not directly cause harm

**Documentation Requirements (Class B):**
- Software Development Plan
- Software Requirements Specification
- Software Architecture Document
- Verification (unit + integration testing)
- Risk Management File
- Configuration Management

### 7.3 Clinical Validation

**Decision:** Undecided - requires further analysis

**Options to evaluate:**
1. Bench testing against reference monitors only
2. Bench testing + small pilot (10-20 patients)
3. Formal clinical study (IRB required)

**Action:** Consult with regulatory expert before Month 3.

---

## 8. Architecture Clarifications

### 8.1 Programming Languages

**Decision:** C for critical components, Python for non-critical services

| Component | Language | Rationale |
|-----------|----------|-----------|
| UI (LVGL) | C99 | LVGL native language |
| Sensor service | C99 | Real-time requirements |
| Alarm engine | C99 | Safety-critical |
| M4 firmware | C99 | Bare-metal embedded |
| Network service | Python 3 | Rapid development, non-critical |
| Audit service | Python 3 or C | TBD based on performance |

### 8.2 IPC Architecture

**Decision:** Hybrid (nanomsg + D-Bus)

| Data Type | IPC Mechanism | Pattern |
|-----------|---------------|---------|
| Sensor data (vitals, waveforms) | nanomsg | Pub/Sub |
| Alarm events | nanomsg | Pub/Sub |
| Configuration changes | D-Bus | Request/Reply |
| User authentication | D-Bus | Request/Reply |
| Service control (start/stop) | D-Bus | Request/Reply |

### 8.3 Cortex-M4 Architecture

**Decision:** Required for V1.0 ECG sampling

| Responsibility | Processor |
|----------------|-----------|
| ECG ADC sampling (500 Hz) | Cortex-M4 |
| ECG signal conditioning | Cortex-M4 |
| R-peak detection | Cortex-M4 |
| Data transfer to A7 | RPMsg over shared memory |
| All other processing | Cortex-A7 |

**Communication:**
- RPMsg for structured messages (heart rate, R-R intervals)
- Shared memory ring buffer for raw waveform samples

---

## 9. Development Team Context

**Team Size:** Small (2-4 developers)

**Implications:**
- Prioritize simplicity over flexibility
- Use proven libraries over custom implementations
- Limit feature scope to essentials
- Automate testing and CI/CD
- Document as we go (not after)

---

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2024 | — | Initial clarifications from requirements review |
