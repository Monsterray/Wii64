#!/usr/bin/env python3
"""Exercise the actual opt-in probe with a noncoherent host cache model.

This tests probe accounting/ownership, not emulated Wii cache performance.
The real cache hypothesis must still be checked on hardware.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    source = (ROOT / "glN64_GX/VI.cpp").read_text()
    start = source.index("int cacheProbeGXTest;")
    end = source.index("\n#endif", start)
    probe = source[start:end]
    harness = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "CacheProbe.h"
using u32 = uint32_t;
using u64 = uint64_t;
static u32 *cpu;
static u32 ram[16];
static bool pending;
static bool corruptGuard;
static unsigned completed, frees;
static char record[128];
static unsigned long long tick;
static unsigned long long gettime() { return ++tick; }
static void *memalign(unsigned alignment, unsigned bytes) {
    assert(alignment == 32 && bytes == 64);
    void *p = nullptr;
    assert(posix_memalign(&p, alignment, bytes) == 0);
    cpu = static_cast<u32*>(p);
    return p;
}
static void free_owned(void *p) {
    assert(!pending && completed == 2);
    ++frees; std::free(p);
}
static void DCFlushRange(void *p, unsigned bytes) {
    assert(p == cpu); memcpy(ram, cpu, bytes);
}
static void DCInvalidateRange(void *p, unsigned bytes) {
    assert(!pending && p == cpu && (bytes == 32 || bytes == 64));
    memcpy(cpu, ram, bytes);
}
static void GX_SetTexCopySrc(int x, int y, int w, int h) {
    assert(x == 0 && y == 0 && w == 4 && h == 4);
}
enum { GX_TF_RGB565, GX_FALSE };
static void GX_SetTexCopyDst(int w, int h, int fmt, int mip) {
    assert(w == 4 && h == 4 && fmt == GX_TF_RGB565 && mip == GX_FALSE);
}
static void GX_CopyTex(void *p, int clear) {
    assert(p == cpu && clear == GX_FALSE && !pending); pending = true;
}
static void GX_PixModeSync() {}
static void GX_DrawDone() {
    if (!pending) return;
    for (unsigned i = 0; i < 8; ++i) ram[i] = 0x12345678u + i;
    if (corruptGuard) ram[8] = 0;
    pending = false; ++completed;
}
static void perfProf_mark(const char *text) { snprintf(record, sizeof(record), "%s", text); }
#define MEM_K0_TO_K1(p) (ram)
#define free free_owned
'''
    harness += probe
    harness += r'''
#undef free
int main() {
    cacheProbeGXTest = 0;
    VI_CacheProbeGXTest();
    assert(frees == 0);
    cacheProbeGXTest = 1;
    VI_CacheProbeGXTest();
    assert(frees == 1);
    assert(!strcmp(record, "cache_gx_test: stale_words=8 fresh_errors=0 guard_errors=0"));
    // A device overrun must be read from RAM, not hidden by the cached guard.
    completed = 0; corruptGuard = true;
    VI_CacheProbeGXTest();
    assert(frees == 2);
    assert(!strcmp(record, "cache_gx_test: stale_words=8 fresh_errors=0 guard_errors=1"));
    corruptGuard = false;
    ram[8] = 0xC3C3C3C3u;

    // A cached/physical mismatch is visible; the witness does NOT repair it.
    alignas(32) u32 data[16] = {};
    VI_CacheProbeReset();
    VI_CacheProbeReadback(data, 64);
    auto s = VI_CacheProbeRead();
    assert(s.calls == 1 && s.checks == 1 && s.stale_checks == 1);
    assert(s.stale_words == 8 && s.wait_changed_checks == 0 && s.ticks == 1);
    for (unsigned i = 0; i < 126; ++i) VI_CacheProbeReadback(data, 64);
    assert(VI_CacheProbeRead().checks == 1);
    VI_CacheProbeReadback(data, 64);
    assert(VI_CacheProbeRead().checks == 2);
    VI_CacheProbeReset();
    VI_CacheProbeReadback(nullptr, 32);
    assert(VI_CacheProbeRead().invalid_ranges == 1);
    VI_CacheProbeReset();
    VI_CacheProbeReadback(data + 1, 32);
    assert(VI_CacheProbeRead().invalid_ranges == 1);
    VI_CacheProbeReset();
    VI_CacheProbeReadback(data, 0);
    assert(VI_CacheProbeRead().invalid_ranges == 1);
    VI_CacheProbeReset();
    VI_CacheProbeReadback(data, 640u * 576u * 2u + 32);
    assert(VI_CacheProbeRead().invalid_ranges == 1);
    VI_CacheProbeReset();
    cacheProbeGXTest = 0;
    VI_CacheProbeReadback(data, 64);
    assert(VI_CacheProbeRead().calls == 0);
    VI_CacheProbeFramebuffer(2048);
    VI_CacheProbeFramebuffer(4096);
    assert(VI_CacheProbeRead().framebuffer_calls == 2);
    assert(VI_CacheProbeRead().framebuffer_bytes == 6144);
    VI_CacheProbeReset();
    assert(VI_CacheProbeRead().framebuffer_calls == 0);
}
'''
    with tempfile.TemporaryDirectory() as directory:
        tmp = Path(directory)
        cpp = tmp / "probe.cpp"
        cpp.write_text(harness)
        subprocess.run(["c++", "-std=c++11", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=address,undefined", "-DPERF_PROF",
                        "-DPERF_CACHE_PROBES", "-DHW_RVL", "-I",
                        str(ROOT / "glN64_GX"), str(cpp), "-o", str(tmp / "probe")], check=True)
        subprocess.run([str(tmp / "probe")], check=True)
        # Neither normal release nor GameCube exposes the Wii-only probe API.
        for flags in ([], ["-DPERF_PROF", "-DPERF_CACHE_PROBES"]):
            cpp.write_text('#include "CacheProbe.h"\nint main() { return 0; }\n')
            subprocess.run(["c++", "-Werror", *flags, "-I", str(ROOT / "glN64_GX"),
                            str(cpp), "-o", str(tmp / "off")], check=True)
    print("cache probe: sanitizer model PASS (native cache validation still required)")


if __name__ == "__main__":
    main()
