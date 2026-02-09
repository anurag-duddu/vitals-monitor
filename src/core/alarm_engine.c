/**
 * @file alarm_engine.c
 * @brief Alarm threshold engine implementation
 *
 * Evaluates vital signs against configurable per-parameter thresholds
 * and manages a state machine for each parameter:
 *
 *   INACTIVE  --[threshold exceeded]--> ACTIVE
 *   ACTIVE    --[acknowledge()]--> ACKNOWLEDGED
 *   ACTIVE/ACKNOWLEDGED --[silence()]--> SILENCED
 *   SILENCED  --[timeout + still violated]--> ACTIVE
 *   ANY       --[vital returns to normal]--> INACTIVE
 *
 * All allocation is static.  No LVGL headers are included.
 * Single-threaded: called exclusively from the LVGL main loop.
 */

#include "alarm_engine.h"
#include "vitals_provider.h"
#include <stdio.h>
#include <string.h>

/* ── Default silence duration (seconds) ──────────────────── */

#define ALARM_DEFAULT_SILENCE_S  120  /* 2 minutes */

/* ── Parameter name strings (for log messages) ───────────── */

static const char *param_names[ALARM_PARAM_COUNT] = {
    "HR",
    "SpO2",
    "RR",
    "Temp",
    "NIBP Sys",
    "NIBP Dia",
};

/* ── State name strings (for log messages) ───────────────── */

static const char *state_names[] = {
    "INACTIVE",
    "ACTIVE",
    "ACKNOWLEDGED",
    "SILENCED",
};

/* ── Severity name strings (for log messages) ────────────── */

static const char *severity_names[] = {
    "NONE",
    "LOW",
    "MEDIUM",
    "HIGH",
};

/* ── Module state (all static, no malloc) ────────────────── */

static alarm_engine_state_t engine_state;
static alarm_limits_t       limits[ALARM_PARAM_COUNT];
static bool                 initialized = false;
static uint32_t             current_time = 0;  /* Cached from last evaluate() */

/* ── Default thresholds ──────────────────────────────────── */

/* HR: critical >150/<40, warning >120/<50 */
static const alarm_limits_t default_hr = {
    .critical_high = 150,
    .critical_low  = 40,
    .warning_high  = 120,
    .warning_low   = 50,
    .enabled       = true,
};

/* SpO2: critical <85, warning <90 (no high limit) */
static const alarm_limits_t default_spo2 = {
    .critical_high = 9999,   /* Effectively disabled (SpO2 max is 100) */
    .critical_low  = 85,
    .warning_high  = 9999,   /* Effectively disabled */
    .warning_low   = 90,
    .enabled       = true,
};

/* RR: critical >30/<8, warning >24/<10 */
static const alarm_limits_t default_rr = {
    .critical_high = 30,
    .critical_low  = 8,
    .warning_high  = 24,
    .warning_low   = 10,
    .enabled       = true,
};

/* Temp: critical >39.0/<35.0, warning >38.0/<36.0 (stored as x10) */
static const alarm_limits_t default_temp = {
    .critical_high = 390,
    .critical_low  = 350,
    .warning_high  = 380,
    .warning_low   = 360,
    .enabled       = true,
};

/* NIBP Sys: critical >180/<80, warning >140/<90 */
static const alarm_limits_t default_nibp_sys = {
    .critical_high = 180,
    .critical_low  = 80,
    .warning_high  = 140,
    .warning_low   = 90,
    .enabled       = true,
};

/* NIBP Dia: critical >120/<40, warning >90/<50 */
static const alarm_limits_t default_nibp_dia = {
    .critical_high = 120,
    .critical_low  = 40,
    .warning_high  = 90,
    .warning_low   = 50,
    .enabled       = true,
};

/* Lookup table indexed by alarm_param_t */
static const alarm_limits_t *default_limits[ALARM_PARAM_COUNT] = {
    &default_hr,
    &default_spo2,
    &default_rr,
    &default_temp,
    &default_nibp_sys,
    &default_nibp_dia,
};

/* ── Forward declarations ────────────────────────────────── */

static void evaluate_param(alarm_param_t param, int value, uint32_t time_s);
static alarm_severity_t check_thresholds(alarm_param_t param, int value);
static void set_state(alarm_param_t param, alarm_state_t new_state);
static void build_message(alarm_param_t param, alarm_severity_t sev, int value, bool high_side);
static void update_highest(void);

/* ── Lifecycle ───────────────────────────────────────────── */

void alarm_engine_init(void) {
    memset(&engine_state, 0, sizeof(engine_state));
    alarm_engine_reset_defaults();
    initialized = true;
    current_time = 0;
    printf("[alarm_engine] Initialized with default thresholds\n");
}

void alarm_engine_deinit(void) {
    memset(&engine_state, 0, sizeof(engine_state));
    initialized = false;
    printf("[alarm_engine] Deinitialized\n");
}

/* ── Evaluation ──────────────────────────────────────────── */

void alarm_engine_evaluate(const vitals_data_t *data, uint32_t current_time_s) {
    if (!initialized || !data) return;

    current_time = current_time_s;

    /* Check audio pause expiry */
    if (engine_state.audio_paused && current_time_s >= engine_state.audio_pause_until_s) {
        engine_state.audio_paused = false;
        engine_state.audio_pause_until_s = 0;
        printf("[alarm_engine] Audio pause expired\n");
    }

    /* Evaluate continuous parameters */
    evaluate_param(ALARM_PARAM_HR,   data->hr,   current_time_s);
    evaluate_param(ALARM_PARAM_SPO2, data->spo2, current_time_s);
    evaluate_param(ALARM_PARAM_RR,   data->rr,   current_time_s);

    /* Temperature: convert float to x10 integer for threshold comparison */
    int temp_x10 = (int)(data->temp * 10.0f + 0.5f);
    evaluate_param(ALARM_PARAM_TEMP, temp_x10, current_time_s);

    /* NIBP: evaluate on every call (thresholds apply to last-known value) */
    evaluate_param(ALARM_PARAM_NIBP_SYS, data->nibp_sys, current_time_s);
    evaluate_param(ALARM_PARAM_NIBP_DIA, data->nibp_dia, current_time_s);

    /* Recompute highest active/any alarm */
    update_highest();
}

/* ── State query ─────────────────────────────────────────── */

const alarm_engine_state_t *alarm_engine_get_state(void) {
    return &engine_state;
}

/* ── Alarm management ────────────────────────────────────── */

bool alarm_engine_acknowledge(alarm_param_t param) {
    if (param >= ALARM_PARAM_COUNT) return false;

    alarm_status_t *s = &engine_state.params[param];
    if (s->state != ALARM_STATE_ACTIVE) return false;

    set_state(param, ALARM_STATE_ACKNOWLEDGED);
    s->ack_time_s = current_time;
    update_highest();
    return true;
}

bool alarm_engine_acknowledge_all(void) {
    bool any = false;
    for (int i = 0; i < ALARM_PARAM_COUNT; i++) {
        if (alarm_engine_acknowledge((alarm_param_t)i)) {
            any = true;
        }
    }
    return any;
}

bool alarm_engine_silence(alarm_param_t param, uint32_t duration_s) {
    if (param >= ALARM_PARAM_COUNT) return false;

    alarm_status_t *s = &engine_state.params[param];
    if (s->state != ALARM_STATE_ACTIVE && s->state != ALARM_STATE_ACKNOWLEDGED) {
        return false;
    }

    set_state(param, ALARM_STATE_SILENCED);
    s->silence_until_s = current_time + duration_s;
    update_highest();
    return true;
}

bool alarm_engine_silence_all(uint32_t duration_s) {
    bool any = false;
    for (int i = 0; i < ALARM_PARAM_COUNT; i++) {
        if (alarm_engine_silence((alarm_param_t)i, duration_s)) {
            any = true;
        }
    }
    return any;
}

/* ── Threshold configuration ─────────────────────────────── */

void alarm_engine_set_limits(alarm_param_t param, const alarm_limits_t *new_limits) {
    if (param >= ALARM_PARAM_COUNT || !new_limits) return;
    limits[param] = *new_limits;
    printf("[alarm_engine] Limits updated for %s: crit=[%d..%d] warn=[%d..%d] %s\n",
           param_names[param],
           new_limits->critical_low, new_limits->critical_high,
           new_limits->warning_low, new_limits->warning_high,
           new_limits->enabled ? "enabled" : "disabled");
}

const alarm_limits_t *alarm_engine_get_limits(alarm_param_t param) {
    if (param >= ALARM_PARAM_COUNT) return NULL;
    return &limits[param];
}

void alarm_engine_reset_defaults(void) {
    for (int i = 0; i < ALARM_PARAM_COUNT; i++) {
        limits[i] = *default_limits[i];
    }
    printf("[alarm_engine] Thresholds reset to defaults\n");
}

/* ── Audio control ───────────────────────────────────────── */

void alarm_engine_pause_audio(uint32_t duration_s) {
    engine_state.audio_paused = true;
    engine_state.audio_pause_until_s = current_time + duration_s;
    printf("[alarm_engine] Audio paused for %u seconds\n", duration_s);
}

bool alarm_engine_is_audio_paused(void) {
    return engine_state.audio_paused;
}

/* ── Private: evaluate a single parameter ────────────────── */

static void evaluate_param(alarm_param_t param, int value, uint32_t time_s) {
    alarm_status_t *s = &engine_state.params[param];
    const alarm_limits_t *lim = &limits[param];

    /* Skip disabled parameters */
    if (!lim->enabled) {
        if (s->state != ALARM_STATE_INACTIVE) {
            set_state(param, ALARM_STATE_INACTIVE);
            s->severity = ALARM_SEV_NONE;
            s->message[0] = '\0';
        }
        return;
    }

    /* Skip invalid values (0 = no signal / not yet measured) */
    if (value == 0) return;

    /* Check current value against thresholds */
    alarm_severity_t new_sev = check_thresholds(param, value);

    /* Determine which side of the threshold was violated */
    bool high_side = false;
    if (new_sev != ALARM_SEV_NONE) {
        if (new_sev == ALARM_SEV_HIGH) {
            high_side = (value > lim->critical_high);
        } else {
            high_side = (value > lim->warning_high);
        }
    }

    /* State machine transitions */
    switch (s->state) {

    case ALARM_STATE_INACTIVE:
        if (new_sev != ALARM_SEV_NONE) {
            /* INACTIVE -> ACTIVE: threshold exceeded */
            s->severity = new_sev;
            s->trigger_time_s = time_s;
            s->ack_time_s = 0;
            s->silence_until_s = 0;
            build_message(param, new_sev, value, high_side);
            set_state(param, ALARM_STATE_ACTIVE);
        }
        break;

    case ALARM_STATE_ACTIVE:
        if (new_sev == ALARM_SEV_NONE) {
            /* ACTIVE -> INACTIVE: vital returned to normal */
            s->severity = ALARM_SEV_NONE;
            s->message[0] = '\0';
            set_state(param, ALARM_STATE_INACTIVE);
        } else {
            /* Still active: update severity if it changed (escalation or de-escalation) */
            if (new_sev != s->severity) {
                alarm_severity_t old_sev = s->severity;
                s->severity = new_sev;
                build_message(param, new_sev, value, high_side);
                printf("[alarm_engine] %s severity changed: %s -> %s\n",
                       param_names[param],
                       severity_names[old_sev],
                       severity_names[new_sev]);
            }
        }
        break;

    case ALARM_STATE_ACKNOWLEDGED:
        if (new_sev == ALARM_SEV_NONE) {
            /* ACKNOWLEDGED -> INACTIVE: vital returned to normal */
            s->severity = ALARM_SEV_NONE;
            s->message[0] = '\0';
            set_state(param, ALARM_STATE_INACTIVE);
        } else {
            /* Still violated: update severity and message */
            if (new_sev != s->severity) {
                alarm_severity_t old_sev = s->severity;
                s->severity = new_sev;
                build_message(param, new_sev, value, high_side);

                /* Escalation to higher severity resets to ACTIVE (re-alert) */
                if (new_sev > old_sev) {
                    s->trigger_time_s = time_s;
                    s->ack_time_s = 0;
                    set_state(param, ALARM_STATE_ACTIVE);
                    printf("[alarm_engine] %s escalated %s -> %s, re-alerting\n",
                           param_names[param],
                           severity_names[old_sev],
                           severity_names[new_sev]);
                }
            }
        }
        break;

    case ALARM_STATE_SILENCED:
        if (new_sev == ALARM_SEV_NONE) {
            /* SILENCED -> INACTIVE: vital returned to normal */
            s->severity = ALARM_SEV_NONE;
            s->message[0] = '\0';
            s->silence_until_s = 0;
            set_state(param, ALARM_STATE_INACTIVE);
        } else if (time_s >= s->silence_until_s) {
            /* Silence expired and condition still present -> ACTIVE */
            s->severity = new_sev;
            s->trigger_time_s = time_s;
            s->ack_time_s = 0;
            s->silence_until_s = 0;
            build_message(param, new_sev, value, high_side);
            set_state(param, ALARM_STATE_ACTIVE);
            printf("[alarm_engine] %s silence expired, re-alerting (%s)\n",
                   param_names[param], severity_names[new_sev]);
        } else {
            /* Still silenced: quietly update severity for when silence ends */
            s->severity = new_sev;
            build_message(param, new_sev, value, high_side);
        }
        break;
    }
}

/* ── Private: check value against thresholds ─────────────── */

static alarm_severity_t check_thresholds(alarm_param_t param, int value) {
    const alarm_limits_t *lim = &limits[param];

    /* Critical thresholds (HIGH severity): strictly > high or < low */
    if (value > lim->critical_high || value < lim->critical_low) {
        return ALARM_SEV_HIGH;
    }

    /* Warning thresholds (MEDIUM severity): strictly > high or < low */
    if (value > lim->warning_high || value < lim->warning_low) {
        return ALARM_SEV_MEDIUM;
    }

    return ALARM_SEV_NONE;
}

/* ── Private: set state with logging ─────────────────────── */

static void set_state(alarm_param_t param, alarm_state_t new_state) {
    alarm_status_t *s = &engine_state.params[param];
    alarm_state_t old_state = s->state;

    if (old_state == new_state) return;

    s->state = new_state;
    printf("[alarm_engine] %s: %s -> %s",
           param_names[param], state_names[old_state], state_names[new_state]);

    if (new_state == ALARM_STATE_ACTIVE || new_state == ALARM_STATE_ACKNOWLEDGED) {
        printf(" [%s: \"%s\"]", severity_names[s->severity], s->message);
    }
    printf("\n");
}

/* ── Private: build human-readable alarm message ─────────── */

static void build_message(alarm_param_t param, alarm_severity_t sev,
                           int value, bool high_side) {
    (void)value;  /* Reserved for future use (e.g., "HR Very High: 155") */

    alarm_status_t *s = &engine_state.params[param];
    const char *name = param_names[param];
    const char *level;

    if (sev == ALARM_SEV_HIGH) {
        level = high_side ? "Very High" : "Very Low";
    } else {
        level = high_side ? "High" : "Low";
    }

    snprintf(s->message, sizeof(s->message), "%s %s", name, level);
}

/* ── Private: recompute highest_active and highest_any ───── */

static void update_highest(void) {
    engine_state.highest_active = ALARM_SEV_NONE;
    engine_state.highest_any = ALARM_SEV_NONE;
    engine_state.highest_message = NULL;

    const char *active_msg = NULL;
    const char *any_msg = NULL;

    for (int i = 0; i < ALARM_PARAM_COUNT; i++) {
        const alarm_status_t *s = &engine_state.params[i];

        if (s->state == ALARM_STATE_INACTIVE) continue;

        /* Track highest of any state (ACTIVE, ACKNOWLEDGED, SILENCED) */
        if (s->severity > engine_state.highest_any) {
            engine_state.highest_any = s->severity;
            any_msg = s->message;
        }

        /* Track highest unacknowledged (ACTIVE only) */
        if (s->state == ALARM_STATE_ACTIVE) {
            if (s->severity > engine_state.highest_active) {
                engine_state.highest_active = s->severity;
                active_msg = s->message;
            }
        }
    }

    /* Prefer the highest active alarm message; fall back to any */
    if (active_msg) {
        engine_state.highest_message = active_msg;
    } else {
        engine_state.highest_message = any_msg;
    }
}
