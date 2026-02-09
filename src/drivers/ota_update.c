/**
 * @file ota_update.c
 * @brief OTA update stub implementation
 *
 * Returns safe default values for the simulator build. On the target,
 * this file is replaced by an implementation that wraps SWUpdate and
 * the A/B partition boot control.
 */

#include "ota_update.h"
#include <stdio.h>
#include <string.h>

/* ============================================================
 *  Static State
 * ============================================================ */

static ota_status_t s_status = {
    .state        = OTA_STATE_IDLE,
    .progress_pct = 0,
    .version      = "",
    .error_msg    = "",
};

/* ============================================================
 *  API Implementation (Stubs)
 * ============================================================ */

bool ota_check_for_update(const char *update_url) {
    (void)update_url;
    printf("[ota_update] check_for_update (stub): no update available\n");
    return false;
}

bool ota_start_update(const char *update_path) {
    (void)update_path;
    printf("[ota_update] start_update (stub): not supported in simulator\n");
    s_status.state = OTA_STATE_ERROR;
    snprintf(s_status.error_msg, sizeof(s_status.error_msg),
             "OTA not supported in simulator build");
    return false;
}

const ota_status_t *ota_get_status(void) {
    return &s_status;
}

bool ota_rollback(void) {
    printf("[ota_update] rollback (stub): not supported in simulator\n");
    return false;
}

const char *ota_get_current_version(void) {
    return "1.0.0-sim";
}

const char *ota_get_active_partition(void) {
    return "sim";
}
