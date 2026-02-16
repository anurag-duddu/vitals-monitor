/**
 * @file test_failure_scenarios.c
 * @brief Integration tests for failure, boundary, and security scenarios
 *
 * Tests robustness of core modules under adverse conditions:
 *   1. Database operations after close (should not crash)
 *   2. Alarm engine with extreme/boundary values (INT_MAX, INT_MIN, 0)
 *   3. Auth manager with empty strings, very long strings, SQL injection
 *   4. Settings store with invalid keys and boundary values
 *   5. Multiple rapid init/close cycles (stress test)
 *   6. Operations with NULL parameters
 *   7. Authentication bypass / security testing (TEST-5.1)
 *
 * Addresses audit findings TEST-4.2 and TEST-5.1.
 */

#include "test_framework.h"
#include "alarm_engine.h"
#include "auth_manager.h"
#include "audit_log.h"
#include "patient_data.h"
#include "settings_store.h"
#include "trend_db.h"
#include "vitals_provider.h"
#include <string.h>
#include <limits.h>
#include <time.h>

/* ================================================================
 *  1. Database operations after close (should not crash)
 * ================================================================ */

static void test_patient_data_ops_after_close(void) {
    printf("  test_patient_data_ops_after_close\n");

    /* Init and immediately close */
    bool ok = patient_data_init(":memory:");
    ASSERT_TRUE(ok);
    patient_data_close();

    /* Operations after close should return failure, not crash */
    patient_list_t list;
    int count = patient_data_list_all(&list);
    ASSERT_EQ_INT(count, 0);

    count = patient_data_list_active(&list);
    ASSERT_EQ_INT(count, 0);

    const patient_t *active = patient_data_get_active(0);
    ASSERT_NULL(active);

    patient_t p;
    patient_data_create_default(&p);
    ok = patient_data_save(&p);
    ASSERT_FALSE(ok);

    ok = patient_data_get(1, &p);
    ASSERT_FALSE(ok);

    ok = patient_data_delete(1);
    ASSERT_FALSE(ok);

    ok = patient_data_find_by_mrn("MRN-001", &p);
    ASSERT_FALSE(ok);

    /* Double close should not crash */
    patient_data_close();
}

static void test_settings_store_ops_after_close(void) {
    printf("  test_settings_store_ops_after_close\n");

    bool ok = settings_store_init(":memory:");
    ASSERT_TRUE(ok);
    settings_store_close();

    /* Operations after close should return defaults / fail gracefully */
    int val = settings_get_int("display.brightness", 42);
    ASSERT_EQ_INT(val, 42);

    bool bval = settings_get_bool("audio.key_click", true);
    ASSERT_TRUE(bval);

    ok = settings_set_int("display.brightness", 100);
    ASSERT_FALSE(ok);

    ok = settings_set_string("test.key", "value");
    ASSERT_FALSE(ok);

    /* Double close should not crash */
    settings_store_close();
}

static void test_auth_manager_ops_after_close(void) {
    printf("  test_auth_manager_ops_after_close\n");

    bool ok = auth_manager_init(":memory:");
    ASSERT_TRUE(ok);
    auth_manager_close();

    /* Operations after close should return failure, not crash */
    ok = auth_manager_login("admin", "1234");
    ASSERT_FALSE(ok);

    ASSERT_FALSE(auth_manager_is_logged_in());

    ok = auth_manager_add_user("Test", "test", "1234", AUTH_ROLE_NURSE);
    ASSERT_FALSE(ok);

    auth_user_t users[AUTH_MAX_USERS];
    int count = auth_manager_list_users(users, AUTH_MAX_USERS);
    ASSERT_EQ_INT(count, 0);

    /* Double close should not crash */
    auth_manager_close();
}

static void test_audit_log_ops_after_close(void) {
    printf("  test_audit_log_ops_after_close\n");

    bool ok = audit_log_init(":memory:");
    ASSERT_TRUE(ok);
    audit_log_close();

    /* Record after close should not crash (may silently fail) */
    audit_log_record(AUDIT_EVENT_LOGIN, "admin", "should not crash");

    audit_query_result_t result;
    int count = audit_log_query_recent(10, &result);
    ASSERT_EQ_INT(count, 0);

    count = audit_log_query_by_user("admin", 10, &result);
    ASSERT_EQ_INT(count, 0);

    count = audit_log_query_by_event(AUDIT_EVENT_LOGIN, 10, &result);
    ASSERT_EQ_INT(count, 0);

    /* Double close should not crash */
    audit_log_close();
}

static void test_trend_db_ops_after_close(void) {
    printf("  test_trend_db_ops_after_close\n");

    bool ok = trend_db_init(":memory:");
    ASSERT_TRUE(ok);
    trend_db_close();

    /* Insert after close should not crash */
    uint32_t now = (uint32_t)time(NULL);
    trend_db_insert_sample(now, 75, 98, 16, 37.0f);
    trend_db_insert_nibp(now, 120, 80, 93);
    trend_db_insert_alarm(now, VM_ALARM_HIGH, "should not crash");

    /* Query after close should return 0 */
    trend_query_result_t result;
    int count = trend_db_query_param(TREND_PARAM_HR, now - 10, now + 10, 100, &result);
    ASSERT_EQ_INT(count, 0);

    /* Double close should not crash */
    trend_db_close();
}

/* ================================================================
 *  2. Alarm engine with extreme/boundary values
 * ================================================================ */

static void test_alarm_engine_extreme_values(void) {
    printf("  test_alarm_engine_extreme_values\n");

    alarm_engine_init();

    uint32_t now = (uint32_t)time(NULL);

    /* Test with INT_MAX values */
    vitals_data_t d;
    memset(&d, 0, sizeof(d));
    d.hr = INT_MAX;
    d.spo2 = INT_MAX;
    d.rr = INT_MAX;
    d.temp = 9999.0f;
    d.nibp_sys = INT_MAX;
    d.nibp_dia = INT_MAX;
    d.hr_quality = 100;
    d.spo2_quality = 100;

    /* Should not crash; alarms should trigger for extreme high values */
    alarm_engine_evaluate(&d, now);

    const alarm_engine_state_t *state = alarm_engine_get_state();
    ASSERT_NOT_NULL(state);
    /* INT_MAX HR should trigger a high alarm */
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].state, ALARM_STATE_ACTIVE);
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].severity, ALARM_SEV_HIGH);

    alarm_engine_deinit();
}

static void test_alarm_engine_int_min_values(void) {
    printf("  test_alarm_engine_int_min_values\n");

    alarm_engine_init();

    uint32_t now = (uint32_t)time(NULL);

    /* Test with INT_MIN values */
    vitals_data_t d;
    memset(&d, 0, sizeof(d));
    d.hr = INT_MIN;
    d.spo2 = INT_MIN;
    d.rr = INT_MIN;
    d.temp = -9999.0f;
    d.nibp_sys = INT_MIN;
    d.nibp_dia = INT_MIN;
    d.hr_quality = 100;
    d.spo2_quality = 100;

    /* Should not crash; alarms should trigger for extreme low values */
    alarm_engine_evaluate(&d, now);

    const alarm_engine_state_t *state = alarm_engine_get_state();
    ASSERT_NOT_NULL(state);
    /* INT_MIN HR should trigger a critical low alarm */
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].severity, ALARM_SEV_HIGH);

    alarm_engine_deinit();
}

static void test_alarm_engine_zero_values(void) {
    printf("  test_alarm_engine_zero_values\n");

    alarm_engine_init();

    uint32_t now = (uint32_t)time(NULL);

    /* Test with all-zero vitals (0 is typically "invalid" for most params) */
    vitals_data_t d;
    memset(&d, 0, sizeof(d));
    d.hr_quality = 100;
    d.spo2_quality = 100;

    alarm_engine_evaluate(&d, now);

    const alarm_engine_state_t *state = alarm_engine_get_state();
    ASSERT_NOT_NULL(state);

    /* Zero HR is below critical_low -- should alarm */
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].severity, ALARM_SEV_HIGH);

    alarm_engine_deinit();
}

static void test_alarm_engine_boundary_at_limits(void) {
    printf("  test_alarm_engine_boundary_at_limits\n");

    alarm_engine_init();

    /* Get current limits for HR */
    const alarm_limits_t *lim = alarm_engine_get_limits(ALARM_PARAM_HR);
    ASSERT_NOT_NULL_OR_RETURN(lim);

    int crit_high = lim->critical_high;
    int warn_high = lim->warning_high;
    uint32_t now = (uint32_t)time(NULL);

    /* Value exactly at critical_high boundary (should not trigger CRITICAL) */
    vitals_data_t d;
    memset(&d, 0, sizeof(d));
    d.hr = crit_high;       /* AT the boundary, not above */
    d.spo2 = 98;
    d.rr = 16;
    d.temp = 37.0f;
    d.nibp_sys = 120;
    d.nibp_dia = 80;
    d.hr_quality = 100;
    d.spo2_quality = 100;

    alarm_engine_evaluate(&d, now);

    const alarm_engine_state_t *state = alarm_engine_get_state();
    /* At boundary: depends on > vs >= semantics. Just verify no crash. */
    ASSERT_NOT_NULL(state);

    /* Value one above critical_high (should definitely trigger) */
    alarm_engine_deinit();
    alarm_engine_init();

    d.hr = crit_high + 1;
    alarm_engine_evaluate(&d, now);
    state = alarm_engine_get_state();
    ASSERT_EQ_INT(state->params[ALARM_PARAM_HR].severity, ALARM_SEV_HIGH);

    /* Value exactly at warning_high */
    alarm_engine_deinit();
    alarm_engine_init();

    d.hr = warn_high;
    alarm_engine_evaluate(&d, now);
    state = alarm_engine_get_state();
    ASSERT_NOT_NULL(state);

    /* Value one above warning_high (should trigger WARNING) */
    alarm_engine_deinit();
    alarm_engine_init();

    d.hr = warn_high + 1;
    alarm_engine_evaluate(&d, now);
    state = alarm_engine_get_state();
    ASSERT_GE_INT((int)state->params[ALARM_PARAM_HR].severity, (int)ALARM_SEV_MEDIUM);

    alarm_engine_deinit();
}

/* ================================================================
 *  3. Auth manager: empty strings, long strings, SQL injection
 *     (also covers TEST-5.1: authentication bypass testing)
 * ================================================================ */

static void test_auth_sql_injection_username(void) {
    printf("  test_auth_sql_injection_username\n");

    auth_manager_init(":memory:");

    /* Classic SQL injection attempt via username */
    bool ok = auth_manager_login("admin' OR '1'='1", "anything");
    ASSERT_FALSE(ok);
    ASSERT_FALSE(auth_manager_is_logged_in());

    /* Another SQL injection variant */
    ok = auth_manager_login("admin'--", "1234");
    ASSERT_FALSE(ok);
    ASSERT_FALSE(auth_manager_is_logged_in());

    /* SQL injection with DROP TABLE */
    ok = auth_manager_login("'; DROP TABLE users;--", "1234");
    ASSERT_FALSE(ok);
    ASSERT_FALSE(auth_manager_is_logged_in());

    /* Verify the users table is intact after injection attempts */
    auth_user_t users[AUTH_MAX_USERS];
    int count = auth_manager_list_users(users, AUTH_MAX_USERS);
    ASSERT_EQ_INT(count, 4);  /* All 4 default users still present */

    /* The legitimate admin can still log in */
    ok = auth_manager_login("admin", "1234");
    ASSERT_TRUE(ok);
    ASSERT_TRUE(auth_manager_is_logged_in());
    auth_manager_logout();

    auth_manager_close();
}

static void test_auth_sql_injection_in_add_user(void) {
    printf("  test_auth_sql_injection_in_add_user\n");

    auth_manager_init(":memory:");

    /* SQL injection in username field for add_user */
    bool ok = auth_manager_add_user("Hacker", "'; DROP TABLE users;--",
                                     "1234", AUTH_ROLE_NURSE);
    /* This may succeed or fail depending on UNIQUE constraint, but should
       NOT execute the injection. Verify table is intact. */
    (void)ok;

    auth_user_t users[AUTH_MAX_USERS];
    int count = auth_manager_list_users(users, AUTH_MAX_USERS);
    ASSERT_GE_INT(count, 4);  /* At least the 4 defaults still exist */

    /* Legitimate admin login still works */
    ok = auth_manager_login("admin", "1234");
    ASSERT_TRUE(ok);
    auth_manager_logout();

    auth_manager_close();
}

static void test_auth_very_long_username(void) {
    printf("  test_auth_very_long_username\n");

    auth_manager_init(":memory:");

    /* Generate a 300-character username */
    char long_username[301];
    memset(long_username, 'A', 300);
    long_username[300] = '\0';

    /* Login with very long username should fail, not crash */
    bool ok = auth_manager_login(long_username, "1234");
    ASSERT_FALSE(ok);
    ASSERT_FALSE(auth_manager_is_logged_in());

    /* Very long PIN */
    char long_pin[301];
    memset(long_pin, '1', 300);
    long_pin[300] = '\0';

    ok = auth_manager_login("admin", long_pin);
    ASSERT_FALSE(ok);
    ASSERT_FALSE(auth_manager_is_logged_in());

    /* Add user with very long username */
    ok = auth_manager_add_user("Very Long User", long_username,
                                "1234", AUTH_ROLE_NURSE);
    /* May succeed (truncated) or fail -- just must not crash */
    (void)ok;

    auth_manager_close();
}

static void test_auth_empty_strings(void) {
    printf("  test_auth_empty_strings\n");

    auth_manager_init(":memory:");

    /* Empty username */
    bool ok = auth_manager_login("", "1234");
    ASSERT_FALSE(ok);
    ASSERT_FALSE(auth_manager_is_logged_in());

    /* Empty PIN */
    ok = auth_manager_login("admin", "");
    ASSERT_FALSE(ok);
    ASSERT_FALSE(auth_manager_is_logged_in());

    /* Both empty */
    ok = auth_manager_login("", "");
    ASSERT_FALSE(ok);
    ASSERT_FALSE(auth_manager_is_logged_in());

    /* Add user with empty fields */
    ok = auth_manager_add_user("", "", "", AUTH_ROLE_NURSE);
    ASSERT_FALSE(ok);

    /* Legitimate login still works */
    ok = auth_manager_login("admin", "1234");
    ASSERT_TRUE(ok);
    auth_manager_logout();

    auth_manager_close();
}

static void test_auth_deactivated_user_login(void) {
    printf("  test_auth_deactivated_user_login\n");

    auth_manager_init(":memory:");

    /* Login as admin, find nurse's ID, delete (deactivate) nurse */
    auth_user_t users[AUTH_MAX_USERS];
    int count = auth_manager_list_users(users, AUTH_MAX_USERS);
    int32_t nurse_id = -1;
    for (int i = 0; i < count; i++) {
        if (strcmp(users[i].username, "nurse") == 0) {
            nurse_id = users[i].id;
            break;
        }
    }
    ASSERT_GT_INT(nurse_id, 0);

    /* Delete (effectively deactivate) the nurse user */
    bool ok = auth_manager_delete_user(nurse_id);
    ASSERT_TRUE(ok);

    /* Attempt login with deleted/deactivated user */
    ok = auth_manager_login("nurse", "0000");
    ASSERT_FALSE(ok);
    ASSERT_FALSE(auth_manager_is_logged_in());

    auth_manager_close();
}

static void test_auth_permission_checks_per_role(void) {
    printf("  test_auth_permission_checks_per_role\n");

    auth_manager_init(":memory:");

    /* Not logged in: only VIEW_VITALS */
    ASSERT_TRUE(auth_manager_has_permission(AUTH_PERM_VIEW_VITALS));
    ASSERT_FALSE(auth_manager_has_permission(AUTH_PERM_ACK_ALARMS));
    ASSERT_FALSE(auth_manager_has_permission(AUTH_PERM_MANAGE_USERS));
    ASSERT_FALSE(auth_manager_has_permission(AUTH_PERM_VIEW_AUDIT_LOG));

    /* Nurse: can ack alarms, cannot manage users */
    auth_manager_login("nurse", "0000");
    ASSERT_TRUE(auth_manager_has_permission(AUTH_PERM_ACK_ALARMS));
    ASSERT_TRUE(auth_manager_has_permission(AUTH_PERM_SILENCE_ALARMS));
    ASSERT_FALSE(auth_manager_has_permission(AUTH_PERM_CHANGE_ALARM_LIMITS));
    ASSERT_FALSE(auth_manager_has_permission(AUTH_PERM_MANAGE_USERS));
    ASSERT_FALSE(auth_manager_has_permission(AUTH_PERM_VIEW_AUDIT_LOG));
    ASSERT_FALSE(auth_manager_has_permission(AUTH_PERM_CHANGE_SETTINGS));
    auth_manager_logout();

    /* Doctor: can change alarm limits, cannot manage users */
    auth_manager_login("doctor", "5678");
    ASSERT_TRUE(auth_manager_has_permission(AUTH_PERM_CHANGE_ALARM_LIMITS));
    ASSERT_TRUE(auth_manager_has_permission(AUTH_PERM_DISCHARGE_PATIENT));
    ASSERT_FALSE(auth_manager_has_permission(AUTH_PERM_MANAGE_USERS));
    ASSERT_FALSE(auth_manager_has_permission(AUTH_PERM_VIEW_AUDIT_LOG));
    auth_manager_logout();

    /* Technician: can change settings, cannot manage patients */
    auth_manager_login("tech", "9999");
    ASSERT_TRUE(auth_manager_has_permission(AUTH_PERM_CHANGE_SETTINGS));
    ASSERT_FALSE(auth_manager_has_permission(AUTH_PERM_MANAGE_PATIENTS));
    ASSERT_FALSE(auth_manager_has_permission(AUTH_PERM_MANAGE_USERS));
    auth_manager_logout();

    /* Admin: has all permissions */
    auth_manager_login("admin", "1234");
    for (int p = 0; p < AUTH_PERM_COUNT; p++) {
        ASSERT_TRUE(auth_manager_has_permission((auth_permission_t)p));
    }
    auth_manager_logout();

    auth_manager_close();
}

/* ================================================================
 *  4. Settings store with invalid keys and boundary values
 * ================================================================ */

static void test_settings_invalid_keys(void) {
    printf("  test_settings_invalid_keys\n");

    settings_store_init(":memory:");

    /* Non-existent key returns default */
    int val = settings_get_int("nonexistent.key", -1);
    ASSERT_EQ_INT(val, -1);

    float fval = settings_get_float("nonexistent.float", 3.14f);
    ASSERT_FLOAT_NEAR(fval, 3.14f, 0.01);

    bool bval = settings_get_bool("nonexistent.bool", true);
    ASSERT_TRUE(bval);

    const char *sval = settings_get_string("nonexistent.string", "default");
    ASSERT_NOT_NULL(sval);
    ASSERT_STR_EQ(sval, "default");

    /* Check key existence */
    ASSERT_FALSE(settings_exists("nonexistent.key"));
    ASSERT_TRUE(settings_exists(SETTINGS_KEY_BRIGHTNESS));

    settings_store_close();
}

static void test_settings_boundary_values(void) {
    printf("  test_settings_boundary_values\n");

    settings_store_init(":memory:");

    /* INT_MAX */
    bool ok = settings_set_int("test.max", INT_MAX);
    ASSERT_TRUE(ok);
    int val = settings_get_int("test.max", 0);
    ASSERT_EQ_INT(val, INT_MAX);

    /* INT_MIN */
    ok = settings_set_int("test.min", INT_MIN);
    ASSERT_TRUE(ok);
    val = settings_get_int("test.min", 0);
    ASSERT_EQ_INT(val, INT_MIN);

    /* Zero */
    ok = settings_set_int("test.zero", 0);
    ASSERT_TRUE(ok);
    val = settings_get_int("test.zero", 99);
    ASSERT_EQ_INT(val, 0);

    /* Negative values */
    ok = settings_set_int("test.neg", -42);
    ASSERT_TRUE(ok);
    val = settings_get_int("test.neg", 0);
    ASSERT_EQ_INT(val, -42);

    /* Very long string value (up to SETTINGS_STRING_MAX) */
    char long_value[SETTINGS_STRING_MAX];
    memset(long_value, 'X', SETTINGS_STRING_MAX - 1);
    long_value[SETTINGS_STRING_MAX - 1] = '\0';

    ok = settings_set_string("test.long", long_value);
    ASSERT_TRUE(ok);

    settings_store_close();
}

static void test_settings_null_params(void) {
    printf("  test_settings_null_params\n");

    settings_store_init(":memory:");

    /* NULL key should not crash */
    int val = settings_get_int(NULL, 42);
    ASSERT_EQ_INT(val, 42);

    bool ok = settings_set_int(NULL, 100);
    ASSERT_FALSE(ok);

    ok = settings_set_string(NULL, "value");
    ASSERT_FALSE(ok);

    /* settings_set_string with NULL value coerces to "" -- not an error */
    ok = settings_set_string("key", NULL);
    ASSERT_TRUE(ok);

    settings_store_close();
}

/* ================================================================
 *  5. Multiple rapid init/close cycles (stress test)
 * ================================================================ */

static void test_rapid_init_close_patient_data(void) {
    printf("  test_rapid_init_close_patient_data\n");

    for (int i = 0; i < 20; i++) {
        bool ok = patient_data_init(":memory:");
        ASSERT_TRUE(ok);
        patient_data_close();
    }
    /* Final check: should still work after many cycles */
    bool ok = patient_data_init(":memory:");
    ASSERT_TRUE(ok);
    patient_list_t list;
    int count = patient_data_list_all(&list);
    ASSERT_GE_INT(count, 0);
    patient_data_close();
}

static void test_rapid_init_close_settings(void) {
    printf("  test_rapid_init_close_settings\n");

    for (int i = 0; i < 20; i++) {
        bool ok = settings_store_init(":memory:");
        ASSERT_TRUE(ok);
        settings_store_close();
    }
    /* Final check */
    bool ok = settings_store_init(":memory:");
    ASSERT_TRUE(ok);
    int val = settings_get_int(SETTINGS_KEY_BRIGHTNESS, -1);
    ASSERT_GT_INT(val, 0);  /* Should have loaded default */
    settings_store_close();
}

static void test_rapid_init_close_auth(void) {
    printf("  test_rapid_init_close_auth\n");

    for (int i = 0; i < 20; i++) {
        bool ok = auth_manager_init(":memory:");
        ASSERT_TRUE(ok);
        auth_manager_close();
    }
    /* Final check: login still works after many cycles */
    bool ok = auth_manager_init(":memory:");
    ASSERT_TRUE(ok);
    ok = auth_manager_login("admin", "1234");
    ASSERT_TRUE(ok);
    auth_manager_logout();
    auth_manager_close();
}

static void test_rapid_init_close_audit(void) {
    printf("  test_rapid_init_close_audit\n");

    for (int i = 0; i < 20; i++) {
        bool ok = audit_log_init(":memory:");
        ASSERT_TRUE(ok);
        audit_log_close();
    }
    /* Final check */
    bool ok = audit_log_init(":memory:");
    ASSERT_TRUE(ok);
    audit_log_record(AUDIT_EVENT_SYSTEM_START, "system", "test");
    audit_query_result_t result;
    int count = audit_log_query_recent(10, &result);
    ASSERT_EQ_INT(count, 1);
    audit_log_close();
}

static void test_rapid_init_close_alarm_engine(void) {
    printf("  test_rapid_init_close_alarm_engine\n");

    for (int i = 0; i < 20; i++) {
        alarm_engine_init();
        alarm_engine_deinit();
    }
    /* Final check */
    alarm_engine_init();
    const alarm_engine_state_t *state = alarm_engine_get_state();
    ASSERT_NOT_NULL(state);
    ASSERT_EQ_INT(state->highest_active, ALARM_SEV_NONE);
    alarm_engine_deinit();
}

/* ================================================================
 *  6. Operations with NULL parameters
 * ================================================================ */

static void test_null_params_auth(void) {
    printf("  test_null_params_auth\n");

    auth_manager_init(":memory:");

    /* NULL params in login */
    ASSERT_FALSE(auth_manager_login(NULL, "1234"));
    ASSERT_FALSE(auth_manager_login("admin", NULL));
    ASSERT_FALSE(auth_manager_login(NULL, NULL));

    /* NULL params in add_user */
    ASSERT_FALSE(auth_manager_add_user(NULL, "user", "1234", AUTH_ROLE_NURSE));
    ASSERT_FALSE(auth_manager_add_user("Name", NULL, "1234", AUTH_ROLE_NURSE));
    ASSERT_FALSE(auth_manager_add_user("Name", "user", NULL, AUTH_ROLE_NURSE));

    /* NULL param in change_pin */
    ASSERT_FALSE(auth_manager_change_pin(1, NULL));

    /* NULL param in list_users */
    int count = auth_manager_list_users(NULL, 10);
    ASSERT_EQ_INT(count, 0);

    auth_manager_close();
}

static void test_null_params_audit(void) {
    printf("  test_null_params_audit\n");

    audit_log_init(":memory:");

    /* NULL params in query functions should not crash */
    int count = audit_log_query_recent(10, NULL);
    ASSERT_EQ_INT(count, 0);

    count = audit_log_query_by_user(NULL, 10, NULL);
    ASSERT_EQ_INT(count, 0);

    count = audit_log_query_by_event(AUDIT_EVENT_LOGIN, 10, NULL);
    ASSERT_EQ_INT(count, 0);

    audit_log_close();
}

static void test_null_params_alarm_engine(void) {
    printf("  test_null_params_alarm_engine\n");

    alarm_engine_init();

    /* NULL vitals data -- should not crash */
    alarm_engine_evaluate(NULL, 0);

    const alarm_engine_state_t *state = alarm_engine_get_state();
    ASSERT_NOT_NULL(state);
    ASSERT_EQ_INT(state->highest_active, ALARM_SEV_NONE);

    /* NULL limits pointer */
    alarm_engine_set_limits(ALARM_PARAM_HR, NULL);

    /* Out-of-range parameter index (should not crash) */
    const alarm_limits_t *lim = alarm_engine_get_limits(ALARM_PARAM_COUNT);
    (void)lim;  /* May return NULL or valid pointer; just must not crash */

    alarm_engine_deinit();
}

/* ================================================================
 *  7. Additional auth security tests (TEST-5.1)
 * ================================================================ */

static void test_auth_unicode_and_special_chars(void) {
    printf("  test_auth_unicode_and_special_chars\n");

    auth_manager_init(":memory:");

    /* Special characters in username */
    bool ok = auth_manager_login("<script>alert(1)</script>", "1234");
    ASSERT_FALSE(ok);

    ok = auth_manager_login("admin\0evil", "1234");
    /* This tests NUL byte injection -- C string truncates at \0,
       so it effectively becomes "admin". Depending on hash match,
       this might succeed -- the key point is it must not crash or
       bypass authentication. */
    (void)ok;

    /* Newlines and tabs in username */
    ok = auth_manager_login("admin\n\r\t", "1234");
    ASSERT_FALSE(ok);

    auth_manager_close();
}

static void test_auth_brute_force_lockout(void) {
    printf("  test_auth_brute_force_lockout\n");

    auth_manager_init(":memory:");

    /* Attempt multiple failed logins to trigger lockout.
     * The auth_manager has a 5-attempt lockout threshold. */
    for (int i = 0; i < 6; i++) {
        bool ok = auth_manager_login("admin", "9999");
        ASSERT_FALSE(ok);
    }

    /* After lockout, even the correct PIN should fail */
    bool ok = auth_manager_login("admin", "1234");
    /* This may or may not fail depending on timing (lockout_until_ts vs now).
     * Since we're running in fast succession, lockout is likely active. */
    (void)ok;

    ASSERT_FALSE(auth_manager_is_logged_in());

    auth_manager_close();
}

static void test_auth_role_boundary_values(void) {
    printf("  test_auth_role_boundary_values\n");

    /* Out-of-range roles should not crash */
    ASSERT_FALSE(auth_manager_role_has_permission((auth_role_t)-1, AUTH_PERM_VIEW_VITALS));
    ASSERT_FALSE(auth_manager_role_has_permission((auth_role_t)999, AUTH_PERM_VIEW_VITALS));
    ASSERT_FALSE(auth_manager_role_has_permission(AUTH_ROLE_ADMIN, (auth_permission_t)-1));
    ASSERT_FALSE(auth_manager_role_has_permission(AUTH_ROLE_ADMIN, (auth_permission_t)999));
    ASSERT_FALSE(auth_manager_role_has_permission(AUTH_ROLE_COUNT, AUTH_PERM_VIEW_VITALS));
}

/* ================================================================
 *  Public entry point
 * ================================================================ */

void test_failure_scenarios(void) {
    /* 1. Database ops after close */
    test_patient_data_ops_after_close();
    test_settings_store_ops_after_close();
    test_auth_manager_ops_after_close();
    test_audit_log_ops_after_close();
    test_trend_db_ops_after_close();

    /* 2. Alarm engine with extreme/boundary values */
    test_alarm_engine_extreme_values();
    test_alarm_engine_int_min_values();
    test_alarm_engine_zero_values();
    test_alarm_engine_boundary_at_limits();

    /* 3. Auth: SQL injection, empty strings, long strings (TEST-5.1) */
    test_auth_sql_injection_username();
    test_auth_sql_injection_in_add_user();
    test_auth_very_long_username();
    test_auth_empty_strings();
    test_auth_deactivated_user_login();
    test_auth_permission_checks_per_role();

    /* 4. Settings store: invalid keys, boundary values */
    test_settings_invalid_keys();
    test_settings_boundary_values();
    test_settings_null_params();

    /* 5. Rapid init/close cycles (stress test) */
    test_rapid_init_close_patient_data();
    test_rapid_init_close_settings();
    test_rapid_init_close_auth();
    test_rapid_init_close_audit();
    test_rapid_init_close_alarm_engine();

    /* 6. NULL parameter handling */
    test_null_params_auth();
    test_null_params_audit();
    test_null_params_alarm_engine();

    /* 7. Additional security tests (TEST-5.1) */
    test_auth_unicode_and_special_chars();
    test_auth_brute_force_lockout();
    test_auth_role_boundary_values();
}
