/**
 * @file patient_data.c
 * @brief SQLite-backed patient data model implementation
 *
 * All database access is single-threaded (LVGL main loop).
 * Uses prepared statements for performance and static buffers
 * to avoid heap allocation. Maintains an in-memory cache of
 * the two active monitor slots for O(1) access.
 */

#include "patient_data.h"
#include "sqlite3.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/* ── Stringification helpers (for LIMIT clause) ──────────────── */

#define _XSTR(x) #x
#define XSTR(x)  _XSTR(x)

/* ── Module state ────────────────────────────────────────────── */

static sqlite3 *db = NULL;

/** Cached patients for each monitor slot (quick access from UI). */
static patient_t  active_patients[2];
static bool       active_valid[2] = { false, false };

/* ── Prepared statements ─────────────────────────────────────── */

static sqlite3_stmt *stmt_insert       = NULL;
static sqlite3_stmt *stmt_update       = NULL;
static sqlite3_stmt *stmt_delete       = NULL;
static sqlite3_stmt *stmt_get_by_id    = NULL;
static sqlite3_stmt *stmt_find_by_mrn  = NULL;
static sqlite3_stmt *stmt_list_active  = NULL;
static sqlite3_stmt *stmt_list_all     = NULL;
static sqlite3_stmt *stmt_set_slot     = NULL;
static sqlite3_stmt *stmt_clear_slot   = NULL;
static sqlite3_stmt *stmt_count        = NULL;

/* ── Schema ──────────────────────────────────────────────────── */

static const char *SCHEMA_SQL =
    "CREATE TABLE IF NOT EXISTS patients ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  name TEXT NOT NULL,"
    "  mrn TEXT UNIQUE,"
    "  dob TEXT,"
    "  gender INTEGER DEFAULT 0,"
    "  blood_type TEXT,"
    "  ward TEXT,"
    "  bed TEXT,"
    "  attending TEXT,"
    "  weight_kg REAL DEFAULT 0,"
    "  height_cm REAL DEFAULT 0,"
    "  allergies TEXT,"
    "  diagnosis TEXT,"
    "  notes TEXT,"
    "  admitted_ts INTEGER DEFAULT 0,"
    "  discharged_ts INTEGER DEFAULT 0,"
    "  active INTEGER DEFAULT 0,"
    "  monitor_slot INTEGER DEFAULT 0"
    ");";

/* ── Helper: prepare a single statement ──────────────────────── */

static bool prepare(sqlite3_stmt **out, const char *sql) {
    int rc = sqlite3_prepare_v2(db, sql, -1, out, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[patient_data] prepare failed: %s\n  SQL: %s\n",
                sqlite3_errmsg(db), sql);
        return false;
    }
    return true;
}

/* ── Helper: finalize a single statement ─────────────────────── */

static void finalize_stmt(sqlite3_stmt **s) {
    if (*s) {
        sqlite3_finalize(*s);
        *s = NULL;
    }
}

/* ── Helper: safely copy text column to fixed buffer ─────────── */

static void copy_text_col(char *dst, size_t dst_size, sqlite3_stmt *stmt, int col) {
    const char *txt = (const char *)sqlite3_column_text(stmt, col);
    if (txt) {
        strncpy(dst, txt, dst_size - 1);
        dst[dst_size - 1] = '\0';
    } else {
        dst[0] = '\0';
    }
}

/* ── Helper: read a patient_t from the current row ───────────── */

static void read_patient_row(sqlite3_stmt *stmt, patient_t *p) {
    memset(p, 0, sizeof(*p));

    p->id = sqlite3_column_int(stmt, 0);
    copy_text_col(p->name,       sizeof(p->name),       stmt, 1);
    copy_text_col(p->mrn,        sizeof(p->mrn),        stmt, 2);
    copy_text_col(p->dob,        sizeof(p->dob),        stmt, 3);
    p->gender = (patient_gender_t)sqlite3_column_int(stmt, 4);
    copy_text_col(p->blood_type, sizeof(p->blood_type), stmt, 5);
    copy_text_col(p->ward,       sizeof(p->ward),       stmt, 6);
    copy_text_col(p->bed,        sizeof(p->bed),        stmt, 7);
    copy_text_col(p->attending,  sizeof(p->attending),  stmt, 8);
    p->weight_kg     = (float)sqlite3_column_double(stmt, 9);
    p->height_cm     = (float)sqlite3_column_double(stmt, 10);
    copy_text_col(p->allergies,  sizeof(p->allergies),  stmt, 11);
    copy_text_col(p->diagnosis,  sizeof(p->diagnosis),  stmt, 12);
    copy_text_col(p->notes,      sizeof(p->notes),      stmt, 13);
    p->admitted_ts   = (uint32_t)sqlite3_column_int(stmt, 14);
    p->discharged_ts = (uint32_t)sqlite3_column_int(stmt, 15);
    p->active        = sqlite3_column_int(stmt, 16) != 0;
    p->monitor_slot  = (uint8_t)sqlite3_column_int(stmt, 17);
}

/* ── Helper: bind patient fields to an INSERT/UPDATE statement ── */

static void bind_patient_fields(sqlite3_stmt *stmt, const patient_t *p, int offset) {
    sqlite3_bind_text(stmt, offset + 0,  p->name,       -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, offset + 1,  p->mrn,        -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, offset + 2,  p->dob,        -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, offset + 3,  (int)p->gender);
    sqlite3_bind_text(stmt, offset + 4,  p->blood_type, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, offset + 5,  p->ward,       -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, offset + 6,  p->bed,        -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, offset + 7,  p->attending,  -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, offset + 8,  (double)p->weight_kg);
    sqlite3_bind_double(stmt, offset + 9,  (double)p->height_cm);
    sqlite3_bind_text(stmt, offset + 10, p->allergies,  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, offset + 11, p->diagnosis,  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, offset + 12, p->notes,      -1, SQLITE_TRANSIENT);
    sqlite3_bind_int (stmt, offset + 13, (int)p->admitted_ts);
    sqlite3_bind_int (stmt, offset + 14, (int)p->discharged_ts);
    sqlite3_bind_int (stmt, offset + 15, p->active ? 1 : 0);
    sqlite3_bind_int (stmt, offset + 16, (int)p->monitor_slot);
}

/* ── Helper: reload active patient cache from DB ─────────────── */

static void reload_active_cache(void) {
    active_valid[0] = false;
    active_valid[1] = false;

    if (!db || !stmt_list_active) return;

    sqlite3_reset(stmt_list_active);

    while (sqlite3_step(stmt_list_active) == SQLITE_ROW) {
        patient_t tmp;
        read_patient_row(stmt_list_active, &tmp);
        if (tmp.monitor_slot < 2) {
            active_patients[tmp.monitor_slot] = tmp;
            active_valid[tmp.monitor_slot] = true;
        }
    }
}

/* ── Helper: insert default mock patient if table is empty ───── */

static void seed_default_patient(void) {
    if (!db || !stmt_count) return;

    sqlite3_reset(stmt_count);
    if (sqlite3_step(stmt_count) == SQLITE_ROW) {
        int count = sqlite3_column_int(stmt_count, 0);
        if (count > 0) return;  /* Table already has data */
    }

    printf("[patient_data] Empty database — inserting default patient\n");

    patient_t def;
    patient_data_create_default(&def);

    strncpy(def.name,       "Rajesh Kumar",                       PATIENT_NAME_MAX - 1);
    strncpy(def.mrn,        "MRN-2024-001234",                    PATIENT_MRN_MAX - 1);
    strncpy(def.dob,        "1958-03-15",                         PATIENT_FIELD_MAX - 1);
    def.gender = PATIENT_GENDER_MALE;
    strncpy(def.blood_type, "O+",                                 7);
    strncpy(def.ward,       "General Ward 3",                     PATIENT_FIELD_MAX - 1);
    strncpy(def.bed,        "Bed 4",                              PATIENT_FIELD_MAX - 1);
    strncpy(def.attending,  "Dr. Priya Patel",                    PATIENT_NAME_MAX - 1);
    def.weight_kg = 72.0f;
    def.height_cm = 170.0f;
    strncpy(def.allergies,  "Penicillin, Sulfonamides",           PATIENT_NOTES_MAX - 1);
    strncpy(def.diagnosis,  "Post-operative cardiac monitoring",  PATIENT_NOTES_MAX - 1);
    strncpy(def.notes,      "Stable condition, continue monitoring", PATIENT_NOTES_MAX - 1);
    def.admitted_ts  = (uint32_t)time(NULL);
    def.active       = true;
    def.monitor_slot = 0;

    if (patient_data_save(&def)) {
        printf("[patient_data] Default patient saved (id=%d)\n", def.id);
    } else {
        fprintf(stderr, "[patient_data] Failed to save default patient\n");
    }
}

/* ── Lifecycle ───────────────────────────────────────────────── */

bool patient_data_init(const char *db_path) {
    const char *path = db_path ? db_path : ":memory:";
    int rc = sqlite3_open(path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[patient_data] Failed to open DB '%s': %s\n",
                path, sqlite3_errmsg(db));
        db = NULL;
        return false;
    }

    /* Performance pragmas */
    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);
    sqlite3_exec(db, "PRAGMA synchronous=NORMAL;", NULL, NULL, NULL);

    /* Create table */
    char *err_msg = NULL;
    rc = sqlite3_exec(db, SCHEMA_SQL, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[patient_data] Schema creation failed: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        db = NULL;
        return false;
    }

    /* Prepare all statements */
    bool ok = true;

    ok = ok && prepare(&stmt_insert,
        "INSERT INTO patients "
        "(name, mrn, dob, gender, blood_type, ward, bed, attending, "
        "weight_kg, height_cm, allergies, diagnosis, notes, "
        "admitted_ts, discharged_ts, active, monitor_slot) "
        "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, "
        "?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17)");

    ok = ok && prepare(&stmt_update,
        "UPDATE patients SET "
        "name=?1, mrn=?2, dob=?3, gender=?4, blood_type=?5, "
        "ward=?6, bed=?7, attending=?8, weight_kg=?9, height_cm=?10, "
        "allergies=?11, diagnosis=?12, notes=?13, "
        "admitted_ts=?14, discharged_ts=?15, active=?16, monitor_slot=?17 "
        "WHERE id=?18");

    ok = ok && prepare(&stmt_delete,
        "DELETE FROM patients WHERE id=?1");

    ok = ok && prepare(&stmt_get_by_id,
        "SELECT id, name, mrn, dob, gender, blood_type, ward, bed, "
        "attending, weight_kg, height_cm, allergies, diagnosis, notes, "
        "admitted_ts, discharged_ts, active, monitor_slot "
        "FROM patients WHERE id=?1");

    ok = ok && prepare(&stmt_find_by_mrn,
        "SELECT id, name, mrn, dob, gender, blood_type, ward, bed, "
        "attending, weight_kg, height_cm, allergies, diagnosis, notes, "
        "admitted_ts, discharged_ts, active, monitor_slot "
        "FROM patients WHERE mrn=?1");

    ok = ok && prepare(&stmt_list_active,
        "SELECT id, name, mrn, dob, gender, blood_type, ward, bed, "
        "attending, weight_kg, height_cm, allergies, diagnosis, notes, "
        "admitted_ts, discharged_ts, active, monitor_slot "
        "FROM patients WHERE active=1 ORDER BY admitted_ts DESC");

    ok = ok && prepare(&stmt_list_all,
        "SELECT id, name, mrn, dob, gender, blood_type, ward, bed, "
        "attending, weight_kg, height_cm, allergies, diagnosis, notes, "
        "admitted_ts, discharged_ts, active, monitor_slot "
        "FROM patients ORDER BY active DESC, admitted_ts DESC "
        "LIMIT " XSTR(PATIENT_MAX_STORED));

    ok = ok && prepare(&stmt_set_slot,
        "UPDATE patients SET monitor_slot=?1, active=1 WHERE id=?2");

    ok = ok && prepare(&stmt_clear_slot,
        "UPDATE patients SET monitor_slot=0 WHERE monitor_slot=?1");

    ok = ok && prepare(&stmt_count,
        "SELECT COUNT(*) FROM patients");

    if (!ok) {
        fprintf(stderr, "[patient_data] Statement preparation failed\n");
        patient_data_close();
        return false;
    }

    /* Seed default patient if table is empty */
    seed_default_patient();

    /* Load active patients into cache */
    reload_active_cache();

    printf("[patient_data] Initialized: %s\n", path);
    return true;
}

void patient_data_close(void) {
    finalize_stmt(&stmt_insert);
    finalize_stmt(&stmt_update);
    finalize_stmt(&stmt_delete);
    finalize_stmt(&stmt_get_by_id);
    finalize_stmt(&stmt_find_by_mrn);
    finalize_stmt(&stmt_list_active);
    finalize_stmt(&stmt_list_all);
    finalize_stmt(&stmt_set_slot);
    finalize_stmt(&stmt_clear_slot);
    finalize_stmt(&stmt_count);

    if (db) {
        sqlite3_close(db);
        db = NULL;
        printf("[patient_data] Closed\n");
    }

    active_valid[0] = false;
    active_valid[1] = false;
}

/* ── CRUD ────────────────────────────────────────────────────── */

bool patient_data_save(patient_t *patient) {
    if (!db || !patient) return false;

    if (patient->id == 0) {
        /* INSERT */
        if (!stmt_insert) return false;

        sqlite3_reset(stmt_insert);
        bind_patient_fields(stmt_insert, patient, 1);

        int rc = sqlite3_step(stmt_insert);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "[patient_data] Insert failed: %s\n",
                    sqlite3_errmsg(db));
            return false;
        }

        patient->id = (int32_t)sqlite3_last_insert_rowid(db);
        printf("[patient_data] Inserted patient id=%d name='%s'\n",
               patient->id, patient->name);
    } else {
        /* UPDATE */
        if (!stmt_update) return false;

        sqlite3_reset(stmt_update);
        bind_patient_fields(stmt_update, patient, 1);
        sqlite3_bind_int(stmt_update, 18, patient->id);

        int rc = sqlite3_step(stmt_update);
        if (rc != SQLITE_DONE) {
            fprintf(stderr, "[patient_data] Update failed (id=%d): %s\n",
                    patient->id, sqlite3_errmsg(db));
            return false;
        }

        printf("[patient_data] Updated patient id=%d\n", patient->id);
    }

    /* Refresh active cache if this patient is monitored */
    if (patient->active && patient->monitor_slot < 2) {
        active_patients[patient->monitor_slot] = *patient;
        active_valid[patient->monitor_slot] = true;
    }

    return true;
}

bool patient_data_delete(int32_t patient_id) {
    if (!db || !stmt_delete || patient_id <= 0) return false;

    /* Clear from cache if active */
    for (int i = 0; i < 2; i++) {
        if (active_valid[i] && active_patients[i].id == patient_id) {
            active_valid[i] = false;
        }
    }

    sqlite3_reset(stmt_delete);
    sqlite3_bind_int(stmt_delete, 1, patient_id);

    int rc = sqlite3_step(stmt_delete);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "[patient_data] Delete failed (id=%d): %s\n",
                patient_id, sqlite3_errmsg(db));
        return false;
    }

    int changes = sqlite3_changes(db);
    if (changes == 0) {
        printf("[patient_data] Delete: patient id=%d not found\n", patient_id);
        return false;
    }

    printf("[patient_data] Deleted patient id=%d\n", patient_id);
    return true;
}

bool patient_data_get(int32_t patient_id, patient_t *out) {
    if (!db || !stmt_get_by_id || !out || patient_id <= 0) return false;

    sqlite3_reset(stmt_get_by_id);
    sqlite3_bind_int(stmt_get_by_id, 1, patient_id);

    if (sqlite3_step(stmt_get_by_id) == SQLITE_ROW) {
        read_patient_row(stmt_get_by_id, out);
        return true;
    }

    return false;
}

/* ── Queries ─────────────────────────────────────────────────── */

int patient_data_list_active(patient_list_t *out) {
    if (!db || !stmt_list_active || !out) return 0;
    out->count = 0;

    sqlite3_reset(stmt_list_active);

    int i = 0;
    while (sqlite3_step(stmt_list_active) == SQLITE_ROW && i < PATIENT_MAX_STORED) {
        read_patient_row(stmt_list_active, &out->patients[i]);
        i++;
    }
    out->count = i;
    return i;
}

int patient_data_list_all(patient_list_t *out) {
    if (!db || !stmt_list_all || !out) return 0;
    out->count = 0;

    sqlite3_reset(stmt_list_all);

    int i = 0;
    while (sqlite3_step(stmt_list_all) == SQLITE_ROW && i < PATIENT_MAX_STORED) {
        read_patient_row(stmt_list_all, &out->patients[i]);
        i++;
    }
    out->count = i;
    return i;
}

bool patient_data_find_by_mrn(const char *mrn, patient_t *out) {
    if (!db || !stmt_find_by_mrn || !mrn || !out) return false;

    sqlite3_reset(stmt_find_by_mrn);
    sqlite3_bind_text(stmt_find_by_mrn, 1, mrn, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt_find_by_mrn) == SQLITE_ROW) {
        read_patient_row(stmt_find_by_mrn, out);
        return true;
    }

    return false;
}

/* ── Monitor association ─────────────────────────────────────── */

bool patient_data_associate(int32_t patient_id, uint8_t slot) {
    if (!db || !stmt_set_slot || !stmt_clear_slot || patient_id <= 0 || slot > 1)
        return false;

    /* Clear any existing patient from this slot */
    sqlite3_reset(stmt_clear_slot);
    sqlite3_bind_int(stmt_clear_slot, 1, (int)slot);
    sqlite3_step(stmt_clear_slot);

    /* Assign the new patient to this slot */
    sqlite3_reset(stmt_set_slot);
    sqlite3_bind_int(stmt_set_slot, 1, (int)slot);
    sqlite3_bind_int(stmt_set_slot, 2, patient_id);

    int rc = sqlite3_step(stmt_set_slot);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "[patient_data] Associate failed (id=%d, slot=%d): %s\n",
                patient_id, slot, sqlite3_errmsg(db));
        return false;
    }

    /* Refresh cache */
    reload_active_cache();

    printf("[patient_data] Associated patient id=%d to slot %d\n",
           patient_id, slot);
    return true;
}

bool patient_data_disassociate(uint8_t slot) {
    if (!db || !stmt_clear_slot || slot > 1) return false;

    sqlite3_reset(stmt_clear_slot);
    sqlite3_bind_int(stmt_clear_slot, 1, (int)slot);

    int rc = sqlite3_step(stmt_clear_slot);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "[patient_data] Disassociate failed (slot=%d): %s\n",
                slot, sqlite3_errmsg(db));
        return false;
    }

    active_valid[slot] = false;

    printf("[patient_data] Disassociated slot %d\n", slot);
    return true;
}

const patient_t *patient_data_get_active(uint8_t slot) {
    if (slot > 1 || !active_valid[slot]) return NULL;
    return &active_patients[slot];
}

/* ── Convenience ─────────────────────────────────────────────── */

void patient_data_create_default(patient_t *out) {
    if (!out) return;
    memset(out, 0, sizeof(*out));
    out->id            = 0;
    out->gender        = PATIENT_GENDER_UNKNOWN;
    out->weight_kg     = 0.0f;
    out->height_cm     = 0.0f;
    out->admitted_ts   = 0;
    out->discharged_ts = 0;
    out->active        = false;
    out->monitor_slot  = 0;
}

bool patient_data_admit(patient_t *patient) {
    if (!patient) return false;

    patient->admitted_ts   = (uint32_t)time(NULL);
    patient->discharged_ts = 0;
    patient->active        = true;

    return patient_data_save(patient);
}

bool patient_data_discharge(int32_t patient_id) {
    if (patient_id <= 0) return false;

    patient_t p;
    if (!patient_data_get(patient_id, &p)) {
        fprintf(stderr, "[patient_data] Discharge failed: patient id=%d not found\n",
                patient_id);
        return false;
    }

    p.discharged_ts = (uint32_t)time(NULL);
    p.active        = false;

    /* Clear from monitor slot cache */
    if (p.monitor_slot < 2) {
        active_valid[p.monitor_slot] = false;
    }

    return patient_data_save(&p);
}
