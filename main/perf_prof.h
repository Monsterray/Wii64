/* perf_prof.h
   Menu-side performance probes, modeled on WiiStation's perf-probe pattern
   (Gamecube/perf_prof.c there): compiled in only when PERF_PROF is defined
   (pass -DPERF_PROF to make), a no-op otherwise so release builds pay
   nothing. Writes sd:/wii64/perf.log, truncated once per boot and appended
   to on every ROM-browser page fill -- read it back the same offline way
   WiiStation's scripts/sdimage_read.py reads perf.log off its SD image, no
   live input required to get the numbers back out.
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
#define PERF_NOW() (0)
#define PERF_US(start) (0)

#endif /* PERF_PROF */

#endif /* PERF_PROF_H */
