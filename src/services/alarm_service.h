/**
 * @file alarm_service.h
 * @brief Alarm evaluation and notification service
 *
 * Evaluates vital signs data against configurable thresholds using the
 * alarm_engine module and manages alarm state transitions.
 *
 * Architecture:
 *   - SIMULATOR (SIMULATOR_BUILD=1): Runs in-process.  On each tick, reads
 *     the current vitals from vitals_provider and calls alarm_engine_evaluate()
 *     directly.  The UI reads alarm state via alarm_engine_get_state().
 *   - TARGET: Runs as a separate process.  Subscribes to IPC_SOCKET_VITALS
 *     via nanomsg SUB socket, evaluates alarms, and publishes alarm events
 *     on IPC_SOCKET_ALARMS.  Also drives the buzzer/LED hardware.
 *
 * Service lifecycle callbacks are designed for use with service_manager.
 */

#ifndef ALARM_SERVICE_H
#define ALARM_SERVICE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise the alarm service.
 * In simulator mode: calls alarm_engine_init().
 * On target: opens IPC subscribe socket and initialises alarm engine.
 * @return true on success.
 */
bool alarm_service_init(void);

/**
 * Start alarm evaluation.
 * In simulator mode: begins evaluating on each tick.
 * On target: starts the IPC subscriber loop.
 */
void alarm_service_start(void);

/**
 * Stop alarm evaluation.
 * In simulator mode: stops evaluating.
 * On target: unsubscribes from IPC and silences hardware alarms.
 */
void alarm_service_stop(void);

/**
 * Periodic tick called by service_manager.
 * Fetches current vitals, evaluates thresholds, updates alarm state.
 * @param current_time_s  Monotonic time in seconds.
 */
void alarm_service_tick(uint32_t current_time_s);

/**
 * Release all resources.
 * In simulator mode: calls alarm_engine_deinit().
 * On target: closes IPC sockets and releases hardware resources.
 */
void alarm_service_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* ALARM_SERVICE_H */
