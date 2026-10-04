/* Synthetic PPC block geometry checks accounting, not hardware allocation.
   The diagnostic boxart probe separately uses the actual native allocator. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../main/perf_memory.h"

static unsigned irq;
unsigned IRQ_Disable(void) { unsigned old = irq; irq = 1; return old; }
void IRQ_Restore(unsigned old) { irq = old; }

u32 __lwp_heap_init(heap_cntrl *h, void *base, u32 size, u32 page)
{
    h->start = base; h->final = (heap_block *)((char *)base + size - 8);
    h->pg_size = page;
    h->start->back_flag = HEAP_DUMMY_FLAG;
    h->start->front_flag = size - 8;
    h->final->back_flag = size - 8;
    h->final->front_flag = HEAP_DUMMY_FLAG;
    return size - 16;
}
void *__lwp_heap_allocate(heap_cntrl *h, u32 size)
{
    if (size > UINT32_MAX - 72) return NULL;
    u32 bytes = ((size + 31) & ~31u) + 40;
    for (heap_block *b = h->start; b < h->final;) {
        u32 span = b->front_flag & ~1u;
        if (!(b->front_flag & 1) && span >= bytes) {
            if (span - bytes < 40) bytes = span;
            else {
                heap_block *next = (heap_block *)((char *)b + bytes);
                next->back_flag = bytes | 1;
                next->front_flag = span - bytes;
                ((heap_block *)((char *)b + span))->back_flag = span - bytes;
            }
            b->front_flag = bytes | 1;
            ((heap_block *)((char *)b + bytes))->back_flag = b->front_flag;
            uintptr_t user = ((uintptr_t)b + 8 + 32) & ~(uintptr_t)31;
            ((u32 *)user)[-1] = user - (uintptr_t)b - 8;
            return (void *)user;
        }
        b = (heap_block *)((char *)b + span);
    }
    return NULL;
}
BOOL __lwp_heap_free(heap_cntrl *h, void *ptr)
{
    heap_block *b = (heap_block *)((char *)ptr - ((u32 *)ptr)[-1] - 8);
    if (!(b->front_flag & 1)) return 0;
    b->front_flag &= ~1u;
    ((heap_block *)((char *)b + b->front_flag))->back_flag = b->front_flag;
    for (b = h->start; b < h->final;) {
        heap_block *next = (heap_block *)((char *)b + (b->front_flag & ~1u));
        if (next < h->final && !(b->front_flag & 1) && !(next->front_flag & 1)) {
            b->front_flag += next->front_flag;
            ((heap_block *)((char *)b + b->front_flag))->back_flag = b->front_flag;
        } else b = next;
    }
    return 1;
}

int main(void)
{
    _Alignas(32) unsigned char memory[4096];
    heap_cntrl heap;
    struct perf_memory_stats s;
    perfMem_configure(0);
    perfMem_init(PERF_MEM_TEXTURE, &heap, memory, sizeof(memory), 32);
    void *a = perfMem_allocate(PERF_MEM_TEXTURE, &heap, 30);
    assert(a && perfMem_free(PERF_MEM_TEXTURE, &heap, a));
    assert(!perfMem_read(PERF_MEM_TEXTURE, &s));
    perfMem_configure(1);
    perfMem_init(PERF_MEM_TEXTURE, &heap, memory, sizeof(memory), 32);
    assert(perfMem_read(PERF_MEM_TEXTURE, &s) && !s.live && !s.errors);
    assert(s.free_bytes == 4088 && s.largest_free == 4088);
    a = perfMem_allocate(PERF_MEM_TEXTURE, &heap, 30);
    void *b = perfMem_allocate(PERF_MEM_TEXTURE, &heap, 100);
    assert(a && b && perfMem_read(PERF_MEM_TEXTURE, &s));
    assert(s.live == 240 && s.peak == 240 && s.blocks == 2 && s.min_free == 3848);
    assert(!perfMem_allocate(PERF_MEM_TEXTURE, &heap, 9999));
    assert(!perfMem_allocate(PERF_MEM_TEXTURE, &heap, UINT32_MAX));
    perfMem_retired(&heap, b, 1);
    assert(perfMem_read(PERF_MEM_TEXTURE, &s) && s.retired == 168);
    perfMem_retired(&heap, b, 0);
    assert(perfMem_free(PERF_MEM_TEXTURE, &heap, b));
    assert(perfMem_free(PERF_MEM_TEXTURE, &heap, a));
    assert(perfMem_read(PERF_MEM_TEXTURE, &s) && !s.live && !s.errors);
    assert(s.failures == 2 && s.failed_request == UINT32_MAX && s.peak_retired == 168);
    assert(s.free_bytes == s.capacity && s.largest_free == s.capacity);
    perfMem_init(PERF_MEM_TEXTURE, &heap, memory, sizeof(memory), 32);
    assert(perfMem_read(PERF_MEM_TEXTURE, &s) && s.resets == 2 && s.peak == 240);
    heap.start->front_flag = 0; /* malformed walker must stop, not loop/read beyond */
    assert(perfMem_read(PERF_MEM_TEXTURE, &s) && s.errors);
    assert(!perfMem_read(PERF_MEM_COUNT, &s));
    assert(!irq);
    puts("Memory census: charged peaks, failures, retirement, reset and bounded walk PASS");
}
