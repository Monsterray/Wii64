/* Opt-in cached/device ownership checks. Never enabled in release builds. */
#ifndef GLN64_CACHE_PROBE_H
#define GLN64_CACHE_PROBE_H
#include <stdint.h>
#if defined(PERF_PROF) && defined(PERF_CACHE_PROBES) && defined(HW_RVL)
#ifdef __cplusplus
extern "C" {
#endif
extern int cacheProbeGXTest;
struct xfb_cache_probe_stats {
    uint32_t calls, checks, stale_checks, stale_words, wait_changed_checks;
    uint32_t invalid_ranges;
    uint32_t framebuffer_calls;
    uint64_t framebuffer_bytes;
    uint64_t ticks;
};
void VI_CacheProbeReset(void);
struct xfb_cache_probe_stats VI_CacheProbeRead(void);
void VI_CacheProbeGXTest(void);
void VI_CacheProbeReadback(const void *xfb, uint32_t bytes);
void VI_CacheProbeFramebuffer(uint32_t bytes);
#ifdef __cplusplus
}
#endif
#endif
#endif
