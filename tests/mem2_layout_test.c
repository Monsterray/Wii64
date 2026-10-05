#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../gc_memory/MEM2.h"
int main(void) {
    struct region { uintptr_t lo; unsigned size; } regions[] = {
        {(uintptr_t)ROMCACHE_LO, ROMCACHE_SIZE},
        {(uintptr_t)TLBLUT_LO, TLBLUT_SIZE},
        {(uintptr_t)TLBLUT_HI, HBC_KEEP_SIZE},
        {(uintptr_t)TEXCACHE_LO, TEXCACHE_SIZE},
        {(uintptr_t)TEX_THUMB_LO, TEX_THUMB_SIZE},
        {(uintptr_t)INVCODE_LO, INVCODE_SIZE},
        {(uintptr_t)FONT_LO, FONT_SIZE},
        {(uintptr_t)FLASHRAM_LO, FLASHRAM_SIZE},
        {(uintptr_t)SRAM_LO, SRAM_SIZE},
        {(uintptr_t)MEMPACK_LO, MEMPACK_SIZE},
        {(uintptr_t)BLOCKS_LO, BLOCKS_SIZE},
        {(uintptr_t)RECOMPMETA_LO, RECOMPMETA_SIZE},
        {(uintptr_t)BOXART_ICON_LO, BOXART_ICON_SIZE},
        {(uintptr_t)XFB0_LO, XFB_SIZE},
        {(uintptr_t)XFB1_LO, XFB_SIZE},
        {(uintptr_t)RECOMP_CACHE_HEAP_SCRATCH_LO, RECOMP_CACHE_HEAP_SCRATCH_SIZE},
        {(uintptr_t)ROM_READAHEAD_LO, ROM_READAHEAD_SIZE},
        {(uintptr_t)DEPTHCOPY_BUF_LO, DEPTHCOPY_BUF_SIZE},
        {(uintptr_t)DEPTHCOPY_LUT_LO, DEPTHCOPY_LUT_SIZE},
    };
    uintptr_t end = (uintptr_t)MEM2_LO;
    for (unsigned i = 0; i < sizeof(regions) / sizeof(regions[0]); i++) {
        assert(regions[i].lo == end); /* no overlaps or unaccounted holes */
        assert(!(regions[i].lo & 31) && !(regions[i].size & 31));
        end += regions[i].size;
        assert(end <= (uintptr_t)MEM2_HI);
    }
    assert(end == (uintptr_t)UNCLAIMED_LO);
    assert(XFB_SIZE >= 640 * 576 * 2); /* retain PAL capacity for both buffers */
    assert(ROM_READAHEAD_SIZE == 32 * KB);
    assert(BOXART_ICON_SIZE >= 768 * KB);
#ifdef EXPECT_BOXART_SIZE
    assert(BOXART_ICON_SIZE == EXPECT_BOXART_SIZE);
#endif
    assert((uintptr_t)BOXART_ICON_LO == 0x930AD800 + HBC_KEEP_SIZE);
    assert((uintptr_t)XFB0_LO == (uintptr_t)BOXART_ICON_LO + BOXART_ICON_SIZE);
    uintptr_t used = (uintptr_t)UNCLAIMED_LO - (uintptr_t)MEM2_LO;
    assert(used == MEM2_USED_SIZE);
    assert((uintptr_t)UNCLAIMED_LO <= (uintptr_t)MEM2_HI);
#ifdef WII64_HBC_AGENT
    /* HBC keeps its log target, crash report and last-output block (SDK 1.9:
       4140 bytes at 0x91800100) here across IOS reload. */
    assert((uintptr_t)TEXCACHE_LO >= 0x91800100 + 4140);
    assert((uintptr_t)TLBLUT_HI <= 0x91800000);
#endif
    assert(!((uintptr_t)UNCLAIMED_LO & 31));
    printf("MEM2 reserved=%lu bytes (%.3f MiB), unclaimed=%lu bytes (%.3f MiB)\n",
           (unsigned long)used, (double)used / MB,
           (unsigned long)UNCLAIMED_SIZE, (double)UNCLAIMED_SIZE / MB);
}
