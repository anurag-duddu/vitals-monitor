# Implementation Roadmap: Bedside Vitals Monitor

## Overview

This document provides a phased implementation plan for building the bedside vitals monitor. Each phase has clear deliverables and can be executed incrementally.

**Total Estimated Timeline:** 6-9 months to MVP

---

## Phase 0: Development Environment Setup
**Duration:** 1 week

### Goals
- Mac development environment ready
- LVGL simulator running
- Docker build environment ready
- Project repository initialized

### Tasks

| Task | Description | Done |
|------|-------------|------|
| 0.1 | Install Homebrew, SDL2, CMake, Docker | ☐ |
| 0.2 | Create project repository with structure | ☐ |
| 0.3 | Add LVGL and lv_drivers as submodules | ☐ |
| 0.4 | Create simulator CMakeLists.txt | ☐ |
| 0.5 | Build and run blank LVGL window | ☐ |
| 0.6 | Create Dockerfile for Buildroot | ☐ |
| 0.7 | Verify Docker build environment works | ☐ |

### Deliverables
- [ ] Simulator displays blank screen
- [ ] Docker container builds successfully
- [ ] README with setup instructions

---

## Phase 1: Basic UI Framework
**Duration:** 2-3 weeks

### Goals
- Main screen layout implemented
- Basic widgets created
- Navigation framework in place
- Mock data displaying

### Tasks

| Task | Description | Done |
|------|-------------|------|
| 1.1 | Create LVGL configuration (lv_conf.h) | ☐ |
| 1.2 | Implement screen manager (stack-based navigation) | ☐ |
| 1.3 | Create main vitals screen layout | ☐ |
| 1.4 | Create numeric display widget | ☐ |
| 1.5 | Create alarm banner widget | ☐ |
| 1.6 | Create navigation bar widget | ☐ |
| 1.7 | Implement mock sensor data generator | ☐ |
| 1.8 | Connect mock data to UI updates | ☐ |
| 1.9 | Create color theme (normal/warning/critical) | ☐ |

### Deliverables
- [ ] Main screen shows HR, SpO2, Temp, RR, NIBP
- [ ] Values update with mock data
- [ ] Navigation bar functional
- [ ] Basic color coding works

### Screen Mockup (ASCII)
```
┌─────────────────────────────────────────┐
│ [No Patient]           12:34  [WiFi]    │
├─────────────────────────────────────────┤
│                         │               │
│   (waveform area)       │   HR    72    │
│                         │   bpm         │
│                         ├───────────────┤
│                         │   SpO2  98    │
│   (waveform area)       │   %           │
│                         ├───────────────┤
│                         │   NIBP 120/80 │
│                         │   mmHg  (92)  │
├─────────────────────────┼───────────────┤
│                         │   Temp  36.8  │
│                         │   °C          │
│                         ├───────────────┤
│                         │   RR    16    │
│                         │   /min        │
├─────────────────────────┴───────────────┤
│ [Trends] [Alarms] [Patient] [Settings]  │
└─────────────────────────────────────────┘
```

---

## Phase 2: Waveform Display
**Duration:** 2-3 weeks

### Goals
- Real-time waveform rendering
- ECG and Pleth waveforms
- Configurable display parameters

### Tasks

| Task | Description | Done |
|------|-------------|------|
| 2.1 | Design waveform widget architecture | ☐ |
| 2.2 | Implement ring buffer for waveform data | ☐ |
| 2.3 | Create waveform rendering with LVGL canvas | ☐ |
| 2.4 | Generate synthetic ECG waveform data | ☐ |
| 2.5 | Generate synthetic pleth waveform data | ☐ |
| 2.6 | Implement sweep/scroll display modes | ☐ |
| 2.7 | Add gain and speed controls | ☐ |
| 2.8 | Optimize rendering for 30 FPS | ☐ |

### Deliverables
- [ ] ECG waveform displays and scrolls
- [ ] Pleth waveform displays and scrolls
- [ ] Configurable speed (12.5, 25, 50 mm/s equivalent)
- [ ] Smooth 30 FPS rendering

---

## Phase 3: Alarm System
**Duration:** 2-3 weeks

### Goals
- Alarm evaluation logic
- Visual and audio alarms
- Alarm acknowledgment flow

### Tasks

| Task | Description | Done |
|------|-------------|------|
| 3.1 | Define alarm data structures | ☐ |
| 3.2 | Implement alarm engine (threshold evaluation) | ☐ |
| 3.3 | Create alarm priority system (High/Medium/Low) | ☐ |
| 3.4 | Implement visual alarm indicators | ☐ |
| 3.5 | Implement audio alarm (simulator: visual only) | ☐ |
| 3.6 | Create alarm limits configuration screen | ☐ |
| 3.7 | Implement alarm acknowledgment flow | ☐ |
| 3.8 | Create alarm history log | ☐ |
| 3.9 | Add alarm silencing (temporary) | ☐ |
| 3.10 | Unit tests for alarm engine | ☐ |

### Deliverables
- [ ] Alarms trigger when vitals exceed limits
- [ ] Visual indicator matches priority
- [ ] Acknowledgment requires tap
- [ ] Alarm limits configurable
- [ ] Alarm history viewable

---

## Phase 4: Patient Management
**Duration:** 2 weeks

### Goals
- Patient association workflow
- Patient display and context
- Multi-patient support (1-2)

### Tasks

| Task | Description | Done |
|------|-------------|------|
| 4.1 | Design patient data model | ☐ |
| 4.2 | Create patient association screen | ☐ |
| 4.3 | Implement manual patient entry | ☐ |
| 4.4 | Implement barcode scanning (mock in simulator) | ☐ |
| 4.5 | Create patient info header display | ☐ |
| 4.6 | Implement patient disassociation flow | ☐ |
| 4.7 | Add multi-patient toggle (1 or 2 patients) | ☐ |
| 4.8 | Create split-screen layout for 2 patients | ☐ |

### Deliverables
- [ ] Can associate patient by name/ID entry
- [ ] Patient name shows in header
- [ ] Can disassociate patient
- [ ] 2-patient mode displays both

---

## Phase 5: Data Storage & Trends
**Duration:** 2-3 weeks

### Goals
- SQLite database integration
- Trend storage (72 hours)
- Trend visualization screen

### Tasks

| Task | Description | Done |
|------|-------------|------|
| 5.1 | Design database schema | ☐ |
| 5.2 | Integrate SQLite | ☐ |
| 5.3 | Implement vital signs storage | ☐ |
| 5.4 | Implement alarm event storage | ☐ |
| 5.5 | Create trend query functions | ☐ |
| 5.6 | Design trends screen UI | ☐ |
| 5.7 | Implement trend chart widget | ☐ |
| 5.8 | Add time range selection (1hr, 4hr, 24hr, 72hr) | ☐ |
| 5.9 | Implement data retention (auto-delete >72hr) | ☐ |
| 5.10 | Add database encryption at rest | ☐ |

### Deliverables
- [ ] Vitals stored to database
- [ ] Trends screen shows historical data
- [ ] Time range selectable
- [ ] Data auto-purges after 72 hours

---

## Phase 6: Multi-Process Architecture
**Duration:** 2-3 weeks

### Goals
- Split into separate services
- IPC with nanomsg
- Process supervision

### Tasks

| Task | Description | Done |
|------|-------------|------|
| 6.1 | Extract sensor reading to sensor-service | ☐ |
| 6.2 | Extract alarm logic to alarm-service | ☐ |
| 6.3 | Implement nanomsg pub/sub for vitals | ☐ |
| 6.4 | Implement D-Bus for control messages | ☐ |
| 6.5 | Create audit-service for logging | ☐ |
| 6.6 | Add watchdog service | ☐ |
| 6.7 | Create systemd unit files | ☐ |
| 6.8 | Test process crash and restart | ☐ |

### Deliverables
- [ ] Services run as separate processes
- [ ] IPC working between services
- [ ] Crashed service auto-restarts
- [ ] UI continues if sensor-service restarts

---

## Phase 7: Authentication & Security
**Duration:** 2 weeks

### Goals
- User authentication
- Role-based access
- Audit logging

### Tasks

| Task | Description | Done |
|------|-------------|------|
| 7.1 | Create user database schema | ☐ |
| 7.2 | Implement PIN entry screen | ☐ |
| 7.3 | Implement PIN hashing (Argon2/bcrypt) | ☐ |
| 7.4 | Add RFID badge support (mock in simulator) | ☐ |
| 7.5 | Implement session management | ☐ |
| 7.6 | Create role-based permission system | ☐ |
| 7.7 | Add screen lock with timeout | ☐ |
| 7.8 | Implement comprehensive audit logging | ☐ |
| 7.9 | Create audit log viewer screen | ☐ |

### Deliverables
- [ ] Login required for configuration
- [ ] PIN authentication working
- [ ] Session timeout locks screen
- [ ] Audit log captures all security events

---

## Phase 8: Buildroot Integration
**Duration:** 2-3 weeks

### Goals
- Target build working
- Boots on STM32MP157F-DK2
- All services run on hardware

### Tasks

| Task | Description | Done |
|------|-------------|------|
| 8.1 | Create Buildroot external tree | ☐ |
| 8.2 | Configure kernel for STM32MP157 | ☐ |
| 8.3 | Create device tree overlay for display | ☐ |
| 8.4 | Create packages for each service | ☐ |
| 8.5 | Configure filesystem overlay | ☐ |
| 8.6 | Build and flash first image | ☐ |
| 8.7 | Debug boot and display issues | ☐ |
| 8.8 | Verify all services start | ☐ |
| 8.9 | Test touchscreen input | ☐ |

### Deliverables
- [ ] System boots to UI on hardware
- [ ] Touchscreen works
- [ ] All services running
- [ ] WiFi connects

---

## Phase 9: Network & Integration
**Duration:** 3-4 weeks

### Goals
- Network configuration UI
- EHR integration (FHIR)
- ABDM integration

### Tasks

| Task | Description | Done |
|------|-------------|------|
| 9.1 | Create network-service | ☐ |
| 9.2 | Implement WiFi configuration UI | ☐ |
| 9.3 | Implement export queue (offline-first) | ☐ |
| 9.4 | Create FHIR client for vitals export | ☐ |
| 9.5 | Create FHIR client for patient import | ☐ |
| 9.6 | Implement ABDM ABHA lookup | ☐ |
| 9.7 | Implement ABDM health record push | ☐ |
| 9.8 | Add network status indicator | ☐ |
| 9.9 | Test offline → online sync | ☐ |

### Deliverables
- [ ] WiFi configurable from UI
- [ ] Vitals export to test EHR
- [ ] ABHA ID lookup works
- [ ] Offline queue syncs when online

---

## Phase 10: Security Hardening
**Duration:** 2 weeks

### Goals
- Secure boot chain
- Read-only rootfs
- Tamper-proofing

### Tasks

| Task | Description | Done |
|------|-------------|------|
| 10.1 | Enable secure boot on STM32MP1 | ☐ |
| 10.2 | Sign bootloader and kernel | ☐ |
| 10.3 | Implement dm-verity for rootfs | ☐ |
| 10.4 | Encrypt data partition with LUKS | ☐ |
| 10.5 | Configure SELinux/AppArmor | ☐ |
| 10.6 | Disable debug interfaces | ☐ |
| 10.7 | Implement kiosk mode | ☐ |
| 10.8 | Security testing and hardening | ☐ |

### Deliverables
- [ ] Secure boot verified
- [ ] Rootfs integrity verified on boot
- [ ] Data partition encrypted
- [ ] No shell escape possible

---

## Phase 11: OTA Updates
**Duration:** 2 weeks

### Goals
- A/B update scheme
- Signed update packages
- Rollback on failure

### Tasks

| Task | Description | Done |
|------|-------------|------|
| 11.1 | Partition layout for A/B | ☐ |
| 11.2 | Integrate SWUpdate | ☐ |
| 11.3 | Create update package format | ☐ |
| 11.4 | Implement update signature verification | ☐ |
| 11.5 | Implement rollback logic | ☐ |
| 11.6 | Create update UI | ☐ |
| 11.7 | Test update and rollback | ☐ |

### Deliverables
- [ ] Update via USB works
- [ ] Invalid update rejected
- [ ] Failed boot triggers rollback

---

## Phase 12: Hardware Integration
**Duration:** 3-4 weeks

### Goals
- Real sensor drivers
- Cortex-M4 co-processor (optional)
- Production sensor validation

### Tasks

| Task | Description | Done |
|------|-------------|------|
| 12.1 | Integrate MAX30102 SpO2 driver | ☐ |
| 12.2 | Integrate ADS1292R ECG driver | ☐ |
| 12.3 | Integrate temperature sensor driver | ☐ |
| 12.4 | Implement NIBP driver (if applicable) | ☐ |
| 12.5 | Validate sensor accuracy | ☐ |
| 12.6 | Optimize sampling rates | ☐ |
| 12.7 | (Optional) Port sensor sampling to M4 | ☐ |
| 12.8 | (Optional) Implement A7-M4 communication | ☐ |

### Deliverables
- [ ] Real SpO2 readings display
- [ ] Real ECG waveform displays
- [ ] Sensor accuracy validated

---

## Phase 13: Testing & Documentation
**Duration:** 2-3 weeks

### Goals
- Comprehensive test coverage
- IEC 62304 documentation
- Usability testing

### Tasks

| Task | Description | Done |
|------|-------------|------|
| 13.1 | Write remaining unit tests | ☐ |
| 13.2 | Write integration tests | ☐ |
| 13.3 | Conduct usability testing with nurses | ☐ |
| 13.4 | Complete software requirements spec | ☐ |
| 13.5 | Complete software architecture document | ☐ |
| 13.6 | Complete software test reports | ☐ |
| 13.7 | Create traceability matrix | ☐ |
| 13.8 | Document risk controls | ☐ |

### Deliverables
- [ ] >80% unit test coverage
- [ ] Usability issues addressed
- [ ] IEC 62304 documentation complete

---

## Milestones Summary

| Milestone | Phases | Target |
|-----------|--------|--------|
| **M1: Simulator Demo** | 0-3 | Week 8 |
| **M2: Target Boot** | 4-8 | Week 16 |
| **M3: Connected Device** | 9 | Week 20 |
| **M4: Secure Device** | 10-11 | Week 24 |
| **M5: Hardware Integrated** | 12 | Week 28 |
| **M6: Regulatory Ready** | 13 | Week 32 |

---

## Definition of Done

A phase is complete when:

1. All tasks checked off
2. Code reviewed and merged
3. Unit tests passing
4. Integration tests passing (if applicable)
5. Simulator demo working
6. Documentation updated

---

## Risk Mitigation

| Risk | Mitigation |
|------|------------|
| LVGL performance issues | Profile early, optimize waveform rendering |
| Buildroot complexity | Start with ST's reference, customize incrementally |
| Sensor driver issues | Use eval boards with known-good drivers first |
| Regulatory timeline | Begin documentation in parallel with development |
| ABDM API changes | Abstract integration layer, monitor NHA updates |
