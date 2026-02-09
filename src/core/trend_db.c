/**
 * @file trend_db.c
 * @brief SQLite-backed trend storage implementation
 *
 * All database access is single-threaded (LVGL main loop).
 * Uses pre-compiled prepared statements for performance.
 * Static result buffers avoid heap allocation in query paths.
 */

#include "trend_db.h"
#include "sqlite3.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ── Retention limits (seconds) ─────────────────────────── */

#define RAW_RETAIN_S     (4 * 3600)    /* 4 hours for raw 1-sec data */
#define AGG_RETAIN_S     (72 * 3600)   /* 72 hours for 1-min aggregates */

/* ── Module state ────────────────────────────────────────── */

static sqlite3 *db = NULL;

/* Prepared statements */
static sqlite3_stmt *stmt_insert_raw    = NULL;
static sqlite3_stmt *stmt_insert_1min   = NULL;
static sqlite3_stmt *stmt_insert_nibp   = NULL;
static sqlite3_stmt *stmt_insert_alarm  = NULL;
static sqlite3_stmt *stmt_query_raw     = NULL;
static sqlite3_stmt *stmt_query_1min    = NULL;
static sqlite3_stmt *stmt_query_nibp    = NULL;
static sqlite3_stmt *stmt_query_alarm   = NULL;
static sqlite3_stmt *stmt_purge_raw     = NULL;
static sqlite3_stmt *stmt_purge_1min    = NULL;
static sqlite3_stmt *stmt_purge_nibp    = NULL;
static sqlite3_stmt *stmt_purge_alarm   = NULL;

/* Aggregation helper */
static sqlite3_stmt *stmt_agg_select    = NULL;

/* ── Schema creation ─────────────────────────────────────── */

static const char *SCHEMA_SQL =
    "CREATE TABLE IF NOT EXISTS vitals_raw ("
    "  timestamp_s INTEGER PRIMARY KEY,"
    "  hr INTEGER NOT NULL,"
    "  spo2 INTEGER NOT NULL,"
    "  rr INTEGER NOT NULL,"
    "  temp_x10 INTEGER NOT NULL"
    ");"
    "CREATE TABLE IF NOT EXISTS vitals_1min ("
    "  minute_ts INTEGER PRIMARY KEY,"
    "  hr_avg INTEGER, hr_min INTEGER, hr_max INTEGER,"
    "  spo2_avg INTEGER, spo2_min INTEGER, spo2_max INTEGER,"
    "  rr_avg INTEGER, rr_min INTEGER, rr_max INTEGER,"
    "  temp_avg_x10 INTEGER, temp_min_x10 INTEGER, temp_max_x10 INTEGER"
    ");"
    "CREATE TABLE IF NOT EXISTS nibp_measurements ("
    "  timestamp_s INTEGER PRIMARY KEY,"
    "  sys INTEGER NOT NULL,"
    "  dia INTEGER NOT NULL,"
    "  map_val INTEGER NOT NULL"
    ");"
    "CREATE TABLE IF NOT EXISTS alarm_events ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  timestamp_s INTEGER NOT NULL,"
    "  severity INTEGER NOT NULL,"
    "  message TEXT NOT NULL"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_alarm_ts ON alarm_events(timestamp_s);";

/* ── Helper: prepare a single statement ──────────────────── */

static bool prepare(sqlite3_stmt **out, const char *sql) {
    int rc = sqlite3_prepare_v2(db, sql, -1, out, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[trend_db] prepare failed: %s\n  SQL: %s\n",
                sqlite3_errmsg(db), sql);
        return false;
    }
    return true;
}

/* ── Lifecycle ───────────────────────────────────────────── */

bool trend_db_init(const char *db_path) {
    const char *path = db_path ? db_path : ":memory:";
    int rc = sqlite3_open(path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[trend_db] Failed to open DB '%s': %s\n",
                path, sqlite3_errmsg(db));
        db = NULL;
        return false;
    }

    /* Performance pragmas */
    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);
    sqlite3_exec(db, "PRAGMA synchronous=NORMAL;", NULL, NULL, NULL);
    sqlite3_exec(db, "PRAGMA cache_size=200;", NULL, NULL, NULL);

    /* Create tables */
    char *err_msg = NULL;
    rc = sqlite3_exec(db, SCHEMA_SQL, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[trend_db] Schema creation failed: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        db = NULL;
        return false;
    }

    /* Prepare all statements */
    bool ok = true;
    ok = ok && prepare(&stmt_insert_raw,
        "INSERT OR REPLACE INTO vitals_raw (timestamp_s, hr, spo2, rr, temp_x10) "
        "VALUES (?1, ?2, ?3, ?4, ?5)");

    ok = ok && prepare(&stmt_insert_1min,
        "INSERT OR REPLACE INTO vitals_1min "
        "(minute_ts, hr_avg, hr_min, hr_max, spo2_avg, spo2_min, spo2_max, "
        "rr_avg, rr_min, rr_max, temp_avg_x10, temp_min_x10, temp_max_x10) "
        "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13)");

    ok = ok && prepare(&stmt_insert_nibp,
        "INSERT OR REPLACE INTO nibp_measurements (timestamp_s, sys, dia, map_val) "
        "VALUES (?1, ?2, ?3, ?4)");

    ok = ok && prepare(&stmt_insert_alarm,
        "INSERT INTO alarm_events (timestamp_s, severity, message) "
        "VALUES (?1, ?2, ?3)");

    ok = ok && prepare(&stmt_agg_select,
        "SELECT AVG(hr), MIN(hr), MAX(hr), "
        "AVG(spo2), MIN(spo2), MAX(spo2), "
        "AVG(rr), MIN(rr), MAX(rr), "
        "AVG(temp_x10), MIN(temp_x10), MAX(temp_x10) "
        "FROM vitals_raw WHERE timestamp_s > ?1 AND timestamp_s <= ?2");

    /* Query statements use dynamic SQL via sqlite3_exec, not prepared.
     * But for the common raw/1min queries we prepare templates. */
    ok = ok && prepare(&stmt_query_nibp,
        "SELECT timestamp_s, sys, dia, map_val FROM nibp_measurements "
        "WHERE timestamp_s >= ?1 AND timestamp_s <= ?2 "
        "ORDER BY timestamp_s LIMIT ?3");

    ok = ok && prepare(&stmt_query_alarm,
        "SELECT timestamp_s, severity, message FROM alarm_events "
        "WHERE timestamp_s >= ?1 AND timestamp_s <= ?2 "
        "ORDER BY timestamp_s LIMIT ?3");

    ok = ok && prepare(&stmt_purge_raw,
        "DELETE FROM vitals_raw WHERE timestamp_s < ?1");
    ok = ok && prepare(&stmt_purge_1min,
        "DELETE FROM vitals_1min WHERE minute_ts < ?1");
    ok = ok && prepare(&stmt_purge_nibp,
        "DELETE FROM nibp_measurements WHERE timestamp_s < ?1");
    ok = ok && prepare(&stmt_purge_alarm,
        "DELETE FROM alarm_events WHERE timestamp_s < ?1");

    if (!ok) {
        fprintf(stderr, "[trend_db] Statement preparation failed\n");
        trend_db_close();
        return false;
    }

    printf("[trend_db] Initialized: %s\n", path);
    return true;
}

static void finalize_stmt(sqlite3_stmt **s) {
    if (*s) {
        sqlite3_finalize(*s);
        *s = NULL;
    }
}

void trend_db_close(void) {
    finalize_stmt(&stmt_insert_raw);
    finalize_stmt(&stmt_insert_1min);
    finalize_stmt(&stmt_insert_nibp);
    finalize_stmt(&stmt_insert_alarm);
    finalize_stmt(&stmt_agg_select);
    finalize_stmt(&stmt_query_raw);
    finalize_stmt(&stmt_query_1min);
    finalize_stmt(&stmt_query_nibp);
    finalize_stmt(&stmt_query_alarm);
    finalize_stmt(&stmt_purge_raw);
    finalize_stmt(&stmt_purge_1min);
    finalize_stmt(&stmt_purge_nibp);
    finalize_stmt(&stmt_purge_alarm);

    if (db) {
        sqlite3_close(db);
        db = NULL;
        printf("[trend_db] Closed\n");
    }
}

/* ── Insertion ───────────────────────────────────────────── */

void trend_db_insert_sample(uint32_t timestamp_s, int hr, int spo2,
                             int rr, float temp) {
    if (!db || !stmt_insert_raw) return;

    sqlite3_reset(stmt_insert_raw);
    sqlite3_bind_int(stmt_insert_raw, 1, (int)timestamp_s);
    sqlite3_bind_int(stmt_insert_raw, 2, hr);
    sqlite3_bind_int(stmt_insert_raw, 3, spo2);
    sqlite3_bind_int(stmt_insert_raw, 4, rr);
    sqlite3_bind_int(stmt_insert_raw, 5, (int)roundf(temp * 10.0f));
    sqlite3_step(stmt_insert_raw);
}

void trend_db_insert_nibp(uint32_t timestamp_s, int sys, int dia,
                           int map_val) {
    if (!db || !stmt_insert_nibp) return;

    sqlite3_reset(stmt_insert_nibp);
    sqlite3_bind_int(stmt_insert_nibp, 1, (int)timestamp_s);
    sqlite3_bind_int(stmt_insert_nibp, 2, sys);
    sqlite3_bind_int(stmt_insert_nibp, 3, dia);
    sqlite3_bind_int(stmt_insert_nibp, 4, map_val);
    sqlite3_step(stmt_insert_nibp);
}

void trend_db_insert_alarm(uint32_t timestamp_s,
                            vm_alarm_severity_t severity,
                            const char *message) {
    if (!db || !stmt_insert_alarm) return;

    sqlite3_reset(stmt_insert_alarm);
    sqlite3_bind_int(stmt_insert_alarm, 1, (int)timestamp_s);
    sqlite3_bind_int(stmt_insert_alarm, 2, (int)severity);
    sqlite3_bind_text(stmt_insert_alarm, 3, message, -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt_insert_alarm);
}

/* ── Aggregation ─────────────────────────────────────────── */

void trend_db_aggregate_minute(uint32_t minute_boundary_ts) {
    if (!db || !stmt_agg_select || !stmt_insert_1min) return;

    uint32_t start = minute_boundary_ts - 59;

    sqlite3_reset(stmt_agg_select);
    sqlite3_bind_int(stmt_agg_select, 1, (int)(start - 1));
    sqlite3_bind_int(stmt_agg_select, 2, (int)minute_boundary_ts);

    if (sqlite3_step(stmt_agg_select) == SQLITE_ROW) {
        /* Check if any data exists (AVG returns NULL if no rows) */
        if (sqlite3_column_type(stmt_agg_select, 0) == SQLITE_NULL) return;

        sqlite3_reset(stmt_insert_1min);
        sqlite3_bind_int(stmt_insert_1min, 1, (int)minute_boundary_ts);
        /* HR: avg, min, max */
        sqlite3_bind_int(stmt_insert_1min, 2, sqlite3_column_int(stmt_agg_select, 0));
        sqlite3_bind_int(stmt_insert_1min, 3, sqlite3_column_int(stmt_agg_select, 1));
        sqlite3_bind_int(stmt_insert_1min, 4, sqlite3_column_int(stmt_agg_select, 2));
        /* SpO2: avg, min, max */
        sqlite3_bind_int(stmt_insert_1min, 5, sqlite3_column_int(stmt_agg_select, 3));
        sqlite3_bind_int(stmt_insert_1min, 6, sqlite3_column_int(stmt_agg_select, 4));
        sqlite3_bind_int(stmt_insert_1min, 7, sqlite3_column_int(stmt_agg_select, 5));
        /* RR: avg, min, max */
        sqlite3_bind_int(stmt_insert_1min, 8, sqlite3_column_int(stmt_agg_select, 6));
        sqlite3_bind_int(stmt_insert_1min, 9, sqlite3_column_int(stmt_agg_select, 7));
        sqlite3_bind_int(stmt_insert_1min, 10, sqlite3_column_int(stmt_agg_select, 8));
        /* Temp: avg, min, max */
        sqlite3_bind_int(stmt_insert_1min, 11, sqlite3_column_int(stmt_agg_select, 9));
        sqlite3_bind_int(stmt_insert_1min, 12, sqlite3_column_int(stmt_agg_select, 10));
        sqlite3_bind_int(stmt_insert_1min, 13, sqlite3_column_int(stmt_agg_select, 11));
        sqlite3_step(stmt_insert_1min);
    }
}

/* ── Queries ─────────────────────────────────────────────── */

/**
 * Map a trend_param_t to column names in the raw and 1min tables.
 * Returns the column prefix string.
 */
static const char *param_col_raw(trend_param_t p) {
    switch (p) {
        case TREND_PARAM_HR:   return "hr";
        case TREND_PARAM_SPO2: return "spo2";
        case TREND_PARAM_RR:   return "rr";
        case TREND_PARAM_TEMP: return "temp_x10";
        default: return "hr";
    }
}

int trend_db_query_param(trend_param_t param, uint32_t start_ts,
                          uint32_t end_ts, int max_points,
                          trend_query_result_t *result) {
    if (!db || !result) return 0;
    result->count = 0;

    if (max_points > TREND_DB_MAX_POINTS) max_points = TREND_DB_MAX_POINTS;

    uint32_t range = end_ts - start_ts;
    const char *col = param_col_raw(param);

    char sql[512];

    if (range <= 7200) {
        /* Short range (≤2h): query vitals_raw with GROUP BY for downsampling */
        int group_interval = (int)range / max_points;
        if (group_interval < 1) group_interval = 1;

        snprintf(sql, sizeof(sql),
            "SELECT (timestamp_s / %d) * %d AS ts, "
            "AVG(%s) AS val, MIN(%s) AS vmin, MAX(%s) AS vmax "
            "FROM vitals_raw "
            "WHERE timestamp_s >= %u AND timestamp_s <= %u "
            "GROUP BY ts ORDER BY ts LIMIT %d",
            group_interval, group_interval,
            col, col, col,
            (unsigned)start_ts, (unsigned)end_ts, max_points);
    } else {
        /* Long range (>2h): query vitals_1min with GROUP BY */
        int group_interval = (int)(range / 60) / max_points;
        if (group_interval < 1) group_interval = 1;

        /* Column names in vitals_1min differ for temp */
        const char *avg_col, *min_col, *max_col;
        if (param == TREND_PARAM_TEMP) {
            avg_col = "temp_avg_x10";
            min_col = "temp_min_x10";
            max_col = "temp_max_x10";
        } else {
            static char abuf[32], nbuf[32], xbuf[32];
            snprintf(abuf, sizeof(abuf), "%s_avg", col);
            snprintf(nbuf, sizeof(nbuf), "%s_min", col);
            snprintf(xbuf, sizeof(xbuf), "%s_max", col);
            avg_col = abuf;
            min_col = nbuf;
            max_col = xbuf;
        }

        snprintf(sql, sizeof(sql),
            "SELECT (minute_ts / %d) * %d AS ts, "
            "AVG(%s) AS val, MIN(%s) AS vmin, MAX(%s) AS vmax "
            "FROM vitals_1min "
            "WHERE minute_ts >= %u AND minute_ts <= %u "
            "GROUP BY ts ORDER BY ts LIMIT %d",
            group_interval * 60, group_interval * 60,
            avg_col, min_col, max_col,
            (unsigned)start_ts, (unsigned)end_ts, max_points);
    }

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "[trend_db] query_param prepare failed: %s\n",
                sqlite3_errmsg(db));
        return 0;
    }

    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < max_points) {
        result->timestamp_s[i] = (uint32_t)sqlite3_column_int(stmt, 0);
        result->value[i]       = sqlite3_column_int(stmt, 1);
        result->value_min[i]   = sqlite3_column_int(stmt, 2);
        result->value_max[i]   = sqlite3_column_int(stmt, 3);
        i++;
    }
    result->count = i;

    sqlite3_finalize(stmt);
    return i;
}

int trend_db_query_nibp(uint32_t start_ts, uint32_t end_ts,
                         trend_nibp_result_t *result) {
    if (!db || !stmt_query_nibp || !result) return 0;
    result->count = 0;

    sqlite3_reset(stmt_query_nibp);
    sqlite3_bind_int(stmt_query_nibp, 1, (int)start_ts);
    sqlite3_bind_int(stmt_query_nibp, 2, (int)end_ts);
    sqlite3_bind_int(stmt_query_nibp, 3, TREND_DB_MAX_POINTS);

    int i = 0;
    while (sqlite3_step(stmt_query_nibp) == SQLITE_ROW && i < TREND_DB_MAX_POINTS) {
        result->timestamp_s[i] = (uint32_t)sqlite3_column_int(stmt_query_nibp, 0);
        result->sys[i]         = sqlite3_column_int(stmt_query_nibp, 1);
        result->dia[i]         = sqlite3_column_int(stmt_query_nibp, 2);
        result->map_val[i]     = sqlite3_column_int(stmt_query_nibp, 3);
        i++;
    }
    result->count = i;
    return i;
}

int trend_db_query_alarms(uint32_t start_ts, uint32_t end_ts,
                           trend_alarm_result_t *result) {
    if (!db || !stmt_query_alarm || !result) return 0;
    result->count = 0;

    sqlite3_reset(stmt_query_alarm);
    sqlite3_bind_int(stmt_query_alarm, 1, (int)start_ts);
    sqlite3_bind_int(stmt_query_alarm, 2, (int)end_ts);
    sqlite3_bind_int(stmt_query_alarm, 3, TREND_DB_MAX_POINTS);

    int i = 0;
    while (sqlite3_step(stmt_query_alarm) == SQLITE_ROW && i < TREND_DB_MAX_POINTS) {
        result->timestamp_s[i] = (uint32_t)sqlite3_column_int(stmt_query_alarm, 0);
        result->severity[i]    = sqlite3_column_int(stmt_query_alarm, 1);
        const char *msg = (const char *)sqlite3_column_text(stmt_query_alarm, 2);
        if (msg) {
            strncpy(result->message[i], msg, 47);
            result->message[i][47] = '\0';
        } else {
            result->message[i][0] = '\0';
        }
        i++;
    }
    result->count = i;
    return i;
}

/* ── Maintenance ─────────────────────────────────────────── */

void trend_db_purge_old(uint32_t current_ts) {
    if (!db) return;

    uint32_t raw_cutoff = (current_ts > RAW_RETAIN_S) ? current_ts - RAW_RETAIN_S : 0;
    uint32_t agg_cutoff = (current_ts > AGG_RETAIN_S) ? current_ts - AGG_RETAIN_S : 0;

    sqlite3_reset(stmt_purge_raw);
    sqlite3_bind_int(stmt_purge_raw, 1, (int)raw_cutoff);
    sqlite3_step(stmt_purge_raw);

    sqlite3_reset(stmt_purge_1min);
    sqlite3_bind_int(stmt_purge_1min, 1, (int)agg_cutoff);
    sqlite3_step(stmt_purge_1min);

    sqlite3_reset(stmt_purge_nibp);
    sqlite3_bind_int(stmt_purge_nibp, 1, (int)agg_cutoff);
    sqlite3_step(stmt_purge_nibp);

    sqlite3_reset(stmt_purge_alarm);
    sqlite3_bind_int(stmt_purge_alarm, 1, (int)agg_cutoff);
    sqlite3_step(stmt_purge_alarm);
}
