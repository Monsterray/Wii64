# Performance statistics: WiiStation comparison

This is a source-backed design, not an implemented profiler or a speedup claim.
Wii64 starts at `b2f16fe`, version 1.6.12. WiiStation's `main` is pinned to
`d3dc67af50750dc9c771a69cc30a3a0276b47627` for this comparison.

## Why change the method

The [current Wii survey](hardware-validation-2026-10-04.md) finds 12.9–16.2%
added CPU cycles from full subsystem probes. Execute/helper estimates vary
despite repeat cycle totals within 0.65%. Limiter-bound total wall hides this
cost. In [perf_subsystem.c](../main/perf_subsystem.c), skipped hot calls still
update counters. Sampled roots force clocks on their excluded children, with
IRQ masking and attribution work. Increasing the call interval alone does not
remove all that work. The existing report already warns against summing nested
spans; preserve that warning, not an apparent whole-system pie chart.

WiiStation offers two useful mechanisms: separate probe presets and a hardware
PC sampler. Its [build script](https://github.com/Monsterray/WiiStation/blob/d3dc67af50750dc9c771a69cc30a3a0276b47627/scripts/build.sh)
separates `min`, `light`, `deep`, `pmc`, `hprof` and trace builds, and cleans
when probe flags change. Wii64 already freezes clean probe/control builds;
extend that workflow rather than replace it.

## Recommended measurements

| Method | Question answered | Wii64 action | Limit |
|---|---|---|---|
| Low-cost counters and PMC totals | Is the same scene doing less CPU work? | Keep existing control runs, cycles/instructions, guest counters and error counts | This is not an uninstrumented release; PMC event configuration must match |
| Hardware PC sampling | Which native functions or generated code occupy the CPU? | Add an opt-in Wii sampler, starting at 1 ms | Sampled residence, not parent inclusive time or sleeping wall |
| Coarse timed spans | How long do RSP tasks, storage and GX waits take? | Reuse tick-based probes, selecting only the group needed | Inclusive; may include interruption and waits |
| VI/present interval distributions | Where does visible stutter occur? | Add bounded histograms only after sampler calibration | Existing 500 ms windows cannot produce per-frame p95/p99 |
| GP and DSP observations | Is hardware busy, starved or waiting? | Keep current GP clocks/XF/raster/copy/FIFO and DSP metrics | FIFO count is not stall duration; DSP observations are not audio CPU time |
| Scene and guest-work checks | Did the run actually reach the same workload? | Reuse hashes, replays, guest counters and inspected captures | A completed VI target or nonflat frame is not full compatibility |

For ordinary A/B tests, use the light control. For CPU ranking, use the sampler.
For a specific mechanism, use a targeted detailed build. Do not enable every
timer just because one hardware session can collect many counters. Counters
and bounded event records can be plentiful without timing every operation.

## What WiiStation's sampler actually does

[hprof.c](https://github.com/Monsterray/WiiStation/blob/d3dc67af50750dc9c771a69cc30a3a0276b47627/Gamecube/hprof.c)
uses PMC1 overflow at 72,900 cycles (100 us on a 729 MHz Wii). Its exception
entry records the interrupted PC in 32-byte buckets, with generated-code and
unknown totals. It also has a second histogram of LR for a selected divide-helper
address range, not a general call graph. The two 4 MiB-span histograms consume
1 MiB. Its [decoder](https://github.com/Monsterray/WiiStation/blob/d3dc67af50750dc9c771a69cc30a3a0276b47627/scripts/hprof_view.py)
resolves buckets against the matching ELF, assigning each to the symbol with
the greatest byte overlap. Short functions and boundary buckets remain approximate.

The [header](https://github.com/Monsterray/WiiStation/blob/d3dc67af50750dc9c771a69cc30a3a0276b47627/Gamecube/perf_prof.h)
quotes about 0.2% cost. No matched overhead data supporting that figure was
retrieved here; do not promise the same cost in Wii64.

WiiStation's [5651800 fix](https://github.com/Monsterray/WiiStation/commit/5651800da5c7c09f921e00941a382a8481450980)
records real-Wii crashes when sampling overflowed in the idle thread or was
rearmed without ordered SPR writes. It freezes counters in idle using MSR[PM]
and disables the performance interrupt before rearming, with `isync` ordering.
Spyro and the FMV chain completed afterward. These are required safety lessons,
not evidence that its exception entry is ABI-compatible with Wii64's SDK.

## Smallest safe adaptation

1. Start with PC buckets plus generated-code and unknown totals. Do not allocate
   the optional LR histogram until helper samples justify it. Size the native
   span from linker text boundaries rather than copying WiiStation's 4 MiB limit.
   Start with a 128 KiB cap and choose bucket width to cover that span; report
   the resulting resolution. Do not silently truncate executable addresses.
2. Publish the recompilation heap's actual base and size once at initialization.
   [Recomp-Cache-Heap.c](../r4300/Recomp-Cache-Heap.c) allocates one code heap;
   its blocks can move and be reused inside that region. Aggregate JIT-region
   sampling needs no tree walk. Do not map a recycled native PC to a guest
   function after the run without lifetime records. Separate fine JIT mapping
   is deferred until aggregate results justify it.
3. Use a mutually exclusive sampler mode: current [PMC accumulation](../main/perf_prof.c)
   must not reset or read the reloaded PMC1 as an aggregate cycle counter.
   Do not assume spare PMC event assignments work without checking them.
   Measure aggregate cycles in matched control runs instead.
4. Audit the [exception entry](https://github.com/Monsterray/WiiStation/blob/d3dc67af50750dc9c771a69cc30a3a0276b47627/Gamecube/hprof_entry.s)
   against current libogc2 and generated assembly. Keep the handler integer-only;
   its entry does not save FP state. No allocation, file/network I/O, locks,
   symbolization or cache/tree queries in the handler.
5. Validate histogram storage against both the Wii64 MEM2 map and runtime arenas.
   Do not steal the HBC record gap, ROM/texture storage or recompilation capacity.
   A fixed write into nominal unclaimed MEM2 is not automatically safe.
   The current glN64 control reports 288,704 bytes free in Arena2, and the Rice
   smoke reports 573,376. WiiStation's 1 MiB pair will not fit those remaining
   arenas. These snapshots are not a guarantee of safe allocation during all
   ROM transitions; retain explicit allocation failure and layout checks.
6. Preserve/restore the previous EX_PERF handler, PMC configuration and relevant
   thread MSR state. Audit idle and new-thread behavior for this SDK rather than
   copying private LWP internals unchanged. Freeze sampling before reset/dump;
   start at the first VI and stop before chain transition and HBC return.
   Keep VM DSI and fatal recovery working. Allocation failure disables sampling.
7. Extract the binary with the existing leased test runner after stopping emulation.
   Keep matching ELF/DOL hashes and settings. Reject malformed headers, impossible
   shifts/counts, wrong lengths and inconsistent totals in the host decoder;
   retain unknown/ambiguous attribution. Use `.dev/env.sh` tool paths on both hosts.
   A new binary-format framework or CRC layer is not needed for the first prototype.

These are design gates. No sampler, extraction path or new preset was added in
this session. Keep queued launchers and frozen artifacts unchanged.

## Denominators and confidence

- Wall time: first guest VI to the requested stop, separately from ROM preparation.
- Non-sleep wall estimate: wall minus **requested** limiter sleep. Wii64's
  [timer](../main/timers.c) records the argument to `usleep`, not actual scheduled
  sleep. This estimate still includes waits/interrupts and is not CPU busy time.
- Sample share: a function's PC samples divided by all eligible samples, retaining
  JIT and unknown totals. Real-Wii idle is excluded by WiiStation's mechanism;
  limiter spinning and blocked-but-clocked code are not automatically useful work.
- PMC totals: label the selected event, hardware and run. Never reconstruct total
  cycles as sample count times period: sampling delays and rearming lose information.
- GP utilization: GP busy clocks divided by valid GP clocks; require the existing
  timebase consistency check. It can overlap CPU execution.

MSR[EE]-off work is attributed late by this sampler. Periodic samples can alias;
changing the period and repeating scenes tests sensitivity, not guaranteed lack
of bias. Compare distributions across repeated windows before presenting rankings.
Do not treat adjacent samples as independent evidence for tight confidence bounds.
Report guest VI rate, actual presents, audio gaps, and their timelines separately.
Do not normalize nested timers to 100% or add CPU, GP and DSP percentages.

## Ranked work and next experiment

| Priority | Work | Evidence | Risk | Smallest check |
|---|---|---|---|---|
| P0 | Keep light controls as performance authority | Full probes add 12.9–16.2% cycles | Low | Existing matched control repeats |
| P1 | Opt-in native PC sampler | WiiStation has real-Wii function/JIT sampling | Exception and SDK integration | ABI/layout tests, then matched sampler/control run |
| P1 | Select only the detailed probe group needed | Forced child timing remains costly | Missing coverage if mislabeled | Existing host scope tests plus explicit enabled-group manifest |
| P1 | Add per-VI/present interval histograms | Current half-second windows hide short stalls | Boundary cost and clock interpretation | Known synthetic intervals and measured probe cost |
| P2 | Selected LR or fine JIT attribution | Aggregate results may identify helper/JIT dominance | LR assumptions and code reuse | Add only after the initial PC histogram identifies a target |

Next experiment: build a minimal opt-in **1 ms PC/JIT/unknown sampler**, after
the ABI, state-restoration and layout checks, then run control/sampler/control
on the existing SM64 and Kart scenes. Prefer the same sampler-capable binary
with sampling enabled/disabled, reserving the same histogram storage in both
modes so memory pressure does not become the variable. Freeze source, SDK,
settings, replays and matching ELF/DOL. Measure non-sleep wall, guest-work parity,
audio/VM health and HBC return; aggregate PMC cycles come from controls, not the
sampler's PMC1. Cycle overhead remains unknown unless a separate compatible
cycle event is validated; sample count is not a substitute. Treat <=3% added
non-sleep wall as the initial acceptance target, not a guaranteed CPU-cycle cost.
Reject crashes, state divergence or excess overhead before trying 500/100 us
and noncommensurate periods. No CPU optimization from uncalibrated new stats.

Local research used Muse Glimmer and Gemma4 concurrently with 10,000-token caps
and 300-second timeouts (69 seconds, neither truncated). Source review rejected
their incorrect busy-time definitions, confusing time-based sampling with the
current call interval, and suggestions to sum overlapping timer estimates.
Qwen3-Coder and Devstral independently reviewed the proposal with the same caps
(42 seconds, neither truncated). Their unsupported denials of supplied source
facts and observed measurements were rejected; model agreement is not proof.
