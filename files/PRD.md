# Product Requirements Document: Bedside Vitals Monitor

## Executive Summary

A bedside vitals monitor for Indian hospital general wards, built on embedded Linux (STM32MP157F) with LVGL UI framework. The device provides continuous vital signs monitoring with waveforms, alarms, and bidirectional EHR/ABDM integration.

**Target Market:** India, general ward / med-surg inpatient settings
**Regulatory Pathway:** CDSCO Class B medical device
**Hardware Platform:** STM32MP157F-DK2 (Cortex-A7 + M4, 512MB RAM)
**Software Stack:** Buildroot Linux + LVGL (MIT license)

---

## Product Vision

Provide Indian hospitals with a modern, connected bedside vitals monitor that:
- Improves patient safety through reliable monitoring and alarms
- Enhances nursing efficiency with intuitive touch UI
- Integrates seamlessly with India's ABDM digital health infrastructure
- Meets regulatory requirements for medical devices

---

## Users

| User | Role | Primary Tasks |
|------|------|---------------|
| **Staff Nurse** | Primary | Vital checks, alarm response, patient association |
| **Physician** | Secondary | Review trends, clinical decisions |
| **Biomedical Engineer** | Tertiary | Configuration, maintenance, network setup |

---

## Core Features

### 1. Vital Signs Monitoring

| Parameter | Type | Display |
|-----------|------|---------|
| Heart Rate (HR) | Continuous | Numeric + waveform source |
| SpO2 | Continuous | Numeric + pleth waveform |
| ECG | Continuous | Waveform |
| NIBP | On-demand | Systolic/Diastolic/MAP |
| Temperature | Spot/Continuous | Numeric |
| Respiration Rate | Derived | Numeric |

### 2. Waveform Display

- ECG waveform (250-500 Hz sampling)
- Plethysmograph waveform (100 Hz sampling)
- Configurable sweep speed, gain, gridlines
- Real-time rendering at 30 FPS minimum

### 3. Alarm System

| Priority | Examples | Visual | Audio |
|----------|----------|--------|-------|
| High | SpO2 < 85%, HR < 30 | Red flashing | Continuous tone |
| Medium | SpO2 < 90%, HR > 120 | Yellow flashing | Intermittent tone |
| Low | Sensor disconnect | Cyan indicator | Single tone |

- Configurable thresholds per patient
- Acknowledgment requires authentication
- Escalation for unacknowledged alarms
- Alarm history logging

### 4. Patient Management

- Support 1-2 patients per device (configurable)
- Patient identification: manual entry, barcode, RFID, ABHA ID
- Demographics import from EHR
- Clear association/disassociation workflow

### 5. Data Storage

- 72-hour minimum on-device storage (offline operation)
- Trend visualization (1hr, 4hr, 8hr, 24hr, 72hr windows)
- Automatic sync when network available
- SQLite database with encryption at rest

### 6. Connectivity & Integration

**EHR Integration (Bidirectional):**
- Import: Patient demographics, allergies, alarm presets
- Export: Vitals, alarms, device status
- Protocol: HL7 FHIR R4

**ABDM Integration:**
- ABHA ID lookup and verification
- Health record push (consent-based)
- NHA certification required

**Network:**
- WiFi (802.11ac, WPA2/WPA3-Enterprise)
- Ethernet (Gigabit)
- Offline-first architecture

### 7. Security

- User authentication: PIN + Badge/RFID
- Role-based access control (Clinical, Biomedical, Admin, Service)
- Comprehensive audit logging
- Secure boot chain with dm-verity
- Encrypted data partition (LUKS)
- Tamper-resistant kiosk mode

---

## Technical Architecture

### Hardware Platform

**STM32MP157F-DK2:**
- Dual Cortex-A7 @ 800 MHz (Linux)
- Cortex-M4 @ 209 MHz (Real-time sensor sampling)
- 512 MB DDR3L RAM
- microSD storage
- 4" 800×480 MIPI-DSI touchscreen
- WiFi/Bluetooth module
- Gigabit Ethernet

### Software Stack

```
┌─────────────────────────────────────────────────────────────┐
│                    LVGL UI Application                       │
│              (Screens, Widgets, Waveforms)                   │
├─────────────────────────────────────────────────────────────┤
│                   Application Services                       │
│   sensor-service │ alarm-service │ network-service │ audit   │
├─────────────────────────────────────────────────────────────┤
│                   Platform / Drivers                         │
│     I2C/SPI sensors │ Display │ SQLite │ Network stack      │
├─────────────────────────────────────────────────────────────┤
│                   Embedded Linux                             │
│                 Buildroot + Linux Kernel                     │
├─────────────────────────────────────────────────────────────┤
│                      Hardware                                │
│                    STM32MP157F SoC                           │
└─────────────────────────────────────────────────────────────┘
```

### Process Architecture

| Process | Function | Criticality |
|---------|----------|-------------|
| sensor-service | Read sensors, publish data | Critical |
| alarm-service | Evaluate alarms, generate alerts | Critical |
| ui-app | Display and user interaction | High |
| network-service | EHR/ABDM sync | Medium |
| audit-service | Event logging | High |
| watchdog-service | Health monitoring | Critical |

### IPC

- **nanomsg** for high-frequency sensor data (pub/sub)
- **D-Bus** for control messages, configuration

### Data Storage

- **SQLite** for structured data (trends, config, audit)
- **Ring buffer** for waveform display (memory)
- **Encrypted partition** for all persistent data

---

## UI/UX Requirements

### Screen Hierarchy

```
Main Vitals Screen (default)
├── Trends Screen
├── Alarm Limits Screen
├── Patient Screen
│   ├── Associate Patient
│   └── Patient Details
├── Settings Screen (auth required)
│   ├── Display Settings
│   ├── Alarm Settings
│   ├── Network Settings
│   └── System Info
└── Admin Screen (admin auth required)
    ├── User Management
    ├── Device Configuration
    └── Audit Logs
```

### Main Vitals Screen Layout

```
┌─────────────────────────────────────────────────────────────────┐
│ [Patient Name/Bed]                    [Time] [WiFi] [Battery]   │
├─────────────────────────────────────────────────────────────────┤
│ ████████████████ ALARM: HIGH HR ████████████████████████████████│
├───────────────────────────────────────────┬─────────────────────┤
│                                           │                     │
│  ~~~ECG Waveform~~~~~~~~~~~~~~~~~~~~~~    │    HR      120     │
│                                           │    /min    ▲ HIGH  │
│                                           ├─────────────────────┤
│  ~~~Pleth Waveform~~~~~~~~~~~~~~~~~~~~    │    SpO2     98     │
│                                           │     %              │
│                                           ├─────────────────────┤
│                                           │    NIBP   120/80   │
│                                           │    mmHg    (92)    │
├───────────────────────────────────────────┼─────────────────────┤
│                                           │    Temp   36.8     │
│                                           │     °C             │
│                                           ├─────────────────────┤
│                                           │    RR       16     │
│                                           │    /min            │
├───────────────────────────────────────────┴─────────────────────┤
│ [Trends]  [Alarms]  [Patient]  [NIBP Start]  [Settings]         │
└─────────────────────────────────────────────────────────────────┘
```

### Design Principles

- Touch-optimized (minimum 44×44 px targets)
- Glove-compatible interaction
- High contrast for visibility
- Consistent color coding (green=normal, yellow=warning, red=critical)
- Responsive layout (support multiple screen sizes)

---

## Regulatory Requirements

### Classification

- **CDSCO Class B** (low-moderate risk)
- **IEC 62304 Software Safety Class B**

### Required Standards

| Standard | Scope |
|----------|-------|
| IEC 60601-1 | Electrical safety |
| IEC 60601-1-2 | EMC |
| IEC 60601-2-49 | Multiparameter monitors |
| IEC 62304 | Software lifecycle |
| ISO 14971 | Risk management |
| IEC 62366-1 | Usability engineering |
| ISO 13485 | Quality management system |

### Documentation Deliverables

- Device Master File (DMF)
- Risk Management File
- Software Development Plan
- Software Requirements Specification
- Software Architecture Document
- Software Test Reports
- Usability Engineering File
- Clinical Evidence Summary

---

## Development Approach

### Technology Choices

| Component | Choice | Rationale |
|-----------|--------|-----------|
| OS | Buildroot Linux | Simpler than Yocto, sufficient for scope |
| UI Framework | LVGL | MIT license, embedded-optimized, medical precedent |
| Language | C (primary) | LVGL native, embedded standard |
| Database | SQLite | Reliable, embedded-friendly |
| IPC | nanomsg + D-Bus | Performance + standard Linux |
| Security | dm-verity, LUKS, secure boot | Industry standard |

### Development Environment

| Environment | Purpose |
|-------------|---------|
| Mac + SDL simulator | UI development, fast iteration |
| Docker + Buildroot | Cross-compilation, image generation |
| STM32MP157F-DK2 | Hardware integration testing |

### Build Outputs

| Output | Description |
|--------|-------------|
| Simulator binary | Mac-native for development |
| rootfs.squashfs | Read-only root filesystem |
| update.swu | Signed update package |
| sdcard.img | Complete SD card image |

---

## Success Criteria

### Clinical

| Metric | Target |
|--------|--------|
| Vital signs accuracy | Per IEC 60601-2-49 |
| Time to first reading | < 30 seconds |
| Alarm response time | < 10 seconds |
| False alarm rate | < 5% |

### Usability

| Metric | Target |
|--------|--------|
| Spot-check completion | < 60 seconds |
| Training time | < 30 minutes |
| Task completion (new users) | > 90% |

### Technical

| Metric | Target |
|--------|--------|
| System uptime | > 99.9% |
| Boot time | < 30 seconds |
| UI frame rate | ≥ 30 FPS |
| Offline operation | 72+ hours |

---

## Constraints

1. **Open source only** — No GPL in application code (LGPL/MIT acceptable)
2. **India market first** — CDSCO pathway, ABDM integration
3. **Hardware fixed** — STM32MP157F-DK2 development platform
4. **Offline-first** — Full functionality without network
5. **Medical device rigor** — IEC 62304, ISO 14971 compliance

---

## Out of Scope (V1.0)

- ICU-level monitoring
- Invasive blood pressure
- Capnography (ETCO2)
- Central monitoring station
- Mobile app companion
- International markets

---

## Reference Documents

| Document | ID | Description |
|----------|-----|-------------|
| Product Overview | REQ-001 | Vision, personas, scope |
| Functional Requirements | REQ-002 | Feature specifications |
| UI/UX Requirements | REQ-003 | Screens, layouts, interaction |
| Alarm Requirements | REQ-004 | Alarm philosophy, behavior |
| Data Storage Requirements | REQ-005 | Storage, sync, encryption |
| Integration Requirements | REQ-006 | EHR, ABDM, network |
| Security Requirements | REQ-007 | Auth, audit, tamper-proofing |
| Regulatory Requirements | REQ-008 | CDSCO, IEC 62304, standards |
| Architecture Requirements | REQ-009 | Services, IPC, build system |
| Development Environment | REQ-010 | Mac setup, Docker, workflow |
