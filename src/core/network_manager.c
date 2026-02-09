/**
 * @file network_manager.c
 * @brief Network connectivity management — simulator stub implementation
 *
 * All network operations are stubbed for the simulator build.
 * WiFi scan returns 3 fake access points; connect/disconnect update
 * internal state only.  The remote hardware team will replace these
 * stubs with actual WiFi/Ethernet driver calls.
 */

#include "network_manager.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/* ── Module state ────────────────────────────────────────── */

static net_state_t state;
static bool        initialized = false;

/* ── Lifecycle ───────────────────────────────────────────── */

void network_manager_init(void) {
    memset(&state, 0, sizeof(state));
    state.status           = NET_STATUS_DISCONNECTED;
    state.active_interface = NET_IF_WIFI;
    state.signal_strength  = 0;
    state.internet_reachable = false;
    initialized = true;

    printf("[network_manager] Initialized (simulator stub)\n");
}

void network_manager_deinit(void) {
    if (!initialized) return;

    memset(&state, 0, sizeof(state));
    initialized = false;

    printf("[network_manager] Deinitialized\n");
}

/* ── State queries ───────────────────────────────────────── */

const net_state_t *network_manager_get_state(void) {
    return &state;
}

bool network_manager_is_connected(void) {
    return state.status == NET_STATUS_CONNECTED;
}

/* ── WiFi operations (simulator stubs) ───────────────────── */

bool network_manager_wifi_scan(net_wifi_scan_result_t *result) {
    if (!initialized || !result) return false;

    memset(result, 0, sizeof(*result));

    /* Return 3 mock access points typical for an Indian hospital */
    strncpy(result->aps[0].ssid, "Hospital-Staff-5G", 32);
    result->aps[0].signal_dbm = -45;
    result->aps[0].secured    = true;

    strncpy(result->aps[1].ssid, "MedDevice-IoT", 32);
    result->aps[1].signal_dbm = -62;
    result->aps[1].secured    = true;

    strncpy(result->aps[2].ssid, "Guest-WiFi", 32);
    result->aps[2].signal_dbm = -78;
    result->aps[2].secured    = false;

    result->count = 3;

    printf("[network_manager] WiFi scan complete: %d APs found (mock)\n",
           result->count);
    return true;
}

bool network_manager_wifi_connect(const char *ssid, const char *password) {
    if (!initialized || !ssid) return false;

    (void)password;  /* Not used in simulator stub */

    /* Simulate successful connection */
    state.status           = NET_STATUS_CONNECTED;
    state.active_interface = NET_IF_WIFI;
    strncpy(state.ssid, ssid, sizeof(state.ssid) - 1);
    state.ssid[sizeof(state.ssid) - 1] = '\0';
    state.signal_strength  = -50;
    strncpy(state.ip_address, "192.168.1.42", sizeof(state.ip_address) - 1);
    state.ip_address[sizeof(state.ip_address) - 1] = '\0';
    state.internet_reachable = true;
    state.last_connected_ts  = (uint32_t)time(NULL);

    printf("[network_manager] Connected to '%s' (mock)\n", state.ssid);
    return true;
}

bool network_manager_wifi_disconnect(void) {
    if (!initialized) return false;

    state.status            = NET_STATUS_DISCONNECTED;
    state.ssid[0]           = '\0';
    state.signal_strength   = 0;
    state.ip_address[0]     = '\0';
    state.internet_reachable = false;

    printf("[network_manager] Disconnected (mock)\n");
    return true;
}

/* ── Mock helpers ────────────────────────────────────────── */

void network_manager_set_mock_status(net_status_t status_val) {
    state.status = status_val;

    /* Update related fields based on mock status */
    if (status_val == NET_STATUS_CONNECTED) {
        state.internet_reachable = true;
        state.signal_strength    = -50;
        state.last_connected_ts  = (uint32_t)time(NULL);
        if (state.ip_address[0] == '\0') {
            strncpy(state.ip_address, "192.168.1.42",
                    sizeof(state.ip_address) - 1);
            state.ip_address[sizeof(state.ip_address) - 1] = '\0';
        }
    } else if (status_val == NET_STATUS_DISCONNECTED ||
               status_val == NET_STATUS_ERROR) {
        state.internet_reachable = false;
        state.signal_strength    = 0;
    }

    printf("[network_manager] Mock status set: %s\n",
           network_manager_status_str(status_val));
}

const char *network_manager_status_str(net_status_t status_val) {
    switch (status_val) {
        case NET_STATUS_DISCONNECTED: return "Disconnected";
        case NET_STATUS_CONNECTING:   return "Connecting";
        case NET_STATUS_CONNECTED:    return "Connected";
        case NET_STATUS_ERROR:        return "Error";
        default:                      return "Unknown";
    }
}
