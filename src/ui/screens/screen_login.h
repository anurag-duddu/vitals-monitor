/**
 * @file screen_login.h
 * @brief PIN-entry login screen — authentication gate before vitals display
 */

#ifndef SCREEN_LOGIN_H
#define SCREEN_LOGIN_H

#include "lvgl.h"

/** Create the login screen (called by screen manager). */
lv_obj_t * screen_login_create(void);

/** Destroy / release widgets (called by screen manager). */
void screen_login_destroy(void);

#endif /* SCREEN_LOGIN_H */
