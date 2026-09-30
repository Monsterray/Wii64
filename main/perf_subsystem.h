/* Opt-in inclusive operation timings; no SDK types in RSP HLE callers. */
#ifndef PERF_SUBSYSTEM_H
#define PERF_SUBSYSTEM_H

enum { PERF_SUB_GFX, PERF_SUB_AUDIO, PERF_SUB_RSP_OTHER, PERF_SUB_LOOKUP,
       PERF_SUB_COMPILE, PERF_SUB_DISPATCH, PERF_SUB_EXECUTE, PERF_SUB_ROM_COPY,
       PERF_SUB_PRESENT, PERF_SUB_LIMITER, PERF_SUB_COUNT };

struct perf_subsystem_stats {
    unsigned int calls, timed_calls;
    unsigned long long ticks;
};

#if defined(PERF_PROF) && defined(PERF_SUBSYSTEM_PROBES)
#define PERF_SUBSYSTEM_ENABLED
#ifdef __cplusplus
extern "C" {
#endif
void perfProf_subsystemReset(void);
unsigned long long perfProf_subsystemBegin(unsigned int stage);
void perfProf_subsystemEnd(unsigned int stage, unsigned long long start);
struct perf_subsystem_stats perfProf_subsystemRead(unsigned int stage);
unsigned int perfProf_subsystemPeriod(unsigned int stage);
#ifdef __cplusplus
}
#endif
#else
#define perfProf_subsystemReset() ((void)0)
#define perfProf_subsystemBegin(stage) (0ULL)
#define perfProf_subsystemEnd(stage, start) ((void)(start))
#endif
#endif
