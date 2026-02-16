# Maintenance Plan

| Field          | Value                                      |
|----------------|--------------------------------------------|
| Document ID    | MTP-001                                    |
| Version        | 1.0                                        |
| Date           | 2026-02-15                                 |
| Author         | Engineering Team                           |
| Reviewer       | Quality Assurance                          |
| Approval       | Regulatory Affairs                         |
| Classification | IEC 62304 Software Safety Class B          |

---

## 1. Purpose and Scope

This document defines the post-market maintenance strategy for the Bedside Vitals Monitor software, per IEC 62304 clause 6 (Software Maintenance Process). It covers procedures for handling problem reports, implementing modifications, managing OTA updates, and ensuring continued safety and regulatory compliance throughout the product lifecycle.

---

## 2. Maintenance Activities

### 2.1 Problem Report Handling

All software defects, anomalies, and customer complaints shall be recorded and tracked in the defect tracking system (GitHub Issues).

| Severity   | Definition                                      | Response SLA | Resolution SLA |
|------------|-------------------------------------------------|--------------|----------------|
| Critical   | Patient safety risk or complete loss of function | 4 hours      | 24 hours       |
| High       | Major feature broken, no workaround             | 8 hours      | 72 hours       |
| Medium     | Feature impaired, workaround available           | 48 hours     | 2 weeks        |
| Low        | Cosmetic or minor usability issue                | 1 week       | Next release   |

Problem reports shall include:
- Description of the observed behavior
- Steps to reproduce
- Severity classification
- Affected software version
- Impact assessment (safety, performance, regulatory)

### 2.2 Change Evaluation

Each proposed change shall be evaluated for:

1. **Safety impact:** Does the change affect any hazard mitigation identified in the Risk Analysis (RA-001)?
2. **Regulatory impact:** Does the change require notification to CDSCO or re-classification?
3. **Regression risk:** Which test suites must be re-executed to verify the change?
4. **SOUP impact:** Does the change update or replace any SOUP component?

Changes classified as safety-relevant shall require full regression testing (all 563 automated tests) and updated risk analysis prior to release.

### 2.3 Modification Implementation

Software modifications shall follow the same development lifecycle activities defined in the Software Development Plan (SDP-001):

1. Requirements update (if applicable)
2. Design modification
3. Implementation with code review
4. Unit and integration testing
5. Traceability matrix update
6. Risk analysis update (if safety-relevant)
7. Release verification

---

## 3. OTA Update Strategy

### 3.1 Update Mechanism

The Bedside Vitals Monitor supports Over-The-Air (OTA) firmware updates via SWUpdate with an A/B partition scheme:

| Component          | Description                                              |
|--------------------|----------------------------------------------------------|
| Update package     | Signed SWUpdate `.swu` archive                           |
| Partition scheme   | A/B rootfs partitions with automatic fallback            |
| Delivery           | HTTPS download from hospital update server               |
| Authentication     | RSA-2048 signature verification of update packages       |
| Rollback           | Automatic rollback to previous partition on boot failure |
| Data preservation  | User data partition (LUKS-encrypted) preserved across updates |

### 3.2 Update Process

1. Update package available on hospital update server (or USB media for air-gapped environments).
2. Network-service downloads the package and verifies the cryptographic signature.
3. SWUpdate writes the new image to the inactive partition (A or B).
4. System reboots into the updated partition.
5. Boot verification confirms successful startup within watchdog timeout.
6. If boot fails, automatic rollback to the previous partition occurs.
7. OTA update event recorded in audit log.

### 3.3 Update Frequency

| Update Type          | Frequency           | Trigger                              |
|----------------------|---------------------|--------------------------------------|
| Security patches     | As needed           | CVE disclosure affecting SOUP components |
| Bug fixes            | Monthly or as needed| Critical/high defect reports         |
| Feature updates      | Quarterly           | Planned roadmap releases             |
| SOUP updates         | Per SOUP monitoring | Vendor advisory or version update    |

---

## 4. SOUP Monitoring

Third-party component anomaly monitoring per SOUP List (SOUP-001):

- **Linux kernel:** Security mailing list, CVE database, Buildroot update announcements.
- **LVGL:** GitHub releases and issues, changelog review.
- **SQLite:** sqlite.org changelog, CVE monitoring.
- **nanomsg:** GitHub releases and issues.

SOUP anomalies affecting the device shall be treated as problem reports and processed per Section 2.1.

---

## 5. Defect Tracking

| Attribute            | Value                                         |
|----------------------|-----------------------------------------------|
| Tool                 | GitHub Issues                                 |
| Labels               | severity (critical/high/medium/low), component, safety-relevant |
| Workflow             | Open -> In Progress -> In Review -> Resolved -> Closed |
| Traceability         | Issue linked to Git commit(s) and test case(s) |
| Retention            | All defect records retained for product lifetime |

---

## 6. Post-Market Surveillance Integration

Software maintenance activities shall feed into the post-market surveillance process:

- Software defect trends analyzed quarterly for systemic issues.
- Field safety corrective actions (FSCA) initiated for critical safety defects.
- Regulatory reporting per CDSCO Medical Device Rules for reportable events.
- Customer complaint records linked to software problem reports.

---

## 7. Archive and Retention

| Record Type              | Retention Period                    |
|--------------------------|-------------------------------------|
| Source code (all versions)| Product lifetime + 10 years        |
| Test results             | Product lifetime + 10 years        |
| Problem reports          | Product lifetime + 10 years        |
| Release records          | Product lifetime + 10 years        |
| SOUP evaluations         | Product lifetime + 10 years        |
| Risk analysis revisions  | Product lifetime + 10 years        |

---

## 8. Referenced Documents

| Document ID | Title                               |
|-------------|-------------------------------------|
| SDP-001     | Software Development Plan           |
| RA-001      | Risk Analysis                       |
| SOUP-001    | SOUP List                           |
| CMP-001     | Configuration Management Plan       |
| STP-001     | Software Test Plan                  |

---

## Revision History

| Version | Date       | Author           | Changes                          |
|---------|------------|------------------|----------------------------------|
| 1.0     | 2026-02-15 | Engineering Team | Initial maintenance plan         |
