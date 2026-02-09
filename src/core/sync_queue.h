/**
 * @file sync_queue.h
 * @brief Offline-first sync queue with SQLite persistence
 *
 * Provides a durable, ordered queue of outbound data items (vitals
 * observations, patient records, alarm events, audit events) that
 * persist across reboots via SQLite.
 *
 * Processing workflow:
 *   1. Caller pushes items via sync_queue_push()
 *   2. Periodic timer calls sync_queue_process() when network is available
 *   3. Each item is marked SENDING, exported via fhir_client, then
 *      marked SENT or FAILED
 *   4. Failed items are retried up to max_retries times
 *   5. Successfully sent items are purged after a configurable age
 *
 * In the simulator build, fhir_client export always succeeds,
 * so process() will mark all pending items as SENT.
 *
 * Thread safety: All calls are single-threaded (LVGL main loop).
 */

#ifndef SYNC_QUEUE_H
#define SYNC_QUEUE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Constants ─────────────────────────────────────────────── */

#define SYNC_QUEUE_MAX     256     /* Max items in queue before rejecting */
#define SYNC_PAYLOAD_MAX   4096    /* Max JSON payload per item */

/* ── Item types ────────────────────────────────────────────── */

typedef enum {
    SYNC_ITEM_VITALS = 0,          /* Periodic vitals observation */
    SYNC_ITEM_PATIENT,             /* Patient record */
    SYNC_ITEM_ALARM_EVENT,         /* Alarm event */
    SYNC_ITEM_AUDIT_EVENT,         /* Audit log event */
    SYNC_ITEM_TYPE_COUNT
} sync_item_type_t;

/* ── Item status ───────────────────────────────────────────── */

typedef enum {
    SYNC_STATUS_PENDING = 0,
    SYNC_STATUS_SENDING,
    SYNC_STATUS_SENT,
    SYNC_STATUS_FAILED,
    SYNC_STATUS_RETRY
} sync_status_t;

/* ── Queue item ────────────────────────────────────────────── */

typedef struct {
    int32_t          id;               /* SQLite rowid */
    sync_item_type_t type;
    sync_status_t    status;
    char             payload[SYNC_PAYLOAD_MAX];   /* JSON payload */
    uint32_t         created_ts;       /* Epoch seconds when enqueued */
    uint32_t         last_attempt_ts;  /* Epoch seconds of last send attempt */
    int              retry_count;      /* Number of send attempts */
    int              max_retries;      /* Maximum retries (default: 5) */
} sync_queue_item_t;

/* ── Queue statistics ──────────────────────────────────────── */

typedef struct {
    int pending;
    int sending;
    int sent;
    int failed;
    int total;
} sync_queue_stats_t;

/* ── Lifecycle ─────────────────────────────────────────────── */

/**
 * Initialize the sync queue with SQLite persistence.
 * Creates or opens the sync_queue table in the given database file.
 * @param db_path  Path to SQLite database file (NULL for in-memory).
 * @return true on success.
 */
bool sync_queue_init(const char *db_path);

/** Close the sync queue and finalize all prepared statements. */
void sync_queue_close(void);

/* ── Enqueue ───────────────────────────────────────────────── */

/**
 * Push a new item onto the sync queue.
 * @param type          Item type (vitals, patient, alarm, audit).
 * @param json_payload  JSON payload to send (copied into queue).
 * @return true if item was enqueued successfully.
 */
bool sync_queue_push(sync_item_type_t type, const char *json_payload);

/* ── Processing ────────────────────────────────────────────── */

/**
 * Process pending items in the queue.
 * Should be called periodically when network is available.
 * @param max_items  Maximum number of items to process in one call.
 * @return Number of items successfully processed (sent).
 */
int sync_queue_process(int max_items);

/* ── Queries ───────────────────────────────────────────────── */

/** Get summary statistics for all items in the queue. */
sync_queue_stats_t sync_queue_get_stats(void);

/**
 * Get pending items from the queue.
 * @param out        Output array of queue items.
 * @param max_count  Maximum number of items to retrieve.
 * @return Number of items written to out.
 */
int sync_queue_get_pending(sync_queue_item_t *out, int max_count);

/* ── Maintenance ───────────────────────────────────────────── */

/**
 * Purge successfully sent items older than max_age_s.
 * @param max_age_s  Maximum age in seconds (e.g. 86400 for 24 hours).
 */
void sync_queue_purge_sent(uint32_t max_age_s);

/** Reset all failed items back to pending status for retry. */
void sync_queue_retry_failed(void);

#ifdef __cplusplus
}
#endif

#endif /* SYNC_QUEUE_H */
