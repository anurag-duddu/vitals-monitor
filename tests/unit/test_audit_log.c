/**
 * @file test_audit_log.c
 * @brief Unit tests for audit_log module
 *
 * Tests event recording, querying by recency/user/event/range,
 * purge of old entries, and edge cases using in-memory SQLite.
 */

#include "test_framework.h"
#include "audit_log.h"
#include <time.h>

/* ── Test: init and close ────────────────────────────────── */

static void test_init_close(void) {
    printf("  test_init_close\n");

    bool ok = audit_log_init(NULL);  /* in-memory */
    ASSERT_TRUE(ok);

    audit_log_close();
}

/* ── Test: record and query recent ───────────────────────── */

static void test_record_and_query_recent(void) {
    printf("  test_record_and_query_recent\n");

    audit_log_init(NULL);

    /* Record several events */
    audit_log_record(AUDIT_EVENT_SYSTEM_START, "system", "System started");
    audit_log_record(AUDIT_EVENT_LOGIN, "admin", "Admin logged in");
    audit_log_record(AUDIT_EVENT_ALARM_ACK, "nurse", "HR alarm acknowledged");

    /* Query recent */
    audit_query_result_t result;
    int count = audit_log_query_recent(10, &result);
    ASSERT_EQ_INT(count, 3);
    ASSERT_EQ_INT(result.count, 3);

    /* Results are newest-first, but since timestamps use time(NULL) they may
     * all have the same timestamp. Just verify we got 3 entries. */
    ASSERT_GT_INT(result.entries[0].id, 0);

    audit_log_close();
}

/* ── Test: query with max_count limit ────────────────────── */

static void test_query_recent_limited(void) {
    printf("  test_query_recent_limited\n");

    audit_log_init(NULL);

    audit_log_record(AUDIT_EVENT_LOGIN, "user1", "msg1");
    audit_log_record(AUDIT_EVENT_LOGIN, "user2", "msg2");
    audit_log_record(AUDIT_EVENT_LOGIN, "user3", "msg3");
    audit_log_record(AUDIT_EVENT_LOGIN, "user4", "msg4");
    audit_log_record(AUDIT_EVENT_LOGIN, "user5", "msg5");

    audit_query_result_t result;
    int count = audit_log_query_recent(3, &result);
    ASSERT_EQ_INT(count, 3);

    audit_log_close();
}

/* ── Test: query by username ─────────────────────────────── */

static void test_query_by_user(void) {
    printf("  test_query_by_user\n");

    audit_log_init(NULL);

    audit_log_record(AUDIT_EVENT_LOGIN, "admin", "Admin login");
    audit_log_record(AUDIT_EVENT_ALARM_ACK, "nurse", "Nurse ack");
    audit_log_record(AUDIT_EVENT_SETTINGS_CHANGED, "admin", "Admin settings");
    audit_log_record(AUDIT_EVENT_LOGOUT, "nurse", "Nurse logout");

    /* Query for admin */
    audit_query_result_t result;
    int count = audit_log_query_by_user("admin", 10, &result);
    ASSERT_EQ_INT(count, 2);

    /* Verify all results belong to admin */
    for (int i = 0; i < count; i++) {
        ASSERT_STR_EQ(result.entries[i].username, "admin");
    }

    /* Query for nurse */
    count = audit_log_query_by_user("nurse", 10, &result);
    ASSERT_EQ_INT(count, 2);

    /* Query for nonexistent user */
    count = audit_log_query_by_user("nobody", 10, &result);
    ASSERT_EQ_INT(count, 0);

    audit_log_close();
}

/* ── Test: query by event type ───────────────────────────── */

static void test_query_by_event(void) {
    printf("  test_query_by_event\n");

    audit_log_init(NULL);

    audit_log_record(AUDIT_EVENT_LOGIN, "admin", "Login 1");
    audit_log_record(AUDIT_EVENT_ALARM_ACK, "nurse", "Ack 1");
    audit_log_record(AUDIT_EVENT_LOGIN, "doctor", "Login 2");
    audit_log_record(AUDIT_EVENT_LOGIN_FAILED, "unknown", "Failed login");
    audit_log_record(AUDIT_EVENT_LOGIN, "tech", "Login 3");

    /* Query for LOGIN events */
    audit_query_result_t result;
    int count = audit_log_query_by_event(AUDIT_EVENT_LOGIN, 10, &result);
    ASSERT_EQ_INT(count, 3);

    for (int i = 0; i < count; i++) {
        ASSERT_EQ_INT(result.entries[i].event, AUDIT_EVENT_LOGIN);
    }

    /* Query for ALARM_ACK */
    count = audit_log_query_by_event(AUDIT_EVENT_ALARM_ACK, 10, &result);
    ASSERT_EQ_INT(count, 1);

    /* Query for LOGIN_FAILED */
    count = audit_log_query_by_event(AUDIT_EVENT_LOGIN_FAILED, 10, &result);
    ASSERT_EQ_INT(count, 1);

    /* Query for event with no entries */
    count = audit_log_query_by_event(AUDIT_EVENT_SYSTEM_SHUTDOWN, 10, &result);
    ASSERT_EQ_INT(count, 0);

    audit_log_close();
}

/* ── Test: query by time range ───────────────────────────── */

static void test_query_range(void) {
    printf("  test_query_range\n");

    audit_log_init(NULL);

    /* Record events (they will all get time(NULL) as timestamp).
     * Since we can't control timestamp in the API, just verify the
     * range query mechanism works for the "current" second. */
    audit_log_record(AUDIT_EVENT_LOGIN, "admin", "Login");
    audit_log_record(AUDIT_EVENT_LOGOUT, "admin", "Logout");

    /* Query a wide range that includes now */
    audit_query_result_t result;
    uint32_t now = (uint32_t)time(NULL);
    int count = audit_log_query_range(now - 60, now + 60, &result);
    ASSERT_GE_INT(count, 2);

    /* Query a range in the far past should return 0 */
    count = audit_log_query_range(0, 100, &result);
    ASSERT_EQ_INT(count, 0);

    audit_log_close();
}

/* ── Test: record_fmt (formatted message) ────────────────── */

static void test_record_fmt(void) {
    printf("  test_record_fmt\n");

    audit_log_init(NULL);

    audit_log_record_fmt(AUDIT_EVENT_ALARM_LIMITS_CHANGED, "doctor",
                         "HR limits changed: %d-%d", 50, 120);

    audit_query_result_t result;
    int count = audit_log_query_recent(1, &result);
    ASSERT_EQ_INT(count, 1);
    ASSERT_STR_EQ(result.entries[0].message, "HR limits changed: 50-120");
    ASSERT_EQ_INT(result.entries[0].event, AUDIT_EVENT_ALARM_LIMITS_CHANGED);
    ASSERT_STR_EQ(result.entries[0].username, "doctor");

    audit_log_close();
}

/* ── Test: NULL username defaults to "system" ────────────── */

static void test_null_username(void) {
    printf("  test_null_username\n");

    audit_log_init(NULL);

    audit_log_record(AUDIT_EVENT_SYSTEM_START, NULL, "Boot");

    audit_query_result_t result;
    audit_log_query_recent(1, &result);
    ASSERT_STR_EQ(result.entries[0].username, "system");

    audit_log_close();
}

/* ── Test: NULL message defaults to empty ────────────────── */

static void test_null_message(void) {
    printf("  test_null_message\n");

    audit_log_init(NULL);

    audit_log_record(AUDIT_EVENT_LOGOUT, "admin", NULL);

    audit_query_result_t result;
    audit_log_query_recent(1, &result);
    ASSERT_STR_EQ(result.entries[0].message, "");

    audit_log_close();
}

/* ── Test: event name strings ────────────────────────────── */

static void test_event_names(void) {
    printf("  test_event_names\n");

    ASSERT_STR_EQ(audit_event_name(AUDIT_EVENT_LOGIN), "LOGIN");
    ASSERT_STR_EQ(audit_event_name(AUDIT_EVENT_LOGOUT), "LOGOUT");
    ASSERT_STR_EQ(audit_event_name(AUDIT_EVENT_LOGIN_FAILED), "LOGIN_FAILED");
    ASSERT_STR_EQ(audit_event_name(AUDIT_EVENT_SESSION_TIMEOUT), "SESSION_TIMEOUT");
    ASSERT_STR_EQ(audit_event_name(AUDIT_EVENT_ALARM_ACK), "ALARM_ACK");
    ASSERT_STR_EQ(audit_event_name(AUDIT_EVENT_ALARM_SILENCE), "ALARM_SILENCE");
    ASSERT_STR_EQ(audit_event_name(AUDIT_EVENT_ALARM_LIMITS_CHANGED), "ALARM_LIMITS_CHANGED");
    ASSERT_STR_EQ(audit_event_name(AUDIT_EVENT_PATIENT_ADMIT), "PATIENT_ADMIT");
    ASSERT_STR_EQ(audit_event_name(AUDIT_EVENT_PATIENT_DISCHARGE), "PATIENT_DISCHARGE");
    ASSERT_STR_EQ(audit_event_name(AUDIT_EVENT_PATIENT_MODIFIED), "PATIENT_MODIFIED");
    ASSERT_STR_EQ(audit_event_name(AUDIT_EVENT_SETTINGS_CHANGED), "SETTINGS_CHANGED");
    ASSERT_STR_EQ(audit_event_name(AUDIT_EVENT_USER_CREATED), "USER_CREATED");
    ASSERT_STR_EQ(audit_event_name(AUDIT_EVENT_USER_DELETED), "USER_DELETED");
    ASSERT_STR_EQ(audit_event_name(AUDIT_EVENT_SYSTEM_START), "SYSTEM_START");
    ASSERT_STR_EQ(audit_event_name(AUDIT_EVENT_SYSTEM_SHUTDOWN), "SYSTEM_SHUTDOWN");
    ASSERT_STR_EQ(audit_event_name(AUDIT_EVENT_COUNT), "UNKNOWN");
}

/* ── Test: purge old entries ─────────────────────────────── */

static void test_purge(void) {
    printf("  test_purge\n");

    audit_log_init(NULL);

    /* Record some events (all with current timestamp) */
    audit_log_record(AUDIT_EVENT_LOGIN, "admin", "Login 1");
    audit_log_record(AUDIT_EVENT_LOGIN, "admin", "Login 2");
    audit_log_record(AUDIT_EVENT_LOGIN, "admin", "Login 3");

    /* Purge entries older than 0 seconds (should purge everything since
     * the cutoff would be now - 0 = now, and entries have timestamp <= now).
     * Actually, audit_log_purge_old(0) uses AUDIT_DEFAULT_RETENTION_S (30 days),
     * so entries created "now" won't be purged. Let's verify they survive. */
    audit_log_purge_old(0);  /* 0 => use default 30 days */

    audit_query_result_t result;
    int count = audit_log_query_recent(10, &result);
    ASSERT_EQ_INT(count, 3);  /* All entries are fresh, should survive */

    /* Purge with max_age_s = 1 second.  Entries created "now" have
     * timestamp ~= now, and cutoff = now - 1.  Since entries are at
     * timestamp == now and cutoff is now-1, entries with ts >= cutoff
     * survive (DELETE WHERE ts < cutoff). So fresh entries should survive. */
    audit_log_purge_old(1);
    count = audit_log_query_recent(10, &result);
    /* Entries at timestamp=now should still exist (now >= now-1) */
    ASSERT_EQ_INT(count, 3);

    audit_log_close();
}

/* ── Test: many records ──────────────────────────────────── */

static void test_many_records(void) {
    printf("  test_many_records\n");

    audit_log_init(NULL);

    /* Insert more than AUDIT_LOG_QUERY_MAX entries */
    for (int i = 0; i < 150; i++) {
        audit_log_record(AUDIT_EVENT_ALARM_ACK, "nurse", "Ack");
    }

    /* Query should cap at AUDIT_LOG_QUERY_MAX */
    audit_query_result_t result;
    int count = audit_log_query_recent(200, &result);
    ASSERT_EQ_INT(count, AUDIT_LOG_QUERY_MAX);

    audit_log_close();
}

/* ── Test: entry fields are correctly stored ─────────────── */

static void test_entry_fields(void) {
    printf("  test_entry_fields\n");

    audit_log_init(NULL);

    audit_log_record(AUDIT_EVENT_PATIENT_ADMIT, "doctor",
                     "Admitted patient MRN-001");

    audit_query_result_t result;
    int count = audit_log_query_recent(1, &result);
    ASSERT_EQ_INT(count, 1);

    audit_entry_t *e = &result.entries[0];
    ASSERT_GT_INT(e->id, 0);
    ASSERT_EQ_INT(e->event, AUDIT_EVENT_PATIENT_ADMIT);
    ASSERT_STR_EQ(e->username, "doctor");
    ASSERT_STR_EQ(e->message, "Admitted patient MRN-001");
    ASSERT_GT_INT(e->timestamp_s, 0);

    audit_log_close();
}

/* ── Test: empty database queries return 0 ───────────────── */

static void test_empty_queries(void) {
    printf("  test_empty_queries\n");

    audit_log_init(NULL);

    audit_query_result_t result;

    int count = audit_log_query_recent(10, &result);
    ASSERT_EQ_INT(count, 0);

    count = audit_log_query_by_user("admin", 10, &result);
    ASSERT_EQ_INT(count, 0);

    count = audit_log_query_by_event(AUDIT_EVENT_LOGIN, 10, &result);
    ASSERT_EQ_INT(count, 0);

    uint32_t now = (uint32_t)time(NULL);
    count = audit_log_query_range(now - 60, now + 60, &result);
    ASSERT_EQ_INT(count, 0);

    audit_log_close();
}

/* ── Public entry point ──────────────────────────────────── */

void test_audit_log(void) {
    test_init_close();
    test_record_and_query_recent();
    test_query_recent_limited();
    test_query_by_user();
    test_query_by_event();
    test_query_range();
    test_record_fmt();
    test_null_username();
    test_null_message();
    test_event_names();
    test_purge();
    test_many_records();
    test_entry_fields();
    test_empty_queries();
}
