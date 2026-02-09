/**
 * @file test_patient_data.c
 * @brief Unit tests for patient_data module
 *
 * Tests CRUD operations, MRN lookup, monitor slot association,
 * admit/discharge workflow, and list queries using in-memory SQLite.
 */

#include "test_framework.h"
#include "patient_data.h"

/* ── Test: init and close with in-memory DB ──────────────── */

static void test_init_close(void) {
    printf("  test_init_close\n");

    bool ok = patient_data_init(NULL);  /* NULL => ":memory:" */
    ASSERT_TRUE(ok);

    patient_data_close();
}

/* ── Test: create_default fills zeroed/empty patient ─────── */

static void test_create_default(void) {
    printf("  test_create_default\n");

    patient_t p;
    patient_data_create_default(&p);

    ASSERT_EQ_INT(p.id, 0);
    ASSERT_EQ_INT(p.gender, PATIENT_GENDER_UNKNOWN);
    ASSERT_FLOAT_NEAR(p.weight_kg, 0.0f, 0.001);
    ASSERT_FLOAT_NEAR(p.height_cm, 0.0f, 0.001);
    ASSERT_FALSE(p.active);
    ASSERT_EQ_INT(p.monitor_slot, 0);
    ASSERT_EQ_INT(p.admitted_ts, 0);
    ASSERT_EQ_INT(p.discharged_ts, 0);
}

/* ── Test: save (insert) and get by ID ───────────────────── */

static void test_save_and_get(void) {
    printf("  test_save_and_get\n");

    bool ok = patient_data_init(NULL);
    ASSERT_TRUE(ok);

    /* The init seeds a default patient, so IDs start at 2 for our new one */
    patient_t p;
    patient_data_create_default(&p);
    strncpy(p.name, "Test Patient", PATIENT_NAME_MAX - 1);
    strncpy(p.mrn, "MRN-TEST-001", PATIENT_MRN_MAX - 1);
    p.gender = PATIENT_GENDER_FEMALE;
    p.weight_kg = 65.5f;
    p.height_cm = 162.0f;
    strncpy(p.blood_type, "A+", 7);
    strncpy(p.ward, "ICU", PATIENT_FIELD_MAX - 1);
    strncpy(p.bed, "Bed 1", PATIENT_FIELD_MAX - 1);
    strncpy(p.attending, "Dr. Test", PATIENT_NAME_MAX - 1);

    ok = patient_data_save(&p);
    ASSERT_TRUE(ok);
    ASSERT_GT_INT(p.id, 0);  /* ID should be assigned */

    /* Retrieve by ID */
    patient_t loaded;
    ok = patient_data_get(p.id, &loaded);
    ASSERT_TRUE(ok);
    ASSERT_STR_EQ(loaded.name, "Test Patient");
    ASSERT_STR_EQ(loaded.mrn, "MRN-TEST-001");
    ASSERT_EQ_INT(loaded.gender, PATIENT_GENDER_FEMALE);
    ASSERT_FLOAT_NEAR(loaded.weight_kg, 65.5f, 0.1);
    ASSERT_FLOAT_NEAR(loaded.height_cm, 162.0f, 0.1);
    ASSERT_STR_EQ(loaded.blood_type, "A+");
    ASSERT_STR_EQ(loaded.ward, "ICU");
    ASSERT_STR_EQ(loaded.bed, "Bed 1");
    ASSERT_STR_EQ(loaded.attending, "Dr. Test");

    patient_data_close();
}

/* ── Test: save (update) existing patient ────────────────── */

static void test_update(void) {
    printf("  test_update\n");

    patient_data_init(NULL);

    patient_t p;
    patient_data_create_default(&p);
    strncpy(p.name, "Original Name", PATIENT_NAME_MAX - 1);
    strncpy(p.mrn, "MRN-UPD-001", PATIENT_MRN_MAX - 1);
    patient_data_save(&p);
    int32_t id = p.id;

    /* Update the name */
    strncpy(p.name, "Updated Name", PATIENT_NAME_MAX - 1);
    bool ok = patient_data_save(&p);
    ASSERT_TRUE(ok);
    ASSERT_EQ_INT(p.id, id);  /* ID should not change */

    /* Verify update persisted */
    patient_t loaded;
    patient_data_get(id, &loaded);
    ASSERT_STR_EQ(loaded.name, "Updated Name");

    patient_data_close();
}

/* ── Test: find by MRN ───────────────────────────────────── */

static void test_find_by_mrn(void) {
    printf("  test_find_by_mrn\n");

    patient_data_init(NULL);

    patient_t p;
    patient_data_create_default(&p);
    strncpy(p.name, "MRN Lookup Test", PATIENT_NAME_MAX - 1);
    strncpy(p.mrn, "MRN-FIND-999", PATIENT_MRN_MAX - 1);
    patient_data_save(&p);

    /* Find by MRN */
    patient_t found;
    bool ok = patient_data_find_by_mrn("MRN-FIND-999", &found);
    ASSERT_TRUE(ok);
    ASSERT_STR_EQ(found.name, "MRN Lookup Test");
    ASSERT_EQ_INT(found.id, p.id);

    /* Non-existent MRN returns false */
    ok = patient_data_find_by_mrn("MRN-NONEXISTENT", &found);
    ASSERT_FALSE(ok);

    patient_data_close();
}

/* ── Test: delete patient ────────────────────────────────── */

static void test_delete(void) {
    printf("  test_delete\n");

    patient_data_init(NULL);

    patient_t p;
    patient_data_create_default(&p);
    strncpy(p.name, "Delete Me", PATIENT_NAME_MAX - 1);
    strncpy(p.mrn, "MRN-DEL-001", PATIENT_MRN_MAX - 1);
    patient_data_save(&p);
    int32_t id = p.id;

    /* Delete */
    bool ok = patient_data_delete(id);
    ASSERT_TRUE(ok);

    /* Get should fail */
    patient_t loaded;
    ok = patient_data_get(id, &loaded);
    ASSERT_FALSE(ok);

    /* Deleting again should fail (not found) */
    ok = patient_data_delete(id);
    ASSERT_FALSE(ok);

    /* Deleting invalid ID should fail */
    ok = patient_data_delete(0);
    ASSERT_FALSE(ok);
    ok = patient_data_delete(-1);
    ASSERT_FALSE(ok);

    patient_data_close();
}

/* ── Test: monitor slot association ──────────────────────── */

static void test_associate_disassociate(void) {
    printf("  test_associate_disassociate\n");

    patient_data_init(NULL);

    /* Create a new patient (the default seed goes to slot 0) */
    patient_t p;
    patient_data_create_default(&p);
    strncpy(p.name, "Slot Test", PATIENT_NAME_MAX - 1);
    strncpy(p.mrn, "MRN-SLOT-001", PATIENT_MRN_MAX - 1);
    patient_data_save(&p);

    /* Associate to slot 1 */
    bool ok = patient_data_associate(p.id, 1);
    ASSERT_TRUE(ok);

    /* Get active for slot 1 */
    const patient_t *active = patient_data_get_active(1);
    ASSERT_NOT_NULL(active);
    ASSERT_EQ_INT(active->id, p.id);
    ASSERT_STR_EQ(active->name, "Slot Test");

    /* Disassociate slot 1 */
    ok = patient_data_disassociate(1);
    ASSERT_TRUE(ok);

    active = patient_data_get_active(1);
    ASSERT_NULL(active);

    /* Invalid slot should fail */
    ok = patient_data_associate(p.id, 5);
    ASSERT_FALSE(ok);
    ok = patient_data_disassociate(5);
    ASSERT_FALSE(ok);

    patient_data_close();
}

/* ── Test: admit and discharge workflow ──────────────────── */

static void test_admit_discharge(void) {
    printf("  test_admit_discharge\n");

    patient_data_init(NULL);

    patient_t p;
    patient_data_create_default(&p);
    strncpy(p.name, "Admit Test", PATIENT_NAME_MAX - 1);
    strncpy(p.mrn, "MRN-ADM-001", PATIENT_MRN_MAX - 1);

    /* Admit */
    bool ok = patient_data_admit(&p);
    ASSERT_TRUE(ok);
    ASSERT_GT_INT(p.id, 0);
    ASSERT_TRUE(p.active);
    ASSERT_GT_INT(p.admitted_ts, 0);
    ASSERT_EQ_INT(p.discharged_ts, 0);

    /* Verify in DB */
    patient_t loaded;
    patient_data_get(p.id, &loaded);
    ASSERT_TRUE(loaded.active);
    ASSERT_GT_INT(loaded.admitted_ts, 0);

    /* Discharge */
    ok = patient_data_discharge(p.id);
    ASSERT_TRUE(ok);

    patient_data_get(p.id, &loaded);
    ASSERT_FALSE(loaded.active);
    ASSERT_GT_INT(loaded.discharged_ts, 0);

    /* Discharge non-existent patient should fail */
    ok = patient_data_discharge(99999);
    ASSERT_FALSE(ok);
    ok = patient_data_discharge(0);
    ASSERT_FALSE(ok);

    patient_data_close();
}

/* ── Test: list active patients ──────────────────────────── */

static void test_list_active(void) {
    printf("  test_list_active\n");

    patient_data_init(NULL);

    /* The default seed patient is active, so we start with 1 */
    patient_list_t list;
    int count = patient_data_list_active(&list);
    ASSERT_GT_INT(count, 0);

    /* Add another active patient */
    patient_t p;
    patient_data_create_default(&p);
    strncpy(p.name, "Active Patient", PATIENT_NAME_MAX - 1);
    strncpy(p.mrn, "MRN-ACT-001", PATIENT_MRN_MAX - 1);
    patient_data_admit(&p);

    int count2 = patient_data_list_active(&list);
    ASSERT_EQ_INT(count2, count + 1);

    patient_data_close();
}

/* ── Test: list all patients ─────────────────────────────── */

static void test_list_all(void) {
    printf("  test_list_all\n");

    patient_data_init(NULL);

    patient_list_t list;
    int initial_count = patient_data_list_all(&list);

    /* Add a couple more patients */
    patient_t p1;
    patient_data_create_default(&p1);
    strncpy(p1.name, "All Test 1", PATIENT_NAME_MAX - 1);
    strncpy(p1.mrn, "MRN-ALL-001", PATIENT_MRN_MAX - 1);
    patient_data_save(&p1);

    patient_t p2;
    patient_data_create_default(&p2);
    strncpy(p2.name, "All Test 2", PATIENT_NAME_MAX - 1);
    strncpy(p2.mrn, "MRN-ALL-002", PATIENT_MRN_MAX - 1);
    patient_data_save(&p2);

    int new_count = patient_data_list_all(&list);
    ASSERT_EQ_INT(new_count, initial_count + 2);

    patient_data_close();
}

/* ── Test: get with invalid arguments ────────────────────── */

static void test_get_invalid_args(void) {
    printf("  test_get_invalid_args\n");

    patient_data_init(NULL);

    patient_t p;
    bool ok = patient_data_get(0, &p);
    ASSERT_FALSE(ok);
    ok = patient_data_get(-1, &p);
    ASSERT_FALSE(ok);
    ok = patient_data_get(99999, &p);
    ASSERT_FALSE(ok);

    patient_data_close();
}

/* ── Test: get_active for empty slot ─────────────────────── */

static void test_get_active_empty_slot(void) {
    printf("  test_get_active_empty_slot\n");

    patient_data_init(NULL);

    /* Slot 1 may not have anyone (seed goes to slot 0) */
    /* Disassociate slot 1 first to be sure */
    patient_data_disassociate(1);
    const patient_t *active = patient_data_get_active(1);
    ASSERT_NULL(active);

    /* Invalid slot */
    active = patient_data_get_active(5);
    ASSERT_NULL(active);

    patient_data_close();
}

/* ── Public entry point ──────────────────────────────────── */

void test_patient_data(void) {
    test_init_close();
    test_create_default();
    test_save_and_get();
    test_update();
    test_find_by_mrn();
    test_delete();
    test_associate_disassociate();
    test_admit_discharge();
    test_list_active();
    test_list_all();
    test_get_invalid_args();
    test_get_active_empty_slot();
}
