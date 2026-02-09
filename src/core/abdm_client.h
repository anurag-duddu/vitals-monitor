/**
 * @file abdm_client.h
 * @brief ABDM (Ayushman Bharat Digital Mission) client for health data exchange
 *
 * Integrates with India's ABDM Health Information Exchange (HIE-CM):
 *   - ABHA ID-based patient lookup
 *   - Health record push (vitals observation bundles)
 *   - Gateway authentication (client credentials flow)
 *   - Consent-based data sharing status
 *
 * In the simulator build, all operations are stubbed:
 *   - ABHA lookup returns a test patient
 *   - Health record push logs success
 *   - Auth token returns a mock bearer token
 *
 * The remote hardware team implements actual HTTPS transport via
 * the ABDM sandbox/production gateway APIs:
 *   - Sandbox: https://dev.abdm.gov.in
 *   - Production: https://live.abdm.gov.in
 *
 * ABDM API references:
 *   - Health Information Exchange: HIP (Health Information Provider) APIs
 *   - ABHA verification: /v1/auth/init, /v1/auth/confirm-with-aadhaar
 *   - Health record link: /v1/links/link/add-contexts
 *
 * Thread safety: All calls are single-threaded (LVGL main loop).
 */

#ifndef ABDM_CLIENT_H
#define ABDM_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include "vitals_provider.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ── Constants ─────────────────────────────────────────────── */

#define ABDM_ABHA_ID_MAX       32      /* Max ABHA address length (e.g. "14-xxxx-xxxx-xxxx") */
#define ABDM_NAME_MAX          64      /* Max patient name length */
#define ABDM_GENDER_MAX        16      /* Max gender string length */
#define ABDM_DOB_MAX           16      /* Max date of birth string (YYYY-MM-DD) */
#define ABDM_PHONE_MAX         16      /* Max phone number length */
#define ABDM_TOKEN_MAX         512     /* Max auth token length */
#define ABDM_ENDPOINT_MAX      256     /* Max gateway URL length */
#define ABDM_ERROR_MSG_MAX     128     /* Max error message length */
#define ABDM_HEALTH_ID_MAX     64      /* Max health ID (PHR address) */
#define ABDM_FACILITY_ID_MAX   64      /* Max facility (HIP) ID */

/* ── Connection state ──────────────────────────────────────── */

typedef enum {
    ABDM_STATE_DISCONNECTED = 0,    /* Not authenticated */
    ABDM_STATE_AUTHENTICATING,      /* Auth request in progress */
    ABDM_STATE_CONNECTED,           /* Authenticated, token valid */
    ABDM_STATE_TOKEN_EXPIRED,       /* Token expired, needs refresh */
    ABDM_STATE_ERROR,               /* Authentication failed */
} abdm_conn_state_t;

/* ── Status structure ──────────────────────────────────────── */

typedef struct {
    abdm_conn_state_t   state;
    uint64_t            token_expiry_ms;        /* Epoch ms when token expires */
    uint32_t            requests_sent;          /* Cumulative API requests */
    uint32_t            requests_failed;        /* Cumulative failed requests */
    uint64_t            last_request_ms;        /* Epoch ms of last request */
    char                error_message[ABDM_ERROR_MSG_MAX];
} abdm_status_t;

/* ── Patient info from ABHA lookup ─────────────────────────── */

typedef struct {
    char    abha_id[ABDM_ABHA_ID_MAX];         /* ABHA number (14-xxxx-xxxx-xxxx) */
    char    health_id[ABDM_HEALTH_ID_MAX];      /* PHR address (user@abdm) */
    char    name[ABDM_NAME_MAX];                /* Full name */
    char    gender[ABDM_GENDER_MAX];            /* "M", "F", "O", "U" */
    char    date_of_birth[ABDM_DOB_MAX];        /* YYYY-MM-DD */
    char    phone[ABDM_PHONE_MAX];              /* Mobile number */
    int     year_of_birth;                      /* Year of birth */
    bool    valid;                              /* True if lookup succeeded */
} abdm_patient_info_t;

/* ── Health record push result ─────────────────────────────── */

typedef struct {
    bool    success;
    char    transaction_id[ABDM_HEALTH_ID_MAX]; /* ABDM transaction reference */
    int     http_status;                        /* HTTP status code */
    char    error_message[ABDM_ERROR_MSG_MAX];
} abdm_result_t;

/* ── Lifecycle ─────────────────────────────────────────────── */

/**
 * Initialize the ABDM client module.
 * Sets default gateway endpoint and clears internal state.
 */
void abdm_client_init(void);

/**
 * Shut down the ABDM client and release resources.
 * Invalidates any active auth token.
 */
void abdm_client_deinit(void);

/* ── Configuration ─────────────────────────────────────────── */

/**
 * Set the ABDM gateway base URL.
 * @param base_url  Gateway URL (e.g. "https://dev.abdm.gov.in")
 */
void abdm_client_set_gateway(const char *base_url);

/** Get the currently configured ABDM gateway URL. */
const char *abdm_client_get_gateway(void);

/**
 * Set the HIP (Health Information Provider) facility ID.
 * Required for health record push operations.
 * @param facility_id  Facility ID registered with ABDM.
 */
void abdm_client_set_facility_id(const char *facility_id);

/** Get the currently configured facility ID. */
const char *abdm_client_get_facility_id(void);

/* ── Authentication ────────────────────────────────────────── */

/**
 * Authenticate with the ABDM gateway using client credentials.
 * On success, an auth token is stored internally for subsequent calls.
 *
 * @param client_id      Registered client ID.
 * @param client_secret  Client secret.
 * @return true if authentication succeeded (token stored).
 */
bool abdm_client_authenticate(const char *client_id, const char *client_secret);

/**
 * Get the current auth token (for custom API calls).
 * @return Token string, or empty string if not authenticated.
 */
const char *abdm_get_auth_token(void);

/**
 * Check if the current auth token is valid (not expired).
 * @return true if authenticated and token has not expired.
 */
bool abdm_client_is_authenticated(void);

/* ── ABHA patient lookup ───────────────────────────────────── */

/**
 * Look up a patient by ABHA ID (14-digit health ID number).
 * Queries the ABDM gateway for demographics associated with the ABHA.
 *
 * @param abha_id  ABHA number (e.g. "14-1234-5678-9012").
 * @param out      Output patient info struct (caller-allocated).
 * @return true if lookup succeeded and patient was found.
 */
bool abdm_abha_lookup(const char *abha_id, abdm_patient_info_t *out);

/* ── Health record operations ──────────────────────────────── */

/**
 * Push a vitals observation as a health record to ABDM.
 * The record is formatted as a FHIR Observation bundle and submitted
 * to the HIP data-push endpoint.
 *
 * @param abha_id  Patient's ABHA ID for record linkage.
 * @param vitals   Current vitals snapshot to push.
 * @return Result with transaction ID on success.
 */
abdm_result_t abdm_push_health_record(const char *abha_id,
                                       const vitals_data_t *vitals);

/* ── Status ────────────────────────────────────────────────── */

/**
 * Get a read-only pointer to current ABDM client status.
 * @return Pointer to status struct (valid until next API call).
 */
const abdm_status_t *abdm_client_get_status(void);

/**
 * Get human-readable string for an ABDM connection state.
 * @param state  Connection state value.
 * @return Static string describing the state.
 */
const char *abdm_client_state_str(abdm_conn_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* ABDM_CLIENT_H */
