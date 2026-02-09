/**
 * @file settings_store.h
 * @brief SQLite-backed persistent settings key-value store
 *
 * Stores typed settings (int, float, bool, string) as TEXT in a single
 * SQLite table with INSERT OR REPLACE semantics. All access is
 * single-threaded from the LVGL main loop. No LVGL dependency.
 *
 * String getter returns a static buffer — only one string value is valid
 * at a time. Copy the result if you need to call settings_get_string again.
 */

#ifndef SETTINGS_STORE_H
#define SETTINGS_STORE_H

#include <stdbool.h>

/* ── Settings categories ─────────────────────────────────── */

typedef enum {
    SETTINGS_CAT_DISPLAY = 0,
    SETTINGS_CAT_AUDIO,
    SETTINGS_CAT_ALARMS,
    SETTINGS_CAT_NETWORK,
    SETTINGS_CAT_SYSTEM,
    SETTINGS_CAT_COUNT
} settings_category_t;

/* ── Value types ─────────────────────────────────────────── */

typedef enum {
    SETTINGS_TYPE_INT = 0,
    SETTINGS_TYPE_FLOAT,
    SETTINGS_TYPE_BOOL,
    SETTINGS_TYPE_STRING
} settings_type_t;

/* ── Limits ──────────────────────────────────────────────── */

#define SETTINGS_KEY_MAX      48
#define SETTINGS_STRING_MAX   128

/* ── Pre-defined setting keys ────────────────────────────── */

#define SETTINGS_KEY_BRIGHTNESS        "display.brightness"
#define SETTINGS_KEY_AUTO_DIM          "display.auto_dim"
#define SETTINGS_KEY_SCREEN_LOCK_S     "display.screen_lock_timeout"
#define SETTINGS_KEY_COLOR_SCHEME      "display.color_scheme"
#define SETTINGS_KEY_WAVEFORM_SPEED    "display.waveform_speed"
#define SETTINGS_KEY_ALARM_VOLUME      "audio.alarm_volume"
#define SETTINGS_KEY_KEY_CLICK         "audio.key_click"
#define SETTINGS_KEY_ALARM_MUTE        "audio.alarm_mute"
#define SETTINGS_KEY_WIFI_SSID         "network.wifi_ssid"
#define SETTINGS_KEY_WIFI_ENABLED      "network.wifi_enabled"
#define SETTINGS_KEY_FHIR_ENDPOINT     "network.fhir_endpoint"
#define SETTINGS_KEY_DEVICE_NAME       "system.device_name"
#define SETTINGS_KEY_SESSION_TIMEOUT   "system.session_timeout"
#define SETTINGS_KEY_NIBP_INTERVAL     "alarms.nibp_interval"

/* ── Lifecycle ───────────────────────────────────────────── */

/** Open (or create) the settings database. Pass NULL for in-memory DB.
 *  Loads defaults on first run (empty table). */
bool settings_store_init(const char *db_path);

/** Close the database and finalize all prepared statements. */
void settings_store_close(void);

/* ── Typed getters (return default_val if key missing) ──── */

int         settings_get_int(const char *key, int default_val);
float       settings_get_float(const char *key, float default_val);
bool        settings_get_bool(const char *key, bool default_val);

/**
 * Returns a pointer to a static buffer (SETTINGS_STRING_MAX bytes).
 * Only ONE string result is valid at a time — copy if you need to
 * call settings_get_string again before using the previous result.
 */
const char *settings_get_string(const char *key, const char *default_val);

/* ── Typed setters (insert or update) ────────────────────── */

bool settings_set_int(const char *key, int value);
bool settings_set_float(const char *key, float value);
bool settings_set_bool(const char *key, bool value);
bool settings_set_string(const char *key, const char *value);

/* ── Batch operations ────────────────────────────────────── */

/** Load all default settings (only inserts keys that don't already exist). */
void settings_load_defaults(void);

/** Drop all settings and reload defaults. Returns true on success. */
bool settings_reset_to_defaults(void);

/** Check whether a key exists in the store. */
bool settings_exists(const char *key);

#endif /* SETTINGS_STORE_H */
