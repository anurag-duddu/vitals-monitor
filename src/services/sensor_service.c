/**
 * @file sensor_service.c
 * @brief Sensor data acquisition service implementation
 *
 * SIMULATOR_BUILD:
 *   Wraps vitals_provider (which uses mock_data) to provide the same service
 *   lifecycle interface that the target build uses.  The actual data generation
 *   is driven by the LVGL timer inside mock_data.c; this service simply
 *   initialises/starts/stops that pipeline and keeps its heartbeat alive.
 *
 * TARGET (documented, not yet implemented):
 *   - Uses sensor_hal to initialise and poll real sensors
 *   - Publishes ipc_msg_vitals_t on IPC_SOCKET_VITALS at 1 Hz
 *   - Publishes ipc_msg_waveform_t on IPC_SOCKET_WAVEFORMS at ~10-20 Hz
 *   - Publishes ipc_msg_sensor_status_t on sensor connect/disconnect
 */

#include "sensor_service.h"
#include "vitals_provider.h"
#include <stdio.h>
#include <string.h>

/*
 * On the target build these headers would be used:
 *   #include "sensor_hal.h"
 *   #include "ipc_messages.h"
 *   #include <nanomsg/nn.h>
 *   #include <nanomsg/pubsub.h>
 */

/* ── Internal state ───────────────────────────────────────── */

static bool     s_running          = false;
static uint32_t s_last_heartbeat_s = 0;
static uint32_t s_tick_count       = 0;

/* ── Simulator implementation ─────────────────────────────── */

#ifdef SIMULATOR_BUILD

bool sensor_service_init(void)
{
    printf("[sensor_service] Initialising (simulator mode)\n");

    int rc = vitals_provider_init();
    if (rc != 0) {
        printf("[sensor_service] vitals_provider_init() failed (rc=%d)\n", rc);
        return false;
    }

    s_running          = false;
    s_last_heartbeat_s = 0;
    s_tick_count       = 0;

    printf("[sensor_service] Initialised (provider=%s)\n",
           vitals_provider_get_type());
    return true;
}

void sensor_service_start(void)
{
    if (s_running) {
        printf("[sensor_service] Already running\n");
        return;
    }

    printf("[sensor_service] Starting vitals provider...\n");
    vitals_provider_start(1000);  /* 1-second update interval */
    s_running = true;
    printf("[sensor_service] Started\n");
}

void sensor_service_stop(void)
{
    if (!s_running) return;

    printf("[sensor_service] Stopping...\n");
    vitals_provider_stop();
    s_running = false;
    printf("[sensor_service] Stopped\n");
}

void sensor_service_tick(uint32_t current_time_s)
{
    if (!s_running) return;

    s_last_heartbeat_s = current_time_s;
    s_tick_count++;

    /*
     * In simulator mode the actual data generation is driven by the LVGL
     * timer in mock_data.c, so there is nothing to poll here.  The tick
     * just keeps the heartbeat alive for the service manager.
     */
}

void sensor_service_deinit(void)
{
    printf("[sensor_service] Deinitialising...\n");

    if (s_running) {
        sensor_service_stop();
    }

    vitals_provider_deinit();

    s_running          = false;
    s_last_heartbeat_s = 0;
    s_tick_count       = 0;

    printf("[sensor_service] Deinitialised\n");
}

#else /* TARGET BUILD */

/*
 * TARGET IMPLEMENTATION (skeleton)
 *
 * On the real hardware the sensor service would:
 *
 * 1. sensor_service_init():
 *    - Call sensor_hal_init_all() to initialise all registered sensors
 *    - Create nanomsg PUB sockets for vitals, waveforms, and sensor status
 *    - Bind to IPC_SOCKET_VITALS and IPC_SOCKET_WAVEFORMS
 *
 * 2. sensor_service_start():
 *    - Start the ECG acquisition via sensor_ops_ecg_t.start_acquisition()
 *    - Register waveform callbacks on SpO2 and ECG HALs
 *
 * 3. sensor_service_tick():
 *    - Poll each sensor HAL for latest readings
 *    - Build ipc_msg_vitals_t from sensor readings and nn_send()
 *    - Forward waveform callback data as ipc_msg_waveform_t via nn_send()
 *    - Detect sensor connect/disconnect and publish status messages
 *    - Feed sd_notify(0, "WATCHDOG=1") for systemd watchdog
 *
 * 4. sensor_service_stop():
 *    - Stop ECG acquisition
 *    - Close nanomsg sockets
 *
 * 5. sensor_service_deinit():
 *    - Call sensor_hal_deinit_all()
 */

bool sensor_service_init(void)
{
    printf("[sensor_service] Initialising (target mode - stub)\n");
    /* TODO: sensor_hal_init_all(), nn_socket(AF_SP, NN_PUB), nn_bind() */
    s_running = false;
    return true;
}

void sensor_service_start(void)
{
    printf("[sensor_service] Starting (target mode - stub)\n");
    /* TODO: start sensor acquisition, register waveform callbacks */
    s_running = true;
}

void sensor_service_stop(void)
{
    printf("[sensor_service] Stopping (target mode - stub)\n");
    /* TODO: stop acquisition, nn_close() */
    s_running = false;
}

void sensor_service_tick(uint32_t current_time_s)
{
    if (!s_running) return;
    s_last_heartbeat_s = current_time_s;
    s_tick_count++;

    /* TODO:
     * - Poll sensor HALs: spo2->get_reading(), ecg->get_reading(), etc.
     * - Build ipc_msg_vitals_t and nn_send() on vitals socket
     * - sd_notify(0, "WATCHDOG=1") to pet the systemd watchdog
     */
}

void sensor_service_deinit(void)
{
    printf("[sensor_service] Deinitialising (target mode - stub)\n");
    if (s_running) sensor_service_stop();
    /* TODO: sensor_hal_deinit_all() */
    s_running = false;
}

#endif /* SIMULATOR_BUILD */
