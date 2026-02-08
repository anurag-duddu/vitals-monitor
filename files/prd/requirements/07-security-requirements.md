# Security Requirements: Bedside Vitals Monitor

## Document Information

| Field | Value |
|-------|-------|
| Document ID | REQ-007 |
| Version | 1.0 |
| Status | Draft |
| Parent Document | REQ-001 Product Overview |

---

## 1. Overview

This document specifies the security requirements for the bedside vitals monitor. As a medical device handling protected health information (PHI), security is a critical quality attribute that affects patient safety, data privacy, and regulatory compliance.

### 1.1 Security Objectives

The security architecture addresses five core objectives:

| Objective | Description |
|-----------|-------------|
| **Confidentiality** | Patient data is accessible only to authorized users and systems |
| **Integrity** | Data and software cannot be modified by unauthorized parties |
| **Availability** | The device remains operational and accessible for patient care |
| **Authenticity** | Users and systems are verified before access is granted |
| **Accountability** | All actions are logged and attributable to specific users |

### 1.2 Threat Model

The device must defend against the following threat categories:

| Threat Category | Examples | Risk Level |
|-----------------|----------|------------|
| Physical Access | Unauthorized user at device, USB attacks, theft | High |
| Network Attack | Man-in-middle, eavesdropping, malicious endpoints | High |
| Software Tampering | Malware installation, configuration manipulation | Critical |
| Data Breach | Patient data exfiltration, unauthorized access | High |
| Denial of Service | Resource exhaustion, network flooding | Medium |
| Insider Threat | Malicious staff, credential sharing | Medium |

### 1.3 Security Principles

1. **Defense in Depth**: Multiple layers of security controls
2. **Least Privilege**: Minimum access required for each function
3. **Secure by Default**: Security enabled out of the box
4. **Fail Secure**: Security maintained during failures
5. **Auditability**: All security-relevant events logged

---

## 2. Authentication

### 2.1 User Authentication Methods

The device supports multiple authentication methods appropriate for clinical settings:

| Method | Use Case | Priority |
|--------|----------|----------|
| PIN | Quick access for routine tasks | Must |
| Badge/RFID | Faster access, integrated with hospital ID | Must |
| PIN + Badge | High-security actions (configuration changes) | Should |

### 2.2 PIN Authentication

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| AUTH-PIN-001 | Support 4-8 digit numeric PIN | Must |
| AUTH-PIN-002 | Minimum PIN length: 4 digits | Must |
| AUTH-PIN-003 | Enforce PIN complexity (no sequential, no repeated) | Should |
| AUTH-PIN-004 | Hash PINs using bcrypt or Argon2 | Must |
| AUTH-PIN-005 | Never store plaintext PINs | Must |
| AUTH-PIN-006 | Mask PIN entry on screen | Must |
| AUTH-PIN-007 | Rate limit failed attempts (3 failures = 30 second lockout) | Must |
| AUTH-PIN-008 | Account lockout after 10 consecutive failures | Must |
| AUTH-PIN-009 | Administrator can reset locked accounts | Must |
| AUTH-PIN-010 | PIN change requires current PIN | Must |
| AUTH-PIN-011 | PIN expiration policy (configurable, default 90 days) | Should |

### 2.3 Badge/RFID Authentication

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| AUTH-BADGE-001 | Support ISO 14443A/B (NFC) badges | Must |
| AUTH-BADGE-002 | Support ISO 15693 badges | Should |
| AUTH-BADGE-003 | Map badge ID to user account | Must |
| AUTH-BADGE-004 | Support encrypted badge data | Should |
| AUTH-BADGE-005 | Reject unregistered badges | Must |
| AUTH-BADGE-006 | Display badge read confirmation | Must |
| AUTH-BADGE-007 | Badge tap within 3 seconds of prompt | Must |

### 2.4 Session Management

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| AUTH-SESS-001 | Automatic session timeout (configurable, default 5 minutes) | Must |
| AUTH-SESS-002 | Display timeout warning before logout | Should |
| AUTH-SESS-003 | Manual logout option always available | Must |
| AUTH-SESS-004 | Session extends with user activity | Must |
| AUTH-SESS-005 | Screen lock does not affect monitoring | Must |
| AUTH-SESS-006 | Alarms visible and audible during screen lock | Must |
| AUTH-SESS-007 | Quick re-authentication from lock screen | Must |
| AUTH-SESS-008 | Different timeout for admin vs clinical sessions | Should |

### 2.5 User Management

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| AUTH-USER-001 | Support local user accounts | Must |
| AUTH-USER-002 | Administrator can create/modify/delete users | Must |
| AUTH-USER-003 | Unique user ID per account | Must |
| AUTH-USER-004 | Display name for audit logs | Must |
| AUTH-USER-005 | Support user account deactivation (not deletion) | Must |
| AUTH-USER-006 | Integration with hospital directory (LDAP/AD) | Should |
| AUTH-USER-007 | Maximum 100 local user accounts | Must |

---

## 3. Authorization

### 3.1 Role-Based Access Control

The device implements role-based access control (RBAC) with predefined roles:

| Role | Description | Typical User |
|------|-------------|--------------|
| Clinical | Full patient monitoring, acknowledge alarms | Nurse, Physician |
| Clinical Read-Only | View vitals and trends, no configuration | Medical student |
| Biomedical | Device configuration, network setup, logs | Biomed engineer |
| Administrator | User management, all settings, factory reset | IT admin |
| Service | Diagnostics, calibration, firmware update | Manufacturer |

### 3.2 Permission Matrix

| Permission | Clinical | Read-Only | Biomedical | Admin | Service |
|------------|----------|-----------|------------|-------|---------|
| View vitals | ✓ | ✓ | ✓ | ✓ | ✓ |
| View trends | ✓ | ✓ | ✓ | ✓ | ✓ |
| Associate patient | ✓ | — | — | ✓ | — |
| Acknowledge alarm | ✓ | — | — | ✓ | ✓ |
| Modify alarm limits | ✓ | — | — | ✓ | — |
| View audit logs | — | — | ✓ | ✓ | ✓ |
| Configure network | — | — | ✓ | ✓ | ✓ |
| Configure EHR | — | — | ✓ | ✓ | — |
| Manage users | — | — | — | ✓ | — |
| Factory reset | — | — | — | ✓ | ✓ |
| Firmware update | — | — | — | — | ✓ |
| Calibration | — | — | — | — | ✓ |

### 3.3 Authorization Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| AUTHZ-001 | Enforce role permissions on all actions | Must |
| AUTHZ-002 | Deny access by default | Must |
| AUTHZ-003 | Log authorization failures | Must |
| AUTHZ-004 | Display "access denied" for unauthorized actions | Must |
| AUTHZ-005 | Support custom role creation | Should |
| AUTHZ-006 | Support user assignment to multiple roles | Should |
| AUTHZ-007 | Role changes take effect immediately | Must |

---

## 4. Audit Logging

### 4.1 Audit Events

The device shall log all security-relevant events:

#### 4.1.1 Authentication Events

| Event | Data Logged | Priority |
|-------|-------------|----------|
| Login success | User ID, timestamp, method | Must |
| Login failure | Attempted user ID, timestamp, method, reason | Must |
| Logout | User ID, timestamp, reason (manual/timeout) | Must |
| Account lockout | User ID, timestamp | Must |
| Password/PIN change | User ID, timestamp | Must |

#### 4.1.2 Authorization Events

| Event | Data Logged | Priority |
|-------|-------------|----------|
| Access denied | User ID, timestamp, action attempted | Must |
| Role change | Admin user ID, affected user, old/new role | Must |
| Permission override | User ID, action, justification | Should |

#### 4.1.3 Patient Data Events

| Event | Data Logged | Priority |
|-------|-------------|----------|
| Patient associated | User ID, patient ID, timestamp | Must |
| Patient disassociated | User ID, patient ID, timestamp | Must |
| Patient data viewed | User ID, patient ID, data type, timestamp | Should |
| Data exported | User ID, patient ID, destination, timestamp | Must |

#### 4.1.4 Alarm Events

| Event | Data Logged | Priority |
|-------|-------------|----------|
| Alarm triggered | Patient ID, parameter, value, limit, priority, timestamp | Must |
| Alarm acknowledged | User ID, alarm ID, timestamp | Must |
| Alarm silenced | User ID, alarm ID, duration, timestamp | Must |
| Alarm limits changed | User ID, patient ID, parameter, old/new limits, timestamp | Must |

#### 4.1.5 Configuration Events

| Event | Data Logged | Priority |
|-------|-------------|----------|
| Setting changed | User ID, setting name, old/new value, timestamp | Must |
| Network configuration | User ID, changes made, timestamp | Must |
| User created/modified | Admin user ID, affected user, changes, timestamp | Must |
| Firmware updated | User ID, old/new version, timestamp | Must |

#### 4.1.6 System Events

| Event | Data Logged | Priority |
|-------|-------------|----------|
| Device startup | Timestamp, firmware version | Must |
| Device shutdown | Timestamp, reason | Must |
| Time synchronization | Old/new time, source, timestamp | Must |
| Error/exception | Error type, details, timestamp | Must |
| Sensor disconnection | Sensor type, timestamp | Must |

### 4.2 Audit Log Requirements

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| AUDIT-001 | Log all events in section 4.1 | Must |
| AUDIT-002 | Timestamp precision: milliseconds | Must |
| AUDIT-003 | Timestamps in ISO 8601 format with timezone | Must |
| AUDIT-004 | Include device serial number in all logs | Must |
| AUDIT-005 | Logs tamper-evident (signed or hash-chained) | Must |
| AUDIT-006 | Minimum log retention: 90 days on device | Must |
| AUDIT-007 | Maximum log retention: 1 year (configurable) | Should |
| AUDIT-008 | Log rotation without data loss | Must |
| AUDIT-009 | Export logs to external system | Must |
| AUDIT-010 | Log export in standard format (syslog, JSON) | Must |
| AUDIT-011 | Logs cannot be deleted by non-service users | Must |
| AUDIT-012 | Log storage separate from main application | Must |
| AUDIT-013 | Continue logging if export fails | Must |

### 4.3 Audit Log Integrity

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| AUDIT-INT-001 | Hash chain each log entry | Must |
| AUDIT-INT-002 | Periodic integrity verification | Should |
| AUDIT-INT-003 | Alert on integrity violation detection | Must |
| AUDIT-INT-004 | Protect log storage from application code | Must |

---

## 5. Data Protection

### 5.1 Data Classification

| Classification | Examples | Protection |
|----------------|----------|------------|
| Critical | Encryption keys, credentials | Hardware-protected, never exported |
| Sensitive | Patient data, audit logs | Encrypted at rest and in transit |
| Internal | Configuration, device settings | Integrity protected |
| Public | Device model, firmware version | No special protection |

### 5.2 Encryption at Rest

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| CRYPT-REST-001 | Encrypt patient data partition | Must |
| CRYPT-REST-002 | Use AES-256 encryption | Must |
| CRYPT-REST-003 | Use LUKS or dm-crypt for partition encryption | Must |
| CRYPT-REST-004 | Encryption key stored in secure element/TPM | Should |
| CRYPT-REST-005 | Key derived from hardware-bound secret | Must |
| CRYPT-REST-006 | Encryption transparent to application | Must |
| CRYPT-REST-007 | Encrypt audit log partition | Must |
| CRYPT-REST-008 | Encrypt configuration data | Should |

### 5.3 Encryption in Transit

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| CRYPT-TRANS-001 | TLS 1.2 minimum for all network communication | Must |
| CRYPT-TRANS-002 | TLS 1.3 preferred | Should |
| CRYPT-TRANS-003 | Strong cipher suites only (AES-GCM, ChaCha20-Poly1305) | Must |
| CRYPT-TRANS-004 | Certificate validation for all endpoints | Must |
| CRYPT-TRANS-005 | Certificate pinning for known endpoints | Should |
| CRYPT-TRANS-006 | Reject self-signed certificates by default | Must |
| CRYPT-TRANS-007 | Support custom CA certificates | Must |

### 5.4 Key Management

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| KEY-001 | Generate cryptographic keys on device | Must |
| KEY-002 | Never export private keys | Must |
| KEY-003 | Use hardware RNG for key generation | Must |
| KEY-004 | Key rotation capability | Should |
| KEY-005 | Secure key storage (TEE/secure element preferred) | Should |
| KEY-006 | Key destruction on factory reset | Must |

---

## 6. System Integrity (Tamper-Proofing)

### 6.1 Secure Boot

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| BOOT-001 | Implement hardware root of trust | Must |
| BOOT-002 | Verify bootloader signature before execution | Must |
| BOOT-003 | Verify kernel signature before execution | Must |
| BOOT-004 | Verify device tree signature | Must |
| BOOT-005 | Chain of trust from ROM to application | Must |
| BOOT-006 | Halt boot on signature verification failure | Must |
| BOOT-007 | Display security failure indicator | Must |
| BOOT-008 | Log secure boot failures | Must |
| BOOT-009 | Support firmware rollback protection | Should |

### 6.2 Filesystem Integrity

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| FS-001 | Read-only root filesystem | Must |
| FS-002 | dm-verity for rootfs integrity verification | Must |
| FS-003 | Separate writable partition for data | Must |
| FS-004 | Squashfs for compressed read-only root | Should |
| FS-005 | Integrity check on every boot | Must |
| FS-006 | Halt boot on integrity failure | Must |
| FS-007 | Application binaries integrity verified | Must |

### 6.3 Runtime Protection

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| RUN-001 | Application runs as non-root user | Must |
| RUN-002 | Mandatory access control (SELinux or AppArmor) | Must |
| RUN-003 | Application sandboxing | Should |
| RUN-004 | Stack canaries enabled | Must |
| RUN-005 | ASLR (Address Space Layout Randomization) enabled | Must |
| RUN-006 | No executable stack | Must |
| RUN-007 | Watchdog timer for application health | Must |
| RUN-008 | Process capability restrictions | Should |

### 6.4 Physical Security

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| PHYS-001 | Disable debug interfaces (JTAG) in production | Must |
| PHYS-002 | Disable serial console in production | Must |
| PHYS-003 | USB ports restricted (no mass storage mount) | Must |
| PHYS-004 | No keyboard shortcuts to escape application | Must |
| PHYS-005 | Kiosk mode (application fullscreen, no shell access) | Must |
| PHYS-006 | Tamper detection (optional hardware) | Should |
| PHYS-007 | Secure erase on tamper detection | Should |

---

## 7. Secure Software Updates

### 7.1 Update Package Security

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| UPD-001 | Digitally sign all update packages | Must |
| UPD-002 | Verify signature before applying update | Must |
| UPD-003 | Use asymmetric cryptography (RSA-2048 or ECDSA P-256 minimum) | Must |
| UPD-004 | Reject unsigned updates | Must |
| UPD-005 | Version validation (no downgrades without override) | Must |
| UPD-006 | Integrity check (SHA-256 hash) | Must |

### 7.2 Update Process

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| UPD-PROC-001 | A/B partition scheme for atomic updates | Must |
| UPD-PROC-002 | Automatic rollback on boot failure | Must |
| UPD-PROC-003 | Maximum 3 boot attempts before rollback | Must |
| UPD-PROC-004 | Monitoring continues during update | Must |
| UPD-PROC-005 | Update requires service role authentication | Must |
| UPD-PROC-006 | Log update events (start, success, failure) | Must |
| UPD-PROC-007 | Display update progress | Should |

### 7.3 Update Sources

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| UPD-SRC-001 | USB drive update (offline) | Must |
| UPD-SRC-002 | Network update (OTA) | Should |
| UPD-SRC-003 | Certificate-pinned connection for OTA | Should |
| UPD-SRC-004 | Validate update server identity | Must |

---

## 8. Network Security

### 8.1 Network Hardening

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| NET-SEC-001 | No inbound ports open by default | Must |
| NET-SEC-002 | Outbound connections to configured endpoints only | Should |
| NET-SEC-003 | Firewall enabled | Must |
| NET-SEC-004 | Disable unused network services | Must |
| NET-SEC-005 | No telnet, FTP, or unencrypted protocols | Must |
| NET-SEC-006 | SSH disabled by default | Must |
| NET-SEC-007 | SSH key-based authentication only (if enabled) | Must |

### 8.2 WiFi Security

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| NET-WIFI-SEC-001 | WPA2-Enterprise or WPA3 required | Must |
| NET-WIFI-SEC-002 | No WEP or WPA-PSK | Must |
| NET-WIFI-SEC-003 | Secure credential storage | Must |
| NET-WIFI-SEC-004 | Certificate validation for RADIUS | Must |

### 8.3 DNS Security

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| NET-DNS-001 | DNS over TLS (DoT) support | Should |
| NET-DNS-002 | DNS cache poisoning protection | Should |

---

## 9. Vulnerability Management

### 9.1 Secure Development

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| VULN-DEV-001 | Static code analysis in build pipeline | Must |
| VULN-DEV-002 | Dependency vulnerability scanning | Must |
| VULN-DEV-003 | No known high/critical vulnerabilities at release | Must |
| VULN-DEV-004 | Security code review for critical components | Must |
| VULN-DEV-005 | Penetration testing before release | Should |

### 9.2 Vulnerability Response

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| VULN-RESP-001 | Documented vulnerability response process | Must |
| VULN-RESP-002 | Security patch release within 30 days (critical) | Should |
| VULN-RESP-003 | Security advisory communication to customers | Should |
| VULN-RESP-004 | CVE assignment for disclosed vulnerabilities | Should |

---

## 10. Incident Response

### 10.1 Detection

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| INC-DET-001 | Log security-relevant events (section 4) | Must |
| INC-DET-002 | Detect repeated authentication failures | Must |
| INC-DET-003 | Detect integrity violations | Must |
| INC-DET-004 | Alert on security events | Should |

### 10.2 Response Capabilities

| Requirement ID | Description | Priority |
|----------------|-------------|----------|
| INC-RESP-001 | Remote log retrieval capability | Should |
| INC-RESP-002 | Factory reset capability | Must |
| INC-RESP-003 | Secure data wipe on decommission | Must |
| INC-RESP-004 | Device isolation mode (network disconnect) | Should |

---

## 11. Compliance Mapping

### 11.1 IEC 62443 (Industrial Cybersecurity)

| IEC 62443 Requirement | Coverage |
|-----------------------|----------|
| Access control | Sections 2, 3 |
| Use control | Section 3 |
| Data integrity | Sections 5, 6 |
| Data confidentiality | Section 5 |
| Restrict data flow | Section 8 |
| Timely response to events | Section 10 |
| Resource availability | Section 6 |

### 11.2 NIST Cybersecurity Framework

| Function | Coverage |
|----------|----------|
| Identify | Section 1 (Threat Model) |
| Protect | Sections 2-8 |
| Detect | Section 10.1 |
| Respond | Section 10.2 |
| Recover | Section 7 (Rollback) |

### 11.3 HIPAA Security Rule (Reference)

While HIPAA is US regulation, security controls are aligned:

| Safeguard | Coverage |
|-----------|----------|
| Access controls | Sections 2, 3 |
| Audit controls | Section 4 |
| Integrity controls | Section 6 |
| Transmission security | Section 5.3 |
| Device and media controls | Sections 6.4, 10.2 |

---

## 12. Security Configuration Defaults

### 12.1 Authentication Defaults

| Setting | Default Value |
|---------|---------------|
| Minimum PIN length | 4 digits |
| Session timeout | 5 minutes |
| Failed login lockout threshold | 10 attempts |
| Lockout duration | 30 minutes |
| PIN expiration | 90 days |

### 12.2 Logging Defaults

| Setting | Default Value |
|---------|---------------|
| Log retention | 90 days |
| Log export format | JSON |
| Security event logging | Enabled |

### 12.3 Network Defaults

| Setting | Default Value |
|---------|---------------|
| Minimum TLS version | TLS 1.2 |
| SSH | Disabled |
| Inbound ports | None |

---

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2024 | — | Initial draft |
