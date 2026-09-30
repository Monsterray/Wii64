/* Streaming invariants and objective pitch checks, not a listening-quality claim. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../gc_audio/output.c" /* also test the internal search against exhaustive scores */
#include "../rsp_hle/audio.h"

static audio_output_t processor;
static int16_t input[96000 * 2], captured[200000 * 2], reference[200000 * 2];
static size_t count;
static void search_test(void)
{
    uint32_t random = 1;
    for (unsigned int trial = 0; trial < 200; trial++) {
        audio_output_reset(&processor, 48000, 0, 1);
        processor.hop = 64;
        processor.seek = 16;
        for (unsigned int i = 0; i < 192; i++)
            for (unsigned int c = 0; c < 2; c++) {
                random = random * 1664525u + 1013904223u;
                processor.input[i][c] = trial < 100 ? (int16_t)(random >> 16) : (i % 4) * 100;
            }
        memcpy(processor.tail, processor.input + (trial % 64), sizeof(processor.tail[0]) * 64);
        unsigned int target = 64, best = target;
        uint64_t score = match_error(&processor, target, UINT64_MAX);
        for (unsigned int offset = 48; offset <= 80; offset++) {
            uint64_t full = match_error(&processor, offset, UINT64_MAX);
            uint64_t pruned = match_error(&processor, offset, score);
            assert(full <= score ? pruned == full : pruned > score);
            if (full < score || (full == score && abs((int)offset - 64) < abs((int)best - 64))) {
                score = full;
                best = offset;
            }
        }
        assert(find_match(&processor, target) == best);
    }
}
static void capture(void *user, const int16_t *samples, size_t frames)
{
    (void)user;
    assert(count + frames <= 200000);
    memcpy(captured + count * 2, samples, frames * 4);
    count += frames;
}
static void tone(unsigned int rate)
{
    for (unsigned int i = 0; i < rate; i++) {
        input[2*i] = (int16_t)lround(12000 * sin(2 * 3.141592653589793 * 440 * i / rate));
        input[2*i+1] = -input[2*i];
    }
}
static double frequency(unsigned int rate)
{
    unsigned int crossings = 0;
    size_t begin = rate / 10, end = count - rate / 10;
    assert(end > begin);
    for (size_t i = begin + 1; i < end; i++)
        if (captured[2*(i-1)] <= 0 && captured[2*i] > 0) crossings++;
    return crossings * (double)rate / (end - begin);
}
static void run(unsigned int rate, int hifi, int preserve, float speed, size_t chunk)
{
    audio_output_reset(&processor, rate, hifi, preserve);
    count = 0;
    for (size_t i = 0; i < rate; i += chunk) {
        size_t n = rate - i < chunk ? rate - i : chunk;
        audio_output_push(&processor, input + 2*i, n, speed, capture, NULL);
        assert(processor.queued < 8192);
    }
}
int main(void)
{
    search_test();
    int16_t ramp[4] = { -1000, 0, 1000, 2000 };
    assert(audio_cubic(ramp, 0) == 0);
    assert(abs(audio_cubic(ramp, 32768) - 500) <= 1);
    int16_t dc[4] = { -32768, -32768, -32768, -32768 };
    for (unsigned int p = 0; p < 65536; p += 137) assert(audio_cubic(dc, p) == -32768);
    assert(audio_mix_hifi(0, 3, 16384, 15) == 2);
    assert(audio_mix_hifi(0, -3, 16384, 15) == -2);
    assert(audio_mix_hifi(32767, 32767, 32767, 15) == 32767);
    assert(audio_mix_hifi(-32768, -32768, INT64_C(1) << 30, 30) == -32768);
    unsigned int rates[] = { 8000, 32000, 44100, 48000, 96000 };
    for (unsigned int r = 0; r < sizeof(rates) / sizeof(rates[0]); r++) {
        unsigned int rate = rates[r];
        tone(rate);
        run(rate, 0, 0, 1, 97);
        assert(count == rate && !memcmp(input, captured, count * 4));
        for (int hifi = 0; hifi <= 1; hifi++) {
            for (int preserve = 0; preserve <= 1; preserve++) {
                for (int mode = 0; mode < 3; mode++) {
                    float speed = mode == 0 ? 0.5f : mode == 1 ? 1 : 2;
                    run(rate, hifi, preserve, speed, rate);
                    size_t expected_count = count;
                    memcpy(reference, captured, count * 4);
                    run(rate, hifi, preserve, speed, 137);
                    assert(count == expected_count && !memcmp(reference, captured, count * 4));
                    unsigned int out_rate = hifi ? 48000 : rate;
                    double hz = frequency(out_rate);
                    assert(fabs(hz - 440) < 5);
                    double duration = count / (double)out_rate;
                    assert(fabs(duration - (preserve ? 1.0 / speed : 1.0)) < 0.06);
                    for (size_t i = 0; i < count; i++)
                        assert(abs(captured[2*i] + captured[2*i+1]) <= 1);
                    printf("rate=%u sinc=%d stretch=%d speed=%.1f: %.3fs %.2fHz\n",
                           rate, hifi, preserve, speed, duration, hz);
                }
            }
        }
    }
    /* Rate limits, empty/silent input, changing speed and long-running FIFO bounds. */
    memset(input, 0, sizeof(input));
    audio_output_reset(&processor, 144000, 1, 1);
    for (unsigned int i = 0; i < 1000; i++) {
        count = 0;
        audio_output_push(&processor, input, 97, i % 2 ? 0.5f : 2, capture, NULL);
        assert(processor.queued < 8192);
        for (size_t k = 0; k < count * 2; k++) assert(captured[k] == 0);
    }
    audio_output_reset(&processor, 0, 1, 1);
    assert(processor.rate == 1000 && processor.step);
    audio_output_push(&processor, input, 0, NAN, capture, NULL);
    puts("audio enhancement invariants: ok");
    return 0;
}
