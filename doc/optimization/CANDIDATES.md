# Optimization candidate matrix

This is a hypothesis matrix, not an approval to port patches. Every candidate needs a release/profile A-B comparison on real hardware and a correctness pass. `P0` means high expected value or high leverage for choosing the next experiment; it does not mean safe to merge or that it precedes measurement. The implementation order below is intentionally measurement-first.

## Ranked candidates

| Rank | Candidate | Local evidence / upstream anchor | Expected value | Risk and portability | Smallest useful experiment |
|---|---|---|---|---|---|
| P0 | Replace the fresh-block `DCFlushRange` with the Not64 code-cache sequence (`DCStoreRange`, then `ICInvalidateRange`) | Wii64 `r4300/ppc/Recompile.c:293-296`; Not64 `deff3be31e666900f50615d3efbe8db7a3826d27` explicitly says it avoided flushing the recompiler’s L2 cache. | Potentially reduces PPC code-generation/link overhead in compile-heavy scenes; benefits CPU-heavy games and cache churn. | Low-to-medium risk, but exact libogc2 cache-coherency semantics must be verified. Keep link/unlink sites separate until measured. | Profile compile/link/block time, code-cache events, and VI/s; change only the fresh-block site; compare generated-code correctness and three runs per game. |
| P0 | Size the Wii ROM cache from physical/simulated MEM2 capacity | Wii64 fixes `ROMCACHE_SIZE` at 16 MiB in `gc_memory/MEM2.h`; Not64 `ed610be8d90c7a17029cee77c234ceea51b92b04` adds hardware-aware 48/64/128/192 MiB choices. | Fewer ROM VM/page faults for large ROMs; likely most valuable for large-ROM titles, not a universal FPS win. | Hardware-specific; must re-audit MEM2 reservations, VM mapping, TLB/LUT, and Wii/Wii U variants. | Add counters for ROM/VM faults and ROM-read time first; then test one conservative size choice per detected memory class. |
| P1 | Fuse unaligned load/store pairs in the PPC generator | Wii64 `Wrappers.h:39-45` has generic `LWL/LWR/LDL/LDR` and `SWL/SWR/SDL/SDR` types but no `ULW/USW/ULD/USD`; Not64 `c73cf02` adds specialized macros. | Could reduce helper calls and instruction count in access-heavy CPU workloads. | Large upstream diff; medium/high correctness risk around endianness, page boundaries, sign extension, and delay/exception behavior. | Port only one pair (for example ULW), compare instruction results against interpreter, then measure; do not import the full commit. |
| P1 | Specialize 32-bit logical/branch operands and register-cache handling | Not64 `c73cf02` includes 32-bit operand specializations; Wii64’s generator/cache are `MIPS-to-PPC.c` and `Register-Cache.c`. | May lower PPC instruction count and register spills in common 32-bit N64 code. | Medium risk in sign/zero extension and constant propagation; requires generated-code differential tests. | Instrument emitted instruction/block size and compare one opcode family at a time. |
| P1 | Compare TLB-cache layout/backends | Wii64 has a tiny two-stage cache in `gc_memory/TLB-Cache.c`; history `ede532f` reverted a Wii LUT consolidation after a Conker slowdown. Not64 has a different hash/cache implementation. | Possible benefit on TLB-heavy titles. | High sensitivity to collision behavior, invalidation, MEM2 layout, and game-specific access patterns; wholesale port is unsuitable. | Count L1/L2/slow TLB paths and time `virtual_to_physical_address`; test Conker plus one non-TLB-heavy control. |
| P1 | Verify and micro-test recompiler heap parent arithmetic | `r4300/Recomp-Cache-Heap.c:49-51` defines two binary children using `2*i+1/2*i+2` but defines the parent as `(i-1)>>2`; this static inconsistency needs confirmation before changing behavior. | Could affect LRU ordering and eviction frequency if confirmed as an error. | Correctness-first investigation; changing cache eviction is behavior-adjacent and needs churn tests. | Add a host-only or target debug invariant for parent/child round trips; no production change in this session. |
| P2 | Optimize glN64 framebuffer clear/presentation path | Not64 `deff3be31e666900f50615d3efbe8db7a3826d27` also optimized framebuffer clearing; Wii64 `glN64_GX/VI.cpp` uses `GX_CopyDisp` and `GX_DrawDone` in `VI_GX_clearEFB`. | Possible GPU/VI win in framebuffer-heavy games. | GX synchronization and visual correctness risk; depends on VI mode and framebuffer emulation. | Measure VI/clear time and visual CRCs/screenshots on OoT, MM, Conker, and DK64 before touching GX ordering. |

## Already present or deliberately not selected

| Area | Finding |
|---|---|
| Fast memory / linking / register cache | Wii64 already has these mechanisms and prior correctness fixes. Re-importing their ancestry is not a first-session task. |
| Full Mupen64Plus core/dynarec port | Reject as a project direction: current Mupen64Plus is portable x86/ARM-oriented and has no PPC/Wii target. Use it for concepts and profiling references only. |
| Full GLideN64 renderer port | Reject as a first-session direction: GLideN64 is desktop/OpenGL-oriented. Prefer selective, license-reviewed behavioral backports only when a measured GX problem maps cleanly. |
| FIX94 fork wholesale merge | Reject as an optimization plan: its value is historical Wii U/GX compatibility and boot fixes, not a current performance baseline. |
| libogc2 migration | Already selected by the Wii64 makefiles; first verify the installed version/toolchain rather than planning a second migration. |

## Recommended order

1. Add/enable the minimum counters described in `BENCHMARKS.md`.
2. Run the fresh-block cache-coherency experiment only.
3. If ROM/VM faults dominate, test dynamic ROM-cache sizing; otherwise test one unaligned pair or one TLB backend change.
4. Keep each patch isolated, with a source revision and license note in the experiment record.
