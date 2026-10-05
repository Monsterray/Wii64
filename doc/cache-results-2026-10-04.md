# Cache ownership experiments

Baseline: master `4cb4fe7d8a43ad3e787910607c8a9d1d4133c211`, version
1.6.15. The retained diagnostic code is released as 1.6.16. It does not
change production memory aliases or publication rules.

## Implemented measurement

- `PERF_PROF` plus `PERF_CACHE_PROBES` enables texture-load, mip-chain and
  framebuffer-copy counters. The mip timer samples calls 1, 128, 255, etc.
  Allocation failures and fallback returns close the sampled timer.
- `cache_probe=1` additionally runs a private GX ownership test at ROM open
  and samples the Mario Kart cached XFB reader every 127 calls. It captures
  the buffer pointer once, waits for GX completion, then compares eight
  cached words with physical RAM. It does not invalidate or write live XFBs.
- The private test owns two aligned cache lines. GX copies into the first;
  the second is a guard. Seed publication and clean-cache warming precede
  the second copy. Payload and guard are refreshed after GX completion and
  before inspection/free. It does not clear or write the EFB.
- `cache_probe=0` disables the private test and XFB witness, not compiled-in
  texture/framebuffer counters. A build without `PERF_CACHE_PROBES` removes
  all these hooks. Ordinary release builds contain no cache-probe symbols.
- The report rejects incomplete, malformed or inconsistent records and
  private-test fresh-data/guard failures. Missing legacy framebuffer counters
  are shown as **not recorded**, never zero.

The XFB witness adds a diagnostic wait and sparse reads. Its residence time
is not a subsystem wall-time share or an optimization result. It samples
only eight words per check; it cannot establish whole-frame correctness or
prove that the selected buffer is the desired completed frame.

## Initial native and Dolphin results

Both platforms completed SM64 / Mario Kart race-grid / SM64, with targets
900 / 3600 / 900 VIs and deterministic interrupt/replay settings. The native
Mario Kart capture shows the race grid, not a black frame.

| Measurement | Real Wii | Dolphin |
|---|---:|---:|
| Private GX test, each of three ROM opens | 8 stale words, 0 fresh errors, 0 guard errors | 0 stale words, 0 fresh errors, 0 guard errors |
| Mario Kart live readback | 1,202 calls, 10 checks, 0 stale/changed checks | 1,202 calls, 10 checks, 0 stale/changed checks |
| Native witness residence | 125 us total across 10 checks | Not a physical-cache measurement |
| Mip-chain calls in these scenes | 0 | 0 |

The private test establishes physical cache noncoherence. It does **not**
establish a defect in the current XFB reader. Dolphin's zero stale words on
the deliberately warmed private buffer demonstrate why it is not the cache
performance/correctness authority.

## Bounded library coverage

All 18 owned ROMs completed their scene targets on both platforms. Native
work used three six-entry batches. Dolphin completed the first 14 entries
before its host time budget stopped the long chain during entry 15; entries
15–18 then completed in a separate retry. The failed original run remains
in the local artifacts. It is not counted as an 18-entry success.

| Library measurement | Real Wii | Dolphin |
|---|---:|---:|
| Complete distinct ROM scene targets | 18 | 18, across initial run and retry |
| Mip-chain calls / levels / staged bytes | 0 / 0 / 0 | 0 / 0 / 0 |
| CPU-reused framebuffer copies / bytes | 0 / 0 | 0 / 0 |
| Mario Kart XFB checks / stale checks | 10 / 0 | 10 / 0 |
| Private GX test at ROM open | 18 × (8 stale, 0 fresh errors, 0 guard errors) | 18 × (0 stale, 0 fresh errors, 0 guard errors) |
| Final captures | 18 valid non-flat frames; selected frames inspected | Captures retained; selected Zelda frame inspected |

These are title/intro and bounded replay scenes, not whole-game
compatibility, active-mip coverage or a full audio-quality test. Native
wall-speed results range from 98.57% to 99.60%, including cold work and log
I/O. Do not label them strict 100% passes. No production speed gain is claimed.
The zero traffic closes the proposed staging/publication experiments for
these scenes: there is no measured work to remove. Later in-game traffic
would require its own active-path and overhead qualification.

## Probe overhead: native A/B/A

A is the counter build with `cache_probe=0`; B is `PERF_PROF` without
`PERF_CACHE_PROBES`; the second A reuses the identical frozen A binary.
Each row has equal VIs, guest exceptions, recompiles, batches and vertices.
Compare the mean of both A values with B, not two different game workloads.

| Scene | Probe/control PMC1 cycles | Probe/control non-sleep less log flush |
|---|---:|---:|
| SM64 first entry, 900 VIs | +0.578% | +0.339% |
| Mario Kart race grid, 3,600 VIs | -0.325% | -0.326% |
| SM64 reload, 900 VIs | +0.179% | +0.117% |

Classification: **neutral/noise in this run**, not a speed improvement or a
universal overhead bound. Non-sleep is wall minus limiter sleep and measured
log-flush time; it includes other work, not only emulator CPU kernels.
Underruns vary by 0–2; no overrun occurred. These scenes do not exercise mip
timing, so active-mip probe overhead remains unqualified.

## Candidate decisions

| Rank | Candidate | Type | Evidence / expected gain | Risk / scope / test |
|---|---|---|---|---|
| Implemented | GX ownership witness | GUARD | Reproduces 8 stale words on Wii; diagnostic capability, no FPS gain | Private allocation only; host stale/overrun model, native/Dolphin checks |
| P2 | XFB completion/identity change | BUG | No sampled stale read reproduced; gain unknown | Wrong-buffer or producer race; needs a failing scene before a fix |
| P2 | Avoid intermediate mip publication | PERF | Zero traffic across the 18 bounded library scenes; expected gain there is zero | Allocation/eviction/fallback lifetime; extracted production tests cover current packing and failures |
| P2 | Store instead of flush CPU-reused framebuffer output | PERF | Zero copy traffic in these scenes; requires an active scene and a later-writer audit | Guest RDRAM has other writers; preserve current publication |
| REJECT | Broad K1 texture/output conversion | PERF | No Wii64 evidence of a device-only hot output phase | Loaders use cached `dcbz`; filtering/packing read data; cannot apply a scalar-copy microbenchmark wholesale |
| REJECT | K1 audio, JIT, guest RDRAM or IOS read-ahead defaults | PERF | Existing ownership audit finds CPU consumers or SDK maintenance | Uncached reads, stale aliases and device lifetime; no changes |

No production cache-operation removal, blanket invalidate, new per-frame
wait, LC activation, SDK change or uncached allocation is retained.

## Reproduce

Use the matching external HBC SDK recorded in the build/test skill. Clean
when changing flags:

```sh
.dev/build_profiling.sh glN64_wii 'DEBUG_FLAGS=-DPERF_PROF -DPERF_CACHE_PROBES' HBC_AGENT_ROOT=/path/to/matching/hbc-reborn
WII64_LIBRARY_CACHE=1 .dev/library_check.sh /absolute/path/to/frozen.dol both
python3 scripts/cache_probe_report.py /absolute/run/perf.log
```

The library driver freezes inputs and divides native work into six-ROM
batches. Do not send the 18-entry `cache_coverage.txt` directly with
`hardware_run.sh`: it exceeds `wiiload`'s argument buffer. That failed launch
never reached emulation. Dolphin can stage the complete file on its SD image.
The queue remains the only native launch path and returns to HBC.

Host checks execute the production mip-chain and timer with fake allocator
and loader boundaries under ASan/UBSan. They verify every level-failure
position, final-chain failure/retry, normal fallback, padded byte identity,
reset and timer cleanup. They do not emulate PPC cache hardware.

Local artifacts remain in `.dev/runs/cache-ownership-TbpWf7` and
`.dev/runs/cache-coverage-MHVzTs`. SDK archive/header version is 1.9.4; HBC
menu is 1.10.0. The survey binary SHA-256 is
`6881e9aebd4bae9c688682475d4c71cd4638827ba0a7877363887ba6936fb9b2`;
control is `8794044205fccdb3fd41266157a8cbb4c2038a2f16ea1754230dd4acb46d9ebb`.
The final guard refresh is a stopped-state diagnostic change after these
builds; it does not change the measured gameplay hooks. Its 1.6.16 native
verification at `.dev/runs/hardware-glN64_wii-20261004-221136-6FNJ` completed
300 SM64 VIs, reproduced 8 stale words with no fresh/guard errors, and returned
to HBC 1.10.0 without a crash record. Dolphin's matching 300-VI test passed.

Clean ordinary glN64 and Rice Wii builds pass with the pinned SDK. The glN64
release boots in muted Dolphin without new invalid-access/DSP/SD-sync
warnings. The final 1.6.16 guard-refresh build passes its Dolphin scene.
Host subsystem, ROM VM, extracted packing/lifetime, cache-overrun model,
report-validation and frozen-library-workflow tests pass. The library cache
switch also fails correctly when supplied a build without cache records.

Retained mechanisms: `b4e4411` adds the gated GX witness and framebuffer
counters; `a478fce` adds texture/mip counters, extracted tests and reporting.
Raw ROMs, captures, logs, frozen binaries and SDK source remain ignored and
are not included in these commits.

Next bounded experiment: record one in-game replay that actually invokes
mip-chain loading, then measure its staging cost before selecting a variant.

## Local model review

Muse Glimmer and Gemma4 completed two asynchronous reviews with 10,000-token
caps in about 55–65 seconds. Their unsupported claims included EFB/source
confusion, four 32-byte strides sharing a 32-byte line, and a fallback
use-after-free contradicted by the code. None is adopted. The extracted
lifetime tests give stronger evidence than agreement between models.

Improve these reviews with explicit source/destination/owner contracts,
SDK facts and executable counterexamples. Keep per-model attribution and
report truncation separately from an adequate answer.
