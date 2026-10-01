#include <assert.h>
#include <stdint.h>
#include <stdio.h>

// Use the renderer's actual XXH32 implementation, not a substitute hash.
#define Hash_Calculate RawHash_Calculate
#include "../glN64_GX/CRC.cpp"
#undef Hash_Calculate
static unsigned hash_calls;
DWORD Hash_Calculate(DWORD hash, void *buffer, DWORD count)
{
    ++hash_calls;
    return RawHash_Calculate(hash, buffer, count);
}
#include "../glN64_GX/texture_hash.h"

static uint32_t legacy(uint64_t *tmem, const TextureHashKey &k)
{
    uint32_t hash = 0xFFFFFFFF;
    for (uint32_t y = 0; y < k.height; ++y) {
        uint32_t offset = (k.tmem + k.line * y) & 0x1FF;
        uint32_t available = (512 - offset) << 3;
        hash = RawHash_Calculate(hash, &tmem[offset],
                                k.bytes_per_line < available ? k.bytes_per_line : available);
    }
    if (k.palette_bytes)
        hash = RawHash_Calculate(hash, &tmem[k.palette_offset], k.palette_bytes);
    return hash;
}

int main()
{
    uint64_t tmem[512];
    uint32_t random = 12345;
    for (unsigned i = 0; i < sizeof(tmem); ++i) {
        random = random * 1664525 + 1013904223;
        reinterpret_cast<unsigned char *>(tmem)[i] = random >> 24;
    }
    // Constant-length dispatch must equal the original hash for every size,
    // including its fallback, palette lengths and zero-byte rows.
    for (unsigned count = 0; count <= sizeof(tmem); ++count) {
        random = random * 1664525 + 1013904223;
        assert(RawHash_Calculate(random, tmem, count) == XXH32(tmem, count, random));
    }
    TextureHashMemo memo[2] = {};
    // Exercise row wrapping, all texel sizes, CI/TLUT ranges, zero-height,
    // both texture units, key changes without writes, and writes with reset.
    for (unsigned i = 0; i < 10000; ++i) {
        random = random * 1664525 + 1013904223;
        const unsigned size = i & 3;
        TextureHashKey k = {random & 511, ((i % 257) << size) >> 1,
                            i % 65, (i % 512) << (size == 3), 0, 0};
        if (i & 4) {
            if (size == 0) { k.palette_offset = 256 + ((i % 16) << 4); k.palette_bytes = 128; }
            else if (size < 3) { k.palette_offset = 256; k.palette_bytes = 2048; }
        }
        unsigned unit = i & 1;
        assert(TextureHash_Get(memo[unit], tmem, k) == legacy(tmem, k));
        unsigned calls = hash_calls;
        assert(TextureHash_Get(memo[unit], tmem, k) == legacy(tmem, k));
        assert(hash_calls == calls);
        reinterpret_cast<unsigned char *>(tmem)[random % sizeof(tmem)] ^= 0xFF;
        memo[0].dirty = memo[1].dirty = true;
        assert(TextureHash_Get(memo[unit], tmem, k) == legacy(tmem, k));
    }
    TextureHashKey k = {0, 8, 1, 1, 0, 0};
    TextureHash_Get(memo[0], tmem, k);
    unsigned calls = hash_calls;
    // A redundant TMEM load must not rehash identical bytes.
    memo[0].dirty = true;
    assert(TextureHash_Get(memo[0], tmem, k) == legacy(tmem, k) && hash_calls == calls);
    // Writes outside this tile do not affect its hash.
    tmem[511] ^= 0xFFFFFFFF;
    memo[0].dirty = true;
    assert(TextureHash_Get(memo[0], tmem, k) == legacy(tmem, k) && hash_calls == calls);
    tmem[0] ^= 0xFFFFFFFF;
    memo[0].dirty = true;
    assert(TextureHash_Get(memo[0], tmem, k) == legacy(tmem, k) && hash_calls > calls);
    // A new ROM/cache reset must discard even a matching key.
    memo[0].valid = false;
    calls = hash_calls;
    TextureHash_Get(memo[0], tmem, k);
    assert(hash_calls > calls);
    // Palette-only changes must invalidate both full and CI4 palette hashes.
    const unsigned palette_sizes[] = {2048, 128};
    for (unsigned palette_bytes : palette_sizes) {
        TextureHashKey palette = {0, 8, 1, 1, palette_bytes == 128 ? 496U : 256U, palette_bytes};
        TextureHash_Get(memo[0], tmem, palette);
        calls = hash_calls;
        tmem[511] ^= 0x1234;
        memo[0].dirty = true;
        assert(TextureHash_Get(memo[0], tmem, palette) == legacy(tmem, palette) && hash_calls > calls);
    }
    TextureHash_Get(memo[0], tmem, k);
    memo[0].hash = 0; // Zero is a valid stored hash, not an empty sentinel.
    calls = hash_calls;
    assert(TextureHash_Get(memo[0], tmem, k) == 0 && hash_calls == calls);
    puts("TMEM hash memo: 10000 exact legacy comparisons and invalidation checks passed");
}
