/* perf_prof.c - see perf_prof.h */
#include "perf_prof.h"

#ifdef PERF_PROF
#include <stdio.h>
#include <string.h>

/* Opened and closed per write rather than held open: this runs interleaved
   with ROM/boxart file I/O elsewhere in the menu, and there's no guarantee
   about how many fds the FAT driver can hold open at once. */

void perfProf_reset(void)
{
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
void perfProf_mark(const char* label)
{
	FILE* f = fopen("sd:/wii64/perf.log", "a");
	if (!f) return;
	fprintf(f, "mark: %s\n", label);
	fclose(f);
}

#endif /* PERF_PROF */
