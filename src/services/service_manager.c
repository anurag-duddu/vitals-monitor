/**
 * @file service_manager.c
 * @brief Service lifecycle manager implementation
 *
 * In SIMULATOR_BUILD mode all services run in-process on the LVGL main thread.
 * service_manager_tick() iterates the registered table, calls each service's
 * tick callback, monitors heartbeats, and auto-restarts crashed services.
 */

#include "service_manager.h"
#include <stdio.h>
#include <string.h>

/* ── Internal service entry ───────────────────────────────── */

typedef struct {
    service_registration_t reg;
    service_info_t         info;
    bool                   in_use;
} service_entry_t;

/* ── Static service table ─────────────────────────────────── */

static service_entry_t s_services[SERVICE_MAX];
static int             s_service_count = 0;
static bool            s_initialised   = false;

/* ── Private helpers ──────────────────────────────────────── */

/**
 * Find an entry by name.
 * @return Pointer to the entry, or NULL if not found.
 */
static service_entry_t *find_entry(const char *name)
{
    if (!name) return NULL;

    for (int i = 0; i < s_service_count; i++) {
        if (s_services[i].in_use &&
            strncmp(s_services[i].reg.name, name, SERVICE_NAME_MAX) == 0) {
            return &s_services[i];
        }
    }
    return NULL;
}

/**
 * Attempt to (re-)start a single entry.
 * @return true if init + start succeeded.
 */
static bool start_entry(service_entry_t *entry, uint32_t current_time_s)
{
    if (!entry) return false;

    const char *name = entry->reg.name;

    /* ── Init phase ───────────────────────────────────────── */
    entry->info.state = SERVICE_STATE_STARTING;
    printf("[service_manager] Starting '%s'...\n", name);

    if (entry->reg.init) {
        if (!entry->reg.init()) {
            printf("[service_manager] '%s' init FAILED\n", name);
            entry->info.state = SERVICE_STATE_ERROR;
            return false;
        }
    }

    /* ── Start phase ──────────────────────────────────────── */
    if (entry->reg.start) {
        entry->reg.start();
    }

    entry->info.state            = SERVICE_STATE_RUNNING;
    entry->info.start_time_s     = current_time_s;
    entry->info.last_heartbeat_s = current_time_s;
    printf("[service_manager] '%s' is RUNNING\n", name);
    return true;
}

/**
 * Gracefully stop a single entry.
 */
static void stop_entry(service_entry_t *entry)
{
    if (!entry) return;

    const char *name = entry->reg.name;

    if (entry->info.state == SERVICE_STATE_STOPPED) return;

    entry->info.state = SERVICE_STATE_STOPPING;
    printf("[service_manager] Stopping '%s'...\n", name);

    if (entry->reg.stop) {
        entry->reg.stop();
    }
    if (entry->reg.deinit) {
        entry->reg.deinit();
    }

    entry->info.state = SERVICE_STATE_STOPPED;
    printf("[service_manager] '%s' is STOPPED\n", name);
}

/* ── Public API ───────────────────────────────────────────── */

void service_manager_init(void)
{
    memset(s_services, 0, sizeof(s_services));
    s_service_count = 0;
    s_initialised   = true;
    printf("[service_manager] Initialised (max %d services)\n", SERVICE_MAX);
}

void service_manager_deinit(void)
{
    if (!s_initialised) return;

    service_manager_stop_all();

    memset(s_services, 0, sizeof(s_services));
    s_service_count = 0;
    s_initialised   = false;
    printf("[service_manager] Deinitialised\n");
}

bool service_manager_register(const service_registration_t *reg)
{
    if (!s_initialised || !reg || reg->name[0] == '\0') {
        printf("[service_manager] Register failed: invalid arguments\n");
        return false;
    }

    if (s_service_count >= SERVICE_MAX) {
        printf("[service_manager] Register failed: table full (%d/%d)\n",
               s_service_count, SERVICE_MAX);
        return false;
    }

    /* Check for duplicate name */
    if (find_entry(reg->name)) {
        printf("[service_manager] Register failed: '%s' already registered\n",
               reg->name);
        return false;
    }

    service_entry_t *entry = &s_services[s_service_count];
    memcpy(&entry->reg, reg, sizeof(service_registration_t));

    /* Initialise runtime info */
    memset(&entry->info, 0, sizeof(service_info_t));
    strncpy(entry->info.name, reg->name, SERVICE_NAME_MAX - 1);
    entry->info.name[SERVICE_NAME_MAX - 1] = '\0';
    entry->info.state        = SERVICE_STATE_STOPPED;
    entry->info.auto_restart = reg->auto_restart;
    entry->in_use            = true;

    s_service_count++;
    printf("[service_manager] Registered '%s' (auto_restart=%s)\n",
           reg->name, reg->auto_restart ? "true" : "false");
    return true;
}

bool service_manager_start(const char *name)
{
    service_entry_t *entry = find_entry(name);
    if (!entry) {
        printf("[service_manager] Start failed: '%s' not found\n",
               name ? name : "(null)");
        return false;
    }

    if (entry->info.state == SERVICE_STATE_RUNNING) {
        printf("[service_manager] '%s' already running\n", name);
        return true;
    }

    return start_entry(entry, 0);
}

bool service_manager_stop(const char *name)
{
    service_entry_t *entry = find_entry(name);
    if (!entry) {
        printf("[service_manager] Stop failed: '%s' not found\n",
               name ? name : "(null)");
        return false;
    }

    stop_entry(entry);
    return true;
}

bool service_manager_start_all(void)
{
    bool all_ok = true;
    printf("[service_manager] Starting all services (%d registered)...\n",
           s_service_count);

    for (int i = 0; i < s_service_count; i++) {
        if (!s_services[i].in_use) continue;
        if (s_services[i].info.state == SERVICE_STATE_RUNNING) continue;

        if (!start_entry(&s_services[i], 0)) {
            all_ok = false;
        }
    }
    return all_ok;
}

void service_manager_stop_all(void)
{
    printf("[service_manager] Stopping all services...\n");

    /* Stop in reverse order for clean dependency teardown */
    for (int i = s_service_count - 1; i >= 0; i--) {
        if (!s_services[i].in_use) continue;
        stop_entry(&s_services[i]);
    }
}

void service_manager_tick(uint32_t current_time_s)
{
    if (!s_initialised) return;

    for (int i = 0; i < s_service_count; i++) {
        service_entry_t *entry = &s_services[i];
        if (!entry->in_use) continue;

        /* ── Tick running services ────────────────────────── */
        if (entry->info.state == SERVICE_STATE_RUNNING) {
            if (entry->reg.tick) {
                entry->reg.tick(current_time_s);
            }

            /* Update heartbeat (service's tick is its heartbeat) */
            entry->info.last_heartbeat_s = current_time_s;

            continue;
        }

        /* ── Auto-restart crashed services ────────────────── */
        if (entry->info.state == SERVICE_STATE_ERROR && entry->info.auto_restart) {
            /* Avoid restart storm: require at least 3 seconds between attempts */
            uint32_t elapsed = current_time_s - entry->info.start_time_s;
            if (elapsed < 3) continue;

            printf("[service_manager] Auto-restarting '%s' "
                   "(restart_count=%u)\n",
                   entry->reg.name, entry->info.restart_count + 1);

            /* Ensure clean state before restart */
            if (entry->reg.stop)   entry->reg.stop();
            if (entry->reg.deinit) entry->reg.deinit();

            if (start_entry(entry, current_time_s)) {
                entry->info.restart_count++;
            }
        }
    }

    /* ── Heartbeat monitoring ─────────────────────────────── */
    /* Check after ticking so a service that just ticked is not falsely
     * flagged.  Only relevant when current_time_s is meaningful (> 0). */
    if (current_time_s > SERVICE_HEARTBEAT_TIMEOUT_S) {
        for (int i = 0; i < s_service_count; i++) {
            service_entry_t *entry = &s_services[i];
            if (!entry->in_use) continue;
            if (entry->info.state != SERVICE_STATE_RUNNING) continue;
            if (!entry->reg.tick) continue;  /* services without tick don't heartbeat */

            uint32_t delta = current_time_s - entry->info.last_heartbeat_s;
            if (delta > SERVICE_HEARTBEAT_TIMEOUT_S) {
                printf("[service_manager] '%s' heartbeat TIMEOUT "
                       "(last=%u, now=%u, delta=%u)\n",
                       entry->reg.name,
                       entry->info.last_heartbeat_s,
                       current_time_s, delta);
                entry->info.state = SERVICE_STATE_ERROR;
            }
        }
    }
}

/* ── Status queries ───────────────────────────────────────── */

const service_info_t *service_manager_get_info(const char *name)
{
    const service_entry_t *entry = find_entry(name);
    return entry ? &entry->info : NULL;
}

int service_manager_list(service_info_t *out, int max_count)
{
    if (!out || max_count <= 0) return 0;

    int written = 0;
    for (int i = 0; i < s_service_count && written < max_count; i++) {
        if (!s_services[i].in_use) continue;
        memcpy(&out[written], &s_services[i].info, sizeof(service_info_t));
        written++;
    }
    return written;
}

const char *service_state_str(service_state_t state)
{
    switch (state) {
        case SERVICE_STATE_STOPPED:  return "STOPPED";
        case SERVICE_STATE_STARTING: return "STARTING";
        case SERVICE_STATE_RUNNING:  return "RUNNING";
        case SERVICE_STATE_STOPPING: return "STOPPING";
        case SERVICE_STATE_ERROR:    return "ERROR";
        default:                     return "UNKNOWN";
    }
}
