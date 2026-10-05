#ifndef WII64_PAGEFILE_H
#define WII64_PAGEFILE_H

#ifndef VM_PAGE_READAHEAD
#define VM_PAGE_READAHEAD 1 /* Reduce NAND waits during gameplay. */
#endif
#define VM_PAGE_SIZE 4096
#include "../main/perf_subsystem.h"
struct pagefile_stats {
    unsigned int reads, cache_hits, read_bytes, writes, write_bytes, errors;
    unsigned int faults, zero_fills;
};
#if defined(PERF_SUBSYSTEM_ENABLED) || (defined(PERF_PROF) && defined(PERF_MEMORY))
#define PERF_PAGEFILE_STATS
void pagefile_stats_reset(void);
struct pagefile_stats pagefile_stats_read(void);
void pagefile_note_fault(int zero_fill);
#else
#define pagefile_stats_reset() ((void)0)
#define pagefile_note_fault(zero_fill) ((void)0)
#endif
void pagefile_cache_reset(void);
int pagefile_read(int fd, void *dst, unsigned int offset, unsigned int limit);
int pagefile_write(int fd, const void *src, unsigned int offset, unsigned int size);
#endif
