/**
 * @file test_auth_manager.c
 * @brief Unit tests for auth_manager module
 *
 * Tests PIN-based authentication, session management, permission checks,
 * user CRUD, and timeout behavior using in-memory SQLite.
 */

#include "test_framework.h"
#include "auth_manager.h"

/* ── Test: init seeds default users ──────────────────────── */

static void test_init_seeds_users(void) {
    printf("  test_init_seeds_users\n");

    bool ok = auth_manager_init(NULL);  /* in-memory */
    ASSERT_TRUE(ok);

    /* List users: should have 4 default users */
    auth_user_t users[AUTH_MAX_USERS];
    int count = auth_manager_list_users(users, AUTH_MAX_USERS);
    ASSERT_EQ_INT(count, 4);

    /* Verify default usernames exist */
    bool found_admin = false, found_doctor = false;
    bool found_nurse = false, found_tech = false;
    for (int i = 0; i < count; i++) {
        if (strcmp(users[i].username, "admin") == 0)  found_admin = true;
        if (strcmp(users[i].username, "doctor") == 0) found_doctor = true;
        if (strcmp(users[i].username, "nurse") == 0)  found_nurse = true;
        if (strcmp(users[i].username, "tech") == 0)   found_tech = true;
    }
    ASSERT_TRUE(found_admin);
    ASSERT_TRUE(found_doctor);
    ASSERT_TRUE(found_nurse);
    ASSERT_TRUE(found_tech);

    auth_manager_close();
}

/* ── Test: login with correct PIN ────────────────────────── */

static void test_login_correct_pin(void) {
    printf("  test_login_correct_pin\n");

    auth_manager_init(NULL);

    /* Admin PIN is "1234" */
    bool ok = auth_manager_login("admin", "1234");
    ASSERT_TRUE(ok);
    ASSERT_TRUE(auth_manager_is_logged_in());

    const auth_session_t *s = auth_manager_get_session();
    ASSERT_NOT_NULL(s);
    ASSERT_TRUE(s->logged_in);
    ASSERT_STR_EQ(s->user.username, "admin");
    ASSERT_EQ_INT(s->user.role, AUTH_ROLE_ADMIN);

    auth_manager_close();
}

/* ── Test: login with wrong PIN fails ────────────────────── */

static void test_login_wrong_pin(void) {
    printf("  test_login_wrong_pin\n");

    auth_manager_init(NULL);

    bool ok = auth_manager_login("admin", "9999");
    ASSERT_FALSE(ok);
    ASSERT_FALSE(auth_manager_is_logged_in());

    auth_manager_close();
}

/* ── Test: login with non-existent user fails ────────────── */

static void test_login_nonexistent_user(void) {
    printf("  test_login_nonexistent_user\n");

    auth_manager_init(NULL);

    bool ok = auth_manager_login("nobody", "1234");
    ASSERT_FALSE(ok);
    ASSERT_FALSE(auth_manager_is_logged_in());

    auth_manager_close();
}

/* ── Test: login with NULL arguments fails ───────────────── */

static void test_login_null_args(void) {
    printf("  test_login_null_args\n");

    auth_manager_init(NULL);

    ASSERT_FALSE(auth_manager_login(NULL, "1234"));
    ASSERT_FALSE(auth_manager_login("admin", NULL));
    ASSERT_FALSE(auth_manager_login(NULL, NULL));

    auth_manager_close();
}

/* ── Test: logout ────────────────────────────────────────── */

static void test_logout(void) {
    printf("  test_logout\n");

    auth_manager_init(NULL);

    auth_manager_login("admin", "1234");
    ASSERT_TRUE(auth_manager_is_logged_in());

    auth_manager_logout();
    ASSERT_FALSE(auth_manager_is_logged_in());

    const auth_session_t *s = auth_manager_get_session();
    ASSERT_FALSE(s->logged_in);

    auth_manager_close();
}

/* ── Test: login as each default user ────────────────────── */

static void test_login_all_defaults(void) {
    printf("  test_login_all_defaults\n");

    auth_manager_init(NULL);

    /* Doctor */
    ASSERT_TRUE(auth_manager_login("doctor", "5678"));
    ASSERT_EQ_INT(auth_manager_get_session()->user.role, AUTH_ROLE_DOCTOR);
    auth_manager_logout();

    /* Nurse */
    ASSERT_TRUE(auth_manager_login("nurse", "0000"));
    ASSERT_EQ_INT(auth_manager_get_session()->user.role, AUTH_ROLE_NURSE);
    auth_manager_logout();

    /* Technician */
    ASSERT_TRUE(auth_manager_login("tech", "9999"));
    ASSERT_EQ_INT(auth_manager_get_session()->user.role, AUTH_ROLE_TECHNICIAN);
    auth_manager_logout();

    auth_manager_close();
}

/* ── Test: session timeout ───────────────────────────────── */

static void test_session_timeout(void) {
    printf("  test_session_timeout\n");

    auth_manager_init(NULL);
    auth_manager_login("admin", "1234");

    /* Set a short timeout */
    auth_manager_set_timeout(60);

    /* Initialize timestamps via first check_timeout */
    bool timed_out = auth_manager_check_timeout(1000);
    ASSERT_FALSE(timed_out);
    ASSERT_TRUE(auth_manager_is_logged_in());

    /* Before timeout expires */
    timed_out = auth_manager_check_timeout(1050);
    ASSERT_FALSE(timed_out);
    ASSERT_TRUE(auth_manager_is_logged_in());

    /* After timeout expires (1000 + 60 = 1060) */
    timed_out = auth_manager_check_timeout(1100);
    ASSERT_TRUE(timed_out);
    ASSERT_FALSE(auth_manager_is_logged_in());

    auth_manager_close();
}

/* ── Test: touch resets timeout ──────────────────────────── */

static void test_touch_resets_timeout(void) {
    printf("  test_touch_resets_timeout\n");

    auth_manager_init(NULL);
    auth_manager_login("admin", "1234");
    auth_manager_set_timeout(60);

    /* Initialize */
    auth_manager_check_timeout(1000);

    /* Activity at t=1050 */
    auth_manager_touch();
    bool timed_out = auth_manager_check_timeout(1050);
    ASSERT_FALSE(timed_out);

    /* Now timeout is relative to 1050, so 1100 should be safe */
    timed_out = auth_manager_check_timeout(1100);
    ASSERT_FALSE(timed_out);

    /* But 1050+60 = 1110, so 1120 should timeout */
    timed_out = auth_manager_check_timeout(1120);
    ASSERT_TRUE(timed_out);

    auth_manager_close();
}

/* ── Test: permission matrix for each role ───────────────── */

static void test_permissions_none(void) {
    printf("  test_permissions_none\n");

    /* NONE role: only VIEW_VITALS */
    ASSERT_TRUE(auth_manager_role_has_permission(AUTH_ROLE_NONE, AUTH_PERM_VIEW_VITALS));
    ASSERT_FALSE(auth_manager_role_has_permission(AUTH_ROLE_NONE, AUTH_PERM_ACK_ALARMS));
    ASSERT_FALSE(auth_manager_role_has_permission(AUTH_ROLE_NONE, AUTH_PERM_CHANGE_ALARM_LIMITS));
    ASSERT_FALSE(auth_manager_role_has_permission(AUTH_ROLE_NONE, AUTH_PERM_MANAGE_PATIENTS));
    ASSERT_FALSE(auth_manager_role_has_permission(AUTH_ROLE_NONE, AUTH_PERM_CHANGE_SETTINGS));
    ASSERT_FALSE(auth_manager_role_has_permission(AUTH_ROLE_NONE, AUTH_PERM_VIEW_AUDIT_LOG));
    ASSERT_FALSE(auth_manager_role_has_permission(AUTH_ROLE_NONE, AUTH_PERM_MANAGE_USERS));
    ASSERT_FALSE(auth_manager_role_has_permission(AUTH_ROLE_NONE, AUTH_PERM_SILENCE_ALARMS));
    ASSERT_FALSE(auth_manager_role_has_permission(AUTH_ROLE_NONE, AUTH_PERM_DISCHARGE_PATIENT));
}

static void test_permissions_nurse(void) {
    printf("  test_permissions_nurse\n");

    ASSERT_TRUE(auth_manager_role_has_permission(AUTH_ROLE_NURSE, AUTH_PERM_VIEW_VITALS));
    ASSERT_TRUE(auth_manager_role_has_permission(AUTH_ROLE_NURSE, AUTH_PERM_ACK_ALARMS));
    ASSERT_FALSE(auth_manager_role_has_permission(AUTH_ROLE_NURSE, AUTH_PERM_CHANGE_ALARM_LIMITS));
    ASSERT_TRUE(auth_manager_role_has_permission(AUTH_ROLE_NURSE, AUTH_PERM_MANAGE_PATIENTS));
    ASSERT_FALSE(auth_manager_role_has_permission(AUTH_ROLE_NURSE, AUTH_PERM_CHANGE_SETTINGS));
    ASSERT_FALSE(auth_manager_role_has_permission(AUTH_ROLE_NURSE, AUTH_PERM_VIEW_AUDIT_LOG));
    ASSERT_FALSE(auth_manager_role_has_permission(AUTH_ROLE_NURSE, AUTH_PERM_MANAGE_USERS));
    ASSERT_TRUE(auth_manager_role_has_permission(AUTH_ROLE_NURSE, AUTH_PERM_SILENCE_ALARMS));
    ASSERT_FALSE(auth_manager_role_has_permission(AUTH_ROLE_NURSE, AUTH_PERM_DISCHARGE_PATIENT));
}

static void test_permissions_doctor(void) {
    printf("  test_permissions_doctor\n");

    ASSERT_TRUE(auth_manager_role_has_permission(AUTH_ROLE_DOCTOR, AUTH_PERM_VIEW_VITALS));
    ASSERT_TRUE(auth_manager_role_has_permission(AUTH_ROLE_DOCTOR, AUTH_PERM_ACK_ALARMS));
    ASSERT_TRUE(auth_manager_role_has_permission(AUTH_ROLE_DOCTOR, AUTH_PERM_CHANGE_ALARM_LIMITS));
    ASSERT_TRUE(auth_manager_role_has_permission(AUTH_ROLE_DOCTOR, AUTH_PERM_MANAGE_PATIENTS));
    ASSERT_FALSE(auth_manager_role_has_permission(AUTH_ROLE_DOCTOR, AUTH_PERM_CHANGE_SETTINGS));
    ASSERT_FALSE(auth_manager_role_has_permission(AUTH_ROLE_DOCTOR, AUTH_PERM_VIEW_AUDIT_LOG));
    ASSERT_FALSE(auth_manager_role_has_permission(AUTH_ROLE_DOCTOR, AUTH_PERM_MANAGE_USERS));
    ASSERT_TRUE(auth_manager_role_has_permission(AUTH_ROLE_DOCTOR, AUTH_PERM_SILENCE_ALARMS));
    ASSERT_TRUE(auth_manager_role_has_permission(AUTH_ROLE_DOCTOR, AUTH_PERM_DISCHARGE_PATIENT));
}

static void test_permissions_admin(void) {
    printf("  test_permissions_admin\n");

    /* Admin has all permissions */
    for (int p = 0; p < AUTH_PERM_COUNT; p++) {
        ASSERT_TRUE(auth_manager_role_has_permission(AUTH_ROLE_ADMIN, (auth_permission_t)p));
    }
}

static void test_permissions_technician(void) {
    printf("  test_permissions_technician\n");

    ASSERT_TRUE(auth_manager_role_has_permission(AUTH_ROLE_TECHNICIAN, AUTH_PERM_VIEW_VITALS));
    ASSERT_TRUE(auth_manager_role_has_permission(AUTH_ROLE_TECHNICIAN, AUTH_PERM_ACK_ALARMS));
    ASSERT_FALSE(auth_manager_role_has_permission(AUTH_ROLE_TECHNICIAN, AUTH_PERM_CHANGE_ALARM_LIMITS));
    ASSERT_FALSE(auth_manager_role_has_permission(AUTH_ROLE_TECHNICIAN, AUTH_PERM_MANAGE_PATIENTS));
    ASSERT_TRUE(auth_manager_role_has_permission(AUTH_ROLE_TECHNICIAN, AUTH_PERM_CHANGE_SETTINGS));
    ASSERT_FALSE(auth_manager_role_has_permission(AUTH_ROLE_TECHNICIAN, AUTH_PERM_VIEW_AUDIT_LOG));
    ASSERT_FALSE(auth_manager_role_has_permission(AUTH_ROLE_TECHNICIAN, AUTH_PERM_MANAGE_USERS));
    ASSERT_TRUE(auth_manager_role_has_permission(AUTH_ROLE_TECHNICIAN, AUTH_PERM_SILENCE_ALARMS));
    ASSERT_FALSE(auth_manager_role_has_permission(AUTH_ROLE_TECHNICIAN, AUTH_PERM_DISCHARGE_PATIENT));
}

/* ── Test: has_permission checks current session ─────────── */

static void test_has_permission_with_session(void) {
    printf("  test_has_permission_with_session\n");

    auth_manager_init(NULL);

    /* Not logged in: only VIEW_VITALS */
    ASSERT_TRUE(auth_manager_has_permission(AUTH_PERM_VIEW_VITALS));
    ASSERT_FALSE(auth_manager_has_permission(AUTH_PERM_ACK_ALARMS));

    /* Login as nurse */
    auth_manager_login("nurse", "0000");
    ASSERT_TRUE(auth_manager_has_permission(AUTH_PERM_ACK_ALARMS));
    ASSERT_FALSE(auth_manager_has_permission(AUTH_PERM_CHANGE_ALARM_LIMITS));

    /* Login as doctor (implicit logout of nurse) */
    auth_manager_logout();
    auth_manager_login("doctor", "5678");
    ASSERT_TRUE(auth_manager_has_permission(AUTH_PERM_CHANGE_ALARM_LIMITS));
    ASSERT_TRUE(auth_manager_has_permission(AUTH_PERM_DISCHARGE_PATIENT));
    ASSERT_FALSE(auth_manager_has_permission(AUTH_PERM_MANAGE_USERS));

    auth_manager_close();
}

/* ── Test: add user ──────────────────────────────────────── */

static void test_add_user(void) {
    printf("  test_add_user\n");

    auth_manager_init(NULL);

    bool ok = auth_manager_add_user("New Nurse", "nurse2", "1111", AUTH_ROLE_NURSE);
    ASSERT_TRUE(ok);

    /* Should now be able to login */
    ok = auth_manager_login("nurse2", "1111");
    ASSERT_TRUE(ok);
    ASSERT_STR_EQ(auth_manager_get_session()->user.name, "New Nurse");
    ASSERT_EQ_INT(auth_manager_get_session()->user.role, AUTH_ROLE_NURSE);
    auth_manager_logout();

    /* Should appear in user list */
    auth_user_t users[AUTH_MAX_USERS];
    int count = auth_manager_list_users(users, AUTH_MAX_USERS);
    ASSERT_EQ_INT(count, 5);  /* 4 default + 1 new */

    auth_manager_close();
}

/* ── Test: add user with invalid role fails ──────────────── */

static void test_add_user_invalid_role(void) {
    printf("  test_add_user_invalid_role\n");

    auth_manager_init(NULL);

    bool ok = auth_manager_add_user("Bad User", "bad", "1234", AUTH_ROLE_NONE);
    ASSERT_FALSE(ok);

    ok = auth_manager_add_user("Bad User", "bad", "1234", AUTH_ROLE_COUNT);
    ASSERT_FALSE(ok);

    auth_manager_close();
}

/* ── Test: add duplicate username fails ──────────────────── */

static void test_add_duplicate_username(void) {
    printf("  test_add_duplicate_username\n");

    auth_manager_init(NULL);

    /* "admin" already exists from seed */
    bool ok = auth_manager_add_user("Another Admin", "admin", "0000", AUTH_ROLE_ADMIN);
    ASSERT_FALSE(ok);

    auth_manager_close();
}

/* ── Test: delete user ───────────────────────────────────── */

static void test_delete_user(void) {
    printf("  test_delete_user\n");

    auth_manager_init(NULL);

    /* Find the nurse user's ID */
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

    /* Delete the nurse */
    bool ok = auth_manager_delete_user(nurse_id);
    ASSERT_TRUE(ok);

    /* Should not be able to login */
    ok = auth_manager_login("nurse", "0000");
    ASSERT_FALSE(ok);

    /* User count should decrease */
    int new_count = auth_manager_list_users(users, AUTH_MAX_USERS);
    ASSERT_EQ_INT(new_count, count - 1);

    /* Deleting non-existent should fail */
    ok = auth_manager_delete_user(99999);
    ASSERT_FALSE(ok);

    auth_manager_close();
}

/* ── Test: cannot delete currently logged-in user ────────── */

static void test_cannot_delete_self(void) {
    printf("  test_cannot_delete_self\n");

    auth_manager_init(NULL);

    auth_manager_login("admin", "1234");
    int32_t admin_id = auth_manager_get_session()->user.id;

    /* Should fail */
    bool ok = auth_manager_delete_user(admin_id);
    ASSERT_FALSE(ok);

    /* Still logged in */
    ASSERT_TRUE(auth_manager_is_logged_in());

    auth_manager_close();
}

/* ── Test: change PIN ────────────────────────────────────── */

static void test_change_pin(void) {
    printf("  test_change_pin\n");

    auth_manager_init(NULL);

    /* Find admin ID */
    auth_user_t users[AUTH_MAX_USERS];
    auth_manager_list_users(users, AUTH_MAX_USERS);
    int32_t admin_id = -1;
    for (int i = 0; i < AUTH_MAX_USERS; i++) {
        if (strcmp(users[i].username, "admin") == 0) {
            admin_id = users[i].id;
            break;
        }
    }
    ASSERT_GT_INT(admin_id, 0);

    /* Change PIN */
    bool ok = auth_manager_change_pin(admin_id, "4321");
    ASSERT_TRUE(ok);

    /* Old PIN should fail */
    ok = auth_manager_login("admin", "1234");
    ASSERT_FALSE(ok);

    /* New PIN should work */
    ok = auth_manager_login("admin", "4321");
    ASSERT_TRUE(ok);

    auth_manager_close();
}

/* ── Test: change PIN for non-existent user fails ────────── */

static void test_change_pin_nonexistent(void) {
    printf("  test_change_pin_nonexistent\n");

    auth_manager_init(NULL);

    bool ok = auth_manager_change_pin(99999, "0000");
    ASSERT_FALSE(ok);

    auth_manager_close();
}

/* ── Test: role name strings ─────────────────────────────── */

static void test_role_names(void) {
    printf("  test_role_names\n");

    ASSERT_STR_EQ(auth_manager_role_name(AUTH_ROLE_NONE), "None");
    ASSERT_STR_EQ(auth_manager_role_name(AUTH_ROLE_NURSE), "Nurse");
    ASSERT_STR_EQ(auth_manager_role_name(AUTH_ROLE_DOCTOR), "Doctor");
    ASSERT_STR_EQ(auth_manager_role_name(AUTH_ROLE_ADMIN), "Admin");
    ASSERT_STR_EQ(auth_manager_role_name(AUTH_ROLE_TECHNICIAN), "Technician");
    ASSERT_STR_EQ(auth_manager_role_name(AUTH_ROLE_COUNT), "Unknown");
}

/* ── Test: check_timeout when not logged in ──────────────── */

static void test_timeout_not_logged_in(void) {
    printf("  test_timeout_not_logged_in\n");

    auth_manager_init(NULL);

    /* Should return false and not crash */
    bool timed_out = auth_manager_check_timeout(1000);
    ASSERT_FALSE(timed_out);

    auth_manager_close();
}

/* ── Public entry point ──────────────────────────────────── */

void test_auth_manager(void) {
    test_init_seeds_users();
    test_login_correct_pin();
    test_login_wrong_pin();
    test_login_nonexistent_user();
    test_login_null_args();
    test_logout();
    test_login_all_defaults();
    test_session_timeout();
    test_touch_resets_timeout();
    test_permissions_none();
    test_permissions_nurse();
    test_permissions_doctor();
    test_permissions_admin();
    test_permissions_technician();
    test_has_permission_with_session();
    test_add_user();
    test_add_user_invalid_role();
    test_add_duplicate_username();
    test_delete_user();
    test_cannot_delete_self();
    test_change_pin();
    test_change_pin_nonexistent();
    test_role_names();
    test_timeout_not_logged_in();
}
