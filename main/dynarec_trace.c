/**
 * Wii64 - dynarec_trace.c
 * See dynarec_trace.h for what this is and why.
**/

#include <stdio.h>
#include "dynarec_trace.h"

extern void wii64_debugBreadcrumb(const char* text); // libgui/GraphicsGX.cpp

static int enabled = 0;
static unsigned int dispatchCount = 0;

void dynarecTrace_setEnabled(int e) {
	enabled = e;
}

/* Two failure modes found (and fixed) by actually testing this against a
   ROM known to boot cleanly (Super Mario 64), not just the hanging one:
     1. An early version resampled every 4096th dispatch by count. A real
        running game dispatches (small) blocks fast enough that "every
        4096th" fired many times per second, and each sample costs two
        blocking vsyncs -- Mario ended up LOOKING exactly like the Banjo
        hang purely from trace overhead. Switching that to a wall-clock
        throttle (at most once a second) fixed the slowdown...
     2. ...but a periodic resample is unsafe on correctness grounds too, not
        just slow: wii64_debugBreadcrumb draws with raw GX calls that assume
        nothing else owns the pipeline, which only holds during the cold
        boot window before the graphics plugin's own renderer has touched
        it (the same assumption Graphics::setInGameVMode's near-identical
        debug print already relies on, gated to r4300.pc's boot vector).
        Firing it again mid-game clobbered the plugin's GX state and left
        the screen a solid white rectangle instead of the game.
   So: sample only the first handful of dispatches, all still inside that
   same safe cold-boot window, and never again after that. Enough to catch
   a hang on/near the very first block (Banjo's case); not a general-purpose
   "is it still alive" probe -- perf_prof.h's marks are the right tool for
   sampling well past boot. */
void dynarecTrace_dispatch(unsigned int pc) {
	if(!enabled) return;
	unsigned int n = dispatchCount++;
	if(n < 8) {
		char text[64];
		sprintf(text, "dynarec #%u: PC=0x%08X", n, pc);
		wii64_debugBreadcrumb(text);
	}
}
