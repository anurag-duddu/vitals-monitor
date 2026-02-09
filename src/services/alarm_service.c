/**
 * @file alarm_service.c
 * @brief Alarm evaluation and notification service implementation
 *
 * SIMULATOR_BUILD:
 *   On each tick, fetches the current vitals snapshot from vitals_provider
 *   and passes it to alarm_engine_evaluate().  The UI reads alarm state
 *   directly via alarm_engine_get_state() (shared address space).
 *
 * TARGET (documented, not yet implemented):
 *   - Subscribes to IPC_SOCKET_VITALS via nanomsg SUB socket
 *   - Deserialises ipc_msg_vitals_t and converts to vitals_data_t
 *   - Evaluates via alarm_engine_evaluate()
 *   - Publishes ipc_msg_alarm_t on IPC_SOCKET_ALARMS
 *   - Drives buzzer GPIO / LED hardware for audible/visual alarms
 *   - Logs alarm events to trend_db
 */

#include "alarm_service.h"
#include "alarm_engine.h"
#include "vitals_provider.h"
#include <stdio.h>
#include <string.h>

/*
 * On the target build these headers would also be used:
 *   #include "ipc_messages.h"
 *   #include "trend_db.h"
 *   #include <nanomsg/nn.h>
 *   #include <nanomsg/pubsub.h>
 */

/* ── Internal state ───────────────────────────────────────── */

static bool     s_running          = false;
static uint32_t s_last_heartbeat_s = 0;
static uint32_t s_tick_count       = 0;

/* ── Simulator implementation ─────────────────────────────── */

#ifdef SIMULATOR_BUILD

bool alarm_service_init(void)
{
    printf("[alarm_service] Initialising (simulator mode)\n");

    alarm_engine_init();

    s_running          = false;
    s_last_heartbeat_s = 0;
    s_tick_count       = 0;

    printf("[alarm_service] Initialised\n");
    return true;
}

void alarm_service_start(void)
{
    if (s_running) {
        printf("[alarm_service] Already running\n");
        return;
    }

    s_running = true;
    printf("[alarm_service] Started\n");
}

void alarm_service_stop(void)
{
    if (!s_running) return;

    printf("[alarm_service] Stopping...\n");
    s_running = false;
    printf("[alarm_service] Stopped\n");
}

void alarm_service_tick(uint32_t current_time_s)
{
    if (!s_running) return;

    s_last_heartbeat_s = current_time_s;
    s_tick_count++;

    /* Get the latest vitals snapshot from the provider (slot 0) */
    const vitals_data_t *vitals = vitals_provider_get_current(0);
    if (!vitals) return;

    /* Evaluate all alarm thresholds against current values */
    alarm_engine_evaluate(vitals, current_time_s);

    /*
     * In simulator mode the UI reads alarm state directly from
     * alarm_engine_get_state() since everything shares the same
     * address space.  No IPC publishing is needed.
     */
}

void alarm_service_deinit(void)
{
    printf("[alarm_service] Deinitialising...\n");

    if (s_running) {
        alarm_service_stop();
    }

    alarm_engine_deinit();

    s_running          = false;
    s_last_heartbeat_s = 0;
    s_tick_count       = 0;

    printf("[alarm_service] Deinitialised\n");
}

#else /* TARGET BUILD */

/*
 * TARGET IMPLEMENTATION (skeleton)
 *
 * On the real hardware the alarm service would:
 *
 * 1. alarm_service_init():
 *    - Call alarm_engine_init()
 *    - Create nanomsg SUB socket, connect to IPC_SOCKET_VITALS
 *    - Subscribe to IPC_MSG_VITALS topic
 *    - Create nanomsg PUB socket, bind to IPC_SOCKET_ALARMS
 *    - Initialise buzzer/LED GPIO
 *
 * 2. alarm_service_start():
 *    - Begin non-blocking recv loop on SUB socket
 *
 * 3. alarm_service_tick():
 *    - nn_recv() on SUB socket (non-blocking)
 *    - Deserialise ipc_msg_vitals_t into vitals_data_t
 *    - alarm_engine_evaluate(&vitals, current_time_s)
 *    - If alarm state changed, build ipc_msg_alarm_t and nn_send()
 *    - Drive buzzer GPIO based on alarm_engine_get_state()->highest_active
 *    - Log alarm events via trend_db_insert_alarm()
 *    - sd_notify(0, "WATCHDOG=1") for systemd watchdog
 *
 * 4. alarm_service_stop():
 *    - Silence buzzer, turn off alarm LEDs
 *    - Close nanomsg sockets
 *
 * 5. alarm_service_deinit():
 *    - alarm_engine_deinit()
 *    - Release GPIO resources
 */

bool alarm_service_init(void)
{
    printf("[alarm_service] Initialising (target mode - stub)\n");
    alarm_engine_init();
    /* TODO: nn_socket(AF_SP, NN_SUB), nn_connect(), nn_socket PUB, nn_bind() */
    s_running = false;
    return true;
}

void alarm_service_start(void)
{
    printf("[alarm_service] Starting (target mode - stub)\n");
    /* TODO: start SUB recv loop */
    s_running = true;
}

void alarm_service_stop(void)
{
    printf("[alarm_service] Stopping (target mode - stub)\n");
    /* TODO: silence buzzer, nn_close() */
    s_running = false;
}

void alarm_service_tick(uint32_t current_time_s)
{
    if (!s_running) return;
    s_last_heartbeat_s = current_time_s;
    s_tick_count++;

    /* TODO:
     * - nn_recv() vitals from SUB socket
     * - Deserialise ipc_msg_vitals_t -> vitals_data_t
     * - alarm_engine_evaluate(&vitals, current_time_s)
     * - Publish changed alarms via nn_send() on PUB socket
     * - Drive buzzer/LED hardware
     * - sd_notify(0, "WATCHDOG=1")
     */
}

void alarm_service_deinit(void)
{
    printf("[alarm_service] Deinitialising (target mode - stub)\n");
    if (s_running) alarm_service_stop();
    alarm_engine_deinit();
    s_running = false;
}

#endif /* SIMULATOR_BUILD */
