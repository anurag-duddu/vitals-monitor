/**
 * @file waveform_gen.c
 * @brief ECG and Pleth waveform synthesis using Gaussian lookup tables
 *
 * Each waveform is modeled as a sum of Gaussian pulses representing the
 * characteristic features of the cardiac cycle.  The lookup tables are
 * built once at init time; runtime sample generation is pure integer math
 * using a fixed-point (16.16) phase accumulator.
 */

#include "waveform_gen.h"
#include <math.h>
#include <string.h>
#include <stdbool.h>

/* ── Lookup table sizes ─────────────────────────────────────── */

#define ECG_LUT_LEN   256
#define PLETH_LUT_LEN 256

static int16_t ecg_lut[ECG_LUT_LEN];
static int16_t pleth_lut[PLETH_LUT_LEN];
static bool    luts_initialized = false;

/* ── Gaussian component descriptors ─────────────────────────── */

typedef struct {
    float center;   /* LUT index of peak */
    float sigma;    /* Width (standard deviation in index units) */
    float amp;      /* Amplitude [-1000..+1000] */
} gauss_component_t;

/*
 * ECG PQRST morphology (Lead II):
 *   P wave  — small positive bump ~index 48
 *   Q wave  — small negative dip  ~index 83
 *   R wave  — tall positive spike ~index 90
 *   S wave  — moderate negative   ~index 97
 *   T wave  — broad positive      ~index 150
 */
static const gauss_component_t ecg_components[] = {
    { 48.0f,   6.0f,   120.0f },  /* P wave */
    { 83.0f,   2.0f,  -100.0f },  /* Q wave */
    { 90.0f,   3.0f,   950.0f },  /* R wave */
    { 97.0f,   3.0f,  -250.0f },  /* S wave */
    { 150.0f, 15.0f,   300.0f },  /* T wave */
};
#define ECG_NUM_COMPONENTS 5

/*
 * Pleth (pulse oximetry) morphology:
 *   Systolic peak   — sharp rise ~index 50
 *   Dicrotic notch  — small dip  ~index 80
 *   Dicrotic wave   — broader    ~index 100
 */
static const gauss_component_t pleth_components[] = {
    { 50.0f,  12.0f,  900.0f },   /* Systolic peak */
    { 80.0f,   6.0f, -200.0f },   /* Dicrotic notch */
    { 100.0f, 18.0f,  400.0f },   /* Dicrotic wave */
};
#define PLETH_NUM_COMPONENTS 3

/* ── LUT generation (called once) ───────────────────────────── */

static void build_lut(int16_t *lut, int len,
                      const gauss_component_t *comps, int num_comps)
{
    for (int i = 0; i < len; i++) {
        float val = 0.0f;
        for (int c = 0; c < num_comps; c++) {
            float d = (float)i - comps[c].center;
            float s = comps[c].sigma;
            val += comps[c].amp * expf(-(d * d) / (2.0f * s * s));
        }
        /* Clamp to [-1000, +1000] */
        if (val > 1000.0f) val = 1000.0f;
        if (val < -1000.0f) val = -1000.0f;
        lut[i] = (int16_t)val;
    }
}

static void ensure_luts_built(void)
{
    if (luts_initialized) return;
    build_lut(ecg_lut,   ECG_LUT_LEN,   ecg_components,   ECG_NUM_COMPONENTS);
    build_lut(pleth_lut, PLETH_LUT_LEN, pleth_components, PLETH_NUM_COMPONENTS);
    luts_initialized = true;
}

/* ── Public API ─────────────────────────────────────────────── */

void waveform_gen_init(waveform_gen_t *gen, waveform_id_t id,
                       int16_t amplitude, int16_t offset)
{
    ensure_luts_built();
    memset(gen, 0, sizeof(*gen));

    gen->id        = id;
    gen->amplitude = amplitude;
    gen->offset    = offset;

    switch (id) {
        case WAVEFORM_ECG:
            gen->lut     = ecg_lut;
            gen->lut_len = ECG_LUT_LEN;
            break;
        case WAVEFORM_PLETH:
            gen->lut     = pleth_lut;
            gen->lut_len = PLETH_LUT_LEN;
            break;
        default:
            gen->lut     = ecg_lut;
            gen->lut_len = ECG_LUT_LEN;
            break;
    }

    /* Default: 72 bpm at 128 samples/sec */
    waveform_gen_set_hr(gen, 72, 128);
}

void waveform_gen_set_hr(waveform_gen_t *gen, int current_hr,
                         int samples_per_sec)
{
    if (!gen || current_hr <= 0 || samples_per_sec <= 0) return;

    /*
     * Phase increment (16.16 fixed-point):
     *
     *   cycles/sec = current_hr / 60
     *   LUT samples consumed per output sample = lut_len * cycles/sec / samples_per_sec
     *                                          = lut_len * current_hr / (60 * samples_per_sec)
     *
     *   In 16.16: phase_inc = (lut_len * current_hr * 65536) / (60 * samples_per_sec)
     */
    uint64_t num = (uint64_t)gen->lut_len * (uint64_t)current_hr * 65536ULL;
    uint64_t den = 60ULL * (uint64_t)samples_per_sec;
    gen->phase_inc = (uint32_t)(num / den);
}

int32_t waveform_gen_next_sample(waveform_gen_t *gen)
{
    if (!gen || !gen->lut) return 0;

    /* Extract integer LUT index from 16.16 phase accumulator */
    uint16_t idx = (uint16_t)((gen->phase_acc >> 16) % gen->lut_len);

    /* Advance phase for next call */
    gen->phase_acc += gen->phase_inc;

    /* Scale raw LUT value [-1000..+1000] to output range and apply offset */
    int32_t raw = gen->lut[idx];
    int32_t scaled = (raw * (int32_t)gen->amplitude) / 1000 + (int32_t)gen->offset;
    return scaled;
}
