/**
 * @file test_alarm_engine.c
 * @brief Unit tests for alarm_engine module
 *
 * Tests alarm threshold evaluation, state machine transitions,
 * acknowledge/silence workflows, custom limits, and audio pause.
 */

#include "test_framework.h"
#include "alarm_engine.h"

/* Helper: create a vitals_data_t with normal values */
static vitals_data_t make_normal_vitals(void) {
    vitals_data_t v;
    memset(&v, 0, sizeof(v));
    v.hr       = 72;
    v.spo2     = 97;
    v.rr       = 16;
    v.temp     = 36.8f;
    v.nibp_sys = 120;
    v.nibp_dia = 80;
    v.nibp_map = 93;
    v.timestamp_ms = 1000;
    return v;
}

/* ── Test: initialization and default state ──────────────── */

static void test_init_defaults(void) {
    printf("  test_init_defaults\n");

    alarm_engine_init();
    const alarm_engine_state_t *state = alarm_engine_get_state();

    ASSERT_NOT_NULL(state);
    ASSERT_EQ_INT(state->highest_active, ALARM_SEV_NONE);
    ASSERT_EQ_INT(state->highest_any, ALARM_SEV_NONE);
    ASSERT_FALSE(state->audio_paused);

    /* All parameters should be INACTIVE */
    for (int i = 0; i < ALARM_PARAM_COUNT; i++) {
        ASSERT_EQ_INT(state->params[i].state, ALARM_STATE_INACTIVE);
        ASSERT_EQ_INT(state->params[i].severity, ALARM_SEV_NONE);
    }

    alarm_engine_deinit();
}

/* ── Test: default threshold values ──────────────────────── */

static void test_default_limits(void) {
    printf("  test_default_limits\n");

    alarm_engine_init();

    /* HR defaults */
    const alarm_limits_t *hr = alarm_engine_get_limits(ALARM_PARAM_HR);
    ASSERT_NOT_NULL(hr);
    ASSERT_TRUE(hr->enabled);
    ASSERT_EQ_INT(hr->critical_high, 150);
    ASSERT_EQ_INT(hr->critical_low, 40);
    ASSERT_EQ_INT(hr->warning_high, 120);
    ASSERT_EQ_INT(hr->warning_low, 50);

    /* SpO2 defaults */
    const alarm_limits_t *spo2 = alarm_engine_get_limits(ALARM_PARAM_SPO2);
    ASSERT_NOT_NULL(spo2);
    ASSERT_TRUE(spo2->enabled);
    ASSERT_EQ_INT(spo2->critical_low, 85);
    ASSERT_EQ_INT(spo2->warning_low, 90);

    /* RR defaults */
    const alarm_limits_t *rr = alarm_engine_get_limits(ALARM_PARAM_RR);
    ASSERT_NOT_NULL(rr);
    ASSERT_EQ_INT(rr->critical_high, 30);
    ASSERT_EQ_INT(rr->critical_low, 8);

    /* Temp defaults (x10 integer) */
    const alarm_limits_t *temp = alarm_engine_get_limits(ALARM_PARAM_TEMP);
    ASSERT_NOT_NULL(temp);
    ASSERT_EQ_INT(temp->critical_high, 390);
    ASSERT_EQ_INT(temp->critical_low, 350);
    ASSERT_EQ_INT(temp->warning_high, 380);
    ASSERT_EQ_INT(temp->warning_low, 360);

    /* NIBP Sys defaults */
    const alarm_limits_t *sys = alarm_engine_get_limits(ALARM_PARAM_NIBP_SYS);
    ASSERT_NOT_NULL(sys);
    ASSERT_EQ_INT(sys->critical_high, 180);
    ASSERT_EQ_INT(sys->critical_low, 80);

    /* NIBP Dia defaults */
    const alarm_limits_t *dia = alarm_engine_get_limits(ALARM_PARAM_NIBP_DIA);
    ASSERT_NOT_NULL(dia);
    ASSERT_EQ_INT(dia->critical_high, 120);
    ASSERT_EQ_INT(dia->critical_low, 40);

    /* Out-of-range param returns NULL */
    ASSERT_NULL(alarm_engine_get_limits(ALARM_PARAM_COUNT));

    alarm_engine_deinit();
}

/* ── Test: normal vitals produce no alarms ───────────────── */

static void test_normal_vitals_no_alarm(void) {
    printf("  test_normal_vitals_no_alarm\n");

    alarm_engine_init();
    const alarm_engine_state_t *state = alarm_engine_get_state();

    vitals_data_t normal = make_normal_vitals();
    alarm_engine_evaluate(&normal, 1);

    ASSERT_EQ_INT(state->highest_active, ALARM_SEV_NONE);
    ASSERT_EQ_INT(state->highest_any, ALARM_SEV_NONE);

    for (int i = 0; i < ALARM_PARAM_COUNT; i++) {
        ASSERT_EQ_INT(state->params[i].state, ALARM_STATE_INACTIVE);
    }

    alarm_engine_deinit();
}

/* ── Test: high HR triggers critical alarm ───────────────── */

static void test_high_hr_critical(void) {
    printf("  test_high_hr_critical\n");

    alarm_engine_init();
    const alarm_engine_state_t *state = alarm_engine_get_state();

    vitals_data_t v = make_normal_vitals();
    v.hr = 160;  /* Above critical_high=150 */
    alarm_engine_evaluate(&v, 10);

    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_ACTIVE);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].severity, ALARM_SEV_HIGH);
    ASSERT_EQ_INT(state->highest_active, ALARM_SEV_HIGH);
    ASSERT_NOT_NULL(state->highest_message);

    alarm_engine_deinit();
}

/* ── Test: warning-level HR alarm ────────────────────────── */

static void test_warning_hr(void) {
    printf("  test_warning_hr\n");

    alarm_engine_init();
    const alarm_engine_state_t *state = alarm_engine_get_state();

    vitals_data_t v = make_normal_vitals();
    v.hr = 125;  /* Above warning_high=120 but below critical_high=150 */
    alarm_engine_evaluate(&v, 10);

    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_ACTIVE);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].severity, ALARM_SEV_MEDIUM);

    alarm_engine_deinit();
}

/* ── Test: low HR triggers critical alarm ────────────────── */

static void test_low_hr_critical(void) {
    printf("  test_low_hr_critical\n");

    alarm_engine_init();
    const alarm_engine_state_t *state = alarm_engine_get_state();

    vitals_data_t v = make_normal_vitals();
    v.hr = 35;  /* Below critical_low=40 */
    alarm_engine_evaluate(&v, 10);

    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_ACTIVE);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].severity, ALARM_SEV_HIGH);

    alarm_engine_deinit();
}

/* ── Test: low SpO2 critical ─────────────────────────────── */

static void test_low_spo2_critical(void) {
    printf("  test_low_spo2_critical\n");

    alarm_engine_init();
    const alarm_engine_state_t *state = alarm_engine_get_state();

    vitals_data_t v = make_normal_vitals();
    v.spo2 = 82;  /* Below critical_low=85 */
    alarm_engine_evaluate(&v, 10);

    ASSERT_EQ_INT(state->params[ALARM_PARAM_SPO2].state, ALARM_STATE_ACTIVE);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_SPO2].severity, ALARM_SEV_HIGH);

    alarm_engine_deinit();
}

/* ── Test: low SpO2 warning ──────────────────────────────── */

static void test_low_spo2_warning(void) {
    printf("  test_low_spo2_warning\n");

    alarm_engine_init();
    const alarm_engine_state_t *state = alarm_engine_get_state();

    vitals_data_t v = make_normal_vitals();
    v.spo2 = 88;  /* Below warning_low=90 but above critical_low=85 */
    alarm_engine_evaluate(&v, 10);

    ASSERT_EQ_INT(state->params[ALARM_PARAM_SPO2].state, ALARM_STATE_ACTIVE);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_SPO2].severity, ALARM_SEV_MEDIUM);

    alarm_engine_deinit();
}

/* ── Test: acknowledge alarm ─────────────────────────────── */

static void test_acknowledge(void) {
    printf("  test_acknowledge\n");

    alarm_engine_init();
    const alarm_engine_state_t *state = alarm_engine_get_state();

    /* Trigger HR alarm */
    vitals_data_t v = make_normal_vitals();
    v.hr = 160;
    alarm_engine_evaluate(&v, 10);

    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_ACTIVE);

    /* Acknowledge */
    bool result = alarm_engine_acknowledge(ALARM_PARAM_HR);
    ASSERT_TRUE(result);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_ACKNOWLEDGED);

    /* Acknowledging again should fail (already acknowledged) */
    result = alarm_engine_acknowledge(ALARM_PARAM_HR);
    ASSERT_FALSE(result);

    /* Acknowledging inactive param should fail */
    result = alarm_engine_acknowledge(ALARM_PARAM_SPO2);
    ASSERT_FALSE(result);

    /* Out-of-range param should fail */
    result = alarm_engine_acknowledge(ALARM_PARAM_COUNT);
    ASSERT_FALSE(result);

    alarm_engine_deinit();
}

/* ── Test: silence alarm ─────────────────────────────────── */

static void test_silence(void) {
    printf("  test_silence\n");

    alarm_engine_init();
    const alarm_engine_state_t *state = alarm_engine_get_state();

    /* Trigger HR alarm */
    vitals_data_t v = make_normal_vitals();
    v.hr = 160;
    alarm_engine_evaluate(&v, 10);

    /* Silence for 120 seconds */
    bool result = alarm_engine_silence(ALARM_PARAM_HR, 120);
    ASSERT_TRUE(result);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_SILENCED);

    /* While silenced, alarm stays silenced */
    alarm_engine_evaluate(&v, 50);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_SILENCED);

    /* After silence expires, alarm reactivates if condition persists */
    alarm_engine_evaluate(&v, 200);  /* 200 > 10 + 120 = 130 */
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_ACTIVE);

    /* Silencing an inactive param should fail */
    result = alarm_engine_silence(ALARM_PARAM_SPO2, 120);
    ASSERT_FALSE(result);

    alarm_engine_deinit();
}

/* ── Test: return to normal clears alarm ─────────────────── */

static void test_return_to_normal(void) {
    printf("  test_return_to_normal\n");

    alarm_engine_init();
    const alarm_engine_state_t *state = alarm_engine_get_state();

    /* Trigger alarm */
    vitals_data_t v = make_normal_vitals();
    v.hr = 160;
    alarm_engine_evaluate(&v, 10);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_ACTIVE);

    /* Return to normal */
    v.hr = 72;
    alarm_engine_evaluate(&v, 20);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_INACTIVE);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].severity, ALARM_SEV_NONE);
    ASSERT_EQ_INT(state->highest_active, ALARM_SEV_NONE);

    alarm_engine_deinit();
}

/* ── Test: acknowledged alarm clears on return to normal ─── */

static void test_acknowledged_clears_on_normal(void) {
    printf("  test_acknowledged_clears_on_normal\n");

    alarm_engine_init();
    const alarm_engine_state_t *state = alarm_engine_get_state();

    vitals_data_t v = make_normal_vitals();
    v.hr = 160;
    alarm_engine_evaluate(&v, 10);
    alarm_engine_acknowledge(ALARM_PARAM_HR);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_ACKNOWLEDGED);

    /* Return to normal */
    v.hr = 72;
    alarm_engine_evaluate(&v, 20);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_INACTIVE);

    alarm_engine_deinit();
}

/* ── Test: silenced alarm clears on return to normal ─────── */

static void test_silenced_clears_on_normal(void) {
    printf("  test_silenced_clears_on_normal\n");

    alarm_engine_init();
    const alarm_engine_state_t *state = alarm_engine_get_state();

    vitals_data_t v = make_normal_vitals();
    v.hr = 160;
    alarm_engine_evaluate(&v, 10);
    alarm_engine_silence(ALARM_PARAM_HR, 120);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_SILENCED);

    /* Return to normal while silenced */
    v.hr = 72;
    alarm_engine_evaluate(&v, 50);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_INACTIVE);

    alarm_engine_deinit();
}

/* ── Test: set custom limits ─────────────────────────────── */

static void test_custom_limits(void) {
    printf("  test_custom_limits\n");

    alarm_engine_init();
    const alarm_engine_state_t *state = alarm_engine_get_state();

    alarm_limits_t custom = {
        .critical_high = 140,
        .critical_low  = 45,
        .warning_high  = 110,
        .warning_low   = 55,
        .enabled       = true,
    };
    alarm_engine_set_limits(ALARM_PARAM_HR, &custom);

    const alarm_limits_t *lim = alarm_engine_get_limits(ALARM_PARAM_HR);
    ASSERT_EQ_INT(lim->critical_high, 140);
    ASSERT_EQ_INT(lim->critical_low, 45);
    ASSERT_EQ_INT(lim->warning_high, 110);
    ASSERT_EQ_INT(lim->warning_low, 55);

    /* Test new limits take effect: HR=115 is warning under custom, but normal under default */
    vitals_data_t v = make_normal_vitals();
    v.hr = 115;
    alarm_engine_evaluate(&v, 10);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_ACTIVE);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].severity, ALARM_SEV_MEDIUM);

    alarm_engine_deinit();
}

/* ── Test: reset defaults ────────────────────────────────── */

static void test_reset_defaults(void) {
    printf("  test_reset_defaults\n");

    alarm_engine_init();

    /* Change a limit */
    alarm_limits_t custom = { 140, 45, 110, 55, true };
    alarm_engine_set_limits(ALARM_PARAM_HR, &custom);
    ASSERT_EQ_INT(alarm_engine_get_limits(ALARM_PARAM_HR)->critical_high, 140);

    /* Reset to defaults */
    alarm_engine_reset_defaults();
    ASSERT_EQ_INT(alarm_engine_get_limits(ALARM_PARAM_HR)->critical_high, 150);
    ASSERT_EQ_INT(alarm_engine_get_limits(ALARM_PARAM_HR)->critical_low, 40);

    alarm_engine_deinit();
}

/* ── Test: acknowledge_all ───────────────────────────────── */

static void test_acknowledge_all(void) {
    printf("  test_acknowledge_all\n");

    alarm_engine_init();
    const alarm_engine_state_t *state = alarm_engine_get_state();

    /* Trigger multiple alarms */
    vitals_data_t v = make_normal_vitals();
    v.hr   = 160;
    v.spo2 = 82;
    alarm_engine_evaluate(&v, 10);

    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_ACTIVE);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_SPO2].state, ALARM_STATE_ACTIVE);

    /* Acknowledge all */
    bool result = alarm_engine_acknowledge_all();
    ASSERT_TRUE(result);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_ACKNOWLEDGED);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_SPO2].state, ALARM_STATE_ACKNOWLEDGED);

    /* highest_active should drop to NONE (acknowledged != active) */
    ASSERT_EQ_INT(state->highest_active, ALARM_SEV_NONE);
    /* highest_any should still reflect the alarm severity */
    ASSERT_EQ_INT(state->highest_any, ALARM_SEV_HIGH);

    alarm_engine_deinit();
}

/* ── Test: silence_all ───────────────────────────────────── */

static void test_silence_all(void) {
    printf("  test_silence_all\n");

    alarm_engine_init();
    const alarm_engine_state_t *state = alarm_engine_get_state();

    vitals_data_t v = make_normal_vitals();
    v.hr   = 160;
    v.spo2 = 82;
    alarm_engine_evaluate(&v, 10);

    bool result = alarm_engine_silence_all(120);
    ASSERT_TRUE(result);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_SILENCED);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_SPO2].state, ALARM_STATE_SILENCED);

    alarm_engine_deinit();
}

/* ── Test: audio pause ───────────────────────────────────── */

static void test_audio_pause(void) {
    printf("  test_audio_pause\n");

    alarm_engine_init();
    const alarm_engine_state_t *state = alarm_engine_get_state();

    ASSERT_FALSE(alarm_engine_is_audio_paused());

    /* Pause audio */
    alarm_engine_pause_audio(60);
    ASSERT_TRUE(alarm_engine_is_audio_paused());
    ASSERT_TRUE(state->audio_paused);

    /* Evaluate at a time before pause expires */
    vitals_data_t v = make_normal_vitals();
    alarm_engine_evaluate(&v, 30);
    ASSERT_TRUE(alarm_engine_is_audio_paused());

    /* Evaluate at time past pause expiry: pause should clear */
    alarm_engine_evaluate(&v, 70);
    ASSERT_FALSE(alarm_engine_is_audio_paused());

    alarm_engine_deinit();
}

/* ── Test: disabled parameter produces no alarm ──────────── */

static void test_disabled_param(void) {
    printf("  test_disabled_param\n");

    alarm_engine_init();
    const alarm_engine_state_t *state = alarm_engine_get_state();

    /* Disable HR alarms */
    alarm_limits_t disabled = { 150, 40, 120, 50, false };
    alarm_engine_set_limits(ALARM_PARAM_HR, &disabled);

    vitals_data_t v = make_normal_vitals();
    v.hr = 200;  /* Far above threshold but disabled */
    alarm_engine_evaluate(&v, 10);

    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_INACTIVE);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].severity, ALARM_SEV_NONE);

    alarm_engine_deinit();
}

/* ── Test: zero value (invalid signal) is skipped ────────── */

static void test_zero_value_skipped(void) {
    printf("  test_zero_value_skipped\n");

    alarm_engine_init();
    const alarm_engine_state_t *state = alarm_engine_get_state();

    vitals_data_t v = make_normal_vitals();
    v.hr = 0;  /* No signal */
    alarm_engine_evaluate(&v, 10);

    /* HR=0 should not trigger alarm even though 0 < critical_low */
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_INACTIVE);

    alarm_engine_deinit();
}

/* ── Test: severity escalation re-alerts ─────────────────── */

static void test_severity_escalation(void) {
    printf("  test_severity_escalation\n");

    alarm_engine_init();
    const alarm_engine_state_t *state = alarm_engine_get_state();

    /* Start with warning-level HR */
    vitals_data_t v = make_normal_vitals();
    v.hr = 125;  /* warning */
    alarm_engine_evaluate(&v, 10);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].severity, ALARM_SEV_MEDIUM);

    /* Acknowledge the warning */
    alarm_engine_acknowledge(ALARM_PARAM_HR);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_ACKNOWLEDGED);

    /* Escalate to critical */
    v.hr = 160;
    alarm_engine_evaluate(&v, 20);

    /* Should re-alert (go back to ACTIVE from ACKNOWLEDGED) */
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_ACTIVE);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].severity, ALARM_SEV_HIGH);

    alarm_engine_deinit();
}

/* ── Test: multiple simultaneous alarms ──────────────────── */

static void test_multiple_alarms(void) {
    printf("  test_multiple_alarms\n");

    alarm_engine_init();
    const alarm_engine_state_t *state = alarm_engine_get_state();

    vitals_data_t v = make_normal_vitals();
    v.hr   = 160;   /* critical */
    v.spo2 = 88;    /* warning */
    v.rr   = 35;    /* critical (>30) */
    alarm_engine_evaluate(&v, 10);

    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_ACTIVE);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].severity, ALARM_SEV_HIGH);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_SPO2].state, ALARM_STATE_ACTIVE);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_SPO2].severity, ALARM_SEV_MEDIUM);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_RR].state, ALARM_STATE_ACTIVE);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_RR].severity, ALARM_SEV_HIGH);

    /* Highest should be HIGH */
    ASSERT_EQ_INT(state->highest_active, ALARM_SEV_HIGH);
    ASSERT_EQ_INT(state->highest_any, ALARM_SEV_HIGH);

    alarm_engine_deinit();
}

/* ── Test: temperature alarm (float->x10 conversion) ─────── */

static void test_temperature_alarm(void) {
    printf("  test_temperature_alarm\n");

    alarm_engine_init();
    const alarm_engine_state_t *state = alarm_engine_get_state();

    /* High temp warning: 38.5 C -> x10 = 385, which is > warning_high=380 */
    vitals_data_t v = make_normal_vitals();
    v.temp = 38.5f;
    alarm_engine_evaluate(&v, 10);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_TEMP].state, ALARM_STATE_ACTIVE);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_TEMP].severity, ALARM_SEV_MEDIUM);

    alarm_engine_deinit();

    /* High temp critical: 39.5 C -> x10 = 395, which is > critical_high=390 */
    alarm_engine_init();
    v = make_normal_vitals();
    v.temp = 39.5f;
    alarm_engine_evaluate(&v, 10);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_TEMP].state, ALARM_STATE_ACTIVE);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_TEMP].severity, ALARM_SEV_HIGH);

    alarm_engine_deinit();
}

/* ── Test: NIBP alarms ───────────────────────────────────── */

static void test_nibp_alarms(void) {
    printf("  test_nibp_alarms\n");

    alarm_engine_init();
    const alarm_engine_state_t *state = alarm_engine_get_state();

    vitals_data_t v = make_normal_vitals();
    v.nibp_sys = 190;  /* > critical_high=180 */
    v.nibp_dia = 130;  /* > critical_high=120 */
    alarm_engine_evaluate(&v, 10);

    ASSERT_EQ_INT(state->params[ALARM_PARAM_NIBP_SYS].state, ALARM_STATE_ACTIVE);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_NIBP_SYS].severity, ALARM_SEV_HIGH);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_NIBP_DIA].state, ALARM_STATE_ACTIVE);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_NIBP_DIA].severity, ALARM_SEV_HIGH);

    alarm_engine_deinit();
}

/* ── Test: NULL data is handled safely ───────────────────── */

static void test_null_data_safe(void) {
    printf("  test_null_data_safe\n");

    alarm_engine_init();
    const alarm_engine_state_t *state = alarm_engine_get_state();

    /* Passing NULL should not crash */
    alarm_engine_evaluate(NULL, 10);
    ASSERT_EQ_INT(state->highest_active, ALARM_SEV_NONE);

    alarm_engine_deinit();
}

/* ── Public entry point ──────────────────────────────────── */

void test_alarm_engine(void) {
    test_init_defaults();
    test_default_limits();
    test_normal_vitals_no_alarm();
    test_high_hr_critical();
    test_warning_hr();
    test_low_hr_critical();
    test_low_spo2_critical();
    test_low_spo2_warning();
    test_acknowledge();
    test_silence();
    test_return_to_normal();
    test_acknowledged_clears_on_normal();
    test_silenced_clears_on_normal();
    test_custom_limits();
    test_reset_defaults();
    test_acknowledge_all();
    test_silence_all();
    test_audio_pause();
    test_disabled_param();
    test_zero_value_skipped();
    test_severity_escalation();
    test_multiple_alarms();
    test_temperature_alarm();
    test_nibp_alarms();
    test_null_data_safe();
}
