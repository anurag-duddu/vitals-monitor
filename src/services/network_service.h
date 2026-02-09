/**
 * @file network_service.h
 * @brief Network connectivity and data synchronisation service
 *
 * Manages network connectivity monitoring and processes the outbound
 * data synchronisation queue when a connection is available.
 *
 * Architecture:
 *   - SIMULATOR (SIMULATOR_BUILD=1): Stubbed.  Reports as running but
 *     performs no actual network operations.  Network state can be
 *     injected via network_manager_set_mock_status() for UI testing.
 *   - TARGET: Monitors WiFi/Ethernet via network_manager, processes the
 *     sync_queue when connected, and reports upload progress.
 *
 * Service lifecycle callbacks are designed for use with service_manager.
 */

#ifndef NETWORK_SERVICE_H
#define NETWORK_SERVICE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise the network service.
 * In simulator mode: sets up internal state only.
 * On target: initialises network_manager and sync_queue.
 * @return true on success.
 */
bool network_service_init(void);

/**
 * Start network monitoring and sync processing.
 * In simulator mode: no-op beyond state tracking.
 * On target: begins periodic connectivity checks and queue draining.
 */
void network_service_start(void);

/**
 * Stop network operations.
 * In simulator mode: no-op beyond state tracking.
 * On target: pauses sync queue and disconnects.
 */
void network_service_stop(void);

/**
 * Periodic tick called by service_manager.
 * On target: checks connectivity and drains sync queue if connected.
 * Called approximately every 1 second; sync check runs every 30 seconds.
 * @param current_time_s  Monotonic time in seconds.
 */
void network_service_tick(uint32_t current_time_s);

/**
 * Release all resources.
 * On target: deinitialises network_manager and sync_queue.
 */
void network_service_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* NETWORK_SERVICE_H */
