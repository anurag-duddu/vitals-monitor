/**
 * @file vitals_provider_mock.c
 * @brief Mock implementation of vitals provider for simulator builds
 *
 * This implementation uses the mock_data generator with LVGL timers.
 * Compile this file when building for simulator (VITALS_PROVIDER_MOCK defined).
 */

#include "vitals_provider.h"
#include "mock_data.h"
#include "waveform_gen.h"
#include "lvgl.h"
#include <string.h>
#include <stdio.h>

/* ── State ─────────────────────────────────────────────────── */

static bool g_initialized = false;
static bool g_running = false;

static vitals_callback_t   g_vitals_cb = NULL;
static void               *g_vitals_user_data = NULL;
static waveform_callback_t g_waveform_cb = NULL;
static void               *g_waveform_user_data = NULL;

/* Current data for dual-patient (mock only supports single for now) */
static vitals_data_t g_current_vitals[2];

/* Waveform timer */
static lv_timer_t *g_waveform_timer = NULL;
#define WAVEFORM_UPDATE_MS 50  /* 20 Hz waveform updates */

/* History (using mock_data's history for now) */
static vitals_history_t g_history[2];

/* Alarm log */
static vitals_alarm_log_t g_alarm_log;
static uint32_t g_start_tick = 0;

/* ── Forward declarations ──────────────────────────────────── */

static void on_mock_data_update(const vitals_data_t *mock_data);
static void waveform_timer_cb(lv_timer_t *timer);

/* ── Public API ────────────────────────────────────────────── */

int vitals_provider_init(void) {
    if (g_initialized) {
        return 0;  /* Already initialized */
    }

    mock_data_init();
    waveform_gen_init();

    memset(g_current_vitals, 0, sizeof(g_current_vitals));
    memset(g_history, 0, sizeof(g_history));
    memset(&g_alarm_log, 0, sizeof(g_alarm_log));
    g_start_tick = lv_tick_get();

    g_initialized = true;
    printf("[vitals_provider] Initialized (type=mock)\n");
    return 0;
}

int vitals_provider_start(uint32_t vitals_interval_ms) {
    if (!g_initialized) {
        return -1;
    }
    if (g_running) {
        return 0;  /* Already running */
    }

    /* Register our internal callback with mock_data */
    mock_data_set_callback(on_mock_data_update);
    mock_data_start(vitals_interval_ms);

    /* Start waveform timer */
    g_waveform_timer = lv_timer_create(waveform_timer_cb, WAVEFORM_UPDATE_MS, NULL);

    g_running = true;
    printf("[vitals_provider] Started (interval=%u ms)\n", vitals_interval_ms);
    return 0;
}

void vitals_provider_stop(void) {
    if (!g_running) {
        return;
    }

    mock_data_stop();

    if (g_waveform_timer) {
        lv_timer_delete(g_waveform_timer);
        g_waveform_timer = NULL;
    }

    g_running = false;
    printf("[vitals_provider] Stopped\n");
}

void vitals_provider_deinit(void) {
    vitals_provider_stop();
    g_initialized = false;
    g_vitals_cb = NULL;
    g_waveform_cb = NULL;
    printf("[vitals_provider] Deinitialized\n");
}

void vitals_provider_set_vitals_callback(vitals_callback_t callback, void *user_data) {
    g_vitals_cb = callback;
    g_vitals_user_data = user_data;
}

void vitals_provider_set_waveform_callback(waveform_callback_t callback, void *user_data) {
    g_waveform_cb = callback;
    g_waveform_user_data = user_data;
}

const vitals_data_t *vitals_provider_get_current(uint8_t slot) {
    if (slot > 1) {
        return NULL;
    }
    return &g_current_vitals[slot];
}

bool vitals_provider_is_running(void) {
    return g_running;
}

const char *vitals_provider_get_type(void) {
    return "mock";
}

const vitals_history_t *vitals_provider_get_history(uint8_t slot) {
    if (slot > 1) {
        return NULL;
    }
    /* For mock, copy from mock_data's history format */
    return &g_history[slot];
}

/* ── Internal callbacks ────────────────────────────────────── */

/**
 * Called by mock_data when new vitals are generated.
 * Converts to vitals_data_t and notifies user callback.
 */
static void on_mock_data_update(const vitals_data_t *mock_data) {
    /* Copy mock data to our current vitals (slot 0) */
    /* The mock_data vitals_data_t is compatible with our vitals_data_t */
    g_current_vitals[0].hr = mock_data->hr;
    g_current_vitals[0].spo2 = mock_data->spo2;
    g_current_vitals[0].rr = mock_data->rr;
    g_current_vitals[0].temp = mock_data->temp;
    g_current_vitals[0].nibp_sys = mock_data->nibp_sys;
    g_current_vitals[0].nibp_dia = mock_data->nibp_dia;
    g_current_vitals[0].nibp_map = mock_data->nibp_map;
    g_current_vitals[0].nibp_fresh = mock_data->nibp_fresh;
    g_current_vitals[0].timestamp_ms = lv_tick_get();
    g_current_vitals[0].patient_slot = 0;
    g_current_vitals[0].hr_quality = 95;   /* Mock: always good quality */
    g_current_vitals[0].spo2_quality = 95;
    g_current_vitals[0].ecg_lead_off = 0;

    /* Update history */
    int idx = g_history[0].write_idx;
    g_history[0].hr[idx] = mock_data->hr;
    g_history[0].spo2[idx] = mock_data->spo2;
    g_history[0].rr[idx] = mock_data->rr;
    g_history[0].timestamps[idx] = g_current_vitals[0].timestamp_ms;
    g_history[0].write_idx = (idx + 1) % VITALS_HISTORY_LEN;
    if (g_history[0].count < VITALS_HISTORY_LEN) {
        g_history[0].count++;
    }

    /* Notify user callback */
    if (g_vitals_cb) {
        g_vitals_cb(&g_current_vitals[0], g_vitals_user_data);
    }
}

/**
 * Generate and send waveform packets.
 */
static void waveform_timer_cb(lv_timer_t *timer) {
    (void)timer;

    if (!g_waveform_cb) {
        return;
    }

    waveform_packet_t packet;

    /* ECG waveform */
    packet.type = WAVEFORM_ECG;
    packet.patient_slot = 0;
    packet.sample_rate_hz = 500;
    packet.sample_count = WAVEFORM_UPDATE_MS * 500 / 1000;  /* samples for this interval */
    if (packet.sample_count > WAVEFORM_SAMPLES_PER_PACKET) {
        packet.sample_count = WAVEFORM_SAMPLES_PER_PACKET;
    }

    /* Get samples from waveform generator */
    float ecg_samples[WAVEFORM_SAMPLES_PER_PACKET];
    waveform_gen_ecg(ecg_samples, packet.sample_count, g_current_vitals[0].hr);
    for (int i = 0; i < packet.sample_count; i++) {
        packet.samples[i] = (int16_t)(ecg_samples[i] * 1000);  /* Scale to int16 */
    }
    packet.timestamp_ms = lv_tick_get();

    g_waveform_cb(&packet, g_waveform_user_data);

    /* Pleth waveform */
    packet.type = WAVEFORM_PLETH;
    packet.sample_rate_hz = 100;
    packet.sample_count = WAVEFORM_UPDATE_MS * 100 / 1000;
    if (packet.sample_count > WAVEFORM_SAMPLES_PER_PACKET) {
        packet.sample_count = WAVEFORM_SAMPLES_PER_PACKET;
    }

    float pleth_samples[WAVEFORM_SAMPLES_PER_PACKET];
    waveform_gen_pleth(pleth_samples, packet.sample_count, g_current_vitals[0].hr);
    for (int i = 0; i < packet.sample_count; i++) {
        packet.samples[i] = (int16_t)(pleth_samples[i] * 1000);
    }
    packet.timestamp_ms = lv_tick_get();

    g_waveform_cb(&packet, g_waveform_user_data);
}

/* ── Alarm Log API ─────────────────────────────────────────── */

const vitals_alarm_log_t *vitals_provider_get_alarm_log(void) {
    return &g_alarm_log;
}

void vitals_provider_log_alarm(vitals_alarm_severity_t severity,
                               const char *message,
                               const char *time_str) {
    int idx = g_alarm_log.write_idx;

    g_alarm_log.entries[idx].severity = severity;

    if (message) {
        strncpy(g_alarm_log.entries[idx].message, message,
                sizeof(g_alarm_log.entries[idx].message) - 1);
        g_alarm_log.entries[idx].message[sizeof(g_alarm_log.entries[idx].message) - 1] = '\0';
    } else {
        g_alarm_log.entries[idx].message[0] = '\0';
    }

    if (time_str) {
        strncpy(g_alarm_log.entries[idx].time_str, time_str,
                sizeof(g_alarm_log.entries[idx].time_str) - 1);
        g_alarm_log.entries[idx].time_str[sizeof(g_alarm_log.entries[idx].time_str) - 1] = '\0';
    } else {
        g_alarm_log.entries[idx].time_str[0] = '\0';
    }

    g_alarm_log.entries[idx].timestamp_s = (lv_tick_get() - g_start_tick) / 1000;

    g_alarm_log.write_idx = (idx + 1) % VITALS_ALARM_LOG_MAX;
    if (g_alarm_log.count < VITALS_ALARM_LOG_MAX) {
        g_alarm_log.count++;
    }
}
