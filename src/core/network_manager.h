/**
 * @file network_manager.h
 * @brief Network connectivity management for WiFi and Ethernet interfaces
 *
 * Provides an abstraction layer for network operations:
 *   - WiFi scanning, connect/disconnect
 *   - Connection state tracking (status, SSID, signal, IP)
 *   - Mock status injection for simulator testing
 *
 * In the simulator build, all operations are stubbed. The remote hardware
 * team implements actual WiFi/Ethernet drivers behind this interface.
 *
 * Thread safety: All calls are single-threaded (LVGL main loop).
 */

#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Connection status ─────────────────────────────────────── */

typedef enum {
    NET_STATUS_DISCONNECTED = 0,
    NET_STATUS_CONNECTING,
    NET_STATUS_CONNECTED,
    NET_STATUS_ERROR
} net_status_t;

/* ── Network interface type ────────────────────────────────── */

typedef enum {
    NET_IF_WIFI = 0,
    NET_IF_ETHERNET,
    NET_IF_COUNT
} net_interface_t;

/* ── Network state snapshot ────────────────────────────────── */

typedef struct {
    net_status_t    status;
    net_interface_t active_interface;
    char            ssid[33];              /* Connected WiFi SSID (max 32 + NUL) */
    int             signal_strength;       /* -100 to 0 dBm (0 = not available) */
    char            ip_address[16];        /* IPv4 dotted decimal */
    bool            internet_reachable;    /* Can reach external services */
    uint32_t        last_connected_ts;     /* Epoch seconds of last connection */
    uint32_t        bytes_sent;            /* Cumulative bytes sent */
    uint32_t        bytes_received;        /* Cumulative bytes received */
} net_state_t;

/* ── WiFi scan results ─────────────────────────────────────── */

#define NET_WIFI_MAX_SCAN  16

typedef struct {
    char ssid[33];
    int  signal_dbm;
    bool secured;
} net_wifi_ap_t;

typedef struct {
    net_wifi_ap_t aps[NET_WIFI_MAX_SCAN];
    int           count;
} net_wifi_scan_result_t;

/* ── Lifecycle ─────────────────────────────────────────────── */

/** Initialize the network manager. */
void network_manager_init(void);

/** Shut down the network manager and release resources. */
void network_manager_deinit(void);

/* ── State queries ─────────────────────────────────────────── */

/** Get a read-only pointer to current network state. */
const net_state_t *network_manager_get_state(void);

/** Returns true if status == NET_STATUS_CONNECTED. */
bool network_manager_is_connected(void);

/* ── WiFi operations (stubbed in simulator) ────────────────── */

/**
 * Scan for available WiFi access points.
 * @param result  Output buffer for scan results.
 * @return true on success (simulator always returns true with mock APs).
 */
bool network_manager_wifi_scan(net_wifi_scan_result_t *result);

/**
 * Connect to a WiFi access point.
 * @param ssid      SSID to connect to (max 32 chars).
 * @param password  WPA2 passphrase (NULL for open networks).
 * @return true if connection initiated successfully.
 */
bool network_manager_wifi_connect(const char *ssid, const char *password);

/**
 * Disconnect from the current WiFi network.
 * @return true if disconnect succeeded.
 */
bool network_manager_wifi_disconnect(void);

/* ── Mock / simulator helpers ──────────────────────────────── */

/** Inject a mock connection status (simulator testing). */
void network_manager_set_mock_status(net_status_t status);

/** Get human-readable string for a net_status_t value. */
const char *network_manager_status_str(net_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* NETWORK_MANAGER_H */
