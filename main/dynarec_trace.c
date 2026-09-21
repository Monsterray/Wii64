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

void dynarecTrace_dispatch(unsigned int pc) {
	if(!enabled) return;
	unsigned int n = dispatchCount++;
	// The first several dispatches catch a hang on the very first blocks
	// (e.g. right at the N64 boot vector, 0xa4000040); every 4096th after
	// that catches a later hang without slowing normal play to a crawl --
	// each sample costs two real vsyncs (see wii64_debugBreadcrumb).
	if(n < 8 || (n & 0xFFF) == 0) {
		char text[64];
		sprintf(text, "dynarec #%u: PC=0x%08X", n, pc);
		wii64_debugBreadcrumb(text);
	}
}
