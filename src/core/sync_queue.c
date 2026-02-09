/**
 * @file sync_queue.c
 * @brief Offline-first sync queue — SQLite-backed implementation
 *
 * Persists outbound data items in a SQLite table so they survive
 * reboots.  Processing dequeues PENDING items, attempts export via
 * fhir_client, and marks them SENT or FAILED.
 *
 * In the simulator build, fhir_client stubs always succeed, so
 * process() will mark every pending item as SENT.
 *
 * Uses prepared statements for all hot-path operations.
 * All database access is single-threaded (LVGL main loop).
 */

#include "sync_queue.h"
#include "fhir_client.h"
#include "sqlite3.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/* ── Default retry limit ─────────────────────────────────── */

#define DEFAULT_MAX_RETRIES  5

/* ── Module state ────────────────────────────────────────── */

static sqlite3 *db = NULL;

/* Prepared statements */
static sqlite3_stmt *stmt_push           = NULL;
static sqlite3_stmt *stmt_get_pending    = NULL;
static sqlite3_stmt *stmt_update_status  = NULL;
static sqlite3_stmt *stmt_count_by_status = NULL;
static sqlite3_stmt *stmt_purge_sent     = NULL;
static sqlite3_stmt *stmt_retry_failed   = NULL;
static sqlite3_stmt *stmt_count_total    = NULL;

/* ── Schema ──────────────────────────────────────────────── */

static const char *SCHEMA_SQL =
    "CREATE TABLE IF NOT EXISTS sync_queue ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  type INTEGER NOT NULL,"
    "  status INTEGER NOT NULL DEFAULT 0,"
    "  payload TEXT NOT NULL,"
    "  created_ts INTEGER NOT NULL,"
    "  last_attempt_ts INTEGER NOT NULL DEFAULT 0,"
    "  retry_count INTEGER NOT NULL DEFAULT 0,"
    "  max_retries INTEGER NOT NULL DEFAULT 5"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_sync_status ON sync_queue(status);"
    "CREATE INDEX IF NOT EXISTS idx_sync_created ON sync_queue(created_ts);";

/* ── Helper: prepare a single statement ──────────────────── */

static bool prepare(sqlite3_stmt **out, const char *sql) {
    int rc = sqlite3_prepare_v2(db, sql, -1, out, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[sync_queue] prepare failed: %s\n  SQL: %s\n",
                sqlite3_errmsg(db), sql);
        return false;
    }
    return true;
}

/* ── Helper: finalize a prepared statement ───────────────── */

static void finalize_stmt(sqlite3_stmt **s) {
    if (*s) {
        sqlite3_finalize(*s);
        *s = NULL;
    }
}

/* ── Lifecycle ───────────────────────────────────────────── */

bool sync_queue_init(const char *db_path) {
    const char *path = db_path ? db_path : ":memory:";

    int rc = sqlite3_open(path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[sync_queue] Failed to open DB '%s': %s\n",
                path, sqlite3_errmsg(db));
        db = NULL;
        return false;
    }

    /* Performance pragmas */
    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);
    sqlite3_exec(db, "PRAGMA synchronous=NORMAL;", NULL, NULL, NULL);

    /* Create tables */
    char *err_msg = NULL;
    rc = sqlite3_exec(db, SCHEMA_SQL, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[sync_queue] Schema creation failed: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        db = NULL;
        return false;
    }

    /* Prepare all statements */
    bool ok = true;

    ok = ok && prepare(&stmt_push,
        "INSERT INTO sync_queue (type, status, payload, created_ts, "
        "last_attempt_ts, retry_count, max_retries) "
        "VALUES (?1, 0, ?2, ?3, 0, 0, ?4)");

    ok = ok && prepare(&stmt_get_pending,
        "SELECT id, type, status, payload, created_ts, last_attempt_ts, "
        "retry_count, max_retries "
        "FROM sync_queue "
        "WHERE status = 0 OR status = 4 "   /* PENDING or RETRY */
        "ORDER BY created_ts ASC LIMIT ?1");

    ok = ok && prepare(&stmt_update_status,
        "UPDATE sync_queue SET status = ?2, last_attempt_ts = ?3, "
        "retry_count = ?4 WHERE id = ?1");

    ok = ok && prepare(&stmt_count_by_status,
        "SELECT status, COUNT(*) FROM sync_queue GROUP BY status");

    ok = ok && prepare(&stmt_count_total,
        "SELECT COUNT(*) FROM sync_queue");

    ok = ok && prepare(&stmt_purge_sent,
        "DELETE FROM sync_queue WHERE status = 2 AND created_ts < ?1");

    ok = ok && prepare(&stmt_retry_failed,
        "UPDATE sync_queue SET status = 0, last_attempt_ts = 0 "
        "WHERE status = 3 AND retry_count < max_retries");

    if (!ok) {
        fprintf(stderr, "[sync_queue] Statement preparation failed\n");
        sync_queue_close();
        return false;
    }

    printf("[sync_queue] Initialized: %s\n", path);
    return true;
}

void sync_queue_close(void) {
    finalize_stmt(&stmt_push);
    finalize_stmt(&stmt_get_pending);
    finalize_stmt(&stmt_update_status);
    finalize_stmt(&stmt_count_by_status);
    finalize_stmt(&stmt_count_total);
    finalize_stmt(&stmt_purge_sent);
    finalize_stmt(&stmt_retry_failed);

    if (db) {
        sqlite3_close(db);
        db = NULL;
        printf("[sync_queue] Closed\n");
    }
}

/* ── Enqueue ─────────────────────────────────────────────── */

bool sync_queue_push(sync_item_type_t type, const char *json_payload) {
    if (!db || !stmt_push || !json_payload) return false;

    /* Check queue capacity */
    sync_queue_stats_t stats = sync_queue_get_stats();
    if (stats.total >= SYNC_QUEUE_MAX) {
        printf("[sync_queue] Queue full (%d items), rejecting push\n",
               stats.total);
        return false;
    }

    uint32_t now = (uint32_t)time(NULL);

    sqlite3_reset(stmt_push);
    sqlite3_bind_int(stmt_push, 1, (int)type);
    sqlite3_bind_text(stmt_push, 2, json_payload, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt_push, 3, (int)now);
    sqlite3_bind_int(stmt_push, 4, DEFAULT_MAX_RETRIES);

    int rc = sqlite3_step(stmt_push);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "[sync_queue] Push failed: %s\n", sqlite3_errmsg(db));
        return false;
    }

    int64_t row_id = sqlite3_last_insert_rowid(db);
    printf("[sync_queue] Pushed item id=%lld type=%d (%zu bytes)\n",
           (long long)row_id, (int)type, strlen(json_payload));

    return true;
}

/* ── Processing ──────────────────────────────────────────── */

/**
 * Attempt to send a single queue item via fhir_client.
 * Returns true if the export succeeded.
 */
static bool attempt_send(const sync_queue_item_t *item) {
    if (!item) return false;

    switch (item->type) {
        case SYNC_ITEM_VITALS: {
            /*
             * In a real implementation, we would parse the JSON payload
             * back into a fhir_vitals_observation_t. For the simulator
             * stub, we construct a minimal observation from the payload
             * and let fhir_client handle it.
             *
             * Since fhir_client_export_observation is stubbed (always
             * succeeds), we just call it with a default observation.
             */
            fhir_vitals_observation_t obs;
            memset(&obs, 0, sizeof(obs));
            strncpy(obs.patient_id, "stub-patient",
                    sizeof(obs.patient_id) - 1);
            obs.timestamp_ms = (uint64_t)item->created_ts * 1000;

            fhir_result_t result = fhir_client_export_observation(&obs);
            return result.success;
        }

        case SYNC_ITEM_PATIENT: {
            fhir_result_t result = fhir_client_export_patient(
                "Sync Patient", "MRN-SYNC", NULL, NULL);
            return result.success;
        }

        case SYNC_ITEM_ALARM_EVENT:
        case SYNC_ITEM_AUDIT_EVENT:
            /*
             * Alarm and audit events would use dedicated FHIR resources
             * (e.g., AuditEvent, Flag) in the full implementation.
             * For now, the stub always succeeds.
             */
            printf("[sync_queue] Export %s item id=%d (stub: success)\n",
                   item->type == SYNC_ITEM_ALARM_EVENT ? "alarm" : "audit",
                   item->id);
            return true;

        default:
            printf("[sync_queue] Unknown item type %d\n", item->type);
            return false;
    }
}

/**
 * Update the status of a queue item after a send attempt.
 */
static void update_item_status(int32_t id, sync_status_t status,
                                int retry_count) {
    if (!db || !stmt_update_status) return;

    uint32_t now = (uint32_t)time(NULL);

    sqlite3_reset(stmt_update_status);
    sqlite3_bind_int(stmt_update_status, 1, id);
    sqlite3_bind_int(stmt_update_status, 2, (int)status);
    sqlite3_bind_int(stmt_update_status, 3, (int)now);
    sqlite3_bind_int(stmt_update_status, 4, retry_count);
    sqlite3_step(stmt_update_status);
}

int sync_queue_process(int max_items) {
    if (!db || !stmt_get_pending || max_items <= 0) return 0;

    /* Fetch pending items */
    sqlite3_reset(stmt_get_pending);
    sqlite3_bind_int(stmt_get_pending, 1, max_items);

    /* Collect items into a local buffer first (to avoid holding the
     * SELECT cursor open while we UPDATE) */
    sync_queue_item_t items[16];
    int count = 0;
    int max_fetch = max_items > 16 ? 16 : max_items;

    while (sqlite3_step(stmt_get_pending) == SQLITE_ROW && count < max_fetch) {
        sync_queue_item_t *item = &items[count];

        item->id           = sqlite3_column_int(stmt_get_pending, 0);
        item->type         = (sync_item_type_t)sqlite3_column_int(stmt_get_pending, 1);
        item->status       = (sync_status_t)sqlite3_column_int(stmt_get_pending, 2);
        item->created_ts   = (uint32_t)sqlite3_column_int(stmt_get_pending, 4);
        item->last_attempt_ts = (uint32_t)sqlite3_column_int(stmt_get_pending, 5);
        item->retry_count  = sqlite3_column_int(stmt_get_pending, 6);
        item->max_retries  = sqlite3_column_int(stmt_get_pending, 7);

        /* Copy payload */
        const char *payload = (const char *)sqlite3_column_text(stmt_get_pending, 3);
        if (payload) {
            strncpy(item->payload, payload, SYNC_PAYLOAD_MAX - 1);
            item->payload[SYNC_PAYLOAD_MAX - 1] = '\0';
        } else {
            item->payload[0] = '\0';
        }

        count++;
    }

    if (count == 0) return 0;

    printf("[sync_queue] Processing %d pending items\n", count);

    /* Process each item */
    int sent_count = 0;
    for (int i = 0; i < count; i++) {
        sync_queue_item_t *item = &items[i];

        /* Mark as SENDING */
        update_item_status(item->id, SYNC_STATUS_SENDING, item->retry_count);

        /* Attempt export */
        bool success = attempt_send(item);
        item->retry_count++;

        if (success) {
            update_item_status(item->id, SYNC_STATUS_SENT, item->retry_count);
            sent_count++;
            printf("[sync_queue] Item id=%d sent successfully\n", item->id);
        } else {
            sync_status_t new_status;
            if (item->retry_count >= item->max_retries) {
                new_status = SYNC_STATUS_FAILED;
                printf("[sync_queue] Item id=%d failed permanently "
                       "(%d/%d retries)\n",
                       item->id, item->retry_count, item->max_retries);
            } else {
                new_status = SYNC_STATUS_RETRY;
                printf("[sync_queue] Item id=%d will retry "
                       "(%d/%d retries)\n",
                       item->id, item->retry_count, item->max_retries);
            }
            update_item_status(item->id, new_status, item->retry_count);
        }
    }

    printf("[sync_queue] Processed %d/%d items successfully\n",
           sent_count, count);

    return sent_count;
}

/* ── Queries ─────────────────────────────────────────────── */

sync_queue_stats_t sync_queue_get_stats(void) {
    sync_queue_stats_t stats;
    memset(&stats, 0, sizeof(stats));

    if (!db || !stmt_count_by_status || !stmt_count_total) return stats;

    /* Get total count */
    sqlite3_reset(stmt_count_total);
    if (sqlite3_step(stmt_count_total) == SQLITE_ROW) {
        stats.total = sqlite3_column_int(stmt_count_total, 0);
    }

    /* Get counts by status */
    sqlite3_reset(stmt_count_by_status);
    while (sqlite3_step(stmt_count_by_status) == SQLITE_ROW) {
        int status_val = sqlite3_column_int(stmt_count_by_status, 0);
        int count      = sqlite3_column_int(stmt_count_by_status, 1);

        switch ((sync_status_t)status_val) {
            case SYNC_STATUS_PENDING: stats.pending += count; break;
            case SYNC_STATUS_SENDING: stats.sending += count; break;
            case SYNC_STATUS_SENT:    stats.sent    += count; break;
            case SYNC_STATUS_FAILED:  stats.failed  += count; break;
            case SYNC_STATUS_RETRY:   stats.pending += count; break;  /* Count retry as pending */
        }
    }

    return stats;
}

int sync_queue_get_pending(sync_queue_item_t *out, int max_count) {
    if (!db || !stmt_get_pending || !out || max_count <= 0) return 0;

    sqlite3_reset(stmt_get_pending);
    sqlite3_bind_int(stmt_get_pending, 1, max_count);

    int count = 0;
    while (sqlite3_step(stmt_get_pending) == SQLITE_ROW && count < max_count) {
        sync_queue_item_t *item = &out[count];

        item->id           = sqlite3_column_int(stmt_get_pending, 0);
        item->type         = (sync_item_type_t)sqlite3_column_int(stmt_get_pending, 1);
        item->status       = (sync_status_t)sqlite3_column_int(stmt_get_pending, 2);
        item->created_ts   = (uint32_t)sqlite3_column_int(stmt_get_pending, 4);
        item->last_attempt_ts = (uint32_t)sqlite3_column_int(stmt_get_pending, 5);
        item->retry_count  = sqlite3_column_int(stmt_get_pending, 6);
        item->max_retries  = sqlite3_column_int(stmt_get_pending, 7);

        const char *payload = (const char *)sqlite3_column_text(stmt_get_pending, 3);
        if (payload) {
            strncpy(item->payload, payload, SYNC_PAYLOAD_MAX - 1);
            item->payload[SYNC_PAYLOAD_MAX - 1] = '\0';
        } else {
            item->payload[0] = '\0';
        }

        count++;
    }

    return count;
}

/* ── Maintenance ─────────────────────────────────────────── */

void sync_queue_purge_sent(uint32_t max_age_s) {
    if (!db || !stmt_purge_sent) return;

    uint32_t cutoff = (uint32_t)time(NULL);
    if (cutoff > max_age_s) {
        cutoff -= max_age_s;
    } else {
        cutoff = 0;
    }

    sqlite3_reset(stmt_purge_sent);
    sqlite3_bind_int(stmt_purge_sent, 1, (int)cutoff);

    int rc = sqlite3_step(stmt_purge_sent);
    if (rc == SQLITE_DONE) {
        int deleted = sqlite3_changes(db);
        if (deleted > 0) {
            printf("[sync_queue] Purged %d sent items older than %u seconds\n",
                   deleted, max_age_s);
        }
    }
}

void sync_queue_retry_failed(void) {
    if (!db || !stmt_retry_failed) return;

    sqlite3_reset(stmt_retry_failed);

    int rc = sqlite3_step(stmt_retry_failed);
    if (rc == SQLITE_DONE) {
        int reset_count = sqlite3_changes(db);
        if (reset_count > 0) {
            printf("[sync_queue] Reset %d failed items to pending\n",
                   reset_count);
        }
    }
}
