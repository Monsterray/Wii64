#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../gc_memory/MEM2.h"
int main(void) {
    uintptr_t used = (uintptr_t)UNCLAIMED_LO - (uintptr_t)MEM2_LO;
    assert(used == MEM2_USED_SIZE);
    assert((uintptr_t)UNCLAIMED_LO <= (uintptr_t)MEM2_HI);
#ifdef WII64_HBC_AGENT
    /* HBC keeps its log target and crash report here across IOS reload. */
    assert((uintptr_t)TEXCACHE_LO >= 0x91800100);
    assert((uintptr_t)TLBLUT_HI <= 0x91800000);
#endif
    assert(!((uintptr_t)UNCLAIMED_LO & 31));
    printf("MEM2 reserved=%lu bytes (%.3f MiB), unclaimed=%lu bytes (%.3f MiB)\n",
           (unsigned long)used, (double)used / MB,
           (unsigned long)UNCLAIMED_SIZE, (double)UNCLAIMED_SIZE / MB);
}
