# Subsystem survey: 2026-09-30

## Run the survey

Configure the central Wii lease client and SD ROMs as described in
[hardware sessions](hardware-session.md). Leave Homebrew Channel open, then run:

```bash
.dev/test_subsystems.sh
.dev/profile_subsystems.sh
```

The driver makes clean glN64 Wii builds with full probes and with those probes
disabled. Both retain the existing `PERF_PROF` counters and audio probes. It
freezes DOL/ELF pairs before queuing full/control/full runs, saves source hashes,
compiler identity, flags, artifact hashes, job IDs, and result paths under one
ignored `.dev/runs/` directory. Each run releases the lease after returning to
Homebrew Channel. Other workstations retain their turns.

Use a chain name as the first argument to select another file in `scripts/chains/`.
ROMs must already be on the Wii SD card, or use the existing LAN ROM transfer
option. With the external HBC-Reborn client, the runner stages the selected
chain's replay files before launch. Run one build workflow at a time: targets
share objects. Do not change engine source while the two builds are compiling.

To inspect an existing result:

```bash
python3 scripts/subsystem_report.py <run-directory>
python3 scripts/subsystem_report.py <run-directory> --json
python3 scripts/subsystem_compare.py <full1> <control> <full2>
```

For Dolphin, build full probes and pass the same chain settings to
`.dev/dolphin_test.sh` with `WII64_DOLPHIN_DSP_HLE=False`. Timed launches mute
host playback while the guest audio engine remains active. Reuse an isolated
profile; frame dumping remains opt-in. A guest log can remain inside the raw
SD image until shutdown, when the launcher extracts it. Hardware validation
also checks wiiload arguments and is not a Dolphin-result validator.
Menu-only Dolphin boots retain earlier logs but do not print their stale chain
tables as new results.

## Probe boundaries

Enable new timers with `DEBUG_FLAGS='-DPERF_PROF -DPERF_SUBSYSTEM_PROBES'`.
Release builds and ordinary profiling builds compile these timers out.
The timers reset at game start and the first measured VI. A span that crosses
that reset is discarded. One summary per game adds no per-frame file writes.

| Stage | Boundary | Timed calls |
|---|---|---|
| `rsp_gfx`, `rsp_audio`, `rsp_other` | HLE execution, classified by declared RSP task type | All |
| `lookup` | `find_func`, including its tree walk | 1 in 127 |
| `compile` | `recompile_block` | All |
| `dispatch` | Address validation through lookup, compilation, and linking | 1 in 127 |
| `execute_inclusive` | `dyna_run` | 1 in 127 |
| `rom_copy` | Wii `ROMCache_read` copy, including synchronous VM faults | All |
| `present` | glN64 exported `UpdateScreen` | All |
| `limiter` | Actual `usleep` duration | All |
| `tex_hash` | Tile TMEM/palette hash or background RDRAM/palette hash | All |
| `tex_lookup` | Texture cache search, excluding activation on a hit | All |
| `tex_load` | Texture/background allocation, conversion and cache flush; each mip level separately | All |
| `tex_activate` | Texture activation, GX setup and LRU promotion | All |
| `draw_triangles` | glN64 triangle batch submission and its state setup | All |
| `draw_rect` | Filled/textured rectangles, including state and texture setup | All |

These are inclusive wall-clock spans, not CPU partitions. Dispatch overlaps
lookup and compilation. Execution can include RSP work, sleep, and exceptions.
Presentation from other graphics paths remains inside the graphics span.
The rectangle span can contain texture spans. Mip-chain assembly and
framebuffer texture paths are not included in `tex_load`. These CPU-side
spans include FIFO stalls but are not measurements of GPU execution time.
The report scales sparse samples by calls/timed calls and labels them estimates.
Deterministic sampling can alias; an estimate can exceed wall time or be smaller
than a fully timed child. Preserve those values and change the sampling interval
for a confirmation run instead of clamping them into an apparent CPU budget.
`PERF_SUBSYSTEM_INTERVAL` changes the hot-operation interval at build time.

## First six-game survey

All scenes use dynarec, Accurate synthesis, Wii DSP output, Accurate mixer,
Stable latency, Native Rate, neutral replay, and 900 guest VIs. Mario Party 3
is PAL, so its nominal duration is 18 seconds; the others use 15 seconds.
These are startup/title scenes, not whole-game benchmarks. Final Wii captures
show the MP3 language menu, TWINE license screen, and DK64 intro. Snap has a
rendered moving background; its screenshot alone does not verify gameplay.
Zelda is excluded.

| ROM | Size | Wii speed | ROM copy, observed | Graphics, observed | Dolphin speed |
|---|---:|---:|---:|---:|---:|
| Super Mario 64 | 8 MB | 0.987x | 24.9 ms | 1,452 ms | 0.999x |
| Banjo-Kazooie | 16 MB | 0.988x | 14.4 ms | 1,323 ms | 0.995x |
| Pokémon Snap | 16 MB | 0.985x | 14.8 ms | 5,587 ms | 0.991x |
| Mario Party 3 PAL | 32 MB | 0.873x | 2,566 ms | 778 ms | 0.877x |
| The World Is Not Enough | 32 MB | 0.832x | 3,252 ms | 504 ms | 0.826x |
| Donkey Kong 64 | 32 MB | 0.843x | 2,954 ms | 294 ms | 0.844x |

Actual limiter time exceeded requested sleep by only 2.4–2.9 ms across each
Wii scene. Seconds-scale oversleep is not supported by these measurements.
The three slow scenes each spent 12–18% of wall time in fully timed ROM copies.
This is a correlation, not proof that all lost speed comes from paging.
Existing audio underruns still occur; all six entries had zero overruns.

## Matched probe overhead

The same frozen full DOL was used before and after the disabled control. All
18 Wii entries reached 900 VIs with the selected replay. Each job returned to
Homebrew Channel and released its lease. The table compares the mean of the
two full runs with the control, not an optimized emulator with an old version.

| ROM | Added CPU cycles | Wall-time change |
|---|---:|---:|
| Super Mario 64 | +4.45% | +0.17% |
| Banjo-Kazooie | +4.11% | −0.02% |
| Pokémon Snap | +2.10% | +0.01% |
| Mario Party 3 PAL | +2.59% | +0.08% |
| The World Is Not Enough | +2.74% | +0.10% |
| Donkey Kong 64 | +2.50% | +0.33% |

Keep the timers opt-in: small wall-time changes can hide CPU work when the
limiter reduces sleep. One A/B/A triple estimates overhead for these scenes;
it does not calibrate timer cost for other games or remove probe perturbation.
The repeat measured ROM-copy spans of 2,578/3,262/2,959 ms in MP3/TWINE/DK64,
and actual limiter excess remained within 3 ms per scene. The fully timed
graphics and ROM-copy spans were stable; sparse execution estimates varied.
All runs had zero audio overruns; underruns still ranged from 0 to 17.

Raw logs, configs, and replay records are filed under:

- `baselines/2026-09-30_hw_subsystems_full1/`
- `baselines/2026-09-30_hw_subsystems_control/`
- `baselines/2026-09-30_hw_subsystems_full2/`
- `baselines/2026-09-30_dolphin_subsystems_1.6.2/`

The Wii triple used instrumented 1.6.1 binaries. The version bump to 1.6.2
was then built and checked in a second six-game Dolphin survey; the engine
probe code was unchanged. Both Dolphin surveys completed all targets without
invalid-access warnings in their latest boot logs. The table's Dolphin column
uses the final 1.6.2 run. Timing does not validate Dolphin's diagnostic XFB images.
Matching DOL/ELF files remain ignored under `.dev/runs/subsystem-survey-20260930/`;
their hashes and job IDs are in `baselines/2026-09-30_subsystems_artifacts.json`.
The ordinary 1.6.2 release build also linked with the new probes disabled.

## Paging status

Paging is now enabled by default after matched hardware checks;
see [paging results](rom-paging-and-agent.md#matched-wii-comparison).
The original investigation plan remains here for later regressions.

## Graphics detail and current probe cost

The 1.6.4 `graphics_survey` adds texture and draw timers. A matched
full/disabled/full Wii triple completed all nine entries and returned to HBC.
Paging defaults, audio settings, replays and VI targets were identical.

| ROM | Added CPU cycles | Wall-time change | Hash time, first full run |
|---|---:|---:|---:|
| SM64 | +5.14% | −0.08% | 282.2 ms |
| Banjo-Kazooie | +4.42% | +0.07% | 384.4 ms |
| Pokémon Snap | +2.54% | +0.27% | 1,593.0 ms |

Snap made 147,492 hash calls but only 136 texture conversions, which took
18.9 ms. Cache searches took 43.8 ms. Hashing is a better next target than
replacing the cache-search structure or adding more texture memory. All timers
remain opt-in: frame-rate limiting can hide probe CPU cost in wall time.
These scene-specific costs do not calibrate a different game or replay.
Results are filed in `baselines/2026-09-30_wii_graphics_probes_*`.

## Texture-hash experiment

Run `bash .dev/test_texture_hash.sh` for exact legacy XXH32 comparisons and
mutation/reset checks under AddressSanitizer and UndefinedBehaviorSanitizer.
For the matched Wii candidate/reference/candidate test:

```bash
bash .dev/profile_texture_hash.sh graphics_survey HBC_AGENT=1
```

The candidate uses two hash memos, one per texture unit. Every TMEM load marks
both dirty. A dirty memo can reuse its hash only if the hash inputs and every
byte the original hash reads still match its snapshot. Palette bytes and
wrapped/clamped rows are included. New ROMs reset both memos; background RDRAM
images keep live hashes. Snapshots occupy about 8 KiB of MEM1, not a new MEM2
reservation. The original hash path remains available with
`GLN64_TMEM_HASH_CACHE=0`. The matched hardware result is mixed, so this
experiment remains disabled by default.

The first generation-only memo passed all nine matched Wii targets but added
0.03%/0.34%/1.41% CPU cycles in SM64/Banjo/Snap, without a useful wall-time gain.
It is rejected: TMEM reloads invalidate that design too often. The byte-snapshot
comparison is a separate experiment. Logs for the rejected
triple remain in `baselines/2026-09-30_wii_texture_memo_initial_*`.

The byte-snapshot candidate completed all nine matched Wii targets, with no
I/O errors or audio overruns, and returned to HBC after every job. Its CPU-cycle
changes were +0.86% in SM64, +1.50% in Banjo and −1.73% in Snap. Wall-time
changes stayed within 0.1% because these scenes run near their VI limits.
Hash time changed by +12.94%/+14.73%/−27.38% in SM64/Banjo/Snap respectively.
It is not a general speed improvement and does not justify a ROM-name override
from one intro scene. Keep it opt-in while testing recorded gameplay and a
cache policy that reduces misses without adding enough work to erase the gain.
Logs are in `baselines/2026-09-30_wii_texture_snapshot_*`; matching artifacts,
flags, source-manifest hashes and queue IDs are recorded in
`baselines/2026-09-30_gameplay-artifacts.json`. Dolphin completed both the
three-game graphics chain and the longer five-entry chain with this candidate.

## Original paging investigation

`gc_memory/MEM2.h` reserves 16 MB for the ROM cache. Larger ROMs use `VM_Init`
in `main/ROM-Cache.c`. `vm/wii_vm.c` backs that mapping with the NAND
`/tmp/pagefile.sys` file. Its DSI handler evicts pages and fetches committed
4 KB pages with synchronous `ISFS_Seek`/`ISFS_Read`. The ROM-copy timer includes
faults encountered while reading that virtual source. ROM load time before the
first measured VI is excluded.

1. Count VM faults, page-ins, dirty evictions, and bytes copied. Time pagefile
   seek/read/write and victim selection separately with opt-in, buffered probes.
   Distinguish faults inside a ROM copy from direct ROM accesses elsewhere.
2. Compare cold and warmed accesses with the same byte count. Verify alignment,
   page boundaries, byte-swapped ROM content, and failed/short pagefile I/O.
   Audit page ownership and PTE invalidation before changing eviction behavior.
3. Select the smallest confirmed fix: reduce repeated page-ins or unnecessary
   dirty work before replacing the VM design. Preserve ROM bytes and saves.
4. Run matched Dolphin correctness checks and Wii A/B/A measurements. Accept an
   optimization only if cycles or wall time improve without new faults, scene
   changes, or audio regressions. Repeat enough runs to separate SD/NAND variance.

## Next probes

Graphics is the next scene-specific candidate: Snap's graphics span uses 36.7%
of Wii wall time. Split display-list parsing, texture conversion/upload, GX
submission, and waits before selecting a change. Function lookup is secondary:
its sparse estimate was 1.6–4.8% in MP3/SM64/Banjo and lower in TWINE/DK64.
Tree depth 31 in Banjo is evidence to investigate, not a reason to replace it.

Version 1.6.4 adds the texture and draw boundaries above, compiled out in
ordinary builds. Run `.dev/profile_subsystems.sh graphics_survey HBC_AGENT=1`
after building the external agent SDK. This queues SM64/Banjo/Snap with
full/disabled/full probes and measures their added cost. Use the matching
chain in muted LLE Dolphin for correctness. Keep GX waits, texture hashes,
cache keys and ownership unchanged until a measured substage justifies a fix.
Use the newer graphics overhead table above, not the original six-game costs,
when estimating the added probes' effect on these three scenes.

## Verification

Host tests cover disabled macros, sparse clock reads, nesting, reset-crossing
spans, alternate intervals, queue guards, spaced paths, frozen build ordering,
reports, and comparison rejection of unmatched or Dolphin PMC data. The existing
audio regression suite passed. Full, control, and release builds linked on this Intel Mac
with devkitPPC r50-1/GCC 16.1.0 and libogc2; existing compiler warnings remain.

The local-model reviews used 10,000-token output caps. Muse and Gemma offered
useful competing hypotheses but also made unsupported claims about macros,
exception timing, and sampled parent/child spans. Tests and source inspection
settled those claims; model agreement alone was not treated as evidence.
