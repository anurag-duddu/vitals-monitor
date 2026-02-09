/**
 * @file trend_db.h
 * @brief SQLite-backed trend storage for 72-hour vital sign history
 *
 * Two-tier storage:
 *   - vitals_raw: 1-second resolution, retained 4 hours (short-range queries)
 *   - vitals_1min: 1-minute aggregates, retained 72 hours (long-range queries)
 *   - nibp_measurements: discrete NIBP events
 *   - alarm_events: alarm timeline markers
 */

#ifndef TREND_DB_H
#define TREND_DB_H

#include <stdint.h>
#include <stdbool.h>
#include "theme_vitals.h"

/* Maximum data points returned from a single query */
#define TREND_DB_MAX_POINTS  480

/* ── Time range presets (seconds) ────────────────────────── */

typedef enum {
    TREND_RANGE_1H  = 3600,
    TREND_RANGE_4H  = 14400,
    TREND_RANGE_8H  = 28800,
    TREND_RANGE_12H = 43200,
    TREND_RANGE_24H = 86400,
    TREND_RANGE_72H = 259200,
} trend_range_t;

/* ── Vital parameter identifiers ─────────────────────────── */

typedef enum {
    TREND_PARAM_HR = 0,
    TREND_PARAM_SPO2,
    TREND_PARAM_RR,
    TREND_PARAM_TEMP,
    TREND_PARAM_COUNT
} trend_param_t;

/* ── Query result structures (static buffers, no malloc) ─── */

typedef struct {
    uint32_t timestamp_s[TREND_DB_MAX_POINTS];
    int32_t  value[TREND_DB_MAX_POINTS];       /* avg values; temp stored as x10 */
    int32_t  value_min[TREND_DB_MAX_POINTS];
    int32_t  value_max[TREND_DB_MAX_POINTS];
    int      count;
} trend_query_result_t;

typedef struct {
    uint32_t timestamp_s[TREND_DB_MAX_POINTS];
    int      sys[TREND_DB_MAX_POINTS];
    int      dia[TREND_DB_MAX_POINTS];
    int      map_val[TREND_DB_MAX_POINTS];
    int      count;
} trend_nibp_result_t;

typedef struct {
    uint32_t timestamp_s[TREND_DB_MAX_POINTS];
    int      severity[TREND_DB_MAX_POINTS];
    char     message[TREND_DB_MAX_POINTS][48];
    int      count;
} trend_alarm_result_t;

/* ── Lifecycle ───────────────────────────────────────────── */

/** Open (or create) the trend database. Pass NULL for in-memory DB. */
bool trend_db_init(const char *db_path);

/** Close the database and finalize all prepared statements. */
void trend_db_close(void);

/* ── Insertion (called from mock_data timer, main thread) ── */

void trend_db_insert_sample(uint32_t timestamp_s, int hr, int spo2,
                             int rr, float temp);

void trend_db_insert_nibp(uint32_t timestamp_s, int sys, int dia,
                           int map_val);

void trend_db_insert_alarm(uint32_t timestamp_s,
                            vm_alarm_severity_t severity,
                            const char *message);

/* ── Aggregation ─────────────────────────────────────────── */

/** Compute and store 1-minute summary for the minute ending at minute_boundary_ts. */
void trend_db_aggregate_minute(uint32_t minute_boundary_ts);

/* ── Queries (called from trends screen) ─────────────────── */

/** Query a single vital parameter over a time range. Returns point count. */
int trend_db_query_param(trend_param_t param, uint32_t start_ts,
                          uint32_t end_ts, int max_points,
                          trend_query_result_t *result);

/** Query NIBP measurements over a time range. */
int trend_db_query_nibp(uint32_t start_ts, uint32_t end_ts,
                         trend_nibp_result_t *result);

/** Query alarm events over a time range. */
int trend_db_query_alarms(uint32_t start_ts, uint32_t end_ts,
                           trend_alarm_result_t *result);

/* ── Maintenance ─────────────────────────────────────────── */

/** Delete data older than retention limits (raw: 4h, aggregates: 72h). */
void trend_db_purge_old(uint32_t current_ts);

#endif /* TREND_DB_H */
