# Data Storage Requirements: Bedside Vitals Monitor

## Document Information

| Attribute | Value |
|-----------|-------|
| Document ID | PRD-005 |
| Version | 1.0 |
| Status | Draft |
| Parent Document | PRD-001 Product Overview |

---

## 1. Introduction

### 1.1 Purpose

This document defines data storage requirements for the bedside vitals monitor. It specifies what data is stored, retention policies, storage architecture, synchronization behavior, and data protection requirements.

### 1.2 Scope

This document covers:
- Data categories and types
- Storage architecture and media
- Retention policies
- Offline operation and synchronization
- Data formats and structures
- Data protection and encryption
- Storage management

### 1.3 Key Design Principles

| Principle | Description |
|-----------|-------------|
| **Offline-First** | Device fully functional without network; sync when available |
| **Data Integrity** | No data loss during power failure or system errors |
| **Patient Privacy** | Patient data protected at rest and in transit |
| **Clinical Continuity** | 72-hour minimum retention for shift continuity and review |
| **Auditability** | All significant events logged and traceable |

### 1.4 Requirement Notation

| Prefix | Category |
|--------|----------|
| STO-ARC | Architecture |
| STO-DAT | Data Types |
| STO-RET | Retention |
| STO-SYN | Synchronization |
| STO-FMT | Formats |
| STO-PRO | Protection |
| STO-MGT | Management |

---

## 2. Data Categories

### 2.1 Data Classification

#### STO-DAT-001: Data Categories
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Data shall be organized into distinct categories |

| Category | Description | Sensitivity |
|----------|-------------|-------------|
| **Patient Data** | Demographics, identifiers, clinical context | High (PHI) |
| **Vital Signs Data** | Numeric vital sign readings | High (PHI) |
| **Waveform Data** | ECG, plethysmograph waveforms | High (PHI) |
| **Alarm Data** | Alarm events, acknowledgments | High (PHI) |
| **Audit Data** | User actions, system events | Medium |
| **Configuration Data** | Device settings, alarm limits | Low |
| **System Data** | Logs, diagnostics, state | Low |

### 2.2 Patient Data

#### STO-DAT-010: Patient Demographics
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Patient identification and demographic data |

**Stored Fields:**

| Field | Type | Required | Source |
|-------|------|----------|--------|
| Patient ID | String (64) | Yes | Entry/Scan/EHR |
| ABHA Number | String (14) | No | ABDM |
| Name | String (128) | No | Entry/EHR |
| Age / DOB | Integer / Date | No | Entry/EHR |
| Gender | Enum | No | Entry/EHR |
| Bed / Location | String (32) | No | Entry/EHR |
| Admission Date | DateTime | No | EHR |
| Allergies | String (256) | No | EHR |
| Diagnosis | String (256) | No | EHR |

#### STO-DAT-011: Monitoring Session
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Monitoring session metadata |

**Stored Fields:**

| Field | Type | Description |
|-------|------|-------------|
| Session ID | UUID | Unique session identifier |
| Patient ID | String | Associated patient |
| Start Time | DateTime | Session start |
| End Time | DateTime | Session end (null if active) |
| Start User | String | User who admitted patient |
| End User | String | User who discharged patient |
| Alarm Config | JSON | Alarm limits for this session |
| Status | Enum | Active, Completed, Transferred |

### 2.3 Vital Signs Data

#### STO-DAT-020: Numeric Vital Signs
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Periodic vital sign measurements |

**Data Structure (per reading):**

| Field | Type | Description |
|-------|------|-------------|
| Timestamp | DateTime (ms) | Measurement time |
| Session ID | UUID | Associated session |
| HR | Integer | Heart rate (bpm) |
| HR Source | Enum | ECG, SpO2 |
| SpO2 | Integer | Oxygen saturation (%) |
| PI | Float | Perfusion index |
| RR | Integer | Respiratory rate (/min) |
| RR Source | Enum | Impedance, SpO2-derived |
| Temp | Float | Temperature (°C) |
| Temp Site | Enum | Oral, Axillary, IR, etc. |
| Quality Flags | Bitmap | Signal quality indicators |

#### STO-DAT-021: NIBP Measurements
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Blood pressure measurements (event-based) |

**Data Structure (per measurement):**

| Field | Type | Description |
|-------|------|-------------|
| Timestamp | DateTime | Measurement time |
| Session ID | UUID | Associated session |
| Systolic | Integer | Systolic pressure (mmHg) |
| Diastolic | Integer | Diastolic pressure (mmHg) |
| MAP | Integer | Mean arterial pressure (mmHg) |
| Pulse Rate | Integer | HR from NIBP cuff |
| Mode | Enum | Manual, Auto |
| Status | Enum | Success, Failed, Artifact |
| Cuff Site | Enum | Upper arm, Wrist |

### 2.4 Waveform Data

#### STO-DAT-030: ECG Waveform Data
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | ECG waveform samples |

**Storage Format:**

| Attribute | Value |
|-----------|-------|
| Sample Rate | 250 Hz (configurable) |
| Resolution | 16-bit signed integer |
| Units | µV (microvolts) |
| Lead | Lead II (default) |
| Compression | Optional (lossless) |

**Data Structure:**

| Field | Type | Description |
|-------|------|-------------|
| Segment Start | DateTime (ms) | Segment start time |
| Session ID | UUID | Associated session |
| Sample Rate | Integer | Samples per second |
| Lead | Enum | ECG lead identifier |
| Samples | Array[Int16] | Waveform samples |
| Quality | Integer | Segment quality score |

#### STO-DAT-031: Plethysmograph Waveform Data
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | SpO2 plethysmograph waveform samples |

**Storage Format:**

| Attribute | Value |
|-----------|-------|
| Sample Rate | 50-100 Hz (configurable) |
| Resolution | 16-bit unsigned integer |
| Units | Normalized (0-65535) |
| Compression | Optional (lossless) |

#### STO-DAT-032: Waveform Snapshots
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | User-captured waveform segments |

**Data Structure:**

| Field | Type | Description |
|-------|------|-------------|
| Snapshot ID | UUID | Unique identifier |
| Timestamp | DateTime | Capture time |
| Session ID | UUID | Associated session |
| User ID | String | Capturing user |
| Duration | Integer | Segment duration (seconds) |
| ECG Data | Binary | ECG waveform segment |
| Pleth Data | Binary | Pleth waveform segment |
| Vitals | JSON | Vital signs at capture |
| Annotation | String | User annotation (optional) |

### 2.5 Alarm Data

#### STO-DAT-040: Alarm Events
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Alarm event records |

**Data Structure:**

| Field | Type | Description |
|-------|------|-------------|
| Alarm ID | UUID | Unique identifier |
| Session ID | UUID | Associated patient session |
| Onset Time | DateTime (ms) | Alarm activation time |
| Parameter | Enum | HR, SpO2, NIBP, Temp, RR, System |
| Condition | String | Alarm condition (e.g., "High") |
| Priority | Enum | High, Medium, Low |
| Trigger Value | Float | Value that triggered alarm |
| Limit Value | Float | Alarm threshold |
| Status | Enum | Active, Acknowledged, Resolved |
| Ack Time | DateTime | Acknowledgment time |
| Ack User | String | Acknowledging user |
| Resolve Time | DateTime | Resolution time |
| Waveform Ref | UUID | Reference to waveform snapshot (optional) |

### 2.6 Audit Data

#### STO-DAT-050: Audit Log Entries
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Audit trail of significant events |

**Data Structure:**

| Field | Type | Description |
|-------|------|-------------|
| Entry ID | UUID | Unique identifier |
| Timestamp | DateTime (ms) | Event time |
| Event Type | Enum | See Event Types below |
| User ID | String | User (if authenticated) |
| Session ID | UUID | Patient session (if applicable) |
| Details | JSON | Event-specific details |
| Source IP | String | Network source (if applicable) |

**Event Types:**

| Category | Events |
|----------|--------|
| Authentication | Login, Logout, Failed Login, Session Timeout |
| Patient | Admit, Discharge, Demographics Change |
| Alarm | Acknowledge, Pause, Limit Change, Enable/Disable |
| Configuration | Setting Change, Network Change, Time Change |
| Data | Export, Delete, Sync |
| System | Boot, Shutdown, Error, Update |

### 2.7 Configuration Data

#### STO-DAT-060: Device Configuration
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Device settings and preferences |

**Configuration Categories:**

| Category | Examples |
|----------|----------|
| Display | Brightness, timeout, layout |
| Audio | Volume levels, tones |
| Alarm Defaults | Default limits, delays |
| Network | WiFi credentials, EHR endpoints |
| Integration | ABDM settings, HL7 config |
| System | Time zone, language, device ID |

---

## 3. Storage Architecture

### 3.1 Storage Media

#### STO-ARC-001: Primary Storage
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Primary data storage medium |

**Requirements:**
- SD card or eMMC (per hardware design)
- Minimum capacity: 4 GB (8+ GB recommended)
- Industrial grade for reliability
- Wear leveling for flash longevity

**Partitioning:**

| Partition | Purpose | Size | Filesystem |
|-----------|---------|------|------------|
| Boot | Bootloader, kernel | 128 MB | FAT32 |
| System | OS, application (read-only) | 1 GB | SquashFS |
| Config | Configuration, persistent settings | 256 MB | ext4 |
| Data | Patient data, trends, logs | Remaining | ext4 / SQLite |

#### STO-ARC-002: Storage Redundancy
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Data redundancy for critical information |

**Approach:**
- Journaled filesystem for crash recovery
- Write-ahead logging for database
- Critical configuration backed up to secondary location
- Audit log replicated (if storage permits)

### 3.2 Data Organization

#### STO-ARC-010: Database Selection
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Database for structured data |

**Selection: SQLite**

**Rationale:**
- Embedded, serverless
- ACID compliant
- Zero configuration
- Public domain license
- Well-suited for embedded Linux
- Proven reliability

**Database Organization:**

| Database | Contents |
|----------|----------|
| `clinical.db` | Patient sessions, vital signs, NIBP, alarms |
| `audit.db` | Audit log entries |
| `config.db` | Device configuration |

#### STO-ARC-011: Waveform Storage
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Waveform data storage approach |

**Approach: File-based with metadata in SQLite**

**Rationale:**
- Waveform data is large and sequential
- File-based storage more efficient for large binary blobs
- Metadata (indexes, timestamps) in SQLite for query
- Easier to manage retention (delete files)

**Directory Structure:**

```
/data/
├── waveforms/
│   ├── {session_id}/
│   │   ├── ecg/
│   │   │   ├── {timestamp}.wav
│   │   │   └── ...
│   │   ├── pleth/
│   │   │   └── ...
│   │   └── snapshots/
│   │       └── ...
│   └── ...
└── ...
```

#### STO-ARC-012: Storage Quotas
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Storage allocation limits |

**Allocation (example for 8GB total):**

| Category | Quota | Rationale |
|----------|-------|-----------|
| Vital signs (numeric) | 500 MB | 72h of high-resolution data |
| Waveforms | 2 GB | Configurable retention |
| Alarms | 100 MB | 72h+ history |
| Audit logs | 200 MB | Extended retention |
| Snapshots | 500 MB | User-captured segments |
| System/Buffer | 700 MB | Temporary, working space |

---

## 4. Retention Policies

### 4.1 Minimum Retention

#### STO-RET-001: 72-Hour Clinical Retention
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Minimum retention for clinical data |

**Requirement:**
- All vital signs, alarms, and patient data retained minimum 72 hours
- Covers typical nursing shift handoffs and clinical review needs
- Retention measured from data creation, not session end

**72-Hour Data Includes:**
- All numeric vital signs
- All NIBP measurements
- All alarm events
- Patient session data
- User-captured waveform snapshots

#### STO-RET-002: Waveform Retention
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Continuous waveform retention |

**Options (configurable based on storage capacity):**

| Mode | Retention | Use Case |
|------|-----------|----------|
| Full | 72 hours | Maximum clinical utility |
| Event-Triggered | Around alarms + snapshots | Balanced |
| Snapshots Only | User-captured only | Minimum storage |
| Disabled | None | Storage constrained |

**Default:** Event-Triggered (capture waveform around alarm events)

#### STO-RET-003: Audit Log Retention
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Audit log retention period |

**Requirement:**
- Audit logs retained minimum 90 days
- Regulatory and compliance requirement
- Older logs archived or exported before deletion

### 4.2 Retention Management

#### STO-RET-010: Data Expiration
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Automatic data expiration |

**Mechanism:**
- Background process checks data age
- Data older than retention period marked for deletion
- Deletion occurs during low-activity periods
- Deletion logged in audit trail

#### STO-RET-011: Session-Based Retention
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Retention tied to patient session |

**Behavior:**
- Data retained for active sessions regardless of age
- Retention timer starts at session end (discharge)
- Long sessions may exceed normal retention (by design)

#### STO-RET-012: Export Before Delete
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Ensure data exported before deletion |

**Behavior:**
- Data flagged as "pending export" until synced to EHR
- Pending export data not deleted even if past retention
- Warning generated if pending export data accumulates
- Manual override available (admin only, logged)

---

## 5. Synchronization

### 5.1 Offline Operation

#### STO-SYN-001: Full Offline Capability
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Device operates fully without network |

**Offline Capabilities:**
- Patient admission (manual entry, barcode, RFID)
- All vital sign monitoring
- All alarming
- All trend display
- All configuration changes
- Data stored locally

**Offline Limitations:**
- No EHR import (demographics)
- No ABHA lookup
- No data export
- No remote access

#### STO-SYN-002: Network State Handling
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Behavior on network state changes |

**On Network Loss:**
- Continue normal operation
- Queue data for later sync
- Display "Offline" indicator
- No alarm (network loss is low-priority technical)

**On Network Restoration:**
- Initiate sync process
- Prioritize alarm data export
- Background sync for bulk data
- Update "Online" indicator

### 5.2 Sync Process

#### STO-SYN-010: Sync Queue
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Queuing data for synchronization |

**Queue Structure:**

| Field | Description |
|-------|-------------|
| Queue ID | Unique identifier |
| Data Type | Vitals, Alarm, Patient, etc. |
| Priority | Sync priority (alarms first) |
| Session ID | Associated patient session |
| Created | Queue entry time |
| Retries | Number of sync attempts |
| Status | Pending, InProgress, Failed, Complete |
| Payload | Data to sync (or reference) |

**Queue Behavior:**
- FIFO within priority level
- Higher priority items sync first
- Failed items retried with backoff
- Queue persisted to survive reboot

#### STO-SYN-011: Sync Priority
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Priority ordering for sync |

| Priority | Data Type | Rationale |
|----------|-----------|-----------|
| 1 (Highest) | Active alarms | Clinical urgency |
| 2 | Alarm history | Documentation |
| 3 | Vital signs (recent) | Current patient state |
| 4 | Patient session events | ADT sync |
| 5 | Vital signs (older) | Historical data |
| 6 | Waveform snapshots | Documentation |
| 7 | Audit logs | Compliance |

#### STO-SYN-012: Sync Frequency
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | How often data is synchronized |

**Real-time Sync:**
- Active alarms: Immediate
- Alarm acknowledgments: Immediate

**Near Real-time Sync:**
- NIBP measurements: Within 30 seconds
- Patient admit/discharge: Within 30 seconds

**Periodic Sync:**
- Trend data: Every 1-5 minutes (configurable)
- Waveform snapshots: Every 5 minutes
- Audit logs: Every 15 minutes

**Background Sync:**
- Historical data backfill: As bandwidth allows

#### STO-SYN-013: Conflict Resolution
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Handling data conflicts |

**Conflict Scenarios:**
- Patient demographics updated in EHR while offline
- Alarm limits changed in both places

**Resolution Strategy:**
- Device-generated data (vitals, alarms): Device is authoritative
- EHR-sourced data (demographics): EHR is authoritative
- Configuration: Last-write-wins with audit trail
- Conflicts logged for review

### 5.3 Import Sync

#### STO-SYN-020: Patient Import
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Importing patient data from EHR |

**Import Triggers:**
- Patient association (ID entered/scanned)
- ABHA lookup request
- ADT message received

**Imported Data:**
- Demographics (name, age, gender)
- Bed/location assignment
- Allergies
- Diagnoses
- Previous alarm limit preferences (if available)

#### STO-SYN-021: Bed Assignment Import
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | ADT integration for bed assignment |

**Behavior:**
- Device registered to specific bed/location
- ADT feed indicates patient assigned to bed
- Auto-suggest patient association
- Nurse confirms association

---

## 6. Data Formats

### 6.1 Internal Formats

#### STO-FMT-001: Timestamp Format
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Standard timestamp format |

**Format:** ISO 8601 with milliseconds
**Example:** `2024-01-15T14:32:45.123Z`
**Timezone:** UTC internally, local for display
**Resolution:** Milliseconds for clinical data

#### STO-FMT-002: Numeric Data Format
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Numeric data representation |

| Data Type | Storage Type | Range |
|-----------|--------------|-------|
| Heart Rate | Integer (16-bit) | 0-300 |
| SpO2 | Integer (8-bit) | 0-100 |
| NIBP | Integer (16-bit) | 0-300 |
| Temperature | Float (32-bit) | 25.0-45.0 |
| Resp Rate | Integer (8-bit) | 0-100 |
| Perfusion Index | Float (32-bit) | 0.0-20.0 |

#### STO-FMT-003: Waveform Data Format
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Waveform sample storage format |

**Binary Format:**
- Header: Metadata (sample rate, start time, count)
- Body: Array of samples (16-bit signed/unsigned)
- Optional: Lossless compression (gzip, LZ4)

**File Naming:** `{session_id}_{start_timestamp}.wfm`

### 6.2 Export Formats

#### STO-FMT-010: HL7 FHIR Export
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Data export in FHIR format |

**FHIR Resources Used:**

| Data | FHIR Resource |
|------|---------------|
| Vital Signs | Observation |
| Patient | Patient |
| Alarm Events | DetectedIssue or Flag |
| Session | Encounter |
| Device | Device |

**Profiles:** India-specific FHIR profiles where available (ABDM)

#### STO-FMT-011: CSV Export
| Attribute | Specification |
|-----------|---------------|
| Priority | P3 |
| Description | CSV export for troubleshooting |

**Use Case:** Admin-only export for analysis
**Format:** Standard CSV with headers
**Encoding:** UTF-8

---

## 7. Data Protection

### 7.1 Encryption

#### STO-PRO-001: Encryption at Rest
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Patient data encrypted when stored |

**Requirements:**
- All patient-identifiable data encrypted
- AES-256 encryption
- Keys stored in secure element or TPM (if available)
- Key derived from device-specific secret

**Encrypted Data:**
- Patient demographics
- Vital signs with patient association
- Waveforms
- Alarm events
- Audit logs with patient context

**Unencrypted Data:**
- Device configuration (non-patient)
- System logs (non-patient)
- Software/firmware

#### STO-PRO-002: Encryption in Transit
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Data encrypted during network transmission |

**Requirements:**
- TLS 1.2+ for all network communication
- Certificate validation
- No patient data over unencrypted channels

### 7.2 Access Control

#### STO-PRO-010: Data Access Logging
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | All data access logged |

**Logged Access:**
- Patient data view
- Trend data view
- Data export
- Configuration access

#### STO-PRO-011: Data Deletion
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Secure data deletion |

**Requirements:**
- Data deletion requires authentication
- Deletion logged in audit trail
- Deleted data overwritten (not just unlinked)
- Factory reset securely erases all patient data

---

## 8. Storage Management

### 8.1 Capacity Management

#### STO-MGT-001: Storage Monitoring
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Monitor storage capacity |

**Monitoring:**
- Current usage by category
- Projected time to capacity
- Oldest data age

**Alerts:**
- Warning at 80% capacity
- Critical at 95% capacity
- Technical alarm if critically low

#### STO-MGT-002: Automatic Cleanup
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Automatic storage reclamation |

**Cleanup Process:**
1. Identify data past retention period
2. Verify data exported (if export enabled)
3. Delete oldest data first
4. Log deletions

**Constraints:**
- Never delete active session data
- Never delete pending-export data (unless override)
- Prioritize deleting waveforms over numerics

### 8.2 Backup and Recovery

#### STO-MGT-010: Configuration Backup
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Configuration backup capability |

**Features:**
- Export configuration to file
- Import configuration from file
- Network-based configuration backup (optional)
- Version tracking for configurations

#### STO-MGT-011: Database Recovery
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Database integrity and recovery |

**Mechanisms:**
- SQLite journal mode (WAL)
- Integrity check on startup
- Automatic recovery from corruption
- Graceful degradation if recovery fails

#### STO-MGT-012: Factory Reset
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Complete data reset capability |

**Factory Reset Actions:**
- Delete all patient data
- Delete all audit logs
- Reset configuration to defaults
- Secure erase of deleted data
- Preserve device identity/certificates

**Access:** Admin only, requires confirmation

---

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2024 | — | Initial draft |
