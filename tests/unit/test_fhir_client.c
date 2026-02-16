/**
 * @file test_fhir_client.c
 * @brief Unit tests for fhir_client module (TEST-5.3)
 *
 * Tests FHIR R4 JSON builders, endpoint URL validation, export/import
 * stubs, buffer overflow handling, NULL safety, and JSON string escaping.
 */

#include "test_framework.h"
#include "fhir_client.h"
#include <string.h>

/* Static buffer for JSON output shared across tests */
static char json_buf[4096];

/* Helper: create a populated fhir_vitals_observation_t */
static fhir_vitals_observation_t make_test_observation(void) {
    fhir_vitals_observation_t obs;
    memset(&obs, 0, sizeof(obs));
    obs.hr       = 72;
    obs.spo2     = 98;
    obs.rr       = 16;
    obs.temp     = 36.8f;
    obs.nibp_sys = 120;
    obs.nibp_dia = 80;
    obs.timestamp_ms = 1700000000000ULL; /* 2023-11-14 ~22:13 UTC */
    strncpy(obs.patient_id, "patient-12345", sizeof(obs.patient_id) - 1);
    return obs;
}

/* ── Test 1: init/deinit lifecycle ───────────────────────── */

static void test_init_deinit_lifecycle(void) {
    printf("  test_init_deinit_lifecycle\n");

    fhir_client_init();

    /* After init, endpoint should be set to default */
    const char *ep = fhir_client_get_endpoint();
    ASSERT_NOT_NULL(ep);
    ASSERT_GT_INT((int)strlen(ep), 0);

    /* Should contain https */
    ASSERT_NOT_NULL(strstr(ep, "https://"));

    fhir_client_deinit();

    /* After deinit, endpoint should be empty */
    ep = fhir_client_get_endpoint();
    ASSERT_NOT_NULL(ep);
    ASSERT_EQ_INT((int)strlen(ep), 0);
}

/* ── Test 2: double deinit is safe ───────────────────────── */

static void test_double_deinit_safe(void) {
    printf("  test_double_deinit_safe\n");

    fhir_client_init();
    fhir_client_deinit();

    /* Second deinit should not crash (early-return on !initialized) */
    fhir_client_deinit();

    ASSERT_TRUE(1);  /* Reached here without crash */
}

/* ── Test 3: HTTPS endpoint accepted ─────────────────────── */

static void test_endpoint_https_accepted(void) {
    printf("  test_endpoint_https_accepted\n");

    fhir_client_init();

    fhir_client_set_endpoint("https://fhir.hospital.in/r4");
    const char *ep = fhir_client_get_endpoint();
    ASSERT_STR_EQ(ep, "https://fhir.hospital.in/r4");

    fhir_client_deinit();
}

/* ── Test 4: http://localhost allowed for dev ─────────────── */

static void test_endpoint_localhost_allowed(void) {
    printf("  test_endpoint_localhost_allowed\n");

    fhir_client_init();

    fhir_client_set_endpoint("http://localhost:8080/fhir");
    const char *ep = fhir_client_get_endpoint();
    ASSERT_STR_EQ(ep, "http://localhost:8080/fhir");

    /* Also test 127.0.0.1 */
    fhir_client_set_endpoint("http://127.0.0.1:9090/r4");
    ep = fhir_client_get_endpoint();
    ASSERT_STR_EQ(ep, "http://127.0.0.1:9090/r4");

    fhir_client_deinit();
}

/* ── Test 5: plain HTTP rejected ─────────────────────────── */

static void test_endpoint_plain_http_rejected(void) {
    printf("  test_endpoint_plain_http_rejected\n");

    fhir_client_init();

    /* Set a known-good endpoint first */
    fhir_client_set_endpoint("https://fhir.good.example.com/r4");
    const char *original = fhir_client_get_endpoint();

    /* Attempt to set a plain HTTP (non-localhost) URL */
    fhir_client_set_endpoint("http://fhir.evil.example.com/r4");

    /* Should still have the original endpoint (rejected URL is ignored) */
    const char *ep = fhir_client_get_endpoint();
    ASSERT_STR_EQ(ep, original);

    fhir_client_deinit();
}

/* ── Test 6: NULL endpoint is rejected ───────────────────── */

static void test_endpoint_null_rejected(void) {
    printf("  test_endpoint_null_rejected\n");

    fhir_client_init();
    const char *before = fhir_client_get_endpoint();

    fhir_client_set_endpoint(NULL);

    /* Should not change */
    ASSERT_STR_EQ(fhir_client_get_endpoint(), before);

    fhir_client_deinit();
}

/* ── Test 7: build observation JSON — FHIR fields ────────── */

static void test_build_observation_json_fields(void) {
    printf("  test_build_observation_json_fields\n");

    fhir_client_init();

    fhir_vitals_observation_t obs = make_test_observation();
    int len = fhir_client_build_observation_json(&obs, json_buf, sizeof(json_buf));

    ASSERT_GT_INT(len, 0);

    /* Verify FHIR resource type */
    ASSERT_NOT_NULL(strstr(json_buf, "\"resourceType\":\"Observation\""));

    /* Verify vital-signs category */
    ASSERT_NOT_NULL(strstr(json_buf, "\"code\":\"vital-signs\""));

    /* Verify component array exists */
    ASSERT_NOT_NULL(strstr(json_buf, "\"component\":["));

    /* Verify LOINC codes for each vital sign */
    ASSERT_NOT_NULL(strstr(json_buf, "\"code\":\"8867-4\""));  /* Heart Rate */
    ASSERT_NOT_NULL(strstr(json_buf, "\"code\":\"2708-6\""));  /* SpO2 */
    ASSERT_NOT_NULL(strstr(json_buf, "\"code\":\"9279-1\""));  /* Resp Rate */
    ASSERT_NOT_NULL(strstr(json_buf, "\"code\":\"8310-5\""));  /* Temperature */
    ASSERT_NOT_NULL(strstr(json_buf, "\"code\":\"8480-6\""));  /* BP Systolic */
    ASSERT_NOT_NULL(strstr(json_buf, "\"code\":\"8462-4\""));  /* BP Diastolic */

    /* Verify patient reference */
    ASSERT_NOT_NULL(strstr(json_buf, "\"reference\":\"Patient/patient-12345\""));

    /* Verify effectiveDateTime present */
    ASSERT_NOT_NULL(strstr(json_buf, "\"effectiveDateTime\":\""));

    fhir_client_deinit();
}

/* ── Test 8: build patient JSON — verify fields ──────────── */

static void test_build_patient_json_fields(void) {
    printf("  test_build_patient_json_fields\n");

    fhir_client_init();

    int len = fhir_client_build_patient_json(
        "Rajesh Kumar", "MRN-2024-001234", "1985-03-15", "male",
        json_buf, sizeof(json_buf));

    ASSERT_GT_INT(len, 0);

    /* Verify FHIR resource type */
    ASSERT_NOT_NULL(strstr(json_buf, "\"resourceType\":\"Patient\""));

    /* Verify MRN identifier */
    ASSERT_NOT_NULL(strstr(json_buf, "\"code\":\"MR\""));
    ASSERT_NOT_NULL(strstr(json_buf, "\"value\":\"MRN-2024-001234\""));

    /* Verify name split: "Rajesh" is given, "Kumar" is family */
    ASSERT_NOT_NULL(strstr(json_buf, "\"family\":\"Kumar\""));
    ASSERT_NOT_NULL(strstr(json_buf, "\"given\":[\"Rajesh\"]"));

    /* Verify birthDate */
    ASSERT_NOT_NULL(strstr(json_buf, "\"birthDate\":\"1985-03-15\""));

    /* Verify gender */
    ASSERT_NOT_NULL(strstr(json_buf, "\"gender\":\"male\""));

    fhir_client_deinit();
}

/* ── Test 9: JSON string escaping — quotes and backslashes ── */

static void test_json_string_escaping(void) {
    printf("  test_json_string_escaping\n");

    fhir_client_init();

    /* Patient name with quotes and backslashes */
    int len = fhir_client_build_patient_json(
        "Raj \"The Doc\" Ku\\mar", "MRN-001", "1990-01-01", "male",
        json_buf, sizeof(json_buf));

    ASSERT_GT_INT(len, 0);

    /* The quotes should be escaped as \" in the JSON output */
    /* The backslash should be escaped as \\ in the JSON output */
    /* The JSON should still be valid (not truncated or errored) */
    ASSERT_NOT_NULL(strstr(json_buf, "\"resourceType\":\"Patient\""));

    /* Verify the escaped backslash appears (Ku\\mar becomes Ku\\\\mar in JSON) */
    ASSERT_NOT_NULL(strstr(json_buf, "Ku\\\\mar"));

    fhir_client_deinit();
}

/* ── Test 10: buffer too small — observation returns -1 ───── */

static void test_observation_buffer_too_small(void) {
    printf("  test_observation_buffer_too_small\n");

    fhir_client_init();

    fhir_vitals_observation_t obs = make_test_observation();
    char tiny_buf[32];  /* Way too small for a FHIR Observation JSON */
    int len = fhir_client_build_observation_json(&obs, tiny_buf, sizeof(tiny_buf));

    ASSERT_EQ_INT(len, -1);

    fhir_client_deinit();
}

/* ── Test 11: buffer too small — patient returns -1 ──────── */

static void test_patient_buffer_too_small(void) {
    printf("  test_patient_buffer_too_small\n");

    fhir_client_init();

    char tiny_buf[16];  /* Way too small for a FHIR Patient JSON */
    int len = fhir_client_build_patient_json(
        "Test Name", "MRN-001", "2000-01-01", "male",
        tiny_buf, sizeof(tiny_buf));

    ASSERT_EQ_INT(len, -1);

    fhir_client_deinit();
}

/* ── Test 12: NULL observation returns -1 ────────────────── */

static void test_null_observation_returns_error(void) {
    printf("  test_null_observation_returns_error\n");

    fhir_client_init();

    int len = fhir_client_build_observation_json(NULL, json_buf, sizeof(json_buf));
    ASSERT_EQ_INT(len, -1);

    fhir_client_deinit();
}

/* ── Test 13: NULL name for patient JSON returns -1 ──────── */

static void test_null_name_patient_json(void) {
    printf("  test_null_name_patient_json\n");

    fhir_client_init();

    int len = fhir_client_build_patient_json(
        NULL, "MRN-001", "2000-01-01", "male",
        json_buf, sizeof(json_buf));
    ASSERT_EQ_INT(len, -1);

    fhir_client_deinit();
}

/* ── Test 14: export observation (stub) ──────────────────── */

static void test_export_observation_stub(void) {
    printf("  test_export_observation_stub\n");

    fhir_client_init();

    fhir_vitals_observation_t obs = make_test_observation();
    fhir_result_t result = fhir_client_export_observation(&obs);

    ASSERT_TRUE(result.success);
    ASSERT_EQ_INT(result.http_status, 201);
    ASSERT_GT_INT((int)strlen(result.resource_id), 0);
    ASSERT_NOT_NULL(strstr(result.resource_id, "obs-"));
    ASSERT_EQ_INT((int)strlen(result.error_message), 0);

    fhir_client_deinit();
}

/* ── Test 15: export patient (stub) ──────────────────────── */

static void test_export_patient_stub(void) {
    printf("  test_export_patient_stub\n");

    fhir_client_init();

    fhir_result_t result = fhir_client_export_patient(
        "Rajesh Kumar", "MRN-2024-001234", "1985-03-15", "male");

    ASSERT_TRUE(result.success);
    ASSERT_EQ_INT(result.http_status, 201);
    ASSERT_GT_INT((int)strlen(result.resource_id), 0);
    ASSERT_NOT_NULL(strstr(result.resource_id, "pat-"));
    ASSERT_EQ_INT((int)strlen(result.error_message), 0);

    fhir_client_deinit();
}

/* ── Test 16: import patient (stub returns mock data) ────── */

static void test_import_patient_stub(void) {
    printf("  test_import_patient_stub\n");

    fhir_client_init();

    char name_out[64] = "";
    char mrn_out[32] = "";

    bool ok = fhir_client_import_patient("patient-12345",
                                          name_out, sizeof(name_out),
                                          mrn_out, sizeof(mrn_out));

    ASSERT_TRUE(ok);
    ASSERT_STR_EQ(name_out, "Rajesh Kumar");
    ASSERT_STR_EQ(mrn_out, "MRN-2024-001234");

    fhir_client_deinit();
}

/* ── Test 17: export observation without init returns failure */

static void test_export_without_init_fails(void) {
    printf("  test_export_without_init_fails\n");

    /* Ensure deinit state: init then deinit to clear any prior state */
    fhir_client_init();
    fhir_client_deinit();

    fhir_vitals_observation_t obs = make_test_observation();
    fhir_result_t result = fhir_client_export_observation(&obs);

    ASSERT_FALSE(result.success);
    ASSERT_EQ_INT(result.http_status, 0);
    ASSERT_GT_INT((int)strlen(result.error_message), 0);
}

/* ── Test 18: export patient without init returns failure ── */

static void test_export_patient_without_init_fails(void) {
    printf("  test_export_patient_without_init_fails\n");

    fhir_client_init();
    fhir_client_deinit();

    fhir_result_t result = fhir_client_export_patient(
        "Test Name", "MRN-001", "2000-01-01", "male");

    ASSERT_FALSE(result.success);
    ASSERT_EQ_INT(result.http_status, 0);
    ASSERT_GT_INT((int)strlen(result.error_message), 0);
}

/* ── Test 19: import patient without init returns false ──── */

static void test_import_without_init_fails(void) {
    printf("  test_import_without_init_fails\n");

    fhir_client_init();
    fhir_client_deinit();

    char name_out[64] = "";
    char mrn_out[32] = "";

    bool ok = fhir_client_import_patient("patient-12345",
                                          name_out, sizeof(name_out),
                                          mrn_out, sizeof(mrn_out));
    ASSERT_FALSE(ok);
}

/* ── Test 20: export observation with NULL obs returns failure */

static void test_export_null_observation_fails(void) {
    printf("  test_export_null_observation_fails\n");

    fhir_client_init();

    fhir_result_t result = fhir_client_export_observation(NULL);

    ASSERT_FALSE(result.success);
    ASSERT_GT_INT((int)strlen(result.error_message), 0);

    fhir_client_deinit();
}

/* ── Test 21: consecutive exports produce unique resource IDs */

static void test_export_unique_ids(void) {
    printf("  test_export_unique_ids\n");

    fhir_client_init();

    fhir_vitals_observation_t obs = make_test_observation();

    fhir_result_t r1 = fhir_client_export_observation(&obs);
    fhir_result_t r2 = fhir_client_export_observation(&obs);

    ASSERT_TRUE(r1.success);
    ASSERT_TRUE(r2.success);

    /* Resource IDs should differ (mock counter increments) */
    ASSERT_TRUE(strcmp(r1.resource_id, r2.resource_id) != 0);

    fhir_client_deinit();
}

/* ── Public entry point ──────────────────────────────────── */

void test_fhir_client(void) {
    test_init_deinit_lifecycle();
    test_double_deinit_safe();
    test_endpoint_https_accepted();
    test_endpoint_localhost_allowed();
    test_endpoint_plain_http_rejected();
    test_endpoint_null_rejected();
    test_build_observation_json_fields();
    test_build_patient_json_fields();
    test_json_string_escaping();
    test_observation_buffer_too_small();
    test_patient_buffer_too_small();
    test_null_observation_returns_error();
    test_null_name_patient_json();
    test_export_observation_stub();
    test_export_patient_stub();
    test_import_patient_stub();
    test_export_without_init_fails();
    test_export_patient_without_init_fails();
    test_import_without_init_fails();
    test_export_null_observation_fails();
    test_export_unique_ids();
}
