/**
 * @file patient_data.h
 * @brief SQLite-backed patient demographic data model
 *
 * Provides CRUD operations for patient records, monitor slot association,
 * and admission/discharge workflow. Designed for Indian hospital bedside
 * vitals monitors targeting CDSCO Class B approval.
 *
 * All data lives in a `patients` table inside the shared vitals_trends.db.
 * Access is single-threaded (LVGL main loop). Static allocation throughout.
 */

#ifndef PATIENT_DATA_H
#define PATIENT_DATA_H

#include <stdint.h>
#include <stdbool.h>

/* ── Field size limits ─────────────────────────────────────── */

#define PATIENT_NAME_MAX     64
#define PATIENT_MRN_MAX      32
#define PATIENT_FIELD_MAX    32
#define PATIENT_NOTES_MAX    256
#define PATIENT_MAX_STORED   8     /* Max patients in database */

/* ── Gender enumeration ────────────────────────────────────── */

typedef enum {
    PATIENT_GENDER_UNKNOWN = 0,
    PATIENT_GENDER_MALE,
    PATIENT_GENDER_FEMALE,
    PATIENT_GENDER_OTHER
} patient_gender_t;

/* ── Patient record ────────────────────────────────────────── */

typedef struct {
    int32_t  id;                              /* SQLite rowid (0 = not saved) */
    char     name[PATIENT_NAME_MAX];          /* Full name */
    char     mrn[PATIENT_MRN_MAX];            /* Medical Record Number */
    char     dob[PATIENT_FIELD_MAX];          /* Date of birth (YYYY-MM-DD) */
    patient_gender_t gender;
    char     blood_type[8];                   /* e.g., "O+", "AB-" */
    char     ward[PATIENT_FIELD_MAX];         /* Ward/department */
    char     bed[PATIENT_FIELD_MAX];          /* Bed identifier */
    char     attending[PATIENT_NAME_MAX];     /* Attending physician */
    float    weight_kg;                       /* 0 = not recorded */
    float    height_cm;                       /* 0 = not recorded */
    char     allergies[PATIENT_NOTES_MAX];    /* Comma-separated */
    char     diagnosis[PATIENT_NOTES_MAX];    /* Primary diagnosis */
    char     notes[PATIENT_NOTES_MAX];        /* Clinical notes */
    uint32_t admitted_ts;                     /* Admission timestamp (epoch seconds) */
    uint32_t discharged_ts;                   /* 0 = currently admitted */
    bool     active;                          /* Currently being monitored */
    uint8_t  monitor_slot;                    /* 0 or 1 (for dual-patient mode) */
} patient_t;

/* ── Query result list ─────────────────────────────────────── */

typedef struct {
    patient_t patients[PATIENT_MAX_STORED];
    int count;
} patient_list_t;

/* ── Lifecycle ─────────────────────────────────────────────── */

/** Open DB and create patients table. Pass NULL for in-memory DB. */
bool patient_data_init(const char *db_path);

/** Close the database and finalize all prepared statements. */
void patient_data_close(void);

/* ── CRUD ──────────────────────────────────────────────────── */

/** Insert or update a patient record. Sets patient->id on insert. */
bool patient_data_save(patient_t *patient);

/** Delete a patient record by ID. */
bool patient_data_delete(int32_t patient_id);

/** Retrieve a single patient by ID. Returns true if found. */
bool patient_data_get(int32_t patient_id, patient_t *out);

/* ── Queries ───────────────────────────────────────────────── */

/** List currently admitted (active) patients. Returns count. */
int patient_data_list_active(patient_list_t *out);

/** List all patients. Returns count. */
int patient_data_list_all(patient_list_t *out);

/** Find a patient by MRN. Returns true if found. */
bool patient_data_find_by_mrn(const char *mrn, patient_t *out);

/* ── Monitor association ───────────────────────────────────── */

/** Assign a patient to a monitor slot (0 or 1). */
bool patient_data_associate(int32_t patient_id, uint8_t slot);

/** Remove patient from a monitor slot. */
bool patient_data_disassociate(uint8_t slot);

/** Get the patient currently assigned to a slot (NULL if none). */
const patient_t *patient_data_get_active(uint8_t slot);

/* ── Convenience ───────────────────────────────────────────── */

/** Fill a patient_t with empty/default values. */
void patient_data_create_default(patient_t *out);

/** Set admitted_ts to now, active=true, and save. */
bool patient_data_admit(patient_t *patient);

/** Set discharged_ts to now, active=false, and save. */
bool patient_data_discharge(int32_t patient_id);

#endif /* PATIENT_DATA_H */
