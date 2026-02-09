/**
 * @file audit_log.h
 * @brief Audit trail logging for regulatory compliance (CDSCO Class B)
 *
 * Records all security-relevant and clinically significant events to a
 * SQLite-backed audit trail. Supports querying by recency, user, event
 * type, and time range.
 *
 * Retention: 30 days by default.
 * Single-threaded — all calls from LVGL main loop.
 * No LVGL dependency (pure data layer).
 */

#ifndef AUDIT_LOG_H
#define AUDIT_LOG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Constants ───────────────────────────────────────────── */

#define AUDIT_MSG_MAX         128
#define AUDIT_USER_MAX        48
#define AUDIT_LOG_QUERY_MAX   100

/* ── Event types ─────────────────────────────────────────── */

typedef enum {
    AUDIT_EVENT_LOGIN = 0,
    AUDIT_EVENT_LOGOUT,
    AUDIT_EVENT_LOGIN_FAILED,
    AUDIT_EVENT_SESSION_TIMEOUT,
    AUDIT_EVENT_ALARM_ACK,
    AUDIT_EVENT_ALARM_SILENCE,
    AUDIT_EVENT_ALARM_LIMITS_CHANGED,
    AUDIT_EVENT_PATIENT_ADMIT,
    AUDIT_EVENT_PATIENT_DISCHARGE,
    AUDIT_EVENT_PATIENT_MODIFIED,
    AUDIT_EVENT_SETTINGS_CHANGED,
    AUDIT_EVENT_USER_CREATED,
    AUDIT_EVENT_USER_DELETED,
    AUDIT_EVENT_SYSTEM_START,
    AUDIT_EVENT_SYSTEM_SHUTDOWN,
    AUDIT_EVENT_COUNT
} audit_event_t;

/* ── Audit entry ─────────────────────────────────────────── */

typedef struct {
    int32_t       id;
    audit_event_t event;
    char          username[AUDIT_USER_MAX];
    char          message[AUDIT_MSG_MAX];
    uint32_t      timestamp_s;
} audit_entry_t;

/* ── Query result (static buffer, no malloc) ─────────────── */

typedef struct {
    audit_entry_t entries[AUDIT_LOG_QUERY_MAX];
    int count;
} audit_query_result_t;

/* ── Lifecycle ───────────────────────────────────────────── */

/** Initialize the audit log. Opens/creates audit_log table. */
bool audit_log_init(const char *db_path);

/** Close the audit log and finalize all prepared statements. */
void audit_log_close(void);

/* ── Record events ───────────────────────────────────────── */

/** Record an audit event with a fixed message string. */
void audit_log_record(audit_event_t event, const char *username,
                      const char *message);

/** Record an audit event with printf-style formatted message. */
void audit_log_record_fmt(audit_event_t event, const char *username,
                          const char *fmt, ...);

/* ── Query ───────────────────────────────────────────────── */

/** Query the most recent audit entries. Returns count. */
int audit_log_query_recent(int max_count, audit_query_result_t *out);

/** Query audit entries by username. Returns count. */
int audit_log_query_by_user(const char *username, int max_count,
                            audit_query_result_t *out);

/** Query audit entries by event type. Returns count. */
int audit_log_query_by_event(audit_event_t event, int max_count,
                             audit_query_result_t *out);

/** Query audit entries within a time range. Returns count. */
int audit_log_query_range(uint32_t start_ts, uint32_t end_ts,
                          audit_query_result_t *out);

/* ── Utility ─────────────────────────────────────────────── */

/** Get human-readable name for an event type. */
const char *audit_event_name(audit_event_t event);

/** Delete audit entries older than max_age_s seconds. Default: 30 days. */
void audit_log_purge_old(uint32_t max_age_s);

#ifdef __cplusplus
}
#endif

#endif /* AUDIT_LOG_H */
