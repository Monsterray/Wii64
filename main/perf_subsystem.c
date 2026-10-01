#include "perf_subsystem.h"
#ifdef PERF_SUBSYSTEM_ENABLED
#include <string.h>
#include <ogc/lwp_watchdog.h>
#include <ogc/irq.h>

#ifndef PERF_SUBSYSTEM_INTERVAL
#define PERF_SUBSYSTEM_INTERVAL 127
#endif
#if PERF_SUBSYSTEM_INTERVAL < 1
#error PERF_SUBSYSTEM_INTERVAL must be positive
#endif

static struct perf_subsystem_stats stats[PERF_SUB_COUNT];
static unsigned int countdown[PERF_SUB_COUNT];
static unsigned long long epoch;

unsigned int perfProf_subsystemPeriod(unsigned int stage)
{
    return stage == PERF_SUB_LOOKUP || stage == PERF_SUB_DISPATCH ||
           stage == PERF_SUB_EXECUTE || stage == PERF_SUB_GFX_COMMAND ||
           stage == PERF_SUB_VERTEX || stage == PERF_SUB_GFX_STATE ||
           stage == PERF_SUB_TLB ||
           stage == PERF_SUB_MEMORY_SLOW || stage == PERF_SUB_INVALIDATE ? PERF_SUBSYSTEM_INTERVAL : 1;
}

void perfProf_subsystemReset(void)
{
    unsigned int level = IRQ_Disable();
    memset(stats, 0, sizeof(stats));
    for (unsigned int i = 0; i < PERF_SUB_COUNT; i++)
        countdown[i] = perfProf_subsystemPeriod(i);
    epoch = gettime();
    IRQ_Restore(level);
}

unsigned long long perfProf_subsystemBegin(unsigned int stage)
{
    if (stage >= PERF_SUB_COUNT) return 0;
    stats[stage].calls++;
    if (--countdown[stage]) return 0;
    countdown[stage] = perfProf_subsystemPeriod(stage);
    return gettime();
}

void perfProf_subsystemEnd(unsigned int stage, unsigned long long start)
{
    /* First-VI reset can occur inside a timed dynarec execution. */
    if (!start || start < epoch || stage >= PERF_SUB_COUNT) return;
    stats[stage].ticks += gettime() - start;
    stats[stage].timed_calls++;
}

struct perf_subsystem_stats perfProf_subsystemRead(unsigned int stage)
{
    /* Main-thread stages have one writer; the audio callback has its own ID.
       Only snapshots/reset mask IRQs, never the hot begin/end operations. */
    unsigned int level = IRQ_Disable();
    struct perf_subsystem_stats result = stage < PERF_SUB_COUNT ? stats[stage] : (struct perf_subsystem_stats){0};
    IRQ_Restore(level);
    return result;
}
#endif
