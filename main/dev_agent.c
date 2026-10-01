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

/* The agent's hang watchdog (SDK 1.9): armed by the first call, then 60 s without
   one is reported as a hang (`hbc.py crash`) and the Wii returns to HBC. Called per
   guest VI and per menu frame, from thread context: a call from the VI interrupt
   would keep it happy while the main thread is stuck. */
void devAgent_alive(void) { hbc_agent_alive(); }
void devAgent_hold(int hold) { hbc_agent_hold(hold != 0); }

/* libogc's vector code sends an exception taken with MSR[RI] clear straight to
   default_exceptionhandler, past the agent's table hook, so the Wii returned to HBC
   with no report (WiiStation, 2026-10-01). Makefile.wii wraps the function every
   crash screen goes through and records the exception here unless the agent already
   recorded this one. From WiiStation's Gamecube/ws_crash.c. */
#include <string.h>
#include <ogc/cache.h>
#include <ogc/context.h>
#include <ogc/machine/processor.h>

static int ram_word(u32 a)
{
    return !(a & 3) && ((a >= 0x80000000 && a < 0x81800000) || (a >= 0x90000000 && a < 0x94000000));
}

void __real_c_default_exceptionhandler(frame_context *ctx);

void __wrap_c_default_exceptionhandler(frame_context *ctx)
{
    static const u8 vector[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0x0c, 0x0d, 0x0f, 0x13, 0x14, 0x17 };
    hbc_crash_block *b = (hbc_crash_block *)HBC_CRASH_ADDR;
    u32 n = ctx->nExcept, sp = ctx->gpr[1], i;

    if (!(b->magic == HBC_CRASH_MAGIC && b->check == hbc_crash_check(b) && b->pc == ctx->srr0)) {
        memset(b, 0, sizeof *b);
        b->magic = HBC_CRASH_MAGIC;
        b->version = HBC_CRASH_VERSION;
        b->kind = HBC_CRASH_EXCEPTION;
        b->exception = n < sizeof vector ? vector[n] : n;
        b->pc = ctx->srr0;
        b->msr = ctx->srr1;
        b->lr = ctx->lr;
        b->cr = ctx->cr;
        b->ctr = ctx->ctr;
        b->dar = mfspr(19);
        b->dsisr = mfspr(18);
        b->sp = sp;
        for (i = 0; i < HBC_CRASH_FRAMES && ram_word(sp); i++) {
            u32 next = *(u32 *)sp;
            if (!next)
                break;
            next |= 0x80000000; /* an exception frame's back chain is a physical address */
            if (!ram_word(next) || next <= sp)
                break;
            b->frames[i] = *(u32 *)(next + 4);
            sp = next;
        }
        strcpy(b->app, "Wii64");
        strcpy(b->reason, "past the agent's hook (MSR[RI] clear?)");
        b->check = hbc_crash_check(b);
        DCFlushRange(b, sizeof *b);
    }
    __real_c_default_exceptionhandler(ctx);
}

#ifdef PERF_PROF
unsigned int devAgent_crashVi;
__attribute__((noinline)) void devAgent_testCrash(void)
{
    *(volatile u32*)0x10 = 0x57494936; // Deliberate fatal DSI, outside the ROM VM range.
}
#endif
#endif
