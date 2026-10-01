#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define MEM2_H
#define ROM_READAHEAD_SIZE 32768
static _Alignas(32) char cache[ROM_READAHEAD_SIZE];
#define ROM_READAHEAD_LO cache
#include "../vm/pagefile.c"
static unsigned char file[10*VM_PAGE_SIZE];
static _Alignas(32) unsigned char page[VM_PAGE_SIZE];
static int position, reads, writes, fail_seek, partial;
int ISFS_Seek(int fd, int offset, int mode) {
    (void)fd; assert(mode == SEEK_SET); position = offset;
    return fail_seek ? -1 : offset;
}
int ISFS_Read(int fd, void *dst, unsigned int size) {
    (void)fd; assert(position >= 0 && position + size <= sizeof(file));
    reads++; if (partial) size /= 2;
    memcpy(dst, file + position, size); return size;
}
int ISFS_Write(int fd, const void *src, unsigned int size) {
    (void)fd; assert(position >= 0 && position + size <= sizeof(file));
    writes++; if (partial) size /= 2;
    memcpy(file + position, src, size); return size;
}
static void read_page(unsigned int offset) {
    assert(pagefile_read(1, page, offset, sizeof(file)));
    assert(!memcmp(page, file + offset, sizeof(page)));
}
int main(void) {
    (void)cache;
    for (unsigned int i = 0; i < sizeof(file); i++) file[i] = (i*17 + i/VM_PAGE_SIZE) & 255;
    pagefile_cache_reset();
    for (int i = 0; i < 8; i++) read_page(i*VM_PAGE_SIZE);
    assert(reads == (VM_PAGE_READAHEAD ? 1 : 8));
    read_page(9*VM_PAGE_SIZE); /* bounded final 2-page window */
    read_page(0); read_page(6*VM_PAGE_SIZE);
    assert(!pagefile_read(1, page, sizeof(file), sizeof(file)));
    assert(!pagefile_read(1, page, 1, sizeof(file)));
    memset(page, 91, sizeof(page));
    assert(pagefile_write(1, page, 0, sizeof(page)) && writes == 1);
    read_page(0); assert(page[0] == 91);
    pagefile_cache_reset(); fail_seek = 1;
    assert(!pagefile_read(1, page, 0, sizeof(file))); fail_seek = 0;
    partial = 1; assert(!pagefile_read(1, page, 0, sizeof(file)));
    partial = 0; read_page(0); /* failed reads cannot poison the cache */
    partial = 1; assert(!pagefile_write(1, page, 0, sizeof(page)));
    partial = 0; read_page(0);
    memset(file, 42, sizeof(file)); pagefile_cache_reset(); read_page(0);
    puts("Pagefile byte identity, bounds, I/O failure and coherence checks passed");
}
