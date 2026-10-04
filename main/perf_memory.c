/* Opt-in private-heap accounting. No allocation, clocks, I/O or heap walks
   in allocation/free hooks, including GX draw-sync interrupt frees. */
#include <stdint.h>
#include <string.h>
#include "perf_memory.h"
#ifdef WII64_PERF_MEMORY
#include <ogc/irq.h>

int perfMem_enabled;
int perfMem_boxartTest;
static heap_cntrl *heaps[PERF_MEM_COUNT];
static struct perf_memory_stats stats[PERF_MEM_COUNT];

void perfMem_configure(int enabled) { perfMem_enabled = !!enabled; }

/* libogc2 lwp_heap.inl, __lwp_heap_usrblockat: user[-1] stores alignment
   padding. Read before free; use the actual block charge, not requested size.
   These checks diagnose accounting drift, not validate arbitrary guest pointers. */
static u32 charge(heap_cntrl *heap, const void *ptr)
{
    uintptr_t p = (uintptr_t)ptr, lo = (uintptr_t)heap->start;
    uintptr_t hi = (uintptr_t)heap->final;
    if (p < lo + HEAP_BLOCK_USED_OVERHEAD + sizeof(u32) || p >= hi) return 0;
    u32 offset = ((const u32 *)ptr)[-1];
    if (offset > p - lo - HEAP_BLOCK_USED_OVERHEAD) return 0;
    heap_block *block = (heap_block *)(p - offset - HEAP_BLOCK_USED_OVERHEAD);
    u32 bytes = block->front_flag & ~HEAP_BLOCK_USED;
    if (!(block->front_flag & HEAP_BLOCK_USED) || !bytes || bytes > hi - (uintptr_t)block)
        return 0;
    return bytes;
}

u32 perfMem_init(unsigned id, heap_cntrl *heap, void *base, u32 size, u32 page)
{
    u32 result = __lwp_heap_init(heap, base, size, page);
    if (perfMem_enabled && id < PERF_MEM_COUNT && result) {
        u32 level = IRQ_Disable();
        heaps[id] = heap;
        struct perf_memory_stats *s = &stats[id];
        s->base = (uintptr_t)heap->start;
        s->capacity = (uintptr_t)heap->final - s->base;
        s->live = s->blocks = s->retired = 0;
        if (!s->resets) s->min_free = s->capacity;
        s->resets++;
        IRQ_Restore(level);
    }
    return result;
}

void *perfMem_allocate(unsigned id, heap_cntrl *heap, u32 size)
{
    if (!perfMem_enabled) return __lwp_heap_allocate(heap, size);
    u32 level = IRQ_Disable(); /* also excludes the GX retired-texture free */
    void *ptr = __lwp_heap_allocate(heap, size);
    if (id < PERF_MEM_COUNT && heaps[id] == heap) {
        struct perf_memory_stats *s = &stats[id];
        s->allocs++;
        if (size > s->max_request) s->max_request = size;
        if (!ptr) { s->failures++; s->failed_request = size; }
        else {
            u32 bytes = charge(heap, ptr);
            if (!bytes || bytes > s->capacity - s->live) s->errors++;
            else {
                s->live += bytes;
                s->blocks++;
                if (s->live > s->peak) s->peak = s->live;
                if (s->blocks > s->peak_blocks) s->peak_blocks = s->blocks;
                if (s->capacity - s->live < s->min_free) s->min_free = s->capacity - s->live;
            }
        }
    }
    IRQ_Restore(level);
    return ptr;
}

BOOL perfMem_free(unsigned id, heap_cntrl *heap, void *ptr)
{
    if (!perfMem_enabled) return __lwp_heap_free(heap, ptr);
    u32 level = IRQ_Disable();
    u32 bytes = ptr ? charge(heap, ptr) : 0;
    BOOL result = __lwp_heap_free(heap, ptr);
    if (id < PERF_MEM_COUNT && heaps[id] == heap) {
        struct perf_memory_stats *s = &stats[id];
        if (!result || !bytes || bytes > s->live || !s->blocks) s->errors++;
        else { s->live -= bytes; s->blocks--; s->frees++; }
    }
    IRQ_Restore(level);
    return result;
}

void perfMem_retired(heap_cntrl *heap, void *ptr, int add)
{
    if (!perfMem_enabled || heaps[PERF_MEM_TEXTURE] != heap) return;
    u32 level = IRQ_Disable();
    struct perf_memory_stats *s = &stats[PERF_MEM_TEXTURE];
    u32 bytes = charge(heap, ptr);
    if (!bytes || (!add && bytes > s->retired)) s->errors++;
    else if (add) {
        s->retired += bytes;
        if (s->retired > s->peak_retired) s->peak_retired = s->retired;
    } else s->retired -= bytes;
    IRQ_Restore(level);
}

void perfMem_evict(u32 requested)
{
    if (!perfMem_enabled) return;
    stats[PERF_MEM_CODE].evictions++;
    stats[PERF_MEM_CODE].evicted_payload += requested;
}

/* Walk only while emulation is stopped. Exclude the final dummy block, bound
   every hop, and reconcile against allocation-boundary accounting. */
int perfMem_read(unsigned id, struct perf_memory_stats *out)
{
    if (!perfMem_enabled || id >= PERF_MEM_COUNT || !heaps[id]) return 0;
    u32 level = IRQ_Disable();
    heap_cntrl *heap = heaps[id];
    struct perf_memory_stats *s = &stats[id];
    uintptr_t p = (uintptr_t)heap->start, end = (uintptr_t)heap->final;
    u32 used = 0, blocks = 0;
    s->free_bytes = s->largest_free = s->free_blocks = 0;
    while (p < end) {
        heap_block *block = (heap_block *)p;
        u32 bytes = block->front_flag & ~HEAP_BLOCK_USED;
        if (bytes < HEAP_OVERHEAD || bytes > end - p || bytes % sizeof(u32)) {
            s->errors++;
            break;
        }
        heap_block *next = (heap_block *)(p + bytes);
        if (next->back_flag != block->front_flag) { s->errors++; break; }
        if (block->front_flag & HEAP_BLOCK_USED) { used += bytes; blocks++; }
        else {
            s->free_bytes += bytes;
            s->free_blocks++;
            if (bytes > s->largest_free) s->largest_free = bytes;
        }
        p += bytes;
    }
    if (p != end || used != s->live || blocks != s->blocks ||
        ((heap_block *)end)->front_flag != HEAP_DUMMY_FLAG) s->errors++;
    *out = *s;
    IRQ_Restore(level);
    return 1;
}
#endif
