/**
 * @file auth_manager.h
 * @brief Authentication, session management, and role-based access control
 *
 * Provides PIN-based authentication with SQLite-backed user storage,
 * session timeout management, and permission checks against a static
 * role-permission matrix.
 *
 * Designed for Indian hospital bedside vitals monitors (CDSCO Class B).
 * Roles: Nurse, Doctor, Admin, Technician.
 *
 * Single-threaded — all calls from LVGL main loop.
 * Static allocation — no malloc in critical paths.
 */

#ifndef AUTH_MANAGER_H
#define AUTH_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Constants ───────────────────────────────────────────── */

#define AUTH_PIN_MAX_LEN                6
#define AUTH_NAME_MAX                   48
#define AUTH_MAX_USERS                  16
#define AUTH_SESSION_TIMEOUT_DEFAULT_S  300   /* 5 minutes */
#define AUTH_PIN_HASH_LEN              17    /* 16 hex chars + NUL */

/* ── Role enumeration ────────────────────────────────────── */

typedef enum {
    AUTH_ROLE_NONE = 0,
    AUTH_ROLE_NURSE,
    AUTH_ROLE_DOCTOR,
    AUTH_ROLE_ADMIN,
    AUTH_ROLE_TECHNICIAN,
    AUTH_ROLE_COUNT
} auth_role_t;

/* ── Permission enumeration ──────────────────────────────── */

typedef enum {
    AUTH_PERM_VIEW_VITALS = 0,       /* Everyone */
    AUTH_PERM_ACK_ALARMS,            /* Nurse+ */
    AUTH_PERM_CHANGE_ALARM_LIMITS,   /* Doctor+ */
    AUTH_PERM_MANAGE_PATIENTS,       /* Nurse+ */
    AUTH_PERM_CHANGE_SETTINGS,       /* Technician, Admin */
    AUTH_PERM_VIEW_AUDIT_LOG,        /* Admin only */
    AUTH_PERM_MANAGE_USERS,          /* Admin only */
    AUTH_PERM_SILENCE_ALARMS,        /* Nurse+ */
    AUTH_PERM_DISCHARGE_PATIENT,     /* Doctor+ */
    AUTH_PERM_COUNT
} auth_permission_t;

/* ── User record ─────────────────────────────────────────── */

typedef struct {
    int32_t     id;                          /* SQLite rowid */
    char        name[AUTH_NAME_MAX];
    char        username[AUTH_NAME_MAX];
    auth_role_t role;
    char        pin_hash[65];                /* Hash hex string (padded) */
    bool        active;                      /* Can be deactivated */
    uint32_t    last_login_ts;
} auth_user_t;

/* ── Session state ───────────────────────────────────────── */

typedef struct {
    bool        logged_in;
    auth_user_t user;                        /* Current user (copied on login) */
    uint32_t    login_time_s;
    uint32_t    last_activity_s;             /* Updated on user interaction */
    uint32_t    timeout_s;                   /* Session timeout duration */
} auth_session_t;

/* ── Lifecycle ───────────────────────────────────────────── */

/** Initialize the auth manager. Opens/creates users table. */
bool auth_manager_init(const char *db_path);

/** Close the auth manager and finalize all prepared statements. */
void auth_manager_close(void);

/* ── Authentication ──────────────────────────────────────── */

/** Authenticate with username and PIN. Returns true on success. */
bool auth_manager_login(const char *username, const char *pin);

/** End the current session. */
void auth_manager_logout(void);

/** Check if a user is currently logged in. */
bool auth_manager_is_logged_in(void);

/** Get the current session state (read-only). */
const auth_session_t *auth_manager_get_session(void);

/* ── Session management ──────────────────────────────────── */

/** Reset the activity timer (call on any user interaction). */
void auth_manager_touch(void);

/** Check if session has timed out. Returns true if timed out (and logs out). */
bool auth_manager_check_timeout(uint32_t current_time_s);

/** Set the session timeout duration (seconds). */
void auth_manager_set_timeout(uint32_t timeout_s);

/* ── Permission checks ───────────────────────────────────── */

/** Check if the current logged-in user has a given permission. */
bool auth_manager_has_permission(auth_permission_t perm);

/** Check if a role has a given permission (no login required). */
bool auth_manager_role_has_permission(auth_role_t role, auth_permission_t perm);

/** Get human-readable name for a role. */
const char *auth_manager_role_name(auth_role_t role);

/* ── User management ─────────────────────────────────────── */

/** Add a new user. Returns true on success. */
bool auth_manager_add_user(const char *name, const char *username,
                           const char *pin, auth_role_t role);

/** Delete a user by ID. Returns true on success. */
bool auth_manager_delete_user(int32_t user_id);

/** Change a user's PIN. Returns true on success. */
bool auth_manager_change_pin(int32_t user_id, const char *new_pin);

/** List all users. Returns count written to out (up to max_count). */
int auth_manager_list_users(auth_user_t *out, int max_count);

#ifdef __cplusplus
}
#endif

#endif /* AUTH_MANAGER_H */
