/**
 * @file screen_patient.h
 * @brief Patient info screen — demographics and admission data
 */

#ifndef SCREEN_PATIENT_H
#define SCREEN_PATIENT_H

#include "lvgl.h"

/** Create the patient screen (called by screen manager). */
lv_obj_t * screen_patient_create(void);

/** Destroy / release widgets (called by screen manager). */
void screen_patient_destroy(void);

#endif /* SCREEN_PATIENT_H */
