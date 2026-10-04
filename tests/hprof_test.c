#include <assert.h>
#include <stdio.h>
#include "../main/hprof_hist.h"

int main(void)
{
    uint32_t buckets[HPROF_BUCKETS] = {0};
    struct hprof_hist h = {.buckets = buckets, .jit_base = 0x80400000, .jit_size = 4096};
    assert(!hprof_layout(&h, 0x80004000, 0x80004000));
    assert(!hprof_layout(&h, 0x80004001, 0x80104000));
    assert(hprof_layout(&h, 0x80004000, 0x80194004));
    assert(h.shift == 6 && h.count <= HPROF_BUCKETS);
    hprof_record(&h, h.base);
    hprof_record(&h, h.end - 4);
    assert(buckets[0] == 1 && buckets[h.count - 1] == 1);
    hprof_record(&h, h.base - 4);
    hprof_record(&h, h.end);
    hprof_record(&h, h.jit_base);
    hprof_record(&h, h.jit_base + h.jit_size - 4);
    hprof_record(&h, h.jit_base + h.jit_size);
    hprof_record(&h, 0);
    assert(h.total == 8 && h.other == 4 && h.jit == 2);
    h.total = UINT32_MAX;
    hprof_record(&h, h.base);
    assert(h.overflow && h.total == UINT32_MAX && buckets[0] == 1);
    assert(hprof_layout(&h, 4, 0xfffffffc));
    assert(h.shift == 17 && h.count <= HPROF_BUCKETS);
    puts("hprof histogram: PASS");
}
