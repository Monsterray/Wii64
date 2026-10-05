#!/usr/bin/env python3
"""Sanitized host check of the production texture mip-chain and probe code."""

from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "glN64_GX" / "TextureProbe.h"
SOURCE = ROOT / "glN64_GX" / "Textures.cpp"


def between(source, start, end):
    return source[source.index(start):source.index(end, source.index(start))]


def main():
    source = SOURCE.read_text()
    probe_api = between(source, "extern \"C\" void TextureCache_ProbeReset",
                        "static void TextureCache_ProbeTextureLoaded")
    loaded = between(source, "static void TextureCache_ProbeTextureLoaded",
                     "struct TextureCacheMipProbeTimer")
    timer = between(source, "struct TextureCacheMipProbeTimer", "#endif")
    cap = between(source, "static u32 _TextureCache_MipLevelCap",
                  "static void _TextureCache_DescribeMipLevel")
    describe = between(source, "static void _TextureCache_DescribeMipLevel",
                       "static void TextureCache_LoadMipChain")
    mip = between(source, "static void TextureCache_LoadMipChain",
                  "\nvoid TextureCache_Update")

    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        off = tmp / "off.cpp"
        off.write_text('#include "TextureProbe.h"\nint main() { return 0; }\n')
        subprocess.run(["c++", "-std=c++11", "-Wall", "-Wextra", "-Werror",
                        "-I", str(HEADER.parent), str(off), "-o", str(tmp / "off")],
                       check=True)
        c_header = tmp / "header.c"
        c_header.write_text('#include "TextureProbe.h"\nint main(void) { return 0; }\n')
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                        "-DPERF_PROF", "-DPERF_CACHE_PROBES", "-I", str(HEADER.parent),
                        str(c_header), "-o", str(tmp / "header")], check=True)

        harness = tmp / "mip.cpp"
        harness.write_text(r'''
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <set>
#include "TextureProbe.h"
using u8 = uint8_t; using u16 = uint16_t; using u32 = uint32_t;
#define PERF_PROF 1
#define PERF_CACHE_PROBES 1
#define PERF_MEM_TEXTURE 0
struct CachedTexture {
    u16 *GXtexture; u8 GXtexfmt, max_level;
    u32 GXrealWidth, GXrealHeight, textureBytes, size, format, realWidth, realHeight;
    u32 tMem, palette, maskS, maskT, line, clampS, clampT, mirrorS, mirrorT;
    u32 width, height, clampWidth, clampHeight;
};
struct gDPTile { u32 tmem,palette,masks,maskt,format,size,line,clamps,clampt,mirrors,mirrort; };
struct { struct { u32 textureLUT; } otherMode; gDPTile tiles[8]; } gDP;
struct { u32 GXsize; } imageFormat[4][8][8];
static void *GXtexCache;
static void *perfMem_allocate(int, void*, u32);
static void perfMem_free(int, void*, void*);
static bool TextureCache_FreeOneTexture();
static void TextureCache_Load(CachedTexture*);
static void DCFlushRange(void*, u32);
static TextureCacheProbeStats textureCacheProbeStats = {};
static unsigned long long fakeTime;
static unsigned long long gettime() { return ++fakeTime; }
''' + probe_api + loaded + timer + cap + describe + mip + r'''
static std::set<void*> live;
static unsigned allocCalls, failAllocAt, loadCalls, failLoadAt, freeRetries;
static bool retryEviction;
static void *perfMem_allocate(int, void*, u32 n) {
    ++allocCalls;
    if (allocCalls == failAllocAt) return nullptr;
    void *p = std::malloc(n ? n : 1); assert(p); live.insert(p); return p;
}
static void perfMem_free(int, void*, void *p) {
    assert(p && live.erase(p) == 1); std::free(p);
}
static bool TextureCache_FreeOneTexture() { ++freeRetries; return retryEviction; }
static void DCFlushRange(void *p, u32 bytes) { assert(live.count(p) == 1 && bytes != 0); }
static void TextureCache_Load(CachedTexture *t) {
    ++textureCacheProbeStats.texture_loads; ++loadCalls;
    if (loadCalls == failLoadAt) { t->GXtexture = nullptr; t->textureBytes = 0; return; }
    t->textureBytes = t->realWidth * t->realHeight;
    t->GXtexture = static_cast<u16*>(perfMem_allocate(0, nullptr, t->textureBytes));
    if (!t->GXtexture) { t->textureBytes = 0; return; }
    t->GXrealWidth = t->realWidth; t->GXrealHeight = t->realHeight; t->GXtexfmt = 7;
    u8 *bytes = reinterpret_cast<u8*>(t->GXtexture);
    for (u32 i = 0; i < t->textureBytes; ++i) bytes[i] = (u8)(loadCalls * 13 + i);
    TextureCache_ProbeTextureLoaded(t);
}
static CachedTexture base(u8 levels = 2) {
    CachedTexture t = {}; t.max_level = levels; t.size = t.format = 0;
    t.realWidth = 64; t.width = 61; t.realHeight = 32; t.height = 29;
    t.clampWidth = t.width; t.clampHeight = t.height; return t;
}
static void clean() { assert(live.empty()); failAllocAt = failLoadAt = 0; retryEviction = false; }
int main() {
    imageFormat[0][0][0].GXsize = 1;
    TextureCache_ProbeReset();
    CachedTexture t = base(); TextureCache_LoadMipChain(&t, 1);
    assert(t.max_level == 2 && t.textureBytes == 64*32 + 32*16 + 16*8);
    const u8 *packed = reinterpret_cast<const u8*>(t.GXtexture);
    unsigned off = 0, expectedLoad = 1;
    for (unsigned size : {2048u, 512u, 128u}) {
        for (unsigned i = 0; i < size; ++i) assert(packed[off+i] == (u8)(expectedLoad*13+i));
        off += size; ++expectedLoad;
    }
    assert(textureCacheProbeStats.mip_calls == 1 && textureCacheProbeStats.mip_levels == 3);
    assert(textureCacheProbeStats.mip_temp_bytes == t.textureBytes && textureCacheProbeStats.mip_pack_bytes == t.textureBytes);
    perfMem_free(0, nullptr, t.GXtexture); clean();

    // Each level can fail: earlier levels are freed, then ordinary load succeeds.
    for (unsigned failure = 1; failure <= 3; ++failure) {
        TextureCache_ProbeReset(); t = base(); loadCalls = 0; failLoadAt = failure;
        TextureCache_LoadMipChain(&t, 1);
        assert(t.max_level == 0 && t.GXtexture && textureCacheProbeStats.mip_levels == failure - 1);
        assert(textureCacheProbeStats.mip_timed_calls == 1 && live.size() == 1);
        perfMem_free(0, nullptr, t.GXtexture); clean();
    }

    // Final chain allocation retries after an eviction, then succeeds.
    TextureCache_ProbeReset(); t = base(); allocCalls = freeRetries = 0;
    failAllocAt = 4; retryEviction = true;
    TextureCache_LoadMipChain(&t, 1);
    assert(t.max_level == 2 && freeRetries == 1 && t.GXtexture && live.size() == 1);
    perfMem_free(0, nullptr, t.GXtexture); clean();

    // A terminal chain allocation failure frees every level and falls back.
    TextureCache_ProbeReset(); t = base(); allocCalls = freeRetries = 0;
    failAllocAt = 4; retryEviction = false; loadCalls = 0;
    TextureCache_LoadMipChain(&t, 1);
    assert(t.max_level == 0 && t.GXtexture && live.size() == 1);
    perfMem_free(0, nullptr, t.GXtexture); clean();

    // Cap-to-zero uses the normal loader; the real timer samples calls 1 and 128.
    TextureCache_ProbeReset(); t = base(); t.realWidth = t.realHeight = 4; loadCalls = 0;
    for (unsigned i = 0; i < 128; ++i) {
        TextureCache_LoadMipChain(&t, 1);
        perfMem_free(0, nullptr, t.GXtexture); t.GXtexture = nullptr;
    }
    TextureCacheProbeStats s = TextureCache_ProbeRead();
    assert(s.mip_calls == 128 && s.mip_levels == 0 && s.texture_loads == 128);
    assert(s.mip_timed_calls == 2 && s.mip_ticks == 2);
    clean();

    // An early return still closes the sampled timer; reset starts sampling over.
    TextureCache_ProbeReset(); t = base(); loadCalls = 0; failLoadAt = 2;
    TextureCache_LoadMipChain(&t, 1);
    s = TextureCache_ProbeRead();
    assert(s.mip_calls == 1 && s.mip_timed_calls == 1 && s.mip_ticks == 1);
    perfMem_free(0, nullptr, t.GXtexture); clean();
    TextureCache_ProbeReset(); s = TextureCache_ProbeRead();
    assert(s.mip_calls == 0 && s.mip_levels == 0 && s.mip_temp_bytes == 0 && s.mip_pack_bytes == 0);
    assert(s.mip_timed_calls == 0 && s.mip_ticks == 0 && s.texture_loads == 0 && s.texture_bytes == 0);
}
''')
        subprocess.run(["c++", "-std=c++11", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=address,undefined", "-DPERF_PROF", "-DPERF_CACHE_PROBES",
                        "-I", str(HEADER.parent), str(harness), "-o", str(tmp / "mip")], check=True)
        subprocess.run([str(tmp / "mip")], check=True)


if __name__ == "__main__":
    main()
