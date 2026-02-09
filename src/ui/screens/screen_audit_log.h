/**
 * @file screen_audit_log.h
 * @brief Audit log viewer screen -- displays audit trail entries
 *
 * Shows a scrollable, filterable list of audit log entries with
 * timestamp, event type, username, and message. Requires
 * AUTH_PERM_VIEW_AUDIT_LOG permission (Admin role).
 */

#ifndef SCREEN_AUDIT_LOG_H
#define SCREEN_AUDIT_LOG_H

#include "lvgl.h"

/** Create the audit log screen (called by screen manager). */
lv_obj_t *screen_audit_log_create(void);

/** Destroy / release widgets (called by screen manager). */
void screen_audit_log_destroy(void);

#endif /* SCREEN_AUDIT_LOG_H */
