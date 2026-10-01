#ifndef GLN64_TEXTURE_HASH_H
#define GLN64_TEXTURE_HASH_H

#include <stdint.h>
#include <string.h>
#include "CRC.h"

// All fields describe bytes read by the existing seeded TMEM hash.
struct TextureHashKey {
    uint32_t tmem, bytes_per_line, height, line, palette_offset, palette_bytes;
};
static_assert(sizeof(TextureHashKey) == 6 * sizeof(uint32_t), "Hash key must not contain padding");

struct TextureHashMemo {
    TextureHashKey key;
    uint32_t hash;
    bool valid;
    bool dirty;
    uint64_t snapshot[512];
};

static inline uint32_t TextureHash_Calculate(void *tmem, const TextureHashKey &key)
{
    uint32_t hash = 0xFFFFFFFF;
    unsigned char *bytes = static_cast<unsigned char *>(tmem);
    for (uint32_t y = 0; y < key.height; ++y) {
        const uint32_t offset = (key.tmem + key.line * y) & 0x1FF;
        const uint32_t available = (512 - offset) << 3;
        hash = Hash_Calculate(hash, bytes + (offset << 3),
                             key.bytes_per_line < available ? key.bytes_per_line : available);
    }
    if (key.palette_bytes)
        hash = Hash_Calculate(hash, bytes + (key.palette_offset << 3), key.palette_bytes);
    return hash;
}

static inline uint32_t TextureHash_Get(TextureHashMemo &memo, void *tmem,
                                       const TextureHashKey &key)
{
    bool matches = memo.valid && !memcmp(&memo.key, &key, sizeof(key));
    unsigned char *bytes = static_cast<unsigned char *>(tmem);
    unsigned char *saved = reinterpret_cast<unsigned char *>(memo.snapshot);
    if (matches && memo.dirty) {
        for (uint32_t y = 0; y < key.height && matches; ++y) {
            const uint32_t offset = (key.tmem + key.line * y) & 0x1FF;
            const uint32_t available = (512 - offset) << 3;
            const uint32_t count = key.bytes_per_line < available ? key.bytes_per_line : available;
            matches = !memcmp(saved + (offset << 3), bytes + (offset << 3), count);
        }
        if (matches && key.palette_bytes)
            matches = !memcmp(saved + (key.palette_offset << 3),
                              bytes + (key.palette_offset << 3), key.palette_bytes);
    }
    if (!matches) {
        memo.hash = TextureHash_Calculate(tmem, key);
        // Save only bytes used by this key. Other TMEM bytes cannot affect its hash.
        for (uint32_t y = 0; y < key.height; ++y) {
            const uint32_t offset = (key.tmem + key.line * y) & 0x1FF;
            const uint32_t available = (512 - offset) << 3;
            const uint32_t count = key.bytes_per_line < available ? key.bytes_per_line : available;
            memcpy(saved + (offset << 3), bytes + (offset << 3), count);
        }
        if (key.palette_bytes)
            memcpy(saved + (key.palette_offset << 3), bytes + (key.palette_offset << 3), key.palette_bytes);
        memo.key = key;
        memo.valid = true;
    }
    memo.dirty = false;
    return memo.hash;
}

#endif
