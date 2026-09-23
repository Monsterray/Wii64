/* perf_prof.c - see perf_prof.h */
#include "perf_prof.h"

#ifdef PERF_PROF
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <ogc/system.h>
#include <ogc/machine/processor.h>

/* Opened and closed per write rather than held open: this runs interleaved
   with ROM/boxart file I/O elsewhere in the menu, and there's no guarantee
   about how many fds the FAT driver can hold open at once. */

static int g_bufLen; // the gameplay sample buffer, below

void perfProf_reset(void)
{
	g_bufLen = 0;
	FILE* f = fopen("sd:/wii64/perf.log", "w");
	if (!f) return;
	fprintf(f, "--- wii64 perf log ---\n");
	fclose(f);
}

void perfProf_pageBegin(int numTiles, int loadLimit)
{
	FILE* f = fopen("sd:/wii64/perf.log", "a");
	if (!f) return;
	fprintf(f, "page: tiles=%d loadLimit=%d\n", numTiles, loadLimit);
	fclose(f);
}

void perfProf_tileLoaded(int index, int wasReal, unsigned int initUs, unsigned int loadUs, unsigned int flushUs)
{
	FILE* f = fopen("sd:/wii64/perf.log", "a");
	if (!f) return;
	fprintf(f, "  tile %2d: real=%d init=%uus load=%uus flush=%uus\n", index, wasReal, initUs, loadUs, flushUs);
	fclose(f);
}

void perfProf_pageEnd(unsigned int invalidateUs, unsigned int totalUs)
{
	FILE* f = fopen("sd:/wii64/perf.log", "a");
	if (!f) return;
	fprintf(f, "page end: invalidate=%uus total=%uus\n", invalidateUs, totalUs);
	fclose(f);
}

/* One-shot sanity check of gettime()/PERF_US() themselves, run once at boot
   with no menu/file-I/O work involved: five back-to-back timestamps with a
   trivial memset between each. If PERF_US() is working, every delta here
   should be on the order of single-digit-to-low-hundreds of microseconds --
   anything wildly larger (or non-monotonic between raw ticks) means the bug
   is in the timer itself, not in whatever it's wrapped around elsewhere. */
void perfProf_selfTest(void)
{
	char scratch[256];
	FILE* f = fopen("sd:/wii64/perf.log", "a");
	if (!f) return;
	fprintf(f, "selftest:\n");

	u64 raw[5];
	unsigned int deltaUs[4];
	raw[0] = gettime();
	memset(scratch, 0, sizeof(scratch));
	raw[1] = gettime();
	deltaUs[0] = (unsigned int)ticks_to_microsecs(raw[1]-raw[0]);
	memset(scratch, 1, sizeof(scratch));
	raw[2] = gettime();
	deltaUs[1] = (unsigned int)ticks_to_microsecs(raw[2]-raw[1]);
	memset(scratch, 2, sizeof(scratch));
	raw[3] = gettime();
	deltaUs[2] = (unsigned int)ticks_to_microsecs(raw[3]-raw[2]);
	memset(scratch, 3, sizeof(scratch));
	raw[4] = gettime();
	deltaUs[3] = (unsigned int)ticks_to_microsecs(raw[4]-raw[3]);

	for (int i = 0; i < 5; i++)
		fprintf(f, "  raw[%d]=%llu\n", i, (unsigned long long)raw[i]);
	for (int i = 0; i < 4; i++)
		fprintf(f, "  delta[%d]=%uus\n", i, deltaUs[i]);
	fclose(f);
}

/* A single labeled checkpoint, immediately flushed to disk (not buffered):
   if the game hangs two lines after some mark, that mark is the last stage
   that provably completed. Deliberately dumb -- no timing, just "did we get
   here" -- because when the hang IS the bug, a crash before the next write
   must not lose the ones already made. */
static void buf_flush(void);

void perfProf_mark(const char* label)
{
	buf_flush(); // keep the log in order: buffered samples first
	FILE* f = fopen("sd:/wii64/perf.log", "a");
	if (!f) return;
	fprintf(f, "mark: %s\n", label);
	fclose(f);
}

/* --- Gameplay samples: buffered in RAM, flushed every PERF_FLUSH_SAMPLES ---
   One sample every ~500ms (main/timers.c's new_vi()/new_frame(), the same
   cadence as the in-game "Show FPS" overlay). vis is the VI-interrupt rate,
   fps the completed-display-list rate -- neither is real N64 speed, both are
   comparable build to build. On real SD every fclose() is a flush the
   emulator thread waits for, so the lines collect here and go out 20 at a
   time (~10 s); a kill loses at most that much, a chain game's end loses
   nothing. */
#define PERF_BUF_SIZE (16 * 1024)
#define PERF_FLUSH_SAMPLES 20
static char g_buf[PERF_BUF_SIZE];
static int g_bufLen;
static int g_samples;

/* Per-game totals; perfProf_gameBegin() zeroes them. Hot-path counters are
   single increments, the ISR-side audio ones volatile. */
static struct {
	u64 start, flushTicks, pmc[4];
	unsigned long long sleepUs;
	unsigned int exceptions, cacheResets, batches, verts, texStalls, recompiles;
	unsigned int treeDepthMax, flushes, visN, fpsN, vi0Retrace;
	double visSum, fpsSum;
} g;
static volatile unsigned int g_underruns, g_overruns;

static void buf_flush(void)
{
	if (!g_bufLen) return;
	u64 t0 = gettime();
	FILE* f = fopen("sd:/wii64/perf.log", "a");
	if (f) {
		fwrite(g_buf, 1, g_bufLen, f);
		fclose(f);
	}
	g_bufLen = 0;
	g.flushes++;
	g.flushTicks += gettime() - t0;
}

static void buf_printf(const char* fmt, ...)
{
	va_list ap;
	if (g_bufLen > PERF_BUF_SIZE - 512) buf_flush();
	va_start(ap, fmt);
	int n = vsnprintf(g_buf + g_bufLen, PERF_BUF_SIZE - g_bufLen, fmt, ap);
	va_end(ap);
	if (n > 0) g_bufLen += n < PERF_BUF_SIZE - g_bufLen ? n : PERF_BUF_SIZE - g_bufLen - 1;
}

/* --- Broadway's performance counters ---
   PMC1..4 count the events MMCR0/MMCR1 select. They are 32 bits wide: at
   729 MHz a cycle count wraps every 5.9 s, so they are read at every 500 ms
   sample and the deltas summed into 64 bits -- a single read at the end of a
   run would report the total modulo 2^32. Defaults: PMC1 = processor
   cycles, PMC2 = instructions completed (their ratio moves when stalls go
   away). Pick other events at build time, e.g.
   DEBUG_FLAGS="-DPERF_PROF -DPMC_MMCR0=0x...". Dolphin counts cycles but not
   instructions completed (PowerPC.cpp UpdatePerformanceMonitor), so PMC2
   reads 0 there and only means something on a Wii. */
#ifndef PMC_MMCR0
#define PMC_MMCR0 ((1 << 6) | 2)
#endif
#ifndef PMC_MMCR1
#define PMC_MMCR1 0
#endif
static u32 g_pmcLast[4];

static void pmc_start(void)
{
	mtmmcr0(0);
	mtpmc1(0); mtpmc2(0); mtpmc3(0); mtpmc4(0);
	mtmmcr1(PMC_MMCR1);
	mtmmcr0(PMC_MMCR0);
	memset(g_pmcLast, 0, sizeof(g_pmcLast));
}

static void pmc_accumulate(void)
{
	u32 v[4] = { mfpmc1(), mfpmc2(), mfpmc3(), mfpmc4() };
	for (int i = 0; i < 4; i++) {
		g.pmc[i] += (u32)(v[i] - g_pmcLast[i]);
		g_pmcLast[i] = v[i];
	}
}

void perfProf_visSample(float vis)
{
	g.visSum += vis;
	g.visN++;
	buf_printf("vis: %.1f\n", vis);
}

void perfProf_fpsSample(float fps)
{
	g.fpsSum += fps;
	g.fpsN++;
	buf_printf("fps: %.1f\n", fps);
}

void perfProf_exceptionOccurred(void) { g.exceptions++; }
void perfProf_cacheReset(void)        { g.cacheResets++; }
void perfProf_recompile(void)         { g.recompiles++; }
void perfProf_texStall(void)          { g.texStalls++; }
void perfProf_audioUnderrun(void)     { g_underruns++; }
void perfProf_audioOverrun(void)      { g_overruns++; }

/* Triangle batches (each re-sends the full GX vertex descriptor/format) and
   the vertices they carry. */
void perfProf_drawBatch(unsigned int verts)
{
	g.batches++;
	g.verts += verts;
}

/* r4300/ppc/FuncTree.c: depth of each function-tree lookup; the per-game
   worst is what says whether the BST has degenerated. */
void perfProf_treeDepth(unsigned int depth)
{
	if (depth > g.treeDepthMax) g.treeDepthMax = depth;
}

/* main/timers.c: time the frame limiter spent asleep. Its share of wall
   time is the headroom: 0 means the game is not keeping full speed. */
void perfProf_limiterSleep(long us)
{
	g.sleepUs += us;
}

void perfProf_cpuSample(void)
{
	pmc_accumulate();
	buf_printf("cpu: exceptions=%u cacheResets=%u batches=%u verts=%u texStalls=%u recompiles=%u underruns=%u overruns=%u sleep_us=%llu\n",
		g.exceptions, g.cacheResets, g.batches, g.verts, g.texStalls, g.recompiles,
		g_underruns, g_overruns, g.sleepUs);
	if (++g_samples % PERF_FLUSH_SAMPLES == 0)
		buf_flush();
}

/* --- Pad trace ---
   What the game read from each controller (pif.c, the bytes that go into
   the PIF reply) next to the GameCube driver's raw reading that produced
   it, one record whenever either changes. Written per game to
   sd:/wii64/padtrace_NN.csv; scripts/padtest.py checks the coverage. */
#define PAD_TRACE_N 8192
static struct {
	u32 vi, value;
	s8 sx, sy, cx, cy;
	u16 btns;
	u8 ctrl;
} g_pad[PAD_TRACE_N];
static int g_padN;
static struct { s8 sx, sy, cx, cy; u16 btns; } g_padRaw;
extern unsigned int diag_vi_count;

void perfProf_padRaw(int sx, int sy, int cx, int cy, unsigned int btns)
{
	g_padRaw.sx = sx; g_padRaw.sy = sy; g_padRaw.cx = cx; g_padRaw.cy = cy;
	g_padRaw.btns = btns;
}

void perfProf_padRead(int control, unsigned int value)
{
	static u32 lastValue[4] = { ~0u, ~0u, ~0u, ~0u };
	static u64 lastRaw;
	/* The raw reading is port 1's (the only one padsweep drives), so only port 1's
	   records change with it; the other ports log only when what the game read changed. */
	u64 raw = ((u64)g_padRaw.btns << 32) | ((u32)(u8)g_padRaw.sx << 24) | ((u8)g_padRaw.sy << 16)
	          | ((u8)g_padRaw.cx << 8) | (u8)g_padRaw.cy;
	if (control < 0 || control > 3 || g_padN >= PAD_TRACE_N) return;
	if (value == lastValue[control] && (control != 0 || raw == lastRaw)) return;
	lastValue[control] = value;
	if (control == 0) lastRaw = raw;
	g_pad[g_padN].vi = diag_vi_count;
	g_pad[g_padN].value = value;
	g_pad[g_padN].sx = g_padRaw.sx; g_pad[g_padN].sy = g_padRaw.sy;
	g_pad[g_padN].cx = g_padRaw.cx; g_pad[g_padN].cy = g_padRaw.cy;
	g_pad[g_padN].btns = g_padRaw.btns;
	g_pad[g_padN].ctrl = control;
	g_padN++;
}

static void pad_dump(int n)
{
	char name[40];
	if (!g_padN) return;
	snprintf(name, sizeof(name), "sd:/wii64/padtrace_%02d.csv", n);
	FILE* f = fopen(name, "w");
	if (!f) return;
	fprintf(f, "vi,ctrl,sx,sy,cx,cy,btns,value\n");
	for (int i = 0; i < g_padN; i++)
		fprintf(f, "%u,%u,%d,%d,%d,%d,%u,%08x\n", g_pad[i].vi, g_pad[i].ctrl,
			g_pad[i].sx, g_pad[i].sy, g_pad[i].cx, g_pad[i].cy, g_pad[i].btns, g_pad[i].value);
	fclose(f);
}

/* --- Per game --- */
void perfProf_gameBegin(void)
{
	buf_flush();
	memset(&g, 0, sizeof(g));
	g_underruns = g_overruns = 0;
	g_padN = 0;
	g.start = gettime();
	pmc_start();
}

/* main/timers.c, at the game's first VI: wall time and the PMCs cover gameplay only,
   not loadROM's tail or the autoboot pause before go(). */
void perfProf_clockStart(void)
{
	g.start = gettime();
	extern volatile unsigned int diag_retraces; // main_gc-menu2.cpp
	g.vi0Retrace = diag_retraces; // host frame of guest VI 1: scripts/dtm2input.py --offset
	buf_printf("first_vi: vi0_retrace=%u\n", g.vi0Retrace); // in a recording run, which never reaches gameEnd
	memset(g.pmc, 0, sizeof(g.pmc));
	pmc_start();
}

void perfProf_gameEnd(int n, int total, unsigned int vis, const char* rom, const char* how)
{
	struct mallinfo mi = mallinfo();
	unsigned long long wallUs = ticks_to_microsecs(gettime() - g.start);
	pmc_accumulate();
	extern float VILimit; // main/timers.c: the VI rate this ROM is paced at (50/60)
	buf_printf("game: n=%d/%d how=%s vis=%u vi_rate=%.0f vi0_retrace=%u wall_us=%llu sleep_us=%llu avg_vis=%.2f avg_fps=%.2f"
		" exceptions=%u cacheResets=%u recompiles=%u batches=%u verts=%u texStalls=%u"
		" treeDepthMax=%u underruns=%u overruns=%u"
		" pmc1=%llu pmc2=%llu pmc3=%llu pmc4=%llu mmcr0=%08x mmcr1=%08x"
		" heap_used=%d heap_free=%d arena1_free=%u arena2_free=%u"
		" flushes=%u flush_us=%llu padtrace=%d rom=%s\n",
		n, total, how, vis, VILimit, g.vi0Retrace, wallUs, g.sleepUs,
		g.visN ? g.visSum / g.visN : 0.0, g.fpsN ? g.fpsSum / g.fpsN : 0.0,
		g.exceptions, g.cacheResets, g.recompiles, g.batches, g.verts, g.texStalls,
		g.treeDepthMax, g_underruns, g_overruns,
		g.pmc[0], g.pmc[1], g.pmc[2], g.pmc[3], (unsigned)PMC_MMCR0, (unsigned)PMC_MMCR1,
		mi.uordblks, mi.fordblks,
		(unsigned)((u32)SYS_GetArena1Hi() - (u32)SYS_GetArena1Lo()),
		(unsigned)((u32)SYS_GetArena2Hi() - (u32)SYS_GetArena2Lo()),
		g.flushes, (unsigned long long)ticks_to_microsecs(g.flushTicks), g_padN, rom);
	buf_printf("=== chain %d/%d end vis=%u how=%s rom=%s ===\n", n, total, vis, how, rom);
	buf_flush();
	pad_dump(n);
}

/* The part of opening the ROM browser that isn't the already-instrumented
   boxart page fill (pageBegin/tileLoaded/pageEnd): the directory scan
   itself and the per-entry ROM header peek that follows it -- see
   menu/SelectRomFrame.cpp's selectRomFrame_OpenDirectory. */
void perfProf_dirScan(int entries, unsigned int readDirUs, unsigned int headersUs)
{
	FILE* f = fopen("sd:/wii64/perf.log", "a");
	if (!f) return;
	fprintf(f, "dirscan: entries=%d readDirUs=%u headersUs=%u\n", entries, readDirUs, headersUs);
	fclose(f);
}

void perfProf_menuFpsSample(float fps)
{
	FILE* f = fopen("sd:/wii64/perf.log", "a");
	if (!f) return;
	fprintf(f, "menufps: %.1f\n", fps);
	fclose(f);
}

#endif /* PERF_PROF */
