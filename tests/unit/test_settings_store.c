/**
 * @file test_settings_store.c
 * @brief Unit tests for settings_store module
 *
 * Tests get/set for each type, default loading, reset, key existence,
 * and missing-key fallback behavior using in-memory SQLite.
 */

#include "test_framework.h"
#include "settings_store.h"

/* ── Test: init and close ────────────────────────────────── */

static void test_init_close(void) {
    printf("  test_init_close\n");

    bool ok = settings_store_init(NULL);  /* in-memory */
    ASSERT_TRUE(ok);

    settings_store_close();
}

/* ── Test: defaults are loaded on init ───────────────────── */

static void test_defaults_loaded(void) {
    printf("  test_defaults_loaded\n");

    settings_store_init(NULL);

    /* Verify known default values */
    int brightness = settings_get_int(SETTINGS_KEY_BRIGHTNESS, -1);
    ASSERT_EQ_INT(brightness, 70);

    bool auto_dim = settings_get_bool(SETTINGS_KEY_AUTO_DIM, false);
    ASSERT_TRUE(auto_dim);

    int lock_s = settings_get_int(SETTINGS_KEY_SCREEN_LOCK_S, -1);
    ASSERT_EQ_INT(lock_s, 120);

    const char *scheme = settings_get_string(SETTINGS_KEY_COLOR_SCHEME, "unknown");
    ASSERT_STR_EQ(scheme, "dark");

    int wave_speed = settings_get_int(SETTINGS_KEY_WAVEFORM_SPEED, -1);
    ASSERT_EQ_INT(wave_speed, 25);

    int alarm_vol = settings_get_int(SETTINGS_KEY_ALARM_VOLUME, -1);
    ASSERT_EQ_INT(alarm_vol, 80);

    bool key_click = settings_get_bool(SETTINGS_KEY_KEY_CLICK, false);
    ASSERT_TRUE(key_click);

    bool mute = settings_get_bool(SETTINGS_KEY_ALARM_MUTE, true);
    ASSERT_FALSE(mute);

    bool wifi = settings_get_bool(SETTINGS_KEY_WIFI_ENABLED, true);
    ASSERT_FALSE(wifi);

    const char *device = settings_get_string(SETTINGS_KEY_DEVICE_NAME, "");
    ASSERT_STR_EQ(device, "VM-800 Bedside");

    int session_to = settings_get_int(SETTINGS_KEY_SESSION_TIMEOUT, -1);
    ASSERT_EQ_INT(session_to, 300);

    int nibp = settings_get_int(SETTINGS_KEY_NIBP_INTERVAL, -1);
    ASSERT_EQ_INT(nibp, 60);

    settings_store_close();
}

/* ── Test: get/set int ───────────────────────────────────── */

static void test_get_set_int(void) {
    printf("  test_get_set_int\n");

    settings_store_init(NULL);

    bool ok = settings_set_int(SETTINGS_KEY_BRIGHTNESS, 42);
    ASSERT_TRUE(ok);

    int val = settings_get_int(SETTINGS_KEY_BRIGHTNESS, -1);
    ASSERT_EQ_INT(val, 42);

    /* Overwrite */
    settings_set_int(SETTINGS_KEY_BRIGHTNESS, 100);
    val = settings_get_int(SETTINGS_KEY_BRIGHTNESS, -1);
    ASSERT_EQ_INT(val, 100);

    settings_store_close();
}

/* ── Test: get/set float ─────────────────────────────────── */

static void test_get_set_float(void) {
    printf("  test_get_set_float\n");

    settings_store_init(NULL);

    bool ok = settings_set_float("test.float_key", 3.14159f);
    ASSERT_TRUE(ok);

    float val = settings_get_float("test.float_key", 0.0f);
    ASSERT_FLOAT_NEAR(val, 3.14159f, 0.001);

    settings_store_close();
}

/* ── Test: get/set bool ──────────────────────────────────── */

static void test_get_set_bool(void) {
    printf("  test_get_set_bool\n");

    settings_store_init(NULL);

    /* Set true */
    bool ok = settings_set_bool("test.bool_key", true);
    ASSERT_TRUE(ok);
    bool val = settings_get_bool("test.bool_key", false);
    ASSERT_TRUE(val);

    /* Set false */
    ok = settings_set_bool("test.bool_key", false);
    ASSERT_TRUE(ok);
    val = settings_get_bool("test.bool_key", true);
    ASSERT_FALSE(val);

    settings_store_close();
}

/* ── Test: get/set string ────────────────────────────────── */

static void test_get_set_string(void) {
    printf("  test_get_set_string\n");

    settings_store_init(NULL);

    bool ok = settings_set_string("test.str_key", "hello world");
    ASSERT_TRUE(ok);

    const char *val = settings_get_string("test.str_key", "");
    ASSERT_STR_EQ(val, "hello world");

    /* Overwrite */
    settings_set_string("test.str_key", "updated");
    val = settings_get_string("test.str_key", "");
    ASSERT_STR_EQ(val, "updated");

    /* Empty string */
    settings_set_string("test.empty_key", "");
    val = settings_get_string("test.empty_key", "default");
    ASSERT_STR_EQ(val, "");

    settings_store_close();
}

/* ── Test: missing key returns default value ─────────────── */

static void test_missing_key_default(void) {
    printf("  test_missing_key_default\n");

    settings_store_init(NULL);

    int ival = settings_get_int("nonexistent.int", 999);
    ASSERT_EQ_INT(ival, 999);

    float fval = settings_get_float("nonexistent.float", 1.5f);
    ASSERT_FLOAT_NEAR(fval, 1.5f, 0.001);

    bool bval = settings_get_bool("nonexistent.bool", true);
    ASSERT_TRUE(bval);

    const char *sval = settings_get_string("nonexistent.string", "fallback");
    ASSERT_STR_EQ(sval, "fallback");

    settings_store_close();
}

/* ── Test: key existence check ───────────────────────────── */

static void test_key_exists(void) {
    printf("  test_key_exists\n");

    settings_store_init(NULL);

    /* Default keys should exist */
    ASSERT_TRUE(settings_exists(SETTINGS_KEY_BRIGHTNESS));
    ASSERT_TRUE(settings_exists(SETTINGS_KEY_DEVICE_NAME));

    /* Non-existent key */
    ASSERT_FALSE(settings_exists("nonexistent.key"));

    /* Set a custom key, then check */
    settings_set_int("custom.key", 42);
    ASSERT_TRUE(settings_exists("custom.key"));

    settings_store_close();
}

/* ── Test: reset to defaults ─────────────────────────────── */

static void test_reset_to_defaults(void) {
    printf("  test_reset_to_defaults\n");

    settings_store_init(NULL);

    /* Modify a default value */
    settings_set_int(SETTINGS_KEY_BRIGHTNESS, 1);
    ASSERT_EQ_INT(settings_get_int(SETTINGS_KEY_BRIGHTNESS, -1), 1);

    /* Add a custom key */
    settings_set_string("custom.key", "value");
    ASSERT_TRUE(settings_exists("custom.key"));

    /* Reset */
    bool ok = settings_reset_to_defaults();
    ASSERT_TRUE(ok);

    /* Default value should be restored */
    ASSERT_EQ_INT(settings_get_int(SETTINGS_KEY_BRIGHTNESS, -1), 70);

    /* Custom key should be gone */
    ASSERT_FALSE(settings_exists("custom.key"));

    settings_store_close();
}

/* ── Test: load_defaults does not overwrite existing keys ── */

static void test_load_defaults_no_overwrite(void) {
    printf("  test_load_defaults_no_overwrite\n");

    settings_store_init(NULL);

    /* Modify brightness */
    settings_set_int(SETTINGS_KEY_BRIGHTNESS, 1);

    /* Call load_defaults again */
    settings_load_defaults();

    /* Modified value should be preserved (not overwritten) */
    ASSERT_EQ_INT(settings_get_int(SETTINGS_KEY_BRIGHTNESS, -1), 1);

    settings_store_close();
}

/* ── Test: set/get with NULL key is safe ─────────────────── */

static void test_null_key_safe(void) {
    printf("  test_null_key_safe\n");

    settings_store_init(NULL);

    /* These should not crash */
    ASSERT_FALSE(settings_exists(NULL));
    ASSERT_EQ_INT(settings_get_int(NULL, -1), -1);
    ASSERT_FALSE(settings_set_int(NULL, 42));

    settings_store_close();
}

/* ── Public entry point ──────────────────────────────────── */

void test_settings_store(void) {
    test_init_close();
    test_defaults_loaded();
    test_get_set_int();
    test_get_set_float();
    test_get_set_bool();
    test_get_set_string();
    test_missing_key_default();
    test_key_exists();
    test_reset_to_defaults();
    test_load_defaults_no_overwrite();
    test_null_key_safe();
}
