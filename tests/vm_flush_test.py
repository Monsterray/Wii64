"""Run the actual startup flush/formatting code with bounded host fixtures."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
vm = (root / 'vm/wii_vm.c').read_text()
fault = vm[vm.index('int vm_dsi_handler('):]
assert fault.index('if (dar<(u32)VM_Base || dar>=0x80000000)') < fault.index('pagefile_note_fault(')
assert fault.index('if (!vm_initialized)') < fault.index('pagefile_note_fault(')
assert fault.index('virt_index =') < fault.index('pagefile_note_fault(') < fault.index('phys_index = locate_oldest()')
flush = vm[vm.index('int VM_Flush('):vm.index('void VM_Deinit(')]
ui = (root / 'libgui/LoadingBar.cpp').read_text()
formatting = ui[ui.index('\tpercentComplete ='):ui.index('\n\tmenu::Gui::getInstance().draw();')]
fixture = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef unsigned short u16;
typedef struct { unsigned valid, locked, dirty, pte_index, page_index; } p_map;
typedef struct { unsigned C; } PTE;
typedef struct { unsigned committed; } vm_map;
#define PAGE_SIZE 4096
static p_map phys_map[64];
static PTE HTABORG[64];
static vm_map virt_map[64];
static char VM_Base[64][PAGE_SIZE], MEM_Base[64][PAGE_SIZE];
static int vm_initialized, vm_mutex, pagefile_fd, locked, writes, fail_write, callbacks;
static unsigned pmap_max;
static float last_progress;
static void LWP_MutexLock(int mutex) { (void)mutex; assert(!locked); locked = 1; }
static void LWP_MutexUnlock(int mutex) { (void)mutex; assert(locked); locked = 0; }
static void tlbie(void *page) { (void)page; assert(locked); }
static int pagefile_write(int fd, void *buffer, unsigned offset, unsigned size) {
    (void)fd; assert(locked && size && size <= 16*PAGE_SIZE);
    unsigned first = offset / PAGE_SIZE, count = size / PAGE_SIZE;
    assert(buffer == MEM_Base[first]);
    for (unsigned i = first; i < first+count; ++i)
        assert(!virt_map[i].committed && (phys_map[i].dirty || HTABORG[i].C));
    return ++writes != fail_write;
}
static void progress(float fraction) {
    assert(!locked); /* A display callback can acquire VM's mutex safely. */
    LWP_MutexLock(vm_mutex); LWP_MutexUnlock(vm_mutex);
    assert(fraction >= last_progress && fraction >= 0 && fraction <= 1);
    last_progress = fraction; ++callbacks;
    if (fraction == 1)
        for (unsigned i = 0; i < pmap_max; ++i)
            if (phys_map[i].valid && !phys_map[i].locked)
                assert(!phys_map[i].dirty && !HTABORG[i].C);
}
static void reset(unsigned pages) {
    memset(phys_map, 0, sizeof(phys_map)); memset(virt_map, 0, sizeof(virt_map));
    memset(HTABORG, 0, sizeof(HTABORG));
    pmap_max = pages; vm_initialized = 1; writes = fail_write = callbacks = locked = 0;
    last_progress = 0;
    for (unsigned i = 0; i < pages; ++i) {
        phys_map[i] = (p_map){1, 0, 1, i, i}; HTABORG[i].C = 1;
    }
}
#define LOADINGBAR_TEXT_WIDTH 100
static char loadingBarText[LOADINGBAR_TEXT_WIDTH];
static float percentComplete;
static void format(float percent, const char *string) {
FORMAT
}
FLUSH
int main(void) {
    reset(64);
    assert(VM_Flush(progress) && writes == 4 && callbacks == 5 && last_progress == 1);
    assert(!locked);
    reset(64); fail_write = 2;
    assert(!VM_Flush(progress) && writes == 2 && last_progress == .25f && !locked);
    for (unsigned i = 0; i < 64; ++i) {
        assert(virt_map[i].committed == (i < 16));
        assert(phys_map[i].dirty == (i >= 16));
        assert(HTABORG[i].C == (i >= 16));
    }
    reset(64); assert(VM_Flush(NULL) && writes == 4 && !callbacks && !locked);
    reset(3); phys_map[0].valid = 0; phys_map[1].locked = 1;
    phys_map[2].dirty = 0; HTABORG[2].C = 0;
    assert(VM_Flush(progress) && !writes && last_progress == 1);
    assert(phys_map[0].dirty && phys_map[1].dirty);
    reset(1); assert(VM_Flush(progress) && writes == 1 && callbacks == 2);
    reset(1); vm_initialized = 0; assert(!VM_Flush(progress) && !callbacks && !locked);
    format(.25f, "Preparing ROM for gameplay");
    assert(!strcmp(loadingBarText, "Preparing ROM for gameplay (25%)"));
    format(-1, "Test"); assert(percentComplete == 0 && strstr(loadingBarText, "(0%)"));
    format(2, "Test"); assert(percentComplete == 1 && strstr(loadingBarText, "(100%)"));
    format(NAN, "Test"); assert(percentComplete == 0);
    char long_text[1024]; memset(long_text, 'x', sizeof(long_text)-1); long_text[1023] = 0;
    format(1, long_text); assert(strlen(loadingBarText) < sizeof(loadingBarText));
    assert(strstr(loadingBarText, "(100%)"));
    puts("Startup flush progress: unlocked callbacks, write failure, clean bits and UI bounds PASS");
}
'''.replace('FORMAT', formatting).replace('FLUSH', flush)
with tempfile.TemporaryDirectory(prefix='wii64-flush-') as directory:
    source, binary = Path(directory) / 'test.c', Path(directory) / 'test'
    source.write_text(fixture)
    subprocess.run(shlex.split(os.environ.get('CC', 'cc')) + [
        '-std=c11', '-O1', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined',
        '-fno-sanitize-recover=all', str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
