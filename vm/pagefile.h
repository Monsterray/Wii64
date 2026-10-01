#ifndef WII64_PAGEFILE_H
#define WII64_PAGEFILE_H

#ifndef VM_PAGE_READAHEAD
#define VM_PAGE_READAHEAD 0 /* Enable only in measured candidate builds. */
#endif
#define VM_PAGE_SIZE 4096
#include "../main/perf_subsystem.h"
struct pagefile_stats {
    unsigned int reads, cache_hits, read_bytes, writes, write_bytes, errors;
};
#ifdef PERF_SUBSYSTEM_ENABLED
void pagefile_stats_reset(void);
struct pagefile_stats pagefile_stats_read(void);
#else
#define pagefile_stats_reset() ((void)0)
#endif
void pagefile_cache_reset(void);
int pagefile_read(int fd, void *dst, unsigned int offset, unsigned int limit);
int pagefile_write(int fd, const void *src, unsigned int offset, unsigned int size);
#endif
