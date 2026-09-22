/* perf_prof.h
   Performance probes, modeled on WiiStation's perf-probe pattern
   (Gamecube/perf_prof.c there): compiled in only when PERF_PROF is defined
   (pass -DPERF_PROF to make, or use .dev/build_profiling.sh), a no-op
   otherwise so release builds pay nothing. Writes sd:/wii64/perf.log,
   truncated once per boot and appended to on every ROM-browser page fill
   and every ~500ms during actual gameplay (visSample/fpsSample, hooked
   into main/timers.c's new_vi()/new_frame() -- the same VI-interrupt-rate
   and completed-display-list-rate numbers the in-game "Show FPS" overlay
   displays) -- read it back with scripts/sdimage_read.py + compare with
   scripts/perf_compare.py (see baselines/README.md), no live input
   required to get the numbers back out.
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
void perfProf_cpuSample(void);
void perfProf_dirScan(int entries, unsigned int readDirUs, unsigned int headersUs);
void perfProf_menuFpsSample(float fps);

#ifdef __cplusplus
}
#endif

#define PERF_NOW() gettime()
#define PERF_US(start) ((unsigned int)ticks_to_microsecs(gettime()-(start)))

#else /* !PERF_PROF */

#define perfProf_reset()
#define perfProf_pageBegin(numTiles, loadLimit)
#define perfProf_tileLoaded(index, wasReal, initUs, loadUs, flushUs)
#define perfProf_pageEnd(invalidateUs, totalUs)
#define perfProf_selfTest()
#define perfProf_mark(label)
#define perfProf_visSample(vis)
#define perfProf_fpsSample(fps)
#define perfProf_exceptionOccurred()
#define perfProf_cacheReset()
#define perfProf_drawBatch(verts)
#define perfProf_texStall()
#define perfProf_cpuSample()
#define perfProf_dirScan(entries, readDirUs, headersUs)
#define perfProf_menuFpsSample(fps)
#define PERF_NOW() (0)
#define PERF_US(start) (0)

#endif /* PERF_PROF */

#endif /* PERF_PROF_H */
