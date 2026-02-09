/**
 * @file main.c
 * @brief LVGL simulator main application for Vitals Monitor
 *
 * Phase 3: Trends with SQLite + Waveform Display
 * - Screen manager with stack-based navigation
 * - Main vitals screen with HR, SpO2, NIBP, Temp, RR
 * - Real-time ECG and Pleth waveforms (lv_chart, circular sweep)
 * - Mock data generator updating values every second
 * - Waveform generator synthesizing samples every frame
 * - Alarm color coding based on threshold evaluation
 * - Navigation bar with placeholder screens
 */

#include "lvgl.h"
#include <SDL2/SDL.h>
#include "sdl_display.h"
#include "sdl_input.h"

/* Application modules */
#include "theme_vitals.h"
#include "screen_manager.h"
#include "screen_main_vitals.h"
#include "screen_trends.h"
#include "screen_alarms.h"
#include "screen_patient.h"
#include "screen_settings.h"
#include "vitals_provider.h"
#include "trend_db.h"
#include "waveform_gen.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>

static bool running = true;
static lv_timer_t *purge_timer = NULL;

/* ── Waveform generators ──────────────────────────────────── */

static waveform_gen_t ecg_gen;
static waveform_gen_t pleth_gen;
static lv_timer_t *waveform_timer = NULL;

/*
 * Waveform sample rate:
 *   25mm/s sweep speed at 130 DPI = ~128 pixels/sec
 *   At 30 FPS = 128/30 ~= 4.3 samples/frame
 *   We push 4 samples per frame for smooth animation.
 */
#define WAVEFORM_SAMPLES_PER_SEC   128
#define WAVEFORM_SAMPLES_PER_FRAME 4
#define WAVEFORM_TIMER_PERIOD_MS   33

/* ── Signal handler ────────────────────────────────────────── */

static void signal_handler(int signum) {
    (void)signum;
    printf("\nShutting down simulator...\n");
    running = false;
}

/* ── Alarm threshold evaluation ────────────────────────────── */

static vm_alarm_severity_t highest_alarm = VM_ALARM_NONE;
static vm_alarm_severity_t prev_alarm = VM_ALARM_NONE;
static const char *alarm_message = NULL;

static void evaluate_alarms(const vitals_data_t *data) {
    highest_alarm = VM_ALARM_NONE;
    alarm_message = NULL;

    /* HR thresholds (from PRD-004) */
    if (data->hr > 150 || data->hr < 40) {
        highest_alarm = VM_ALARM_HIGH;
        alarm_message = data->hr > 150 ? "HR Very High" : "HR Very Low";
    } else if (data->hr > 120 || data->hr < 50) {
        if (highest_alarm < VM_ALARM_MEDIUM) {
            highest_alarm = VM_ALARM_MEDIUM;
            alarm_message = data->hr > 120 ? "HR High" : "HR Low";
        }
    }

    /* SpO2 thresholds */
    if (data->spo2 < 85) {
        highest_alarm = VM_ALARM_HIGH;
        alarm_message = "SpO2 Critical";
    } else if (data->spo2 < 90) {
        if (highest_alarm < VM_ALARM_MEDIUM) {
            highest_alarm = VM_ALARM_MEDIUM;
            alarm_message = "SpO2 Low";
        }
    }

    /* RR thresholds */
    if (data->rr > 30 || data->rr < 8) {
        if (highest_alarm < VM_ALARM_HIGH) {
            highest_alarm = VM_ALARM_HIGH;
            alarm_message = data->rr > 30 ? "RR Very High" : "RR Very Low";
        }
    } else if (data->rr > 24 || data->rr < 10) {
        if (highest_alarm < VM_ALARM_MEDIUM) {
            highest_alarm = VM_ALARM_MEDIUM;
            alarm_message = data->rr > 24 ? "RR High" : "RR Low";
        }
    }

    /* Temperature thresholds */
    if (data->temp > 39.0f || data->temp < 35.0f) {
        if (highest_alarm < VM_ALARM_HIGH) {
            highest_alarm = VM_ALARM_HIGH;
            alarm_message = data->temp > 39.0f ? "Temp Very High" : "Temp Very Low";
        }
    } else if (data->temp > 38.0f || data->temp < 36.0f) {
        if (highest_alarm < VM_ALARM_MEDIUM) {
            highest_alarm = VM_ALARM_MEDIUM;
            alarm_message = data->temp > 38.0f ? "Temp High" : "Temp Low";
        }
    }
}

/* ── Waveform timer callback ──────────────────────────────── */

static void waveform_timer_cb(lv_timer_t *timer) {
    (void)timer;

    /* Only generate waveforms when main vitals screen is active */
    if (screen_manager_get_active() != SCREEN_ID_MAIN_VITALS) return;

    /* Push multiple samples per frame for smooth sweep */
    for (int i = 0; i < WAVEFORM_SAMPLES_PER_FRAME; i++) {
        int32_t ecg_val  = waveform_gen_next_sample(&ecg_gen);
        int32_t pleth_val = waveform_gen_next_sample(&pleth_gen);
        screen_main_vitals_push_ecg_sample(ecg_val);
        screen_main_vitals_push_pleth_sample(pleth_val);
    }

    /* Single chart refresh after all samples pushed (efficient) */
    screen_main_vitals_refresh_waveforms();
}

/* ── Vitals data callback ──────────────────────────────────── */

static void on_vitals_update(const vitals_data_t *data, void *user_data) {
    (void)user_data;  /* Unused in simulator */
    /* Update vital sign displays */
    screen_main_vitals_update_hr(data->hr);
    screen_main_vitals_update_spo2(data->spo2);
    screen_main_vitals_update_temp(data->temp);
    screen_main_vitals_update_rr(data->rr);

    /* NIBP only updates when a measurement is taken */
    if (data->nibp_fresh) {
        screen_main_vitals_update_nibp(data->nibp_sys, data->nibp_dia, data->nibp_map);
    }

    /* Update waveform generators with current heart rate */
    waveform_gen_set_hr(&ecg_gen,  data->hr, WAVEFORM_SAMPLES_PER_SEC);
    waveform_gen_set_hr(&pleth_gen, data->hr, WAVEFORM_SAMPLES_PER_SEC);

    /* Evaluate alarm thresholds */
    evaluate_alarms(data);

    /* Get current time for alarm logging */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_buf[8];
    snprintf(time_buf, sizeof(time_buf), "%02d:%02d", tm_info->tm_hour, tm_info->tm_min);

    /* Log alarm transitions (only on state change to avoid flooding) */
    if (highest_alarm != prev_alarm && highest_alarm != VM_ALARM_NONE && alarm_message) {
        /* Map vm_alarm_severity_t to vitals_alarm_severity_t (same numeric values) */
        vitals_alarm_severity_t sev = (vitals_alarm_severity_t)highest_alarm;
        vitals_provider_log_alarm(sev, alarm_message, time_buf);
        trend_db_insert_alarm((uint32_t)(data->timestamp_ms / 1000), highest_alarm, alarm_message);
    }
    prev_alarm = highest_alarm;

    /* Update alarm banner */
    if (highest_alarm != VM_ALARM_NONE && alarm_message) {
        screen_main_vitals_set_alarm(highest_alarm, alarm_message);
    } else {
        screen_main_vitals_set_alarm(VM_ALARM_NONE, NULL);
    }

    /* Update clock */
    screen_main_vitals_update_time(time_buf);
}

/* ── Trend database purge timer ────────────────────────────── */

static void trend_purge_timer_cb(lv_timer_t *timer) {
    (void)timer;
    const vitals_data_t *d = vitals_provider_get_current(0);
    if (d) {
        trend_db_purge_old((uint32_t)(d->timestamp_ms / 1000));
    }
}

/* ── Main ──────────────────────────────────────────────────── */

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    printf("========================================\n");
    printf("  Bedside Vitals Monitor - Simulator\n");
    printf("  Phase 2: Waveform Display\n");
    printf("========================================\n\n");

    /* Signal handlers for clean exit */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Initialize LVGL */
    lv_init();
    printf("LVGL initialized (version %d.%d.%d)\n",
           lv_version_major(), lv_version_minor(), lv_version_patch());

    /* Initialize SDL display (must be before lv_tick_set_cb since SDL needs init first) */
    if (!sdl_display_init(VM_SCREEN_WIDTH, VM_SCREEN_HEIGHT)) {
        fprintf(stderr, "Failed to initialize SDL display\n");
        return 1;
    }

    /* Provide SDL tick as LVGL's time source (SDL must be initialized first) */
    lv_tick_set_cb(SDL_GetTicks);

    /* Initialize SDL input */
    if (!sdl_input_init()) {
        fprintf(stderr, "Failed to initialize SDL input\n");
        sdl_display_deinit();
        return 1;
    }

    /* Initialize theme */
    theme_vitals_init();

    /* Initialize screen manager */
    screen_manager_init();

    /* Register all screens */
    screen_reg_t registrations[] = {
        { SCREEN_ID_MAIN_VITALS, screen_main_vitals_create, screen_main_vitals_destroy, "Main Vitals" },
        { SCREEN_ID_TRENDS,      screen_trends_create,      screen_trends_destroy,      "Trends" },
        { SCREEN_ID_ALARMS,      screen_alarms_create,      screen_alarms_destroy,      "Alarms" },
        { SCREEN_ID_PATIENT,     screen_patient_create,      screen_patient_destroy,      "Patient" },
        { SCREEN_ID_SETTINGS,    screen_settings_create,    screen_settings_destroy,    "Settings" },
    };
    for (int i = 0; i < (int)(sizeof(registrations) / sizeof(registrations[0])); i++) {
        screen_manager_register(&registrations[i]);
    }

    /* Navigate to main vitals screen */
    screen_manager_push(SCREEN_ID_MAIN_VITALS);

    /* Auto-return to main vitals after 2 minutes of inactivity */
    screen_manager_set_auto_return(120000);

    /* Initialize trend database (before vitals provider, which inserts into it) */
    trend_db_init("vitals_trends.db");

    /* Initialize and start vitals provider (uses mock implementation for simulator) */
    vitals_provider_init();
    vitals_provider_set_vitals_callback(on_vitals_update, NULL);
    vitals_provider_start(1000);  /* 1 second interval */

    /* Initialize waveform generators */
    waveform_gen_init(&ecg_gen,  WAVEFORM_ECG,  180, 200);  /* scaled to chart Y range [0..400] */
    waveform_gen_init(&pleth_gen, WAVEFORM_PLETH, 150, 200);

    /* Set initial HR (72 bpm baseline from vitals provider) */
    waveform_gen_set_hr(&ecg_gen,  72, WAVEFORM_SAMPLES_PER_SEC);
    waveform_gen_set_hr(&pleth_gen, 72, WAVEFORM_SAMPLES_PER_SEC);

    /* Start waveform timer (fires each frame) */
    waveform_timer = lv_timer_create(waveform_timer_cb, WAVEFORM_TIMER_PERIOD_MS, NULL);
    printf("Waveform generators started (%d samples/sec, %d per frame)\n",
           WAVEFORM_SAMPLES_PER_SEC, WAVEFORM_SAMPLES_PER_FRAME);

    /* Purge old trend data every 5 minutes */
    purge_timer = lv_timer_create(trend_purge_timer_cb, 300000, NULL);

    printf("\nSimulator running. Press Ctrl+C to exit.\n");
    printf("Window size: %dx%d (matching target hardware)\n\n",
           VM_SCREEN_WIDTH, VM_SCREEN_HEIGHT);

    /* Main event loop */
    while (running) {
        /* Process SDL events */
        if (!sdl_input_process_events()) {
            running = false;
            break;
        }

        /* Handle LVGL tasks */
        uint32_t time_till_next = lv_timer_handler();

        /* Sleep for a short time */
        usleep(time_till_next * 1000);
    }

    /* Cleanup */
    printf("Cleaning up...\n");
    if (purge_timer) {
        lv_timer_delete(purge_timer);
        purge_timer = NULL;
    }
    if (waveform_timer) {
        lv_timer_delete(waveform_timer);
        waveform_timer = NULL;
    }
    vitals_provider_deinit();  /* This internally calls stop() */
    trend_db_close();
    sdl_display_deinit();

    printf("Simulator exited cleanly.\n");
    return 0;
}
