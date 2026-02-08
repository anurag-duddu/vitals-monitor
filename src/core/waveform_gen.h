/**
 * @file waveform_gen.h
 * @brief ECG and Pleth waveform sample synthesis from lookup tables
 *
 * Generates one sample at a time, rate-adjusted to the current heart rate.
 * Each call to waveform_gen_next_sample() advances a fixed-point phase
 * accumulator, producing smooth continuous output even when HR changes.
 */

#ifndef WAVEFORM_GEN_H
#define WAVEFORM_GEN_H

#include <stdint.h>

/* Waveform identifiers */
typedef enum {
    WAVEFORM_ECG  = 0,
    WAVEFORM_PLETH,
    WAVEFORM_COUNT
} waveform_id_t;

/* Waveform generator state (one per waveform) */
typedef struct {
    waveform_id_t id;
    const int16_t *lut;       /* Pointer to lookup table (one cardiac cycle) */
    uint16_t       lut_len;   /* Number of samples in the LUT */
    uint32_t       phase_acc; /* Fixed-point phase accumulator (16.16) */
    uint32_t       phase_inc; /* Fixed-point phase increment per output sample */
    int16_t        amplitude; /* Scale factor (chart Y range) */
    int16_t        offset;    /* DC offset for chart positioning */
} waveform_gen_t;

/**
 * Initialize a waveform generator and build the lookup table (once).
 *
 * @param gen       Pointer to generator state
 * @param id        Which waveform (WAVEFORM_ECG or WAVEFORM_PLETH)
 * @param amplitude Y-axis scale factor (e.g. 180 for chart range 0..400)
 * @param offset    Y-axis DC offset (e.g. 200 for centering in 0..400)
 */
void waveform_gen_init(waveform_gen_t *gen, waveform_id_t id,
                       int16_t amplitude, int16_t offset);

/**
 * Update the phase increment based on the current heart rate.
 * Call when mock_data delivers a new HR value (~1 Hz).
 *
 * @param gen             Pointer to generator state
 * @param current_hr      Current heart rate in bpm
 * @param samples_per_sec Output sample rate (pixels/sec, e.g. 128)
 */
void waveform_gen_set_hr(waveform_gen_t *gen, int current_hr,
                         int samples_per_sec);

/**
 * Generate the next output sample. Advances the phase accumulator.
 *
 * @param gen  Pointer to generator state
 * @return     Scaled sample value suitable for lv_chart_set_next_value()
 */
int32_t waveform_gen_next_sample(waveform_gen_t *gen);

#endif /* WAVEFORM_GEN_H */
