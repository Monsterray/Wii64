#ifndef GLN64_GX_TEXTURE_PROBE_H
#define GLN64_GX_TEXTURE_PROBE_H

#if defined(PERF_PROF) && defined(PERF_CACHE_PROBES)
#include <stdint.h>

typedef struct TextureCacheProbeStats
{
	uint64_t mip_calls;
	uint64_t mip_levels;
	uint64_t mip_temp_bytes;
	uint64_t mip_pack_bytes;
	uint64_t mip_timed_calls;
	uint64_t mip_ticks;
	uint64_t texture_loads;
	uint64_t texture_bytes;
} TextureCacheProbeStats;

#ifdef __cplusplus
extern "C" {
#endif
void TextureCache_ProbeReset(void);
TextureCacheProbeStats TextureCache_ProbeRead(void);
#ifdef __cplusplus
}
#endif
#endif

#endif
