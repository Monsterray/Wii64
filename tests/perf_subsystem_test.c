#include <assert.h>
#include <stdio.h>
#include "../main/perf_subsystem.h"

static unsigned long long tick;
unsigned long long gettime(void) { return tick += 100; }

static void early_return(void)
{
    PERF_SUBSYSTEM_C_SCOPE(PERF_SUB_DMA_PI);
    return;
}

int main(void)
{
    perfProf_subsystemReset();
    early_return();
#ifdef PERF_SUBSYSTEM_ENABLED
    assert(perfProf_subsystemRead(PERF_SUB_DMA_PI).ticks == 100);
    assert(perfProf_subsystemPeriod(PERF_SUB_GFX) == 1);
    unsigned int period = perfProf_subsystemPeriod(PERF_SUB_LOOKUP);
    unsigned long long before = tick;
    for (unsigned int i = 1; i < period; i++) {
        assert(perfProf_subsystemBegin(PERF_SUB_LOOKUP) == 0);
        perfProf_subsystemEnd(PERF_SUB_LOOKUP, 0);
    }
    assert(tick == before); /* No clock reads on skipped hot operations. */
    unsigned long long timer = perfProf_subsystemBegin(PERF_SUB_LOOKUP);
    assert(timer != 0);
    perfProf_subsystemEnd(PERF_SUB_LOOKUP, timer);
    struct perf_subsystem_stats s = perfProf_subsystemRead(PERF_SUB_LOOKUP);
    assert(s.calls == period && s.timed_calls == 1 && s.ticks == 100);
    timer = perfProf_subsystemBegin(PERF_SUB_GFX);
    unsigned long long child = perfProf_subsystemBegin(PERF_SUB_PRESENT);
    perfProf_subsystemEnd(PERF_SUB_PRESENT, child);
    perfProf_subsystemEnd(PERF_SUB_GFX, timer);
    assert(perfProf_subsystemRead(PERF_SUB_PRESENT).ticks == 100);
    assert(perfProf_subsystemRead(PERF_SUB_GFX).ticks == 300); /* Inclusive, not additive. */
    timer = perfProf_subsystemBegin(PERF_SUB_GFX);
    perfProf_subsystemReset();
    perfProf_subsystemEnd(PERF_SUB_GFX, timer);
    s = perfProf_subsystemRead(PERF_SUB_GFX);
    assert(s.calls == 0 && s.timed_calls == 0 && s.ticks == 0);
    assert(perfProf_subsystemBegin(PERF_SUB_COUNT) == 0);
    perfProf_subsystemEnd(PERF_SUB_COUNT, tick);
    assert(perfProf_subsystemRead(PERF_SUB_COUNT).calls == 0);
#else
    unsigned long long timer = perfProf_subsystemBegin(PERF_SUB_GFX);
    perfProf_subsystemEnd(PERF_SUB_GFX, timer);
    assert(timer == 0 && tick == 0); /* Reference/release probes compile away. */
#endif
    puts("subsystem timing, sparse clocks, nesting and ROM reset: ok");
}
