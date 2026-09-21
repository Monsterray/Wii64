/**
 * Wii64 - dynarec_trace.h
 *
 * Debugging tool for JIT hangs like the Banjo-Kazooie freeze: the dynarec
 * dispatch loop in r4300/ppc/Wrappers.c calls dynarecTrace_dispatch() with
 * the address it is ABOUT to execute, before the (potentially
 * never-returning) native call into the recompiled block. Off by default --
 * enabled by sd:/wii64/diag.cfg's "dynarec_trace=1" via
 * dynarecTrace_setEnabled(), called from apply_diag_automation() -- because
 * it burns real vsyncs drawing each sampled PC straight to the screen, so
 * even a hang on the very first dispatched block leaves that PC visible in
 * the last frame rendered (screenshottable via PrintWindow even while
 * Dolphin reports the process as hung/unresponsive to a close request).
 *
 * Not gated behind a build flag like perf_prof.h's PERF_PROF -- this is
 * meant to be available in any build without a rebuild, same as diag.cfg's
 * other autoboot_rom/autonav knobs.
**/

#ifndef DYNAREC_TRACE_H
#define DYNAREC_TRACE_H

#ifdef __cplusplus
extern "C" {
#endif

void dynarecTrace_setEnabled(int enabled);

/* pc is the address about to be dispatched, sampled BEFORE the native call
   that might never return. Cheap no-op when disabled (the common case). */
void dynarecTrace_dispatch(unsigned int pc);

#ifdef __cplusplus
}
#endif

#endif
