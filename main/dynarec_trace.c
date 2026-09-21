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
// Ring buffer over the last WINDOW dispatches, tracked for the first CAP
// dispatches total (cheap: just an array write) -- CAP comfortably covers
// Wrappers.c's DYNAREC_WATCHDOG_REPEAT_LIMIT so a future investigation can
// trace right up to where the watchdog would stop it. WINDOW keeps the
// on-screen text to what fits.
#define WINDOW 16
#define CAP 250000
static unsigned int history[WINDOW];

/* First version of this only sampled dispatches 0-7, and the LAST one drawn
   (PC=0xA40009AC, into what turned out to be a data table, not code) looked
   like the hang -- it was actually still running fine; the trace tool had
   just stopped updating the screen. Confirmed by widening the window: real
   execution continues dispatching (through at least #15) with the screen
   never changing again after that, while CPU stays busy -- i.e. an actual
   later hang, just further out than the original 8-sample cap could show.
   Moral: this tool can prove aliveness up to its last sample, never prove a
   hang -- only "stopped sampling here" vs "the display stopped changing
   after here," which needs a follow-up screenshot to tell apart.

   Second lesson, from CAP's first attempt at 2000: redrawing (2 blocking
   vsyncs) on EVERY dispatch up to CAP added ~66 seconds of pure trace
   overhead by itself, badly distorting how long boot "actually" takes to
   watch. Decimated below so CAP can be pushed much higher without the same
   cost: every dispatch is still recorded (cheap), but only drawn (the
   actually-expensive part) periodically. */
void dynarecTrace_dispatch(unsigned int pc) {
	if(!enabled) return;
	unsigned int n = dispatchCount++;
	if(n >= CAP) return;
	history[n % WINDOW] = pc;
	// Fine granularity while still inside IPL3/early boot (where a hang has
	// historically shown up within the first couple hundred dispatches),
	// coarser after that so a long run doesn't spend most of its time
	// redrawing instead of executing.
	if(n >= 256 && (n % 256) != 0) return;
	unsigned int shown = (n+1 < WINDOW) ? (n+1) : WINDOW;
	unsigned int first = (n+1 < WINDOW) ? 0 : (n+1-WINDOW);
	char text[WINDOW * 20];
	char* p = text;
	for(unsigned int i = 0; i < shown; i++) {
		unsigned int idx = first + i;
		p += sprintf(p, "#%u: PC=0x%08X\n", idx, history[idx % WINDOW]);
	}
	wii64_debugBreadcrumb(text);
}
