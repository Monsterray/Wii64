# Benchmark and instrumentation plan

## Measurement rules

Use real Wii hardware for performance conclusions. Record the exact console/model and MEM2 configuration, renderer, video mode, build flags, libogc2/devkitPPC revisions, ROM identity (title, region, revision, CRC), emulator settings, storage medium, and controller/input route. Do not commit ROMs, BIOS files, save files, copyrighted screenshots, or raw ROM-derived data.

For each A/B pair:

1. Boot from a clean, documented state and use the same deterministic input route or fixed gameplay segment.
2. Warm up before collecting data so code and texture caches reach a comparable state.
3. Capture at least 60 seconds or a fixed number of VI intervals, with three repeats per configuration.
4. Report median and p95 frame/VI time, VI/s, and relevant counters; keep individual runs for auditability.
5. Reject a speedup if it changes rendering correctness, audio stability, bootability, save behavior, or target compatibility.

A host build on macOS is useful for static checks only. It cannot establish Wii GX, cache-coherency, MEM2, VM, or audio performance.

## Proposed game matrix

Subsystem labels are hypotheses to verify with counters and traces, not claims that a title always stresses only that subsystem.

| Priority | Title | Primary stress hypothesis | Secondary checks |
|---|---|---|---|
| Smoke | Super Mario 64 | Baseline CPU/RSP/graphics path | Boot, input, stable VI/s |
| Smoke | Mario Kart 64 | CPU, display lists, VI pacing | Multiplayer/menu transitions |
| Smoke | Mario Party 1 | Dynarec cache churn, CPU/timing | Run Dolphin with PPC MMU enabled; boot a board, return to the menu |
| Broad | The Legend of Zelda: Ocarina of Time | Framebuffer textures, timing, F3DZEX2 | Pause/menu, transitions |
| Broad | The Legend of Zelda: Majora’s Mask | Framebuffer/memory/timing | VM/ROM reads, transitions |
| Broad | GoldenEye 007 | F3DGOLDEN, CPU, textures | Heavy scene changes |
| Broad | Perfect Dark | F3DPD, CPU, memory | Stress segment and boot |
| Stress | Banjo-Tooie | Fast memory, CPU/RSP, expansion | Save/load and dense scenes |
| Stress | Donkey Kong 64 | Expansion, RSP/framebuffer, CPU | Boot and large scenes |
| Stress | Conker’s Bad Fur Day | TLB/cache behavior, ROM, framebuffer | TLB candidate control |
| Stress | Rogue Squadron | F5Rogue, framebuffer, CPU | Dense 3D segment |
| Broad | Star Wars: Episode I Racer | F5Rogue/VI/fullscreen behavior | Race segment |
| Smoke | F-Zero X | RSP/graphics and fast paths | High-motion race |
| Broad | Paper Mario | ROM/CPU/framebuffer/2D | Menus and battle scene |
| Broad | Yoshi’s Story | S2DEX/2D, texture cache | Background and transitions |
| Broad | World Driver Championship | ZSortBOSS, CPU/graphics | Race segment |

The first pass should use the smoke set plus the title most relevant to the active candidate; the full matrix is the compatibility gate before merging a candidate.

## Existing versus needed metrics

| Metric | Current support | Required follow-up |
|---|---|---|
| VI/s and DL/s | Displayed by glN64 from `main/timers.c` counters. | Preserve as headline throughput, but capture raw samples. |
| Section time | `PROFILE` in `r4300/profile.c` reports graphics/audio/TLB/compiler/interpreter/trampoline/link/unlink/dyna-mem/ROM-read/blocks once per second. | Use only in profile builds; document its overhead. |
| Frame/VI time distribution | Not a reusable persisted metric. | Add a disabled-by-default fixed-size sample or aggregate in the existing profile path. |
| Fresh blocks, evictions, code bytes, links | Not currently counted together. | Add counters around `Recompile.c` and `Recomp-Cache-Heap.c`. |
| Fast-memory paths | No dedicated hit/slow-path counter. | Count direct, fallback, and invalidation paths. |
| TLB paths | Section time exists, but not hit/miss breakdown. | Count TLB-cache levels and slow translations. |
| ROM/VM | ROM-read time exists; cache/VM fault counts do not. | Count resident hits, loads/faults, bytes, and read latency. |
| Audio | Audio section time exists. | Count underruns/starvation events and queue depth. |
| Memory | Static MEM2 map is documented in `MEM2.h`. | Record high-water marks and selected cache occupancy. |

## Instrumentation contract

The first implementation experiment is exactly one small measurement improvement: add a disabled-by-default per-VI aggregate for frame time plus existing profile-section snapshots, emitted once per second in the current debug/profile channel. It must not alter release behavior or renderer/CPU scheduling. Do not add candidate-specific counters until the measurement build is confirmed on hardware.

Suggested capture columns:

```text
build,game,rom_crc,renderer,video_mode,hardware,mem2_class,run,window_s,vi_s,dl_s,frame_us_median,frame_us_p95,gfx_pct,audio_pct,compiler_pct,tlb_pct,dynamem_pct,romread_pct,blocks_pct,notes
```

## Decision gates

For a candidate to proceed, require a repeatable improvement in the candidate’s stated metric and no regression in the smoke set. A broad performance claim should survive three runs and remain meaningful after profile overhead is removed. For memory/VM work, fewer faults and lower ROM-read latency are primary gates; for code-cache work, compile/link time and frame-time tail are primary gates; for renderer work, VI/clear time plus visual correctness are primary gates.
