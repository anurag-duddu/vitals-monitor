/**
 * @file sensor_hal.c
 * @brief Sensor HAL registry implementation
 *
 * Minimal implementation providing the registration and lookup mechanism.
 * No actual hardware access -- that is provided by individual sensor drivers
 * (e.g., hal_ecg_ads1292r.c, hal_spo2_max30102.c) which register themselves
 * via sensor_hal_register().
 */

#include "sensor_hal.h"
#include <stdio.h>
#include <string.h>

/* ============================================================
 *  Static Registry
 * ============================================================ */

static const sensor_hal_t *hal_registry[SENSOR_COUNT] = {0};

/* ============================================================
 *  Registration
 * ============================================================ */

void sensor_hal_register(sensor_type_t type, const sensor_hal_t *hal) {
    if (type < SENSOR_COUNT) {
        hal_registry[type] = hal;
        printf("[sensor_hal] Registered: %s\n", hal->name);
    }
}

const sensor_hal_t *sensor_hal_get(sensor_type_t type) {
    return (type < SENSOR_COUNT) ? hal_registry[type] : NULL;
}

/* ============================================================
 *  Bulk Init / Deinit
 * ============================================================ */

bool sensor_hal_init_all(void) {
    bool all_ok = true;
    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (hal_registry[i] && hal_registry[i]->init) {
            if (!hal_registry[i]->init()) {
                printf("[sensor_hal] Failed to init: %s\n",
                       hal_registry[i]->name);
                all_ok = false;
            }
        }
    }
    return all_ok;
}

void sensor_hal_deinit_all(void) {
    for (int i = SENSOR_COUNT - 1; i >= 0; i--) {
        if (hal_registry[i] && hal_registry[i]->deinit) {
            hal_registry[i]->deinit();
        }
    }
}
