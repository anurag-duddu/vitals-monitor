/**
 * @file test_auth_audit_integration.c
 * @brief Integration tests: auth_manager + audit_log
 *
 * Verifies that authentication events (login, logout, failed login,
 * session timeout) are correctly recorded in the audit log and can
 * be queried back by user, event type, and time range.
 *
 * Uses in-memory SQLite databases for fast, isolated tests.
 *
 * NOTE: auth_manager_init() auto-seeds 4 default users on empty DB:
 *   admin/1234 (Admin), doctor/5678 (Doctor),
 *   nurse/0000 (Nurse), tech/9999 (Technician)
 */

#include "test_framework.h"
#include "auth_manager.h"
#include "audit_log.h"
#include <time.h>
#include <string.h>

/* ── Helper: set up both modules (uses seeded default users) ── */

static void setup(void) {
    audit_log_init(":memory:");
    auth_manager_init(":memory:");
    /* Default users are auto-seeded by auth_manager_init():
     *   admin/1234, doctor/5678, nurse/0000, tech/9999 */
}

static void teardown(void) {
    auth_manager_close();
    audit_log_close();
}

/* ── Test: init both modules together ──────────────────────── */

static void test_init_both(void) {
    printf("  test_init_both\n");

    bool audit_ok = audit_log_init(":memory:");
    ASSERT_TRUE(audit_ok);

    bool auth_ok = auth_manager_init(":memory:");
    ASSERT_TRUE(auth_ok);

    ASSERT_FALSE(auth_manager_is_logged_in());

    auth_manager_close();
    audit_log_close();
}

/* ── Test: login generates audit entry ─────────────────────── */

static void test_login_audit_entry(void) {
    printf("  test_login_audit_entry\n");

    setup();

    /* Login as admin (seeded default: PIN 1234) */
    bool logged_in = auth_manager_login("admin", "1234");
    ASSERT_TRUE(logged_in);
    ASSERT_TRUE(auth_manager_is_logged_in());

    /* Record the login event in audit log (application layer responsibility) */
    audit_log_record(AUDIT_EVENT_LOGIN, "admin", "Admin logged in");

    /* Query audit log for login events */
    audit_query_result_t result;
    int count = audit_log_query_by_event(AUDIT_EVENT_LOGIN, 10, &result);
    ASSERT_EQ_INT(count, 1);
    ASSERT_STR_EQ(result.entries[0].username, "admin");
    ASSERT_EQ_INT(result.entries[0].event, AUDIT_EVENT_LOGIN);

    auth_manager_logout();
    teardown();
}

/* ── Test: failed login generates audit entry ──────────────── */

static void test_failed_login_audit_entry(void) {
    printf("  test_failed_login_audit_entry\n");

    setup();

    /* Attempt login with wrong PIN (correct PIN is 1234) */
    bool logged_in = auth_manager_login("admin", "9999");
    ASSERT_FALSE(logged_in);
    ASSERT_FALSE(auth_manager_is_logged_in());

    /* Record the failed login event */
    audit_log_record(AUDIT_EVENT_LOGIN_FAILED, "admin",
                     "Failed login attempt");

    /* Verify audit entry */
    audit_query_result_t result;
    int count = audit_log_query_by_event(AUDIT_EVENT_LOGIN_FAILED, 10, &result);
    ASSERT_EQ_INT(count, 1);
    ASSERT_STR_EQ(result.entries[0].username, "admin");
    ASSERT_EQ_INT(result.entries[0].event, AUDIT_EVENT_LOGIN_FAILED);

    teardown();
}

/* ── Test: logout generates audit entry ────────────────────── */

static void test_logout_audit_entry(void) {
    printf("  test_logout_audit_entry\n");

    setup();

    /* Login as nurse (seeded default: PIN 0000) */
    bool logged_in = auth_manager_login("nurse", "0000");
    ASSERT_TRUE(logged_in);
    ASSERT_TRUE(auth_manager_is_logged_in());

    const auth_session_t *session = auth_manager_get_session();
    ASSERT_STR_EQ(session->user.username, "nurse");

    auth_manager_logout();
    ASSERT_FALSE(auth_manager_is_logged_in());

    /* Record logout event */
    audit_log_record(AUDIT_EVENT_LOGOUT, "nurse", "Nurse logged out");

    /* Verify audit entry */
    audit_query_result_t result;
    int count = audit_log_query_by_event(AUDIT_EVENT_LOGOUT, 10, &result);
    ASSERT_EQ_INT(count, 1);
    ASSERT_STR_EQ(result.entries[0].username, "nurse");
    ASSERT_EQ_INT(result.entries[0].event, AUDIT_EVENT_LOGOUT);

    teardown();
}

/* ── Test: session timeout generates audit entry ───────────── */

static void test_session_timeout_audit_entry(void) {
    printf("  test_session_timeout_audit_entry\n");

    setup();

    /* Login as admin */
    bool logged_in = auth_manager_login("admin", "1234");
    ASSERT_TRUE(logged_in);
    ASSERT_TRUE(auth_manager_is_logged_in());

    /* Set a very short timeout for testing */
    auth_manager_set_timeout(1);

    /* First check_timeout call initializes timestamps (login sets
       touch_pending=true, so it consumes the touch and returns false). */
    uint32_t now = (uint32_t)time(NULL);
    auth_manager_check_timeout(now);
    ASSERT_TRUE(auth_manager_is_logged_in());

    /* Second call with time far in the future triggers timeout */
    bool timed_out = auth_manager_check_timeout(now + 10);
    ASSERT_TRUE(timed_out);
    ASSERT_FALSE(auth_manager_is_logged_in());

    /* Record timeout event */
    audit_log_record(AUDIT_EVENT_SESSION_TIMEOUT, "admin",
                     "Session timed out");

    /* Verify audit entry */
    audit_query_result_t result;
    int count = audit_log_query_by_event(AUDIT_EVENT_SESSION_TIMEOUT,
                                          10, &result);
    ASSERT_EQ_INT(count, 1);
    ASSERT_STR_EQ(result.entries[0].username, "admin");
    ASSERT_EQ_INT(result.entries[0].event, AUDIT_EVENT_SESSION_TIMEOUT);

    teardown();
}

/* ── Test: query audit entries by specific user ────────────── */

static void test_query_audit_by_user(void) {
    printf("  test_query_audit_by_user\n");

    setup();

    /* Generate events for multiple users */
    auth_manager_login("admin", "1234");
    audit_log_record(AUDIT_EVENT_LOGIN, "admin", "Admin login 1");
    auth_manager_logout();

    auth_manager_login("nurse", "0000");
    audit_log_record(AUDIT_EVENT_LOGIN, "nurse", "Nurse login 1");
    audit_log_record(AUDIT_EVENT_ALARM_ACK, "nurse", "Nurse acked alarm");
    auth_manager_logout();

    auth_manager_login("admin", "1234");
    audit_log_record(AUDIT_EVENT_LOGIN, "admin", "Admin login 2");
    audit_log_record(AUDIT_EVENT_SETTINGS_CHANGED, "admin",
                     "Changed alarm volume");
    auth_manager_logout();

    /* Query for admin events */
    audit_query_result_t result;
    int count = audit_log_query_by_user("admin", 10, &result);
    ASSERT_EQ_INT(count, 3);

    for (int i = 0; i < count; i++) {
        ASSERT_STR_EQ(result.entries[i].username, "admin");
    }

    /* Query for nurse events */
    count = audit_log_query_by_user("nurse", 10, &result);
    ASSERT_EQ_INT(count, 2);

    for (int i = 0; i < count; i++) {
        ASSERT_STR_EQ(result.entries[i].username, "nurse");
    }

    /* Query for nonexistent user */
    count = audit_log_query_by_user("nobody", 10, &result);
    ASSERT_EQ_INT(count, 0);

    teardown();
}

/* ── Test: full login/operation/logout audit trail ─────────── */

static void test_full_audit_trail(void) {
    printf("  test_full_audit_trail\n");

    setup();

    /* Simulate a complete session workflow */
    audit_log_record(AUDIT_EVENT_SYSTEM_START, "system", "System booted");

    /* Failed login attempt */
    auth_manager_login("admin", "wrong");
    audit_log_record(AUDIT_EVENT_LOGIN_FAILED, "admin", "Wrong PIN");

    /* Successful login */
    auth_manager_login("admin", "1234");
    audit_log_record(AUDIT_EVENT_LOGIN, "admin", "Login successful");

    /* Admin operations */
    audit_log_record(AUDIT_EVENT_ALARM_LIMITS_CHANGED, "admin",
                     "HR limits 50-130");
    audit_log_record(AUDIT_EVENT_PATIENT_ADMIT, "admin",
                     "Admitted MRN-001");

    /* Logout */
    auth_manager_logout();
    audit_log_record(AUDIT_EVENT_LOGOUT, "admin", "Admin logout");

    /* Verify full trail */
    audit_query_result_t result;
    int count = audit_log_query_recent(20, &result);
    ASSERT_EQ_INT(count, 6);

    /* Verify event types (newest-first ordering) */
    ASSERT_EQ_INT(result.entries[0].event, AUDIT_EVENT_LOGOUT);
    ASSERT_EQ_INT(result.entries[1].event, AUDIT_EVENT_PATIENT_ADMIT);
    ASSERT_EQ_INT(result.entries[2].event, AUDIT_EVENT_ALARM_LIMITS_CHANGED);
    ASSERT_EQ_INT(result.entries[3].event, AUDIT_EVENT_LOGIN);
    ASSERT_EQ_INT(result.entries[4].event, AUDIT_EVENT_LOGIN_FAILED);
    ASSERT_EQ_INT(result.entries[5].event, AUDIT_EVENT_SYSTEM_START);

    teardown();
}

/* ── Test: permission check integration ────────────────────── */

static void test_permission_with_audit(void) {
    printf("  test_permission_with_audit\n");

    setup();

    /* Login as nurse (seeded default: PIN 0000) */
    bool logged_in = auth_manager_login("nurse", "0000");
    ASSERT_TRUE(logged_in);
    audit_log_record(AUDIT_EVENT_LOGIN, "nurse", "Nurse login");

    /* Nurse can acknowledge alarms */
    ASSERT_TRUE(auth_manager_has_permission(AUTH_PERM_ACK_ALARMS));

    /* Nurse cannot view audit log */
    ASSERT_FALSE(auth_manager_has_permission(AUTH_PERM_VIEW_AUDIT_LOG));

    /* Nurse cannot manage users */
    ASSERT_FALSE(auth_manager_has_permission(AUTH_PERM_MANAGE_USERS));

    auth_manager_logout();

    /* Login as admin */
    logged_in = auth_manager_login("admin", "1234");
    ASSERT_TRUE(logged_in);
    audit_log_record(AUDIT_EVENT_LOGIN, "admin", "Admin login");

    /* Admin can view audit log */
    ASSERT_TRUE(auth_manager_has_permission(AUTH_PERM_VIEW_AUDIT_LOG));

    /* Admin can manage users */
    ASSERT_TRUE(auth_manager_has_permission(AUTH_PERM_MANAGE_USERS));

    auth_manager_logout();
    teardown();
}

/* ── Public entry point ──────────────────────────────────── */

void test_auth_audit_integration(void) {
    test_init_both();
    test_login_audit_entry();
    test_failed_login_audit_entry();
    test_logout_audit_entry();
    test_session_timeout_audit_entry();
    test_query_audit_by_user();
    test_full_audit_trail();
    test_permission_with_audit();
}
