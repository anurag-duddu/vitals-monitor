# Configuration Management Plan

| Field          | Value                                      |
|----------------|--------------------------------------------|
| Document ID    | CMP-001                                    |
| Version        | 1.0                                        |
| Date           | 2026-02-15                                 |
| Author         | Engineering Team                           |
| Reviewer       | Quality Assurance                          |
| Approval       | Regulatory Affairs                         |
| Classification | IEC 62304 Software Safety Class B          |

---

## 1. Purpose and Scope

This document defines the configuration management strategy for the Bedside Vitals Monitor software, per IEC 62304 clause 8 (Software Configuration Management). It covers version control, branching strategy, build reproducibility, release identification, and configuration item management.

---

## 2. Configuration Items

The following items are under configuration management:

| Category             | Items                                                      | Repository Location |
|----------------------|------------------------------------------------------------|---------------------|
| Application source   | All `.c` and `.h` files in `src/`                          | `src/`              |
| UI screens           | Screen implementation and headers                          | `src/ui/screens/`   |
| UI widgets           | Widget implementation and headers                          | `src/ui/widgets/`   |
| Core modules         | alarm_engine, patient_data, settings_store, auth_manager, audit_log, trend_db, fhir_client, sync_queue, mock_data | `src/core/` |
| Service stubs        | service_manager, sensor_service, alarm_service, network_service | `src/services/` |
| IPC layer            | ipc_transport, ipc_messages                                | `src/common/ipc/`   |
| Drivers              | sensor_hal, ota_update                                     | `src/drivers/`      |
| Build configuration  | CMakeLists.txt files                                       | `simulator/`, `tests/unit/`, `tests/integration/` |
| Buildroot external   | Package definitions, kernel config, genimage, overlays     | `buildroot-external/` |
| Security configs     | AppArmor profiles, dm-verity scripts, LUKS config          | `deploy/security/`  |
| Test source          | Unit and integration test files                            | `tests/unit/`, `tests/integration/` |
| Regulatory docs      | All IEC 62304 compliance documents                         | `docs/regulatory/`  |
| SOUP components      | SQLite amalgamation, LVGL (submodule or vendored copy)     | Managed via Buildroot or vendored |

---

## 3. Version Control System

| Attribute           | Value                                              |
|---------------------|----------------------------------------------------|
| Tool                | Git                                                |
| Hosting             | GitHub                                             |
| Repository          | vitals-monitor (private)                           |
| Primary branch      | `main`                                             |
| Branch protection   | Requires passing CI (all 563 tests), code review   |

---

## 4. Branching Strategy

| Branch Type     | Naming Convention      | Purpose                                   | Lifetime        |
|-----------------|------------------------|-------------------------------------------|-----------------|
| Main            | `main`                 | Production-ready code; always builds and passes all tests | Permanent |
| Feature         | `feature/<description>`| New feature development                   | Until merged    |
| Bugfix          | `fix/<description>`    | Defect resolution                         | Until merged    |
| Release         | `release/vX.Y.Z`       | Release stabilization and tagging         | Until tagged    |
| Hotfix          | `hotfix/<description>` | Critical production fixes                 | Until merged    |

### 4.1 Merge Rules

- All branches merge to `main` via pull request.
- Pull requests require at least one code review approval.
- All 563 automated tests (403 unit + 160 integration) must pass before merge.
- Safety-relevant changes require additional review by Quality Assurance.

---

## 5. Build Reproducibility

### 5.1 Simulator Build

```
cd simulator/build
cmake ..
cmake --build . -j8
```

Build configuration is fully determined by `simulator/CMakeLists.txt` and the Git commit hash. The `SIMULATOR_BUILD=1` define enables mock data and IPC stubs.

### 5.2 Unit Test Build

```
cd tests/unit/build
cmake ..
cmake --build . -j8
./test_runner
```

### 5.3 Integration Test Build

```
cd tests/integration/build
cmake ..
cmake --build . -j8
./integration_test_runner
```

### 5.4 Target (Production) Build

```
cd buildroot-external
make BR2_EXTERNAL=$PWD -C /path/to/buildroot vitals_monitor_defconfig
make -C /path/to/buildroot
```

The Buildroot external tree locks all SOUP versions via package `.mk` files and `BR2_PACKAGE_*_VERSION` variables. The `genimage` configuration produces a reproducible SD card image.

### 5.5 Build Identification

Each build is identified by:
- Git commit hash (full SHA-1)
- Git tag (for releases: `vX.Y.Z`)
- Build timestamp
- Compiler version and host OS

This information is embedded in the firmware image and accessible via the settings screen.

---

## 6. Release Management

### 6.1 Release Identification

| Attribute        | Format                 | Example        |
|------------------|------------------------|----------------|
| Version number   | Semantic versioning    | v1.0.0         |
| Git tag          | `vX.Y.Z`              | v1.0.0         |
| Release branch   | `release/vX.Y.Z`      | release/v1.0.0 |

### 6.2 Release Checklist

Prior to each release, the following must be verified:

- [ ] All 563 automated tests pass (403 unit + 160 integration)
- [ ] No unresolved critical or high defects
- [ ] Traceability matrix (RTM-001) updated
- [ ] Risk analysis (RA-001) current
- [ ] SOUP list (SOUP-001) current
- [ ] Release notes prepared
- [ ] Regulatory documents reviewed and approved
- [ ] Git tag created and pushed
- [ ] Build artifact archived

### 6.3 Release Archive

Release artifacts are archived in a controlled location and include:
- Source code snapshot (Git tag)
- Build output (firmware image)
- Test execution results
- Release notes
- Regulatory document versions at time of release

---

## 7. Change Control

### 7.1 Change Request Process

1. Change request filed as GitHub Issue with description, rationale, and impact assessment.
2. Safety impact evaluated per Risk Analysis (RA-001).
3. Change approved by Engineering Team lead; safety-relevant changes additionally approved by Quality Assurance.
4. Implementation on feature/fix branch.
5. Code review and automated test verification.
6. Merge to main via pull request.
7. Traceability matrix updated if requirements affected.

### 7.2 Emergency Changes

Critical safety defects may bypass the standard feature branch process but must still be:
- Code reviewed (expedited)
- Regression tested (all 563 automated tests)
- Documented with rationale for expedited process
- Retroactively added to the change control record

---

## 8. Backup and Recovery

| Item                    | Backup Method                      | Frequency    |
|-------------------------|------------------------------------|--------------|
| Git repository          | GitHub cloud hosting + local clones| Continuous   |
| Build artifacts         | Release archive (controlled location) | Per release |
| Test results            | Stored alongside release archive   | Per release  |
| Regulatory documents    | Git-tracked in `docs/regulatory/`  | Per commit   |

---

## 9. Referenced Documents

| Document ID | Title                               |
|-------------|-------------------------------------|
| SDP-001     | Software Development Plan           |
| SRS-001     | Software Requirements Specification |
| RTM-001     | Requirements Traceability Matrix    |
| RA-001      | Risk Analysis                       |
| SOUP-001    | SOUP List                           |
| MTP-001     | Maintenance Plan                    |

---

## Revision History

| Version | Date       | Author           | Changes                                     |
|---------|------------|------------------|---------------------------------------------|
| 1.0     | 2026-02-15 | Engineering Team | Initial configuration management plan        |
