#include "dev_agent.h"
#ifdef WII64_HBC_AGENT
#include <network.h>
#include <hbc_agent.h>
#include "version.h"
#include "../gc_memory/MEM2.h"
#include "perf_prof.h"

_Static_assert(HBC_CRASH_ADDR >= (unsigned long)TLBLUT_HI &&
               HBC_CRASH_ADDR + sizeof(hbc_crash_block) <= (unsigned long)TEXCACHE_LO,
               "HBC crash record must fit the reserved MEM2 gap");
#ifdef HBC_LASTLOG_ADDR
_Static_assert(HBC_LASTLOG_ADDR + sizeof(hbc_lastlog_block) <= (unsigned long)TEXCACHE_LO,
               "HBC last-output block must fit the reserved MEM2 gap");
#endif

void devAgent_init(void)
{
    /* SDK starts networking asynchronously; uploads wait before net_init(). */
    hbc_agent_config cfg = {0};
    cfg.name = "Wii64";
    cfg.version = WII64_VERSION;
    cfg.priority = 40;
    cfg.app_polls_exit = true;
    cfg.exit_grace_ms = 10000;
    cfg.crash_reload_s = 3;
    cfg.gc_pads = true;
    /* Leave HOME closed: it needs ~1.8 MiB transient MEM1, not spare MEM2. */
    int result = hbc_agent_init(&cfg);
    perfProf_mark(result == 0 ? "HBC agent ready" : "HBC agent init failed");
}

int devAgent_exitRequested(void)
{
    PERF_SUBSYSTEM_C_SCOPE(PERF_SUB_AGENT_POLL);
    return hbc_agent_exit_requested();
}
int devAgent_netReady(void) { return hbc_agent_net_wait(5000) >= 0; }

#ifdef PERF_PROF
unsigned int devAgent_crashVi;
__attribute__((noinline)) void devAgent_testCrash(void)
{
    *(volatile u32*)0x10 = 0x57494936; // Deliberate fatal DSI, outside the ROM VM range.
}
#endif
#endif
