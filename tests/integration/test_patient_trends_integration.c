/**
 * @file test_patient_trends_integration.c
 * @brief Integration tests: patient_data + trend_db
 *
 * Verifies that patient admission/discharge workflows interact
 * correctly with trend data storage. Tests that vitals data is
 * associated with patients and that trends persist after discharge.
 *
 * Uses in-memory SQLite databases for fast, isolated tests.
 */

#include "test_framework.h"
#include "patient_data.h"
#include "trend_db.h"
#include <time.h>
#include <string.h>

/* ── Helper: create a default patient ──────────────────────── */

static patient_t make_test_patient(const char *name, const char *mrn) {
    patient_t p;
    patient_data_create_default(&p);
    strncpy(p.name, name, PATIENT_NAME_MAX - 1);
    strncpy(p.mrn, mrn, PATIENT_MRN_MAX - 1);
    strncpy(p.ward, "ICU", PATIENT_FIELD_MAX - 1);
    strncpy(p.bed, "B-01", PATIENT_FIELD_MAX - 1);
    p.gender = PATIENT_GENDER_MALE;
    return p;
}

/* ── Test: init both modules together ──────────────────────── */

static void test_init_both(void) {
    printf("  test_init_both\n");

    bool pd_ok = patient_data_init(":memory:");
    ASSERT_TRUE(pd_ok);

    bool td_ok = trend_db_init(":memory:");
    ASSERT_TRUE(td_ok);

    trend_db_close();
    patient_data_close();
}

/* ── Test: admit patient and record vitals ─────────────────── */

static void test_admit_and_record_vitals(void) {
    printf("  test_admit_and_record_vitals\n");

    patient_data_init(":memory:");
    trend_db_init(":memory:");

    /* Create and admit a patient */
    patient_t p = make_test_patient("Rajesh Kumar", "MRN-001");
    bool ok = patient_data_admit(&p);
    ASSERT_TRUE(ok);
    ASSERT_GT_INT(p.id, 0);
    ASSERT_TRUE(p.active);
    ASSERT_GT_INT((int)p.admitted_ts, 0);

    /* Associate with monitor slot 0 */
    ok = patient_data_associate(p.id, 0);
    ASSERT_TRUE(ok);

    /* Verify patient is on slot 0 */
    const patient_t *active = patient_data_get_active(0);
    ASSERT_NOT_NULL(active);
    ASSERT_STR_EQ(active->name, "Rajesh Kumar");
    ASSERT_STR_EQ(active->mrn, "MRN-001");

    /* Record vitals data into trend_db */
    uint32_t now = (uint32_t)time(NULL);
    for (int i = 0; i < 10; i++) {
        trend_db_insert_sample(now + i, 72 + i, 97, 16, 37.0f);
    }

    /* Query trends */
    trend_query_result_t result;
    int count = trend_db_query_param(TREND_PARAM_HR, now - 10,
                                     now + 100, 100, &result);
    ASSERT_EQ_INT(count, 10);
    ASSERT_EQ_INT(result.value[0], 72);
    ASSERT_EQ_INT(result.value[9], 81);

    trend_db_close();
    patient_data_close();
}

/* ── Test: discharge preserves trends ──────────────────────── */

static void test_discharge_preserves_trends(void) {
    printf("  test_discharge_preserves_trends\n");

    patient_data_init(":memory:");
    trend_db_init(":memory:");

    /* Admit patient */
    patient_t p = make_test_patient("Priya Sharma", "MRN-002");
    patient_data_admit(&p);
    patient_data_associate(p.id, 0);

    /* Record vitals */
    uint32_t now = (uint32_t)time(NULL);
    for (int i = 0; i < 5; i++) {
        trend_db_insert_sample(now + i, 80, 95 + i, 18, 37.2f);
    }

    /* Discharge the patient */
    bool ok = patient_data_discharge(p.id);
    ASSERT_TRUE(ok);

    /* Verify patient is discharged */
    patient_t discharged;
    ok = patient_data_get(p.id, &discharged);
    ASSERT_TRUE(ok);
    ASSERT_FALSE(discharged.active);
    ASSERT_GT_INT((int)discharged.discharged_ts, 0);

    /* Trend data should still be in the DB */
    trend_query_result_t result;
    int count = trend_db_query_param(TREND_PARAM_HR, now - 10,
                                     now + 100, 100, &result);
    ASSERT_EQ_INT(count, 5);

    /* SpO2 data should also persist */
    count = trend_db_query_param(TREND_PARAM_SPO2, now - 10,
                                 now + 100, 100, &result);
    ASSERT_EQ_INT(count, 5);
    ASSERT_EQ_INT(result.value[0], 95);
    ASSERT_EQ_INT(result.value[4], 99);

    trend_db_close();
    patient_data_close();
}

/* ── Test: query trends for specific timeframe ─────────────── */

static void test_query_trends_timeframe(void) {
    printf("  test_query_trends_timeframe\n");

    patient_data_init(":memory:");
    trend_db_init(":memory:");

    uint32_t base = (uint32_t)time(NULL);

    /* Admit patient and record vitals over a span */
    patient_t p = make_test_patient("Amit Patel", "MRN-003");
    patient_data_admit(&p);

    /* Phase 1: normal vitals (base to base+10) */
    for (int i = 0; i < 10; i++) {
        trend_db_insert_sample(base + i, 70, 98, 14, 36.8f);
    }

    /* Phase 2: elevated vitals (base+100 to base+110) */
    for (int i = 0; i < 10; i++) {
        trend_db_insert_sample(base + 100 + i, 110, 92, 22, 38.5f);
    }

    /* Query only phase 1 */
    trend_query_result_t result;
    int count = trend_db_query_param(TREND_PARAM_HR, base - 1,
                                     base + 11, 100, &result);
    ASSERT_EQ_INT(count, 10);
    for (int i = 0; i < count; i++) {
        ASSERT_EQ_INT(result.value[i], 70);
    }

    /* Query only phase 2 */
    count = trend_db_query_param(TREND_PARAM_HR, base + 99,
                                 base + 111, 100, &result);
    ASSERT_EQ_INT(count, 10);
    for (int i = 0; i < count; i++) {
        ASSERT_EQ_INT(result.value[i], 110);
    }

    /* Query temperature for phase 2 */
    count = trend_db_query_param(TREND_PARAM_TEMP, base + 99,
                                 base + 111, 100, &result);
    ASSERT_EQ_INT(count, 10);
    /* Temp stored as x10: 38.5 -> 385 */
    for (int i = 0; i < count; i++) {
        ASSERT_EQ_INT(result.value[i], 385);
    }

    trend_db_close();
    patient_data_close();
}

/* ── Test: multiple patients with separate vitals windows ─── */

static void test_multiple_patients(void) {
    printf("  test_multiple_patients\n");

    patient_data_init(":memory:");
    trend_db_init(":memory:");

    /* Admit two patients */
    patient_t p1 = make_test_patient("Patient One", "MRN-P01");
    patient_t p2 = make_test_patient("Patient Two", "MRN-P02");

    patient_data_admit(&p1);
    patient_data_admit(&p2);

    /* Associate to different slots */
    patient_data_associate(p1.id, 0);
    patient_data_associate(p2.id, 1);

    /* Verify both are active */
    const patient_t *slot0 = patient_data_get_active(0);
    const patient_t *slot1 = patient_data_get_active(1);
    ASSERT_NOT_NULL(slot0);
    ASSERT_NOT_NULL(slot1);
    ASSERT_STR_EQ(slot0->name, "Patient One");
    ASSERT_STR_EQ(slot1->name, "Patient Two");

    /* Record vitals (note: trend_db is global, not per-patient) */
    uint32_t now = (uint32_t)time(NULL);
    trend_db_insert_sample(now, 75, 98, 16, 37.0f);
    trend_db_insert_sample(now + 1, 90, 94, 20, 37.5f);

    /* Both samples are in the DB */
    trend_query_result_t result;
    int count = trend_db_query_param(TREND_PARAM_HR, now - 10,
                                     now + 10, 100, &result);
    ASSERT_EQ_INT(count, 2);

    /* List active patients (includes auto-seeded default patient) */
    patient_list_t list;
    int active_count = patient_data_list_active(&list);
    ASSERT_GE_INT(active_count, 2);

    trend_db_close();
    patient_data_close();
}

/* ── Test: NIBP measurements alongside patient data ────────── */

static void test_nibp_with_patient(void) {
    printf("  test_nibp_with_patient\n");

    patient_data_init(":memory:");
    trend_db_init(":memory:");

    /* Admit patient */
    patient_t p = make_test_patient("NIBP Patient", "MRN-NIBP");
    patient_data_admit(&p);

    uint32_t now = (uint32_t)time(NULL);

    /* Record NIBP measurements */
    trend_db_insert_nibp(now,     120, 80, 93);
    trend_db_insert_nibp(now + 60, 135, 88, 104);
    trend_db_insert_nibp(now + 120, 140, 90, 107);

    /* Query NIBP data */
    trend_nibp_result_t nibp_result;
    int count = trend_db_query_nibp(now - 10, now + 200, &nibp_result);
    ASSERT_EQ_INT(count, 3);
    ASSERT_EQ_INT(nibp_result.sys[0], 120);
    ASSERT_EQ_INT(nibp_result.dia[0], 80);
    ASSERT_EQ_INT(nibp_result.sys[2], 140);
    ASSERT_EQ_INT(nibp_result.dia[2], 90);

    trend_db_close();
    patient_data_close();
}

/* ── Test: patient data CRUD alongside trends ──────────────── */

static void test_patient_crud_with_trends(void) {
    printf("  test_patient_crud_with_trends\n");

    patient_data_init(":memory:");
    trend_db_init(":memory:");

    /* Create patient */
    patient_t p = make_test_patient("CRUD Patient", "MRN-CRUD");
    bool ok = patient_data_save(&p);
    ASSERT_TRUE(ok);
    ASSERT_GT_INT(p.id, 0);

    /* Admit */
    ok = patient_data_admit(&p);
    ASSERT_TRUE(ok);

    /* Record some vitals */
    uint32_t now = (uint32_t)time(NULL);
    trend_db_insert_sample(now, 72, 97, 15, 36.9f);

    /* Find by MRN */
    patient_t found;
    ok = patient_data_find_by_mrn("MRN-CRUD", &found);
    ASSERT_TRUE(ok);
    ASSERT_STR_EQ(found.name, "CRUD Patient");
    ASSERT_TRUE(found.active);

    /* Modify patient data */
    strncpy(found.diagnosis, "Hypertension", PATIENT_NOTES_MAX - 1);
    ok = patient_data_save(&found);
    ASSERT_TRUE(ok);

    /* Verify modification */
    patient_t updated;
    ok = patient_data_get(found.id, &updated);
    ASSERT_TRUE(ok);
    ASSERT_STR_EQ(updated.diagnosis, "Hypertension");

    /* Trends remain untouched after patient update */
    trend_query_result_t result;
    int count = trend_db_query_param(TREND_PARAM_HR, now - 10,
                                     now + 10, 100, &result);
    ASSERT_EQ_INT(count, 1);
    ASSERT_EQ_INT(result.value[0], 72);

    trend_db_close();
    patient_data_close();
}

/* ── Public entry point ──────────────────────────────────── */

void test_patient_trends_integration(void) {
    test_init_both();
    test_admit_and_record_vitals();
    test_discharge_preserves_trends();
    test_query_trends_timeframe();
    test_multiple_patients();
    test_nibp_with_patient();
    test_patient_crud_with_trends();
}
