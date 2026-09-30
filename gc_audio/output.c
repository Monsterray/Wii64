// SPDX-License-Identifier: GPL-2.0-or-later
#include "output.h"
#include <math.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

static int16_t saturate(int64_t value)
{
    return value > 32767 ? 32767 : value < -32768 ? -32768 : (int16_t)value;
}

void audio_output_reset(audio_output_t *s, unsigned int rate, int hifi, int preserve)
{
    memset(s, 0, sizeof(*s));
    /* A corrupt DAC register must not create zero steps or unbounded DSP ratios. */
    s->rate = rate < 1000 ? 1000 : rate > 144000 ? 144000 : rate;
    s->hifi = !!hifi;
    s->preserve = !!preserve;
    s->step = ((uint64_t)s->rate << 16) / 48000;
    unsigned int processing_rate = hifi ? 48000 : s->rate;
    s->hop = processing_rate / 100; /* 20 ms window, 10 ms output hop */
    if (s->hop < 32) s->hop = 32;
    if (s->hop > 1024) s->hop = 1024;
    s->seek = processing_rate / 500; /* joint stereo search, +/- 2 ms */
    if (s->seek > 288) s->seek = 288;
    if (!hifi) return;
    double cutoff = s->rate > 48000 ? 48000.0 / s->rate : 1.0;
    cutoff *= 0.90;
    for (unsigned int phase = 0; phase < 256; phase++) {
        double weights[8], sum = 0;
        for (unsigned int tap = 0; tap < 8; tap++) {
            double x = (int)tap - 3 - phase / 256.0;
            double sinc = fabs(x) < 1e-12 ? cutoff :
                sin(3.141592653589793 * cutoff * x) / (3.141592653589793 * x);
            double window = fabs(x) >= 4 ? 0 : 0.5 + 0.5 * cos(3.141592653589793 * x / 4);
            weights[tap] = sinc * window;
            sum += weights[tap];
        }
        int32_t total = 0;
        for (unsigned int tap = 0; tap < 8; tap++) {
            s->coefficients[phase][tap] = (int32_t)lround(weights[tap] * 65536 / sum);
            total += s->coefficients[phase][tap];
        }
        s->coefficients[phase][3] += 65536 - total; /* exact unity DC gain */
    }
}

static void emit_frame(audio_output_t *s, int16_t left, int16_t right,
                       audio_emit_t emit, void *user)
{
    s->output[s->emitted][0] = left;
    s->output[s->emitted++][1] = right;
    if (s->emitted == 288) {
        emit(user, &s->output[0][0], s->emitted);
        s->emitted = 0;
    }
}

static uint64_t match_error(const audio_output_t *s, unsigned int offset, uint64_t limit)
{
    uint64_t error = 0;
    for (unsigned int i = 0; i < s->hop; i += 8) {
        for (unsigned int channel = 0; channel < 2; channel++) {
            int64_t d = s->tail[i][channel] - s->input[offset + i][channel];
            error += (uint64_t)(d * d);
        }
        if (error > limit) break; /* cannot beat the current match */
    }
    return error;
}

static unsigned int find_match(const audio_output_t *s, unsigned int target)
{
    unsigned int lo = target > s->seek ? target - s->seek : 0;
    unsigned int hi = target + s->seek, best = target;
    uint64_t best_error = match_error(s, target, UINT64_MAX);
    if (!best_error) return target;
    for (unsigned int offset = lo; offset <= hi; offset++) {
        uint64_t error = match_error(s, offset, best_error);
        if (error < best_error || (error == best_error &&
                abs((int)offset - (int)target) < abs((int)best - (int)target))) {
            best_error = error;
            best = offset;
        }
    }
    return best;
}

static void stretch_frame(audio_output_t *s, int16_t left, int16_t right,
                          float speed, audio_emit_t emit, void *user)
{
    if (!s->preserve) {
        emit_frame(s, left, right, emit, user);
        return;
    }
    /* FIFO stays below one window plus one analysis hop/search margin. */
    s->input[s->queued][0] = left;
    s->input[s->queued++][1] = right;
    unsigned int h = s->hop, window = 2 * h;
    if (!s->primed && s->queued >= window) {
        for (unsigned int i = 0; i < h; i++)
            emit_frame(s, s->input[i][0], s->input[i][1], emit, user);
        memcpy(s->tail, s->input + h, h * sizeof(s->tail[0]));
        s->analysis = speed * h;
        s->primed = 1;
    }
    while (s->primed && s->analysis + s->seek + window <= s->queued) {
        unsigned int best = find_match(s, (unsigned int)s->analysis);
        for (unsigned int i = 0; i < h; i++) {
            int16_t pair[2];
            for (unsigned int channel = 0; channel < 2; channel++) {
                int32_t mixed = s->tail[i][channel] * (int32_t)(h - i) +
                    s->input[best + i][channel] * (int32_t)i;
                pair[channel] = saturate(mixed / (int32_t)h);
            }
            emit_frame(s, pair[0], pair[1], emit, user);
        }
        memcpy(s->tail, s->input + best + h, h * sizeof(s->tail[0]));
        s->analysis += speed * h; /* nominal clock prevents seek-induced tempo drift */
        unsigned int drop = s->analysis > s->seek ? (unsigned int)s->analysis - s->seek : 0;
        if (drop > s->queued) drop = s->queued;
        memmove(s->input, s->input + drop, (s->queued - drop) * sizeof(s->input[0]));
        s->queued -= drop;
        s->analysis -= drop;
    }
}

void audio_output_push(audio_output_t *s, const int16_t *input, size_t frames,
                       float speed, audio_emit_t emit, void *user)
{
    if (!isfinite(speed) || speed <= 0) speed = 1;
    if (speed < 0.5f) speed = 0.5f;
    if (speed > 2) speed = 2;
    if (!s->hifi && !s->preserve) {
        if (frames) emit(user, input, frames);
        return;
    }
    for (size_t i = 0; i < frames; i++) {
        if (!s->hifi) {
            stretch_frame(s, input[2*i], input[2*i+1], speed, emit, user);
            continue;
        }
        uint64_t latest = s->received++;
        s->history[latest & 7][0] = input[2*i];
        s->history[latest & 7][1] = input[2*i+1];
        while ((s->position >> 16) + 4 <= latest) {
            int64_t center = (int64_t)(s->position >> 16);
            unsigned int phase = (s->position & 65535) >> 8;
            int16_t pair[2];
            for (unsigned int channel = 0; channel < 2; channel++) {
                int64_t sum = 0;
                for (unsigned int tap = 0; tap < 8; tap++) {
                    int64_t index = center + tap - 3;
                    if (index >= 0)
                        sum += (int64_t)s->history[index & 7][channel] * s->coefficients[phase][tap];
                }
                pair[channel] = saturate((sum + 32768) >> 16);
            }
            stretch_frame(s, pair[0], pair[1], speed, emit, user);
            s->position += s->step;
        }
    }
    if (s->emitted) {
        emit(user, &s->output[0][0], s->emitted);
        s->emitted = 0;
    }
}
