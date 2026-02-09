/**
 * @file test_alarm_db_integration.c
 * @brief Integration tests: alarm_engine + trend_db
 *
 * Verifies that alarm events triggered by alarm_engine are correctly
 * recorded in trend_db and can be queried back. Uses in-memory SQLite
 * databases for fast, isolated tests.
 */

#include "test_framework.h"
#include "alarm_engine.h"
#include "trend_db.h"
#include "vitals_provider.h"
#include <time.h>
#include <string.h>

/* ── Helper: create normal vitals data ─────────────────────── */

static vitals_data_t make_normal_vitals(void) {
    vitals_data_t d;
    memset(&d, 0, sizeof(d));
    d.hr   = 75;
    d.spo2 = 98;
    d.rr   = 16;
    d.temp  = 37.0f;
    d.nibp_sys = 120;
    d.nibp_dia = 80;
    d.nibp_map = 93;
    d.hr_quality   = 100;
    d.spo2_quality = 100;
    return d;
}

/* ── Helper: create critical vitals (HR very high) ─────────── */

static vitals_data_t make_critical_hr_vitals(void) {
    vitals_data_t d = make_normal_vitals();
    d.hr = 200;  /* Well above critical_high of 150 */
    return d;
}

/* ── Test: init both modules together ──────────────────────── */

static void test_init_both(void) {
    printf("  test_init_both\n");

    bool db_ok = trend_db_init(":memory:");
    ASSERT_TRUE(db_ok);

    alarm_engine_init();

    /* Verify alarm engine starts clean */
    const alarm_engine_state_t *state = alarm_engine_get_state();
    ASSERT_NOT_NULL(state);
    ASSERT_EQ_INT(state->highest_active, ALARM_SEV_NONE);
    ASSERT_EQ_INT(state->highest_any, ALARM_SEV_NONE);

    alarm_engine_deinit();
    trend_db_close();
}

/* ── Test: trigger alarm and write event to DB ─────────────── */

static void test_alarm_triggers_db_event(void) {
    printf("  test_alarm_triggers_db_event\n");

    bool db_ok = trend_db_init(":memory:");
    ASSERT_TRUE(db_ok);
    alarm_engine_init();

    uint32_t now = (uint32_t)time(NULL);

    /* Evaluate normal vitals -- no alarm */
    vitals_data_t normal = make_normal_vitals();
    alarm_engine_evaluate(&normal, now);

    const alarm_engine_state_t *state = alarm_engine_get_state();
    ASSERT_EQ_INT(state->highest_active, ALARM_SEV_NONE);

    /* Evaluate critical HR -- should trigger alarm */
    vitals_data_t critical = make_critical_hr_vitals();
    alarm_engine_evaluate(&critical, now + 1);

    state = alarm_engine_get_state();
    ASSERT_EQ_INT(state->highest_active, ALARM_SEV_HIGH);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_ACTIVE);

    /* Manually record the alarm event to trend_db (simulating what
       the application layer does when alarm_engine fires) */
    trend_db_insert_alarm(now + 1, VM_ALARM_HIGH,
                          state->params[ALARM_PARAM_HR].message);

    /* Query back alarm events from DB */
    trend_alarm_result_t alarms;
    int count = trend_db_query_alarms(now - 10, now + 100, &alarms);
    ASSERT_EQ_INT(count, 1);
    ASSERT_EQ_INT(alarms.severity[0], VM_ALARM_HIGH);
    ASSERT_GT_INT((int)strlen(alarms.message[0]), 0);

    alarm_engine_deinit();
    trend_db_close();
}

/* ── Test: query alarm events returns correct data ─────────── */

static void test_query_alarm_events(void) {
    printf("  test_query_alarm_events\n");

    trend_db_init(":memory:");
    alarm_engine_init();

    uint32_t now = (uint32_t)time(NULL);

    /* Insert several alarm events at different times */
    trend_db_insert_alarm(now,     VM_ALARM_HIGH,   "HR > 150 bpm");
    trend_db_insert_alarm(now + 1, VM_ALARM_MEDIUM, "SpO2 < 90%");
    trend_db_insert_alarm(now + 2, VM_ALARM_LOW,    "HR > 120 bpm");

    /* Query all */
    trend_alarm_result_t alarms;
    int count = trend_db_query_alarms(now - 10, now + 100, &alarms);
    ASSERT_EQ_INT(count, 3);

    /* Verify severity values are correct */
    ASSERT_EQ_INT(alarms.severity[0], VM_ALARM_HIGH);
    ASSERT_EQ_INT(alarms.severity[1], VM_ALARM_MEDIUM);
    ASSERT_EQ_INT(alarms.severity[2], VM_ALARM_LOW);

    /* Query a narrower time range */
    count = trend_db_query_alarms(now + 1, now + 2, &alarms);
    ASSERT_GE_INT(count, 1);

    alarm_engine_deinit();
    trend_db_close();
}

/* ── Test: alarm state persists across engine reinit ───────── */

static void test_alarm_state_persists(void) {
    printf("  test_alarm_state_persists\n");

    trend_db_init(":memory:");
    alarm_engine_init();

    uint32_t now = (uint32_t)time(NULL);

    /* Record an alarm event */
    trend_db_insert_alarm(now, VM_ALARM_HIGH, "HR critical alarm");

    /* Deinit and reinit alarm engine */
    alarm_engine_deinit();
    alarm_engine_init();

    /* The alarm event in the DB should still be queryable */
    trend_alarm_result_t alarms;
    int count = trend_db_query_alarms(now - 10, now + 100, &alarms);
    ASSERT_EQ_INT(count, 1);
    ASSERT_STR_EQ(alarms.message[0], "HR critical alarm");
    ASSERT_EQ_INT(alarms.severity[0], VM_ALARM_HIGH);

    /* The alarm engine state itself resets (stateless re-init) */
    const alarm_engine_state_t *state = alarm_engine_get_state();
    ASSERT_EQ_INT(state->highest_active, ALARM_SEV_NONE);

    alarm_engine_deinit();
    trend_db_close();
}

/* ── Test: alarm limits + trend storage workflow ──────────── */

static void test_alarm_limits_and_trends(void) {
    printf("  test_alarm_limits_and_trends\n");

    trend_db_init(":memory:");
    alarm_engine_init();

    uint32_t now = (uint32_t)time(NULL);

    /* Adjust HR alarm limits to lower threshold */
    alarm_limits_t hr_limits = {
        .critical_high = 130,
        .critical_low  = 35,
        .warning_high  = 110,
        .warning_low   = 45,
        .enabled       = true
    };
    alarm_engine_set_limits(ALARM_PARAM_HR, &hr_limits);

    /* Verify limits were set */
    const alarm_limits_t *lim = alarm_engine_get_limits(ALARM_PARAM_HR);
    ASSERT_NOT_NULL(lim);
    ASSERT_EQ_INT(lim->critical_high, 130);
    ASSERT_TRUE(lim->enabled);

    /* Insert vitals samples into trend_db alongside alarm evaluation */
    vitals_data_t d = make_normal_vitals();
    d.hr = 135;  /* Above the new critical_high of 130 */

    alarm_engine_evaluate(&d, now);
    trend_db_insert_sample(now, d.hr, d.spo2, d.rr, d.temp);

    /* Alarm should have triggered */
    const alarm_engine_state_t *state = alarm_engine_get_state();
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_ACTIVE);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].severity, ALARM_SEV_HIGH);

    /* Record alarm to DB */
    trend_db_insert_alarm(now, VM_ALARM_HIGH,
                          state->params[ALARM_PARAM_HR].message);

    /* Verify both the vitals sample and alarm event are in the DB */
    trend_query_result_t trend_result;
    int trend_count = trend_db_query_param(TREND_PARAM_HR, now - 10,
                                           now + 10, 100, &trend_result);
    ASSERT_GE_INT(trend_count, 1);
    ASSERT_EQ_INT(trend_result.value[0], 135);

    trend_alarm_result_t alarm_result;
    int alarm_count = trend_db_query_alarms(now - 10, now + 10, &alarm_result);
    ASSERT_EQ_INT(alarm_count, 1);

    alarm_engine_deinit();
    trend_db_close();
}

/* ── Test: multiple alarm parameters triggering simultaneously ── */

static void test_multiple_alarms_with_trends(void) {
    printf("  test_multiple_alarms_with_trends\n");

    trend_db_init(":memory:");
    alarm_engine_init();

    uint32_t now = (uint32_t)time(NULL);

    /* Vitals with multiple violations */
    vitals_data_t d = make_normal_vitals();
    d.hr   = 180;  /* Critical high */
    d.spo2 = 80;   /* Critical low */

    alarm_engine_evaluate(&d, now);
    trend_db_insert_sample(now, d.hr, d.spo2, d.rr, d.temp);

    const alarm_engine_state_t *state = alarm_engine_get_state();

    /* Both HR and SpO2 should be alarming */
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_ACTIVE);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_SPO2].state, ALARM_STATE_ACTIVE);

    /* Record both alarm events */
    trend_db_insert_alarm(now, VM_ALARM_HIGH,
                          state->params[ALARM_PARAM_HR].message);
    trend_db_insert_alarm(now + 1, VM_ALARM_HIGH,
                          state->params[ALARM_PARAM_SPO2].message);

    /* Query alarm events */
    trend_alarm_result_t alarms;
    int count = trend_db_query_alarms(now - 10, now + 100, &alarms);
    ASSERT_EQ_INT(count, 2);

    /* Acknowledge all and verify they transition */
    bool acked = alarm_engine_acknowledge_all();
    ASSERT_TRUE(acked);

    state = alarm_engine_get_state();
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_ACKNOWLEDGED);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_SPO2].state, ALARM_STATE_ACKNOWLEDGED);

    alarm_engine_deinit();
    trend_db_close();
}

/* ── Public entry point ──────────────────────────────────── */

void test_alarm_db_integration(void) {
    test_init_both();
    test_alarm_triggers_db_event();
    test_query_alarm_events();
    test_alarm_state_persists();
    test_alarm_limits_and_trends();
    test_multiple_alarms_with_trends();
}
