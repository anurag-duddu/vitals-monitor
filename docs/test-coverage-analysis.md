# Test Coverage Analysis — Vitals Monitor

**Date:** 2026-02-17
**Scope:** All source modules under `src/` and all tests under `tests/`

---

## Executive Summary

The vitals-monitor codebase has **35 source files** (11,522 lines) and **12 test files**
(3,521 unit + 1,844 integration lines). The **8 core data/logic modules** are unit-tested
and **4 integration suites** cover key cross-module flows. However, **14+ modules have
zero test coverage**, including the entire services layer, waveform generation, IPC
transport, and all UI code. The existing tests also leave specific error paths, boundary
conditions, and security-relevant code branches unexercised.

### Coverage at a Glance

| Category | Modules | Test Status |
|----------|---------|-------------|
| Core data/logic | alarm_engine, patient_data, auth_manager, audit_log, settings_store, sync_queue, fhir_client, trend_db | Unit tested + partial integration |
| Services | service_manager, sensor_service, alarm_service, network_service | **No tests** |
| Data generation | waveform_gen, mock_data | **No tests** |
| Providers | vitals_provider_mock, vitals_provider_ipc | **No tests** |
| IPC | ipc_transport | **No tests** |
| Drivers | sensor_hal, ota_update | **No tests** (stubs) |
| Interop | abdm_client, network_manager | **No tests** (stubs) |
| UI (8 screens + 4 widgets + themes) | 14 files | **No tests** |

---

## 1. Modules With No Test Coverage

### 1.1 Service Layer (High Priority)

These modules orchestrate the application's runtime behavior. They are non-trivial and
contain real logic that should be tested.

**`service_manager.c`** (327 lines) — Central service lifecycle coordinator.
- Contains a state machine (STOPPED → STARTING → RUNNING) with auto-restart logic
  and heartbeat monitoring.
- Functions to test: `service_manager_register()`, `start_all()`, `tick()`, `stop_all()`
- Key untested paths: duplicate registration rejection, `SERVICE_MAX_RESTARTS` exceeded,
  init failure propagation, tick with uninitialized manager, heartbeat expiry triggering
  restart.

**`alarm_service.c`** (197 lines) — Alarm evaluation orchestration.
- Wraps alarm_engine and vitals_provider into a service.
- Key untested path: `alarm_service_tick()` when `vitals_provider_get_current(0)` returns
  NULL (no vitals available). This is a safety-relevant path — if the sensor disconnects,
  alarms must degrade gracefully.

**`sensor_service.c`** (188 lines) — Sensor data polling.
- Wraps vitals_provider in a service interface with heartbeat tracking.
- Key untested paths: failure propagation from `vitals_provider_init()`, double
  start/stop cycles, tick after stop.

**`network_service.c`** (199 lines) — Network monitoring.
- Wraps network_manager. Mostly stubs, but still has lifecycle logic worth testing.

**Recommendation:** Add a `test_service_manager.c` unit test suite. The service_manager
has the most complex logic (state machine, restart counting, heartbeat). The thinner
service wrappers (alarm/sensor/network) can share a single test file or be tested via
integration tests that exercise the full service stack.

### 1.2 Waveform Generation (Medium Priority)

**`waveform_gen.c`** (154 lines) — Synthetic ECG/Pleth waveform synthesis.
- Uses a Gaussian LUT and fixed-point (16.16) phase accumulator.
- Has no LVGL dependency — fully testable in the unit test harness.
- Key untested behaviors:
  - Phase accumulator wraparound correctness at varying heart rates
  - Extreme HR input (e.g., 0, 1, 300, 500 BPM) — could overflow `phase_inc`
  - Output range bounds (are samples always within expected amplitude?)
  - ECG vs. Pleth morphology differences

**Recommendation:** Add `test_waveform_gen.c`. This module has pure numerical logic with
no dependencies — ideal for unit testing. Verify output stays within expected ranges
across a sweep of HR values.

### 1.3 IPC Transport (Medium Priority)

**`ipc_transport.c`** (401 lines) — Pub/sub transport abstraction.
- In simulator mode, this is a stub with counters and logging. On target, it wraps
  nanomsg.
- Key untested behaviors: NULL parameter handling, endpoint string truncation at
  `IPC_ENDPOINT_MAX`, message type parsing, close-when-not-active.

**Recommendation:** Add `test_ipc_transport.c` for the simulator stub path. Verify
create/send/close lifecycle, counter increments, and NULL safety.

### 1.4 Vitals Providers (Lower Priority)

**`vitals_provider_mock.c`** (266 lines) — Mock vitals source for simulator.
- Dual-patient slot support, callback mechanism, LVGL timer-driven.
- Testing is complicated by the LVGL timer dependency, but the callback registration
  and slot isolation logic could be tested with a mocked timer.

**`vitals_provider_ipc.c`** (368 lines) — IPC-based vitals source for target hardware.
- Depends on nanomsg. Could be tested with a mock IPC transport.

### 1.5 UI Code (Lower Priority for Unit Tests)

All 14 UI files (screens, widgets, themes) have no tests. UI testing in an embedded
LVGL context is non-trivial and may not be worthwhile at the unit level. Consider:
- Testing the **theme/style logic** separately (color calculations, token lookups)
- Using the simulator for manual or screenshot-based regression testing
- Extracting testable logic (e.g., formatting functions) out of screen files

---

## 2. Gaps in Existing Tested Modules

### 2.1 Security-Critical Gaps

**SQL Injection (all SQLite-backed modules)**
The `test_failure_scenarios.c` integration test has a comment referencing "Security
testing (TEST-5.1)" but no actual SQL injection tests exist. Since this is a medical
device with CDSCO regulatory requirements, these should be tested:
- `patient_data_find_by_mrn("' OR '1'='1")` — MRN injection
- `audit_log_record()` with SQL metacharacters in the message field
- `settings_store_set_string()` with key containing SQL metacharacters
- `auth_manager_login()` with SQL in username field

**Auth PIN buffer boundary (`auth_manager.c:43-48`)**
The DJB2a hash function copies the PIN into a `salted` buffer. No test verifies behavior
with very long PIN strings or special characters. A malformed PIN should not cause a
buffer overflow.

**Permission matrix completeness (`auth_manager.c:73-134`)**
Tests spot-check a few role/permission pairs, but the full 4-role × N-permission matrix
is not exhaustively verified. For a medical device with role-based access control, every
combination should be asserted.

### 2.2 Error Handling & Robustness Gaps

**alarm_engine — evaluate before init (`alarm_engine.c:156-163`)**
Calling `alarm_engine_evaluate()` before `alarm_engine_init()` logs an error but the
test suite never exercises this path. For a safety-critical alarm system, this defensive
code should be proven to work.

**alarm_engine — unknown state recovery (`alarm_engine.c:410-416`)**
The default case in the state machine resets corrupted state to INACTIVE. This recovery
path is never tested.

**patient_data — transaction rollback (`patient_data.c:479-507`)**
`patient_data_associate()` uses BEGIN/COMMIT/ROLLBACK transactions. Tests only exercise
the happy path; the ROLLBACK path on step failure is never verified.

**patient_data — MRN uniqueness violation**
The schema defines a UNIQUE constraint on MRN, but no test attempts to insert a
duplicate MRN and verify the expected error behavior.

**sync_queue — crash recovery (`sync_queue.c:109-112`)**
On init, the queue resets SENDING→PENDING items (crash recovery). Tests don't exercise
this with multiple stuck items or items with high retry counts.

**fhir_client — JSON escape buffer exhaustion (`fhir_client.c:69`)**
Very long strings can exhaust the output buffer during JSON escaping. The truncation
path is not tested.

**trend_db — build failure**
The unit tests for trend_db currently **fail to compile** because `trend_db.h` includes
`theme_vitals.h` which pulls in `lvgl.h`. This is both a build config issue and a
design concern — core data modules should not depend on UI headers.

### 2.3 Boundary Condition Gaps

| Module | Missing Boundary Test | Why It Matters |
|--------|----------------------|----------------|
| alarm_engine | Temperature rounding at `36.74999f` / `36.75001f` | Float→int conversion could cause off-by-one alarm triggers |
| alarm_engine | `alarm_engine_silence_all(duration=0)` | Zero duration could cause immediate re-trigger or infinite silence |
| audit_log | Query at `AUDIT_LOG_QUERY_MAX` limit | Buffer overflow if limit is not enforced |
| audit_log | UTF-8 multibyte truncation in `copy_text_col()` | Corrupted characters in audit display |
| sync_queue | >16 pending items (static buffer in `process_items`) | Silent data loss if batch size exceeded |
| trend_db | Timestamp values > 2^31 (year 2038) | Aggregation queries may break |
| fhir_client | URL with mixed-case scheme (`HTTPS://`) | Validation bypass |

---

## 3. Integration Test Gaps

### 3.1 Missing Cross-Module Scenarios

**Dual-patient slot isolation (HIGH PRIORITY)**
The system supports two simultaneous patients (slot 0 and slot 1). No integration test
verifies that:
- Vitals from slot 0 don't appear in slot 1's trend data
- Alarm thresholds for slot 0 don't affect slot 1
- Discharging slot 0's patient doesn't disrupt slot 1

This is the most important missing integration test for patient safety.

**Alarm lifecycle with database persistence**
The existing `test_alarm_db_integration.c` only tests alarm triggering. Missing:
- Alarm de-escalation then re-escalation (e.g., HR 160→normal→180)
- Acknowledged alarm's DB record (no duplicate entries)
- Alarm severity changes while alarm is active
- `trend_db_purge_old_data()` running while alarms are active

**Auth + audit completeness**
The existing `test_auth_audit_integration.c` covers login/logout. Missing:
- Permission denial audit trail (attempt action without permission)
- User creation/deletion audit records
- Concurrent sessions from different users

**Full service stack integration**
No test exercises service_manager → sensor_service → alarm_service → trend_db as a
connected pipeline. This would verify that vitals flow from provider through alarm
evaluation to database storage.

### 3.2 Failure Scenario Gaps

The `test_failure_scenarios.c` suite is valuable but incomplete:
- No SQL injection vectors tested (see §2.1)
- No `INT_MIN` / `INT_MAX` boundary testing for alarm thresholds
- No timestamp overflow testing
- No test of operations-after-close for all modules (only a subset is covered)

---

## 4. CI/CD & Infrastructure Gaps

**Integration tests not run in CI.**
The GitHub Actions workflow only runs unit tests (`tests/unit`). The integration test
suite (`tests/integration`) is not executed in the pipeline.

**No code coverage measurement.**
The CMake configuration supports `-DENABLE_COVERAGE=ON` (gcov/lcov), but no CI job
generates or tracks coverage reports. Without quantitative coverage data, regressions
are invisible.

**No AddressSanitizer in CI.**
The CMake supports `-DENABLE_ASAN=ON`, but CI doesn't enable it. For a medical device
codebase, running tests under ASan would catch memory errors that could cause
patient-safety issues.

**cppcheck warnings non-blocking.**
The code-quality job uses `continue-on-error: true` and `--error-exitcode=0`, meaning
static analysis findings don't fail the build.

---

## 5. Prioritized Recommendations

### Tier 1 — Safety & Security (address first)

1. **Add SQL injection tests** to `test_failure_scenarios.c` for all SQLite-backed
   modules (patient_data, audit_log, auth_manager, settings_store, sync_queue, trend_db).

2. **Add dual-patient slot isolation integration test** — verify vitals, alarms, and
   trend data don't cross between slot 0 and slot 1.

3. **Test alarm_engine defensive paths** — evaluate-before-init, NULL data, unknown
   state recovery. These are safety-critical code paths.

4. **Exhaustive permission matrix test** — verify all role/permission combinations in
   auth_manager for CDSCO compliance.

5. **Fix trend_db test build** — the `trend_db.h → theme_vitals.h → lvgl.h` dependency
   chain breaks unit test compilation. Refactor the header to remove the UI dependency
   from a core data module.

### Tier 2 — Reliability & Robustness

6. **Add `test_service_manager.c`** — test lifecycle state machine, heartbeat monitoring,
   auto-restart limits, and failure propagation.

7. **Add `test_waveform_gen.c`** — test phase accumulator correctness, extreme HR
   values, output range bounds.

8. **Test transaction rollback** in patient_data — verify ROLLBACK executes when a
   step in `patient_data_associate()` fails.

9. **Test crash recovery** in sync_queue — verify SENDING→PENDING reset works with
   multiple stuck items.

10. **Add boundary tests** — temperature rounding, audit query limits, sync_queue batch
    overflow, timestamp > 2^31.

### Tier 3 — CI/CD Infrastructure

11. **Run integration tests in CI** — add an `integration-tests` job to `build.yml`.

12. **Enable code coverage reporting** — add a CI job with `-DENABLE_COVERAGE=ON` and
    publish lcov results.

13. **Enable AddressSanitizer in CI** — run both unit and integration tests with
    `-DENABLE_ASAN=ON`.

14. **Make cppcheck blocking** — change `--error-exitcode=0` to `--error-exitcode=1`
    once existing warnings are resolved.

### Tier 4 — Expanded Coverage

15. **Add `test_ipc_transport.c`** — test simulator stub lifecycle, counters, NULL
    safety.

16. **Add full service stack integration test** — vitals_provider → sensor_service →
    alarm_service → trend_db pipeline.

17. **Extract testable UI logic** — formatting functions, color calculations, and
    state management that can be tested without LVGL rendering.

---

## Appendix: File-Level Coverage Map

```
src/core/
  alarm_engine.c        (511 lines)  ✅ Unit + Integration
  patient_data.c        (577 lines)  ✅ Unit + Integration
  auth_manager.c        (754 lines)  ✅ Unit + Integration
  audit_log.c           (333 lines)  ✅ Unit + Integration
  trend_db.c            (446 lines)  ⚠️  Unit test exists but FAILS TO BUILD
  settings_store.c      (305 lines)  ✅ Unit only
  sync_queue.c          (457 lines)  ✅ Unit only
  fhir_client.c         (588 lines)  ✅ Unit only
  waveform_gen.c        (154 lines)  ❌ No tests
  mock_data.c           (350 lines)  ❌ No tests
  vitals_provider_mock.c(266 lines)  ❌ No tests
  vitals_provider_ipc.c (368 lines)  ❌ No tests
  network_manager.c     (146 lines)  ❌ No tests (stub)
  abdm_client.c         (282 lines)  ❌ No tests (stub)

src/services/
  service_manager.c     (327 lines)  ❌ No tests
  alarm_service.c       (197 lines)  ❌ No tests
  sensor_service.c      (188 lines)  ❌ No tests
  network_service.c     (199 lines)  ❌ No tests

src/common/ipc/
  ipc_transport.c       (401 lines)  ❌ No tests

src/drivers/
  sensor_hal.c           (62 lines)  ❌ No tests (stub)
  ota_update.c           (59 lines)  ❌ No tests (stub)

src/ui/ (14 files, ~3,553 lines)    ❌ No tests
```
