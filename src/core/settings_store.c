/**
 * @file settings_store.c
 * @brief SQLite-backed persistent settings key-value store implementation
 *
 * All database access is single-threaded (LVGL main loop).
 * Uses prepared statements for get/set performance.
 * Values are stored as TEXT and converted on read.
 * Static buffers avoid heap allocation.
 */

#include "settings_store.h"
#include "sqlite3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Module state ────────────────────────────────────────── */

static sqlite3 *db = NULL;

/* Prepared statements */
static sqlite3_stmt *stmt_get     = NULL;
static sqlite3_stmt *stmt_set     = NULL;
static sqlite3_stmt *stmt_exists  = NULL;
static sqlite3_stmt *stmt_delete  = NULL;

/* Static buffer for string getter (one at a time) */
static char string_buf[SETTINGS_STRING_MAX];

/* ── Schema ──────────────────────────────────────────────── */

static const char *SCHEMA_SQL =
    "CREATE TABLE IF NOT EXISTS settings ("
    "  key TEXT PRIMARY KEY,"
    "  value TEXT NOT NULL,"
    "  type INTEGER NOT NULL"
    ");";

/* ── Default settings table ──────────────────────────────── */

typedef struct {
    const char      *key;
    settings_type_t  type;
    const char      *value;     /* stored as text */
} settings_default_t;

static const settings_default_t defaults[] = {
    { SETTINGS_KEY_BRIGHTNESS,      SETTINGS_TYPE_INT,    "70"              },
    { SETTINGS_KEY_AUTO_DIM,        SETTINGS_TYPE_BOOL,   "1"               },
    { SETTINGS_KEY_SCREEN_LOCK_S,   SETTINGS_TYPE_INT,    "120"             },
    { SETTINGS_KEY_COLOR_SCHEME,    SETTINGS_TYPE_STRING, "dark"            },
    { SETTINGS_KEY_WAVEFORM_SPEED,  SETTINGS_TYPE_INT,    "25"              },
    { SETTINGS_KEY_ALARM_VOLUME,    SETTINGS_TYPE_INT,    "80"              },
    { SETTINGS_KEY_KEY_CLICK,       SETTINGS_TYPE_BOOL,   "1"               },
    { SETTINGS_KEY_ALARM_MUTE,      SETTINGS_TYPE_BOOL,   "0"               },
    { SETTINGS_KEY_WIFI_SSID,       SETTINGS_TYPE_STRING, ""                },
    { SETTINGS_KEY_WIFI_ENABLED,    SETTINGS_TYPE_BOOL,   "0"               },
    { SETTINGS_KEY_FHIR_ENDPOINT,   SETTINGS_TYPE_STRING, ""                },
    { SETTINGS_KEY_DEVICE_NAME,     SETTINGS_TYPE_STRING, "VM-800 Bedside"  },
    { SETTINGS_KEY_SESSION_TIMEOUT, SETTINGS_TYPE_INT,    "300"             },
    { SETTINGS_KEY_NIBP_INTERVAL,   SETTINGS_TYPE_INT,    "60"              },
};

#define DEFAULTS_COUNT  (sizeof(defaults) / sizeof(defaults[0]))

/* ── Helper: prepare a single statement ──────────────────── */

static bool prepare(sqlite3_stmt **out, const char *sql) {
    int rc = sqlite3_prepare_v2(db, sql, -1, out, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[settings] prepare failed: %s\n  SQL: %s\n",
                sqlite3_errmsg(db), sql);
        return false;
    }
    return true;
}

/* ── Helper: finalize a prepared statement ───────────────── */

static void finalize_stmt(sqlite3_stmt **s) {
    if (*s) {
        sqlite3_finalize(*s);
        *s = NULL;
    }
}

/* ── Lifecycle ───────────────────────────────────────────── */

bool settings_store_init(const char *db_path) {
    const char *path = db_path ? db_path : ":memory:";
    int rc = sqlite3_open(path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[settings] Failed to open DB '%s': %s\n",
                path, sqlite3_errmsg(db));
        db = NULL;
        return false;
    }

    /* Performance pragmas */
    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);
    sqlite3_exec(db, "PRAGMA synchronous=NORMAL;", NULL, NULL, NULL);

    /* Create table */
    char *err_msg = NULL;
    rc = sqlite3_exec(db, SCHEMA_SQL, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[settings] Schema creation failed: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        db = NULL;
        return false;
    }

    /* Prepare statements */
    bool ok = true;
    ok = ok && prepare(&stmt_get,
        "SELECT value, type FROM settings WHERE key = ?1");
    ok = ok && prepare(&stmt_set,
        "INSERT OR REPLACE INTO settings (key, value, type) "
        "VALUES (?1, ?2, ?3)");
    ok = ok && prepare(&stmt_exists,
        "SELECT 1 FROM settings WHERE key = ?1 LIMIT 1");
    ok = ok && prepare(&stmt_delete,
        "DELETE FROM settings");

    if (!ok) {
        fprintf(stderr, "[settings] Statement preparation failed\n");
        settings_store_close();
        return false;
    }

    /* Load defaults if table is empty */
    settings_load_defaults();

    printf("[settings] Initialized: %s\n", path);
    return true;
}

void settings_store_close(void) {
    finalize_stmt(&stmt_get);
    finalize_stmt(&stmt_set);
    finalize_stmt(&stmt_exists);
    finalize_stmt(&stmt_delete);

    if (db) {
        sqlite3_close(db);
        db = NULL;
        printf("[settings] Closed\n");
    }
}

/* ── Internal: raw get (returns value text, caller interprets) ── */

/**
 * Fetch the raw TEXT value for a key.  Returns true if found,
 * and copies the value into the static string_buf.
 */
static bool raw_get(const char *key) {
    if (!db || !stmt_get || !key) return false;

    sqlite3_reset(stmt_get);
    sqlite3_bind_text(stmt_get, 1, key, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt_get) == SQLITE_ROW) {
        const char *val = (const char *)sqlite3_column_text(stmt_get, 0);
        if (val) {
            strncpy(string_buf, val, SETTINGS_STRING_MAX - 1);
            string_buf[SETTINGS_STRING_MAX - 1] = '\0';
        } else {
            string_buf[0] = '\0';
        }
        return true;
    }
    return false;
}

/* ── Internal: raw set ──────────────────────────────────── */

static bool raw_set(const char *key, const char *value, settings_type_t type) {
    if (!db || !stmt_set || !key || !value) return false;

    sqlite3_reset(stmt_set);
    sqlite3_bind_text(stmt_set, 1, key, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt_set, 2, value, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt_set,  3, (int)type);

    int rc = sqlite3_step(stmt_set);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "[settings] set '%s' failed: %s\n",
                key, sqlite3_errmsg(db));
        return false;
    }
    return true;
}

/* ── Typed getters ───────────────────────────────────────── */

int settings_get_int(const char *key, int default_val) {
    if (!raw_get(key)) return default_val;
    return atoi(string_buf);
}

float settings_get_float(const char *key, float default_val) {
    if (!raw_get(key)) return default_val;
    return (float)atof(string_buf);
}

bool settings_get_bool(const char *key, bool default_val) {
    if (!raw_get(key)) return default_val;
    /* "1" / "true" => true; "0" / "false" / "" => false */
    return (string_buf[0] == '1' || string_buf[0] == 't' || string_buf[0] == 'T');
}

const char *settings_get_string(const char *key, const char *default_val) {
    if (!raw_get(key)) {
        /* Copy default into static buffer so caller always gets a stable pointer */
        if (default_val) {
            strncpy(string_buf, default_val, SETTINGS_STRING_MAX - 1);
            string_buf[SETTINGS_STRING_MAX - 1] = '\0';
        } else {
            string_buf[0] = '\0';
        }
        return string_buf;
    }
    return string_buf;
}

/* ── Typed setters ───────────────────────────────────────── */

bool settings_set_int(const char *key, int value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", value);
    return raw_set(key, buf, SETTINGS_TYPE_INT);
}

bool settings_set_float(const char *key, float value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.6f", value);
    return raw_set(key, buf, SETTINGS_TYPE_FLOAT);
}

bool settings_set_bool(const char *key, bool value) {
    return raw_set(key, value ? "1" : "0", SETTINGS_TYPE_BOOL);
}

bool settings_set_string(const char *key, const char *value) {
    if (!value) value = "";
    return raw_set(key, value, SETTINGS_TYPE_STRING);
}

/* ── Batch operations ────────────────────────────────────── */

void settings_load_defaults(void) {
    if (!db) return;

    for (int i = 0; i < (int)DEFAULTS_COUNT; i++) {
        /* Only insert if key does not already exist */
        if (!settings_exists(defaults[i].key)) {
            raw_set(defaults[i].key, defaults[i].value, defaults[i].type);
            printf("[settings] Default: %s = %s\n",
                   defaults[i].key, defaults[i].value);
        }
    }
}

bool settings_reset_to_defaults(void) {
    if (!db || !stmt_delete) return false;

    /* Delete all rows */
    sqlite3_reset(stmt_delete);
    int rc = sqlite3_step(stmt_delete);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "[settings] reset failed: %s\n", sqlite3_errmsg(db));
        return false;
    }

    printf("[settings] Table cleared, reloading defaults\n");
    settings_load_defaults();
    return true;
}

bool settings_exists(const char *key) {
    if (!db || !stmt_exists || !key) return false;

    sqlite3_reset(stmt_exists);
    sqlite3_bind_text(stmt_exists, 1, key, -1, SQLITE_STATIC);

    return (sqlite3_step(stmt_exists) == SQLITE_ROW);
}
