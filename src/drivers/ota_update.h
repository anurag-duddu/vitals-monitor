/**
 * @file ota_update.h
 * @brief Over-the-Air update interface (Phase 11)
 *
 * Provides an abstraction over the SWUpdate A/B partition scheme used on
 * the STM32MP157F-DK2 target. The UI and network-service use this API to
 * check for updates, apply them, and roll back on failure.
 *
 * On the simulator build, all functions return safe stub values so the UI
 * can render the update screen without real OTA infrastructure.
 *
 * TARGET IMPLEMENTATION NOTES:
 *   - Uses SWUpdate (swupdate) under the hood
 *   - A/B partition layout: rootfsA + rootfsB, one active, one standby
 *   - Update packages are .swu files signed with RSA-2048 + SHA-256
 *   - On successful boot after update, the bootloader marks the new
 *     partition as confirmed; otherwise it rolls back automatically
 *   - ota_rollback() forces a revert to the previous partition
 */

#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 *  OTA State Machine
 * ============================================================ */

typedef enum {
    OTA_STATE_IDLE = 0,       /* No update in progress */
    OTA_STATE_DOWNLOADING,    /* Fetching update from server or USB */
    OTA_STATE_VERIFYING,      /* Checking signature and integrity */
    OTA_STATE_INSTALLING,     /* Writing to standby partition */
    OTA_STATE_REBOOTING,      /* About to reboot into new partition */
    OTA_STATE_ERROR           /* Something went wrong -- see error_msg */
} ota_state_t;

/* ============================================================
 *  OTA Status Structure
 * ============================================================ */

typedef struct {
    ota_state_t state;
    uint8_t progress_pct;     /* 0-100, meaningful during DOWNLOADING/INSTALLING */
    char version[32];         /* Version string of the pending/installing update */
    char error_msg[128];      /* Human-readable error if state == OTA_STATE_ERROR */
} ota_status_t;

/* ============================================================
 *  OTA API
 * ============================================================ */

/**
 * Check if an update is available at the given URL.
 * Connects to the update server and compares versions.
 *
 * @param update_url  HTTPS URL of the update manifest
 * @return true if a newer version is available
 */
bool ota_check_for_update(const char *update_url);

/**
 * Start downloading and installing an update.
 * Progress can be monitored via ota_get_status().
 *
 * @param update_path  URL or local path (e.g., /mnt/usb/update.swu)
 * @return true if the update process started successfully
 */
bool ota_start_update(const char *update_path);

/**
 * Get the current OTA status.
 *
 * @return Pointer to static status structure (valid until next call)
 */
const ota_status_t *ota_get_status(void);

/**
 * Force a rollback to the previous partition.
 * Marks the standby partition as active and requests a reboot.
 *
 * @return true if rollback was initiated
 */
bool ota_rollback(void);

/**
 * Get the currently running firmware version string.
 *
 * @return Null-terminated version string (e.g., "1.0.0" or "1.0.0-sim")
 */
const char *ota_get_current_version(void);

/**
 * Get the currently active partition label.
 *
 * @return "A", "B", or "sim" (simulator)
 */
const char *ota_get_active_partition(void);

#ifdef __cplusplus
}
#endif

#endif /* OTA_UPDATE_H */
