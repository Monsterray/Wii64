#ifndef WII64_HPROF_HIST_H
#define WII64_HPROF_HIST_H

#include <stdint.h>
#include <limits.h>

#define HPROF_BYTES (128u * 1024u)
#define HPROF_BUCKETS (HPROF_BYTES / sizeof(uint32_t))

struct hprof_hist {
    uint32_t *buckets;
    uint32_t base, end, shift, count, jit_base, jit_size;
    uint32_t total, jit, other, overflow;
};

static inline int hprof_layout(struct hprof_hist *h, uint32_t base, uint32_t end)
{
    if (end <= base || (base & 3) || (end & 3)) return 0;
    h->base = base;
    h->end = end;
    h->shift = 5;
    while ((((uint64_t)end - base + ((1u << h->shift) - 1)) >> h->shift) > HPROF_BUCKETS)
        ++h->shift;
    h->count = (uint32_t)(((uint64_t)end - base + ((1u << h->shift) - 1)) >> h->shift);
    return 1;
}

/* No allocation, clocks, locks or FP in the interrupt path. */
static inline void hprof_record(struct hprof_hist *h, uint32_t pc)
{
    if (h->total == UINT32_MAX) { h->overflow = 1; return; }
    ++h->total;
    if (pc - h->base < h->end - h->base)
        ++h->buckets[(pc - h->base) >> h->shift];
    else if (h->jit_size && pc - h->jit_base < h->jit_size)
        ++h->jit;
    else
        ++h->other;
}
#endif
