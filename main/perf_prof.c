/* perf_prof.c - see perf_prof.h */
#include "perf_prof.h"

#ifdef PERF_PROF
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <aesndlib.h>
#include <ogc/gx.h>
#include <ogc/system.h>
#include <ogc/machine/processor.h>
#include <ogc/irq.h>
#ifdef HW_RVL
#include "../vm/pagefile.h"
#endif

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
	unsigned int treeDepthMax, flushes, visN, fpsN, dspN, vi0Retrace, queuePeakMs;
	unsigned int alistResampleCalls, alistResampleSamples, alistZohCalls, alistZohSamples;
	unsigned int musyxSubframes, musyxVoices, musyxVoicePeak;
	unsigned int audioStageCalls[PERF_AUDIO_STAGE_COUNT], audioStageSamples[PERF_AUDIO_STAGE_COUNT];
	unsigned int audioSteadyCalls[PERF_AUDIO_STAGE_COUNT];
	unsigned int audioGapCalls[PERF_AUDIO_GAP_COUNT];
	unsigned int audioTimedCalls[PERF_AUDIO_STAGE_COUNT], audioTimedSamples[PERF_AUDIO_STAGE_COUNT];
	unsigned int audioTimerCountdown[PERF_AUDIO_STAGE_COUNT];
	u64 audioTimedTicks[PERF_AUDIO_STAGE_COUNT];
	double visSum, fpsSum, dspSum;
	float dspPeak;
} g;
static volatile unsigned int g_underruns, g_overruns;

/* Interrupt context (the VI retrace and power callbacks) must not touch the SD card:
   libfat and IOS sleep, and the interrupted thread may hold the FAT lock. So
   perfProf_markLater only records the label, a string literal, and the next flush
   from thread context writes it. Up to MARK_LATER_MAX between flushes; more are
   dropped. Callers in either context take the list with interrupts off. */
#define MARK_LATER_MAX 8
static const char* g_markLater[MARK_LATER_MAX];
static unsigned int g_markLaterN;
static void buf_printf(const char* fmt, ...);

void perfProf_markLater(const char* label)
{
	u32 level = IRQ_Disable();
	if (g_markLaterN < MARK_LATER_MAX)
		g_markLater[g_markLaterN++] = label;
	IRQ_Restore(level);
}

static void mark_later_drain(void)
{
	const char* labels[MARK_LATER_MAX];
	u32 level = IRQ_Disable();
	unsigned int n = g_markLaterN;
	memcpy(labels, g_markLater, n * sizeof(labels[0]));
	g_markLaterN = 0;
	IRQ_Restore(level);
	for (unsigned int i = 0; i < n; i++)
		buf_printf("mark: %s\n", labels[i]);
}

static void buf_flush(void)
{
	mark_later_drain();
	if (!g_bufLen) return;
	PERF_SUBSYSTEM_C_SCOPE(PERF_SUB_PROBE_IO);
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

/* --- The GP's own counters ---
   GX_InitXfRasMetric makes four CP counters count at once: GP clocks,
   rasterizer-busy clocks, and the clocks the transform unit (XF) waited for
   input (nothing to do: idle, or the CPU does not feed the FIFO fast enough)
   or for output (setup, raster, TEV or PE behind it). The PE counts pixels
   into and out of the early (top) and late (bottom) Z tests, pixels into the
   blender, and EFB copy clocks. All are 32 bits: the 243 MHz clock wraps
   every 17.7 s, so they are read at every 500 ms sample, as the PMCs are.
   They are selected once per game, from loadROM; after that only registers
   are read: no GX commands and no waits. tbClks is the same span from the
   time base (GP clock = 4 x time base on Wii and GameCube): clks must match
   it, else the counter did not count. Dolphin returns 0 for the XF/RAS
   counters; only a Wii measures them. libogc counts FIFO overflows: each one
   suspends the GX thread until the GP has read the FIFO down. */
enum { GP_XF_WAIT_IN, GP_XF_WAIT_OUT, GP_RAS_BUSY, GP_CLKS, GP_ZTOP_IN, GP_ZTOP_OUT,
       GP_Z_IN, GP_Z_OUT, GP_BLEND_IN, GP_COPY_CLKS, GP_COUNT };
static struct { u64 total[GP_COUNT], tbClks, lastTb; u32 last[GP_COUNT], rasPeak; } gp;

static void gp_read(u32 v[GP_COUNT])
{
	GX_ReadXfRasMetric(&v[GP_XF_WAIT_IN], &v[GP_XF_WAIT_OUT], &v[GP_RAS_BUSY], &v[GP_CLKS]);
	GX_ReadPixMetric(&v[GP_ZTOP_IN], &v[GP_ZTOP_OUT], &v[GP_Z_IN], &v[GP_Z_OUT],
		&v[GP_BLEND_IN], &v[GP_COPY_CLKS]);
}

static void gp_start(void)
{
	memset(&gp, 0, sizeof(gp));
	gp_read(gp.last);
	gp.lastTb = gettime();
	GX_ResetOverflowCount();
}

static void gp_accumulate(void)
{
	u32 v[GP_COUNT], d[GP_COUNT];
	gp_read(v);
	u64 now = gettime();
	for (int i = 0; i < GP_COUNT; i++) {
		d[i] = v[i] - gp.last[i];
		gp.total[i] += d[i];
		gp.last[i] = v[i];
	}
	// Busiest 500 ms of rasterizer, in 1/1000: an average hides a GP-bound scene.
	if (d[GP_CLKS] && (u64)d[GP_RAS_BUSY] * 1000 / d[GP_CLKS] > gp.rasPeak)
		gp.rasPeak = (u64)d[GP_RAS_BUSY] * 1000 / d[GP_CLKS];
	gp.tbClks += (now - gp.lastTb) * 4;
	gp.lastTb = now;
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
#ifndef PERF_AUDIO_WORK_DISABLE
static u64 audio_timer_start(unsigned int stage, unsigned int samples)
{
#ifdef PERF_AUDIO_TIMING_DISABLE
	(void)stage; (void)samples;
	return 0;
#else
	if (--g.audioTimerCountdown[stage]) return 0;
	g.audioTimerCountdown[stage] = 127;
	g.audioTimedSamples[stage] += samples;
	return gettime();
#endif
}
unsigned long long perfProf_alistResample(unsigned int samples)
{
	g.alistResampleCalls++;
	g.alistResampleSamples += samples;
	return audio_timer_start(PERF_AUDIO_RESAMPLE, samples);
}
unsigned long long perfProf_alistZoh(unsigned int samples)
{
	g.alistZohCalls++;
	g.alistZohSamples += samples;
	return audio_timer_start(PERF_AUDIO_ZOH, samples);
}
void perfProf_musyxVoices(unsigned int voices)
{
	g.musyxSubframes++;
	g.musyxVoices += voices;
	if (voices > g.musyxVoicePeak) g.musyxVoicePeak = voices;
}
unsigned long long perfProf_audioStage(unsigned int stage, unsigned int samples)
{
	if (stage >= PERF_AUDIO_STAGE_COUNT) return 0;
	g.audioStageCalls[stage]++;
	g.audioStageSamples[stage] += samples;
	return audio_timer_start(stage, samples);
}
void perfProf_audioStageEnd(unsigned int stage, unsigned long long start)
{
	if (!start || stage >= PERF_AUDIO_STAGE_COUNT) return;
	g.audioTimedCalls[stage]++;
	g.audioTimedTicks[stage] += gettime() - start;
}
void perfProf_audioSteady(unsigned int stage)
{
	if (stage < PERF_AUDIO_STAGE_COUNT) g.audioSteadyCalls[stage]++;
}
void perfProf_audioGap(unsigned int gap)
{
	if (gap < PERF_AUDIO_GAP_COUNT) g.audioGapCalls[gap]++;
}
#endif

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
	gp_accumulate();
	extern unsigned int audioQueuedMilliseconds(void);
	unsigned int queuedMs = audioQueuedMilliseconds();
	if (queuedMs > g.queuePeakMs) g.queuePeakMs = queuedMs;
	/* AESND reports usage of its latest 2 ms DSP block. Sample outside hot audio paths. */
	float dsp = AESND_GetDSPProcessUsage();
	g.dspSum += dsp;
	g.dspN++;
	if (dsp > g.dspPeak) g.dspPeak = dsp;
	buf_printf("cpu: exceptions=%u cacheResets=%u batches=%u verts=%u texStalls=%u recompiles=%u underruns=%u overruns=%u sleep_us=%llu queue_ms=%u\n",
		g.exceptions, g.cacheResets, g.batches, g.verts, g.texStalls, g.recompiles,
		g_underruns, g_overruns, g.sleepUs, queuedMs);
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
#ifdef HW_RVL
	pagefile_stats_reset();
#endif
	buf_flush();
	memset(&g, 0, sizeof(g));
	perfProf_subsystemReset();
	GX_InitXfRasMetric(); // loadROM, between frames: three GX commands, once per game
	GX_ClearPixMetric();
	GX_Flush();
	for (unsigned int i = 0; i < PERF_AUDIO_STAGE_COUNT; i++) g.audioTimerCountdown[i] = 127;
	g_underruns = g_overruns = 0;
	g_padN = 0;
	g.start = gettime();
	pmc_start();
	gp_start(); // again at the first VI, when the GP has run those commands
}

/* main/timers.c, at the game's first VI: wall time and the PMCs cover gameplay only,
   not loadROM's tail or the autoboot pause before go(). */
void perfProf_clockStart(void)
{
#ifdef HW_RVL
	pagefile_stats_reset();
#endif
	g.start = gettime();
	perfProf_subsystemReset();
	extern volatile unsigned int diag_retraces; // main_gc-menu2.cpp
	g.vi0Retrace = diag_retraces; // host frame of guest VI 1: scripts/dtm2input.py --offset
	buf_printf("first_vi: vi0_retrace=%u\n", g.vi0Retrace); // in a recording run, which never reaches gameEnd
	memset(g.pmc, 0, sizeof(g.pmc));
	pmc_start();
	gp_start();
}

void perfProf_gameEnd(int n, int total, unsigned int vis, const char* rom, const char* how)
{
	static const char *const audioStageNames[] = {
		"resample", "zoh", "adpcm", "envmix_exp", "envmix_ge", "envmix_lin",
		"envmix_nead", "mix", "musyx_voice", "musyx_fx", "output"
	};
	unsigned long long wallUs = ticks_to_microsecs(gettime() - g.start);
	mark_later_drain(); // this game's stop reasons go before its game: line
	struct mallinfo mi = mallinfo();
	pmc_accumulate();
	gp_accumulate();
	unsigned int streamRequests, streamFed, inputHz, queuePeakMs, playbackHz;
	extern void audioOutputStats(unsigned int *, unsigned int *, unsigned int *, unsigned int *, unsigned int *);
	audioOutputStats(&streamRequests, &streamFed, &inputHz, &queuePeakMs, &playbackHz);
	extern float VILimit; // main/timers.c: the VI rate this ROM is paced at (50/60)
	buf_printf("audio: alist_resample_calls=%u alist_resample_samples=%u alist_zoh_calls=%u alist_zoh_samples=%u musyx_subframes=%u musyx_voices=%u musyx_voice_peak=%u\n",
		g.alistResampleCalls, g.alistResampleSamples, g.alistZohCalls,
		g.alistZohSamples, g.musyxSubframes,
		g.musyxVoices, g.musyxVoicePeak);
	unsigned int envmixCalls = 0, envmixSamples = 0;
	for (unsigned int i = PERF_AUDIO_ENVMIX_EXP; i <= PERF_AUDIO_ENVMIX_NEAD; i++) {
		envmixCalls += g.audioStageCalls[i];
		envmixSamples += g.audioStageSamples[i];
	}
	buf_printf("audio_stages: adpcm_calls=%u adpcm_samples=%u envmix_calls=%u envmix_samples=%u mix_calls=%u mix_samples=%u musyx_fx_calls=%u musyx_fx_taps=%u output_calls=%u output_samples=%u\n",
		g.audioStageCalls[PERF_AUDIO_ADPCM], g.audioStageSamples[PERF_AUDIO_ADPCM],
		envmixCalls, envmixSamples,
		g.audioStageCalls[PERF_AUDIO_MIX], g.audioStageSamples[PERF_AUDIO_MIX],
		g.audioStageCalls[PERF_AUDIO_MUSYX_FX], g.audioStageSamples[PERF_AUDIO_MUSYX_FX],
		g.audioStageCalls[PERF_AUDIO_OUTPUT], g.audioStageSamples[PERF_AUDIO_OUTPUT]);
	buf_printf("audio_envmix: exp_calls=%u ge_calls=%u lin_calls=%u nead_calls=%u exp_steady=%u ge_steady=%u lin_steady=%u\n",
		g.audioStageCalls[PERF_AUDIO_ENVMIX_EXP], g.audioStageCalls[PERF_AUDIO_ENVMIX_GE],
		g.audioStageCalls[PERF_AUDIO_ENVMIX_LIN], g.audioStageCalls[PERF_AUDIO_ENVMIX_NEAD],
		g.audioSteadyCalls[PERF_AUDIO_ENVMIX_EXP], g.audioSteadyCalls[PERF_AUDIO_ENVMIX_GE],
		g.audioSteadyCalls[PERF_AUDIO_ENVMIX_LIN]);
	buf_printf("audio_gaps: nead_mats=%u nead_efz=%u resample_flag2=%u musyx_ptr10=%u\n",
		g.audioGapCalls[PERF_AUDIO_GAP_NEAD_MATS], g.audioGapCalls[PERF_AUDIO_GAP_NEAD_EFZ],
		g.audioGapCalls[PERF_AUDIO_GAP_RESAMPLE_FLAG2], g.audioGapCalls[PERF_AUDIO_GAP_MUSYX_PTR10]);
	buf_printf("audio_output: stream_requests=%u stream_fed=%u input_hz=%u playback_hz=%u queue_peak_ms=%u\n",
		streamRequests, streamFed, inputHz, playbackHz, queuePeakMs);
#ifdef PERF_SUBSYSTEM_ENABLED
	buf_printf("probe_schema: version=2 self=%u interval=%u\n", PERF_SUBSYSTEM_SELF,
        perfProf_subsystemPeriod(PERF_SUB_EXECUTE));
#ifdef HW_RVL
	struct pagefile_stats io = pagefile_stats_read();
	buf_printf("vm_io: read_ahead=%d reads=%u hits=%u read_bytes=%u writes=%u write_bytes=%u errors=%u\n",
		VM_PAGE_READAHEAD, io.reads, io.cache_hits, io.read_bytes, io.writes, io.write_bytes, io.errors);
#endif
	static const char *const subsystemNames[] = {
		"rsp_gfx", "rsp_audio", "rsp_other", "lookup", "compile", "dispatch",
		"execute_inclusive", "rom_copy", "present", "limiter",
		"vm_fault", "vm_victim", "vm_read", "vm_write",
        "tex_hash", "tex_lookup", "tex_load", "tex_activate", "draw_triangles", "draw_rect",
        "gfx_list", "gfx_command", "vertex", "gfx_state", "gx_wait",
        "dma_pi", "dma_sp", "dma_si", "tlb_translate", "pif", "input",
        "guest_interrupt", "interpreter_inclusive", "audio_submit", "audio_callback",
        "memory_slow", "jit_invalidate", "cpu_helper", "storage_read", "storage_write",
        "save_load", "save_write", "state_load", "state_save", "probe_io", "agent_poll"
	};
    _Static_assert(sizeof(subsystemNames) / sizeof(subsystemNames[0]) == PERF_SUB_COUNT,
        "Every subsystem needs a log name");
	for (unsigned int i = 0; i < PERF_SUB_COUNT; i++) {
		struct perf_subsystem_stats s = perfProf_subsystemRead(i);
		buf_printf("subsystem_time: stage=%s calls=%u timed_calls=%u period=%u timed_us=%llu\n",
			subsystemNames[i], s.calls, s.timed_calls, perfProf_subsystemPeriod(i),
			(unsigned long long)ticks_to_microsecs(s.ticks));
        if (i == PERF_SUB_GFX_COMMAND) {
            buf_printf("gfx_ucodes: mask=%08x\n", perfProf_subsystemUcodes());
            for (unsigned int op = 0; op < 256; op++) {
                struct perf_subsystem_stats o = perfProf_subsystemOpcodeRead(op);
                if (o.calls)
                    buf_printf("gfx_opcode: op=%02x calls=%u timed_calls=%u timed_us=%llu\n", op,
                        o.calls, o.timed_calls, (unsigned long long)ticks_to_microsecs(o.ticks));
            }
        }
        if (perfProf_subsystemHasSelf(i))
            buf_printf("subsystem_self: stage=%s calls=%u timed_calls=%u self_us=%llu dropped=%u\n",
                subsystemNames[i], s.calls, s.self_calls,
                (unsigned long long)ticks_to_microsecs(s.self_ticks), s.self_dropped);
	}
#endif
	for (unsigned int i = 0; i < PERF_AUDIO_STAGE_COUNT; i++)
		buf_printf("audio_time: stage=%s sampled_calls=%u sampled_samples=%u sampled_us=%llu\n",
			audioStageNames[i], g.audioTimedCalls[i], g.audioTimedSamples[i],
			(unsigned long long)ticks_to_microsecs(g.audioTimedTicks[i]));
	buf_printf("gpu_counters: clks=%llu tb_clks=%llu ras_busy=%llu xf_wait_in=%llu xf_wait_out=%llu"
		" ras_peak_permille=%u ztop_in=%llu ztop_out=%llu z_in=%llu z_out=%llu blend_in=%llu"
		" copy_clks=%llu fifo_overflows=%u\n",
		gp.total[GP_CLKS], gp.tbClks, gp.total[GP_RAS_BUSY], gp.total[GP_XF_WAIT_IN],
		gp.total[GP_XF_WAIT_OUT], gp.rasPeak, gp.total[GP_ZTOP_IN], gp.total[GP_ZTOP_OUT],
		gp.total[GP_Z_IN], gp.total[GP_Z_OUT], gp.total[GP_BLEND_IN], gp.total[GP_COPY_CLKS],
		GX_GetOverflowCount());
	buf_printf("game: n=%d/%d how=%s vis=%u vi_rate=%.0f vi0_retrace=%u wall_us=%llu sleep_us=%llu avg_vis=%.2f avg_fps=%.2f"
		" exceptions=%u cacheResets=%u recompiles=%u batches=%u verts=%u texStalls=%u"
		" treeDepthMax=%u underruns=%u overruns=%u queue_peak_ms=%u dsp_avg=%.2f dsp_peak=%.2f dsp_samples=%u"
		" pmc1=%llu pmc2=%llu pmc3=%llu pmc4=%llu mmcr0=%08x mmcr1=%08x"
		" heap_used=%d heap_free=%d arena1_free=%u arena2_free=%u"
		" flushes=%u flush_us=%llu padtrace=%d rom=%s\n",
		n, total, how, vis, VILimit, g.vi0Retrace, wallUs, g.sleepUs,
		g.visN ? g.visSum / g.visN : 0.0, g.fpsN ? g.fpsSum / g.fpsN : 0.0,
		g.exceptions, g.cacheResets, g.recompiles, g.batches, g.verts, g.texStalls,
		g.treeDepthMax, g_underruns, g_overruns, g.queuePeakMs,
		g.dspN ? g.dspSum / g.dspN : 0.0, g.dspPeak, g.dspN,
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
