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

/* ── SEC-MEM-02: JSON string escape (RFC 8259) ──────────── */

/**
 * Escape a string for safe inclusion in JSON (RFC 8259).
 * Escapes: " \ / \b \f \n \r \t and control chars as \uXXXX
 * Returns bytes written (excluding NUL), or -1 if buffer too small.
 */
static int json_escape_string(const char *input, char *out, size_t out_size) {
    if (!input || !out || out_size == 0) return -1;

    size_t pos = 0;
    for (const char *p = input; *p; p++) {
        size_t remaining = out_size - pos;
        switch (*p) {
            case '"':  if (remaining < 3) return -1; out[pos++] = '\\'; out[pos++] = '"'; break;
            case '\\': if (remaining < 3) return -1; out[pos++] = '\\'; out[pos++] = '\\'; break;
            case '\n': if (remaining < 3) return -1; out[pos++] = '\\'; out[pos++] = 'n'; break;
            case '\r': if (remaining < 3) return -1; out[pos++] = '\\'; out[pos++] = 'r'; break;
            case '\t': if (remaining < 3) return -1; out[pos++] = '\\'; out[pos++] = 't'; break;
            case '\b': if (remaining < 3) return -1; out[pos++] = '\\'; out[pos++] = 'b'; break;
            case '\f': if (remaining < 3) return -1; out[pos++] = '\\'; out[pos++] = 'f'; break;
            default:
                if ((unsigned char)*p < 0x20) {
                    /* Control character: \uXXXX */
                    if (remaining < 7) return -1;
                    pos += snprintf(out + pos, remaining, "\\u%04x", (unsigned char)*p);
                } else {
                    if (remaining < 2) return -1;
                    out[pos++] = *p;
                }
                break;
        }
    }
    if (pos >= out_size) return -1;
    out[pos] = '\0';
    return (int)pos;
}

/* ── SEC-INPUT-03: URL validation ────────────────────────── */

/**
 * Validate a FHIR server URL.
 * Requires https:// in production; allows http://localhost and
 * http://127.0.0.1 for development/testing.
 */
static bool validate_fhir_url(const char *url) {
    if (!url) return false;
    /* Require https:// (allow http://localhost for development) */
    if (strncmp(url, "https://", 8) == 0) return true;
    if (strncmp(url, "http://localhost", 16) == 0) return true;
    if (strncmp(url, "http://127.0.0.1", 16) == 0) return true;
    return false;
}

/* ── SEC-MISC-01: Redaction helpers for logs ─────────────── */

/**
 * Redact a patient ID for logging — show only last 4 characters.
 * Output is written to a static buffer (not thread-safe; LVGL is
 * single-threaded).
 */
static const char *redact_patient_id(const char *id) {
    static char redacted[16];
    if (!id || id[0] == '\0') return "[empty]";
    size_t len = strlen(id);
    if (len <= 4) {
        snprintf(redacted, sizeof(redacted), "***%s", id);
    } else {
        snprintf(redacted, sizeof(redacted), "***%s", id + len - 4);
    }
    return redacted;
}

/**
 * Redact a patient name for logging — show first initial only.
 */
static const char *redact_patient_name(const char *name) {
    static char redacted[16];
    if (!name || name[0] == '\0') return "[REDACTED]";
    snprintf(redacted, sizeof(redacted), "%c.[REDACTED]", name[0]);
    return redacted;
}

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

    /* SEC-INPUT-03: Validate URL scheme before accepting */
    if (!validate_fhir_url(base_url)) {
        fprintf(stderr, "[fhir_client] Rejected endpoint URL: invalid scheme "
                        "(require https:// or http://localhost)\n");
        return;
    }

    strncpy(endpoint_url, base_url, sizeof(endpoint_url) - 1);
    endpoint_url[sizeof(endpoint_url) - 1] = '\0';

    printf("[fhir_client] Endpoint configured (URL validated)\n");
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

    /* SEC-MEM-02: Escape patient_id for safe JSON inclusion */
    char escaped_patient_id[FHIR_ID_MAX * 2];
    if (json_escape_string(obs->patient_id, escaped_patient_id,
                           sizeof(escaped_patient_id)) < 0) {
        fprintf(stderr, "[fhir_client] Failed to escape patient_id for JSON\n");
        return -1;
    }

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
        escaped_patient_id,
        ts_str);

    if (offset < 0 || offset >= buf_size) return -1;

    /* Track whether we need a comma separator */
    bool needs_comma = false;

    /* Heart Rate */
    if (obs->hr > 0) {
        if (needs_comma) {
            /* SEC-MEM-01: Bounds check before comma write */
            if (offset + 1 >= buf_size) {
                fprintf(stderr, "[fhir_client] Buffer overflow prevented\n");
                return -1;
            }
            buf[offset++] = ',';
        }
        offset = append_component(buf, buf_size, offset,
            LOINC_HR, "Heart rate",
            "\"valueQuantity\":{\"value\":%d,\"unit\":\"/min\","
            "\"system\":\"http://unitsofmeasure.org\",\"code\":\"/min\"}",
            obs->hr);
        needs_comma = true;
    }

    /* SpO2 */
    if (obs->spo2 > 0) {
        if (needs_comma) {
            /* SEC-MEM-01: Bounds check before comma write */
            if (offset + 1 >= buf_size) {
                fprintf(stderr, "[fhir_client] Buffer overflow prevented\n");
                return -1;
            }
            buf[offset++] = ',';
        }
        offset = append_component(buf, buf_size, offset,
            LOINC_SPO2, "Oxygen saturation in Arterial blood",
            "\"valueQuantity\":{\"value\":%d,\"unit\":\"%%\","
            "\"system\":\"http://unitsofmeasure.org\",\"code\":\"%%\"}",
            obs->spo2);
        needs_comma = true;
    }

    /* Respiratory Rate */
    if (obs->rr > 0) {
        if (needs_comma) {
            /* SEC-MEM-01: Bounds check before comma write */
            if (offset + 1 >= buf_size) {
                fprintf(stderr, "[fhir_client] Buffer overflow prevented\n");
                return -1;
            }
            buf[offset++] = ',';
        }
        offset = append_component(buf, buf_size, offset,
            LOINC_RR, "Respiratory rate",
            "\"valueQuantity\":{\"value\":%d,\"unit\":\"/min\","
            "\"system\":\"http://unitsofmeasure.org\",\"code\":\"/min\"}",
            obs->rr);
        needs_comma = true;
    }

    /* Temperature */
    if (obs->temp > 0.0f) {
        if (needs_comma) {
            /* SEC-MEM-01: Bounds check before comma write */
            if (offset + 1 >= buf_size) {
                fprintf(stderr, "[fhir_client] Buffer overflow prevented\n");
                return -1;
            }
            buf[offset++] = ',';
        }
        offset = append_component(buf, buf_size, offset,
            LOINC_TEMP, "Body temperature",
            "\"valueQuantity\":{\"value\":%.1f,\"unit\":\"Cel\","
            "\"system\":\"http://unitsofmeasure.org\",\"code\":\"Cel\"}",
            (double)obs->temp);
        needs_comma = true;
    }

    /* Systolic BP */
    if (obs->nibp_sys > 0) {
        if (needs_comma) {
            /* SEC-MEM-01: Bounds check before comma write */
            if (offset + 1 >= buf_size) {
                fprintf(stderr, "[fhir_client] Buffer overflow prevented\n");
                return -1;
            }
            buf[offset++] = ',';
        }
        offset = append_component(buf, buf_size, offset,
            LOINC_BP_SYS, "Systolic blood pressure",
            "\"valueQuantity\":{\"value\":%d,\"unit\":\"mmHg\","
            "\"system\":\"http://unitsofmeasure.org\",\"code\":\"mm[Hg]\"}",
            obs->nibp_sys);
        needs_comma = true;
    }

    /* Diastolic BP */
    if (obs->nibp_dia > 0) {
        if (needs_comma) {
            /* SEC-MEM-01: Bounds check before comma write */
            if (offset + 1 >= buf_size) {
                fprintf(stderr, "[fhir_client] Buffer overflow prevented\n");
                return -1;
            }
            buf[offset++] = ',';
        }
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

    /* SEC-MEM-02: Escape all user-supplied strings for safe JSON inclusion */
    char escaped_family[128] = "";
    char escaped_given[128] = "";
    char escaped_mrn[128] = "";
    char escaped_dob[32] = "";
    char escaped_gender[32] = "";

    if (json_escape_string(family, escaped_family, sizeof(escaped_family)) < 0) {
        fprintf(stderr, "[fhir_client] Failed to escape family name for JSON\n");
        return -1;
    }
    if (json_escape_string(given, escaped_given, sizeof(escaped_given)) < 0) {
        fprintf(stderr, "[fhir_client] Failed to escape given name for JSON\n");
        return -1;
    }
    if (mrn && json_escape_string(mrn, escaped_mrn, sizeof(escaped_mrn)) < 0) {
        fprintf(stderr, "[fhir_client] Failed to escape MRN for JSON\n");
        return -1;
    }
    if (dob && dob[0] != '\0' &&
        json_escape_string(dob, escaped_dob, sizeof(escaped_dob)) < 0) {
        fprintf(stderr, "[fhir_client] Failed to escape DOB for JSON\n");
        return -1;
    }
    if (gender && gender[0] != '\0' &&
        json_escape_string(gender, escaped_gender, sizeof(escaped_gender)) < 0) {
        fprintf(stderr, "[fhir_client] Failed to escape gender for JSON\n");
        return -1;
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
        mrn ? escaped_mrn : "",
        escaped_family,
        escaped_given);

    if (offset < 0 || offset >= buf_size) return -1;

    /* Optional birthDate */
    if (dob && dob[0] != '\0') {
        int n = snprintf(buf + offset, buf_size - offset,
                         ",\"birthDate\":\"%s\"", escaped_dob);
        if (n > 0) offset += n;
    }

    /* Optional gender */
    if (gender && gender[0] != '\0') {
        int n = snprintf(buf + offset, buf_size - offset,
                         ",\"gender\":\"%s\"", escaped_gender);
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

    /* Build JSON (needed for actual transport; not logged to avoid leaking vitals) */
    char json_buf[FHIR_JSON_MAX];
    int json_len = fhir_client_build_observation_json(obs, json_buf, sizeof(json_buf));

    /* SEC-MISC-01: Do NOT log full JSON — it contains patient vitals and IDs */
    if (json_len > 0) {
        printf("[fhir_client] Export Observation (%d bytes) for patient=%s\n",
               json_len, redact_patient_id(obs->patient_id));
    } else {
        fprintf(stderr, "[fhir_client] Failed to build Observation JSON\n");
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

    /* Build JSON (needed for actual transport; not logged to avoid leaking PII) */
    char json_buf[FHIR_JSON_MAX];
    int json_len = fhir_client_build_patient_json(name, mrn, dob, gender,
                                                   json_buf, sizeof(json_buf));

    /* SEC-MISC-01: Log only redacted patient info, never the full JSON */
    if (json_len > 0) {
        printf("[fhir_client] Export Patient (%d bytes): name=%s, mrn=%s\n",
               json_len, redact_patient_name(name),
               mrn ? redact_patient_id(mrn) : "[none]");
    } else {
        fprintf(stderr, "[fhir_client] Failed to build Patient JSON\n");
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

    /* SEC-MISC-01: Redact patient ID in log */
    printf("[fhir_client] Import Patient: id=%s (mock)\n",
           redact_patient_id(patient_id));

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
