/**
 * @file abdm_client.c
 * @brief ABDM client — simulator stub implementation
 *
 * All ABDM gateway operations are stubbed for the simulator build.
 * ABHA lookup returns a test patient; health record push logs the
 * operation and returns success. Authentication returns a mock token.
 *
 * The remote hardware team implements actual HTTPS transport via
 * libcurl or similar, calling the ABDM sandbox/production APIs.
 *
 * Mock test data:
 *   - Patient: Priya Sharma, ABHA 14-1234-5678-9012
 *   - Facility: FACILITY-HIP-001 (test HIP)
 *   - Token: mock-abdm-bearer-token-xxxx (expires in 1 hour)
 */

#include "abdm_client.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/* ── Module tag for printf logging ───────────────────────── */

#define TAG "[abdm_client] "

/* ── Module state ────────────────────────────────────────── */

static char           gateway_url[ABDM_ENDPOINT_MAX];
static char           facility_id[ABDM_FACILITY_ID_MAX];
static char           auth_token[ABDM_TOKEN_MAX];
static abdm_status_t  status;
static bool           initialized = false;
static int            mock_txn_counter = 5000;

/* ── Helper: get current epoch ms ────────────────────────── */

static uint64_t now_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
    }
    return (uint64_t)time(NULL) * 1000;
}

/* ── Lifecycle ───────────────────────────────────────────── */

void abdm_client_init(void) {
    memset(&status, 0, sizeof(status));
    memset(auth_token, 0, sizeof(auth_token));

    /* Default to ABDM sandbox gateway */
    strncpy(gateway_url, "https://dev.abdm.gov.in",
            sizeof(gateway_url) - 1);
    gateway_url[sizeof(gateway_url) - 1] = '\0';

    /* Default test facility */
    strncpy(facility_id, "FACILITY-HIP-001",
            sizeof(facility_id) - 1);
    facility_id[sizeof(facility_id) - 1] = '\0';

    status.state = ABDM_STATE_DISCONNECTED;
    mock_txn_counter = 5000;
    initialized = true;

    printf(TAG "Initialized (simulator stub)\n");
    printf(TAG "Gateway: %s\n", gateway_url);
    printf(TAG "Facility: %s\n", facility_id);
}

void abdm_client_deinit(void) {
    if (!initialized) return;

    memset(auth_token, 0, sizeof(auth_token));
    memset(&status, 0, sizeof(status));
    gateway_url[0] = '\0';
    facility_id[0] = '\0';
    initialized = false;

    printf(TAG "Deinitialized\n");
}

/* ── Configuration ───────────────────────────────────────── */

void abdm_client_set_gateway(const char *base_url) {
    if (!base_url) return;

    strncpy(gateway_url, base_url, sizeof(gateway_url) - 1);
    gateway_url[sizeof(gateway_url) - 1] = '\0';

    printf(TAG "Gateway set: %s\n", gateway_url);
}

const char *abdm_client_get_gateway(void) {
    return gateway_url;
}

void abdm_client_set_facility_id(const char *fid) {
    if (!fid) return;

    strncpy(facility_id, fid, sizeof(facility_id) - 1);
    facility_id[sizeof(facility_id) - 1] = '\0';

    printf(TAG "Facility ID set: %s\n", facility_id);
}

const char *abdm_client_get_facility_id(void) {
    return facility_id;
}

/* ── Authentication ──────────────────────────────────────── */

bool abdm_client_authenticate(const char *client_id, const char *client_secret) {
    if (!initialized) {
        printf(TAG "Cannot authenticate: not initialized\n");
        return false;
    }

    if (!client_id || !client_secret) {
        printf(TAG "Cannot authenticate: NULL credentials\n");
        status.state = ABDM_STATE_ERROR;
        strncpy(status.error_message, "NULL credentials provided",
                sizeof(status.error_message) - 1);
        return false;
    }

    printf(TAG "Authenticating with client_id=%s (mock)\n", client_id);

    status.state = ABDM_STATE_AUTHENTICATING;
    status.requests_sent++;
    status.last_request_ms = now_ms();

    /* Simulator stub: always succeeds with a mock token */
    snprintf(auth_token, sizeof(auth_token),
             "mock-abdm-bearer-token-%04d", mock_txn_counter++);

    /* Token "expires" in 1 hour */
    status.token_expiry_ms = now_ms() + (uint64_t)3600 * 1000;
    status.state = ABDM_STATE_CONNECTED;
    status.error_message[0] = '\0';

    printf(TAG "Authenticated successfully (mock token)\n");
    printf(TAG "Token expires at: +3600s\n");

    return true;
}

const char *abdm_get_auth_token(void) {
    return auth_token;
}

bool abdm_client_is_authenticated(void) {
    if (!initialized) return false;
    if (status.state != ABDM_STATE_CONNECTED) return false;

    /* Check token expiry */
    if (now_ms() > status.token_expiry_ms) {
        printf(TAG "Auth token expired\n");
        status.state = ABDM_STATE_TOKEN_EXPIRED;
        return false;
    }

    return true;
}

/* ── ABHA patient lookup ─────────────────────────────────── */

bool abdm_abha_lookup(const char *abha_id, abdm_patient_info_t *out) {
    if (!initialized || !abha_id || !out) {
        printf(TAG "ABHA lookup failed: invalid params or not initialized\n");
        return false;
    }

    memset(out, 0, sizeof(*out));

    printf(TAG "ABHA lookup: %s (mock)\n", abha_id);

    status.requests_sent++;
    status.last_request_ms = now_ms();

    /* Simulator stub: return a mock Indian patient */
    strncpy(out->abha_id, abha_id, ABDM_ABHA_ID_MAX - 1);
    out->abha_id[ABDM_ABHA_ID_MAX - 1] = '\0';

    strncpy(out->health_id, "priya.sharma@abdm", ABDM_HEALTH_ID_MAX - 1);
    out->health_id[ABDM_HEALTH_ID_MAX - 1] = '\0';

    strncpy(out->name, "Priya Sharma", ABDM_NAME_MAX - 1);
    out->name[ABDM_NAME_MAX - 1] = '\0';

    strncpy(out->gender, "F", ABDM_GENDER_MAX - 1);
    out->gender[ABDM_GENDER_MAX - 1] = '\0';

    strncpy(out->date_of_birth, "1990-05-15", ABDM_DOB_MAX - 1);
    out->date_of_birth[ABDM_DOB_MAX - 1] = '\0';

    strncpy(out->phone, "+91-98765-43210", ABDM_PHONE_MAX - 1);
    out->phone[ABDM_PHONE_MAX - 1] = '\0';

    out->year_of_birth = 1990;
    out->valid = true;

    printf(TAG "ABHA lookup result: name=%s gender=%s dob=%s (mock)\n",
           out->name, out->gender, out->date_of_birth);

    return true;
}

/* ── Health record operations ────────────────────────────── */

abdm_result_t abdm_push_health_record(const char *abha_id,
                                       const vitals_data_t *vitals) {
    abdm_result_t result;
    memset(&result, 0, sizeof(result));

    if (!initialized || !abha_id || !vitals) {
        result.success = false;
        result.http_status = 0;
        strncpy(result.error_message,
                "ABDM client not initialized or NULL parameters",
                sizeof(result.error_message) - 1);
        printf(TAG "Health record push failed: %s\n", result.error_message);
        return result;
    }

    if (!abdm_client_is_authenticated()) {
        result.success = false;
        result.http_status = 401;
        strncpy(result.error_message,
                "Not authenticated with ABDM gateway",
                sizeof(result.error_message) - 1);
        printf(TAG "Health record push failed: %s\n", result.error_message);
        status.requests_failed++;
        return result;
    }

    status.requests_sent++;
    status.last_request_ms = now_ms();

    printf(TAG "Pushing health record for ABHA=%s (mock)\n", abha_id);
    printf(TAG "  HR=%d SpO2=%d RR=%d Temp=%.1f BP=%d/%d\n",
           vitals->hr, vitals->spo2, vitals->rr, (double)vitals->temp,
           vitals->nibp_sys, vitals->nibp_dia);

    /* Simulator stub: always succeeds */
    result.success = true;
    result.http_status = 202;  /* 202 Accepted (async processing) */
    snprintf(result.transaction_id, sizeof(result.transaction_id),
             "ABDM-TXN-%04d", mock_txn_counter++);
    result.error_message[0] = '\0';

    printf(TAG "Health record pushed: txn=%s (mock)\n",
           result.transaction_id);

    return result;
}

/* ── Status ──────────────────────────────────────────────── */

const abdm_status_t *abdm_client_get_status(void) {
    return &status;
}

const char *abdm_client_state_str(abdm_conn_state_t state_val) {
    switch (state_val) {
        case ABDM_STATE_DISCONNECTED:   return "Disconnected";
        case ABDM_STATE_AUTHENTICATING: return "Authenticating";
        case ABDM_STATE_CONNECTED:      return "Connected";
        case ABDM_STATE_TOKEN_EXPIRED:  return "Token Expired";
        case ABDM_STATE_ERROR:          return "Error";
        default:                        return "Unknown";
    }
}
