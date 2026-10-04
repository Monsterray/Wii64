/* Opt-in emulation-thread PC sampling. See doc/performance-methods-2026-10-04.md.
 * PMC selection/rearm follows WiiStation d3dc67a, Gamecube/hprof.c.
 */
#include "hprof.h"
#ifdef WII64_HPROF
#include "hprof_hist.h"
#include <gccore.h>
#include <ogc/irq.h>
#include <ogc/machine/processor.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#define HPROF_PM 4u
#ifndef HPROF_PERIOD
#define HPROF_PERIOD 729000u /* 1 ms of eligible processor cycles on Broadway. */
#endif
#if HPROF_PERIOD < 72900 || HPROF_PERIOD >= 0x80000000u
#error "HPROF_PERIOD must be at least 100 us and below the overflow bit"
#endif
#define HPROF_MMCR0 (0x04000000u | 0x08000000u | 0x00008000u | (1u << 6))

extern char hprof_text_start[], hprof_text_end[];
extern void hprof_entry(void);
extern void (*_exceptionhandlertable[])(frame_context *);
extern void __exception_sethandler(u32, void (*)(frame_context *));

static struct hprof_hist hist;
static int requested, active, started, restored;
static struct {
    u32 mmcr0, mmcr1, pmc[4], pm;
    void (*handler)(frame_context *);
} saved;

void hprof_configure(int enabled) { requested = enabled != 0; }
int hprof_requested(void) { return requested; }

void hprof_prepare(void)
{
    hprof_stop();
    if (!hist.buckets) hist.buckets = memalign(32, HPROF_BYTES);
    if (hist.buckets) memset(hist.buckets, 0, HPROF_BYTES);
    hprof_layout(&hist, (u32)hprof_text_start, (u32)hprof_text_end);
    hist.total = hist.jit = hist.other = hist.overflow = 0;
    started = restored = 0;
}

void hprof_jitRange(void *base, unsigned int size)
{
    /* Called once, before sampling starts; aggregate range survives block reuse. */
    hist.jit_base = (u32)base;
    hist.jit_size = base ? size : 0;
}

void hprof_sample(u32 pc)
{
    hprof_record(&hist, pc);
    /* Ordered rearm is required on real Wii; never enable EE here. */
    mtmmcr0(0); __asm__ volatile ("isync" ::: "memory");
    mtpmc1(0x80000000u - HPROF_PERIOD); __asm__ volatile ("isync" ::: "memory");
    mtmmcr0(HPROF_MMCR0); __asm__ volatile ("isync" ::: "memory");
}

void hprof_start(void)
{
    if (!requested || active || !hist.buckets || !hist.count) return;
    u32 level = IRQ_Disable();
    saved.mmcr0 = mfmmcr0(); saved.mmcr1 = mfmmcr1();
    mtmmcr0(0); __asm__ volatile ("isync" ::: "memory");
    saved.pmc[0] = mfpmc1(); saved.pmc[1] = mfpmc2();
    saved.pmc[2] = mfpmc3(); saved.pmc[3] = mfpmc4();
    saved.handler = _exceptionhandlertable[EX_PERF];
    saved.pm = mfmsr() & HPROF_PM;
    __exception_sethandler(EX_PERF, (void (*)(frame_context *))hprof_entry);
    /* New/background/idle threads remain unmarked. libogc saves/restores PM. */
    mtmsr(mfmsr() | HPROF_PM); __asm__ volatile ("isync" ::: "memory");
    mtmmcr1(0);
    mtpmc1(0x80000000u - HPROF_PERIOD); __asm__ volatile ("isync" ::: "memory");
    active = started = 1;
    mtmmcr0(HPROF_MMCR0); __asm__ volatile ("isync" ::: "memory");
    IRQ_Restore(level);
}

void hprof_stop(void)
{
    if (!active) return;
    u32 level = IRQ_Disable();
    mtmmcr0(0); __asm__ volatile ("isync" ::: "memory");
    __exception_sethandler(EX_PERF, saved.handler);
    mtmmcr1(saved.mmcr1);
    mtpmc1(saved.pmc[0]); mtpmc2(saved.pmc[1]);
    mtpmc3(saved.pmc[2]); mtpmc4(saved.pmc[3]);
    __asm__ volatile ("isync" ::: "memory");
    int countersRestored = mfpmc1() == saved.pmc[0] && mfpmc2() == saved.pmc[1] &&
        mfpmc3() == saved.pmc[2] && mfpmc4() == saved.pmc[3];
    mtmsr((mfmsr() & ~HPROF_PM) | saved.pm);
    __asm__ volatile ("isync" ::: "memory");
    mtmmcr0(saved.mmcr0); __asm__ volatile ("isync" ::: "memory");
    restored = countersRestored && mfmmcr0() == saved.mmcr0 && mfmmcr1() == saved.mmcr1 &&
        (mfmsr() & HPROF_PM) == saved.pm && _exceptionhandlertable[EX_PERF] == saved.handler;
    active = 0;
    IRQ_Restore(level);
}

int hprof_dump(int game)
{
    hprof_stop();
    /* Big-endian u32 header, then PC buckets. Controls reserve the same memory. */
    u32 header[16] = {0x48363450, 1, hist.base, hist.end, hist.shift, hist.count,
        HPROF_PERIOD, hist.total, hist.jit, hist.other, requested, started,
        restored, hist.overflow, hist.buckets ? HPROF_BYTES : 0, 1 /* main-thread scope */};
    char path[40];
    snprintf(path, sizeof(path), "sd:/wii64/hprof_%02d.bin", game);
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    int ok = fwrite(header, sizeof(header), 1, f) == 1;
    if (ok && hist.buckets) ok = fwrite(hist.buckets, sizeof(u32), hist.count, f) == hist.count;
    if (fclose(f)) ok = 0;
    if (!ok) remove(path);
    return ok;
}
#endif
