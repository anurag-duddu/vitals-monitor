/**
 * @file test_trend_db.c
 * @brief Unit tests for trend_db module
 *
 * Tests SQLite-backed trend storage including lifecycle management,
 * raw sample insertion/query, NIBP measurements, alarm events,
 * minute-level aggregation, data purge, and edge cases.
 *
 * All tests use in-memory SQLite databases (NULL path) so no files
 * are created on disk.
 */

#include "test_framework.h"
#include "trend_db.h"

/* ── Static result buffers (too large for stack) ─────────── */

static trend_query_result_t  s_param_result;
static trend_nibp_result_t   s_nibp_result;
static trend_alarm_result_t  s_alarm_result;

/* ── Helpers ─────────────────────────────────────────────── */

/** Clear the param result buffer before each use. */
static void clear_param_result(void) {
    memset(&s_param_result, 0, sizeof(s_param_result));
}

static void clear_nibp_result(void) {
    memset(&s_nibp_result, 0, sizeof(s_nibp_result));
}

static void clear_alarm_result(void) {
    memset(&s_alarm_result, 0, sizeof(s_alarm_result));
}

/* ── Test 1: Init/close lifecycle ────────────────────────── */

static void test_init_close(void) {
    printf("  test_init_close\n");

    /* Init with NULL path (in-memory DB) should succeed */
    bool ok = trend_db_init(NULL);
    ASSERT_TRUE(ok);

    /* Close should not crash */
    trend_db_close();

    /* Double close should be safe */
    trend_db_close();
}

/* ── Test 2: Re-init after close ─────────────────────────── */

static void test_reinit(void) {
    printf("  test_reinit\n");

    bool ok = trend_db_init(NULL);
    ASSERT_TRUE(ok);
    trend_db_close();

    /* Re-init should work cleanly */
    ok = trend_db_init(NULL);
    ASSERT_TRUE(ok);
    trend_db_close();
}

/* ── Test 3: Insert and query raw HR samples ─────────────── */

static void test_insert_query_raw_hr(void) {
    printf("  test_insert_query_raw_hr\n");

    trend_db_init(NULL);

    /* Insert 10 raw samples at 1-second intervals starting at ts=1000 */
    for (int i = 0; i < 10; i++) {
        trend_db_insert_sample(1000 + (uint32_t)i, 72 + i, 97, 16, 36.8f);
    }

    /* Query HR over the short range (<=2h uses vitals_raw table) */
    clear_param_result();
    int count = trend_db_query_param(TREND_PARAM_HR, 1000, 1009, 480,
                                     &s_param_result);

    ASSERT_GT_INT(count, 0);
    ASSERT_EQ_INT(s_param_result.count, count);

    /* First point should have timestamp >= 1000 */
    ASSERT_GE_INT((int)s_param_result.timestamp_s[0], 1000);

    /* Values should be in the 72-81 range */
    ASSERT_GE_INT(s_param_result.value[0], 72);

    trend_db_close();
}

/* ── Test 4: Insert and query SpO2 samples ───────────────── */

static void test_insert_query_raw_spo2(void) {
    printf("  test_insert_query_raw_spo2\n");

    trend_db_init(NULL);

    for (int i = 0; i < 5; i++) {
        trend_db_insert_sample(2000 + (uint32_t)i, 72, 95 + i, 16, 36.5f);
    }

    clear_param_result();
    int count = trend_db_query_param(TREND_PARAM_SPO2, 2000, 2004, 480,
                                     &s_param_result);

    ASSERT_GT_INT(count, 0);
    ASSERT_GE_INT(s_param_result.value[0], 95);

    trend_db_close();
}

/* ── Test 5: Insert and query temperature (stored as x10) ── */

static void test_insert_query_raw_temp(void) {
    printf("  test_insert_query_raw_temp\n");

    trend_db_init(NULL);

    /* Insert temp=37.5 -> stored as 375 */
    trend_db_insert_sample(3000, 72, 97, 16, 37.5f);
    trend_db_insert_sample(3001, 72, 97, 16, 36.0f);

    clear_param_result();
    int count = trend_db_query_param(TREND_PARAM_TEMP, 3000, 3001, 480,
                                     &s_param_result);

    ASSERT_GT_INT(count, 0);
    /* The average of 375 and 360 depends on GROUP BY interval.
     * With interval=1 we should get 2 separate points. */

    trend_db_close();
}

/* ── Test 6: Insert and query NIBP measurements ──────────── */

static void test_insert_query_nibp(void) {
    printf("  test_insert_query_nibp\n");

    trend_db_init(NULL);

    trend_db_insert_nibp(5000, 120, 80, 93);
    trend_db_insert_nibp(5060, 130, 85, 100);
    trend_db_insert_nibp(5120, 118, 78, 91);

    clear_nibp_result();
    int count = trend_db_query_nibp(5000, 5120, &s_nibp_result);

    ASSERT_EQ_INT(count, 3);
    ASSERT_EQ_INT(s_nibp_result.count, 3);

    /* Verify first NIBP reading */
    ASSERT_EQ_INT((int)s_nibp_result.timestamp_s[0], 5000);
    ASSERT_EQ_INT(s_nibp_result.sys[0], 120);
    ASSERT_EQ_INT(s_nibp_result.dia[0], 80);
    ASSERT_EQ_INT(s_nibp_result.map_val[0], 93);

    /* Verify second reading */
    ASSERT_EQ_INT(s_nibp_result.sys[1], 130);
    ASSERT_EQ_INT(s_nibp_result.dia[1], 85);

    trend_db_close();
}

/* ── Test 7: Insert and query alarm events ───────────────── */

static void test_insert_query_alarms(void) {
    printf("  test_insert_query_alarms\n");

    trend_db_init(NULL);

    trend_db_insert_alarm(6000, VM_ALARM_HIGH, "HR > 150 bpm");
    trend_db_insert_alarm(6010, VM_ALARM_MEDIUM, "SpO2 < 92%");
    trend_db_insert_alarm(6020, VM_ALARM_LOW, "RR elevated");

    clear_alarm_result();
    int count = trend_db_query_alarms(6000, 6020, &s_alarm_result);

    ASSERT_EQ_INT(count, 3);
    ASSERT_EQ_INT(s_alarm_result.count, 3);

    /* Verify first alarm */
    ASSERT_EQ_INT((int)s_alarm_result.timestamp_s[0], 6000);
    ASSERT_EQ_INT(s_alarm_result.severity[0], VM_ALARM_HIGH);
    ASSERT_STR_EQ(s_alarm_result.message[0], "HR > 150 bpm");

    /* Verify second alarm */
    ASSERT_EQ_INT(s_alarm_result.severity[1], VM_ALARM_MEDIUM);
    ASSERT_STR_EQ(s_alarm_result.message[1], "SpO2 < 92%");

    /* Verify third alarm */
    ASSERT_EQ_INT(s_alarm_result.severity[2], VM_ALARM_LOW);

    trend_db_close();
}

/* ── Test 8: Minute aggregation ──────────────────────────── */

static void test_aggregation(void) {
    printf("  test_aggregation\n");

    trend_db_init(NULL);

    /* Insert 60 raw samples covering one full minute (ts 10001..10060).
     * HR values range from 60 to 119 for easy min/max/avg verification. */
    uint32_t base_ts = 10000;
    for (int i = 1; i <= 60; i++) {
        trend_db_insert_sample(base_ts + (uint32_t)i,
                               60 + i,    /* HR: 61..120 */
                               96,        /* SpO2 constant */
                               15,        /* RR constant */
                               37.0f);    /* Temp constant */
    }

    /* Aggregate the minute ending at base_ts + 60.
     * The aggregation window is [minute_boundary - 59, minute_boundary]
     * which is [10001, 10060]. */
    trend_db_aggregate_minute(base_ts + 60);

    /* Query using long range (>2h) to hit the vitals_1min table.
     * We ask for the range [base_ts, base_ts + 7201] which is >2h. */
    clear_param_result();
    int count = trend_db_query_param(TREND_PARAM_HR,
                                     base_ts, base_ts + 7201,
                                     480, &s_param_result);

    ASSERT_EQ_INT(count, 1);
    ASSERT_EQ_INT(s_param_result.count, 1);

    /* AVG of HR 61..120 = 90 (integer average via SQLite) */
    ASSERT_EQ_INT(s_param_result.value[0], 90);

    /* MIN should be 61, MAX should be 120 */
    ASSERT_EQ_INT(s_param_result.value_min[0], 61);
    ASSERT_EQ_INT(s_param_result.value_max[0], 120);

    trend_db_close();
}

/* ── Test 9: Aggregation with no data produces no row ────── */

static void test_aggregation_empty(void) {
    printf("  test_aggregation_empty\n");

    trend_db_init(NULL);

    /* Aggregate a minute with no raw data -- should produce nothing */
    trend_db_aggregate_minute(99960);

    clear_param_result();
    int count = trend_db_query_param(TREND_PARAM_HR,
                                     99900, 99900 + 7201,
                                     480, &s_param_result);
    ASSERT_EQ_INT(count, 0);

    trend_db_close();
}

/* ── Test 10: Purge old raw data ─────────────────────────── */

static void test_purge_raw(void) {
    printf("  test_purge_raw\n");

    trend_db_init(NULL);

    /* Insert samples at "old" timestamps (ts=100) */
    for (int i = 0; i < 5; i++) {
        trend_db_insert_sample(100 + (uint32_t)i, 72, 97, 16, 36.8f);
    }

    /* Insert samples at "recent" timestamps relative to current_ts.
     * Raw retention is 4 hours (14400s).
     * Use current_ts = 50000, so cutoff = 50000 - 14400 = 35600.
     * Old samples (100-104) should be purged. */
    uint32_t recent_ts = 50000;
    for (int i = 0; i < 3; i++) {
        trend_db_insert_sample(recent_ts - 100 + (uint32_t)i, 80, 98, 18, 37.0f);
    }

    /* Purge with current_ts = 50000 */
    trend_db_purge_old(recent_ts);

    /* Query old range -- should be empty */
    clear_param_result();
    int count = trend_db_query_param(TREND_PARAM_HR, 100, 104, 480,
                                     &s_param_result);
    ASSERT_EQ_INT(count, 0);

    /* Query recent range -- should still have data */
    clear_param_result();
    count = trend_db_query_param(TREND_PARAM_HR,
                                 recent_ts - 200, recent_ts, 480,
                                 &s_param_result);
    ASSERT_GT_INT(count, 0);

    trend_db_close();
}

/* ── Test 11: Purge old NIBP and alarm data ──────────────── */

static void test_purge_nibp_alarm(void) {
    printf("  test_purge_nibp_alarm\n");

    trend_db_init(NULL);

    /* Insert old NIBP and alarm at ts=200 */
    trend_db_insert_nibp(200, 120, 80, 93);
    trend_db_insert_alarm(200, VM_ALARM_HIGH, "Old alarm");

    /* Insert recent ones at ts=500000 */
    trend_db_insert_nibp(500000, 125, 82, 96);
    trend_db_insert_alarm(500000, VM_ALARM_LOW, "Recent alarm");

    /* Purge with current_ts = 500000.
     * NIBP/alarm use aggregate retention (72h = 259200s).
     * Cutoff = 500000 - 259200 = 240800. Ts=200 < 240800 => purged. */
    trend_db_purge_old(500000);

    /* Old NIBP should be gone */
    clear_nibp_result();
    int count = trend_db_query_nibp(200, 200, &s_nibp_result);
    ASSERT_EQ_INT(count, 0);

    /* Recent NIBP should remain */
    clear_nibp_result();
    count = trend_db_query_nibp(500000, 500000, &s_nibp_result);
    ASSERT_EQ_INT(count, 1);

    /* Old alarm should be gone */
    clear_alarm_result();
    count = trend_db_query_alarms(200, 200, &s_alarm_result);
    ASSERT_EQ_INT(count, 0);

    /* Recent alarm should remain */
    clear_alarm_result();
    count = trend_db_query_alarms(500000, 500000, &s_alarm_result);
    ASSERT_EQ_INT(count, 1);

    trend_db_close();
}

/* ── Test 12: NULL result pointer returns 0 ──────────────── */

static void test_null_result_pointer(void) {
    printf("  test_null_result_pointer\n");

    trend_db_init(NULL);

    /* query_param with NULL result should return 0 */
    int count = trend_db_query_param(TREND_PARAM_HR, 0, 1000, 480, NULL);
    ASSERT_EQ_INT(count, 0);

    /* query_nibp with NULL result should return 0 */
    count = trend_db_query_nibp(0, 1000, NULL);
    ASSERT_EQ_INT(count, 0);

    /* query_alarms with NULL result should return 0 */
    count = trend_db_query_alarms(0, 1000, NULL);
    ASSERT_EQ_INT(count, 0);

    trend_db_close();
}

/* ── Test 13: Queries on empty database return 0 ─────────── */

static void test_empty_db_queries(void) {
    printf("  test_empty_db_queries\n");

    trend_db_init(NULL);

    clear_param_result();
    int count = trend_db_query_param(TREND_PARAM_HR, 0, 1000, 480,
                                     &s_param_result);
    ASSERT_EQ_INT(count, 0);
    ASSERT_EQ_INT(s_param_result.count, 0);

    clear_nibp_result();
    count = trend_db_query_nibp(0, 1000, &s_nibp_result);
    ASSERT_EQ_INT(count, 0);

    clear_alarm_result();
    count = trend_db_query_alarms(0, 1000, &s_alarm_result);
    ASSERT_EQ_INT(count, 0);

    trend_db_close();
}

/* ── Test 14: Query with narrow time range excludes data ─── */

static void test_query_time_range_filter(void) {
    printf("  test_query_time_range_filter\n");

    trend_db_init(NULL);

    trend_db_insert_sample(1000, 72, 97, 16, 36.8f);
    trend_db_insert_sample(2000, 80, 95, 18, 37.0f);
    trend_db_insert_sample(3000, 90, 93, 20, 37.5f);

    /* Query only the middle range -- should get only ts=2000 */
    clear_param_result();
    int count = trend_db_query_param(TREND_PARAM_HR, 1500, 2500, 480,
                                     &s_param_result);
    ASSERT_EQ_INT(count, 1);
    ASSERT_EQ_INT(s_param_result.value[0], 80);

    /* Query a range with no data */
    clear_param_result();
    count = trend_db_query_param(TREND_PARAM_HR, 9000, 9999, 480,
                                 &s_param_result);
    ASSERT_EQ_INT(count, 0);

    trend_db_close();
}

/* ── Test 15: NIBP query time range filtering ────────────── */

static void test_nibp_time_range_filter(void) {
    printf("  test_nibp_time_range_filter\n");

    trend_db_init(NULL);

    trend_db_insert_nibp(1000, 120, 80, 93);
    trend_db_insert_nibp(2000, 130, 85, 100);
    trend_db_insert_nibp(3000, 110, 70, 83);

    /* Query only ts 1500..2500 */
    clear_nibp_result();
    int count = trend_db_query_nibp(1500, 2500, &s_nibp_result);
    ASSERT_EQ_INT(count, 1);
    ASSERT_EQ_INT(s_nibp_result.sys[0], 130);

    trend_db_close();
}

/* ── Test 16: Query RR parameter ─────────────────────────── */

static void test_insert_query_raw_rr(void) {
    printf("  test_insert_query_raw_rr\n");

    trend_db_init(NULL);

    trend_db_insert_sample(4000, 72, 97, 14, 36.8f);
    trend_db_insert_sample(4001, 72, 97, 18, 36.8f);
    trend_db_insert_sample(4002, 72, 97, 22, 36.8f);

    clear_param_result();
    int count = trend_db_query_param(TREND_PARAM_RR, 4000, 4002, 480,
                                     &s_param_result);
    ASSERT_GT_INT(count, 0);

    trend_db_close();
}

/* ── Test 17: INSERT OR REPLACE behavior (duplicate ts) ──── */

static void test_duplicate_timestamp(void) {
    printf("  test_duplicate_timestamp\n");

    trend_db_init(NULL);

    /* Insert two samples at same timestamp -- second should replace */
    trend_db_insert_sample(7000, 72, 97, 16, 36.8f);
    trend_db_insert_sample(7000, 85, 95, 20, 37.5f);

    clear_param_result();
    int count = trend_db_query_param(TREND_PARAM_HR, 7000, 7000, 480,
                                     &s_param_result);
    ASSERT_EQ_INT(count, 1);
    /* The replaced value should be 85 */
    ASSERT_EQ_INT(s_param_result.value[0], 85);

    trend_db_close();
}

/* ── Test 18: Operations before init (no crash) ──────────── */

static void test_operations_without_init(void) {
    printf("  test_operations_without_init\n");

    /* Ensure no prior DB is open */
    trend_db_close();

    /* These should all be safe no-ops (no crash) */
    trend_db_insert_sample(1000, 72, 97, 16, 36.8f);
    trend_db_insert_nibp(1000, 120, 80, 93);
    trend_db_insert_alarm(1000, VM_ALARM_HIGH, "test");
    trend_db_aggregate_minute(1060);
    trend_db_purge_old(99999);

    /* Queries should return 0 */
    clear_param_result();
    int count = trend_db_query_param(TREND_PARAM_HR, 0, 9999, 480,
                                     &s_param_result);
    ASSERT_EQ_INT(count, 0);

    count = trend_db_query_nibp(0, 9999, &s_nibp_result);
    ASSERT_EQ_INT(count, 0);

    count = trend_db_query_alarms(0, 9999, &s_alarm_result);
    ASSERT_EQ_INT(count, 0);
}

/* ── Public entry point ──────────────────────────────────── */

void test_trend_db(void) {
    test_init_close();
    test_reinit();
    test_insert_query_raw_hr();
    test_insert_query_raw_spo2();
    test_insert_query_raw_temp();
    test_insert_query_raw_rr();
    test_insert_query_nibp();
    test_insert_query_alarms();
    test_aggregation();
    test_aggregation_empty();
    test_purge_raw();
    test_purge_nibp_alarm();
    test_null_result_pointer();
    test_empty_db_queries();
    test_query_time_range_filter();
    test_nibp_time_range_filter();
    test_duplicate_timestamp();
    test_operations_without_init();
}
