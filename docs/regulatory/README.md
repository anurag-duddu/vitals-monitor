# Regulatory Documentation Index

This directory contains IEC 62304 regulatory documentation templates for the Bedside Vitals Monitor. These are living documents to be completed during the software development lifecycle and regulatory submission process.

**Device:** Bedside Vitals Monitor (CDSCO Class B)
**Software Safety Classification:** IEC 62304 Class B

---

## Documents

| Document                        | File                            | Doc ID  | Status      |
|---------------------------------|---------------------------------|---------|-------------|
| Software Requirements Spec      | [software_requirements_spec.md](software_requirements_spec.md) | SRS-001 | Template    |
| Software Architecture Document  | [software_architecture_doc.md](software_architecture_doc.md)   | SAD-001 | Template    |
| Requirements Traceability Matrix| [traceability_matrix.md](traceability_matrix.md)               | RTM-001 | Template    |
| Risk Analysis (ISO 14971)       | [risk_analysis.md](risk_analysis.md)                           | RA-001  | Template    |
| Software Test Plan              | [software_test_plan.md](software_test_plan.md)                 | STP-001 | Template    |

---

## Applicable Standards

| Standard         | Title                                  | Relevance                |
|------------------|----------------------------------------|--------------------------|
| IEC 62304        | Medical device software lifecycle      | Software development     |
| ISO 14971        | Risk management for medical devices    | Risk analysis            |
| IEC 60601-1      | General safety and essential performance | Electrical safety       |
| IEC 60601-1-8    | Alarm systems                          | Alarm design             |
| IEC 60601-2-49   | Multiparameter patient monitors        | Device-specific safety   |
| IEC 62366-1      | Usability engineering                  | Usability validation     |
| ISO 13485        | Quality management systems             | QMS processes            |

---

## How to Use These Templates

1. Each document contains `[TODO]` markers where project-specific content must be filled in.
2. Update the document header (version, date, author) with each revision.
3. Maintain the revision history table at the bottom of each document.
4. As requirements, design, and tests are finalized, update the traceability matrix to reflect current status.
5. Risk analysis should be revisited whenever the design changes or new hazards are identified.

---

## Additional Documents (Not Yet Created)

The following IEC 62304 deliverables are not yet templated and should be created as the project matures:

- Software Development Plan (SDP)
- Software Maintenance Plan
- Software Configuration Management Plan
- Usability Engineering File (IEC 62366-1)
- Clinical Evidence Summary
- Device Master File (DMF)
