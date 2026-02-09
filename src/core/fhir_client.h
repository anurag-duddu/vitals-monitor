/**
 * @file fhir_client.h
 * @brief FHIR R4 client for vitals export and patient import
 *
 * Generates standards-compliant FHIR R4 JSON resources:
 *   - Observation (vital-signs category) with LOINC-coded components
 *   - Patient with MRN identifier, name, birthDate, gender
 *
 * In the simulator build, export/import operations are stubbed (no HTTP).
 * The JSON builders produce valid FHIR R4 JSON for verification and testing.
 * The remote hardware team implements actual HTTP transport.
 *
 * LOINC codes used:
 *   Heart Rate:    8867-4
 *   SpO2:          2708-6
 *   Resp Rate:     9279-1
 *   Temperature:   8310-5
 *   BP Systolic:   8480-6
 *   BP Diastolic:  8462-4
 *
 * Thread safety: All calls are single-threaded (LVGL main loop).
 */

#ifndef FHIR_CLIENT_H
#define FHIR_CLIENT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Constants ─────────────────────────────────────────────── */

#define FHIR_JSON_MAX      4096    /* Max JSON buffer size */
#define FHIR_ENDPOINT_MAX  256     /* Max FHIR server URL length */
#define FHIR_ID_MAX        64      /* Max resource ID length */

/* ── Resource types ────────────────────────────────────────── */

typedef enum {
    FHIR_RESOURCE_PATIENT = 0,
    FHIR_RESOURCE_OBSERVATION,
    FHIR_RESOURCE_DEVICE,
    FHIR_RESOURCE_COUNT
} fhir_resource_type_t;

/* ── Export result ─────────────────────────────────────────── */

typedef struct {
    bool success;
    char resource_id[FHIR_ID_MAX];     /* Server-assigned resource ID */
    int  http_status;                   /* HTTP status code */
    char error_message[128];            /* Error description (empty on success) */
} fhir_result_t;

/* ── Vitals observation for FHIR export ────────────────────── */

typedef struct {
    int      hr;                        /* Heart rate (bpm) */
    int      spo2;                      /* SpO2 (%) */
    int      rr;                        /* Respiratory rate (breaths/min) */
    float    temp;                      /* Temperature (Celsius) */
    int      nibp_sys;                  /* Systolic BP (mmHg) */
    int      nibp_dia;                  /* Diastolic BP (mmHg) */
    uint64_t timestamp_ms;              /* Epoch milliseconds (UTC) */
    char     patient_id[FHIR_ID_MAX];   /* FHIR patient reference */
} fhir_vitals_observation_t;

/* ── Lifecycle ─────────────────────────────────────────────── */

/** Initialize the FHIR client. */
void fhir_client_init(void);

/** Shut down the FHIR client and release resources. */
void fhir_client_deinit(void);

/* ── Configuration ─────────────────────────────────────────── */

/**
 * Set the FHIR server base URL.
 * @param base_url  e.g. "https://fhir.hospital.in/r4"
 */
void fhir_client_set_endpoint(const char *base_url);

/** Get the currently configured FHIR server base URL. */
const char *fhir_client_get_endpoint(void);

/* ── JSON builders (produce valid FHIR R4 JSON) ───────────── */

/**
 * Build a FHIR Observation resource (vital-signs) as JSON.
 * @param obs       Vitals observation data.
 * @param buf       Output buffer for JSON.
 * @param buf_size  Size of output buffer.
 * @return Number of bytes written (excluding NUL), or -1 on error.
 */
int fhir_client_build_observation_json(const fhir_vitals_observation_t *obs,
                                        char *buf, int buf_size);

/**
 * Build a FHIR Patient resource as JSON.
 * @param name    Patient full name (e.g. "Rajesh Kumar").
 * @param mrn     Medical Record Number.
 * @param dob     Date of birth in ISO format (e.g. "1985-03-15").
 * @param gender  FHIR gender code: "male", "female", "other", "unknown".
 * @param buf     Output buffer for JSON.
 * @param buf_size Size of output buffer.
 * @return Number of bytes written (excluding NUL), or -1 on error.
 */
int fhir_client_build_patient_json(const char *name, const char *mrn,
                                    const char *dob, const char *gender,
                                    char *buf, int buf_size);

/* ── Export operations (stubbed in simulator) ──────────────── */

/**
 * Export a vitals observation to the FHIR server.
 * @param obs  Vitals observation to export.
 * @return Result with success status and server-assigned ID.
 */
fhir_result_t fhir_client_export_observation(const fhir_vitals_observation_t *obs);

/**
 * Export a patient resource to the FHIR server.
 * @return Result with success status and server-assigned ID.
 */
fhir_result_t fhir_client_export_patient(const char *name, const char *mrn,
                                          const char *dob, const char *gender);

/* ── Import operations (stubbed in simulator) ──────────────── */

/**
 * Import patient demographics from FHIR server.
 * @param patient_id  FHIR patient resource ID to fetch.
 * @param name_out    Output buffer for patient name.
 * @param name_max    Size of name output buffer.
 * @param mrn_out     Output buffer for MRN.
 * @param mrn_max     Size of MRN output buffer.
 * @return true on success.
 */
bool fhir_client_import_patient(const char *patient_id,
                                 char *name_out, int name_max,
                                 char *mrn_out, int mrn_max);

#ifdef __cplusplus
}
#endif

#endif /* FHIR_CLIENT_H */
