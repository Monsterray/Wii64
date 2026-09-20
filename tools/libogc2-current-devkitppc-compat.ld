/*
 * libogc2 (Extrems' fork) was built against devkitPPC r41-2, whose rvl.ld computed
 * __stack_addr/__gxregs/etc. directly from the end of .bss. Current devkitPPC's
 * rvl.ld/libogc_common.ld no longer define these (upstream libogc now sets up its
 * stack/thread bookkeeping without them), so libogc2's prebuilt .a files are left
 * with undefined references to them. This fragment restores them, using the same
 * formulas r41-2's rvl.ld used, reading .bss's real address/size directly (its
 * __bss_end equivalent is a PROVIDE_HIDDEN placeholder in the included script,
 * which is not resolved yet when this second -T script runs -- ADDR/SIZEOF read
 * the section table directly instead, which is available by this point).
 *
 * Deliberately left alone: __Arena1Lo/Hi, __Arena2Lo/Hi, __ipcbufferLo/Hi -- the
 * current rvl.ld/libogc_common.ld already provide those, and libogc2's compiled
 * code just reads whatever value the linker supplies for them.
 *
 * Pass alongside the default script: -Wl,-T,tools/libogc2-current-devkitppc-compat.ld
 *
 * STATUS: this closes the undefined-symbol gap (verified), but a *second*, deeper
 * incompatibility remains -- the current binutils' ld rejects prebuilt libwiiuse.a's
 * .sbss.<symbol> layout for R_PPC_EMB_SDA21 relocations ("target ... is in the wrong
 * output section"), independent of this script's contents (reproduces identically
 * with or without it, once this file gets the link past the undefined-symbol stage).
 * That needs libwiiuse rebuilt against the current toolchain to fix; a linker script
 * can't relocate an already-compiled object's small-data relocations back into range.
 * Until libwiiuse (and, untested beyond this point, potentially other libogc2
 * components) are rebuilt, use the pinned devkitPPC-r41-2 toolchain documented in
 * the README instead -- it's the one actually verified end-to-end (build + boot).
 */
__wii64_bss_end = ADDR(.bss) + SIZEOF(.bss);

__stack_addr = (__wii64_bss_end + 0x20000 + 7) & (-8);
__stack_end = __wii64_bss_end;
__intrstack_addr = (__stack_addr + 0x4000);
__intrstack_end = (__stack_addr);
/* 0x81800000 mirrors current rvl.ld's own literal for __Arena1Hi (PROVIDE_HIDDEN,
   not readable here yet -- see note above); keep in sync if that literal changes. */
__gxregs = (0x81800000 + 31) & (-32);
__end__ = __wii64_bss_end; /* newlib sbrk() heap-end marker; current rvl.ld drops this too */
PROVIDE(end = __wii64_bss_end);
