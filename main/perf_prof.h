/* perf_prof.h
   Performance probes, modeled on WiiStation's perf-probe pattern
   (Gamecube/perf_prof.c there): compiled in only when PERF_PROF is defined
   (pass -DPERF_PROF to make, or use .dev/build_profiling.sh), a no-op
   otherwise so release builds pay nothing. Writes sd:/wii64/perf.log --
   read it back with scripts/sdimage_read.py, split a chained run with
   scripts/chain_table.py, compare with scripts/perf_compare.py (see
   baselines/README.md).

   Cost rules (from WiiStation's measured mistakes -- a probe that costs more
   than what it measures makes the number meaningless):
   - hot-path probes are one integer op; no gettime() or 64-bit divide per call;
   - nothing touches the SD card during gameplay except the sample buffer's
     flush, once per PERF_FLUSH_SAMPLES samples, and that flush times itself
     (flush_us on the game: line) so its cost is visible, not hidden;
   - marks (perfProf_mark) still write immediately: they exist for hangs, where
     a buffered line would be lost with the hang.

   Per game (loadROM -> perfProf_gameBegin, chain end -> perfProf_gameEnd) the
   totals below restart from zero, so a chain of games gives one comparable
   game: line per game.
 */
#ifndef PERF_PROF_H
#define PERF_PROF_H

#ifdef PERF_PROF
#include <ogc/lwp_watchdog.h>

#ifdef __cplusplus
extern "C" {
#endif

void perfProf_reset(void);
void perfProf_pageBegin(int numTiles, int loadLimit);
void perfProf_tileLoaded(int index, int wasReal, unsigned int initUs, unsigned int loadUs, unsigned int flushUs);
void perfProf_pageEnd(unsigned int invalidateUs, unsigned int totalUs);
void perfProf_selfTest(void);
void perfProf_mark(const char* label);
void perfProf_visSample(float vis);
void perfProf_fpsSample(float fps);
void perfProf_exceptionOccurred(void);
void perfProf_cacheReset(void);
void perfProf_drawBatch(unsigned int verts);
void perfProf_texStall(void);
void perfProf_recompile(void);
void perfProf_treeDepth(unsigned int depth);
void perfProf_limiterSleep(long us);
void perfProf_audioUnderrun(void);
void perfProf_audioOverrun(void);
void perfProf_padRaw(int sx, int sy, int cx, int cy, unsigned int btns);
void perfProf_padRead(int control, unsigned int value);
void perfProf_cpuSample(void);
void perfProf_dirScan(int entries, unsigned int readDirUs, unsigned int headersUs);
void perfProf_menuFpsSample(float fps);
void perfProf_gameBegin(void);
void perfProf_clockStart(void);
void perfProf_gameEnd(int n, int total, unsigned int vis, const char* rom, const char* how);

#ifdef __cplusplus
}
#endif

#define PERF_NOW() gettime()
#define PERF_US(start) ((unsigned int)ticks_to_microsecs(gettime()-(start)))

#else /* !PERF_PROF */

#define perfProf_reset() ((void)0)
#define perfProf_pageBegin(numTiles, loadLimit) ((void)0)
#define perfProf_tileLoaded(index, wasReal, initUs, loadUs, flushUs) ((void)0)
#define perfProf_pageEnd(invalidateUs, totalUs) ((void)0)
#define perfProf_selfTest() ((void)0)
#define perfProf_mark(label) ((void)0)
#define perfProf_visSample(vis) ((void)0)
#define perfProf_fpsSample(fps) ((void)0)
#define perfProf_exceptionOccurred() ((void)0)
#define perfProf_cacheReset() ((void)0)
#define perfProf_drawBatch(verts) ((void)0)
#define perfProf_texStall() ((void)0)
#define perfProf_recompile() ((void)0)
#define perfProf_treeDepth(depth) ((void)0)
#define perfProf_limiterSleep(us) ((void)0)
#define perfProf_audioUnderrun() ((void)0)
#define perfProf_audioOverrun() ((void)0)
#define perfProf_padRaw(sx, sy, cx, cy, btns) ((void)0)
#define perfProf_padRead(control, value) ((void)0)
#define perfProf_cpuSample() ((void)0)
#define perfProf_dirScan(entries, readDirUs, headersUs) ((void)0)
#define perfProf_menuFpsSample(fps) ((void)0)
#define perfProf_gameBegin() ((void)0)
#define perfProf_clockStart() ((void)0)
#define perfProf_gameEnd(n, total, vis, rom, how) ((void)(how))
#define PERF_NOW() (0)
#define PERF_US(start) (0)

#endif /* PERF_PROF */

#endif /* PERF_PROF_H */
