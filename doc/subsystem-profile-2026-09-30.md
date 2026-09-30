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
ROMs and replay files must already be on the Wii SD card, or use the existing
LAN ROM transfer option. Run one build workflow at a time: targets share objects.

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

These are inclusive wall-clock spans, not CPU partitions. Dispatch overlaps
lookup and compilation. Execution can include RSP work, sleep, and exceptions.
Presentation from other graphics paths remains inside the graphics span.
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

## Next subsystem: large-ROM paging

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

Graphics is the next scene-specific candidate: Snap's graphics span uses 36.7%
of Wii wall time. Split display-list parsing, texture conversion/upload, GX
submission, and waits before selecting a change. Function lookup is secondary:
its sparse estimate was 1.6–4.8% in MP3/SM64/Banjo and lower in TWINE/DK64.
Tree depth 31 in Banjo is evidence to investigate, not a reason to replace it.

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
