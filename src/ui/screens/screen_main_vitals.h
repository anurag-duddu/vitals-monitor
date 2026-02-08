/**
 * @file screen_main_vitals.h
 * @brief Main vitals monitoring screen
 *
 * The primary operating screen showing all vital parameters,
 * real-time ECG/Pleth waveforms, alarm banner, and navigation bar.
 */

#ifndef SCREEN_MAIN_VITALS_H
#define SCREEN_MAIN_VITALS_H

#include "lvgl.h"
#include "theme_vitals.h"

/** Create the main vitals screen (called by screen manager). */
lv_obj_t * screen_main_vitals_create(void);

/** Cleanup (called by screen manager before screen deletion). */
void screen_main_vitals_destroy(void);

/* ── Per-parameter update functions ─────────────────────── */

void screen_main_vitals_update_hr(int value);
void screen_main_vitals_update_spo2(int value);
void screen_main_vitals_update_nibp(int sys, int dia, int map);
void screen_main_vitals_update_temp(float value);
void screen_main_vitals_update_rr(int value);

/** Update alarm state and banner message. */
void screen_main_vitals_set_alarm(vm_alarm_severity_t severity, const char *message);

/** Update the clock display. */
void screen_main_vitals_update_time(const char *time_str);

/* ── Waveform functions ─────────────────────────────────── */

/** Push a waveform sample to the ECG chart (~4 times per frame). */
void screen_main_vitals_push_ecg_sample(int32_t value);

/** Push a waveform sample to the Pleth chart (~4 times per frame). */
void screen_main_vitals_push_pleth_sample(int32_t value);

/** Refresh all waveform charts. Call once per frame after samples pushed. */
void screen_main_vitals_refresh_waveforms(void);

#endif /* SCREEN_MAIN_VITALS_H */
