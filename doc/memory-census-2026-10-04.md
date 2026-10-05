# Memory census: unchanged layout

This records the initial unchanged-layout prototype. The
[completed measured pass](memory-results-2026-10-04.md) has the corrected
full-library census and the validated 768 KiB boxart reservation.

Phase 1 of the [memory plan](memory-optimization-plan-2026-10-04.md) adds
optional measurements. It does not reclaim memory or change cache placement.

## Measurement boundaries

Build with `PERF_PROF` and `PERF_MEMORY`; select `memory=1` in the diagnostic
config. Ordinary release builds call the native allocator directly and contain
no census hooks. Runtime `memory=0` uses the same profiling executable with
accounting disabled. That control is not an ordinary release benchmark.

The five pools are generated code, recompilation metadata, cache nodes, glN64
textures and browser boxart. Allocation/free hooks record actual native block
charges, peaks, counts, failed requests and reset counts. They also record JIT
capacity evictions and texture bytes awaiting GX completion. They do not read
clocks, walk heaps, allocate memory or write logs. IRQ exclusion keeps texture
retirement accounting consistent with draw-sync callbacks.

Stopped-context snapshots run at boot, load entry, teardown, loaded state,
game end and the end of each browser stress cycle. A bounded physical-block
walk checks allocation accounting and records free block count and largest
free block. Largest free is a raw block charge, not a guaranteed allocation
payload. Peaks and operation counts are cumulative across ROM transitions;
live bytes reset when a heap is reinitialized. These counters do not establish
a whole-library capacity bound.

Keep private-heap free space separate from newlib free space and unallocated
Arena1/Arena2. None reports newlib's largest allocatable block. Do not add these
values together and promise an allocation of that size.

## Reusable commands

Use the repository's existing toolchain and matching HBC agent SDK setup.
Clean builds are mandatory when changing probe flags. To build and queue a
same-executable off/on/off run:

```sh
bash .dev/build_profiling.sh glN64_wii \
  'DEBUG_FLAGS=-DPERF_PROF -DPERF_MEMORY'
WII64_SURVEY_QUEUE_ONLY=1 bash .dev/profile_memory.sh
```

The wrapper uses `scripts/chains/memory_census.txt`: SM64 file menu, Kart race
grid, then SM64 again. This is a transition test, not validated racing or a
whole-library workload. The existing queue owns the lease and returns each
run to HBC. The existing freezer records source, compiler and agent hashes
along with the DOL, ELF and configs. Do not rebuild or edit frozen artifacts.

Collect the directory printed by the queue command:

```sh
bash .dev/profile_memory.sh --collect /absolute/path/to/printed/survey
```

The report rejects changed executables/settings, incomplete VI targets,
paging I/O errors and invalid heap accounting. Calibration additionally needs
exact guest-work parity, at most 1% control drift in CPU cycles and non-sleep
wall time, at most 3% additional probe cost, and no probe audio-underrun/overrun
increase above both controls. Failed calibration preserves the census but
makes timing results provisional. Lifecycle snapshot/log cost is outside the
game timing window; compare job duration and log-flush cost separately.

For functional browser testing in an isolated, host-muted Dolphin profile:

```sh
WII64_DOLPHIN_PROFILE="$PWD/.dev/dolphin_memory" \
WII64_DOLPHIN_MUTE_AUDIO=True \
bash .dev/dolphin_test.sh wii64-glN64.dol 25 \
  memory=1 memory_boxart_probe=1 autonav=selectrom_sd stress_selectrom=3
```

`memory_boxart_probe=1` is independent of runtime accounting. Before UI buffers
exist, it initializes the native boxart heap at 768 KiB, allocates all 16
46,080-byte textures, checks 32-byte alignment and distinct data patterns,
then frees them. It restores the configured boxart heap on either result. The
temporary test allocations are not included in census peaks.

For the owned-ROM library and HOME checks, keep the frozen census DOL and
matching ELF together, then run:

```sh
WII64_LIBRARY_MEMORY=1 WII64_DOLPHIN_MUTE_AUDIO=True \
bash .dev/library_check.sh /absolute/path/to/build.dol both
bash .dev/test_memory_home.sh /absolute/path/to/build.dol
```

The library wrapper freezes ROMs/configs and queues hardware while Dolphin
runs separately. Set `WII64_ROM_DIR` to the owned-ROM folder if needed.
The HOME wrapper owns its lease, records stopped enter/active/closed snapshots,
resumes, and waits for HBC return. Do not use its pause time as a speed result.

## Initial evidence

Host/subsystem tests include sanitizer-backed synthetic PPC allocator geometry,
failed requests, retirement, reset, bounded walks, report rejection cases and
the frozen off/on/off queue workflow. The native allocator itself is checked
by the boxart probe, not by the host mock. ROM/VM regression tests passed.

Clean glN64 and Rice Wii census builds passed with devkitPPC GCC 16.1.0 and
libogc2 `43f26d22deaacae287c2f8d96eba00dba1ee0155`. The frozen native chain uses
the existing HBC agent SDK 1.9.4 archive; the HBC menu reports 1.9.9. Do not
confuse those two versions or rebuild the archive against unrelated headers.
The ordinary glN64 clean release build passed and its ELF has no `perfMem_*`
symbols. Its muted-host SM64 smoke had no invalid-access, DSP or SD-sync warnings.
Rice's enabled-census SM64 Dolphin smoke also completed its VI target
without invalid-access, DSP or SD-sync warnings. GameCube linking remains
limited by this Mac's existing missing GameCube libfat; vWii is not tested.

Dolphin completed the three-entry glN64 transition chain without invalid
access, DSP or SD-sync warnings. Its captures show SM64 at file selection and
Kart at the race grid. This proves neither extended gameplay nor audible
quality. Browser stress completed three cycles: 48 allocations, 48 frees,
737,920-byte charged peak, zero final live bytes and zero accounting errors.
The native 768 KiB bound probe passed in Dolphin and on Wii. It leaves 48,504
charged bytes unused in that temporary heap; no reservation has changed.

Initial native enabled-chain peaks:

| Pool | Charged capacity (bytes) | Charged peak (bytes) | Failed requests |
|---|---:|---:|---:|
| Code | 9,961,464 | 4,631,848 | 0 |
| Metadata | 4,194,296 | 923,040 | 0 |
| Nodes | 393,208 | 164,576 | 0 |
| glN64 texture | 15,728,632 | 5,650,576 | 0 |

There were no JIT capacity evictions or heap-accounting errors. Boxart remains
unallocated during this autoboot chain, so use the separate browser test for
its peak. Texture retirement was not exercised by these scenes; zero retired
bytes do not prove that GPU lifetime overlap is safe.

All three native runs completed their three-entry chains, passed VI/replay
validation and returned to HBC. The final calibration failed guest-work parity
for all entries. The two controls themselves differ in exceptions, recompiles
and render work, consistent with the project's existing replay/parity
limitation. This is not proof of an emulation regression or acceptable probe
overhead. Kart also had 11 probe underruns versus 8 in each control; all runs
had zero overruns. Do not suppress that signal because the timing cost is small.

| Entry | Probe CPU-cycle change | CPU control drift | Probe non-sleep change | Non-sleep control drift |
|---|---:|---:|---:|---:|
| SM64 first | +0.037% | -0.037% | +0.270% | +0.018% |
| Kart | +0.030% | +0.029% | +0.408% | +0.025% |
| SM64 repeat | +0.050% | -0.011% | +0.812% | +0.441% |

These are provisional calibration values, not confirmed overhead or speed
improvements. Job durations were 125/129/128 seconds; game-report log-flush
cost was 250.6/326.0/258.6 ms. The source/hash manifests and raw results stay
ignored under `.dev/runs/pc-survey-20261004-155156-CPMk`; no ROMs, raw logs or
captures belong in the commit.

Do not resize the hot heaps from these small-scene peaks. Do not claim a speed
improvement from the census.

## Next bounded experiment

Stabilize one short replay's guest work and repeat the frozen off/on/off
calibration. Keep the current reservations and allocator. Only after that
gate passes should broader gameplay/library census results drive pool sizing.
The independently bounded 256 KiB boxart reclamation remains a candidate, not
an implemented saving.

## Local-model review

Muse Glimmer, Gemma and Qwen reviewed design/diffs with 10,000-token caps;
Muse also checked disputed allocator claims in a four-item parallel batch.
Ollama was available; vLLM was offline. Source inspection and native tests
rejected incorrect claims about heap-init return values, dummy-block geometry
and capacity arithmetic. Successful completion is not factual reliability.
The final Muse/Qwen evidence court completed in 23.8 seconds with the same caps;
its claim of non-intrusiveness is not accepted without passing calibration.
Local-llm should require source excerpts and fact/hypothesis separation, and
return compact job metadata without duplicated result payloads.
