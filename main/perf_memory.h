#ifndef WII64_PERF_MEMORY_H
#define WII64_PERF_MEMORY_H

#include <stdint.h>
#include <ogc/lwp_heap.h>

enum perf_memory_heap { PERF_MEM_CODE, PERF_MEM_META, PERF_MEM_NODE,
                        PERF_MEM_TEXTURE, PERF_MEM_BOXART, PERF_MEM_COUNT };

#if defined(PERF_PROF) && defined(PERF_MEMORY)
#define WII64_PERF_MEMORY
#ifdef __cplusplus
extern "C" {
#endif
struct perf_memory_stats {
    u32 capacity, live, peak, blocks, peak_blocks, min_free;
    u32 allocs, frees, failures, max_request, failed_request, resets, errors;
    u32 free_bytes, largest_free, free_blocks, retired, peak_retired;
    u32 evictions;
    u64 evicted_payload;
    uintptr_t base;
};
extern int perfMem_enabled;
extern int perfMem_boxartTest;
void perfMem_configure(int enabled);
u32 perfMem_init(unsigned id, heap_cntrl *heap, void *base, u32 size, u32 page);
void *perfMem_allocate(unsigned id, heap_cntrl *heap, u32 size);
BOOL perfMem_free(unsigned id, heap_cntrl *heap, void *ptr);
void perfMem_retired(heap_cntrl *heap, void *ptr, int add);
void perfMem_evict(u32 requested);
int perfMem_read(unsigned id, struct perf_memory_stats *out);
void perfMem_snapshot(const char *stage); /* stopped thread context only */
#ifdef __cplusplus
}
#endif
#else
#define perfMem_configure(enabled) ((void)0)
#define perfMem_init(id, heap, base, size, page) __lwp_heap_init(heap, base, size, page)
#define perfMem_allocate(id, heap, size) __lwp_heap_allocate(heap, size)
#define perfMem_free(id, heap, ptr) __lwp_heap_free(heap, ptr)
#define perfMem_retired(heap, ptr, add) ((void)0)
#define perfMem_evict(requested) ((void)0)
#define perfMem_snapshot(stage) ((void)0)
#endif
#endif
