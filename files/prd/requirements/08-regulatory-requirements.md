# Regulatory Requirements: Bedside Vitals Monitor

## Document Information

| Field | Value |
|-------|-------|
| Document ID | REQ-008 |
| Version | 1.0 |
| Status | Draft |
| Parent Document | REQ-001 Product Overview |
| Target Market | India |
| Regulatory Authority | CDSCO (Central Drugs Standard Control Organisation) |

---

## 1. Overview

This document specifies the regulatory requirements for the bedside vitals monitor targeting the Indian market. The device must comply with Indian medical device regulations administered by CDSCO, as well as relevant international standards for medical device software and electrical safety.

### 1.1 Regulatory Strategy

The primary regulatory pathway is CDSCO registration under the Medical Device Rules, 2017. The device is classified as a non-notified medical device requiring manufacturing and import license.

### 1.2 Applicable Regulations and Standards

| Regulation/Standard | Description | Applicability |
|---------------------|-------------|---------------|
| Medical Device Rules, 2017 | Indian medical device regulation | Mandatory |
| IEC 60601-1 | Medical electrical equipment safety | Mandatory |
| IEC 60601-1-2 | Electromagnetic compatibility | Mandatory |
| IEC 60601-2-49 | Multifunction patient monitors | Mandatory |
| IEC 62304 | Medical device software lifecycle | Mandatory |
| ISO 14971 | Risk management | Mandatory |
| IEC 62366-1 | Usability engineering | Mandatory |
| ISO 13485 | Quality management system | Mandatory |
| ISO 62443 | Cybersecurity | Recommended |

---

## 2. Device Classification

### 2.1 CDSCO Classification

Under the Medical Device Rules, 2017, medical devices are classified into Classes A, B, C, and D based on risk.

| Factor | Assessment |
|--------|------------|
| Duration of use | Continuous monitoring |
| Invasiveness | Non-invasive |
| Active device | Yes (electrically powered) |
| Contact with patient | Indirect (through sensors) |
| Diagnostic/therapeutic | Diagnostic (monitoring) |
| Life-sustaining | No (general ward, not ICU) |

**Proposed Classification: Class B**

Rationale: The device is an active diagnostic device that measures physiological parameters (vital signs) but is not life-sustaining or implantable. Class B classification applies to medium-risk devices that are non-invasive and used for measuring physiological data.

### 2.2 Classification Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| REG-CLASS-001 | Document classification rationale | Must |
| REG-CLASS-002 | Obtain CDSCO classification confirmation | Should |
| REG-CLASS-003 | Maintain classification throughout lifecycle | Must |

---

## 3. CDSCO Registration

### 3.1 Registration Pathway

For Class B medical devices manufactured in India:

1. Obtain Manufacturing License (Form MD-5)
2. Establish Quality Management System (ISO 13485)
3. Conduct performance testing
4. Prepare Technical Documentation
5. Submit application to CDSCO
6. Obtain Registration Certificate

### 3.2 Documentation Requirements

| Document | Description | Priority |
|----------|-------------|----------|
| Device Master File | Comprehensive device documentation | Must |
| Quality Manual | ISO 13485 compliant QMS documentation | Must |
| Risk Management File | ISO 14971 compliant risk documentation | Must |
| Clinical Evidence | Performance data, literature review | Must |
| Labeling | Instructions for use, packaging | Must |
| Software Documentation | IEC 62304 compliant software lifecycle | Must |

### 3.3 Registration Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| REG-CDSCO-001 | Prepare Device Master File per CDSCO format | Must |
| REG-CDSCO-002 | Establish ISO 13485 certified QMS | Must |
| REG-CDSCO-003 | Conduct safety and performance testing | Must |
| REG-CDSCO-004 | Obtain test reports from NABL-accredited lab | Must |
| REG-CDSCO-005 | Submit registration application | Must |
| REG-CDSCO-006 | Maintain registration through renewal | Must |

---

## 4. IEC 62304: Software Lifecycle

### 4.1 Software Safety Classification

IEC 62304 classifies medical device software based on potential harm:

| Class | Harm Potential | Documentation Rigor |
|-------|----------------|---------------------|
| Class A | No injury or damage to health | Minimal |
| Class B | Non-serious injury | Moderate |
| Class C | Death or serious injury | Maximum |

**Proposed Software Safety Class: Class B**

Rationale: The bedside vitals monitor is used in general ward settings (not ICU). While incorrect vital signs display could delay intervention, the clinical context includes human oversight (nurses, physicians) and the device is not the sole basis for treatment decisions. Failure would not directly cause death or serious injury but could contribute to patient harm.

If the device is later marketed for ICU or life-critical use, reclassification to Class C would be required.

### 4.2 IEC 62304 Process Requirements

#### 4.2.1 Development Planning

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| SW-PLAN-001 | Document Software Development Plan | Must |
| SW-PLAN-002 | Define software development lifecycle model | Must |
| SW-PLAN-003 | Establish software configuration management | Must |
| SW-PLAN-004 | Define verification and validation approach | Must |
| SW-PLAN-005 | Document software development standards | Must |
| SW-PLAN-006 | Plan software maintenance activities | Must |

#### 4.2.2 Requirements Analysis

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| SW-REQ-001 | Document software requirements specification | Must |
| SW-REQ-002 | Trace requirements to system requirements | Must |
| SW-REQ-003 | Trace requirements to risk controls | Must |
| SW-REQ-004 | Include functional and non-functional requirements | Must |
| SW-REQ-005 | Define software interfaces | Must |
| SW-REQ-006 | Verify requirements for completeness and consistency | Must |

#### 4.2.3 Architecture Design

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| SW-ARCH-001 | Document software architecture | Must |
| SW-ARCH-002 | Identify software items (components) | Must |
| SW-ARCH-003 | Document interfaces between components | Must |
| SW-ARCH-004 | Verify architecture implements requirements | Must |
| SW-ARCH-005 | Identify SOUP (Software of Unknown Provenance) | Must |
| SW-ARCH-006 | Assess SOUP risks | Must |

#### 4.2.4 Detailed Design

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| SW-DES-001 | Document detailed design for each software unit | Must |
| SW-DES-002 | Trace design to architecture | Must |
| SW-DES-003 | Verify design implements architecture | Must |
| SW-DES-004 | Review detailed design | Must |

#### 4.2.5 Implementation

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| SW-IMP-001 | Implement software per detailed design | Must |
| SW-IMP-002 | Follow coding standards | Must |
| SW-IMP-003 | Perform code review | Must |
| SW-IMP-004 | Use version control | Must |

#### 4.2.6 Verification

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| SW-VER-001 | Verify software units (unit testing) | Must |
| SW-VER-002 | Verify software integration (integration testing) | Must |
| SW-VER-003 | Verify software system (system testing) | Must |
| SW-VER-004 | Document test procedures and results | Must |
| SW-VER-005 | Trace tests to requirements | Must |
| SW-VER-006 | Address test failures | Must |

#### 4.2.7 Risk Management

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| SW-RISK-001 | Identify software hazardous situations | Must |
| SW-RISK-002 | Identify potential causes of hazards | Must |
| SW-RISK-003 | Evaluate risk for each hazard | Must |
| SW-RISK-004 | Implement risk control measures | Must |
| SW-RISK-005 | Verify risk control effectiveness | Must |
| SW-RISK-006 | Document software risk management | Must |

#### 4.2.8 Configuration Management

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| SW-CM-001 | Identify configuration items | Must |
| SW-CM-002 | Control changes to configuration items | Must |
| SW-CM-003 | Maintain traceability | Must |
| SW-CM-004 | Document software releases | Must |
| SW-CM-005 | Maintain SOUP list and versions | Must |

#### 4.2.9 Problem Resolution

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| SW-PROB-001 | Document problem resolution process | Must |
| SW-PROB-002 | Analyze problems for safety impact | Must |
| SW-PROB-003 | Track problems to resolution | Must |
| SW-PROB-004 | Verify problem corrections | Must |

### 4.3 SOUP Management

The device uses several Software of Unknown Provenance (SOUP) components:

| SOUP | Version | Function | Risk Assessment Required |
|------|---------|----------|--------------------------|
| Linux Kernel | TBD | Operating system | Yes |
| Buildroot | TBD | Build system | Yes |
| LVGL | TBD | UI framework | Yes |
| OpenSSL/mbedTLS | TBD | Cryptography | Yes |
| SQLite | TBD | Data storage | Yes |
| D-Bus/nanomsg | TBD | IPC | Yes |

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| SOUP-001 | Maintain SOUP list with versions | Must |
| SOUP-002 | Document SOUP intended use | Must |
| SOUP-003 | Assess SOUP failure modes | Must |
| SOUP-004 | Verify SOUP requirements | Must |
| SOUP-005 | Monitor SOUP for security vulnerabilities | Must |
| SOUP-006 | Document SOUP update process | Must |

---

## 5. ISO 14971: Risk Management

### 5.1 Risk Management Process

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| RISK-001 | Establish risk management plan | Must |
| RISK-002 | Define risk acceptability criteria | Must |
| RISK-003 | Conduct hazard identification | Must |
| RISK-004 | Estimate risks (severity × probability) | Must |
| RISK-005 | Evaluate risks against acceptance criteria | Must |
| RISK-006 | Implement risk control measures | Must |
| RISK-007 | Verify risk control effectiveness | Must |
| RISK-008 | Assess residual risk | Must |
| RISK-009 | Conduct risk-benefit analysis | Must |
| RISK-010 | Maintain risk management file | Must |

### 5.2 Hazard Categories

The following hazard categories shall be analyzed:

| Category | Examples |
|----------|----------|
| Electrical | Electric shock, burns, EMI |
| Mechanical | Sharp edges, device falling |
| Thermal | Overheating, burns |
| Biological | Cross-contamination |
| Software | Incorrect display, missed alarms, data loss |
| Use error | Incorrect patient association, alarm fatigue |
| Cybersecurity | Data breach, unauthorized access, tampering |

### 5.3 Risk Analysis Methods

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| RISK-METH-001 | Conduct FMEA for software components | Must |
| RISK-METH-002 | Conduct FTA for critical functions | Should |
| RISK-METH-003 | Conduct use-related risk analysis | Must |
| RISK-METH-004 | Conduct cybersecurity threat modeling | Must |

### 5.4 Risk Acceptability Matrix

| Severity | Probability | | | | |
|----------|-------------|---|---|---|---|
| | Improbable | Remote | Occasional | Probable | Frequent |
| Negligible | Acceptable | Acceptable | Acceptable | Acceptable | ALARP |
| Minor | Acceptable | Acceptable | Acceptable | ALARP | ALARP |
| Serious | Acceptable | Acceptable | ALARP | Unacceptable | Unacceptable |
| Critical | Acceptable | ALARP | Unacceptable | Unacceptable | Unacceptable |
| Catastrophic | ALARP | Unacceptable | Unacceptable | Unacceptable | Unacceptable |

ALARP = As Low As Reasonably Practicable (requires risk-benefit analysis)

---

## 6. IEC 60601-1: Electrical Safety

### 6.1 Applicable Sections

| Section | Title | Applicability |
|---------|-------|---------------|
| General | Basic safety and essential performance | Full |
| Clause 4 | General requirements | Full |
| Clause 8 | Electrical hazards | Full |
| Clause 9 | Mechanical hazards | Full |
| Clause 11 | Temperature limits | Full |
| Clause 14 | PEMS (Programmable Electrical Medical Systems) | Full |
| Clause 15 | Construction | Full |

### 6.2 Classification

| Parameter | Classification |
|-----------|----------------|
| Protection against electric shock | Class II (double insulation) or Class I |
| Degree of protection | Type BF (patient contact via sensors) |
| Mode of operation | Continuous |
| Ingress protection | IPX1 minimum (protection against dripping water) |

### 6.3 Essential Performance

Essential performance is the performance necessary to achieve freedom from unacceptable risk. For this device:

| Essential Performance | Description | Failure Impact |
|-----------------------|-------------|----------------|
| Vital signs display accuracy | Display correct values within tolerance | Could lead to delayed intervention |
| Alarm function | Generate alarms when limits exceeded | Could lead to missed deterioration |
| Data integrity | Maintain accurate patient-device association | Could lead to treatment errors |

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| PERF-001 | Define essential performance | Must |
| PERF-002 | Test essential performance under normal conditions | Must |
| PERF-003 | Test essential performance under single fault | Must |
| PERF-004 | Document essential performance verification | Must |

### 6.4 IEC 60601-1-2: EMC Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| EMC-001 | Conduct EMC emissions testing | Must |
| EMC-002 | Conduct EMC immunity testing | Must |
| EMC-003 | Maintain essential performance during EMC testing | Must |
| EMC-004 | Document EMC test plan per IEC 60601-1-2 | Must |
| EMC-005 | Include EMC guidance in labeling | Must |

### 6.5 IEC 60601-2-49: Particular Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| 60601-2-49-001 | Comply with alarm system requirements | Must |
| 60601-2-49-002 | Comply with patient data accuracy requirements | Must |
| 60601-2-49-003 | Comply with displayed value accuracy | Must |
| 60601-2-49-004 | Document compliance to IEC 60601-2-49 | Must |

---

## 7. IEC 62366-1: Usability Engineering

### 7.1 Usability Engineering Process

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| USE-001 | Establish usability engineering file | Must |
| USE-002 | Identify use-related hazards | Must |
| USE-003 | Develop use specification | Must |
| USE-004 | Identify user interface characteristics | Must |
| USE-005 | Conduct formative usability evaluation | Must |
| USE-006 | Conduct summative usability evaluation | Must |
| USE-007 | Document residual use-related risk | Must |

### 7.2 User Profiles

| User Group | Characteristics | Training Level |
|------------|-----------------|----------------|
| Staff Nurse | Primary user, time-constrained, may wear gloves | Moderate |
| Physician | Occasional user, reviews trends | Minimal |
| Biomedical Engineer | Configuration and maintenance | Trained |
| Patient Family | May view screen (no interaction) | None |

### 7.3 Use Environments

| Environment | Characteristics |
|-------------|-----------------|
| General Ward | Multiple patients, moderate lighting, interruptions |
| Night Shift | Low lighting, reduced staffing |
| Isolation Room | PPE required, limited access |
| Step-down Unit | Higher acuity, more frequent monitoring |

### 7.4 Critical Tasks

| Task | Frequency | Risk if Error |
|------|-----------|---------------|
| Associate patient | Each admission | Wrong patient data |
| Acknowledge alarm | Multiple per shift | Missed deterioration |
| Review vitals | Continuous | Delayed intervention |
| Modify alarm limits | Occasional | Inappropriate alarms |
| Disassociate patient | Each discharge | Data integrity |

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| USE-TASK-001 | Validate critical tasks in usability testing | Must |
| USE-TASK-002 | Achieve >90% task success for critical tasks | Must |
| USE-TASK-003 | Document usability test results | Must |

---

## 8. ISO 13485: Quality Management System

### 8.1 QMS Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| QMS-001 | Establish ISO 13485 compliant QMS | Must |
| QMS-002 | Obtain ISO 13485 certification | Must |
| QMS-003 | Maintain quality manual | Must |
| QMS-004 | Document design and development procedures | Must |
| QMS-005 | Document production procedures | Must |
| QMS-006 | Establish supplier control | Must |
| QMS-007 | Establish complaint handling | Must |
| QMS-008 | Conduct internal audits | Must |
| QMS-009 | Conduct management review | Must |

### 8.2 Design Controls

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| QMS-DC-001 | Document design and development planning | Must |
| QMS-DC-002 | Document design inputs | Must |
| QMS-DC-003 | Document design outputs | Must |
| QMS-DC-004 | Conduct design reviews | Must |
| QMS-DC-005 | Conduct design verification | Must |
| QMS-DC-006 | Conduct design validation | Must |
| QMS-DC-007 | Control design changes | Must |
| QMS-DC-008 | Maintain design history file | Must |

---

## 9. Labeling Requirements

### 9.1 Device Labeling

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| LABEL-001 | Include manufacturer name and address | Must |
| LABEL-002 | Include device name and model | Must |
| LABEL-003 | Include serial number | Must |
| LABEL-004 | Include manufacturing date | Must |
| LABEL-005 | Include regulatory symbols per ISO 15223-1 | Must |
| LABEL-006 | Include electrical ratings | Must |
| LABEL-007 | Include IP rating | Must |
| LABEL-008 | Include safety warnings | Must |

### 9.2 Instructions for Use

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| IFU-001 | Provide comprehensive instructions for use | Must |
| IFU-002 | Include intended use statement | Must |
| IFU-003 | Include contraindications | Must |
| IFU-004 | Include warnings and precautions | Must |
| IFU-005 | Include setup and installation instructions | Must |
| IFU-006 | Include operation instructions | Must |
| IFU-007 | Include maintenance instructions | Must |
| IFU-008 | Include troubleshooting guide | Must |
| IFU-009 | Include technical specifications | Must |
| IFU-010 | Include EMC guidance | Must |
| IFU-011 | Provide IFU in English and Hindi | Should |

---

## 10. Post-Market Requirements

### 10.1 Vigilance

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| VIG-001 | Establish adverse event reporting procedure | Must |
| VIG-002 | Report serious incidents to CDSCO | Must |
| VIG-003 | Conduct field safety corrective actions when needed | Must |
| VIG-004 | Maintain vigilance records | Must |

### 10.2 Post-Market Surveillance

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| PMS-001 | Establish post-market surveillance plan | Must |
| PMS-002 | Collect and analyze complaint data | Must |
| PMS-003 | Monitor device performance in field | Must |
| PMS-004 | Update risk management based on PMS data | Must |
| PMS-005 | Conduct periodic safety update reports | Should |

---

## 11. Clinical Evidence

### 11.1 Clinical Evidence Strategy

For a Class B vital signs monitor, clinical evidence requirements are moderate. The strategy combines:

1. Literature review of similar devices
2. Bench testing against reference standards
3. Clinical performance validation
4. Usability validation with intended users

### 11.2 Clinical Evidence Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| CLIN-001 | Conduct systematic literature review | Must |
| CLIN-002 | Document equivalence to predicate devices | Should |
| CLIN-003 | Validate vital signs accuracy against reference | Must |
| CLIN-004 | Validate alarm function | Must |
| CLIN-005 | Conduct clinical usability validation | Must |
| CLIN-006 | Document clinical evidence summary | Must |

### 11.3 Performance Criteria

| Parameter | Accuracy Requirement | Reference Standard |
|-----------|---------------------|-------------------|
| SpO2 | ARMS ≤ 3% (70-100%) | ISO 80601-2-61 |
| Heart Rate | ± 3 bpm or 3% | IEC 60601-2-27 |
| NIBP | ± 5 mmHg mean, 8 mmHg SD | ISO 81060-2 |
| Temperature | ± 0.2°C | ISO 80601-2-56 |
| Respiration Rate | ± 2 breaths/min | Manufacturer defined |

---

## 12. Cybersecurity Requirements

### 12.1 Regulatory Context

CDSCO and NHA (National Health Authority) have issued guidance on medical device cybersecurity. Additionally, international standards apply:

| Standard/Guidance | Description |
|-------------------|-------------|
| IEC 62443 | Industrial automation security |
| FDA Premarket Cybersecurity Guidance | Reference for best practices |
| CDSCO Cybersecurity Guidance | Indian regulatory guidance |
| NHA Security Standards | ABDM security requirements |

### 12.2 Cybersecurity Documentation

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| CYBER-001 | Conduct threat modeling | Must |
| CYBER-002 | Document security risk assessment | Must |
| CYBER-003 | Document security controls | Must |
| CYBER-004 | Document vulnerability management process | Must |
| CYBER-005 | Document software bill of materials (SBOM) | Must |
| CYBER-006 | Document security testing results | Must |
| CYBER-007 | Document security update process | Must |

---

## 13. Regulatory Timeline

### 13.1 Estimated Timeline

| Phase | Duration | Key Activities |
|-------|----------|----------------|
| QMS Establishment | 3-6 months | ISO 13485 implementation and certification |
| Design and Development | 12-18 months | IEC 62304 lifecycle |
| Testing | 3-6 months | IEC 60601 testing, clinical validation |
| Documentation | 2-3 months | Technical file preparation |
| CDSCO Submission | 1-2 months | Application submission |
| CDSCO Review | 3-6 months | Regulatory review and approval |
| **Total** | **24-41 months** | |

### 13.2 Critical Path Items

1. ISO 13485 certification (prerequisite for CDSCO submission)
2. IEC 60601 testing (requires accredited lab, lead time)
3. Clinical validation (requires site agreements, IRB if applicable)
4. CDSCO review timeline (variable, expedite programs available)

---

## 14. Regulatory Deliverables Checklist

| Deliverable | IEC 62304 | ISO 14971 | CDSCO | Status |
|-------------|-----------|-----------|-------|--------|
| Software Development Plan | ✓ | | | |
| Software Requirements Specification | ✓ | | ✓ | |
| Software Architecture Document | ✓ | | ✓ | |
| Software Detailed Design | ✓ | | | |
| Software Test Plan | ✓ | | ✓ | |
| Software Test Reports | ✓ | | ✓ | |
| SOUP List | ✓ | | | |
| Traceability Matrix | ✓ | | ✓ | |
| Risk Management Plan | | ✓ | ✓ | |
| Risk Management File | | ✓ | ✓ | |
| Hazard Analysis (FMEA) | | ✓ | ✓ | |
| Clinical Evidence Summary | | | ✓ | |
| Usability Engineering File | | | ✓ | |
| IEC 60601 Test Reports | | | ✓ | |
| Technical Documentation | | | ✓ | |
| Instructions for Use | | | ✓ | |
| Labeling | | | ✓ | |

---

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2024 | — | Initial draft |
