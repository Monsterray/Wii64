# Subsystem survey: 2026-09-30

See [the 2026-10-02 GPU survey](gpu-results-2026-10-02.md) for where graphics time goes, and
[the 2026-10-01 whole-system results](subsystem-results-2026-10-01.md) for
the expanded coverage, renderer surveys, probe cost and invalidation experiment.

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
New builds also retain the tracked working-tree diff. Source hashes detect
changes; they are not a substitute for retaining new untracked source files.

Use a chain name as the first argument to select another file in `scripts/chains/`.
`system_survey` covers eight ROMs; `system_audio_survey` adds five ROMs and
repeats Mario Party 3 PAL. These are separate chains to stay within Wiiload's
argument-size limit. Both use existing recorded inputs; other entries are
neutral title/intro scenes. Neither is full gameplay coverage.

Set `WII64_SURVEY_TARGET=Rice_wii` for the Rice target. To queue without waiting,
set `WII64_SURVEY_QUEUE_ONLY=1`. Collect with
`bash .dev/profile_subsystems.sh --collect <survey-directory>`.
To test another chain with verified binaries and no rebuild, use
`bash .dev/profile_subsystems.sh --reuse <survey-directory> <chain>`.
The new survey records its parent and freezes the new chain. Keep the same
settings and probe boundaries when comparing emulator changes.
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
Completed Dolphin chains retain logs, config, replay traces, diagnostic frames,
and DOL/ELF hashes in ignored `.dev/runs/dolphin-chain-*` directories. Their
validator checks VI targets, ROMs, replay loads, CPU core and audio modes; it
does not require the hardware-only Wiiload marker. A completed VI count does
not prove rendering or gameplay is correct.
The launcher checks the whole latest Dolphin boot for invalid accesses, unknown
DSP ucode and SD-sync failures, not only its last 20 log lines. A leftover
`Load/WiiSDSync.xxx` backup stops folder-sync launches. Preserve that folder
outside `Load/` before retrying and recover missing files while Dolphin is
stopped. COMMON logging records the sync failure cause. New chain ROMs must
exist in the staged profile before boot.

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
| `gfx_list` | Display-list parser, both renderers; overlaps `rsp_gfx` | All |
| `gfx_command` | glN64 GBI command handler | 1 in 127 |
| `vertex` | glN64 per-vertex processing; Rice vertex batch processing | 1 in 127 |
| `gfx_state` | glN64 `OGL_UpdateStates`, including nested texture work | 1 in 127 |
| `gx_wait` | Existing glN64 `GX_DrawDone` calls; adds no new waits | All |
| `dma_pi`, `dma_sp`, `dma_si` | Guest DMA handlers, including copies/invalidation/PIF work | All |
| `tlb_translate` | `virtual_to_physical_address` | 1 in 127 |
| `memory_slow` | Dynarec `dyna_mem` slow path, including mapped-device handlers and RSP work | 1 in 127 |
| `jit_invalidate` | Eligible-page tree search/free; already-invalid pages remain in DMA/memory spans | 1 in 127 |
| `pif`, `input` | PIF read/write and guest `GetKeys` input | All |
| `guest_interrupt` | Guest interrupt handler, including VI limiter and presentation | All |
| `interpreter_inclusive` | Pure interpreter, blocks of 256 opcodes plus final partial block | All blocks |
| `audio_submit`, `audio_callback` | Audio-length submission and AESND callback, separate IDs | All |

Audio synthesis substage timings are reported as sampled time only; they are
not extrapolated. Snapshot/reset masks host IRQs. Self-attribution builds also
mask IRQs during clock reads and bookkeeping, not across measured operations.
Callback timing has one writer and its own stage. GPU execution, host scheduler/other
IRQs and inline guest loads/stores remain unmeasured directly. Rice has coarse
display-list, vertex and presentation timers, not glN64's texture/state detail.
Inactive counters are distinct from absent instrumentation and unsampled calls.

Failed loads and non-VI stops are excluded from the report ranking: a load
failure can retain the previous game's counters. Very low graphics-task counts
raise a scene warning. Inspect captures before using such rows as a gameplay
baseline. The comparison also reports non-sleep wall time (wall minus requested
limiter sleep); this includes other waits and interruptions, not CPU self time.

The first 1.6.5 instrumented artifacts timed every invalidation call, including
already-invalid pages. Their `jit_invalidate` boundary differs from the reduced
probe boundary above. Compare only binaries with the same definition. The
range-skip experiment is opt-in with `DYNAREC_INVALIDATE_PAGE_SKIP=1`; value 0
retains the original walker. It skips already-invalid pages but visits eligible
addresses at the original four-byte stride, including unaligned and wrapped
ranges. Run `python3 tests/invalidation_range_test.py` for differential checks.

These are inclusive wall-clock spans, not CPU partitions. Dispatch overlaps
lookup and compilation. Execution can include RSP work, sleep, and exceptions.
The memory slow path includes mapped-device handlers and RSP execution; its
estimate is not memory access self time.
Presentation from other graphics paths remains inside the graphics span.
The rectangle span can contain texture spans. Mip-chain assembly and
framebuffer texture paths are not included in `tex_load`. These CPU-side
spans include FIFO stalls but are not measurements of GPU execution time.
The report scales sparse samples by calls/timed calls and labels them estimates.
Deterministic sampling can alias; an estimate can exceed wall time or be smaller
than a fully timed child. Preserve those values and change the sampling interval
for a confirmation run instead of clamping them into an apparent CPU budget.
`PERF_SUBSYSTEM_INTERVAL` changes the hot-operation interval at build time.

## Self attribution and remaining boundaries (1.6.8)

Schema 2 retains the inclusive timers and adds `subsystem_self` records.
Execute, slow-memory, dispatch, guest-interrupt, graphics-state and interpreter
spans exclude named nested work **within the same timed invocation**. Nested
exclusions count their union, not the sum of overlapping children.

| Self stage | Work excluded |
|---|---|
| `execute_inclusive` | Instrumented C bridges, slow-memory handlers, guest interrupts, VM faults and AESND callbacks |
| `memory_slow` | RSP tasks, DMA, ROM copying, VM faults, PIF, interrupts, audio submission/callbacks, TLB translation, invalidation and C helpers |
| `dispatch` | Lookup, compilation, invalidation, TLB translation, VM faults and AESND callbacks |
| `guest_interrupt` | Limiter sleep, presentation, RSP tasks, AESND callbacks, save states, buffered probe writes and agent polling |
| `gfx_state` | Texture hashing/search/loading/activation, existing GX waits and AESND callbacks |
| `interpreter_inclusive` | RSP tasks, DMA, interrupts, VM faults, PIF and audio submission/callbacks |

A skipped child receives clock reads only while an active sampled parent
needs to exclude it. Those forced readings do not enter the child's inclusive
sample total. A reentrant root is rejected and counted in `dropped`; the report
does not extrapolate a stage with dropped self samples. Reset-crossing spans
are discarded. The original hot sampling interval still applies, so self
estimates can alias and are not an additive CPU partition.

Clock reads and attribution updates are IRQ-atomic. An audio callback between
an end timestamp and its update previously produced dropped samples on Wii;
an injected-IRQ regression test covers that boundary. Skipped calls with no
active self root return before attribution work.

Execute self still includes inline guest memory instructions, generated
prologue/epilogue work, uninstrumented arithmetic helpers and other host IRQs.
Memory self is the remaining handler work, not the cost of all guest memory
accesses. Host scheduler time is not isolated. Do not call these hardware CPU
self-cycle measurements.

New inclusive stages cover `cpu_helper`, FAT `storage_read`/`storage_write`,
EEPROM/SRAM/FlashRAM/mempak `save_load`/`save_write`, `state_load`/`state_save`,
buffered `probe_io` and synchronous `agent_poll`. They do not time the agent's
background networking. Startup remains separate; counters reset at the first
guest VI. Automated chains do not write the user's saves to exercise these
probes. All existing active `GX_DrawDone` calls in glN64, Rice and the UI are
timed; no new waits are added.

`dsp_samples` records the number of existing periodic AESND usage
observations; its mean/peak describe the latest DSP block budget, not CPU wall
time. Separate inline load/store cost still requires other measurement methods.

### GP counters (1.6.8, every `PERF_PROF` build)

The GP's own counters replace the old `gpu_status` busy-bit samples, which
could not measure utilization. Each game writes one `gpu_counters:` line:

| Field | What the GP counted |
|---|---|
| `clks` | GP clocks (243 MHz) |
| `tb_clks` | The same span from the time base x 4; `clks` must match it |
| `ras_busy` | Clocks the rasterizer was busy; `ras_peak_permille` is the busiest 500 ms |
| `xf_wait_in` | Clocks the transform unit (XF) waited for input: idle, or not fed fast enough |
| `xf_wait_out` | Clocks XF waited for setup, raster, TEV or PE |
| `ztop_in`/`ztop_out`, `z_in`/`z_out` | Pixels into and out of the early and late Z tests |
| `blend_in` | Pixels into the blender |
| `copy_clks` | EFB copy clocks |
| `fifo_overflows` | libogc FIFO overflows: each suspends the CPU until the GP reads the FIFO down |

`perfProf_gameBegin` selects the counters with three GX commands
(`GX_InitXfRasMetric`, `GX_ClearPixMetric`), once per game from `loadROM`.
After that the probe only reads registers, at the existing 500 ms sample and
at the game end: no GX commands and no waits. The 32-bit counters wrap after
17.7 s, so 500 ms deltas are summed into 64 bits, as for the PMCs. The control
build has the counters too, so it measures the GP without CPU probe slowdown.
libogc2 documents the XF/RAS meanings with question marks: trust a run only
when `clks` matches `tb_clks`. Dolphin returns 0 for the XF/RAS counters, and
the report then says "not valid in this run". `subsystem_report.py` prints
the GP section for control runs as well, with a first verdict: a GP with
spare time that never overflows the FIFO means CPU-side graphics limits the
scene. Use the CPU spans (`gfx_list`, `vertex`, `gfx_state`, `tex_*`,
`draw_*`) to find which part.

Full-probe glN64 builds also split `gfx_command` by GBI opcode: one
`gfx_opcode:` line per command seen, with exact calls and the stage's own
1-in-127 samples, and a `gfx_ucodes:` mask of microcode types. The report
names the opcodes for the microcode family and ranks them. Results and the
ranked optimization targets: [GPU survey](gpu-results-2026-10-02.md).

Run the targeted CPU/graphics/VM chain, or the 3D GPU chain, with
full/control/full probes:

```bash
bash .dev/test_subsystems.sh
bash .dev/profile_subsystems.sh subsystem_gaps
bash .dev/profile_subsystems.sh gpu_survey
```

To measure just self-attribution overhead, use the same inclusive probes in
both builds, with `-DPERF_SUBSYSTEM_SELF=1` and `=0` respectively through
`WII64_SURVEY_FULL_FLAGS`/`WII64_SURVEY_CONTROL_FLAGS`; set
`WII64_SURVEY_SAME_PROBES=1`. The normal disabled control measures the full
subsystem probe cost. Match schema version and interval; do not reuse an old
inclusive baseline as the new control or subtract a universal probe percentage.

### Schema 2 hardware validation — 2026-10-01

The `subsystem_gaps` chain completed full/control/full on Wii and returned to
Homebrew Channel after each run. All eight full-probe game rows had zero dropped
self samples; each sampled self total was bounded by its inclusive total.
The same four-game chain passed in muted Dolphin with MMU and DSP LLE.
Rice's 900-VI SM64 smoke test, a 300-VI pure-interpreter test and an ordinary
release boot passed without invalid-access or DSP warnings. The release ELF
has no subsystem timer symbols. Host C/C++ sanitizer and report/workflow tests
passed, including injected IRQs, nested/separate exclusions and reentry recovery.

Tracked results:
[full 1](../baselines/2026-10-01_hw_subsystem_schema2_full1/perf.log),
[control](../baselines/2026-10-01_hw_subsystem_schema2_control/perf.log),
[full 2](../baselines/2026-10-01_hw_subsystem_schema2_full2/perf.log).
Each baseline retains config, replay traces and artifact hashes. Frozen source,
build flags and binaries remain in `.dev/runs/subsystem-survey-20261001-121502-83R1`.
Full DOL SHA-256: `82411fe55c292dba0c87316a9eb598a7364102e7a9d22fa4123311904d4d289f`.
Zero-trace games retain no trace file in these baselines: older SD files can
remain after a run. The importer now uses each game row's trace count.

Full-run mean versus disabled control; these are probe costs, not gameplay gains:

| Scene | CPU cycles | Non-sleep wall | Total wall |
|---|---:|---:|---:|
| Super Mario 64 | +20.98% | +19.53% | +0.02% |
| Mario Kart 64 race start | +9.28% | +9.32% | +0.51% |
| Pokémon Snap intro | +18.12% | +17.64% | +0.54% |
| Donkey Kong 64 intro | +18.58% | +16.70% | +0.18% |

Limiter sleep hides CPU cost. Full-probe underruns were 105/91 versus 74 in
the Kart control and 4/2 versus 0 in Snap. Keep ordinary performance comparisons
on disabled probes; use full probes for diagnosis. One triple is not a universal
overhead correction.

New self-time estimates below use the full-run mean, except execute, which
shows both repeats. A `wall/non-sleep` pair uses those two denominators.

| Scene | Execute self, wall %, full 1 / full 2 | Memory self, wall/non-sleep % | Dispatch self, wall/non-sleep % |
|---|---:|---:|---:|
| Super Mario 64 | 22.80 / 34.00 | 1.91 / 5.04 | 11.56 / 30.50 |
| Mario Kart 64 | 28.12 / 29.40 | 1.29 / 1.68 | 8.90 / 11.57 |
| Pokémon Snap | 36.45 / 35.92 | 3.03 / 3.58 | 15.00 / 17.73 |
| Donkey Kong 64 | 23.69 / 33.61 | 2.33 / 5.95 | 7.45 / 18.98 |

SM64 and DK64 execute estimates show substantial repeat variation despite
stable CPU totals. Confirm those spans with another sampling interval before
ranking them. Do not sum sampled self estimates into a CPU budget. Graphics
remains the largest fully timed non-sleep workload in Kart and Snap.
Storage/save/state counters were inactive in gameplay; startup resets and
disabled autosaves explain that result, not missing probes. These runs do not
validate save writes, GPU execution time or separate inline load/store cost.

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
bash .dev/profile_texture_hash.sh graphics_survey
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
ordinary builds. Run `.dev/profile_subsystems.sh graphics_survey`
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
