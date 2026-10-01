/* Exact NAND I/O. The cache contains already byte-swapped ROM bytes. */
#include <stdio.h>
#include <string.h>
#include <ogc/isfs.h>
#include "../gc_memory/MEM2.h"
#include "../main/perf_subsystem.h"
#include "pagefile.h"

#ifdef PERF_SUBSYSTEM_ENABLED
static struct pagefile_stats io_stats;
#define IO_COUNT(field, n) (io_stats.field += (n))
void pagefile_stats_reset(void) { memset(&io_stats, 0, sizeof(io_stats)); }
struct pagefile_stats pagefile_stats_read(void) { return io_stats; }
#else
#define IO_COUNT(field, n) ((void)0)
#endif

#if VM_PAGE_READAHEAD
static unsigned int cache_start, cache_size;
#endif

void pagefile_cache_reset(void)
{
#if VM_PAGE_READAHEAD
    cache_size = 0;
#endif
}

int pagefile_read(int fd, void *dst, unsigned int offset, unsigned int limit)
{
    unsigned int start = offset, size = VM_PAGE_SIZE;
    void *buffer = dst;
    if ((offset & (VM_PAGE_SIZE-1)) || offset > limit || limit-offset < VM_PAGE_SIZE)
        return 0;
#if VM_PAGE_READAHEAD
    if (cache_size && offset >= cache_start && offset-cache_start < cache_size) {
        IO_COUNT(cache_hits, 1);
        memcpy(dst, ROM_READAHEAD_LO + (offset-cache_start), VM_PAGE_SIZE);
        return 1;
    }
    pagefile_cache_reset();
    start = offset & ~(ROM_READAHEAD_SIZE-1);
    size = limit-start < ROM_READAHEAD_SIZE ? limit-start : ROM_READAHEAD_SIZE;
    buffer = ROM_READAHEAD_LO;
#endif
    unsigned long long timer = perfProf_subsystemBegin(PERF_SUB_VM_READ);
    IO_COUNT(reads, 1);
    int ok = ISFS_Seek(fd, start, SEEK_SET) == (int)start &&
             ISFS_Read(fd, buffer, size) == (int)size;
    perfProf_subsystemEnd(PERF_SUB_VM_READ, timer);
    if (!ok) { IO_COUNT(errors, 1); return 0; }
    IO_COUNT(read_bytes, size);
#if VM_PAGE_READAHEAD
    cache_start = start;
    cache_size = size;
    memcpy(dst, ROM_READAHEAD_LO + (offset-start), VM_PAGE_SIZE);
#endif
    return 1;
}

int pagefile_write(int fd, const void *src, unsigned int offset, unsigned int size)
{
    /* A failed/partial write must invalidate too; never serve stale bytes. */
    pagefile_cache_reset();
    unsigned long long timer = perfProf_subsystemBegin(PERF_SUB_VM_WRITE);
    IO_COUNT(writes, 1);
    int ok = ISFS_Seek(fd, offset, SEEK_SET) == (int)offset &&
             ISFS_Write(fd, src, size) == (int)size;
    perfProf_subsystemEnd(PERF_SUB_VM_WRITE, timer);
    IO_COUNT(write_bytes, ok ? size : 0);
    IO_COUNT(errors, !ok);
    return ok;
}
