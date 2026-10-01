/* Opt-in inclusive operation timings; no SDK types in RSP HLE callers. */
#ifndef PERF_SUBSYSTEM_H
#define PERF_SUBSYSTEM_H

enum { PERF_SUB_GFX, PERF_SUB_AUDIO, PERF_SUB_RSP_OTHER, PERF_SUB_LOOKUP,
       PERF_SUB_COMPILE, PERF_SUB_DISPATCH, PERF_SUB_EXECUTE, PERF_SUB_ROM_COPY,
       PERF_SUB_PRESENT, PERF_SUB_LIMITER, PERF_SUB_VM_FAULT, PERF_SUB_VM_VICTIM,
       PERF_SUB_VM_READ, PERF_SUB_VM_WRITE, PERF_SUB_TEX_HASH, PERF_SUB_TEX_LOOKUP,
       PERF_SUB_TEX_LOAD, PERF_SUB_TEX_ACTIVATE, PERF_SUB_DRAW_TRIANGLES,
       PERF_SUB_DRAW_RECT, PERF_SUB_COUNT };

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
#ifdef __cplusplus
#ifdef PERF_SUBSYSTEM_ENABLED
/* Scoped C++ probes also finish on allocation failures and early returns. */
class PerfSubsystemScope {
    unsigned int stage;
    unsigned long long start;
public:
    explicit PerfSubsystemScope(unsigned int s) : stage(s), start(perfProf_subsystemBegin(s)) {}
    ~PerfSubsystemScope() { perfProf_subsystemEnd(stage, start); }
    PerfSubsystemScope(const PerfSubsystemScope&) = delete;
    PerfSubsystemScope& operator=(const PerfSubsystemScope&) = delete;
};
#define PERF_SUBSYSTEM_SCOPE(stage) PerfSubsystemScope perf_scope(stage)
#else
#define PERF_SUBSYSTEM_SCOPE(stage) ((void)0)
#endif
#endif
#endif
