#include <cassert>
#include "../main/perf_subsystem.h"

static unsigned long long tick;
extern "C" unsigned long long gettime(void) { return tick += 100; }

static void early_return()
{
    PERF_SUBSYSTEM_SCOPE(PERF_SUB_TEX_LOAD);
    return;
}

int main()
{
    perfProf_subsystemReset();
    early_return();
#ifdef PERF_SUBSYSTEM_ENABLED
    assert(perfProf_subsystemRead(PERF_SUB_TEX_LOAD).timed_calls == 1);
    assert(perfProf_subsystemRead(PERF_SUB_TEX_LOAD).ticks == 100);
    {
        PERF_SUBSYSTEM_SCOPE(PERF_SUB_DRAW_RECT);
        early_return();
    }
    assert(perfProf_subsystemRead(PERF_SUB_DRAW_RECT).ticks == 300);
    {
        PERF_SUBSYSTEM_SCOPE(PERF_SUB_TEX_HASH);
        perfProf_subsystemReset();
    }
    assert(perfProf_subsystemRead(PERF_SUB_TEX_HASH).timed_calls == 0);
#else
    assert(tick == 0);
#endif
}
