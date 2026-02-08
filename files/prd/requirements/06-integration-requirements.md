# Integration Requirements: Bedside Vitals Monitor

## Document Information

| Attribute | Value |
|-----------|-------|
| Document ID | PRD-006 |
| Version | 1.0 |
| Status | Draft |
| Parent Document | PRD-001 Product Overview |
| Target Market | India |

---

## 1. Introduction

### 1.1 Purpose

This document defines integration requirements for the bedside vitals monitor, specifying how the device connects with hospital information systems, electronic health records (EHR), and India's Ayushman Bharat Digital Mission (ABDM) infrastructure.

### 1.2 Scope

This document covers:
- Network connectivity
- EHR integration (bidirectional)
- ABDM/ABHA integration
- Communication protocols
- Data exchange formats
- Security requirements for integration
- Offline operation and sync

### 1.3 Integration Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│                        HOSPITAL NETWORK                             │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────────────────┐ │
│  │   Bedside   │    │  EHR/HIS    │    │      ABDM Gateway       │ │
│  │   Monitor   │◄──►│   Server    │◄──►│  (National Infra)       │ │
│  └─────────────┘    └─────────────┘    └─────────────────────────┘ │
│        │                   │                       │               │
│        │            ┌──────┴──────┐               │               │
│        │            │             │               │               │
│        │      ┌─────▼───┐  ┌──────▼────┐         │               │
│        │      │   ADT   │  │  Clinical │         │               │
│        │      │  Server │  │   Repo    │         │               │
│        │      └─────────┘  └───────────┘         │               │
│        │                                          │               │
│        └──────────────────────────────────────────┘               │
│                    (Direct ABDM if no EHR)                        │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### 1.4 Requirement Notation

| Prefix | Category |
|--------|----------|
| INT-NET | Network |
| INT-EHR | EHR Integration |
| INT-ABD | ABDM Integration |
| INT-PRT | Protocols |
| INT-SEC | Security |
| INT-OFL | Offline |

---

## 2. Network Connectivity

### 2.1 Network Interfaces

#### INT-NET-001: WiFi Connectivity
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Wireless network connectivity |

**Specifications:**
- IEEE 802.11 b/g/n/ac
- 2.4 GHz and 5 GHz bands
- WPA2/WPA3 security
- Enterprise authentication (802.1X) support
- Multiple saved networks
- Automatic reconnection

#### INT-NET-002: Ethernet Connectivity
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Wired network connectivity |

**Specifications:**
- 10/100 Mbps Ethernet
- Auto-negotiation
- DHCP client (default)
- Static IP configuration option
- Preferred over WiFi when connected

#### INT-NET-003: Network Priority
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Network interface preference |

**Priority Order:**
1. Ethernet (if connected)
2. WiFi (if configured and available)
3. Offline mode (if no network)

**Failover Behavior:**
- Automatic failover to available interface
- Reconnection attempts on interface loss
- Status indication on UI

### 2.2 Network Configuration

#### INT-NET-010: WiFi Configuration
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | WiFi setup options |

**Configuration Options:**
- Network SSID (scan or manual entry)
- Security type (Open, WPA2-Personal, WPA2-Enterprise)
- Password / certificates
- Hidden network support
- Multiple network profiles

#### INT-NET-011: IP Configuration
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | IP address configuration |

**Options:**
- DHCP (default)
- Static IP (manual configuration)
  - IP address
  - Subnet mask
  - Gateway
  - DNS servers

#### INT-NET-012: Proxy Configuration
| Attribute | Specification |
|-----------|---------------|
| Priority | P3 |
| Description | HTTP proxy support |

**Options:**
- No proxy (direct connection)
- Manual proxy configuration
- Proxy authentication support

### 2.3 Network Security

#### INT-NET-020: Transport Security
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Encryption for network communications |

**Requirements:**
- TLS 1.2 minimum, TLS 1.3 preferred
- Strong cipher suites only
- Certificate validation
- Certificate pinning for known services (optional)

#### INT-NET-021: Firewall Considerations
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Network ports and protocols |

**Outbound Connections (device initiates):**

| Service | Protocol | Port | Purpose |
|---------|----------|------|---------|
| EHR API | HTTPS | 443 | Data exchange |
| ABDM | HTTPS | 443 | National health ID |
| NTP | UDP | 123 | Time sync |
| DNS | UDP/TCP | 53 | Name resolution |

**Inbound Connections (optional):**

| Service | Protocol | Port | Purpose |
|---------|----------|------|---------|
| Management | HTTPS | 8443 | Remote configuration |
| Discovery | mDNS | 5353 | Service discovery |

---

## 3. EHR Integration

### 3.1 Integration Approach

#### INT-EHR-001: Integration Architecture
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Approach to EHR connectivity |

**Primary Approach: API-based Integration**
- RESTful APIs over HTTPS
- FHIR R4 data model
- Event-driven updates
- Pull and push capabilities

**Alternative: HL7 v2.x (for legacy systems)**
- TCP/MLLP transport
- ADT messages for patient events
- ORU messages for results

### 3.2 Data Import (EHR to Device)

#### INT-EHR-010: Patient Demographics Import
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Import patient information from EHR |

**Trigger:** Patient ID entered/scanned on device

**Imported Data:**

| Field | FHIR Path | Required |
|-------|-----------|----------|
| Name | Patient.name | Yes |
| MRN | Patient.identifier | Yes |
| Date of Birth | Patient.birthDate | Yes |
| Gender | Patient.gender | Yes |
| Bed/Location | Encounter.location | No |
| Allergies | AllergyIntolerance | No |
| Diagnoses | Condition | No |

**Error Handling:**
- Patient not found: Allow manual entry
- Partial data: Import available, note missing
- Network error: Queue for retry, allow manual

#### INT-EHR-011: ADT Event Subscription
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Receive patient movement events |

**Events:**
- Admission (A01): Patient admitted to bed
- Transfer (A02): Patient moved to new location
- Discharge (A03): Patient leaving
- Update (A08): Demographics updated

**Behavior:**
- Device subscribes to events for assigned location
- Auto-suggest patient association on admission
- Alert on transfer/discharge

#### INT-EHR-012: Alarm Limit Import
| Attribute | Specification |
|-----------|---------------|
| Priority | P3 |
| Description | Import patient-specific alarm settings |

**Use Case:** Patient has known conditions requiring non-default limits

**Data:**
- Previous alarm configurations
- Clinical notes suggesting limits
- Condition-based presets

### 3.3 Data Export (Device to EHR)

#### INT-EHR-020: Vital Signs Export
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Export vital sign readings to EHR |

**Data Model: FHIR Observation**

```json
{
  "resourceType": "Observation",
  "status": "final",
  "category": [{
    "coding": [{
      "system": "http://terminology.hl7.org/CodeSystem/observation-category",
      "code": "vital-signs"
    }]
  }],
  "code": {
    "coding": [{
      "system": "http://loinc.org",
      "code": "8867-4",
      "display": "Heart rate"
    }]
  },
  "subject": {
    "reference": "Patient/123"
  },
  "effectiveDateTime": "2024-01-15T14:32:45Z",
  "valueQuantity": {
    "value": 72,
    "unit": "beats/minute",
    "system": "http://unitsofmeasure.org",
    "code": "/min"
  },
  "device": {
    "reference": "Device/monitor-001"
  }
}
```

**LOINC Codes for Vital Signs:**

| Parameter | LOINC Code | Display Name |
|-----------|------------|--------------|
| Heart Rate | 8867-4 | Heart rate |
| SpO2 | 2708-6 | Oxygen saturation |
| Systolic BP | 8480-6 | Systolic blood pressure |
| Diastolic BP | 8462-4 | Diastolic blood pressure |
| Temperature | 8310-5 | Body temperature |
| Respiratory Rate | 9279-1 | Respiratory rate |

#### INT-EHR-021: NIBP Measurement Export
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Export blood pressure measurements |

**Data Model: FHIR Observation (Panel)**

```json
{
  "resourceType": "Observation",
  "status": "final",
  "code": {
    "coding": [{
      "system": "http://loinc.org",
      "code": "85354-9",
      "display": "Blood pressure panel"
    }]
  },
  "component": [
    {
      "code": {"coding": [{"code": "8480-6"}]},
      "valueQuantity": {"value": 120, "unit": "mmHg"}
    },
    {
      "code": {"coding": [{"code": "8462-4"}]},
      "valueQuantity": {"value": 80, "unit": "mmHg"}
    }
  ]
}
```

#### INT-EHR-022: Alarm Event Export
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Export alarm events to EHR |

**Data Model Options:**
- FHIR Flag resource
- FHIR DetectedIssue resource
- Custom extension on Observation

**Exported Data:**
- Alarm time
- Parameter and value
- Priority
- Acknowledgment status
- Acknowledging user

#### INT-EHR-023: Session Events Export
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Export monitoring session events |

**Events:**
- Monitoring started (Encounter start)
- Monitoring ended (Encounter end)
- Patient associated/disassociated

### 3.4 Export Frequency and Batching

#### INT-EHR-030: Real-time Export
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Immediate export of critical data |

**Real-time Data:**
- Active alarms
- Alarm acknowledgments
- Patient admission/discharge

**Latency Target:** < 5 seconds

#### INT-EHR-031: Batch Export
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Periodic batch export of trend data |

**Batch Data:**
- Vital sign readings
- NIBP measurements

**Frequency:** Configurable (1-15 minutes, default 5 minutes)

**Batch Size:** Up to 100 observations per request

#### INT-EHR-032: Export Retry Logic
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Handling export failures |

**Retry Strategy:**
- Immediate retry (1x) on transient failure
- Exponential backoff for persistent failures
- Maximum retry period: 24 hours
- Data queued locally until successful export
- Alert if queue grows excessively

---

## 4. ABDM Integration

### 4.1 ABDM Overview

#### INT-ABD-001: ABDM Scope
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Ayushman Bharat Digital Mission integration |

**ABDM Components:**
- **ABHA (Ayushman Bharat Health Account):** Unique health ID for patients
- **HPR (Healthcare Professional Registry):** Provider identification
- **HFR (Health Facility Registry):** Facility identification
- **HIE-CM (Health Information Exchange - Consent Manager):** Consent management
- **HIP (Health Information Provider):** Data sharing role

**Device Role:** Health Information Provider (HIP)
- Links monitoring sessions to patient ABHA
- Shares health records per consent

### 4.2 ABHA Integration

#### INT-ABD-010: ABHA Patient Lookup
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Identify patient via ABHA number |

**Flow:**
1. User enters ABHA number (14 digits) or scans ABHA QR
2. Device queries ABDM registry
3. Patient demographics returned
4. User confirms patient identity
5. Session linked to ABHA

**API:** ABDM PHR APIs

**Data Retrieved:**
- Name
- Date of Birth
- Gender
- ABHA Address

#### INT-ABD-011: ABHA QR Code Scanning
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Scan patient's ABHA QR code |

**QR Format:** ABDM standard ABHA QR
**Camera:** Device camera or connected barcode scanner
**Fallback:** Manual ABHA number entry

### 4.3 Health Record Sharing

#### INT-ABD-020: Health Record Linking
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Link monitoring data to ABHA health record |

**Process:**
1. Patient provides consent via ABDM app
2. Consent notification received by device/HIP
3. Monitoring session data packaged as FHIR bundle
4. Data shared to HIE-CM
5. Available in patient's PHR

**FHIR Bundle Contents:**
- Vital sign Observations
- Encounter (monitoring session)
- DiagnosticReport (summary)

#### INT-ABD-021: Consent Management
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Handle patient consent for data sharing |

**Consent Flow:**
- Device displays consent request status
- Patient approves via ABDM mobile app
- Consent status reflected on device
- Data sharing proceeds only with valid consent

**Consent States:**
- Pending: Waiting for patient action
- Granted: Data sharing allowed
- Denied: No data sharing
- Expired: Consent period ended

### 4.4 ABDM Technical Requirements

#### INT-ABD-030: ABDM API Authentication
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Authentication with ABDM services |

**Requirements:**
- Health Facility registered in HFR
- Facility credentials (client ID, secret)
- OAuth 2.0 token-based authentication
- Token refresh handling

#### INT-ABD-031: ABDM Sandbox and Production
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Environment configuration |

**Environments:**
- Sandbox: For development and testing
- Production: Live ABDM infrastructure

**Configuration:**
- Environment selectable in settings
- Separate credentials per environment

---

## 5. Communication Protocols

### 5.1 FHIR Implementation

#### INT-PRT-001: FHIR Version
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | FHIR specification version |

**Version:** FHIR R4 (4.0.1)
**Rationale:** Current stable version, ABDM compatible

#### INT-PRT-002: FHIR Operations
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Supported FHIR interactions |

**Required Operations:**

| Operation | Use Case |
|-----------|----------|
| read | Retrieve patient by ID |
| search | Find patient by identifier |
| create | Create new Observation |
| batch | Submit multiple Observations |

**Search Parameters:**
- Patient: identifier, name
- Observation: patient, date, code

#### INT-PRT-003: FHIR Profiles
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | FHIR profiles for conformance |

**Profiles:**
- ABDM FHIR profiles (when available)
- International Patient Summary (IPS) vital signs
- US Core vital signs (for reference)

### 5.2 HL7 v2.x Support (Optional)

#### INT-PRT-010: HL7 v2.x Messages
| Attribute | Specification |
|-----------|---------------|
| Priority | P3 |
| Description | Legacy HL7 v2.x support |

**Supported Messages:**

| Message | Use |
|---------|-----|
| ADT^A01 | Admission notification |
| ADT^A02 | Transfer notification |
| ADT^A03 | Discharge notification |
| ORU^R01 | Observation result (vitals export) |

**Transport:** MLLP over TCP

### 5.3 API Design

#### INT-PRT-020: REST API Standards
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | RESTful API conventions |

**Standards:**
- HTTPS only
- JSON content type
- Standard HTTP status codes
- Rate limiting awareness
- Pagination support

#### INT-PRT-021: Error Handling
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | API error handling |

**Error Response Format:**

```json
{
  "error": {
    "code": "PATIENT_NOT_FOUND",
    "message": "No patient found with identifier 12345",
    "details": {}
  }
}
```

**Retry Logic:**
- 4xx errors: Do not retry (client error)
- 5xx errors: Retry with backoff
- Network errors: Retry with backoff

---

## 6. Integration Security

### 6.1 Authentication

#### INT-SEC-001: EHR API Authentication
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Authentication to EHR systems |

**Supported Methods:**
- OAuth 2.0 (client credentials flow)
- API key / secret
- Certificate-based (mTLS)

**Credential Storage:**
- Credentials stored encrypted
- No credentials in logs
- Credential rotation support

#### INT-SEC-002: ABDM Authentication
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Authentication to ABDM services |

**Method:** OAuth 2.0 per ABDM specification
**Flow:** Client credentials grant
**Token Management:** Automatic refresh before expiry

### 6.2 Authorization

#### INT-SEC-010: Data Access Scope
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Scope of data access |

**Principle:** Minimum necessary access
- Device requests only needed data
- Export only data for associated patients
- No bulk data access

#### INT-SEC-011: Consent Enforcement
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Enforce patient consent |

**Requirements:**
- ABDM data sharing requires valid consent
- Consent checked before each share
- Expired/revoked consent blocks sharing
- Consent status logged

### 6.3 Audit Trail

#### INT-SEC-020: Integration Audit
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Audit logging for integrations |

**Logged Events:**
- API calls (endpoint, method, response status)
- Data imports (what data, source)
- Data exports (what data, destination)
- Authentication events
- Consent changes

---

## 7. Offline Operation

### 7.1 Offline Capabilities

#### INT-OFL-001: Full Offline Operation
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Device works without network |

**Available Offline:**
- Patient monitoring (all parameters)
- Alarming
- Trend storage and display
- Manual patient entry
- All clinical functions

**Unavailable Offline:**
- EHR patient lookup
- ABHA lookup
- Data export (queued)
- Remote configuration

#### INT-OFL-002: Offline Data Queue
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Queue data during offline periods |

**Queue Behavior:**
- All exportable data queued locally
- Queue persists across reboots
- Queue capacity: 72+ hours of data
- Priority ordering maintained

### 7.2 Reconnection

#### INT-OFL-010: Automatic Sync on Reconnection
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Sync behavior when network restored |

**Behavior:**
1. Detect network availability
2. Authenticate with services
3. Export queued data (priority order)
4. Resume real-time operations
5. Clear synced items from queue

**Indicators:**
- Sync progress displayed
- Queue size indicator
- Sync completion notification

#### INT-OFL-011: Graceful Degradation
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Handle partial connectivity |

**Scenarios:**
- EHR available, ABDM unavailable: Continue EHR sync
- ABDM available, EHR unavailable: Continue ABDM sync
- Intermittent connectivity: Opportunistic sync

---

## 8. Configuration

### 8.1 Integration Configuration

#### INT-CFG-001: EHR Endpoint Configuration
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Configure EHR connection |

**Settings:**
- EHR type (FHIR, HL7 v2, custom)
- Server URL / hostname
- Port (if non-standard)
- Authentication method
- Credentials
- Timeout values

#### INT-CFG-002: ABDM Configuration
| Attribute | Specification |
|-----------|---------------|
| Priority | P1 |
| Description | Configure ABDM connection |

**Settings:**
- Environment (sandbox/production)
- Facility ID (HFR registered)
- Client credentials
- Callback URLs

#### INT-CFG-003: Export Configuration
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Configure data export behavior |

**Settings:**
- Export enabled (yes/no)
- Export frequency
- Data types to export
- Real-time vs. batch per data type

### 8.2 Discovery and Registration

#### INT-CFG-010: Service Discovery
| Attribute | Specification |
|-----------|---------------|
| Priority | P3 |
| Description | Automatic service discovery |

**Mechanisms:**
- mDNS/DNS-SD for local EHR
- Configuration server for enterprise setup
- Manual configuration fallback

#### INT-CFG-011: Device Registration
| Attribute | Specification |
|-----------|---------------|
| Priority | P2 |
| Description | Register device with hospital systems |

**Registration Data:**
- Device serial number
- Device type/model
- Software version
- Assigned location (bed/ward)
- Network address

---

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2024 | — | Initial draft |
