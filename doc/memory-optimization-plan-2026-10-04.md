# Memory layout and usage plan

Planning baseline: Wii64 `83ea846e997795acd76227a283bcb63f06315b58`,
master, version 1.6.13. No emulator or layout change is made by this plan.
Use measured capacity and workload costs, not a rule that one RAM bank is
always faster. Keep upstream mergeability and separate Wii, vWii and GameCube.

## Execution status

The measured implementation pass is complete in version 1.6.15; see the
[ownership, full-library census and candidate decisions](memory-results-2026-10-04.md).

| Phase | Outcome |
|---|---|
| 1. Census | All 18 owned ROMs on Wii and Dolphin reconcile; a dynarec map overrun found by the census is fixed. Census timing remains provisional after its strict audio gate failed. |
| 2. Bounded capacity / failures | Boxart is 768 KiB; two native old/new/old trials, allocation-failure tests, layouts, HOME and Rice checks passed. 256 KiB recovered, no FPS claim. |
| 3. Traffic | Deterministic guest counters and a lower-rate PC sampler calibrated; existing conversion/copy reductions verified. No additional redundant copy is proved. |
| 4. Placement / budgets | Evaluated, no change selected: short-scene peaks and PC samples cannot establish long-game cache misses or safe pool bounds. |
| 5. Structural savings | Not selected: no measured capacity failure justifies additional lookup work or changing global coverage. |
| 6. LC | Not selected: no eligible kernel has measured end-to-end/cache-stall evidence; retain normal L1 and cached buffers. |

The unselected conditional experiments remain future work, not implemented
features. Native PAL output and vWii coverage remain separate hardware gaps.

## Sources and current limits

The reference is WiiXplorer-NG
[`59f75f4ea770d9c4d1382eb1691e27e766893293`, MEMORY.md](https://github.com/Monsterray/wiixplorer-ng/blob/59f75f4ea770d9c4d1382eb1691e27e766893293/MEMORY.md).
Its v0.1.5 native run verified 144 operations on one Wii. It is evidence for
those access patterns, not a prediction of Wii64's speed. Its allocator and
debug build differ from Wii64's.

The current [fixed map](../gc_memory/MEM2.h) reserves the following on Wii:

| MEM2 region | Reservation | Initial policy |
|---|---:|---|
| ROM backing cache | 16 MiB | Preserve paging geometry |
| Read/write TLB lookup tables | 8 MiB | Preserve full guest address coverage |
| Texture heap | 15 MiB | Measure peak, fragmentation and evictions |
| Block pointer table | 4 MiB | Measure live entries before changing representation |
| Recompilation metadata heap | 4 MiB | Measure allocated bytes and eviction pressure |
| Invalid-code bytes | 1 MiB | Preserve aliases and invalidation semantics |
| Boxart image heap | 1 MiB | Check its bounded allocation requirement first |
| Two XFBs | 1,474,560 bytes | Keep VI/GX/HBC ownership and PAL capacity |
| Thumbnail | 153,600 bytes | Keep pause/menu consumers |
| Font | 256 KiB | Check lifetime before any reuse |
| Flash/SRAM/mempacks | 288 KiB combined | Keep save persistence |
| Recompilation scratch / ROM read-ahead | 64 / 32 KiB | Keep separate from live code and PTEs |
| HBC persistent records, agent builds | 8 KiB | Never allocate or overlay here |
| glN64 depth scratch / lookup table | 153,600 / 131,072 bytes | Keep Zelda/DK64 depth behavior |

The ceiling is `0x933E0000`, not the end of physical MEM2. Runtime Arena2Hi
can be lower. [Startup](../main/main_gc-menu2.cpp) already checks the fixed
map against that ceiling and excludes it from the SDK arena.

All four existing [layout tests](../tests/mem2_layout_test.c) passed during
this planning session:

| Build | Fixed bytes | Nominal unclaimed bytes |
|---|---:|---:|
| Rice, agent off | 53,663,744 | 731,136 |
| Rice, agent on | 53,671,936 | 722,944 |
| glN64, agent off | 53,948,416 | 446,464 |
| glN64, agent on | 53,956,608 | 438,272 |

Thus glN64+agent reserves 51.457 MiB of its 51.875 MiB fixed-map envelope.
This does **not** mean every reservation is occupied. The older illustrative
budget in [ROM paging notes](rom-paging-and-agent.md) is not the current
glN64+agent budget; use the header and tests above.

MEM1 also has little measured headroom. The current release ELF has
1,643,232 text, 556,332 data and 10,492,288 BSS bytes. Its symbols locate
8 MiB guest RDRAM, 256 KiB `rwmem` and 409,552 bytes of texture slots in MEM1.
These are section/symbol sizes, not a complete allocation census. The ELF
SHA-256 is `f8935e4dd2062441ce50cf3cf35b67d86f9e7a9b6b545c0f872bc8d170312069`.
[RecompCache_Init](../r4300/Recomp-Cache-Heap.c) separately allocates a
9.5 MiB glN64 Wii code heap and 384 KiB node heap through MEM1 malloc;
Rice's code heap is 9 MiB. The 4 MiB metadata heap is in fixed MEM2.

Recent sampler-capable controls reported:

| Scene | Newlib heap free | Unallocated Arena1 | Unallocated Arena2 |
|---|---:|---:|---:|
| SM64 file menu | 38,088 | 212,992 | 288,704 |
| Kart race grid | 35,592 | 159,744 | 288,704 |

These are separate counters from `hardware-glN64_wii-20261004-083633-RbU1`.
Private texture/JIT heaps can have free space inside allocations counted as
used by newlib. Arena space is not malloc's largest free block. Do not present
their sum as an allocation guarantee. Wii64 sets `MALLOC_MEM2=0`; the inspected
libogc2 `43f26d22deaacae287c2f8d96eba00dba1ee0155`, `libogc/sbrk.c`, then
keeps newlib in MEM1. Both controls reserved the same 128 KiB MEM1 histogram;
ordinary release does not. Do not infer release headroom by simply adding it.

## What transfers from WiiXplorer

| Observation from its native run | Consequence for Wii64 |
|---|---|
| Warm cached 8 KiB reads: MEM1 and MEM2 both 2217.910 MiB/s | A small cached MEM2 object is not automatically worth moving |
| Cached 256 KiB reads: 1210.837 / 1193.495 MiB/s | Measure misses and access pattern, not just bank names |
| Cached libc copies: MEM1-to-MEM1 201.745; MEM2-to-MEM2 66.522 MiB/s | Investigate repeated large copies and working-set size |
| Uncached MEM2 reads: 19.805 MiB/s | Keep CPU-read sources cached; K1 is not a general speed switch |
| Some write-only uncached destinations won | Test only a proven producer/device-consumer buffer, with alias ownership |
| LC DMA MEM2-to-LC 439.996; LC-to-MEM2 1465.738 MiB/s | DMA-only timing excludes setup/coherency; test a complete useful pipeline |

Cached and uncached aliases name the same physical bytes. Flush dirty cached
data before an uncached/device reader; prevent dirty cached lines overwriting
device results; invalidate before the CPU consumes device writes, with the
required producer completion. Keep cache operations on cached aliases and
ownership exclusive. Align buffers/ranges to 32 bytes and avoid touching a
neighbor's cache line. Preserve JIT D-cache publication and I-cache invalidation.
Use the actual GX, DSP and IOS contracts rather than one rule for every device.

LC is not extra RAM. Enabling it splits the 32 KiB L1 data cache into 16 KiB
normal and 16 KiB locked, reducing normal associativity. L2 is 256 KiB with
64-byte lines. These constraints are documented in the
[IBM 750CL manual, sections 9.1 and 9.2](https://fail0verflow.com/media/files/ppc_750cl.pdf).
WiiXplorer's LC fixes are not an instruction to patch Wii64's SDK: the inspected
libogc2 `libogc/cache_asm.S` already retains MEM2's address bit and LC tag, and
`libogc/cache.c` extracts the four-bit DMA queue length correctly.

## Ranked candidates

The original candidates below now have measured outcomes. The
[results](memory-results-2026-10-04.md) record the additional dynarec bug and
all validation. Each mechanism has its own commit. Conditional experiments
were not implemented without the required evidence.

| Rank | Candidate | Type | Evidence | Expected gain | Risk / scope | Smallest test |
|---|---|---|---|---|---|---|
| P0 | Separate memory accounting and peak measurements | CLEANUP | Implemented; all 18 ROMs reconcile on both platforms; deterministic guest parity restored, census audio gate still fails | Capacity observations only; no speed claim | Low; opt-in stopped census | Keep timing claims provisional |
| P1 | Right-size boxart reservation | PERF | Implemented; all 16 textures charge 737,920 bytes | Confirmed 256 KiB MEM2 capacity recovered | Low/medium; one constant plus capacity test | Two native old/new/old trials, browser, HOME and Rice passed |
| P1 | Allocation-failure fallback where a failure is demonstrated | GUARD | Fixed unchecked boxart memset and controller failure | Avoid a crash, not higher FPS | Low; menu allocation sites only | Every failing slot and controller init passed |
| P1 | Reduce one measured copy/conversion working set | PERF | Existing YUYV optimization already handles invariant columns/clamping; no new copy removal proved | Not selected | Medium; one conversion and exact fallback | Establish producer/reader coherency before another variant |
| P2 | Move/split one small hot metadata set | PERF | Large texture metadata resides in MEM1; access locality not yet known | Unknown; possibly headroom or fewer misses | Medium; address/lifetime and lookup cost | Record slot peaks and access mix before selecting any fields/bank |
| P2 | Tune texture or recompilation metadata reservation | PERF | Valid short-scene peaks and largest requests recorded; no capacity failures or evictions | Not selected without long-gameplay bounds | Medium; one pool, no eviction redesign | Long-game peak/eviction census before a size step |
| P2 | Compact TLB, block or invalidation representation | PERF | 8+4+1 MiB tables cover guest address space, not just RDRAM | Potential capacity; lookup cost could regress | High; separate one-table experiments | Host alias/TLB/store/DMA tests before any native performance trial |
| P2 | LC staging for a measured reusable tile/audio kernel | PERF | Verified reference DMA works, but normal L1 shrinks | Unknown end-to-end | High; opt-in bounded kernel only | Normal cached vs LC load+compute+store+coherency+restore |

The boxart bound comes from [SelectRomFrame.cpp](../menu/SelectRomFrame.cpp)
and [boxart.h](../main/boxart.h): 16 x 120 x 192 x 2 = 737,280 payload bytes.
768 KiB would leave 49,152 bytes for allocator overhead. Verify that bound
against the actual LWP allocator; do not infer it from payload alone.
Keep every texture slot and existing menu behavior. This is simpler than
overlapping menu storage with a gameplay heap. `BOXART_DeInit` frees a header,
not the fixed MEM2 reservation; closing the boxart file does not reclaim 1 MiB.

## Phases and acceptance gates

### 1. Establish a memory census without changing placement

Use the existing graph, ELF tools and profiler. Extend the existing report,
not a replacement allocator, queue client or profiler framework.

- Record fixed region bounds, actual arena bounds, allocator ownership and
  linker sections. Include MEM1 stacks, network pools, audio queues and temporary
  texture/conversion allocations, not only the large MEM2 reservations.
- For texture, JIT code, JIT metadata, node and boxart heaps, record peak live
  bytes/counts, failed requests and request sizes, eviction count/bytes and
  minimum free bytes. Account for the heap's real allocated sizes/rounding.
- Read largest free blocks through audited allocator accounting at safe points
  only. Do not allocate trial buffers or walk heaps inside a render/dispatch loop.
  If an allocator cannot report a largest block safely, label it unavailable.
- Snapshot boot, post-load, active gameplay, pause/HOME, ROM teardown and next
  load. Update peaks at existing allocate/free/realloc sites so brief peaks are
  not lost between snapshots. No memory scan, clock read or I/O per instruction,
  vertex or sample. Buffer small records; upload after stopping.
- Include VM faults, read-ahead hits/misses, dirty writebacks and loaded ROM size.
  Record GX retired-texture bytes separately from immediately reusable bytes.

Deliverable: bank/pool ownership table and peak/fragmentation report. Keep the
layout unchanged and compare census-on/off overhead with identical reservations.
Check repeated identical ROM transitions for growth after expected persistent
allocations stabilize. A full-library sweep follows the bounded prototype.

### 2. Recover bounded capacity and test realistic failures

First test the 16-buffer boxart bound with the current allocator and a 768 KiB
trial. If it passes, change only that reservation, update the derived layout,
and leave the HBC record address fixed. Verify shifted XFB/scratch pointers,
runtime Arena2 exclusion and HOME borrowing. Record actual recovered headroom;
more free capacity alone is not a frame-rate improvement.

Add only justified allocation guards in separate commits. On failure, keep a
valid placeholder or report a load error without dereferencing NULL. Extend
existing layout tests for full region ordering/alignment and needed build
variants. Do not write canaries into live buffers or add checks to proven hot
paths. Preserve save, font, thumbnail and depth lifetimes.

### 3. Remove traffic before relocating data

Resolve the current profiler's guest-work calibration first. Its
[initial run](performance-methods-2026-10-04.md) passed the requested wall-cost
target but failed aggregate guest exception parity, including control-to-control
variation. Per-cause exception counts and matched scene/workload checks must
explain that variation before sampler shares support optimization claims.
Capacity accounting can proceed independently.

Then measure one candidate's bytes, calls, producer, consumer and waits. For
Kart conversion, compare exact RGBA5551 output, scaling, swapped halfwords and
framebuffer markers before testing locality changes. Keep GX completion and
dirty-generation rules. Require active-gameplay validation as well as the
existing race-grid scene. Remove a copy only when ownership permits direct use;
do not confuse a cached memcpy cost with a redundant-copy proof.

### 4. Tune placement and pool budgets from peaks

Preserve RDRAM, JIT executable code and CPU state in MEM1 initially. Consider
moving cold metadata or keeping a small hot subset only after identifying actual
accesses; PC samples alone cannot identify which data cache lines miss. Use
audited PMC event pairs where supported, with the same configuration in controls.
Do not spend all recovered space: set reserve from observed transition peaks,
maximum requests, fragmentation and explicit failure behavior, not an arbitrary
percentage. Change one pool size or one placement, never both in the same trial.

### 5. Investigate structural savings only if capacity is still limiting

Wii uses direct TLB tables; GameCube's existing `USE_TLB_CACHE` implementation
is a reference, not a free port. Sparse/packed tables must preserve the complete
guest virtual page namespace, read/write permission, TLB remaps and savestates.
Block/invalidation changes must retain KSEG/TLB aliases, DMA and short-store
invalidation, JR/JALR targets, linked-code lifetime and page-boundary behavior.
Add host invariants before changing representation; measure the extra lookup
work on Wii. Never reduce table coverage to the physical RDRAM size.

Keep ROM preflush and 32 KiB read-ahead, which already exist. The 16 MiB VM
backing contains a 64 KiB PTE hole: it is not 16 MiB of usable resident ROM pages.
`vm/wii_vm.c` has 4096 physical entries, a 12-bit page index and a 16 MiB limit.
Larger ROMs already page; this limit is not a 16 MiB maximum ROM size. Growing
the backing would need an independent representation/sentinel/DSI review, not
a larger constant. Do that only if measured fault/writeback stalls justify it.

### 6. Try LC only for a remaining measured kernel

First use a small, opt-in kernel experiment with cached input/output and an
8 KiB staging budget. Include enable/restore, DMA setup, flush/invalidate, queue
drain and computation in end-to-end timing. Compare CPU cycles for neighboring
work with 32 KiB normal L1 versus LC's 16 KiB normal L1. Verify exact output and
preserve HID2/BAT/machine-check state; reject existing LC ownership and handle
bounded DMA timeout safely. Inspect SDK and generated assembly, not just API
names. A kernel-only DMA win is insufficient to enable LC in production.

## Shared validation and stop rules

For each implemented mechanism, run its existing host/subsystem tests, a clean
affected Wii build and muted-host Dolphin smoke/library checks. Mute Dolphin
playback, not the emulated sound engine. Freeze source, ELF/DOL hashes, compiler,
libogc2/HBC agent versions, settings and replay through the existing Wii queue;
the job may wait for hardware availability. Return to HBC, never power off.

Use real-Wii old/new/old trials with identical probe flags, reservations, scenes,
VI targets, renderer, resolution and audio settings. Test SM64, Kart, the owned
Zelda/DK64 depth/large-ROM cases and texture/audio-heavy library cases with
validated gameplay replays. Include PAL-sized output, ROMs above 16 MiB,
repeated menu/ROM/HOME transitions and interrupted loading. Preserve crash
records and VM recovery. Inspect final frames and guest-visible state/counters;
monitor audio underruns, page faults, eviction pressure and frame-time tails.
Check Rice separately; retain GameCube build/tests and separate vWii hardware
status rather than treating native Wii validation as validation of all targets.

Report capacity separately from speed. Performance labels: confirmed improvement,
neutral/noise, regression, correctness failure or hardware validation pending.
Reject results within control drift or with unexplained guest-work changes.
Revert a regression. Do not retain a complex mechanism to justify sunk effort.
Keep ROMs, raw logs and copyrighted captures out of commits.

Rejected transfers: wholesale WiiXplorer malloc wrappers; treating all 64 MiB
MEM2 as application space; using K1 for CPU-read data; stealing HBC/IOS regions;
overlapping menu/game heaps before proving asynchronous lifetimes; reducing XFB
capacity to NTSC; moving working MEM2 XFBs into MEM1 because of WiiXplorer's
video comment; enabling LC globally. The YUYV helper already records that
`dcbz/dcbt` prefetch regressed from 2.55 to 2.85 ms on Wii; do not repeat it as
an untested optimization.

## Next bounded experiment

Test XFB reader coherency with distinct cached/device patterns and an explicit
producer-complete boundary. Prove a stale read before changing cache operations;
retain the existing conversion and rendering output as the reference. This is
the highest-value bounded follow-up, not permission for a general LC rewrite.

## Local-model review

Muse Glimmer/Gemma4 researched the supplied sources in 76.2 seconds;
Qwen3-Coder/Devstral reviewed the proposed phases in 57.5 seconds. All four
completed through Ollama with 10,000-token caps and 300-second timeouts;
vLLM was offline. Completion was reliable, factual accuracy was not. Source
checks rejected proposals to implement the existing sampler again, use
WiiXplorer's free-byte counts as Wii64's, or treat VM backing as a maximum ROM
size. Safety objections became explicit proof/test gates, not claims of newly
discovered bugs. Improve local-llm review with required source excerpts,
existing-versus-new classification and separate fact/hypothesis fields.
