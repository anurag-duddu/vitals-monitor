/**
 * @file network_service.c
 * @brief Network connectivity and data synchronisation service implementation
 *
 * SIMULATOR_BUILD:
 *   Stubbed implementation.  Reports as running but performs no real network
 *   operations.  The network_manager mock status can be injected for UI
 *   testing of the settings/network screens.
 *
 * TARGET (documented, not yet implemented):
 *   - Initialises network_manager for WiFi/Ethernet management
 *   - Initialises sync_queue for outbound data buffering
 *   - Every SYNC_CHECK_INTERVAL_S seconds (30s), checks connectivity
 *   - If connected, drains the sync_queue by POSTing records to the
 *     hospital EMR/cloud endpoint
 *   - Reports upload progress and errors
 */

#include "network_service.h"
#include "network_manager.h"
#include <stdio.h>
#include <string.h>

/*
 * On the target build these headers would also be used:
 *   #include "sync_queue.h"
 *   #include <curl/curl.h>   (or similar HTTP client)
 */

/* ── Configuration ────────────────────────────────────────── */

/** How often (in seconds) the sync logic runs within tick(). */
#define SYNC_CHECK_INTERVAL_S  30

/* ── Internal state ───────────────────────────────────────── */

static bool     s_running          = false;
static uint32_t s_last_heartbeat_s = 0;
static uint32_t s_tick_count       = 0;
static uint32_t s_last_sync_time_s = 0;

/* ── Simulator implementation ─────────────────────────────── */

#ifdef SIMULATOR_BUILD

bool network_service_init(void)
{
    printf("[network_service] Initialising (simulator mode - stub)\n");

    s_running          = false;
    s_last_heartbeat_s = 0;
    s_tick_count       = 0;
    s_last_sync_time_s = 0;

    printf("[network_service] Initialised\n");
    return true;
}

void network_service_start(void)
{
    if (s_running) {
        printf("[network_service] Already running\n");
        return;
    }

    s_running = true;
    printf("[network_service] Started (simulator - no-op)\n");
}

void network_service_stop(void)
{
    if (!s_running) return;

    printf("[network_service] Stopping...\n");
    s_running = false;
    printf("[network_service] Stopped\n");
}

void network_service_tick(uint32_t current_time_s)
{
    if (!s_running) return;

    s_last_heartbeat_s = current_time_s;
    s_tick_count++;

    /*
     * In simulator mode we do nothing on tick.  The network_manager
     * mock can be used from the settings screen to test UI states,
     * but no actual sync processing occurs.
     */
}

void network_service_deinit(void)
{
    printf("[network_service] Deinitialising...\n");

    if (s_running) {
        network_service_stop();
    }

    s_running          = false;
    s_last_heartbeat_s = 0;
    s_tick_count       = 0;
    s_last_sync_time_s = 0;

    printf("[network_service] Deinitialised\n");
}

#else /* TARGET BUILD */

/*
 * TARGET IMPLEMENTATION (skeleton)
 *
 * On the real hardware the network service would:
 *
 * 1. network_service_init():
 *    - Call network_manager_init()
 *    - Call sync_queue_init() to open the persistent outbound queue
 *    - Optionally initialise an HTTP client (curl_global_init)
 *
 * 2. network_service_start():
 *    - Start periodic connectivity monitoring
 *
 * 3. network_service_tick():
 *    - Every SYNC_CHECK_INTERVAL_S seconds:
 *      a. Call network_manager_is_connected()
 *      b. If connected, call sync_queue_peek() to get next record
 *      c. POST the record to the configured EMR endpoint
 *      d. On success, call sync_queue_pop() to remove it
 *      e. Repeat until queue is empty or network drops
 *    - sd_notify(0, "WATCHDOG=1") for systemd watchdog
 *
 * 4. network_service_stop():
 *    - Abort in-flight HTTP requests
 *    - Flush sync_queue to persistent storage
 *
 * 5. network_service_deinit():
 *    - sync_queue_deinit()
 *    - network_manager_deinit()
 *    - curl_global_cleanup()
 */

bool network_service_init(void)
{
    printf("[network_service] Initialising (target mode - stub)\n");
    /* TODO: network_manager_init(), sync_queue_init() */
    s_running = false;
    return true;
}

void network_service_start(void)
{
    printf("[network_service] Starting (target mode - stub)\n");
    s_running = true;
}

void network_service_stop(void)
{
    printf("[network_service] Stopping (target mode - stub)\n");
    /* TODO: abort HTTP requests, flush sync_queue */
    s_running = false;
}

void network_service_tick(uint32_t current_time_s)
{
    if (!s_running) return;
    s_last_heartbeat_s = current_time_s;
    s_tick_count++;

    /* Only run sync logic every SYNC_CHECK_INTERVAL_S seconds */
    if (current_time_s - s_last_sync_time_s < SYNC_CHECK_INTERVAL_S) {
        return;
    }
    s_last_sync_time_s = current_time_s;

    /* TODO:
     * if (network_manager_is_connected()) {
     *     while (sync_queue_count() > 0) {
     *         sync_record_t *rec = sync_queue_peek();
     *         if (http_post(rec) == 0) {
     *             sync_queue_pop();
     *         } else {
     *             break;  // network error, retry next cycle
     *         }
     *     }
     * }
     * sd_notify(0, "WATCHDOG=1");
     */
}

void network_service_deinit(void)
{
    printf("[network_service] Deinitialising (target mode - stub)\n");
    if (s_running) network_service_stop();
    /* TODO: sync_queue_deinit(), network_manager_deinit() */
    s_running = false;
}

#endif /* SIMULATOR_BUILD */
