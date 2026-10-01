/* Exercise the Wii loader itself, with a bounded mock file and RAM backing. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define R4300_H
#define DEBUG_H
#define MEM2_H
#define HW_RVL
#define KB 1024
#define ROMCACHE_SIZE (16*1024*1024)
static char backing[ROMCACHE_SIZE + 32768];
#define ROMCACHE_LO backing
typedef uint8_t u8;
typedef int BOOL;
static int rom_length;
#define BYTE_SWAP_BAD -1
static int swap_calls, reads, closes, vm_closes, fail_vm, available, short_read, fail_flush;
int init_byte_swap(unsigned int magic) { return magic == 0x80371240 ? 0 : -1; }
void byte_swap(char *p, unsigned int n, int type) {
    (void)p; (void)type; assert(n && !(n & 3)); swap_calls++;
}
#include "../main/ROM-Cache.c"
static int read_file(fileBrowser_file *f, void *p, unsigned int n) {
    assert(n <= f->size - f->offset); reads++;
    if (available <= 0) return available;
    if (short_read && n > 8) n = 8;
    if (n > (unsigned int)available) n = available;
    memset(p, 0, n);
    if (!f->offset && n >= 4) *(unsigned int *)p = 0x80371240;
    f->offset += n; available -= n; return n;
}
static int seek_file(fileBrowser_file *f, unsigned int off, unsigned int how) {
    (void)how; f->offset = off; return 0;
}
static int close_file(fileBrowser_file *f) { (void)f; closes++; return 0; }
int (*romFile_readFile)(fileBrowser_file *, void *, unsigned int) = read_file;
int (*romFile_seekFile)(fileBrowser_file *, unsigned int, unsigned int) = seek_file;
int (*romFile_deinit)(fileBrowser_file *) = close_file;
void *VM_Init(size_t v, size_t m) { (void)v; (void)m; return fail_vm ? NULL : backing; }
void VM_Deinit(void) { vm_closes++; }
int VM_Flush(void) { return !fail_flush; }
void LoadingBar_showBar(float p, const char *s) { (void)s; assert(p >= 0 && p <= 1); }
static int load(unsigned int size, int bytes) {
    fileBrowser_file f = {0}; f.size = size; rom_length = size; available = bytes;
    swap_calls = reads = 0; ROMCache_init(&f); return ROMCache_load(&f);
}
int main(void) {
    assert(load(64, 0) == ROM_CACHE_ERROR_READ); /* stale valid header must not pass */
    assert(!swap_calls);
    assert(load(64, 32) == ROM_CACHE_ERROR_READ);
    assert(load(64, -1) == ROM_CACHE_ERROR_READ);
    assert(load(2, 2) == ROM_CACHE_INVALID_ROM);
    assert(load(65, 65) == ROM_CACHE_INVALID_ROM);
    assert(load(64, 64) == 0 && reads == 1);
    short_read = 1;
    assert(load(64, 64) == 0 && reads == 8); /* aligned partial reads are legal */
    short_read = 0;
    assert(load(32772, 32772) == 0 && reads == 2);
    fail_vm = 1;
    assert(load(ROMCACHE_SIZE + 4, ROMCACHE_SIZE + 4) == ROM_CACHE_ERROR_READ);
    ROMCache_deinit(); ROMCache_deinit(); assert(!vm_closes);
    fail_vm = 0;
#if VM_ROM_PREFLUSH
    fail_flush = 1;
    assert(load(ROMCACHE_SIZE + 4, ROMCACHE_SIZE + 4) == ROM_CACHE_ERROR_READ);
    ROMCache_deinit(); vm_closes = 0; fail_flush = 0;
#endif
    assert(load(ROMCACHE_SIZE + 4, ROMCACHE_SIZE + 4) == 0);
    ROMCache_deinit(); ROMCache_deinit(); assert(vm_closes == 1);
    puts("ROM loader regression checks passed");
}
