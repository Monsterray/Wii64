/* Host check of shared residual arithmetic and both alist ADPCM formats.
 * cc -std=c11 -O2 -ffunction-sections -fdata-sections -Wl,-dead_strip \
 *   -fsanitize=address,undefined -fno-sanitize-recover=all tests/adpcm_test.c \
 *   rsp_hle/alist.c rsp_hle/audio.c rsp_hle/memory.c -o /tmp/wii64_adpcm_test
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../rsp_hle/alist.h"
#include "../rsp_hle/audio.h"
#include "../rsp_hle/hle_internal.h"
#include "../rsp_hle/memory.h"

static uint32_t seed = 0x64ad9c1u;

static uint32_t random_value(void)
{
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

/* Convert modulo 2^32, then arithmetic-shift before 16-bit saturation. */
static int16_t reference_sample(int64_t value)
{
    uint32_t bits = (uint32_t)value;
    value = bits <= INT32_MAX ? bits : (int64_t)bits - INT64_C(4294967296);
    value = value >= 0 ? value / 2048 : -((-value + 2047) / 2048);
    return value < INT16_MIN ? INT16_MIN : value > INT16_MAX ? INT16_MAX : value;
}

static void check_residuals(void)
{
    for (unsigned trial = 0; trial < 512; trial++) {
        for (size_t count = 0; count <= 8; count++) {
            int16_t actual[64], expected[64];
            for (unsigned i = 0; i < 64; i++)
                actual[i] = trial == 0 ? INT16_MIN : trial == 1 ? INT16_MAX :
                    trial == 2 ? (i & 1 ? INT16_MIN : INT16_MAX) : (int16_t)random_value();
            memcpy(expected, actual, sizeof(actual));
            unsigned dst = 24, src = 8, book = 40, history = 22;
            if (trial % 5 == 1) src = dst;
            if (trial % 5 == 2) src = dst - 1;
            if (trial % 5 == 3) src = dst + 1;
            if (trial % 5 == 4) book = dst;
            if (trial & 1) history = dst + 6;
            int16_t l1 = expected[history], l2 = expected[history + 1];
            for (size_t i = 0; i < count; i++) {
                int64_t value = (int64_t)expected[src + i] * 2048;
                value += (int64_t)expected[book + i] * l1;
                value += (int64_t)expected[book + 8 + i] * l2;
                for (size_t j = 0; j < i; j++)
                    value += (int64_t)expected[book + 8 + j] * expected[src + i - 1 - j];
                expected[dst + i] = reference_sample(value);
            }
            adpcm_compute_residuals(actual + dst, actual + src, actual + book,
                actual + history, count);
            assert(memcmp(actual, expected, sizeof(actual)) == 0);
        }
    }
    int16_t x[32], y[32];
    for (unsigned trial = 0; trial < 256; trial++) {
        for (unsigned i = 0; i < 32; i++) {
            x[i] = trial == 0 ? INT16_MIN : (int16_t)random_value();
            y[i] = trial == 0 ? INT16_MIN : (int16_t)random_value();
        }
        for (size_t count = 0; count <= 32; count++) {
            int64_t sum = 0;
            for (size_t i = 0; i < count; i++) sum += (int64_t)x[i] * y[count - i - 1];
            assert((uint32_t)rdot(count, x, y) == (uint32_t)sum);
        }
    }
    puts("ADPCM residual and reverse-dot reference: pass");
}

static uint64_t hash_bytes(uint64_t hash, const void *data, size_t size)
{
    const uint8_t *bytes = data;
    for (size_t i = 0; i < size; i++) {
        hash ^= bytes[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

static void check_frame_boundaries(void)
{
    const uint16_t starts[] = {0, 0xfe0, 0xfe2, 0xff0, 0xffe, 0x1fe0, 0xffe0};
    struct hle_t hle = {0};
    uint8_t dram[0x1000], expected[0x1000];
    int16_t book[256] = {0};
    hle.dram = dram;
    for (size_t n = 0; n < sizeof(starts) / sizeof(starts[0]); n++) {
        for (unsigned decoded = 0; decoded < 2; decoded++) {
            memset(dram, 0, sizeof(dram));
            memset(hle.alist_buffer, 0xa5, sizeof(hle.alist_buffer));
            memset(hle.alist_buffer + 0x800, 0, 32);
            memcpy(expected, hle.alist_buffer, sizeof(expected));
            for (unsigned i = 0; i < 16; i++) {
                int16_t sample = (int)i * 4095 - 30000;
                *u16(dram, 0x100 + i * 2) = (uint16_t)sample;
                memcpy(expected + (((starts[n] + i * 2) ^ S16) & 0xfff), &sample, 2);
            }
            if (decoded) {
                for (unsigned i = 0; i < 16; i++)
                    memset(expected + (((starts[n] + 32 + i * 2) ^ S16) & 0xfff), 0, 2);
            }
            alist_adpcm(&hle, 0, 0, 0, starts[n], 0x800, decoded * 32,
                book, 0x200, 0x100);
            assert(memcmp(expected, hle.alist_buffer, sizeof(expected)) == 0);
        }
    }
    puts("ADPCM frame boundary stores: pass");
}

int main(void)
{
    check_residuals();
    check_frame_boundaries();
    struct hle_t hle = {0};
    uint8_t dram[0x1000];
    int16_t book[256];
    uint64_t hash = UINT64_C(14695981039346656037);
    hle.dram = dram;
    seed = 0x64ad9c1u;
    for (unsigned trial = 0; trial < 256; trial++) {
        for (size_t i = 0; i < sizeof(hle.alist_buffer); i++) hle.alist_buffer[i] = random_value();
        for (size_t i = 0; i < sizeof(dram); i++) dram[i] = random_value();
        for (unsigned i = 0; i < 256; i++) book[i] = (int16_t)random_value();
        uint16_t dst = trial % 5 == 0 ? 0xff0 : 0x400;
        uint16_t src = trial % 7 == 0 ? dst : trial % 3 == 0 ? 0xff8 : 0x800;
        for (unsigned pass = 0; pass < 2; pass++) {
            alist_adpcm(&hle, pass == 0 && trial % 3 == 0, trial & 1,
                trial & 2, dst, src, (trial % 7) * 32, book, 0x200, 0x100);
            hash = hash_bytes(hash, hle.alist_buffer, sizeof(hle.alist_buffer));
            hash = hash_bytes(hash, dram, sizeof(dram));
        }
    }
    printf("alist ADPCM PCM/state snapshot: %016llx\n", (unsigned long long)hash);
#ifdef _BIG_ENDIAN
    return hash != UINT64_C(0x06b765ee2c02a0c4);
#else
    return hash != UINT64_C(0x9b02f6bc319d81f1);
#endif
}
