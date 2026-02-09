/**
 * @file fhir_client.c
 * @brief FHIR R4 client — JSON builders and simulator stub transport
 *
 * JSON generation uses snprintf (no JSON library dependency).
 * All export/import operations are stubbed for the simulator build;
 * the remote hardware team implements actual HTTP transport via
 * libcurl or similar.
 *
 * FHIR R4 compliance:
 *   - Observation uses vital-signs category with LOINC-coded components
 *   - Patient uses MRN identifier system, HumanName, birthDate, gender
 */

#include "fhir_client.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

/* ── LOINC codes for vital signs ─────────────────────────── */

#define LOINC_HR       "8867-4"
#define LOINC_SPO2     "2708-6"
#define LOINC_RR       "9279-1"
#define LOINC_TEMP     "8310-5"
#define LOINC_BP_SYS   "8480-6"
#define LOINC_BP_DIA   "8462-4"

/* ── Module state ────────────────────────────────────────── */

static char    endpoint_url[FHIR_ENDPOINT_MAX];
static bool    initialized = false;
static int     mock_id_counter = 1000;

/* ── Lifecycle ───────────────────────────────────────────── */

void fhir_client_init(void) {
    memset(endpoint_url, 0, sizeof(endpoint_url));
    strncpy(endpoint_url, "https://fhir.example.hospital.in/r4",
            sizeof(endpoint_url) - 1);
    mock_id_counter = 1000;
    initialized = true;

    printf("[fhir_client] Initialized (simulator stub)\n");
    printf("[fhir_client] Default endpoint: %s\n", endpoint_url);
}

void fhir_client_deinit(void) {
    if (!initialized) return;

    endpoint_url[0] = '\0';
    initialized = false;

    printf("[fhir_client] Deinitialized\n");
}

/* ── Configuration ───────────────────────────────────────── */

void fhir_client_set_endpoint(const char *base_url) {
    if (!base_url) return;

    strncpy(endpoint_url, base_url, sizeof(endpoint_url) - 1);
    endpoint_url[sizeof(endpoint_url) - 1] = '\0';

    printf("[fhir_client] Endpoint set: %s\n", endpoint_url);
}

const char *fhir_client_get_endpoint(void) {
    return endpoint_url;
}

/* ── Helper: format ISO-8601 timestamp from epoch ms ─────── */

static void format_iso_timestamp(uint64_t timestamp_ms, char *buf, int buf_size) {
    time_t secs = (time_t)(timestamp_ms / 1000);
    struct tm *tm_info = gmtime(&secs);
    if (tm_info) {
        snprintf(buf, buf_size, "%04d-%02d-%02dT%02d:%02d:%02dZ",
                 tm_info->tm_year + 1900, tm_info->tm_mon + 1,
                 tm_info->tm_mday, tm_info->tm_hour,
                 tm_info->tm_min, tm_info->tm_sec);
    } else {
        strncpy(buf, "1970-01-01T00:00:00Z", buf_size - 1);
        buf[buf_size - 1] = '\0';
    }
}

/* ── Helper: append component to observation JSON ────────── */

static int append_component(char *buf, int buf_size, int offset,
                             const char *loinc_code, const char *display,
                             const char *value_fmt, ...) {
    /* Build the component JSON fragment */
    char component[512];
    int n;

    /* Value portion — use va_args for flexible formatting */
    char value_part[128];
    {
        va_list args;
        va_start(args, value_fmt);
        vsnprintf(value_part, sizeof(value_part), value_fmt, args);
        va_end(args);
    }

    n = snprintf(component, sizeof(component),
        "{"
            "\"code\":{"
                "\"coding\":[{"
                    "\"system\":\"http://loinc.org\","
                    "\"code\":\"%s\","
                    "\"display\":\"%s\""
                "}]"
            "},"
            "%s"
        "}",
        loinc_code, display, value_part);

    if (n < 0 || offset + n >= buf_size) return offset;

    memcpy(buf + offset, component, n);
    return offset + n;
}

/* ── JSON builders ───────────────────────────────────────── */

int fhir_client_build_observation_json(const fhir_vitals_observation_t *obs,
                                        char *buf, int buf_size) {
    if (!obs || !buf || buf_size < 256) return -1;

    /* Format the timestamp */
    char ts_str[32];
    format_iso_timestamp(obs->timestamp_ms, ts_str, sizeof(ts_str));

    /* Build the observation header */
    int offset = snprintf(buf, buf_size,
        "{"
            "\"resourceType\":\"Observation\","
            "\"status\":\"final\","
            "\"category\":[{"
                "\"coding\":[{"
                    "\"system\":\"http://terminology.hl7.org/CodeSystem/observation-category\","
                    "\"code\":\"vital-signs\","
                    "\"display\":\"Vital Signs\""
                "}]"
            "}],"
            "\"code\":{"
                "\"coding\":[{"
                    "\"system\":\"http://loinc.org\","
                    "\"code\":\"85353-1\","
                    "\"display\":\"Vital signs, weight, height, head circumference, oxygen saturation and BMI panel\""
                "}],"
                "\"text\":\"Vital Signs Panel\""
            "},"
            "\"subject\":{"
                "\"reference\":\"Patient/%s\""
            "},"
            "\"effectiveDateTime\":\"%s\","
            "\"component\":[",
        obs->patient_id,
        ts_str);

    if (offset < 0 || offset >= buf_size) return -1;

    /* Track whether we need a comma separator */
    bool needs_comma = false;

    /* Heart Rate */
    if (obs->hr > 0) {
        if (needs_comma) { buf[offset++] = ','; }
        offset = append_component(buf, buf_size, offset,
            LOINC_HR, "Heart rate",
            "\"valueQuantity\":{\"value\":%d,\"unit\":\"/min\","
            "\"system\":\"http://unitsofmeasure.org\",\"code\":\"/min\"}",
            obs->hr);
        needs_comma = true;
    }

    /* SpO2 */
    if (obs->spo2 > 0) {
        if (needs_comma) { buf[offset++] = ','; }
        offset = append_component(buf, buf_size, offset,
            LOINC_SPO2, "Oxygen saturation in Arterial blood",
            "\"valueQuantity\":{\"value\":%d,\"unit\":\"%%\","
            "\"system\":\"http://unitsofmeasure.org\",\"code\":\"%%\"}",
            obs->spo2);
        needs_comma = true;
    }

    /* Respiratory Rate */
    if (obs->rr > 0) {
        if (needs_comma) { buf[offset++] = ','; }
        offset = append_component(buf, buf_size, offset,
            LOINC_RR, "Respiratory rate",
            "\"valueQuantity\":{\"value\":%d,\"unit\":\"/min\","
            "\"system\":\"http://unitsofmeasure.org\",\"code\":\"/min\"}",
            obs->rr);
        needs_comma = true;
    }

    /* Temperature */
    if (obs->temp > 0.0f) {
        if (needs_comma) { buf[offset++] = ','; }
        offset = append_component(buf, buf_size, offset,
            LOINC_TEMP, "Body temperature",
            "\"valueQuantity\":{\"value\":%.1f,\"unit\":\"Cel\","
            "\"system\":\"http://unitsofmeasure.org\",\"code\":\"Cel\"}",
            (double)obs->temp);
        needs_comma = true;
    }

    /* Systolic BP */
    if (obs->nibp_sys > 0) {
        if (needs_comma) { buf[offset++] = ','; }
        offset = append_component(buf, buf_size, offset,
            LOINC_BP_SYS, "Systolic blood pressure",
            "\"valueQuantity\":{\"value\":%d,\"unit\":\"mmHg\","
            "\"system\":\"http://unitsofmeasure.org\",\"code\":\"mm[Hg]\"}",
            obs->nibp_sys);
        needs_comma = true;
    }

    /* Diastolic BP */
    if (obs->nibp_dia > 0) {
        if (needs_comma) { buf[offset++] = ','; }
        offset = append_component(buf, buf_size, offset,
            LOINC_BP_DIA, "Diastolic blood pressure",
            "\"valueQuantity\":{\"value\":%d,\"unit\":\"mmHg\","
            "\"system\":\"http://unitsofmeasure.org\",\"code\":\"mm[Hg]\"}",
            obs->nibp_dia);
        needs_comma = true;
    }

    (void)needs_comma;

    /* Close the component array and resource */
    int tail = snprintf(buf + offset, buf_size - offset, "]}");
    if (tail < 0) return -1;
    offset += tail;

    return offset;
}

int fhir_client_build_patient_json(const char *name, const char *mrn,
                                    const char *dob, const char *gender,
                                    char *buf, int buf_size) {
    if (!name || !buf || buf_size < 128) return -1;

    /* Split name into family and given (simple: last space separates) */
    const char *space = strrchr(name, ' ');
    char given[64] = "";
    char family[64] = "";

    if (space && space != name) {
        int given_len = (int)(space - name);
        if (given_len > 63) given_len = 63;
        memcpy(given, name, given_len);
        given[given_len] = '\0';

        strncpy(family, space + 1, sizeof(family) - 1);
        family[sizeof(family) - 1] = '\0';
    } else {
        /* Single-word name — treat as family name */
        strncpy(family, name, sizeof(family) - 1);
        family[sizeof(family) - 1] = '\0';
    }

    /* Build patient JSON */
    int offset = snprintf(buf, buf_size,
        "{"
            "\"resourceType\":\"Patient\","
            "\"identifier\":[{"
                "\"type\":{"
                    "\"coding\":[{"
                        "\"system\":\"http://terminology.hl7.org/CodeSystem/v2-0203\","
                        "\"code\":\"MR\","
                        "\"display\":\"Medical Record Number\""
                    "}]"
                "},"
                "\"value\":\"%s\""
            "}],"
            "\"name\":[{"
                "\"use\":\"official\","
                "\"family\":\"%s\","
                "\"given\":[\"%s\"]"
            "}]",
        mrn ? mrn : "",
        family,
        given);

    if (offset < 0 || offset >= buf_size) return -1;

    /* Optional birthDate */
    if (dob && dob[0] != '\0') {
        int n = snprintf(buf + offset, buf_size - offset,
                         ",\"birthDate\":\"%s\"", dob);
        if (n > 0) offset += n;
    }

    /* Optional gender */
    if (gender && gender[0] != '\0') {
        int n = snprintf(buf + offset, buf_size - offset,
                         ",\"gender\":\"%s\"", gender);
        if (n > 0) offset += n;
    }

    /* Close resource */
    int tail = snprintf(buf + offset, buf_size - offset, "}");
    if (tail < 0) return -1;
    offset += tail;

    return offset;
}

/* ── Export operations (simulator stubs) ─────────────────── */

fhir_result_t fhir_client_export_observation(const fhir_vitals_observation_t *obs) {
    fhir_result_t result;
    memset(&result, 0, sizeof(result));

    if (!initialized || !obs) {
        result.success = false;
        result.http_status = 0;
        strncpy(result.error_message, "FHIR client not initialized or NULL observation",
                sizeof(result.error_message) - 1);
        return result;
    }

    /* Build JSON for logging */
    char json_buf[FHIR_JSON_MAX];
    int json_len = fhir_client_build_observation_json(obs, json_buf, sizeof(json_buf));

    if (json_len > 0) {
        printf("[fhir_client] Export Observation (%d bytes):\n%s\n",
               json_len, json_buf);
    }

    /* Simulator stub: always succeeds */
    result.success = true;
    result.http_status = 201;  /* 201 Created */
    snprintf(result.resource_id, sizeof(result.resource_id),
             "obs-%d", mock_id_counter++);
    result.error_message[0] = '\0';

    printf("[fhir_client] Observation exported: id=%s (mock)\n",
           result.resource_id);

    return result;
}

fhir_result_t fhir_client_export_patient(const char *name, const char *mrn,
                                          const char *dob, const char *gender) {
    fhir_result_t result;
    memset(&result, 0, sizeof(result));

    if (!initialized || !name) {
        result.success = false;
        result.http_status = 0;
        strncpy(result.error_message, "FHIR client not initialized or NULL name",
                sizeof(result.error_message) - 1);
        return result;
    }

    /* Build JSON for logging */
    char json_buf[FHIR_JSON_MAX];
    int json_len = fhir_client_build_patient_json(name, mrn, dob, gender,
                                                   json_buf, sizeof(json_buf));

    if (json_len > 0) {
        printf("[fhir_client] Export Patient (%d bytes):\n%s\n",
               json_len, json_buf);
    }

    /* Simulator stub: always succeeds */
    result.success = true;
    result.http_status = 201;
    snprintf(result.resource_id, sizeof(result.resource_id),
             "pat-%d", mock_id_counter++);
    result.error_message[0] = '\0';

    printf("[fhir_client] Patient exported: id=%s (mock)\n",
           result.resource_id);

    return result;
}

/* ── Import operations (simulator stubs) ─────────────────── */

bool fhir_client_import_patient(const char *patient_id,
                                 char *name_out, int name_max,
                                 char *mrn_out, int mrn_max) {
    if (!initialized || !patient_id) return false;

    printf("[fhir_client] Import Patient: id=%s (mock)\n", patient_id);

    /* Return mock patient data */
    if (name_out && name_max > 0) {
        strncpy(name_out, "Rajesh Kumar", name_max - 1);
        name_out[name_max - 1] = '\0';
    }

    if (mrn_out && mrn_max > 0) {
        strncpy(mrn_out, "MRN-2024-001234", mrn_max - 1);
        mrn_out[mrn_max - 1] = '\0';
    }

    return true;
}
