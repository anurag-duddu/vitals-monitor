# Vitals Monitor -- Comprehensive Codebase Audit Report

**Date:** 2026-02-15
**Scope:** Full codebase audit covering security, reliability, performance, test quality, UI/UX, and build/deployment
**Application:** IEC 62304 Class B medical device software (bedside patient vital signs monitor)
**Target:** ARM Cortex-A7 (STM32MP157F-DK2), 800x480 display, Buildroot Linux

---

## Executive Summary

Six independent audit agents performed a thorough analysis of the entire codebase. The audit identified **~150 findings** across all categories.

| Category | CRITICAL | HIGH | MEDIUM | LOW | INFO | Total |
|---|---|---|---|---|---|---|
| Security | 1 | 7 | 9 | 5 | 2 | 24 |
| Reliability | 1 | 7 | 10 | 5 | 3 | 26 |
| Performance | 1 | 4 | 8 | 6 | 1 | 20 |
| Test Quality | 5 | 9 | 6 | 2 | 0 | 22 |
| UI/UX Reliability | 5 | 6 | 7 | 7 | 2 | 27 |
| Build & Deployment | 8 | 10 | 7 | 4 | 3 | 32 |
| **Totals** | **21** | **43** | **47** | **29** | **11** | **~151** |

### Top 10 Most Critical Issues (Immediate Action Required)

1. **DJB2a PIN hashing** -- non-cryptographic, trivially reversible for 4-digit PINs (SEC-AUTH-01)
2. **No mutex in IPC provider** -- LVGL corruption and display freeze on target build (REL-3.1)
3. **Alarm volume can be set to zero** -- violates IEC 60601-1-8 (UI-8.1)
4. **No signal quality display** -- HR=0 shows "0" instead of "NO SIGNAL" (UI-5.1)
5. **No alarm ACK button on main screen** -- violates IEC 60601-1-8 (UI-4.1)
6. **Screen stack growth** -- push() without pop causes stack overflow and UI freeze (UI-1.1)
7. **15+ modules have zero test coverage** -- blocks IEC 62304 certification (TEST-1.1)
8. **Placeholder root password + SSH enabled** -- remote root access on every device (BUILD-2.2, BUILD-2.3)
9. **Debug tools (gdb/strace) in production image** -- violates IEC 62443-4-2 (BUILD-2.1)
10. **All regulatory documents are templates** -- IEC 62304 submission not possible (BUILD-6.1)

### Positive Findings

- Zero `malloc`/`calloc`/`realloc` in application code -- excellent for safety-critical systems
- Consistent use of `snprintf` throughout -- no unsafe C string functions found
- SQLite prepared statements used in 5 of 6 modules -- good SQL injection prevention
- Well-designed AppArmor profiles exist (though not deployed)
- Comprehensive LUKS encryption design with hardware-bound keys
- Session timeout with NTP clock-backward safety handling
- Constant-time PIN comparison function (with minor caveats)
- `auto_del=true` consistently applied across screen transitions

---

## 1. Security Findings

### SEC-AUTH-01: DJB2a Hash Used for PIN Storage (CRITICAL)

- **File:** `src/core/auth_manager.c:29-47`
- **Description:** PINs are hashed using DJB2a, a non-cryptographic hash producing a 64-bit value stored as 16 hex characters. For 4-digit PINs (10,000 possibilities), the entire keyspace can be brute-forced in microseconds. The code comments acknowledge this: "simulator-only; target firmware will replace with Argon2 via mbedTLS" -- but no production implementation or compile guard exists.
- **Impact:** Any process with database read access can recover all user PINs instantly. This violates the project's own SRS requirement SRS-X003 which specifies Argon2 or bcrypt.
- **Fix:** Replace with Argon2id using per-user random salts. Add `#ifdef SIMULATOR_BUILD` guard around DJB2a so production builds fail to compile without the real implementation.

### SEC-AUTH-02: Hardcoded Default Credentials (HIGH)

- **File:** `src/core/auth_manager.c:189-194`
- **Description:** Four default users with trivial PINs are hardcoded: admin/1234, doctor/5678, nurse/0000, tech/9999. The seeding is gated behind `#ifdef SIMULATOR_BUILD` (line 351), but the `default_users[]` array itself is compiled into ALL builds -- PINs are visible in the binary's `.rodata` section.
- **Impact:** An attacker can extract default PINs from the binary. If `SIMULATOR_BUILD` is accidentally enabled in production, these credentials grant immediate admin access.
- **Fix:** Wrap the entire `default_users[]` array and `seed_default_users()` inside `#ifdef SIMULATOR_BUILD`. For production, require a first-boot provisioning step.

### SEC-AUTH-03: Fixed (Non-Random) Salt for PIN Hashing (HIGH)

- **File:** `src/core/auth_manager.c:23`
- **Description:** `#define PIN_SALT "vitals_monitor_2024_"` is a static string identical for every device and user. Two users sharing the same PIN produce identical hashes.
- **Impact:** Enables precomputed rainbow table attacks. One lookup table cracks every user on every device.
- **Fix:** Generate a cryptographically random 16+ byte salt per user, store it in the database alongside the hash.

### SEC-AUTH-04: No PIN Complexity Enforcement (MEDIUM)

- **File:** `src/core/auth_manager.c:547-574, 605-630`
- **Description:** Neither `add_user()` nor `change_pin()` validates PIN strength. Empty strings, single characters, or trivially weak PINs like "0000" are accepted.
- **Fix:** Enforce minimum 4-digit length, reject commonly used PINs (sequential, all-same digits).

### SEC-AUTH-05: Global (Not Per-User) Brute-Force Lockout (MEDIUM)

- **File:** `src/core/auth_manager.c:162-165, 388-412`
- **Description:** `failed_attempts` and `lockout_until_s` are global. An attacker can lock out ALL users by failing 5 attempts against any username -- a denial-of-service in a clinical environment.
- **Impact:** Could prevent authorized staff from accessing the device during a critical patient event.
- **Fix:** Track failed attempts per username in the SQLite `users` table. Consider progressive delays rather than hard lockout for a medical device.

### SEC-AUTH-06: No Permission Check on User Management Functions (MEDIUM)

- **File:** `src/core/auth_manager.c:547, 576, 605`
- **Description:** `add_user()`, `delete_user()`, and `change_pin()` do not internally verify `AUTH_PERM_MANAGE_USERS`. Authorization checks are only at the UI layer.
- **Fix:** Add `auth_manager_has_permission()` checks within the data layer functions (defense-in-depth).

### SEC-AUTH-07: pin_hash Exposed via list_users (MEDIUM)

- **File:** `src/core/auth_manager.c:632-644, 219-246`
- **Description:** `auth_manager_list_users()` returns full `auth_user_t` structs including `pin_hash`. Combined with DJB2a, any code listing users can trivially crack all PINs.
- **Fix:** Exclude `pin_hash` from the list query, or zero out the field in returned structs.

### SEC-AUTH-08: Session Timeout Can Be Set to Zero (LOW)

- **File:** `src/core/auth_manager.c:517-519`
- **Description:** `auth_manager_set_timeout()` accepts 0, which disables session timeout entirely.
- **Fix:** Enforce minimum timeout (e.g., 60 seconds).

### SEC-DB-01: Dynamic SQL Construction in Trend Query (HIGH)

- **File:** `src/core/trend_db.c:303-349`
- **Description:** `trend_db_query_param()` constructs SQL via `snprintf()`. Column names come from a hardcoded enum mapping (safe today), but the pattern is fragile and violates parameterized query principles.
- **Fix:** Use prepared statements with `sqlite3_bind_*()` for all dynamic values.

### SEC-DB-02: Patient Data Stored in Plaintext (MEDIUM)

- **File:** `src/core/patient_data.c`
- **Description:** Patient PII (name, MRN, DOB, allergies, diagnosis) stored in plaintext in SQLite. LUKS provides partition encryption, but any process on the unlocked device can read the data.
- **Fix:** Consider application-level encryption of sensitive fields.

### SEC-DB-03: No Database File Permission Setting (LOW)

- **File:** `src/core/patient_data.c:211` and all `sqlite3_open()` calls
- **Description:** SQLite default file creation mode is 0644 (world-readable). No explicit permissions set.
- **Fix:** Set `umask(0077)` before `sqlite3_open()`, or use `sqlite3_open_v2()` and `fchmod()`.

### SEC-MEM-01: FHIR JSON -- Unchecked Buffer Offset Growth (MEDIUM)

- **File:** `src/core/fhir_client.c:137-242`
- **Description:** Comma insertions (`buf[offset++] = ','`) lack bounds checks. A single-byte buffer overwrite is possible if the buffer is exactly full.
- **Fix:** Check `offset < buf_size - 1` before each `buf[offset++]`.

### SEC-MEM-02: FHIR JSON -- No Input Sanitization (JSON String Injection) (HIGH)

- **File:** `src/core/fhir_client.c:270-314`
- **Description:** Patient names, MRN, DOB, gender are inserted directly into JSON without escaping. A name containing `"` produces malformed JSON or enables injection.
- **Fix:** Implement a JSON string escape function per RFC 8259.

### SEC-MEM-03: IPC Waveform Sample Count Validation (MEDIUM)

- **File:** `src/core/vitals_provider_ipc.c:311-329`
- **Description:** `sample_count` is clamped to 50, but if the received message buffer has fewer samples, `memcpy` reads past the end.
- **Fix:** Validate `bytes >= header_size + sample_count * sizeof(int16_t)` before processing.

### SEC-INPUT-01: No Bounds Checking on Vital Sign Values from IPC (HIGH)

- **File:** `src/core/vitals_provider_ipc.c:269-308`
- **Description:** Vital sign values from IPC have no upper-bound validation. HR=32767 or SpO2=32767% would be accepted and propagated.
- **Impact:** Physiologically impossible values corrupt trend data, trigger incorrect alarms, and display misleading information.
- **Fix:** Add physiological range checks: HR (20-300), SpO2 (0-100), RR (0-80), Temp (25.0-45.0), NIBP Sys (20-300), NIBP Dia (10-200). Out-of-range values treated as invalid.

### SEC-INPUT-02: Alarm Engine Treats Zero as "No Signal" (LOW)

- **File:** `src/core/alarm_engine.c:291`
- **Description:** When `value == 0`, the alarm engine skips evaluation. A genuine asystole (HR=0) would NOT trigger an alarm.
- **Fix:** Separate "no signal" from "zero value" using a sentinel or quality flag.

### SEC-INPUT-03: FHIR Endpoint URL Not Validated (MEDIUM)

- **File:** `src/core/fhir_client.c:60-67`
- **Description:** Any string accepted as a FHIR URL, no HTTPS enforcement.
- **Fix:** Validate URL starts with `https://`.

### SEC-CRYPTO-01: No TLS Configuration in Application Code (HIGH)

- **File:** `src/core/fhir_client.c`, `abdm_client.c`, `network_manager.c`
- **Description:** No TLS/SSL configuration, certificate validation, or secure transport exists. When HTTP is implemented, there is no framework to prevent cleartext transmission of patient data.
- **Fix:** Create a TLS configuration module centralizing certificate loading, TLS 1.2+ enforcement, and certificate pinning.

### SEC-IPC-01: No Authentication Between IPC Processes (HIGH)

- **File:** `src/common/ipc/ipc_transport.c`, `ipc_messages.h`
- **Description:** Any process knowing the socket path can connect and inject false sensor data, subscribe to patient vitals, or send false alarm acknowledgments. No HMAC, sequence numbers, or credential verification.
- **Fix:** Add HMAC-based message authentication. Use Unix `SO_PEERCRED` for PID/UID verification.

### SEC-IPC-02: No Message Integrity Validation (MEDIUM)

- **File:** `src/common/ipc/ipc_transport.c:313-344`
- **Description:** No validation of `payload_len` vs received bytes, no `version` check, no CRC.
- **Fix:** Validate payload_len, verify protocol version, add CRC-32.

### SEC-IPC-03: IPC Sockets in /tmp Are World-Accessible (MEDIUM)

- **File:** `src/common/ipc/ipc_messages.h:208-211`
- **Description:** Sockets at `/tmp/vitals-monitor/` use default permissions in a world-writable directory.
- **Fix:** Use restrictive permissions (0750) and the `vitals` group. Consider abstract Unix socket namespace.

### SEC-MISC-01: Sensitive Information in Log Output (MEDIUM)

- **File:** `src/core/auth_manager.c:405,424`, `fhir_client.c:335-337`
- **Description:** Login failures distinguish "user not found" from "incorrect PIN" (enables username enumeration). FHIR client logs complete patient data JSON.
- **Fix:** Use generic "Authentication failed" message. Redact patient data from logs.

### SEC-MISC-02: Constant-Time Comparison May Be Optimized Away (LOW)

- **File:** `src/core/auth_manager.c:55-61`
- **Description:** `constant_time_cmp()` does not use `volatile` for the `result` variable. Compiler may optimize the loop.
- **Fix:** Declare `result` as `volatile unsigned char` or use a library function.

### SEC-MISC-03: ABDM Client Logs client_id (LOW)

- **File:** `src/core/abdm_client.c:127`
- **Fix:** Remove or redact credential from log output.

---

## 2. Reliability Findings

### REL-1.1: SQLite Init Failures Silently Disable Modules (HIGH)

- **File:** All 6 module `_init()` functions
- **Description:** Each module's init function calls `sqlite3_open()` and `sqlite3_exec()` for schema creation. If any step fails, the function returns without retrying or escalating. The module is non-functional for the rest of the session.
- **Impact:** A transient filesystem issue during boot permanently disables critical subsystems (patient data, authentication) requiring full system restart.
- **Fix:** Implement retry with exponential backoff (3 attempts, 100ms/500ms/1000ms). Fall back to `:memory:` where safe.

### REL-1.2: Multiple Modules Use PRAGMA synchronous=NORMAL (HIGH)

- **File:** All 6 database module init functions
- **Description:** WAL mode with `synchronous=NORMAL` may lose recent committed transactions on power failure.
- **Impact:** For the audit log, this creates a compliance gap. For sync queue, it may cause duplicate FHIR submissions.
- **Fix:** Use `PRAGMA synchronous=FULL` for audit_log specifically.

### REL-1.3: alarm_engine_evaluate() Silently Fails if Not Initialized (HIGH)

- **File:** `src/core/alarm_engine.c:155`
- **Description:** If `alarm_engine_init()` was never called or failed, `evaluate()` silently returns without processing any vital signs.
- **Fix:** Return a status code and generate a "ALARM SYSTEM INOPERATIVE" technical alarm.

### REL-2.1: No malloc in Critical Paths (INFO -- positive)

- **File:** All `src/` files
- **Description:** Confirmed zero dynamic allocations. All state is statically allocated.

### REL-2.2: Static Buffer Re-use Risk in settings_get_string() (MEDIUM)

- **File:** `src/core/settings_store.c:27-28, 214-226`
- **Description:** Returns a pointer to a single static buffer. Two consecutive calls overwrite the first result.
- **Fix:** Accept caller-provided buffer, or use two alternating static buffers.

### REL-2.3: Widget Pool Exhaustion Returns NULL Without Recovery (MEDIUM)

- **File:** `src/ui/widgets/widget_numeric_display.c:35-45`, `widget_waveform.c:29-40`, `widget_alarm_banner.c:43-52`
- **Description:** Pool `_alloc()` returns NULL when full. Screen creation functions don't check returns. A silent missing vital sign display is extremely dangerous.
- **Fix:** Check all creation return values. Log system error and show degraded-mode indicator.

### REL-2.4: LVGL Object Lifecycle -- Pool Reuse During Animation (LOW)

- **File:** `src/ui/widgets/*.c` free functions
- **Description:** During 200ms fade animation, freed pool slots could be reused while old LVGL objects still exist.
- **Fix:** Current alarm_banner correctly deletes flash timer in `_free()`. Document timing relationship.

### REL-2.5: SQLite Statement Finalization Is Complete (INFO -- positive)

- **File:** All `*_close()` functions
- **Description:** All prepared statements properly finalized.

### REL-3.1: No Mutex Protection in IPC Provider Callback Path (CRITICAL)

- **File:** `src/core/vitals_provider_ipc.c:269-309, 311-330`
- **Description:** In the target build, `process_vitals_message()` and `process_waveform_message()` run on dedicated pthreads. They directly write to shared state and invoke callbacks that update LVGL UI -- all without any mutex. LVGL is NOT thread-safe.
- **Impact:** Display corruption, crashes, or frozen displays. The monitor could freeze while a life-threatening alarm condition is present.
- **Fix:** Receiver threads must write to a lock-free ring buffer. The LVGL main loop polls this buffer during its tick. Add mutex around shared vital sign data.

### REL-3.2: volatile Qualifier Insufficient for Thread Synchronization (HIGH)

- **File:** `src/core/vitals_provider_ipc.c:45`
- **Description:** `static volatile bool g_threads_running` -- `volatile` does not guarantee atomic access or memory visibility across CPU cores in C99.
- **Fix:** Use C11 `_Atomic bool` or `__atomic_store_n()` builtins.

### REL-3.3: Static Buffers in trend_db Not Re-entrant (MEDIUM)

- **File:** `src/core/trend_db.c:331`
- **Description:** `static char abuf[32], nbuf[32], xbuf[32]` inside `trend_db_query_param()`.
- **Fix:** Use stack-allocated local buffers.

### REL-4.1: Alarm Engine Missing default Case in Switch (LOW)

- **File:** `src/core/alarm_engine.c:307`
- **Description:** `switch (s->state)` covers all four enum values but no `default`. Memory corruption would bypass all threshold checking.
- **Fix:** Add `default` case that resets to INACTIVE and logs critical error.

### REL-4.2: Screen Manager Does Not Validate Home Screen Registration (MEDIUM)

- **File:** `src/ui/screens/screen_manager.c:96-116`
- **Description:** `go_home()` hardcodes `SCREEN_ID_MAIN_VITALS` but doesn't check registration. Could leave a blank screen.
- **Fix:** Assert `screen_registered[SCREEN_ID_MAIN_VITALS]` is true.

### REL-4.3: OTA State Machine Is Stub-Only (INFO)

- **File:** `src/drivers/ota_update.c`
- **Description:** Entirely stubbed. Documented as requiring target implementation.

### REL-4.4: Alarm Silence Time uint32_t Overflow (LOW)

- **File:** `src/core/alarm_engine.c:223, 266`
- **Description:** `uint32_t` wraps at ~136 years. Negligible practical risk.

### REL-5.1: sensor_hal_register() NULL Dereference (MEDIUM)

- **File:** `src/drivers/sensor_hal.c:28`
- **Description:** Dereferences `hal->name` without checking if `hal` is NULL.
- **Fix:** Add `if (!hal) return;`.

### REL-5.2: alarm_engine_get_state() Returns Internal Pointer (LOW)

- **File:** `src/core/alarm_engine.c:186-188`
- **Description:** Returned `highest_message` pointer becomes invalid after next `evaluate()` call.
- **Fix:** Document lifetime constraint or copy message into a dedicated buffer.

### REL-5.3: auth_manager Signed Enum Comparison (LOW)

- **File:** `src/core/auth_manager.c:532-535`
- **Description:** `role < 0` may always be false if enum underlying type is unsigned.
- **Fix:** Cast to `int` before comparison.

### REL-5.4: mock_data_log_alarm() NULL Message Crash (LOW)

- **File:** `src/core/mock_data.c:250-266`
- **Description:** `strncpy` with NULL source is undefined behavior.
- **Fix:** Add `if (!message) message = "";`.

### REL-6.1: No Software Watchdog (HIGH)

- **File:** Entire codebase
- **Description:** No mechanism to detect a hung main loop. A frozen display showing stale vital signs is a critical failure.
- **Fix:** Implement software watchdog in a separate thread. For target, implement `sd_notify("WATCHDOG=1")`.

### REL-6.2: Service Manager Auto-Restart Has No Retry Limit (MEDIUM)

- **File:** `src/services/service_manager.c:244-260`
- **Description:** `restart_count` is incremented but never checked against a limit. Failing services restart indefinitely.
- **Fix:** Add maximum retry count (e.g., 5-10). Exponential backoff.

### REL-6.3: LVGL Timer Callbacks Block on SQLite (MEDIUM)

- **File:** `src/core/mock_data.c:170-176`
- **Description:** `trend_db_insert_sample()` called synchronously from LVGL timer context. Slow writes block rendering.
- **Fix:** Defer DB writes to a queue flushed outside the critical render path. Set `PRAGMA busy_timeout=100`.

### REL-7.1: No Transaction Wrapping on Multi-Step Operations (HIGH)

- **File:** `src/core/patient_data.c:469-496`, `sync_queue.c:268-345`, `settings_store.c:253-280`, `auth_manager.c:250-283`
- **Description:** Zero `BEGIN TRANSACTION`/`COMMIT`/`ROLLBACK` in the entire codebase. Power loss between `clear_slot` and `set_slot` loses a patient from the monitor.
- **Fix:** Wrap all multi-step operations in `BEGIN IMMEDIATE`/`COMMIT` transactions.

### REL-7.2: Six Separate Database Handles -- Potential SQLITE_BUSY (MEDIUM)

- **File:** All 6 modules with `static sqlite3 *db`
- **Description:** Six connections to the same file. No `busy_timeout` set, so concurrent access fails immediately.
- **Fix:** Share a single connection, or set `PRAGMA busy_timeout=5000` on all connections.

### REL-7.3: Sync Queue Items Stuck in SENDING State (HIGH)

- **File:** `src/core/sync_queue.c:312-314`
- **Description:** Items marked SENDING before export. If process crashes, they're stuck forever (never re-fetched by `get_pending`). Zombie items consume the 256-item capacity.
- **Fix:** On startup, reset items with `status=SENDING` and old timestamps back to PENDING.

### REL-8.1: Duplicate vitals_provider API Implementations (MEDIUM)

- **File:** `src/core/mock_data.c:285-348` and `src/core/vitals_provider_mock.c:47-266`
- **Description:** Both files define the full `vitals_provider_*` API. Linking both causes duplicate symbols.
- **Fix:** Remove embedded provider functions from `mock_data.c`.

### REL-8.2: DJB2a Not Guarded for Production (HIGH)

- **File:** `src/core/auth_manager.c:29-47`
- **Description:** No `#ifdef SIMULATOR_BUILD` guard prevents DJB2a from shipping in production.
- **Fix:** Add guard. In `#else` branch: `#error "Production build requires Argon2 PIN hashing"`.

---

## 3. Performance Findings

### PERF-1.1: Screen Recreation on Every Navigation (HIGH)

- **File:** `src/ui/screens/screen_manager.c:56-71, 80-93`
- **Description:** Every push/pop destroys the screen and rebuilds the entire LVGL widget tree (~40+ objects). Returning to main vitals causes a ~200ms+ gap with lost waveform data.
- **Fix:** Cache the main vitals screen object instead of destroying it. Use hide/show. Reduce fade animation to 50ms.

### PERF-1.2: Double Screen Creation on Nav Bar Press (HIGH)

- **File:** `src/ui/widgets/widget_nav_bar.c:181-187`
- **Description:** Non-home nav buttons call `go_home()` then `push(target)`. Main vitals screen is created, animated, then immediately destroyed -- entirely wasted work (~400ms).
- **Fix:** Add `screen_manager_replace()` or `go_home_then_push()` that skips intermediate screen creation.

### PERF-1.3: Waveform Chart -- Minor Invalidation Overhead (LOW)

- **File:** `src/ui/widgets/widget_waveform.c:113`
- **Description:** `lv_chart_set_next_value()` internally invalidates the chart. 8 invalidations per frame.
- **Fix:** Directly modify series y-array and call `lv_chart_refresh()` once.

### PERF-1.4: snprintf in Vitals Update Hot Path Without Change Detection (MEDIUM)

- **File:** `src/ui/screens/screen_main_vitals.c:183-216`
- **Description:** 5 `snprintf` + `lv_label_set_text()` calls every second regardless of value changes.
- **Fix:** Cache previous values, only update when changed.

### PERF-1.5: Alarm Banner Flash Resets All Styles Every Toggle (LOW)

- **File:** `src/ui/widgets/widget_alarm_banner.c:185-220`
- **Description:** 8 style writes per flash cycle (2 Hz for HIGH alarms = 16/sec).
- **Fix:** Track actual state change before applying styles.

### PERF-2.1: usleep Main Loop Timing -- FPS Drop (CRITICAL)

- **File:** `simulator/main.c:261-273`
- **Description:** `usleep(time_till_next * 1000)` does not account for time already spent processing. If processing takes 15ms and timer returns 33ms, total frame time = 48ms, dropping FPS from 30 to ~21.
- **Impact:** Waveform jitter. Irregular ECG display could be misinterpreted as arrhythmia (rendering artifact).
- **Fix:** Record time before handler, compute elapsed, sleep only remaining time. Cap sleep at 5ms minimum.

### PERF-3.1: Six Separate sqlite3_open Calls (HIGH)

- **File:** `simulator/main.c:224-231` and all 6 module init functions
- **Description:** Six connections, six page caches, six WAL readers. 6x memory, 6x startup I/O. WAL checkpointing delayed by six reader snapshots.
- **Fix:** Create shared database connection module. Open once, share handle.

### PERF-3.2: trend_db Re-Prepares SQL Every Query Call (HIGH)

- **File:** `src/core/trend_db.c:292-370`
- **Description:** `sqlite3_prepare_v2()` + `sqlite3_finalize()` on every call. 4 compile-execute-destroy cycles per 10-second refresh. On ARM, `prepare_v2` is the most expensive SQLite operation.
- **Fix:** Pre-compile statements at init time. Use `sqlite3_reset()` + `sqlite3_bind_*()` for re-execution.

### PERF-3.3: No Transaction Wrapping for Batch Purge (MEDIUM)

- **File:** `src/core/trend_db.c:423-444`
- **Description:** 4 separate DELETE statements without transaction. 4 WAL syncs instead of 1.
- **Fix:** Wrap in `BEGIN IMMEDIATE`/`COMMIT`.

### PERF-3.4: patient_data Missing Index on monitor_slot (MEDIUM)

- **File:** `src/core/patient_data.c:45-65, 281-285`
- **Description:** No index on `monitor_slot` or `active`. Full table scan for slot operations.
- **Fix:** `CREATE INDEX IF NOT EXISTS idx_patient_active ON patients(active, monitor_slot);`

### PERF-3.5: settings_load_defaults -- 28 Operations Without Transaction (LOW)

- **File:** `src/core/settings_store.c:253-264`
- **Description:** 14 `EXISTS` + 14 `INSERT` = 28 auto-commit operations at startup.
- **Fix:** Wrap in a transaction.

### PERF-3.6: Audit Log Sync Write on Every Alarm Transition (MEDIUM)

- **File:** `src/core/audit_log.c:215-239`
- **Description:** Synchronous SQLite INSERT in the vitals update callback. `SQLITE_TRANSIENT` forces extra memory copy.
- **Fix:** Use `SQLITE_STATIC`. Buffer audit writes and flush in a separate timer.

### PERF-4.1: No malloc in Application Code (INFO -- positive)

### PERF-4.2: Large Static Buffers on Stack -- Stack Overflow Risk (MEDIUM)

- **File:** `src/ui/screens/screen_audit_log.c:289`, `src/core/sync_queue.c:277`
- **Description:** ~19 KB (`audit_query_result_t tmp`) and ~66 KB (`sync_queue_item_t items[16]`) on the stack. ARM embedded typical stack size is 8-32 KB.
- **Impact:** Immediate stack overflow and hard crash on target.
- **Fix:** Make these static (file-scope) instead of local.

### PERF-5.1: Alarm Engine O(1) Per Parameter (LOW -- positive)

### PERF-5.2: Waveform LUT Generation One-Time Cost (LOW -- acceptable)

### PERF-5.3: Audit Log Filter -- Multiple Sequential DB Queries (MEDIUM)

- **File:** `src/ui/screens/screen_audit_log.c:282-397`
- **Description:** Up to 5 separate queries per filter, unsorted after merge. `lv_obj_clean()` destroys and recreates 100 rows.
- **Fix:** Single `WHERE event IN (...)` query. Use diff approach for UI updates.

### PERF-5.4: Trend Chart Refresh -- 7 DB Queries + Layout Update (MEDIUM)

- **File:** `src/ui/screens/screen_trends.c:468-479`
- **Description:** 6 SQLite queries + 4 chart refreshes + 1 forced layout every 10 seconds. Could cause 50-100ms stall.
- **Fix:** Stagger queries across multiple timer callbacks. Use prepared statements.

### PERF-6.1: Target Build IPC -- Blocking nn_recv (MEDIUM)

- **File:** `src/common/ipc/ipc_transport.c:313-344`
- **Description:** `nn_recv()` blocks up to 100ms. If called from LVGL main loop, catastrophic for 30 FPS rendering.
- **Fix:** Never call from LVGL thread. Use separate receiver thread with lock-free queue.

### PERF-7.1: Serial Database Initialization of 6 Modules (HIGH -- covered by PERF-3.1)

### PERF-7.2: Initial Screen Load Uses 200ms Fade Animation (MEDIUM)

- **File:** `src/ui/screens/screen_manager.c:156`
- **Fix:** Use `LV_SCR_LOAD_ANIM_NONE` for initial load.

### PERF-7.3: 341 printf Calls -- Slow on Serial Console (LOW)

- **File:** All source files
- **Description:** On ARM with 9600 baud serial, ~30 startup messages add 3-5 seconds.
- **Fix:** Compile-time disable in release builds via log-level macros.

### PERF-8.1: localtime() Not Thread-Safe (MEDIUM)

- **File:** `simulator/main.c:122-125`
- **Fix:** Use `localtime_r()`. Cache time string, update only when minute changes.

---

## 4. Test Quality Findings

### TEST-1.1: 15+ Core/Driver/Service Modules Completely Untested (CRITICAL)

- **Files:** `network_manager.c`, `fhir_client.c`, `abdm_client.c`, `sync_queue.c`, `waveform_gen.c`, `vitals_provider_ipc.c`, `sensor_hal.c`, `ota_update.c`, `service_manager.c`, all 4 services, `ipc_transport.c`
- **Description:** Only 5 of ~20+ production modules have any unit tests. ~75% of source modules have zero coverage.
- **Impact:** Blocks IEC 62304 certification (clause requires documented verification of all software units).
- **Fix:** Write unit tests for every untested module. Prioritize: `fhir_client`, `sync_queue`, `sensor_hal`, `ota_update`, `service_manager`.

### TEST-1.2: trend_db Has No Direct Unit Tests (HIGH)

- **File:** `src/core/trend_db.c`
- **Description:** Only tested indirectly through integration. No tests for aggregation, purge, long-range query, NULL handling.
- **Fix:** Create `tests/unit/test_trend_db.c`.

### TEST-2.1: Brute-Force Lockout Feature Has Zero Tests (CRITICAL)

- **File:** `src/core/auth_manager.c:158-166, 388-394`
- **Description:** Recently added lockout (`MAX_FAILED=5`, `LOCKOUT_DURATION=300s`) and constant-time comparison have zero test coverage. Static lockout state is never reset between tests.
- **Fix:** Add tests for lockout trigger, expiry, reset on success, and state isolation between tests.

### TEST-2.2: SIMULATOR_BUILD Not Defined in Test Builds (CRITICAL)

- **File:** `tests/unit/CMakeLists.txt`, `tests/integration/CMakeLists.txt`
- **Description:** Neither test CMakeLists.txt defines `SIMULATOR_BUILD`. `auth_manager_init()` wraps `seed_default_users()` in `#ifdef SIMULATOR_BUILD`. Without the define, test databases have empty user tables and ALL 32 auth-related tests should fail on a clean rebuild.
- **Impact:** Latent regression. Current builds may pass only due to stale build artifacts.
- **Fix:** Add `add_definitions(-DSIMULATOR_BUILD=1)` to both test CMakeLists.txt files.

### TEST-2.3: Static State Leaks Between Tests (HIGH)

- **File:** All test files
- **Description:** `failed_attempts` and `lockout_until_s` in auth_manager are never reset by `init()`/`close()`. Failed logins in one test accumulate and trigger lockout in later tests.
- **Fix:** Reset all static state in `auth_manager_init()`.

### TEST-2.4: No Boundary Value Tests for Alarm Thresholds (HIGH)

- **File:** `tests/unit/test_alarm_engine.c`
- **Description:** No tests for exactly-at-threshold values, negative values, INT_MAX overflow, SpO2=0, RR=0, temp=0.0, temperature float precision.
- **Fix:** Add boundary tests for each parameter at each threshold (exact, -1, +1).

### TEST-2.5: Tests Use NULL for DB Path Inconsistently (MEDIUM)

- **Description:** Unit tests pass `NULL`, integration tests pass `":memory:"`. No test verifies NULL mapping.
- **Fix:** Standardize on one approach.

### TEST-2.6: Audit Log Purge Test Never Actually Purges (MEDIUM)

- **File:** `tests/unit/test_audit_log.c:241-272`
- **Description:** Entries created "now" survive all cutoffs. The test never verifies entries ARE deleted. It's a no-op that always passes.
- **Fix:** Add test API for arbitrary-timestamp entries, or mock `time()`.

### TEST-2.7: No Alarm De-escalation Tests (MEDIUM)

- **File:** `tests/unit/test_alarm_engine.c`
- **Description:** Escalation (MEDIUM->HIGH) is tested but de-escalation (HIGH->MEDIUM, CRITICAL->NORMAL) is not.
- **Fix:** Add de-escalation tests for ACTIVE and ACKNOWLEDGED states.

### TEST-3.1: Test Framework Has No Setup/Teardown Mechanism (HIGH)

- **File:** `tests/unit/test_framework.h`
- **Description:** No `TEST_SETUP()`/`TEST_TEARDOWN()`. Each test manually calls `init()`/`close()`. Assertions don't abort -- corrupted state produces misleading subsequent results.
- **Fix:** Add fatal assertion macros and setup/teardown hooks.

### TEST-3.2: Extern Counter Pattern Is Fragile (LOW)

- **File:** `tests/unit/test_framework.h:19-22`
- **Fix:** Add clear comment about single-definition requirement.

### TEST-3.3: Assertions Don't Stop on Failure (MEDIUM)

- **File:** `tests/unit/test_framework.h:24-32`
- **Description:** After `ASSERT_NOT_NULL(ptr)` fails, code continues and dereferences ptr -- segfault crashes entire test runner.
- **Fix:** Add `ASSERT_NOT_NULL_OR_RETURN(ptr)` variant.

### TEST-4.1: Integration Tests Are Sequenced Unit Tests (HIGH)

- **File:** All integration test files
- **Description:** Tests manually simulate module integration by calling functions from both modules. The actual glue code is never tested.
- **Fix:** Write true end-to-end tests exercising actual application workflow (e.g., `alarm_service_tick()`).

### TEST-4.2: No Failure Scenario Integration Tests (HIGH)

- **Description:** All 21 integration tests test happy-path only. No DB corruption, network disconnect, sensor failure, or concurrent access tests.
- **Fix:** Add failure-mode integration tests.

### TEST-4.3: trend_db Is Not Per-Patient (MEDIUM)

- **File:** `tests/integration/test_patient_trends_integration.c:218-219`
- **Description:** Vitals data mixed in one table with no patient identifier. Test acknowledges but doesn't address.
- **Fix:** Add patient_id column or document the limitation.

### TEST-5.1: No Authentication Bypass Testing (CRITICAL)

- **Description:** No tests for SQL injection via username/PIN, buffer overflow with long strings, empty strings, deactivated users, or session hijacking.
- **Fix:** Create dedicated `test_auth_security.c`.

### TEST-5.2: No OTA Failure/Rollback Testing (HIGH)

- **Fix:** Create `test_ota_update.c` covering every state transition and error path.

### TEST-5.3: No FHIR JSON Validation Testing (HIGH)

- **Description:** No tests for JSON validity, LOINC codes, required FHIR R4 fields, buffer overflow, special characters.
- **Fix:** Create `test_fhir_client.c`.

### TEST-5.4: No Sync Queue Durability Testing (HIGH)

- **Description:** Core offline-first promise (push, close, reopen, verify persistence) is never tested.
- **Fix:** Create `test_sync_queue.c`.

### TEST-6.1: SIMULATOR_BUILD Not Defined in CMake (CRITICAL -- duplicate of TEST-2.2)

### TEST-6.2: Integration Tests Include Full LVGL Unnecessarily (LOW)

- **Fix:** Remove `theme_vitals.c` from integration build or stub LVGL dependency.

### TEST-6.3: No Code Coverage Tooling (HIGH)

- **Description:** No `--coverage` flags, no lcov/gcov configured. Cannot prove which code paths are exercised.
- **Fix:** Add CMake option: `option(ENABLE_COVERAGE "Enable coverage" OFF)` with lcov integration.

---

## 5. UI/UX Reliability Findings

### UI-1.1: Screen Stack Growth from In-Screen push() Calls (CRITICAL)

- **File:** `src/ui/screens/screen_settings.c:394`, `screen_patient.c:228`
- **Description:** `reset_defaults_cb` and `discharge_cb` call `screen_manager_push()` on their OWN screen ID to "refresh." Each call pushes without popping. After 8 clicks the stack (MAX_SCREENS=8) overflows -- push silently dropped, user stuck.
- **Fix:** Add `screen_manager_replace_current()` that pops before pushing.

### UI-1.2: Login-to-Main Navigation Leaves Login on Stack (HIGH)

- **File:** `src/ui/screens/screen_login.c:324`
- **Description:** On login success, `push(MAIN_VITALS)` pushes on top of Login. Login remains in stack. Auto-return timeout sends user back to Login instead of Main Vitals.
- **Fix:** Use `go_home()` or `replace_current()` after login.

### UI-1.3: Duplicate Screen Push Not Prevented (MEDIUM)

- **File:** `src/ui/screens/screen_manager.c:45-72`
- **Description:** Double-tapping a nav button during animation causes duplicate stack entries and pool conflicts.
- **Fix:** Add guard: `if (nav_stack_top >= 0 && nav_stack[nav_stack_top] == id) return;`

### UI-1.4: Destroy Called Before New Screen Exists (MEDIUM)

- **File:** `src/ui/screens/screen_manager.c:57-62`
- **Description:** Current screen's destroy callback fires before new screen is created. Timer callbacks in the gap find NULL widget pointers.
- **Fix:** Defer destroy until after `lv_screen_load_anim()`.

### UI-1.5: go_home() Only Destroys Top Screen (LOW)

- **File:** `src/ui/screens/screen_manager.c:96-116`
- **Description:** Intermediate screens with non-LVGL resources (timers) could leak when go_home skips their destroy.
- **Fix:** Iterate through all intermediate entries calling destroy.

### UI-2.1: Timer Callbacks Fire on Destroyed LVGL Objects (CRITICAL)

- **File:** `src/ui/widgets/widget_alarm_banner.c:185-191`, `screen_trends.c:481-485`, `screen_alarms.c:488-506`, `screen_audit_log.c:512-516`
- **Description:** `lv_screen_load_anim(auto_del=true)` deletes old screen after 200ms. During that window, destroy has already been called (nulling pointers), but LVGL timers may still fire on stale objects. Pool slots may be reallocated.
- **Fix:** Use instant `lv_screen_load()` for safety-critical transitions, or call destroy from animation-complete callback.

### UI-2.2: Static Widget Pools Shared Across Transition (HIGH)

- **File:** All widget pool files (`widget_*.c`)
- **Description:** During 200ms overlap, both old (not yet deleted) and new LVGL objects reference same pool slot.
- **Fix:** Double-buffered pool slots or instant screen swap.

### UI-2.3: Nav Bar Event Callbacks Persist After Destruction (HIGH)

- **File:** `src/ui/widgets/widget_nav_bar.c:133-135`
- **Description:** Old nav buttons remain clickable during fade. Clicks corrupt navigation state.
- **Fix:** Disable old screen: `lv_obj_remove_flag(old_scr, LV_OBJ_FLAG_CLICKABLE)`.

### UI-2.4: auto_del Consistently Applied (INFO -- positive)

### UI-3.1: Waveform Push During Screen Transition (HIGH)

- **File:** `simulator/main.c:78-94`, `src/ui/widgets/widget_waveform.c:108-126`
- **Description:** During 200ms transition, waveform samples pushed to chart objects from both old and new screens via same pool slot.
- **Fix:** Add `waveform_ready` flag set only after new screen fully loaded.

### UI-3.2: Erase Bar Width Not Bounds-Checked (MEDIUM)

- **File:** `src/ui/widgets/widget_waveform.c:121-125`
- **Fix:** Add `_Static_assert(VM_WAVEFORM_ERASE_WIDTH < VM_WAVEFORM_POINT_COUNT / 2, "Erase bar too wide")`.

### UI-3.3: Waveform Write Position Independent of LVGL Cursor (MEDIUM)

- **File:** `src/ui/widgets/widget_waveform.c:112-116`
- **Fix:** Synchronize with LVGL chart's internal cursor.

### UI-3.4: No Handling of Missing Data or Lead-Off (MEDIUM)

- **File:** `src/ui/widgets/widget_waveform.c`, `simulator/main.c`
- **Description:** `ecg_lead_off` and quality fields not checked. Lead-off still shows synthesized waveform.
- **Fix:** Check quality fields. Display "LEAD OFF" or "NO SIGNAL" when below threshold.

### UI-4.1: No Alarm Acknowledgment Button on Main Screen (CRITICAL)

- **File:** `src/ui/widgets/widget_alarm_banner.c`, `screen_main_vitals.c`
- **Description:** Alarm ACK only on Alarms screen. IEC 60601-1-8 requires immediate accessibility from the primary display.
- **Fix:** Add ACK button to alarm banner widget.

### UI-4.2: Flash Timer Not Gated on Visibility (MEDIUM)

- **File:** `src/ui/widgets/widget_alarm_banner.c:135-141`
- **Fix:** Verify pool reallocation safety during auto_del window.

### UI-4.3: Only Highest Alarm Shown -- Lower Alarms Invisible (HIGH)

- **File:** `src/ui/widgets/widget_alarm_banner.c:103-144`
- **Description:** Acknowledging highest alarm shows "No Alarms" while lower-severity alarms remain active but invisible.
- **Fix:** Cycle through multiple active alarms. Add count indicator.

### UI-5.1: No Signal Quality / "No Signal" Display (CRITICAL)

- **File:** `src/ui/widgets/widget_numeric_display.c:171-174`, `screen_main_vitals.c:183-215`
- **Description:** HR=0 (meaning "invalid") displays as "0" -- looks like asystole. SpO2=0 looks like zero saturation. No checks of quality fields or `ecg_lead_off`.
- **Impact:** Nurse could see "HR: 0" and initiate resuscitation for a disconnected lead. True asystole might be dismissed as sensor issue because display looks identical.
- **Fix:** Display "---" or "NO SIGNAL" when value is 0 or quality indicates no signal.

### UI-5.2: No Range Clamping for Extreme Values (LOW)

- **File:** `src/ui/screens/screen_main_vitals.c:183-215`
- **Fix:** Clamp to physiological ranges. Display "ERR" for out-of-range.

### UI-5.3: NaN/Infinity Not Handled for Temperature (LOW)

- **File:** `src/ui/screens/screen_main_vitals.c:207`
- **Fix:** Add `isnan() || isinf()` check.

### UI-6.1: Nav Bar Double-Navigation -- Triple Screen Load (HIGH)

- **File:** `src/ui/widgets/widget_nav_bar.c:172-188`
- **Description:** Creates 3 screens in 200ms queue. `MAX_NAV_BARS=2` means third allocation fails.
- **Fix:** `screen_manager_go_home_then_push(target)` -- set stack directly, load only target.

### UI-6.2: Audit Log Missing Nav Bar Highlight (LOW)

- **Fix:** Minor -- screen title makes location obvious.

### UI-7.1: Error Timer Race with Screen Destruction (MEDIUM)

- **File:** `src/ui/screens/screen_login.c:300-307`
- **Fix:** Add `static bool login_active` guard for defense-in-depth.

### UI-7.2: No Login Attempt Rate Limiting at UI Level (MEDIUM)

- **File:** `src/ui/screens/screen_login.c:310-337`
- **Description:** All 10,000 PINs can be tried in ~83 minutes at 2s per attempt.
- **Fix:** Progressive lockout (30s after 3 failures, 5min after 5).

### UI-8.1: Alarm Volume Can Be Set to Zero (CRITICAL)

- **File:** `src/ui/screens/screen_settings.c:167`
- **Description:** Slider range starts at 0. Combined with mute, permanently silences all alarms. Violates IEC 60601-1-8.
- **Impact:** Clinician could accidentally silence all critical alarms.
- **Fix:** `lv_slider_set_range(volume_slider, 20, 100)`. Require auth for changes below safety threshold.

### UI-8.2: Network Status Not Live-Updating (INFO)

- **File:** `src/ui/screens/screen_settings.c:232-251`
- **Fix:** Add periodic refresh timer.

### UI-9.1: Shared Static Query Buffers (LOW)

- **Fix:** Document sequential dependency.

### UI-9.2: Time Range Not Persisted Between Visits (LOW)

- **Fix:** Persist `active_range_idx` in settings store.

---

## 6. Build & Deployment Findings

### BUILD-1.1: Simulator CMake Missing Warning Flags (HIGH)

- **File:** `simulator/CMakeLists.txt`
- **Description:** Only `-Wall -Wextra` enabled. Missing `-Wshadow`, `-Wconversion`, `-Wdouble-promotion`, `-Wformat=2`, `-Wundef`.
- **Fix:** Add all recommended warning flags for safety-critical C code.

### BUILD-1.2: Unit Test CMake Missing Security and Warning Flags (HIGH)

- **File:** `tests/unit/CMakeLists.txt`
- **Description:** No warnings or security flags at all. Only `C99` standard and `-g`.
- **Fix:** Match simulator warning flags in test builds.

### BUILD-1.3: Missing Compiler Hardening Flags (CRITICAL)

- **File:** `simulator/CMakeLists.txt`
- **Description:** Missing `-fstack-protector-strong`, `-D_FORTIFY_SOURCE=2`, `-fPIE`/`-pie`, `-Wformat-security`, `-Wl,-z,relro,-z,now`. The project's OWN hardening checklist (items 10.6-10.10) requires these.
- **Fix:** Add all hardening flags to both simulator and Buildroot package builds.

### BUILD-1.4: No Sanitizer Build Options (MEDIUM)

- **Fix:** Add `option(ENABLE_ASAN "AddressSanitizer" OFF)` CMake option.

### BUILD-1.5: No CMAKE_BUILD_TYPE Configuration (MEDIUM)

- **Fix:** Add Debug/Release/RelWithDebInfo support. Disable `BR2_ENABLE_DEBUG` in production.

### BUILD-1.6: vitals_provider_mock.c Never Built (LOW)

- **Fix:** Integrate into build or remove dead code.

### BUILD-1.7: GLOB_RECURSE for LVGL Sources (LOW)

- **Fix:** Pin files explicitly or add `CMAKE_CONFIGURE_DEPENDS`.

### BUILD-2.1: Debug Tools (strace/gdb) in Production Image (CRITICAL)

- **File:** `buildroot-external/configs/vitals_monitor_stm32mp157f_dk2_defconfig:144-148`
- **Description:** `BR2_PACKAGE_STRACE=y`, `BR2_PACKAGE_GDB=y`, `BR2_PACKAGE_GDB_SERVER=y`, `BR2_ENABLE_DEBUG=y`.
- **Fix:** Remove. Create separate `_dev_defconfig` for development.

### BUILD-2.2: SSH Server Enabled in Production (CRITICAL)

- **File:** defconfig line 114: `BR2_PACKAGE_OPENSSH=y`
- **Fix:** Remove from production defconfig.

### BUILD-2.3: Placeholder Root Password (CRITICAL)

- **File:** defconfig line 133: `BR2_TARGET_GENERIC_ROOT_PASSWD="$$6$$vitals$$placeholder"`
- **Fix:** Lock root account entirely or generate unique per-device passwords at provisioning.

### BUILD-2.4: IPC Socket Path Mismatch (HIGH)

- **File:** `buildroot-external/board/vitals-monitor/post-build.sh:220-222` vs `src/common/ipc/ipc_messages.h:208-211`
- **Description:** Post-build uses `/run/vitals-monitor/{sensor,alarm,command}.ipc`. Source code uses `/tmp/vitals-monitor/{vitals,waveforms}.ipc`. Names also differ.
- **Impact:** IPC layer will fail on target -- application non-functional after deployment.
- **Fix:** Unify to `/run/vitals-monitor/` across all files.

### BUILD-2.5: Systemd Services Duplicated and Inconsistent (HIGH)

- **File:** `post-build.sh:40-172` vs `deploy/systemd/` (5 files)
- **Description:** Post-build.sh generates inline services running as ROOT without sandboxing. `deploy/systemd/` contains hardened versions with `User=vitals`, `ProtectSystem=strict`, `NoNewPrivileges=yes`. Only the root-running versions are actually installed.
- **Fix:** Install `deploy/systemd/*.service` from post-build.sh instead of generating inline versions.

### BUILD-2.6: AppArmor Not Installed in Buildroot (HIGH)

- **File:** defconfig (missing `BR2_PACKAGE_APPARMOR`)
- **Description:** 5 well-crafted AppArmor profiles exist in `deploy/security/apparmor/` but are never deployed. No MAC enforcement.
- **Fix:** Add `BR2_PACKAGE_APPARMOR=y`, kernel `CONFIG_SECURITY_APPARMOR=y`, install profiles in post-build.

### BUILD-2.7: No Firewall Configuration (MEDIUM)

- **Fix:** Add iptables/nftables with default-DROP policy.

### BUILD-3.1: AppArmor Profile Missing DRM Device Access (HIGH)

- **File:** `deploy/security/apparmor/vitals-ui:49`
- **Description:** Grants `/dev/fb0` but Buildroot uses DRM (`/dev/dri/card0`).
- **Fix:** Add `/dev/dri/card*` and `/dev/dri/renderD*` rules.

### BUILD-3.2: AppArmor IPC Path Inconsistency (HIGH)

- **File:** All AppArmor profiles reference `/tmp/vitals-monitor/` but Buildroot configures `/run/vitals-monitor/`.
- **Fix:** Update profiles to match canonical IPC path.

### BUILD-3.3: vitals-ui Database Access Too Broad (MEDIUM)

- **Fix:** Restrict wildcard `*.db` to specific file paths.

### BUILD-3.4: LUKS Key Salt Hardcoded in Source (MEDIUM)

- **File:** `deploy/security/setup-luks.sh:172`
- **Fix:** Use device-provisioning secret not stored in source control.

### BUILD-3.5: Bash Variable Clearing Ineffective (LOW)

- **Fix:** Use compiled C helper with `explicit_bzero()`.

### BUILD-4.1: No Production OTA Implementation (CRITICAL)

- **File:** `src/drivers/ota_update.c`
- **Description:** Entirely stubbed. `ota_check_for_update()` returns false. `ota_start_update()` returns false. Header claims RSA-2048 + SHA-256 and A/B partitions -- none implemented.
- **Fix:** Implement production OTA with SWUpdate integration before deployment.

### BUILD-4.2: OTA Header Claims Unimplemented Features (HIGH)

- **File:** `src/drivers/ota_update.h:14-18`
- **Fix:** Mark as "PLANNED" or "TODO" in documentation.

### BUILD-5.1: Hardcoded Credentials in Production Binary (CRITICAL -- cross-ref SEC-AUTH-02)

### BUILD-5.2: DJB2a in Production Binary (CRITICAL -- cross-ref SEC-AUTH-01)

### BUILD-5.3: Fixed Hash Salt in Source (HIGH -- cross-ref SEC-AUTH-03)

### BUILD-5.4: Hardcoded Mock IP Address (LOW)

### BUILD-5.5: .gitignore Missing Sensitive File Patterns (MEDIUM)

- **Fix:** Add `*.pem`, `*.key`, `*.crt`, `.env`, `credentials.*`, `secrets.*`, `*.keystore`.

### BUILD-6.1: All Regulatory Documents Are Templates (CRITICAL)

- **File:** All files in `docs/regulatory/`
- **Description:** Every document has unfilled [TODO] metadata (version, date, author, reviewer, approval). Multiple requirements have [TODO] entries. README explicitly states "Template" status.
- **Fix:** Complete all documents with project-specific content.

### BUILD-6.2: No SOUP Documentation (HIGH)

- **Description:** IEC 62304 clause 8 requires SOUP documentation. The project uses LVGL 9.3, SQLite, nanomsg, mbedTLS, cJSON, Linux kernel -- none documented with version, anomaly list, risk assessment.
- **Fix:** Create SOUP List document.

### BUILD-6.3: Traceability Matrix Has No Test Case IDs (HIGH)

- **Description:** All Test Case ID fields are [TODO]. 563 tests exist but none mapped to requirements.
- **Fix:** Map test functions to SRS requirement IDs.

### BUILD-6.4: Risk Analysis Missing Verification Evidence (HIGH)

- **Description:** Every hazard's Verification field is [TODO].
- **Fix:** Link to specific test results.

### BUILD-6.5: Missing Mandatory IEC 62304 Documents (HIGH)

- **Description:** Software Development Plan, Maintenance Plan, Configuration Management Plan, Usability Engineering File are all absent.
- **Fix:** Create these documents.

### BUILD-6.6: Overall Residual Risk Evaluation Missing (MEDIUM)

- **Fix:** Author after individual hazard verifications are complete.

### BUILD-7.1: Header Include Guards Consistent (INFO -- positive)
### BUILD-7.2: vitals_provider.h Abstraction Well-Designed (INFO -- positive)
### BUILD-7.3: SIMULATOR_BUILD Guard Properly Used (INFO -- positive)

### BUILD-7.4: Drivers Not Compiled in Simulator Build (LOW)

- **Fix:** Include with stubs for type-checking, or add a separate CMake target.

### BUILD-7.5: Two Divergent Deploy Configurations (MEDIUM)

- **Fix:** Establish single source of truth in `deploy/systemd/`.

---

## Priority Remediation Roadmap

### Phase 1: Immediate (Patient Safety & Security -- Blocks Deployment)

| # | Finding | Description | Effort |
|---|---------|-------------|--------|
| 1 | SEC-AUTH-01 | Replace DJB2a with Argon2id | Medium |
| 2 | REL-3.1 | Add mutex/ring buffer for IPC provider | Medium |
| 3 | UI-8.1 | Set minimum alarm volume > 0 | Small |
| 4 | UI-5.1 | Add signal quality / "NO SIGNAL" display | Medium |
| 5 | UI-4.1 | Add alarm ACK button on main screen | Medium |
| 6 | UI-1.1 | Fix screen stack growth (replace_current) | Small |
| 7 | UI-2.1 | Fix timer race during screen transitions | Medium |
| 8 | SEC-INPUT-01 | Add vital sign range validation | Small |
| 9 | BUILD-2.1-2.3 | Remove debug tools, SSH, placeholder password | Small |
| 10 | BUILD-1.3 | Add compiler hardening flags | Small |

### Phase 2: Short-Term (Reliability & Correctness)

| # | Finding | Description | Effort |
|---|---------|-------------|--------|
| 11 | REL-7.1 | Add SQL transactions to multi-step ops | Medium |
| 12 | REL-7.3 | Fix sync queue SENDING zombie items | Small |
| 13 | REL-6.1 | Implement software watchdog | Medium |
| 14 | PERF-3.1 | Share single SQLite connection | Medium |
| 15 | PERF-3.2 | Pre-compile trend_db queries | Small |
| 16 | PERF-2.1 | Fix main loop timing | Small |
| 17 | PERF-1.1/1.2 | Cache main screen, fix double navigation | Medium |
| 18 | UI-2.2/2.3 | Fix pool overlap & event persistence | Medium |
| 19 | UI-4.3 | Show multiple active alarms | Medium |
| 20 | BUILD-2.4 | Fix IPC socket path mismatch | Small |

### Phase 3: Test Coverage & Compliance (Blocks IEC 62304)

| # | Finding | Description | Effort |
|---|---------|-------------|--------|
| 21 | TEST-2.2 | Add SIMULATOR_BUILD to test CMake | Trivial |
| 22 | TEST-1.1 | Write tests for 15+ untested modules | Large |
| 23 | TEST-5.1 | Add auth security tests | Medium |
| 24 | TEST-5.3 | Add FHIR JSON validation tests | Medium |
| 25 | TEST-5.4 | Add sync queue durability tests | Medium |
| 26 | TEST-6.3 | Configure code coverage tooling | Small |
| 27 | BUILD-6.1 | Complete regulatory documents | Large |
| 28 | BUILD-6.2 | Create SOUP documentation | Medium |
| 29 | BUILD-6.3 | Map tests to requirements in RTM | Medium |

### Phase 4: Hardening & Defense-in-Depth

| # | Finding | Description | Effort |
|---|---------|-------------|--------|
| 30 | SEC-IPC-01 | Add IPC authentication (HMAC/SO_PEERCRED) | Medium |
| 31 | SEC-MEM-02 | Add JSON string escaping in FHIR | Small |
| 32 | SEC-CRYPTO-01 | Build TLS configuration module | Large |
| 33 | BUILD-2.5 | Install hardened systemd services | Small |
| 34 | BUILD-2.6 | Enable and deploy AppArmor | Medium |
| 35 | BUILD-4.1 | Implement production OTA module | Large |

---

## Methodology

This audit was performed by 6 independent analysis agents running in parallel:

1. **Security Agent** -- Reviewed auth, crypto, SQL injection, buffer safety, IPC security, input validation
2. **Reliability Agent** -- Reviewed error handling, memory management, concurrency, state machines, data integrity
3. **Performance Agent** -- Reviewed rendering, main loop timing, database efficiency, memory allocation
4. **Test Quality Agent** -- Reviewed coverage matrix, test quality, framework robustness, missing scenarios
5. **UI/UX Reliability Agent** -- Reviewed screen manager, widget lifecycle, LVGL object management, alarm display
6. **Build & Deployment Agent** -- Reviewed CMake, Buildroot, systemd, AppArmor, OTA, regulatory docs

Each agent independently read all relevant source files and produced findings with severity ratings, file locations, impact assessments, and remediation recommendations.
