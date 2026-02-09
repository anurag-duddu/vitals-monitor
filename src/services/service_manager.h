/**
 * @file service_manager.h
 * @brief Service lifecycle manager for multi-service vitals monitor architecture
 *
 * Manages registration, startup, shutdown, heartbeat monitoring, and automatic
 * restart of application services.
 *
 * Architecture:
 *   - SIMULATOR (SIMULATOR_BUILD=1): All services run in-process on the LVGL
 *     main thread.  service_manager_tick() calls each service's tick callback
 *     directly from the main loop.
 *   - TARGET: Each service runs as a separate Linux process.  The systemd unit
 *     files in deploy/systemd/ handle process lifecycle.  This manager is used
 *     inside each process to register the single local service and manage its
 *     heartbeat toward the watchdog.
 *
 * Design:
 *   - Static allocation throughout (no malloc)
 *   - Maximum SERVICE_MAX services registered simultaneously
 *   - 30-second heartbeat timeout; services that fail to tick are marked ERROR
 *   - Auto-restart: services with auto_restart=true are restarted on ERROR
 */

#ifndef SERVICE_MANAGER_H
#define SERVICE_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Constants ────────────────────────────────────────────── */

#define SERVICE_MAX       8
#define SERVICE_NAME_MAX  32

/** Heartbeat timeout in seconds.  If a service does not tick within
 *  this window it is marked SERVICE_STATE_ERROR. */
#define SERVICE_HEARTBEAT_TIMEOUT_S  30

/* ── Service state enumeration ────────────────────────────── */

typedef enum {
    SERVICE_STATE_STOPPED = 0,
    SERVICE_STATE_STARTING,
    SERVICE_STATE_RUNNING,
    SERVICE_STATE_STOPPING,
    SERVICE_STATE_ERROR
} service_state_t;

/* ── Runtime information for a registered service ─────────── */

typedef struct {
    char            name[SERVICE_NAME_MAX];
    service_state_t state;
    uint32_t        start_time_s;
    uint32_t        restart_count;
    uint32_t        last_heartbeat_s;
    bool            auto_restart;
} service_info_t;

/* ── Service lifecycle callbacks ──────────────────────────── */

/** Return true on successful initialisation. */
typedef bool (*service_init_fn)(void);

/** Begin service work (called after init succeeds). */
typedef void (*service_start_fn)(void);

/** Gracefully stop the service. */
typedef void (*service_stop_fn)(void);

/** Periodic tick called from main loop.
 *  @param current_time_s  Monotonic seconds (e.g., SDL_GetTicks/1000). */
typedef void (*service_tick_fn)(uint32_t current_time_s);

/** Release all resources. */
typedef void (*service_deinit_fn)(void);

/* ── Registration descriptor ──────────────────────────────── */

typedef struct {
    char              name[SERVICE_NAME_MAX];
    service_init_fn   init;
    service_start_fn  start;
    service_stop_fn   stop;
    service_tick_fn   tick;          /* NULL if service needs no periodic updates */
    service_deinit_fn deinit;
    bool              auto_restart;  /* Restart automatically on crash / error */
} service_registration_t;

/* ── Lifecycle ────────────────────────────────────────────── */

/** Initialise the service manager (clears all registrations). */
void service_manager_init(void);

/** Deinitialise the service manager (stops and deinits all services). */
void service_manager_deinit(void);

/* ── Service management ───────────────────────────────────── */

/**
 * Register a service with the manager.
 * @param reg  Pointer to registration descriptor (contents are copied).
 * @return true on success, false if table is full or name is empty.
 */
bool service_manager_register(const service_registration_t *reg);

/**
 * Start a single registered service by name.
 * Calls init() then start().
 * @return true on success, false if not found or init fails.
 */
bool service_manager_start(const char *name);

/**
 * Stop a single registered service by name.
 * Calls stop() then deinit().
 * @return true on success, false if not found.
 */
bool service_manager_stop(const char *name);

/** Start all registered services in registration order. */
bool service_manager_start_all(void);

/** Stop all registered services in reverse registration order. */
void service_manager_stop_all(void);

/* ── Periodic tick ────────────────────────────────────────── */

/**
 * Called from the main loop each iteration.
 * - Calls tick() on each running service
 * - Checks heartbeat timeouts
 * - Auto-restarts crashed services when auto_restart is true
 *
 * @param current_time_s  Current monotonic time in seconds.
 */
void service_manager_tick(uint32_t current_time_s);

/* ── Status queries ───────────────────────────────────────── */

/**
 * Get runtime information for a named service.
 * @return Pointer to internal info struct (valid until next register/deinit),
 *         or NULL if not found.
 */
const service_info_t *service_manager_get_info(const char *name);

/**
 * List all registered services.
 * @param out        Caller-provided array to receive service_info_t copies.
 * @param max_count  Size of the out array.
 * @return Number of entries written (<= max_count).
 */
int service_manager_list(service_info_t *out, int max_count);

/** Get human-readable string for a service_state_t value. */
const char *service_state_str(service_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* SERVICE_MANAGER_H */
