/**
 * @file sensor_service.h
 * @brief Sensor data acquisition service
 *
 * Responsible for acquiring vital signs data from hardware sensors and
 * making it available to other services and the UI.
 *
 * Architecture:
 *   - SIMULATOR (SIMULATOR_BUILD=1): Thin wrapper around the mock data
 *     generator via vitals_provider.  All data flows through the existing
 *     LVGL timer-based mock pipeline.
 *   - TARGET: Reads real sensors through sensor_hal, packages data into
 *     IPC messages (ipc_messages.h), and publishes via nanomsg PUB socket
 *     on IPC_SOCKET_VITALS and IPC_SOCKET_WAVEFORMS.
 *
 * Service lifecycle callbacks are designed for use with service_manager.
 */

#ifndef SENSOR_SERVICE_H
#define SENSOR_SERVICE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise the sensor service.
 * In simulator mode: calls vitals_provider_init().
 * On target: opens sensor HAL and binds IPC publish sockets.
 * @return true on success.
 */
bool sensor_service_init(void);

/**
 * Start acquiring sensor data.
 * In simulator mode: calls vitals_provider_start(1000).
 * On target: begins sensor polling loop and publishes via IPC.
 */
void sensor_service_start(void);

/**
 * Stop acquiring sensor data.
 * In simulator mode: calls vitals_provider_stop().
 * On target: halts sensor polling and closes publish sockets.
 */
void sensor_service_stop(void);

/**
 * Periodic tick called by service_manager.
 * Updates the service heartbeat and (on target) polls sensors.
 * @param current_time_s  Monotonic time in seconds.
 */
void sensor_service_tick(uint32_t current_time_s);

/**
 * Release all resources.
 * In simulator mode: calls vitals_provider_deinit().
 * On target: shuts down sensor HAL and releases IPC resources.
 */
void sensor_service_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* SENSOR_SERVICE_H */
