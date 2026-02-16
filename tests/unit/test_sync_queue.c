/**
 * @file test_sync_queue.c
 * @brief Unit tests for sync_queue module (TEST-5.4)
 *
 * Tests the offline-first sync queue: lifecycle, push/get/process,
 * stats tracking, NULL-safety, capacity enforcement, and close/re-init.
 *
 * The sync_queue depends on fhir_client for process() — in the
 * simulator build the fhir_client stubs always succeed, so
 * process() marks all pending items as SENT.
 */

#include "test_framework.h"
#include "sync_queue.h"
#include "fhir_client.h"
#include <string.h>

/* Static buffer for retrieving pending items */
static sync_queue_item_t items[4];

/* ── Test: init and close lifecycle ──────────────────────── */

static void test_init_close(void) {
    printf("  test_init_close\n");

    /* Init with in-memory DB */
    bool ok = sync_queue_init(NULL);
    ASSERT_TRUE(ok);

    /* Stats should be empty after fresh init */
    sync_queue_stats_t stats = sync_queue_get_stats();
    ASSERT_EQ_INT(stats.total, 0);
    ASSERT_EQ_INT(stats.pending, 0);
    ASSERT_EQ_INT(stats.sent, 0);
    ASSERT_EQ_INT(stats.failed, 0);

    sync_queue_close();

    /* Double-close should not crash */
    sync_queue_close();
}

/* ── Test: push items and verify stats ───────────────────── */

static void test_push_and_stats(void) {
    printf("  test_push_and_stats\n");

    sync_queue_init(NULL);
    fhir_client_init();

    /* Push 3 items of different types */
    bool ok;
    ok = sync_queue_push(SYNC_ITEM_VITALS, "{\"hr\":72}");
    ASSERT_TRUE(ok);

    ok = sync_queue_push(SYNC_ITEM_PATIENT, "{\"name\":\"Test\"}");
    ASSERT_TRUE(ok);

    ok = sync_queue_push(SYNC_ITEM_ALARM_EVENT, "{\"alarm\":\"high_hr\"}");
    ASSERT_TRUE(ok);

    sync_queue_stats_t stats = sync_queue_get_stats();
    ASSERT_EQ_INT(stats.total, 3);
    ASSERT_EQ_INT(stats.pending, 3);
    ASSERT_EQ_INT(stats.sent, 0);

    fhir_client_deinit();
    sync_queue_close();
}

/* ── Test: push and get_pending (verify payload, type, id) ── */

static void test_push_get_pending(void) {
    printf("  test_push_get_pending\n");

    sync_queue_init(NULL);
    fhir_client_init();

    sync_queue_push(SYNC_ITEM_VITALS, "{\"hr\":80}");
    sync_queue_push(SYNC_ITEM_AUDIT_EVENT, "{\"action\":\"login\"}");

    memset(items, 0, sizeof(items));
    int count = sync_queue_get_pending(items, 4);
    ASSERT_EQ_INT(count, 2);

    /* First item: vitals */
    ASSERT_GT_INT(items[0].id, 0);
    ASSERT_EQ_INT(items[0].type, SYNC_ITEM_VITALS);
    ASSERT_EQ_INT(items[0].status, SYNC_STATUS_PENDING);
    ASSERT_STR_EQ(items[0].payload, "{\"hr\":80}");

    /* Second item: audit event */
    ASSERT_GT_INT(items[1].id, 0);
    ASSERT_EQ_INT(items[1].type, SYNC_ITEM_AUDIT_EVENT);
    ASSERT_STR_EQ(items[1].payload, "{\"action\":\"login\"}");

    /* IDs should be monotonically increasing */
    ASSERT_GT_INT(items[1].id, items[0].id);

    fhir_client_deinit();
    sync_queue_close();
}

/* ── Test: process items (push, process, verify sent) ────── */

static void test_process_items(void) {
    printf("  test_process_items\n");

    sync_queue_init(NULL);
    fhir_client_init();

    sync_queue_push(SYNC_ITEM_VITALS, "{\"hr\":72}");
    sync_queue_push(SYNC_ITEM_PATIENT, "{\"name\":\"Kumar\"}");

    /* Process all pending — fhir stubs always succeed */
    int sent = sync_queue_process(10);
    ASSERT_EQ_INT(sent, 2);

    /* Stats: both should now be SENT */
    sync_queue_stats_t stats = sync_queue_get_stats();
    ASSERT_EQ_INT(stats.sent, 2);
    ASSERT_EQ_INT(stats.pending, 0);
    ASSERT_EQ_INT(stats.total, 2);

    /* get_pending should return 0 (nothing pending) */
    memset(items, 0, sizeof(items));
    int pending = sync_queue_get_pending(items, 4);
    ASSERT_EQ_INT(pending, 0);

    fhir_client_deinit();
    sync_queue_close();
}

/* ── Test: stats tracking (pending/sent/failed counts) ───── */

static void test_stats_tracking(void) {
    printf("  test_stats_tracking\n");

    sync_queue_init(NULL);
    fhir_client_init();

    /* Push 4 items */
    sync_queue_push(SYNC_ITEM_VITALS, "{\"hr\":60}");
    sync_queue_push(SYNC_ITEM_VITALS, "{\"hr\":65}");
    sync_queue_push(SYNC_ITEM_ALARM_EVENT, "{\"alarm\":\"low_spo2\"}");
    sync_queue_push(SYNC_ITEM_AUDIT_EVENT, "{\"action\":\"logout\"}");

    sync_queue_stats_t s1 = sync_queue_get_stats();
    ASSERT_EQ_INT(s1.total, 4);
    ASSERT_EQ_INT(s1.pending, 4);

    /* Process only 2 */
    int sent = sync_queue_process(2);
    ASSERT_EQ_INT(sent, 2);

    sync_queue_stats_t s2 = sync_queue_get_stats();
    ASSERT_EQ_INT(s2.sent, 2);
    ASSERT_EQ_INT(s2.pending, 2);
    ASSERT_EQ_INT(s2.total, 4);

    fhir_client_deinit();
    sync_queue_close();
}

/* ── Test: close and re-init lifecycle ───────────────────── */

static void test_close_reinit_lifecycle(void) {
    printf("  test_close_reinit_lifecycle\n");

    /*
     * With :memory: each open creates a fresh DB, so items do NOT
     * persist across close/re-init. This test verifies the module
     * survives the close+re-init cycle and starts clean.
     */
    sync_queue_init(NULL);
    fhir_client_init();

    sync_queue_push(SYNC_ITEM_VITALS, "{\"hr\":99}");

    sync_queue_stats_t s1 = sync_queue_get_stats();
    ASSERT_EQ_INT(s1.total, 1);

    sync_queue_close();

    /* Re-init: in-memory DB is brand new */
    bool ok = sync_queue_init(NULL);
    ASSERT_TRUE(ok);

    sync_queue_stats_t s2 = sync_queue_get_stats();
    ASSERT_EQ_INT(s2.total, 0);
    ASSERT_EQ_INT(s2.pending, 0);

    /* Push should still work after re-init */
    ok = sync_queue_push(SYNC_ITEM_PATIENT, "{\"name\":\"Re-init\"}");
    ASSERT_TRUE(ok);

    sync_queue_stats_t s3 = sync_queue_get_stats();
    ASSERT_EQ_INT(s3.total, 1);

    fhir_client_deinit();
    sync_queue_close();
}

/* ── Test: NULL payload rejected ─────────────────────────── */

static void test_null_payload_rejected(void) {
    printf("  test_null_payload_rejected\n");

    sync_queue_init(NULL);
    fhir_client_init();

    bool ok = sync_queue_push(SYNC_ITEM_VITALS, NULL);
    ASSERT_FALSE(ok);

    /* Queue should remain empty */
    sync_queue_stats_t stats = sync_queue_get_stats();
    ASSERT_EQ_INT(stats.total, 0);

    fhir_client_deinit();
    sync_queue_close();
}

/* ── Test: empty queue process returns 0 ─────────────────── */

static void test_empty_process(void) {
    printf("  test_empty_process\n");

    sync_queue_init(NULL);
    fhir_client_init();

    int sent = sync_queue_process(10);
    ASSERT_EQ_INT(sent, 0);

    /* Zero or negative max_items should also return 0 */
    sent = sync_queue_process(0);
    ASSERT_EQ_INT(sent, 0);

    fhir_client_deinit();
    sync_queue_close();
}

/* ── Test: queue capacity tracking ───────────────────────── */

static void test_capacity_tracking(void) {
    printf("  test_capacity_tracking\n");

    sync_queue_init(NULL);
    fhir_client_init();

    /* Push a batch and verify total tracks correctly */
    for (int i = 0; i < 5; i++) {
        bool ok = sync_queue_push(SYNC_ITEM_VITALS, "{\"hr\":70}");
        ASSERT_TRUE(ok);
    }

    sync_queue_stats_t stats = sync_queue_get_stats();
    ASSERT_EQ_INT(stats.total, 5);
    ASSERT_EQ_INT(stats.pending, 5);

    fhir_client_deinit();
    sync_queue_close();
}

/* ── Test: get_pending with NULL out or zero max ─────────── */

static void test_get_pending_null_safety(void) {
    printf("  test_get_pending_null_safety\n");

    sync_queue_init(NULL);
    fhir_client_init();

    sync_queue_push(SYNC_ITEM_VITALS, "{\"hr\":55}");

    /* NULL output buffer */
    int count = sync_queue_get_pending(NULL, 4);
    ASSERT_EQ_INT(count, 0);

    /* Zero max_count */
    count = sync_queue_get_pending(items, 0);
    ASSERT_EQ_INT(count, 0);

    fhir_client_deinit();
    sync_queue_close();
}

/* ── Public entry point ──────────────────────────────────── */

void test_sync_queue(void) {
    test_init_close();
    test_push_and_stats();
    test_push_get_pending();
    test_process_items();
    test_stats_tracking();
    test_close_reinit_lifecycle();
    test_null_payload_rejected();
    test_empty_process();
    test_capacity_tracking();
    test_get_pending_null_safety();
}
