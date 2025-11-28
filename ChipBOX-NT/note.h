#ifndef NOTE_H
#define NOTE_H

#include <stdint.h>
#include "instrument.h"

typedef enum {
    NOTE_OFF,
    NOTE_ON
} note_status_t;

typedef struct {
    uint8_t note = 0;
    uint8_t note_rel = 0;
    uint8_t vol = 0;
    uint8_t rel_vol = 0;
    int8_t vibSpd = 0;
    int32_t vibX = 0;
    float freq = 0;
    wave_type_t wave;

    uint32_t master_tick = 0;
    uint8_t vol_micro_tick = 0;
    uint8_t arp_micro_tick = 0;
    uint8_t lfo_micro_tick = 0;
    uint8_t dty_micro_tick = 0;

    int8_t vol_tick = 0;
    int8_t arp_tick = 0;
    int8_t lfo_tick = 0;
    int8_t dty_tick = 0;

    note_status_t status = NOTE_OFF;
    int16_t last_sample = 0;
} note_info_t;

typedef struct {
    note_status_t status = NOTE_OFF;
    uint32_t freq = 0;
    uint8_t vol = 0;
    float time = 0;
} sample_info_t;

sample_info_t samples[MAX_POLYPHONY];
note_info_t notes[MAX_POLYPHONY];

#endif