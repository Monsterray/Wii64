#include <assert.h>
#include <stdio.h>
#include "../main/perf_subsystem.h"

static unsigned long long tick;
static unsigned int irq_masked, pending_callback;
static void deliver_callback(void)
{
    if (!irq_masked && pending_callback) {
        pending_callback = 0;
        unsigned long long callback = perfProf_subsystemBegin(PERF_SUB_AUDIO_CALLBACK);
        tick += 100;
        perfProf_subsystemEnd(PERF_SUB_AUDIO_CALLBACK, callback);
    }
}
unsigned int IRQ_Disable(void)
{
    unsigned int level = irq_masked;
    irq_masked = 1;
    return level;
}
void IRQ_Restore(unsigned int level) { irq_masked = level; deliver_callback(); }
unsigned long long gettime(void)
{
    unsigned long long now = tick += 100;
    deliver_callback(); /* Model an IRQ immediately after the clock read. */
    return now;
}

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

#if PERF_SUBSYSTEM_SELF
    /* Same-window exclusion, including a child skipped by its own sampler. */
    perfProf_subsystemReset();
    timer = 0;
    while (!timer) timer = perfProf_subsystemBegin(PERF_SUB_EXECUTE);
    child = perfProf_subsystemBegin(PERF_SUB_MEMORY_SLOW);
    assert(child != 0); /* Forced attribution must not become an inclusive sample. */
    unsigned long long nested = perfProf_subsystemBegin(PERF_SUB_GFX);
    perfProf_subsystemEnd(PERF_SUB_GFX, nested);
    perfProf_subsystemEnd(PERF_SUB_MEMORY_SLOW, child);
    perfProf_subsystemEnd(PERF_SUB_EXECUTE, timer);
    s = perfProf_subsystemRead(PERF_SUB_EXECUTE);
    assert(s.ticks == 500 && s.self_ticks == 200 && s.self_calls == 1);
    assert(perfProf_subsystemRead(PERF_SUB_MEMORY_SLOW).timed_calls == 0);

    perfProf_subsystemReset();
    timer = 0;
    while (!timer) timer = perfProf_subsystemBegin(PERF_SUB_EXECUTE);
    for (unsigned int i = 0; i < 2; i++) {
        child = perfProf_subsystemBegin(PERF_SUB_CPU_HELPER);
        perfProf_subsystemEnd(PERF_SUB_CPU_HELPER, child);
    }
    perfProf_subsystemEnd(PERF_SUB_EXECUTE, timer);
    assert(perfProf_subsystemRead(PERF_SUB_EXECUTE).self_ticks == 300);

    perfProf_subsystemReset();
    timer = 0;
    while (!timer) timer = perfProf_subsystemBegin(PERF_SUB_MEMORY_SLOW);
    child = perfProf_subsystemBegin(PERF_SUB_DMA_PI);
    nested = perfProf_subsystemBegin(PERF_SUB_GFX);
    perfProf_subsystemEnd(PERF_SUB_GFX, nested);
    perfProf_subsystemEnd(PERF_SUB_DMA_PI, child);
    perfProf_subsystemEnd(PERF_SUB_MEMORY_SLOW, timer);
    s = perfProf_subsystemRead(PERF_SUB_MEMORY_SLOW);
    assert(s.ticks == 500 && s.self_ticks == 200); /* Union, not DMA + RSP. */

    timer = perfProf_subsystemBegin(PERF_SUB_INTERRUPT);
    child = perfProf_subsystemBegin(PERF_SUB_LIMITER);
    perfProf_subsystemReset();
    perfProf_subsystemEnd(PERF_SUB_LIMITER, child);
    perfProf_subsystemEnd(PERF_SUB_INTERRUPT, timer);
    s = perfProf_subsystemRead(PERF_SUB_INTERRUPT);
    assert(!s.self_calls && !s.self_ticks && !s.self_dropped);

    /* A reentrant root must be rejected, not silently counted twice. */
    timer = perfProf_subsystemBegin(PERF_SUB_INTERRUPT);
    nested = perfProf_subsystemBegin(PERF_SUB_LIMITER);
    child = perfProf_subsystemBegin(PERF_SUB_INTERRUPT);
    perfProf_subsystemEnd(PERF_SUB_INTERRUPT, child);
    perfProf_subsystemEnd(PERF_SUB_LIMITER, nested);
    perfProf_subsystemEnd(PERF_SUB_INTERRUPT, timer);
    s = perfProf_subsystemRead(PERF_SUB_INTERRUPT);
    assert(s.self_dropped == 1 && s.self_calls == 0);
    timer = perfProf_subsystemBegin(PERF_SUB_INTERRUPT);
    perfProf_subsystemEnd(PERF_SUB_INTERRUPT, timer);
    assert(perfProf_subsystemRead(PERF_SUB_INTERRUPT).self_calls == 1);

    perfProf_subsystemReset();
    for (unsigned int i = 1; i < period; i++) {
        assert(!perfProf_subsystemBegin(PERF_SUB_MEMORY_SLOW));
        assert(!perfProf_subsystemBegin(PERF_SUB_EXECUTE));
    }
    timer = perfProf_subsystemBegin(PERF_SUB_EXECUTE);
    child = perfProf_subsystemBegin(PERF_SUB_MEMORY_SLOW);
    nested = perfProf_subsystemBegin(PERF_SUB_GFX);
    perfProf_subsystemEnd(PERF_SUB_GFX, nested);
    perfProf_subsystemEnd(PERF_SUB_MEMORY_SLOW, child);
    perfProf_subsystemEnd(PERF_SUB_EXECUTE, timer);
    assert(perfProf_subsystemRead(PERF_SUB_MEMORY_SLOW).self_ticks == 200);
    assert(perfProf_subsystemRead(PERF_SUB_EXECUTE).self_ticks == 200);

    /* Snapshot in an attribution window must not disturb it. */
    perfProf_subsystemReset();
    timer = perfProf_subsystemBegin(PERF_SUB_INTERPRETER);
    s = perfProf_subsystemRead(PERF_SUB_INTERPRETER);
    child = perfProf_subsystemBegin(PERF_SUB_INTERRUPT);
    nested = perfProf_subsystemBegin(PERF_SUB_LIMITER);
    perfProf_subsystemEnd(PERF_SUB_LIMITER, nested);
    perfProf_subsystemEnd(PERF_SUB_INTERRUPT, child);
    perfProf_subsystemEnd(PERF_SUB_INTERPRETER, timer);
    assert(perfProf_subsystemRead(PERF_SUB_INTERPRETER).self_ticks == 200);
    assert(perfProf_subsystemRead(PERF_SUB_INTERRUPT).self_ticks == 200);

    /* An IRQ after the end clock read must not exclude time past that end. */
    perfProf_subsystemReset();
    timer = 0;
    while (!timer) timer = perfProf_subsystemBegin(PERF_SUB_EXECUTE);
    pending_callback = 1;
    perfProf_subsystemEnd(PERF_SUB_EXECUTE, timer);
    s = perfProf_subsystemRead(PERF_SUB_EXECUTE);
    assert(!pending_callback && !irq_masked);
    assert(s.self_dropped == 0 && s.self_calls == 1 && s.self_ticks == 100);

    /* Never enable IRQs when the caller already has them masked. */
    unsigned int level = IRQ_Disable();
    timer = perfProf_subsystemBegin(PERF_SUB_INTERRUPT);
    perfProf_subsystemEnd(PERF_SUB_INTERRUPT, timer);
    assert(irq_masked);
    IRQ_Restore(level);
    assert(!irq_masked);
#endif
#else
    unsigned long long timer = perfProf_subsystemBegin(PERF_SUB_GFX);
    perfProf_subsystemEnd(PERF_SUB_GFX, timer);
    assert(timer == 0 && tick == 0); /* Reference/release probes compile away. */
#endif
    puts("subsystem timing, sparse clocks, nesting and ROM reset: ok");
}
