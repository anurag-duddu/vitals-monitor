# Product Overview: Bedside Vitals Monitor

## Document Information

| Attribute | Value |
|-----------|-------|
| Document ID | PRD-001 |
| Version | 1.0 |
| Status | Draft |
| Target Market | India |
| Regulatory Pathway | CDSCO (Medical Device Rules 2017) |

---

## 1. Executive Summary

This document defines the product vision, scope, and high-level requirements for a bedside vitals monitor designed for general ward and medical-surgical (med-surg) inpatient settings in Indian hospitals. The device provides continuous and intermittent monitoring of vital signs, waveform visualization, intelligent alarming, and bidirectional integration with Electronic Health Record (EHR) systems including India's Ayushman Bharat Digital Mission (ABDM) infrastructure.

The product targets the gap between expensive ICU-grade monitors and basic spot-check devices, providing modern user experience, reliable connectivity, and workflow efficiency for nursing staff in resource-conscious healthcare environments.

---

## 2. Product Vision

### 2.1 Vision Statement

To deliver a reliable, intuitive, and connected bedside vitals monitor that empowers nursing staff to deliver efficient patient care in general ward settings, while seamlessly integrating with India's evolving digital health infrastructure.

### 2.2 Strategic Positioning

| Dimension | Position |
|-----------|----------|
| Market Segment | General Ward / Med-Surg / Step-down |
| Price Tier | Mid-market (below Philips/GE, above Contec/Edan) |
| Differentiation | Modern UX, ABDM-native, open architecture |
| Geographic Focus | India (primary), emerging markets (secondary) |

### 2.3 Key Value Propositions

**For Nurses:**
- Faster vital sign capture (target: under 60 seconds for full assessment)
- Intuitive touch interface requiring minimal training
- Clear alarm prioritization reducing alarm fatigue
- Automatic EHR documentation eliminating manual transcription

**For Hospital Administration:**
- Lower total cost of ownership (no per-unit software licensing)
- ABDM compliance for government scheme participation
- Reduced documentation errors and associated liability
- Modern equipment supporting accreditation goals

**For Patients:**
- Improved monitoring coverage in general wards
- Faster response to deterioration through smart alarming
- Continuity of health records via ABHA integration

---

## 3. Target Users

### 3.1 Primary User: Staff Nurse

| Attribute | Description |
|-----------|-------------|
| Role | Registered nurse assigned to general ward |
| Typical Ratio | 1:4 to 1:6 (nurse to patient) |
| Technical Proficiency | Basic to moderate; smartphone-literate |
| Key Tasks | Vital sign rounds (Q4h), medication administration, patient assessment |
| Pain Points | Time pressure, alarm fatigue, manual charting, equipment complexity |
| Usage Context | Standing at bedside, often wearing gloves, frequent interruptions |

**Workflow Characteristics:**
- Rounds every 4 hours with spot checks between
- Must capture vitals quickly and move to next patient
- Needs immediate visibility of concerning trends
- Cannot spend time troubleshooting equipment

### 3.2 Secondary User: Physician

| Attribute | Description |
|-----------|-------------|
| Role | Attending physician or resident |
| Technical Proficiency | Moderate to high |
| Key Tasks | Review trends, assess patient status, adjust alarm limits |
| Usage Pattern | Intermittent review during rounds, consultation on alarms |

**Workflow Characteristics:**
- Reviews trends and history rather than capturing vitals
- Needs quick access to historical data
- May adjust alarm thresholds based on patient condition
- Relies on nursing alerts for deterioration

### 3.3 Tertiary User: Biomedical Engineer

| Attribute | Description |
|-----------|-------------|
| Role | Hospital biomedical/clinical engineering staff |
| Technical Proficiency | High |
| Key Tasks | Device configuration, network setup, troubleshooting, calibration |
| Usage Pattern | Initial setup, periodic maintenance, issue resolution |

**Access Requirements:**
- Administrator-level access for configuration
- Network and integration settings
- Calibration and diagnostic functions
- Audit log access

### 3.4 Tertiary User: Hospital IT Administrator

| Attribute | Description |
|-----------|-------------|
| Role | IT infrastructure and security management |
| Technical Proficiency | High |
| Key Tasks | Network integration, security policy compliance, EHR connectivity |
| Usage Pattern | Initial deployment, ongoing monitoring |

**Access Requirements:**
- Network configuration
- Security certificate management
- Integration endpoint configuration
- System logs for security auditing

---

## 4. Clinical Setting Context

### 4.1 Target Environment

| Setting | Characteristics |
|---------|-----------------|
| Ward Type | General ward, medical-surgical, step-down |
| Bed Count | Typically 20-40 beds per ward |
| Monitoring Level | Intermittent to basic continuous |
| Acuity | Low to moderate (high acuity transfers to ICU) |

**NOT intended for:**
- Intensive Care Units (ICU/CCU)
- Operating rooms
- Emergency departments (primary monitoring)
- Neonatal/pediatric (different parameter ranges)

### 4.2 Physical Environment

| Factor | Consideration |
|--------|---------------|
| Mounting | Wall-mounted, roll stand, or bed rail |
| Lighting | Variable (daylight, artificial, dimmed at night) |
| Noise | Moderate ambient noise |
| Cleaning | Frequent surface disinfection with hospital-grade agents |
| Power | Hospital electrical supply with potential fluctuations |
| Temperature | 18-30°C typical, may exceed during summer/AC failure |

### 4.3 Operational Characteristics

| Aspect | Requirement |
|--------|-------------|
| Operating Hours | 24/7 continuous operation |
| Network Availability | Variable; must function fully offline |
| User Turnover | High (shift changes, rotating staff) |
| Training Time | Minimal (< 30 minutes for basic operation) |

---

## 5. Scope Definition

### 5.1 In Scope (Version 1.0)

**Vital Sign Parameters:**
- Heart Rate (HR) via ECG and/or SpO2
- Oxygen Saturation (SpO2) with plethysmograph waveform
- Non-Invasive Blood Pressure (NIBP) - systolic, diastolic, mean
- Body Temperature (peripheral)
- Respiratory Rate (RR) - derived from ECG or SpO2

**Waveform Display:**
- ECG (single lead)
- Plethysmograph (SpO2 waveform)

**Core Features:**
- Real-time vital sign display with configurable layouts
- Waveform visualization with freeze/capture capability
- Configurable alarm thresholds per patient
- Three-tier alarm priority system (high/medium/low)
- 72-hour trend storage with graphical visualization
- Patient association (manual, barcode, RFID)
- 1-2 patient per device configuration
- User authentication (PIN and badge/RFID)
- Comprehensive audit logging
- Bidirectional EHR integration
- ABDM/ABHA integration
- WiFi and Ethernet connectivity
- Full offline operation capability

**Platform:**
- Custom Linux-based embedded system
- LVGL user interface framework
- Touch-based interaction
- Responsive design for multiple screen sizes

### 5.2 Out of Scope (Version 1.0)

| Feature | Rationale |
|---------|-----------|
| Advanced arrhythmia analysis | ICU-level feature, regulatory complexity |
| Central monitoring station | Future product; requires separate development |
| Invasive blood pressure | ICU-level parameter |
| Capnography (EtCO2) | Specialized monitoring, sensor cost |
| Cardiac output | ICU-level parameter |
| Predictive deterioration (AI/ML) | Future enhancement |
| Multi-language UI | V1.0 English only; localization in future |
| Voice commands | Future enhancement |
| Mobile app companion | Future product |
| Pediatric/neonatal modes | Requires separate parameter validation |

### 5.3 Future Roadmap Considerations

**Version 1.1 (Near-term):**
- Hindi language support
- Enhanced trend analytics
- Nurse call system integration

**Version 2.0 (Medium-term):**
- Central monitoring station
- Predictive Early Warning Score
- Additional languages

**Version 3.0 (Long-term):**
- AI-powered deterioration prediction
- Mobile companion application
- Expanded parameter set

---

## 6. High-Level Feature Categories

### 6.1 Vital Signs Acquisition and Display

Real-time capture and visualization of physiological parameters from connected sensors. Primary interface for clinical assessment.

**Key Capabilities:**
- Continuous numeric display with configurable refresh rates
- Color-coded normal/abnormal indication
- Large, readable numerics visible from bedside distance
- Parameter-specific precision and units

### 6.2 Waveform Visualization

Continuous display of ECG and plethysmograph waveforms for rhythm assessment and perfusion evaluation.

**Key Capabilities:**
- Configurable sweep speed and scale
- Grid overlay for measurement
- Freeze and capture for documentation
- Waveform quality indicators

### 6.3 Alarm Management

Intelligent alerting system to notify clinical staff of parameter deviations and technical issues.

**Key Capabilities:**
- Three-tier priority system (high/medium/low)
- Configurable thresholds per patient
- Audible and visual alarm presentation
- Alarm pause and acknowledgment workflow
- Alarm history and statistics

### 6.4 Trend Storage and Visualization

Historical data retention and graphical presentation for pattern recognition and clinical decision support.

**Key Capabilities:**
- 72-hour minimum on-device storage
- Multiple timeframe views (1h, 4h, 12h, 24h, 72h)
- Graphical trend display with alarm events overlay
- Tabular data view with export capability

### 6.5 Patient Context Management

Association of monitoring session with patient identity and demographic information.

**Key Capabilities:**
- Manual patient ID entry
- Barcode scanning (wristband, chart)
- RFID/badge tap for patient identification
- Demographic display (name, age, bed, allergies)
- Session start/end documentation

### 6.6 EHR and ABDM Integration

Bidirectional connectivity with hospital information systems and national health infrastructure.

**Key Capabilities:**
- Import patient demographics from EHR
- Export vital signs and alarm events
- ABHA (Ayushman Bharat Health Account) patient lookup
- ABDM health record linking
- Offline queue with sync on reconnection

### 6.7 Security and Audit

Access control, data protection, and activity logging for compliance and forensic requirements.

**Key Capabilities:**
- User authentication (PIN, RFID badge)
- Role-based access control
- Comprehensive audit logging
- Encrypted data storage
- Tamper-resistant system architecture

### 6.8 Configuration and Administration

Device setup, customization, and maintenance functions for technical staff.

**Key Capabilities:**
- Network configuration (WiFi, Ethernet)
- Default alarm threshold configuration
- Display and sound preferences
- System diagnostics
- Software update management

---

## 7. Technical Foundation

### 7.1 Hardware Platform

| Component | Specification |
|-----------|---------------|
| Processor | ARM Cortex-A class SoC (Snapdragon or equivalent) |
| Architecture | 32-bit or 64-bit Linux-capable |
| Boot Media | SD card / eMMC |
| Display | Industrial-grade TFT (size responsive) |
| Input | Capacitive touch (glove-compatible preferred) |
| Connectivity | WiFi 802.11 b/g/n/ac, Ethernet 10/100 |
| Sensors | External medical-grade sensor modules |

### 7.2 Software Platform

| Layer | Technology |
|-------|------------|
| Operating System | Embedded Linux (Buildroot-based) |
| UI Framework | LVGL (MIT license) |
| Programming Languages | C (primary), potentially Rust for services |
| IPC | D-Bus or Unix sockets with structured messages |
| Data Storage | SQLite for structured data, filesystem for waveforms |
| Security | Secure boot, dm-verity, SELinux/AppArmor |

### 7.3 Open Source Commitment

All core software components utilize permissive open source licenses (MIT, BSD, Apache 2.0) to avoid per-unit licensing costs and ensure long-term maintainability without vendor dependency.

| Component | License | Rationale |
|-----------|---------|-----------|
| LVGL | MIT | No commercial restrictions |
| Buildroot | GPL | Build system only, not runtime |
| Linux Kernel | GPL | Standard for embedded devices |
| SQLite | Public Domain | Maximum flexibility |
| Application Code | Proprietary | Core IP |

---

## 8. Regulatory Context

### 8.1 Classification

| Jurisdiction | Classification | Basis |
|--------------|----------------|-------|
| India (CDSCO) | Class B (likely) | Medical Device Rules 2017; diagnostic monitoring device |
| Reference: EU MDR | Class IIa | Monitoring of vital physiological parameters |
| Reference: FDA | Class II | 510(k) pathway for patient monitors |

### 8.2 Applicable Standards

| Standard | Description | Applicability |
|----------|-------------|---------------|
| IEC 62304 | Medical device software lifecycle | Mandatory |
| IEC 62366-1 | Usability engineering | Mandatory |
| ISO 14971 | Risk management | Mandatory |
| IEC 60601-1 | Medical electrical equipment - General safety | Hardware |
| IEC 60601-1-2 | Electromagnetic compatibility | Hardware |
| IEC 60601-1-8 | Alarm systems | Alarm design |
| IEC 60601-2-49 | Multifunction patient monitors | Product-specific |

### 8.3 ABDM Compliance

| Requirement | Description |
|-------------|-------------|
| ABHA Integration | Support for Ayushman Bharat Health Account patient identification |
| Health Record Linking | Ability to link vitals data to patient's longitudinal health record |
| Consent Management | Implementation of ABDM consent framework |
| Certification | NHA (National Health Authority) product certification |

---

## 9. Success Metrics

### 9.1 Product Performance Metrics

| Metric | Target | Measurement |
|--------|--------|-------------|
| Vital Capture Time | < 60 seconds | Time from patient approach to data available |
| System Uptime | > 99.9% | Excluding planned maintenance |
| False Alarm Rate | < 10% of total alarms | Clinical validation study |
| User Satisfaction | > 4.0/5.0 | Nurse satisfaction survey |
| Training Time | < 30 minutes | Time to basic proficiency |

### 9.2 Business Metrics

| Metric | Target | Timeframe |
|--------|--------|-----------|
| Regulatory Clearance | CDSCO approval | 12-18 months from submission |
| First Hospital Deployment | 3 pilot sites | 6 months post-approval |
| Unit Cost | < ₹50,000 BOM | Production volume 1000+ |

---

## 10. Assumptions and Dependencies

### 10.1 Assumptions

| ID | Assumption | Impact if Invalid |
|----|------------|-------------------|
| A1 | Target hospitals have WiFi or Ethernet infrastructure | Offline-only mode; delayed EHR sync |
| A2 | Nursing staff have basic smartphone proficiency | Extended training requirements |
| A3 | ABDM APIs remain stable and accessible | Integration rework required |
| A4 | Medical-grade sensors available from third-party suppliers | Hardware redesign required |
| A5 | CDSCO Class B pathway applicable | Regulatory strategy revision |

### 10.2 Dependencies

| ID | Dependency | Owner | Impact |
|----|------------|-------|--------|
| D1 | Sensor module selection and integration | Hardware Team | Blocks clinical validation |
| D2 | ABDM sandbox access for integration testing | External (NHA) | Blocks integration testing |
| D3 | Hospital pilot site agreements | Business Development | Blocks clinical validation |
| D4 | IEC 62304 documentation templates | Quality/Regulatory | Blocks compliance documentation |

---

## 11. Document References

| Document ID | Title | Purpose |
|-------------|-------|---------|
| PRD-002 | Functional Requirements | Detailed feature specifications |
| PRD-003 | UI/UX Requirements | Screen design and interaction |
| PRD-004 | Alarm Requirements | Alarm philosophy and behavior |
| PRD-005 | Data Storage Requirements | Storage architecture and retention |
| PRD-006 | Integration Requirements | EHR and ABDM connectivity |
| PRD-007 | Security Requirements | Access control and hardening |
| PRD-008 | Regulatory Requirements | Compliance specifications |
| PRD-009 | Architecture Requirements | Technical architecture |

---

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2024 | — | Initial draft |
