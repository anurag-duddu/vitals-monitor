/**
 * @file audit_log.c
 * @brief Audit trail logging implementation
 *
 * SQLite-backed audit log with prepared statements for insertion and
 * querying. All queries return newest-first ordering.
 *
 * Single-threaded — all access from LVGL main loop.
 * Static result buffers avoid heap allocation in query paths.
 */

#include "audit_log.h"
#include "sqlite3.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

/* ── Retention ───────────────────────────────────────────── */

#define AUDIT_DEFAULT_RETENTION_S  (30 * 24 * 3600)  /* 30 days */

/* ── Event name strings ──────────────────────────────────── */

static const char *event_names[AUDIT_EVENT_COUNT] = {
    [AUDIT_EVENT_LOGIN]                = "LOGIN",
    [AUDIT_EVENT_LOGOUT]               = "LOGOUT",
    [AUDIT_EVENT_LOGIN_FAILED]         = "LOGIN_FAILED",
    [AUDIT_EVENT_SESSION_TIMEOUT]      = "SESSION_TIMEOUT",
    [AUDIT_EVENT_ALARM_ACK]            = "ALARM_ACK",
    [AUDIT_EVENT_ALARM_SILENCE]        = "ALARM_SILENCE",
    [AUDIT_EVENT_ALARM_LIMITS_CHANGED] = "ALARM_LIMITS_CHANGED",
    [AUDIT_EVENT_PATIENT_ADMIT]        = "PATIENT_ADMIT",
    [AUDIT_EVENT_PATIENT_DISCHARGE]    = "PATIENT_DISCHARGE",
    [AUDIT_EVENT_PATIENT_MODIFIED]     = "PATIENT_MODIFIED",
    [AUDIT_EVENT_SETTINGS_CHANGED]     = "SETTINGS_CHANGED",
    [AUDIT_EVENT_USER_CREATED]         = "USER_CREATED",
    [AUDIT_EVENT_USER_DELETED]         = "USER_DELETED",
    [AUDIT_EVENT_SYSTEM_START]         = "SYSTEM_START",
    [AUDIT_EVENT_SYSTEM_SHUTDOWN]      = "SYSTEM_SHUTDOWN",
};

/* ── Module state ────────────────────────────────────────── */

static sqlite3 *db = NULL;

/* Prepared statements */
static sqlite3_stmt *stmt_insert       = NULL;
static sqlite3_stmt *stmt_query_recent = NULL;
static sqlite3_stmt *stmt_query_user   = NULL;
static sqlite3_stmt *stmt_query_event  = NULL;
static sqlite3_stmt *stmt_query_range  = NULL;
static sqlite3_stmt *stmt_purge        = NULL;

/* ── Schema ──────────────────────────────────────────────── */

static const char *SCHEMA_SQL =
    "CREATE TABLE IF NOT EXISTS audit_log ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  event INTEGER NOT NULL,"
    "  username TEXT NOT NULL,"
    "  message TEXT NOT NULL,"
    "  timestamp_s INTEGER NOT NULL"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_audit_ts ON audit_log(timestamp_s);"
    "CREATE INDEX IF NOT EXISTS idx_audit_user ON audit_log(username);"
    "CREATE INDEX IF NOT EXISTS idx_audit_event ON audit_log(event);";

/* ── Helper: prepare a single statement ──────────────────── */

static bool prepare(sqlite3_stmt **out, const char *sql) {
    int rc = sqlite3_prepare_v2(db, sql, -1, out, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[audit_log] prepare failed: %s\n  SQL: %s\n",
                sqlite3_errmsg(db), sql);
        return false;
    }
    return true;
}

static void finalize_stmt(sqlite3_stmt **s) {
    if (*s) {
        sqlite3_finalize(*s);
        *s = NULL;
    }
}

/* ── Helper: read a row into an audit_entry_t ────────────── */

static void read_entry_row(sqlite3_stmt *stmt, audit_entry_t *entry) {
    memset(entry, 0, sizeof(audit_entry_t));

    entry->id = sqlite3_column_int(stmt, 0);
    entry->event = (audit_event_t)sqlite3_column_int(stmt, 1);

    const char *user_str = (const char *)sqlite3_column_text(stmt, 2);
    if (user_str) {
        strncpy(entry->username, user_str, AUDIT_USER_MAX - 1);
        entry->username[AUDIT_USER_MAX - 1] = '\0';
    }

    const char *msg_str = (const char *)sqlite3_column_text(stmt, 3);
    if (msg_str) {
        strncpy(entry->message, msg_str, AUDIT_MSG_MAX - 1);
        entry->message[AUDIT_MSG_MAX - 1] = '\0';
    }

    entry->timestamp_s = (uint32_t)sqlite3_column_int(stmt, 4);
}

/* ── Helper: run a query and fill result buffer ──────────── */

static int run_query(sqlite3_stmt *stmt, int max_count,
                     audit_query_result_t *out) {
    if (!out) return 0;
    out->count = 0;

    if (max_count > AUDIT_LOG_QUERY_MAX) {
        max_count = AUDIT_LOG_QUERY_MAX;
    }

    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < max_count) {
        read_entry_row(stmt, &out->entries[i]);
        i++;
    }

    out->count = i;
    return i;
}

/* ── Lifecycle ───────────────────────────────────────────── */

bool audit_log_init(const char *db_path) {
    const char *path = db_path ? db_path : ":memory:";
    int rc = sqlite3_open(path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[audit_log] Failed to open DB '%s': %s\n",
                path, sqlite3_errmsg(db));
        db = NULL;
        return false;
    }

    /* Performance pragmas */
    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);
    sqlite3_exec(db, "PRAGMA synchronous=NORMAL;", NULL, NULL, NULL);

    /* Create tables and indexes */
    char *err_msg = NULL;
    rc = sqlite3_exec(db, SCHEMA_SQL, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[audit_log] Schema creation failed: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        db = NULL;
        return false;
    }

    /* Prepare all statements */
    bool ok = true;

    ok = ok && prepare(&stmt_insert,
        "INSERT INTO audit_log (event, username, message, timestamp_s) "
        "VALUES (?1, ?2, ?3, ?4)");

    ok = ok && prepare(&stmt_query_recent,
        "SELECT id, event, username, message, timestamp_s "
        "FROM audit_log ORDER BY timestamp_s DESC LIMIT ?1");

    ok = ok && prepare(&stmt_query_user,
        "SELECT id, event, username, message, timestamp_s "
        "FROM audit_log WHERE username = ?1 "
        "ORDER BY timestamp_s DESC LIMIT ?2");

    ok = ok && prepare(&stmt_query_event,
        "SELECT id, event, username, message, timestamp_s "
        "FROM audit_log WHERE event = ?1 "
        "ORDER BY timestamp_s DESC LIMIT ?2");

    ok = ok && prepare(&stmt_query_range,
        "SELECT id, event, username, message, timestamp_s "
        "FROM audit_log WHERE timestamp_s >= ?1 AND timestamp_s <= ?2 "
        "ORDER BY timestamp_s DESC LIMIT ?3");

    ok = ok && prepare(&stmt_purge,
        "DELETE FROM audit_log WHERE timestamp_s < ?1");

    if (!ok) {
        fprintf(stderr, "[audit_log] Statement preparation failed\n");
        audit_log_close();
        return false;
    }

    printf("[audit_log] Initialized: %s\n", path);
    return true;
}

void audit_log_close(void) {
    finalize_stmt(&stmt_insert);
    finalize_stmt(&stmt_query_recent);
    finalize_stmt(&stmt_query_user);
    finalize_stmt(&stmt_query_event);
    finalize_stmt(&stmt_query_range);
    finalize_stmt(&stmt_purge);

    if (db) {
        sqlite3_close(db);
        db = NULL;
        printf("[audit_log] Closed\n");
    }
}

/* ── Record events ───────────────────────────────────────── */

void audit_log_record(audit_event_t event, const char *username,
                      const char *message) {
    if (!db || !stmt_insert) return;

    const char *user = username ? username : "system";
    const char *msg  = message  ? message  : "";

    uint32_t ts = (uint32_t)time(NULL);

    sqlite3_reset(stmt_insert);
    sqlite3_bind_int(stmt_insert, 1, (int)event);
    sqlite3_bind_text(stmt_insert, 2, user, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt_insert, 3, msg, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt_insert, 4, (int)ts);

    int rc = sqlite3_step(stmt_insert);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "[audit_log] insert failed: %s\n", sqlite3_errmsg(db));
        return;
    }

    /* Console logging for debugging */
    const char *event_str = audit_event_name(event);
    printf("[audit_log] %s | user=%s | %s\n", event_str, user, msg);
}

void audit_log_record_fmt(audit_event_t event, const char *username,
                          const char *fmt, ...) {
    char buf[AUDIT_MSG_MAX];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    audit_log_record(event, username, buf);
}

/* ── Query ───────────────────────────────────────────────── */

int audit_log_query_recent(int max_count, audit_query_result_t *out) {
    if (!db || !stmt_query_recent || !out) return 0;

    if (max_count > AUDIT_LOG_QUERY_MAX) max_count = AUDIT_LOG_QUERY_MAX;

    sqlite3_reset(stmt_query_recent);
    sqlite3_bind_int(stmt_query_recent, 1, max_count);

    return run_query(stmt_query_recent, max_count, out);
}

int audit_log_query_by_user(const char *username, int max_count,
                            audit_query_result_t *out) {
    if (!db || !stmt_query_user || !out || !username) return 0;

    if (max_count > AUDIT_LOG_QUERY_MAX) max_count = AUDIT_LOG_QUERY_MAX;

    sqlite3_reset(stmt_query_user);
    sqlite3_bind_text(stmt_query_user, 1, username, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt_query_user, 2, max_count);

    return run_query(stmt_query_user, max_count, out);
}

int audit_log_query_by_event(audit_event_t event, int max_count,
                             audit_query_result_t *out) {
    if (!db || !stmt_query_event || !out) return 0;

    if (max_count > AUDIT_LOG_QUERY_MAX) max_count = AUDIT_LOG_QUERY_MAX;

    sqlite3_reset(stmt_query_event);
    sqlite3_bind_int(stmt_query_event, 1, (int)event);
    sqlite3_bind_int(stmt_query_event, 2, max_count);

    return run_query(stmt_query_event, max_count, out);
}

int audit_log_query_range(uint32_t start_ts, uint32_t end_ts,
                          audit_query_result_t *out) {
    if (!db || !stmt_query_range || !out) return 0;

    sqlite3_reset(stmt_query_range);
    sqlite3_bind_int(stmt_query_range, 1, (int)start_ts);
    sqlite3_bind_int(stmt_query_range, 2, (int)end_ts);
    sqlite3_bind_int(stmt_query_range, 3, AUDIT_LOG_QUERY_MAX);

    return run_query(stmt_query_range, AUDIT_LOG_QUERY_MAX, out);
}

/* ── Utility ─────────────────────────────────────────────── */

const char *audit_event_name(audit_event_t event) {
    if (event >= 0 && event < AUDIT_EVENT_COUNT) {
        return event_names[event];
    }
    return "UNKNOWN";
}

void audit_log_purge_old(uint32_t max_age_s) {
    if (!db || !stmt_purge) return;

    uint32_t age = (max_age_s > 0) ? max_age_s : AUDIT_DEFAULT_RETENTION_S;
    uint32_t now = (uint32_t)time(NULL);
    uint32_t cutoff = (now > age) ? now - age : 0;

    sqlite3_reset(stmt_purge);
    sqlite3_bind_int(stmt_purge, 1, (int)cutoff);

    int rc = sqlite3_step(stmt_purge);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "[audit_log] purge failed: %s\n", sqlite3_errmsg(db));
        return;
    }

    int deleted = sqlite3_changes(db);
    if (deleted > 0) {
        printf("[audit_log] Purged %d entries older than %u s\n", deleted, age);
    }
}
