# Wii64/glN64 architecture map

## Runtime layers

| Layer | Key paths | Responsibility and likely cost boundary |
|---|---|---|
| Front end/platform | `main/`, `fileBrowser/`, `libgui/`, `Makefile.*` | ROM selection, configuration, input, video mode, platform startup, target-specific linking. |
| R4300 execution | `r4300/r4300.c`, `r4300/recomp.c`, `r4300/pure_interp.c`, `r4300/ppc/` | Dispatches emulated PC; chooses PPC dynarec, recompiler/interpreter paths, and exception/count handling. |
| PPC dynarec | `r4300/ppc/MIPS-to-PPC.c`, `Recompile.c`, `Register-Cache.c`, `FuncTree.c`, `Wrappers.c` | Translates blocks, maps GPR/FPR state, emits fast paths, enters/leaves compiled functions, and handles slow helpers. |
| Recompiled-code cache | `r4300/Recomp-Cache-Heap.c`, `Recomp-Cache.h`, `Invalid_Code.c` | Allocates code in MEM2/ARAM, maintains LRU metadata, patches links, invalidates code after writes. |
| Address translation/memory | `gc_memory/memory.c`, `tlb.c`, `TLB-Cache.c`, `dma.c` | Maps N64 virtual addresses, dispatches RDRAM/MMIO, handles TLB refill and PI/SI/DMA. |
| Timing/interrupts | `r4300/interupt.c`, `main/timers.c`, `gc_memory/memory.c` | Queues events and schedules VI, AI, SP, DP, and Count-related work. |
| ROM/virtual memory | `main/ROM-Cache.c`, `vm/wii_vm.c`, `gc_memory/MEM2.h`, `ARAM.h` | Loads/byteswaps ROMs; Wii uses a fixed MEM2 resident cache plus VM for ROMs larger than the cache; GC uses ARAM block caching. |
| RSP HLE/audio | `rsp_hle/`, `gc_audio/audio.c` | HLE graphics/audio tasks and threaded AESND buffering/callbacks. |
| glN64 GX renderer | `glN64_GX/` (`GBI.cpp`, `RSP.cpp`, `RDP.cpp`, `gSP.cpp`, `Textures.cpp`, `FrameBuffer.cpp`, `VI.cpp`) | Detects ucode, translates display lists, rasterizes through GX, manages textures/framebuffers, and presents VI output. |
| Rice GX renderer | `Rice_GX/` | Alternative GX renderer with its own parser, texture manager, framebuffer, TEV, and video paths. |

## Main execution flow

```text
ROM load / byte swap
  -> memory, VM/ARAM, RSP, audio, and renderer initialization
  -> r4300 go()
       -> dynarec(r4300.pc) or interpreter/recompiler dispatch
            -> translated block / PPC function
                 -> register-cache fast path
                 -> fast memory or dyna_mem helper
                 -> MMIO, DMA, TLB, interrupt, or exception
       -> RSP task dispatch
            -> graphics GBI/RDP/texture/framebuffer work
            -> audio HLE / AESND queue
  -> VI timing and GX presentation
```

The hot-path boundary is not a single function: CPU block dispatch, compiled-code entry/trampolines, virtual-address translation, ROM access, GX display-list work, and framebuffer/VI operations can all dominate depending on the game.

## Platform memory shape

On Wii, `gc_memory/MEM2.h` reserves fixed MEM2 regions for the ROM cache, TLB LUT, textures, invalid-code data, code/recomp metadata, XFBs, and scratch. `ROMCACHE_SIZE` is currently 16 MiB. `ROM-Cache.c` calls `VM_Init(rom_length, ROMCACHE_SIZE)` when a ROM exceeds that resident cache. On GameCube, `ARAM.h` and the `HW_DOL` paths use ARAM-backed block loading and a different TLB-cache configuration.

This makes memory-layout changes hardware-sensitive: changing the ROM cache can collide with code, texture, VM, or XFB reservations, and the VM/TLB invalidation path must be tested on each actual MEM2 configuration.

## Compile-time switches worth recording in every run

| Switch | Effect |
|---|---|
| `PPC_DYNAREC` | Enables the PPC dynamic recompiler path. |
| `FASTMEM` / `FASTMEM_HOT_NOFLUSH` | Enables direct/optimized memory paths and their associated invalidation assumptions. |
| `USE_RECOMP_CACHE` | Uses the recompiled-code cache and linking machinery. |
| `PROFILE` | Enables existing section timers and debug reporting. Adds measurement overhead. |
| `GLN64_GX` / Rice target defines | Selects renderer-specific GX code. |
| `HW_RVL`, `HW_DOL`, `USE_EXPANSION`, `RVL_LIBWIIDRC` | Selects Wii, GameCube, expansion-memory, and Wii VC behavior. |

## Existing observability

`r4300/r4300.h` defines 14 profile sections: graphics, audio, compiler, idle, TLB, floating point, interpreter, trampoline, functions, link, unlink, dynamic memory, ROM read, and blocks. `r4300/profile.c` emits once-per-second section percentages when `PROFILE` is enabled. glN64 also displays VI/s and DL/s from `main/timers.c`.

Missing reusable counters include frame/VI time distributions, blocks generated/evicted, code-cache occupancy, fast-memory hit/slow-path counts, TLB hit/miss counts, ROM-cache/VM faults, audio underruns, and MEM1/MEM2 high-water marks. The first implementation experiment should add only the smallest disabled-by-default measurement slice needed to choose among candidates.
