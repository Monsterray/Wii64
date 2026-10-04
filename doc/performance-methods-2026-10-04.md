# Performance statistics: WiiStation comparison

The opt-in profiler is implemented in 1.6.13; its initial calibration is below.
It is instrumentation, not an emulator speedup.
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

The first prototype uses only the emulation thread. libogc2 saves/restores its
MSR on context switches; newly created and idle threads have PM clear. This
avoids private thread enumeration. Background networking, audio callbacks,
interrupt-disabled work and idle time are not independently measured. Do not
label these samples whole-system CPU use.

`main/hprof.c` saves and restores EX_PERF, MMCR0/1, PMC1–4 and the main thread's
PM bit with interrupts disabled. The binary records restoration checks.
It allocates one 128 KiB MEM1 histogram through `memalign`, including in controls;
no fixed MEM2 region or JIT capacity changes. Allocation failure disables
sampling and the decoder rejects that run. The integer-only exception entry
follows Wii64's existing `vm/dsihandler.s`, using installed libogc2 offsets.
It starts at the first VI and stops when `go()` returns, including HOME exits.
An interactive HOME/resume session is not a full-window calibration chain.
Files are written only after stopping, then uploaded through the existing runner.

### Run the bounded calibration

```bash
source .dev/env.sh
bash .dev/build_profiling.sh glN64_wii 'DEBUG_FLAGS=-DPERF_PROF -DPERF_HPROF'
WII64_SURVEY_QUEUE_ONLY=1 bash .dev/profile_pc.sh wii64-glN64.dol pc_sampler
bash .dev/profile_pc.sh --collect /absolute/path/printed/by/the/driver
python3 scripts/hprof_view.py /path/to/hprof_01.bin /path/to/frozen/build.elf
```

Pass `HBC_AGENT_ROOT=/path/to/hbc-reborn` to the build and set `WII64_HBC_ROOT`
for the queued client if the SDK is outside the default sibling checkout.
The driver freezes one DOL/ELF, chain variants, source hashes and compiler
version. Controls use `hprof=0`, the sampler `hprof=1`. All reserve the same
histogram. Detailed subsystem timers stay off. The receiver accepts only
`hprof_NN.bin` names, alongside its existing narrow allowlist.

The decoder rejects malformed geometry, lifecycle/allocation failures,
counter overflow, inconsistent totals and wrong lengths. It checks ELF text
bounds and prints the ELF SHA-256; bounds alone are not proof of an exact match.
Use the frozen artifact hashes, not a later rebuilt ELF. JIT remains an aggregate.
Native buckets are approximate. Boundary/overlap bins remain separately marked
with their address and a maximum-overlap hint; they are not exact function totals.

Dolphin checks handler execution, restoration, chains and file transport, not
Broadway gating or cost. Dolphin's [performance monitor source](https://github.com/dolphin-emu/dolphin/blob/23e8a3c569a4db3b48b93dd36ef37eef3030c76c/Source/Core/Core/PowerPC/PowerPC.cpp)
(`UpdatePerformanceMonitor`) increments
cycle counters without checking FCM0/MSR[PM] in the inspected source. The test
therefore samples idle too. A 64-byte bucket straddling `__lwp_thread_coreinit`
and `idle_func` dominated the functional test; treating its maximum-overlap
hint as a hot emulator function would be wrong. Do not use Dolphin rankings.

### Initial validation, 2026-10-04

Host ASan/UBSan histogram/subsystem/ROM-VM tests and offline queue/decoder gates
passed. Two-ROM Dolphin sampler and control chains reached their VI targets,
with host playback muted and no invalid-access/DSP/SD-sync warnings. Assembly
inspection found `hprof_sample` integer-only and leaf, with ordered SPR writes.
The exception ABI was checked against libogc2 source
`43f26d22deaacae287c2f8d96eba00dba1ee0155`, `libogc/exception_handler.S` and
`libogc/lwp_handler.S`. This does not establish ABI compatibility with older SDKs.

The queue ran control/sampler/control with one frozen glN64 DOL/ELF and HBC
SDK 1.9.4 (`0b214c7a78d81d87f4a2b24e53feb3c967e26d5c`). Every run reached
900 SM64 VIs and 3600 Kart VIs, uploaded both histograms and returned to HBC
1.9.3. No power-off occurred. Sampler restoration checks passed for both games;
controls had zero samples. All six game rows had 8 underruns and 0 overruns.
Existing audio gaps remain; this is not an audio correctness/listening test.

| Scene | Control non-sleep ms, mean | Sampler ms | Added estimate | Control drift | Samples |
|---|---:|---:|---:|---:|---:|
| SM64 file menu | 4201.592 | 4209.071 | +0.18% | 0.08% | 3849 |
| Kart race grid | 22915.463 | 22922.870 | +0.03% | 0.02% | 22398 |

Control PMC cycle drift was +0.0048% / -0.0015%, respectively. This is control
repeatability, **not sampler cycle overhead**. Both scenes pass the initial
<=3% requested-non-sleep wall-cost target. The strict overall calibration
**fails guest exception-count parity**, and the driver returns nonzero:

| Scene | Control 1 exceptions | Sampler | Control 2 |
|---|---:|---:|---:|
| SM64 | 5594 | 5593 | 5592 |
| Kart | 25080 | 25100 | 25065 |

The controls themselves differ. `exception_general` counts multiple causes;
this aggregate does not distinguish interrupt timing variation from a state
error. Kart's sampler also submitted 76,616 batches versus 76,811 in both
controls (0.25%); its final grid capture looked equivalent. Keep the strict
failure visible rather than relaxing the threshold to claim full validation.
SM64 and Kart final captures were inspected; both had expected scene content.

First-pass sampled leads (not accepted optimization rankings): JIT 53.57% in
SM64 and 31.86% in Kart; Kart YUYV conversion 23.33%, texture CRC 6.23%.
Boundary bins accounted for 6.24% / 5.54%. Repeat at another period and resolve
guest-work stability before acting on these shares. The two-game trial does
not establish safety across the complete ROM library or both renderers.

Frozen survey: `.dev/runs/pc-survey-20261004-083611-tGLD`. Queue IDs end
`abc0df`, `5b45ce`, `09f297`. DOL SHA-256:
`3e7bddab6a9242f8a156784d3eb37599ece99f75ad174e6f80030cc8526c8772`.
ELF SHA-256: `2c16f056604fc6dceeda2928847c68e96f9f30e7c584b7fae61b6ca4f8e3a941`.
The run directories retain configs, hashes, health checks and raw histograms;
ROMs and captures remain untracked.

Next bounded experiment: repeat these scenes with one matched sampler-capable
binary that adds light per-cause guest-exception counts, to explain Kart's
mismatch before accepting rankings.

The final clean sampler DOL matched the Wii-tested frozen DOL byte for byte.
The ordinary release was clean-built afterward with no sampler symbols; a
muted 25-second Dolphin smoke had no invalid-access/DSP/SD-sync warnings.
Use the frozen ELF for this survey, not the restored release ELF.

### Local-model checks

Muse Glimmer/Gemma4 reviewed the lifecycle proposal in 52.7 seconds;
Qwen3-Coder/Devstral reviewed the actual source/diff in 20.6 seconds.
Each had a 10,000-token cap and 300-second timeout; all four completed without
truncation through Ollama (vLLM was offline). Review findings were checked
against source, tests and assembly. Claims that MSR leaked across thread
contexts, guarded `memset` dereferenced NULL, or 64-bit layout arithmetic
overflowed at 32 bits were unsupported and rejected.

Local-LLM improvement: require a quoted source line plus a concrete failing
input or execution trace for each finding. Returning structured evidence and
verified usage/truncation fields once, rather than duplicated result payloads,
would reduce review cost. A large token cap prevents truncation, not false claims.

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
