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

#if PERF_SUBSYSTEM_SELF
/* Only six roots need self attribution. No stack or clocks on skipped calls
   unless a sampled parent must exclude them. Tokens are opaque to callers. */
#define FORCED_SAMPLE (1ULL << 63)
enum { SELF_EXECUTE, SELF_MEMORY, SELF_DISPATCH, SELF_INTERRUPT, SELF_STATE, SELF_INTERPRETER, SELF_COUNT };
static const unsigned int self_stage[SELF_COUNT] = {
    PERF_SUB_EXECUTE, PERF_SUB_MEMORY_SLOW, PERF_SUB_DISPATCH,
    PERF_SUB_INTERRUPT, PERF_SUB_GFX_STATE, PERF_SUB_INTERPRETER
};
static struct {
    unsigned long long start, excluded_start, excluded_ticks;
    unsigned int depth;
} self[SELF_COUNT];
static unsigned int active;

static unsigned int exclusions(unsigned int stage)
{
    unsigned int mask = 0;
    if (stage == PERF_SUB_MEMORY_SLOW || stage == PERF_SUB_CPU_HELPER ||
        stage == PERF_SUB_INTERRUPT || stage == PERF_SUB_AUDIO_CALLBACK || stage == PERF_SUB_VM_FAULT)
        mask |= 1u << SELF_EXECUTE;
    if (stage == PERF_SUB_GFX || stage == PERF_SUB_AUDIO || stage == PERF_SUB_RSP_OTHER ||
        stage == PERF_SUB_DMA_PI || stage == PERF_SUB_DMA_SP || stage == PERF_SUB_DMA_SI ||
        stage == PERF_SUB_ROM_COPY || stage == PERF_SUB_VM_FAULT || stage == PERF_SUB_PIF ||
        stage == PERF_SUB_INTERRUPT || stage == PERF_SUB_AUDIO_SUBMIT ||
        stage == PERF_SUB_AUDIO_CALLBACK || stage == PERF_SUB_INVALIDATE || stage == PERF_SUB_TLB ||
        stage == PERF_SUB_CPU_HELPER)
        mask |= 1u << SELF_MEMORY;
    if (stage == PERF_SUB_LOOKUP || stage == PERF_SUB_COMPILE || stage == PERF_SUB_INVALIDATE ||
        stage == PERF_SUB_TLB || stage == PERF_SUB_VM_FAULT || stage == PERF_SUB_AUDIO_CALLBACK)
        mask |= 1u << SELF_DISPATCH;
    if (stage == PERF_SUB_LIMITER || stage == PERF_SUB_PRESENT || stage == PERF_SUB_GFX ||
        stage == PERF_SUB_AUDIO || stage == PERF_SUB_RSP_OTHER || stage == PERF_SUB_AUDIO_CALLBACK ||
        stage == PERF_SUB_STATE_LOAD || stage == PERF_SUB_STATE_SAVE ||
        stage == PERF_SUB_PROBE_IO || stage == PERF_SUB_AGENT_POLL)
        mask |= 1u << SELF_INTERRUPT;
    if (stage == PERF_SUB_TEX_HASH || stage == PERF_SUB_TEX_LOOKUP || stage == PERF_SUB_TEX_LOAD ||
        stage == PERF_SUB_TEX_ACTIVATE || stage == PERF_SUB_GX_WAIT || stage == PERF_SUB_AUDIO_CALLBACK)
        mask |= 1u << SELF_STATE;
    if (stage == PERF_SUB_GFX || stage == PERF_SUB_AUDIO || stage == PERF_SUB_RSP_OTHER ||
        stage == PERF_SUB_DMA_PI || stage == PERF_SUB_DMA_SP || stage == PERF_SUB_DMA_SI ||
        stage == PERF_SUB_INTERRUPT || stage == PERF_SUB_VM_FAULT || stage == PERF_SUB_AUDIO_SUBMIT ||
        stage == PERF_SUB_AUDIO_CALLBACK || stage == PERF_SUB_PIF)
        mask |= 1u << SELF_INTERPRETER;
    return mask & active;
}
#endif

unsigned int perfProf_subsystemHasSelf(unsigned int stage)
{
#if PERF_SUBSYSTEM_SELF
    for (unsigned int i = 0; i < SELF_COUNT; i++)
        if (self_stage[i] == stage) return 1;
#else
    (void)stage;
#endif
    return 0;
}

unsigned int perfProf_subsystemPeriod(unsigned int stage)
{
    return stage == PERF_SUB_LOOKUP || stage == PERF_SUB_DISPATCH ||
           stage == PERF_SUB_EXECUTE || stage == PERF_SUB_GFX_COMMAND ||
           stage == PERF_SUB_VERTEX || stage == PERF_SUB_GFX_STATE ||
           stage == PERF_SUB_TLB ||
           stage == PERF_SUB_MEMORY_SLOW || stage == PERF_SUB_CPU_HELPER ||
           stage == PERF_SUB_INVALIDATE ? PERF_SUBSYSTEM_INTERVAL : 1;
}

void perfProf_subsystemReset(void)
{
    unsigned int level = IRQ_Disable();
    memset(stats, 0, sizeof(stats));
#if PERF_SUBSYSTEM_SELF
    memset(self, 0, sizeof(self));
    active = 0;
#endif
    for (unsigned int i = 0; i < PERF_SUB_COUNT; i++)
        countdown[i] = perfProf_subsystemPeriod(i);
    epoch = gettime();
    IRQ_Restore(level);
}

unsigned long long perfProf_subsystemBegin(unsigned int stage)
{
    if (stage >= PERF_SUB_COUNT) return 0;
    stats[stage].calls++;
    unsigned int sampled = --countdown[stage] == 0;
    if (sampled) countdown[stage] = perfProf_subsystemPeriod(stage);
#if PERF_SUBSYSTEM_SELF
    if (!sampled && !active) return 0;
    /* Keep the clock and attribution update atomic against audio callbacks.
       IRQs remain enabled across the measured operation itself. */
    unsigned int level = IRQ_Disable();
    unsigned int mask = exclusions(stage);
    if (!sampled && !mask) {
        IRQ_Restore(level);
        return 0;
    }
    unsigned long long now = gettime();
    for (unsigned int i = 0; i < SELF_COUNT; i++) {
        if (mask & (1u << i)) {
            if (!self[i].depth++) self[i].excluded_start = now;
        }
        if (sampled && self_stage[i] == stage) {
            if (active & (1u << i)) {
                /* Reentrant roots cannot be attributed with one slot. */
                stats[stage].self_dropped++;
                active &= ~(1u << i);
                self[i].start = 0;
            } else {
                self[i].start = now;
                self[i].depth = 0;
                self[i].excluded_ticks = 0;
                active |= 1u << i;
            }
        }
    }
    IRQ_Restore(level);
    return now | (sampled ? 0 : FORCED_SAMPLE);
#else
    return sampled ? gettime() : 0;
#endif
}

void perfProf_subsystemEnd(unsigned int stage, unsigned long long start)
{
    /* First-VI reset can occur inside a timed dynarec execution. */
    if (!start || stage >= PERF_SUB_COUNT) return;
#if PERF_SUBSYSTEM_SELF
    unsigned int sampled = !(start & FORCED_SAMPLE);
    start &= ~FORCED_SAMPLE;
#endif
    if (start < epoch) return;
#if PERF_SUBSYSTEM_SELF
    unsigned int level = IRQ_Disable();
#endif
    unsigned long long now = gettime();
#if PERF_SUBSYSTEM_SELF
    unsigned int mask = exclusions(stage);
    for (unsigned int i = 0; i < SELF_COUNT; i++) {
        if ((mask & (1u << i)) && self[i].depth && !--self[i].depth)
            self[i].excluded_ticks += now - self[i].excluded_start;
        if (self_stage[i] == stage && self[i].start == start) {
            if (self[i].depth || self[i].excluded_ticks > now - start) {
                stats[stage].self_dropped++;
            } else {
                stats[stage].self_ticks += now - start - self[i].excluded_ticks;
                stats[stage].self_calls++;
            }
            self[i].start = 0;
            active &= ~(1u << i);
        }
    }
    if (!sampled) {
        IRQ_Restore(level);
        return;
    }
#endif
    stats[stage].ticks += now - start;
    stats[stage].timed_calls++;
#if PERF_SUBSYSTEM_SELF
    IRQ_Restore(level);
#endif
}

struct perf_subsystem_stats perfProf_subsystemRead(unsigned int stage)
{
    /* Main-thread stages have one writer; the audio callback has its own ID. */
    unsigned int level = IRQ_Disable();
    struct perf_subsystem_stats result = stage < PERF_SUB_COUNT ? stats[stage] : (struct perf_subsystem_stats){0};
    IRQ_Restore(level);
    return result;
}
#endif
