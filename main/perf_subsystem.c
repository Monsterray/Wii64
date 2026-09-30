#include "perf_subsystem.h"
#ifdef PERF_SUBSYSTEM_ENABLED
#include <string.h>
#include <ogc/lwp_watchdog.h>

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
           stage == PERF_SUB_EXECUTE ? PERF_SUBSYSTEM_INTERVAL : 1;
}

void perfProf_subsystemReset(void)
{
    memset(stats, 0, sizeof(stats));
    for (unsigned int i = 0; i < PERF_SUB_COUNT; i++)
        countdown[i] = perfProf_subsystemPeriod(i);
    epoch = gettime();
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
    return stage < PERF_SUB_COUNT ? stats[stage] : (struct perf_subsystem_stats){0};
}
#endif
