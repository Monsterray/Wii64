// SPDX-License-Identifier: GPL-2.0-or-later
/* Optional CPU enhancements before AESND's DSP playback. Stereo frames throughout. */
#ifndef WII64_AUDIO_OUTPUT_H
#define WII64_AUDIO_OUTPUT_H
#include <stddef.h>
#include <stdint.h>

typedef void (*audio_emit_t)(void *, const int16_t *, size_t);
typedef struct {
    unsigned int rate, hifi, preserve, hop, seek, queued, primed, emitted;
    uint64_t received, position, step;
    double analysis;
    int32_t coefficients[256][8];
    int16_t history[8][2], input[8192][2], tail[1024][2], output[288][2];
} audio_output_t;

void audio_output_reset(audio_output_t *, unsigned int rate, int hifi, int preserve);
void audio_output_push(audio_output_t *, const int16_t *, size_t frames,
                       float speed, audio_emit_t, void *);
#endif
