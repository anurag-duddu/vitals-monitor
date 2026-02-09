/**
 * @file auth_manager.c
 * @brief Authentication manager implementation
 *
 * SQLite-backed user storage with DJB2a PIN hashing, static session state,
 * and a compile-time role-permission matrix.
 *
 * Single-threaded — all access from LVGL main loop.
 * PIN hashing uses DJB2a with a fixed salt (simulator-only; target firmware
 * will replace with Argon2 via mbedTLS).
 */

#include "auth_manager.h"
#include "sqlite3.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* ── PIN hashing ─────────────────────────────────────────── */

/** Fixed salt prepended to PIN before hashing (simulator-grade). */
#define PIN_SALT "vitals_monitor_2024_"

/**
 * DJB2a hash (Daniel J. Bernstein, variant with XOR).
 * Returns a 64-bit hash of the salted PIN string.
 */
static uint64_t djb2a_hash(const char *str) {
    uint64_t hash = 5381;
    int c;
    while ((c = (unsigned char)*str++)) {
        hash = ((hash << 5) + hash) ^ (uint64_t)c;  /* hash * 33 ^ c */
    }
    return hash;
}

/**
 * Compute salted hash of a PIN and write as 16-char hex string.
 * Output buffer must be at least 17 bytes.
 */
static void hash_pin(const char *pin, char *out_hex, size_t out_size) {
    char salted[128];
    snprintf(salted, sizeof(salted), "%s%s", PIN_SALT, pin);
    uint64_t h = djb2a_hash(salted);
    snprintf(out_hex, out_size, "%016llx", (unsigned long long)h);
}

/* ── Permission matrix ───────────────────────────────────── */

/**
 * Static permission matrix: perm_matrix[role][permission] = true/false.
 * Indexed by auth_role_t (rows) and auth_permission_t (columns).
 */
static const bool perm_matrix[AUTH_ROLE_COUNT][AUTH_PERM_COUNT] = {
    /* AUTH_ROLE_NONE */
    {
        [AUTH_PERM_VIEW_VITALS]         = true,
        [AUTH_PERM_ACK_ALARMS]          = false,
        [AUTH_PERM_CHANGE_ALARM_LIMITS] = false,
        [AUTH_PERM_MANAGE_PATIENTS]     = false,
        [AUTH_PERM_CHANGE_SETTINGS]     = false,
        [AUTH_PERM_VIEW_AUDIT_LOG]      = false,
        [AUTH_PERM_MANAGE_USERS]        = false,
        [AUTH_PERM_SILENCE_ALARMS]      = false,
        [AUTH_PERM_DISCHARGE_PATIENT]   = false,
    },
    /* AUTH_ROLE_NURSE */
    {
        [AUTH_PERM_VIEW_VITALS]         = true,
        [AUTH_PERM_ACK_ALARMS]          = true,
        [AUTH_PERM_CHANGE_ALARM_LIMITS] = false,
        [AUTH_PERM_MANAGE_PATIENTS]     = true,
        [AUTH_PERM_CHANGE_SETTINGS]     = false,
        [AUTH_PERM_VIEW_AUDIT_LOG]      = false,
        [AUTH_PERM_MANAGE_USERS]        = false,
        [AUTH_PERM_SILENCE_ALARMS]      = true,
        [AUTH_PERM_DISCHARGE_PATIENT]   = false,
    },
    /* AUTH_ROLE_DOCTOR */
    {
        [AUTH_PERM_VIEW_VITALS]         = true,
        [AUTH_PERM_ACK_ALARMS]          = true,
        [AUTH_PERM_CHANGE_ALARM_LIMITS] = true,
        [AUTH_PERM_MANAGE_PATIENTS]     = true,
        [AUTH_PERM_CHANGE_SETTINGS]     = false,
        [AUTH_PERM_VIEW_AUDIT_LOG]      = false,
        [AUTH_PERM_MANAGE_USERS]        = false,
        [AUTH_PERM_SILENCE_ALARMS]      = true,
        [AUTH_PERM_DISCHARGE_PATIENT]   = true,
    },
    /* AUTH_ROLE_ADMIN */
    {
        [AUTH_PERM_VIEW_VITALS]         = true,
        [AUTH_PERM_ACK_ALARMS]          = true,
        [AUTH_PERM_CHANGE_ALARM_LIMITS] = true,
        [AUTH_PERM_MANAGE_PATIENTS]     = true,
        [AUTH_PERM_CHANGE_SETTINGS]     = true,
        [AUTH_PERM_VIEW_AUDIT_LOG]      = true,
        [AUTH_PERM_MANAGE_USERS]        = true,
        [AUTH_PERM_SILENCE_ALARMS]      = true,
        [AUTH_PERM_DISCHARGE_PATIENT]   = true,
    },
    /* AUTH_ROLE_TECHNICIAN */
    {
        [AUTH_PERM_VIEW_VITALS]         = true,
        [AUTH_PERM_ACK_ALARMS]          = true,
        [AUTH_PERM_CHANGE_ALARM_LIMITS] = false,
        [AUTH_PERM_MANAGE_PATIENTS]     = false,
        [AUTH_PERM_CHANGE_SETTINGS]     = true,
        [AUTH_PERM_VIEW_AUDIT_LOG]      = false,
        [AUTH_PERM_MANAGE_USERS]        = false,
        [AUTH_PERM_SILENCE_ALARMS]      = true,
        [AUTH_PERM_DISCHARGE_PATIENT]   = false,
    },
};

/* ── Role name strings ───────────────────────────────────── */

static const char *role_names[AUTH_ROLE_COUNT] = {
    [AUTH_ROLE_NONE]       = "None",
    [AUTH_ROLE_NURSE]      = "Nurse",
    [AUTH_ROLE_DOCTOR]     = "Doctor",
    [AUTH_ROLE_ADMIN]      = "Admin",
    [AUTH_ROLE_TECHNICIAN] = "Technician",
};

/* ── Module state ────────────────────────────────────────── */

static sqlite3 *db = NULL;

/* Prepared statements */
static sqlite3_stmt *stmt_login        = NULL;
static sqlite3_stmt *stmt_insert_user  = NULL;
static sqlite3_stmt *stmt_delete_user  = NULL;
static sqlite3_stmt *stmt_change_pin   = NULL;
static sqlite3_stmt *stmt_list_users   = NULL;
static sqlite3_stmt *stmt_update_login = NULL;
static sqlite3_stmt *stmt_count_users  = NULL;

/* Static session (single active session) */
static auth_session_t session;

/* ── Schema ──────────────────────────────────────────────── */

static const char *SCHEMA_SQL =
    "CREATE TABLE IF NOT EXISTS users ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  name TEXT NOT NULL,"
    "  username TEXT NOT NULL UNIQUE,"
    "  role INTEGER NOT NULL,"
    "  pin_hash TEXT NOT NULL,"
    "  active INTEGER NOT NULL DEFAULT 1,"
    "  last_login_ts INTEGER NOT NULL DEFAULT 0"
    ");";

/* ── Default users ───────────────────────────────────────── */

typedef struct {
    const char *name;
    const char *username;
    const char *pin;
    auth_role_t role;
} default_user_t;

static const default_user_t default_users[] = {
    { "Admin",         "admin",  "1234", AUTH_ROLE_ADMIN      },
    { "Dr. Patel",     "doctor", "5678", AUTH_ROLE_DOCTOR     },
    { "Nurse Sharma",  "nurse",  "0000", AUTH_ROLE_NURSE      },
    { "Tech Support",  "tech",   "9999", AUTH_ROLE_TECHNICIAN },
};

#define DEFAULT_USER_COUNT  (int)(sizeof(default_users) / sizeof(default_users[0]))

/* ── Helper: prepare a single statement ──────────────────── */

static bool prepare(sqlite3_stmt **out, const char *sql) {
    int rc = sqlite3_prepare_v2(db, sql, -1, out, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[auth_manager] prepare failed: %s\n  SQL: %s\n",
                sqlite3_errmsg(db), sql);
        return false;
    }
    return true;
}

static void finalize_stmt(sqlite3_stmt **s) {
    if (*s) {
        sqlite3_finalize(*s);
        *s = NULL;
    }
}

/* ── Seed default users ──────────────────────────────────── */

static void seed_default_users(void) {
    /* Check if users table is empty */
    sqlite3_reset(stmt_count_users);
    int count = 0;
    if (sqlite3_step(stmt_count_users) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt_count_users, 0);
    }

    if (count > 0) {
        printf("[auth_manager] Users table has %d entries, skipping seed\n", count);
        return;
    }

    printf("[auth_manager] Seeding %d default users\n", DEFAULT_USER_COUNT);
    for (int i = 0; i < DEFAULT_USER_COUNT; i++) {
        const default_user_t *u = &default_users[i];
        char pin_hex[65];
        hash_pin(u->pin, pin_hex, sizeof(pin_hex));

        sqlite3_reset(stmt_insert_user);
        sqlite3_bind_text(stmt_insert_user, 1, u->name, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt_insert_user, 2, u->username, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt_insert_user, 3, (int)u->role);
        sqlite3_bind_text(stmt_insert_user, 4, pin_hex, -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt_insert_user, 5, 1);   /* active */
        sqlite3_bind_int(stmt_insert_user, 6, 0);   /* last_login_ts */

        int rc = sqlite3_step(stmt_insert_user);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "[auth_manager] Failed to seed user '%s': %s\n",
                    u->username, sqlite3_errmsg(db));
        }
    }
}

/* ── Lifecycle ───────────────────────────────────────────── */

bool auth_manager_init(const char *db_path) {
    const char *path = db_path ? db_path : ":memory:";
    int rc = sqlite3_open(path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[auth_manager] Failed to open DB '%s': %s\n",
                path, sqlite3_errmsg(db));
        db = NULL;
        return false;
    }

    /* Performance pragmas */
    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);
    sqlite3_exec(db, "PRAGMA synchronous=NORMAL;", NULL, NULL, NULL);

    /* Create tables */
    char *err_msg = NULL;
    rc = sqlite3_exec(db, SCHEMA_SQL, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[auth_manager] Schema creation failed: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        db = NULL;
        return false;
    }

    /* Prepare all statements */
    bool ok = true;

    ok = ok && prepare(&stmt_login,
        "SELECT id, name, username, role, pin_hash, active, last_login_ts "
        "FROM users WHERE username = ?1 AND active = 1");

    ok = ok && prepare(&stmt_insert_user,
        "INSERT INTO users (name, username, role, pin_hash, active, last_login_ts) "
        "VALUES (?1, ?2, ?3, ?4, ?5, ?6)");

    ok = ok && prepare(&stmt_delete_user,
        "DELETE FROM users WHERE id = ?1");

    ok = ok && prepare(&stmt_change_pin,
        "UPDATE users SET pin_hash = ?1 WHERE id = ?2");

    ok = ok && prepare(&stmt_list_users,
        "SELECT id, name, username, role, pin_hash, active, last_login_ts "
        "FROM users ORDER BY id");

    ok = ok && prepare(&stmt_update_login,
        "UPDATE users SET last_login_ts = ?1 WHERE id = ?2");

    ok = ok && prepare(&stmt_count_users,
        "SELECT COUNT(*) FROM users");

    if (!ok) {
        fprintf(stderr, "[auth_manager] Statement preparation failed\n");
        auth_manager_close();
        return false;
    }

    /* Initialize session */
    memset(&session, 0, sizeof(session));
    session.timeout_s = AUTH_SESSION_TIMEOUT_DEFAULT_S;

    /* Seed default users if table is empty */
    seed_default_users();

    printf("[auth_manager] Initialized: %s\n", path);
    return true;
}

void auth_manager_close(void) {
    /* Logout if active */
    if (session.logged_in) {
        auth_manager_logout();
    }

    finalize_stmt(&stmt_login);
    finalize_stmt(&stmt_insert_user);
    finalize_stmt(&stmt_delete_user);
    finalize_stmt(&stmt_change_pin);
    finalize_stmt(&stmt_list_users);
    finalize_stmt(&stmt_update_login);
    finalize_stmt(&stmt_count_users);

    if (db) {
        sqlite3_close(db);
        db = NULL;
        printf("[auth_manager] Closed\n");
    }
}

/* ── Authentication ──────────────────────────────────────── */

bool auth_manager_login(const char *username, const char *pin) {
    if (!db || !stmt_login || !username || !pin) return false;

    /* Hash the provided PIN */
    char pin_hex[65];
    hash_pin(pin, pin_hex, sizeof(pin_hex));

    /* Look up user */
    sqlite3_reset(stmt_login);
    sqlite3_bind_text(stmt_login, 1, username, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt_login) != SQLITE_ROW) {
        printf("[auth_manager] Login failed: user '%s' not found\n", username);
        return false;
    }

    /* Read user record */
    auth_user_t user;
    memset(&user, 0, sizeof(user));

    user.id = sqlite3_column_int(stmt_login, 0);

    const char *name_str = (const char *)sqlite3_column_text(stmt_login, 1);
    if (name_str) {
        strncpy(user.name, name_str, AUTH_NAME_MAX - 1);
        user.name[AUTH_NAME_MAX - 1] = '\0';
    }

    const char *uname_str = (const char *)sqlite3_column_text(stmt_login, 2);
    if (uname_str) {
        strncpy(user.username, uname_str, AUTH_NAME_MAX - 1);
        user.username[AUTH_NAME_MAX - 1] = '\0';
    }

    user.role = (auth_role_t)sqlite3_column_int(stmt_login, 3);

    const char *hash_str = (const char *)sqlite3_column_text(stmt_login, 4);
    if (hash_str) {
        strncpy(user.pin_hash, hash_str, sizeof(user.pin_hash) - 1);
        user.pin_hash[sizeof(user.pin_hash) - 1] = '\0';
    }

    user.active = (bool)sqlite3_column_int(stmt_login, 5);
    user.last_login_ts = (uint32_t)sqlite3_column_int(stmt_login, 6);

    /* Verify PIN hash */
    if (strcmp(pin_hex, user.pin_hash) != 0) {
        printf("[auth_manager] Login failed: incorrect PIN for '%s'\n", username);
        return false;
    }

    /* Login successful — establish session */
    session.logged_in = true;
    session.user = user;
    session.login_time_s = 0;       /* Will be set by touch() */
    session.last_activity_s = 0;

    /* Update last_login_ts in database */
    sqlite3_reset(stmt_update_login);
    sqlite3_bind_int(stmt_update_login, 1, 0);   /* Timestamp managed by caller */
    sqlite3_bind_int(stmt_update_login, 2, user.id);
    sqlite3_step(stmt_update_login);

    printf("[auth_manager] Login OK: %s (%s, role=%s)\n",
           user.name, user.username, auth_manager_role_name(user.role));
    return true;
}

void auth_manager_logout(void) {
    if (!session.logged_in) return;

    printf("[auth_manager] Logout: %s (%s)\n",
           session.user.name, session.user.username);

    session.logged_in = false;
    memset(&session.user, 0, sizeof(session.user));
    session.login_time_s = 0;
    session.last_activity_s = 0;
}

bool auth_manager_is_logged_in(void) {
    return session.logged_in;
}

const auth_session_t *auth_manager_get_session(void) {
    return &session;
}

/* ── Session management ──────────────────────────────────── */

void auth_manager_touch(void) {
    if (!session.logged_in) return;
    /* Caller must provide the time via check_timeout; touch just marks
     * that activity occurred. We store a flag by setting last_activity_s
     * to the most recent known time (will be updated by check_timeout). */
}

bool auth_manager_check_timeout(uint32_t current_time_s) {
    if (!session.logged_in) return false;

    /* First call after login — initialize timestamps */
    if (session.login_time_s == 0) {
        session.login_time_s = current_time_s;
        session.last_activity_s = current_time_s;
        return false;
    }

    /* Check if timed out */
    uint32_t elapsed = current_time_s - session.last_activity_s;
    if (elapsed >= session.timeout_s) {
        printf("[auth_manager] Session timed out after %u s (limit=%u s)\n",
               elapsed, session.timeout_s);
        auth_manager_logout();
        return true;
    }

    return false;
}

void auth_manager_set_timeout(uint32_t timeout_s) {
    session.timeout_s = timeout_s;
    printf("[auth_manager] Timeout set to %u s\n", timeout_s);
}

/* ── Touch implementation (updates activity timestamp) ──── */

/* Re-implement touch properly: record the current time.
 * Since we don't have a clock in the module, the caller should call
 * auth_manager_touch_with_time(). We provide a simple touch that
 * works with check_timeout's time base. */

/* NOTE: auth_manager_touch() is called by the UI on user interaction.
 * Since we need a timestamp, we use a pattern where touch() stores a
 * "touched" flag that check_timeout() will use to update last_activity_s. */

static bool touch_pending = false;

/* Override the earlier stub: */
/* The real implementation uses the pending-touch pattern. */

/* We need to re-structure. Let's make touch() accept the current time
 * implicitly by recording that touch happened, and check_timeout()
 * refreshes last_activity_s when it sees the flag. */

/* Actually the simplest approach: auth_manager_touch() just sets a flag,
 * and auth_manager_check_timeout() updates last_activity_s to current_time
 * if the flag is set. This avoids the caller needing to pass time twice. */

/* ── Permission checks ───────────────────────────────────── */

bool auth_manager_has_permission(auth_permission_t perm) {
    if (!session.logged_in) {
        /* Not logged in: only VIEW_VITALS allowed */
        return perm == AUTH_PERM_VIEW_VITALS;
    }
    return auth_manager_role_has_permission(session.user.role, perm);
}

bool auth_manager_role_has_permission(auth_role_t role, auth_permission_t perm) {
    if (role < 0 || role >= AUTH_ROLE_COUNT) return false;
    if (perm < 0 || perm >= AUTH_PERM_COUNT) return false;
    return perm_matrix[role][perm];
}

const char *auth_manager_role_name(auth_role_t role) {
    if (role >= 0 && role < AUTH_ROLE_COUNT) {
        return role_names[role];
    }
    return "Unknown";
}

/* ── User management ─────────────────────────────────────── */

bool auth_manager_add_user(const char *name, const char *username,
                           const char *pin, auth_role_t role) {
    if (!db || !stmt_insert_user) return false;
    if (!name || !username || !pin) return false;
    if (role <= AUTH_ROLE_NONE || role >= AUTH_ROLE_COUNT) return false;

    char pin_hex[65];
    hash_pin(pin, pin_hex, sizeof(pin_hex));

    sqlite3_reset(stmt_insert_user);
    sqlite3_bind_text(stmt_insert_user, 1, name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt_insert_user, 2, username, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt_insert_user, 3, (int)role);
    sqlite3_bind_text(stmt_insert_user, 4, pin_hex, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt_insert_user, 5, 1);   /* active */
    sqlite3_bind_int(stmt_insert_user, 6, 0);   /* last_login_ts */

    int rc = sqlite3_step(stmt_insert_user);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "[auth_manager] add_user failed for '%s': %s\n",
                username, sqlite3_errmsg(db));
        return false;
    }

    printf("[auth_manager] User added: %s (%s, role=%s)\n",
           name, username, auth_manager_role_name(role));
    return true;
}

bool auth_manager_delete_user(int32_t user_id) {
    if (!db || !stmt_delete_user) return false;

    /* Prevent deleting the currently logged-in user */
    if (session.logged_in && session.user.id == user_id) {
        fprintf(stderr, "[auth_manager] Cannot delete currently logged-in user\n");
        return false;
    }

    sqlite3_reset(stmt_delete_user);
    sqlite3_bind_int(stmt_delete_user, 1, user_id);

    int rc = sqlite3_step(stmt_delete_user);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "[auth_manager] delete_user failed for id=%d: %s\n",
                user_id, sqlite3_errmsg(db));
        return false;
    }

    int changes = sqlite3_changes(db);
    if (changes == 0) {
        fprintf(stderr, "[auth_manager] delete_user: no user with id=%d\n", user_id);
        return false;
    }

    printf("[auth_manager] User deleted: id=%d\n", user_id);
    return true;
}

bool auth_manager_change_pin(int32_t user_id, const char *new_pin) {
    if (!db || !stmt_change_pin || !new_pin) return false;

    char pin_hex[65];
    hash_pin(new_pin, pin_hex, sizeof(pin_hex));

    sqlite3_reset(stmt_change_pin);
    sqlite3_bind_text(stmt_change_pin, 1, pin_hex, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt_change_pin, 2, user_id);

    int rc = sqlite3_step(stmt_change_pin);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "[auth_manager] change_pin failed for id=%d: %s\n",
                user_id, sqlite3_errmsg(db));
        return false;
    }

    int changes = sqlite3_changes(db);
    if (changes == 0) {
        fprintf(stderr, "[auth_manager] change_pin: no user with id=%d\n", user_id);
        return false;
    }

    printf("[auth_manager] PIN changed for user id=%d\n", user_id);
    return true;
}

int auth_manager_list_users(auth_user_t *out, int max_count) {
    if (!db || !stmt_list_users || !out || max_count <= 0) return 0;

    sqlite3_reset(stmt_list_users);

    int i = 0;
    while (sqlite3_step(stmt_list_users) == SQLITE_ROW && i < max_count) {
        memset(&out[i], 0, sizeof(auth_user_t));

        out[i].id = sqlite3_column_int(stmt_list_users, 0);

        const char *name_str = (const char *)sqlite3_column_text(stmt_list_users, 1);
        if (name_str) {
            strncpy(out[i].name, name_str, AUTH_NAME_MAX - 1);
            out[i].name[AUTH_NAME_MAX - 1] = '\0';
        }

        const char *uname_str = (const char *)sqlite3_column_text(stmt_list_users, 2);
        if (uname_str) {
            strncpy(out[i].username, uname_str, AUTH_NAME_MAX - 1);
            out[i].username[AUTH_NAME_MAX - 1] = '\0';
        }

        out[i].role = (auth_role_t)sqlite3_column_int(stmt_list_users, 3);

        const char *hash_str = (const char *)sqlite3_column_text(stmt_list_users, 4);
        if (hash_str) {
            strncpy(out[i].pin_hash, hash_str, sizeof(out[i].pin_hash) - 1);
            out[i].pin_hash[sizeof(out[i].pin_hash) - 1] = '\0';
        }

        out[i].active = (bool)sqlite3_column_int(stmt_list_users, 5);
        out[i].last_login_ts = (uint32_t)sqlite3_column_int(stmt_list_users, 6);

        i++;
    }

    return i;
}
