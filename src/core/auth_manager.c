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
#include <stdint.h>
#include <time.h>

/* ── PIN hashing ─────────────────────────────────────────── */

/** Fixed salt prepended to PIN before hashing (simulator-grade). */
#define PIN_SALT "vitals_monitor_2024_"

#ifdef SIMULATOR_BUILD
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
#else
#error "Production builds must replace DJB2a with Argon2id (via mbedTLS). See docs/security/pin-hashing.md"
#endif

/* ── Constant-time comparison ────────────────────────────── */

/**
 * Constant-time string comparison to prevent timing attacks.
 * Compares exactly `len` bytes; returns 0 if equal, non-zero otherwise.
 */
static int constant_time_cmp(const char *a, const char *b, size_t len) {
    volatile unsigned char result = 0;
    for (size_t i = 0; i < len; i++) {
        result |= (unsigned char)a[i] ^ (unsigned char)b[i];
    }
    return result;
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
static sqlite3_stmt *stmt_get_lockout  = NULL;
static sqlite3_stmt *stmt_set_lockout  = NULL;

/* Static session (single active session) */
static auth_session_t session;

/* Touch flag: set by auth_manager_touch(), consumed by check_timeout() */
static bool touch_pending = false;

/* Brute-force lockout constants (per-user state stored in DB) */
#define AUTH_MAX_FAILED_ATTEMPTS  5
#define AUTH_LOCKOUT_DURATION_S   300  /* 5 minutes */

/* ── Schema ──────────────────────────────────────────────── */

static const char *SCHEMA_SQL =
    "CREATE TABLE IF NOT EXISTS users ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  name TEXT NOT NULL,"
    "  username TEXT NOT NULL UNIQUE,"
    "  role INTEGER NOT NULL,"
    "  pin_hash TEXT NOT NULL,"
    "  active INTEGER NOT NULL DEFAULT 1,"
    "  last_login_ts INTEGER NOT NULL DEFAULT 0,"
    "  failed_attempts INTEGER NOT NULL DEFAULT 0,"
    "  lockout_until_ts INTEGER NOT NULL DEFAULT 0"
    ");";

/* ── Default users (simulator only) ──────────────────────── */

#ifdef SIMULATOR_BUILD
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
#endif /* SIMULATOR_BUILD */

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

/* ── Helper: read a user row from a stepped statement ────── */

static void read_user_row(sqlite3_stmt *stmt, auth_user_t *user) {
    memset(user, 0, sizeof(auth_user_t));

    user->id = sqlite3_column_int(stmt, 0);

    const char *name_str = (const char *)sqlite3_column_text(stmt, 1);
    if (name_str) {
        strncpy(user->name, name_str, AUTH_NAME_MAX - 1);
        user->name[AUTH_NAME_MAX - 1] = '\0';
    }

    const char *uname_str = (const char *)sqlite3_column_text(stmt, 2);
    if (uname_str) {
        strncpy(user->username, uname_str, AUTH_NAME_MAX - 1);
        user->username[AUTH_NAME_MAX - 1] = '\0';
    }

    user->role = (auth_role_t)sqlite3_column_int(stmt, 3);

    const char *hash_str = (const char *)sqlite3_column_text(stmt, 4);
    if (hash_str) {
        strncpy(user->pin_hash, hash_str, sizeof(user->pin_hash) - 1);
        user->pin_hash[sizeof(user->pin_hash) - 1] = '\0';
    }

    user->active = (bool)sqlite3_column_int(stmt, 5);
    user->last_login_ts = (uint32_t)sqlite3_column_int(stmt, 6);
}

/* ── Seed default users (simulator only) ─────────────────── */

#ifdef SIMULATOR_BUILD
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
#endif /* SIMULATOR_BUILD */

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

    /* Migrate: add lockout columns if they don't exist */
    sqlite3_exec(db, "ALTER TABLE users ADD COLUMN failed_attempts INTEGER NOT NULL DEFAULT 0;",
                 NULL, NULL, NULL);
    sqlite3_exec(db, "ALTER TABLE users ADD COLUMN lockout_until_ts INTEGER NOT NULL DEFAULT 0;",
                 NULL, NULL, NULL);

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
        "SELECT id, name, username, role, '', active, last_login_ts "
        "FROM users ORDER BY id");

    ok = ok && prepare(&stmt_update_login,
        "UPDATE users SET last_login_ts = ?1 WHERE id = ?2");

    ok = ok && prepare(&stmt_count_users,
        "SELECT COUNT(*) FROM users");

    ok = ok && prepare(&stmt_get_lockout,
        "SELECT failed_attempts, lockout_until_ts FROM users WHERE username = ?1 AND active = 1");
    ok = ok && prepare(&stmt_set_lockout,
        "UPDATE users SET failed_attempts = ?1, lockout_until_ts = ?2 WHERE username = ?3");

    if (!ok) {
        fprintf(stderr, "[auth_manager] Statement preparation failed\n");
        auth_manager_close();
        return false;
    }

    /* Initialize session */
    memset(&session, 0, sizeof(session));
    session.timeout_s = AUTH_SESSION_TIMEOUT_DEFAULT_S;
    touch_pending = false;

    /* Seed default users if table is empty */
#ifdef SIMULATOR_BUILD
    seed_default_users();
#else
    /* Production: require first-time setup via admin console */
    printf("[auth_manager] Production mode: no default users seeded\n");
#endif

    /* Reset lockout state for all users (clean slate on restart) */
    sqlite3_exec(db, "UPDATE users SET failed_attempts = 0, lockout_until_ts = 0;",
                 NULL, NULL, NULL);

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
    finalize_stmt(&stmt_get_lockout);
    finalize_stmt(&stmt_set_lockout);

    if (db) {
        sqlite3_close(db);
        db = NULL;
        printf("[auth_manager] Closed\n");
    }
}

/* ── Authentication ──────────────────────────────────────── */

bool auth_manager_login(const char *username, const char *pin) {
    if (!db || !stmt_login || !username || !pin) return false;

    /* Query per-user lockout state from DB */
    uint32_t user_failed = 0;
    uint32_t user_lockout_ts = 0;
    sqlite3_reset(stmt_get_lockout);
    sqlite3_bind_text(stmt_get_lockout, 1, username, -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt_get_lockout) == SQLITE_ROW) {
        user_failed    = (uint32_t)sqlite3_column_int(stmt_get_lockout, 0);
        user_lockout_ts = (uint32_t)sqlite3_column_int(stmt_get_lockout, 1);
    }

    /* Check per-user lockout */
    uint32_t now_s = (uint32_t)time(NULL);
    if (user_failed >= AUTH_MAX_FAILED_ATTEMPTS && now_s < user_lockout_ts) {
        printf("[auth_manager] Account locked out for %u more seconds\n",
               user_lockout_ts - now_s);
        return false;
    }

    /* Hash the provided PIN */
    char pin_hex[65];
    hash_pin(pin, pin_hex, sizeof(pin_hex));

    /* Look up user by username (active only) */
    sqlite3_reset(stmt_login);
    sqlite3_bind_text(stmt_login, 1, username, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt_login) != SQLITE_ROW) {
        printf("[auth_manager] Authentication failed\n");
        /* Update per-user lockout in DB (if user exists but inactive, no-op) */
        user_failed++;
        if (user_failed >= AUTH_MAX_FAILED_ATTEMPTS) {
            user_lockout_ts = (uint32_t)time(NULL) + AUTH_LOCKOUT_DURATION_S;
            printf("[auth_manager] Too many failed attempts, locked for %d seconds\n",
                   AUTH_LOCKOUT_DURATION_S);
        }
        sqlite3_reset(stmt_set_lockout);
        sqlite3_bind_int(stmt_set_lockout, 1, (int)user_failed);
        sqlite3_bind_int(stmt_set_lockout, 2, (int)user_lockout_ts);
        sqlite3_bind_text(stmt_set_lockout, 3, username, -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt_set_lockout);
        return false;
    }

    /* Read user record from query result */
    auth_user_t user;
    read_user_row(stmt_login, &user);

    /* Verify PIN hash — constant-time comparison to prevent timing attacks */
    size_t pin_hex_len = strlen(pin_hex);
    size_t stored_len  = strlen(user.pin_hash);
    if (pin_hex_len != stored_len ||
        constant_time_cmp(pin_hex, user.pin_hash, pin_hex_len) != 0) {
        printf("[auth_manager] Authentication failed\n");
        user_failed++;
        if (user_failed >= AUTH_MAX_FAILED_ATTEMPTS) {
            user_lockout_ts = (uint32_t)time(NULL) + AUTH_LOCKOUT_DURATION_S;
            printf("[auth_manager] Too many failed attempts, locked for %d seconds\n",
                   AUTH_LOCKOUT_DURATION_S);
        }
        sqlite3_reset(stmt_set_lockout);
        sqlite3_bind_int(stmt_set_lockout, 1, (int)user_failed);
        sqlite3_bind_int(stmt_set_lockout, 2, (int)user_lockout_ts);
        sqlite3_bind_text(stmt_set_lockout, 3, username, -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt_set_lockout);
        return false;
    }

    /* Login successful — reset per-user lockout state in DB */
    sqlite3_reset(stmt_set_lockout);
    sqlite3_bind_int(stmt_set_lockout, 1, 0);
    sqlite3_bind_int(stmt_set_lockout, 2, 0);
    sqlite3_bind_text(stmt_set_lockout, 3, username, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt_set_lockout);

    /* Establish session */
    session.logged_in = true;
    session.user = user;
    session.login_time_s = 0;        /* Initialized on first check_timeout call */
    session.last_activity_s = 0;
    touch_pending = true;            /* Treat login as activity */

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
    touch_pending = false;
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
    touch_pending = true;
}

bool auth_manager_check_timeout(uint32_t current_time_s) {
    if (!session.logged_in) return false;

    /* First call after login — initialize timestamps */
    if (session.login_time_s == 0) {
        session.login_time_s = current_time_s;
        session.last_activity_s = current_time_s;
        touch_pending = false;
        return false;
    }

    /* Consume pending touch — update last_activity to current time */
    if (touch_pending) {
        session.last_activity_s = current_time_s;
        touch_pending = false;
    }

    /* Check if timed out — use signed arithmetic for backward-clock safety */
    int64_t elapsed = (int64_t)current_time_s - (int64_t)session.last_activity_s;
    if (elapsed < 0) {
        /* Clock went backward (NTP correction) - update baseline, don't timeout */
        session.last_activity_s = current_time_s;
        return false;
    }
    if ((uint32_t)elapsed >= session.timeout_s) {
        printf("[auth_manager] Session timed out after %u s (limit=%u s)\n",
               (uint32_t)elapsed, session.timeout_s);
        auth_manager_logout();
        return true;
    }

    return false;
}

void auth_manager_set_timeout(uint32_t timeout_s) {
    if (timeout_s < 60) {
        timeout_s = 60;
        printf("[auth_manager] Timeout clamped to minimum 60s\n");
    }
    session.timeout_s = timeout_s;
    printf("[auth_manager] Timeout set to %u s\n", timeout_s);
}

/* ── Permission checks ───────────────────────────────────── */

bool auth_manager_has_permission(auth_permission_t perm) {
    if (!session.logged_in) {
        /* Not logged in: only VIEW_VITALS allowed */
        return perm == AUTH_PERM_VIEW_VITALS;
    }
    return auth_manager_role_has_permission(session.user.role, perm);
}

bool auth_manager_role_has_permission(auth_role_t role, auth_permission_t perm) {
    if ((int)role < 0 || (int)role >= AUTH_ROLE_COUNT) return false;
    if ((int)perm < 0 || (int)perm >= AUTH_PERM_COUNT) return false;
    return perm_matrix[role][perm];
}

const char *auth_manager_role_name(auth_role_t role) {
    if (role >= 0 && role < AUTH_ROLE_COUNT) {
        return role_names[role];
    }
    return "Unknown";
}

/* ── User management ─────────────────────────────────────── */

/**
 * Validate PIN strength: minimum 4 digits, reject all-same-digit PINs.
 */
static bool validate_pin(const char *pin) {
    if (!pin) return false;
    size_t len = strlen(pin);
    if (len < 4 || len > AUTH_PIN_MAX_LEN) return false;

    /* Check all digits */
    for (size_t i = 0; i < len; i++) {
        if (pin[i] < '0' || pin[i] > '9') return false;
    }

    /* Reject all-same-digit PINs (e.g., "0000", "1111") */
    bool all_same = true;
    for (size_t i = 1; i < len; i++) {
        if (pin[i] != pin[0]) { all_same = false; break; }
    }
    if (all_same) return false;

    return true;
}

bool auth_manager_add_user(const char *name, const char *username,
                           const char *pin, auth_role_t role) {
    if (!db || !stmt_insert_user) return false;
    if (!name || !username || !pin) return false;
    if (role <= AUTH_ROLE_NONE || role >= AUTH_ROLE_COUNT) return false;

    if (!validate_pin(pin)) {
        fprintf(stderr, "[auth_manager] add_user: PIN does not meet complexity requirements\n");
        return false;
    }

    /* Only ADMIN can add users */
    if (session.logged_in && !auth_manager_has_permission(AUTH_PERM_MANAGE_USERS)) {
        fprintf(stderr, "[auth_manager] add_user: insufficient permissions\n");
        return false;
    }

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

    /* Only ADMIN can delete users */
    if (session.logged_in && !auth_manager_has_permission(AUTH_PERM_MANAGE_USERS)) {
        fprintf(stderr, "[auth_manager] delete_user: insufficient permissions\n");
        return false;
    }

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

    if (!validate_pin(new_pin)) {
        fprintf(stderr, "[auth_manager] change_pin: PIN does not meet complexity requirements\n");
        return false;
    }

    /* Only ADMIN can change PINs (or user changing own PIN) */
    if (session.logged_in && !auth_manager_has_permission(AUTH_PERM_MANAGE_USERS)) {
        /* Allow users to change their own PIN */
        if (session.user.id != user_id) {
            fprintf(stderr, "[auth_manager] change_pin: insufficient permissions\n");
            return false;
        }
    }

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
        read_user_row(stmt_list_users, &out[i]);
        i++;
    }

    return i;
}
