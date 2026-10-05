# Memory investigation results

This report follows the [memory plan](memory-optimization-plan-2026-10-04.md).
Capacity, sampled residence and elapsed time are different measurements.
No RAM-bank relocation or locked-cache path is enabled by this work.

## Implemented changes

| Commit | Type | Mechanism | Result |
|---|---|---|---|
| `bc68e2d` | CLEANUP | Guest exception causes and light paging records | Deterministic guest-work comparisons available |
| `57933e6` | GUARD | Boxart failure cleanup before use | Wii/GC allocation-failure host tests pass |
| `5c0f32d` | CLEANUP | Opt-in frozen library census | Corrected sweep covers 18/18 ROMs on both platforms |
| `1630c5a` | BUG | Zero-pad raw-SD capture filenames | Short-chain host test and library captures pass |
| `6bed00b` | BUG | Keep relocation inside the allocated address map | Red/green sentinel test; native/Dolphin accounting reconciles |
| `3239a84` | BUG | Pin queued defaults and nested crash-test config | Queue fixture and native fatal-DSI recovery pass |
| `27e7b84` | PERF | Reduce only the boxart reservation to 768 KiB | Confirmed 256 KiB capacity gain; no FPS claim |
| `1ed0be8` | CLEANUP | Count valid VM faults/zero fills in light probes | Host tests; matching full-library paging counts |
| `91d4556` | CLEANUP | Stopped HOME census and leased test wrapper | Native enter/active/closed, resume and HBC return pass |
| `d001941` | CLEANUP | Cover controller allocation/heap-init failures | Sanitizer-backed Wii/GC extracted-path tests pass |
| `26d83cc` | CLEANUP | Deterministic PC diagnostic chain | Guest parity and lower-rate sampler calibration pass |
| `fd6c629` | CLEANUP | Version 1.6.15 | Clean glN64 release and Rice census builds pass |

Documentation also links these results from `AGENTS.md` and closes the measured
pass in the plan. ROMs, captures, PCM, SDK checkouts and raw run logs are not
committed. Conditional experiments remain explicitly unimplemented.

## Ownership audit

The fixed MEM2 map remains in `gc_memory/MEM2.h`. Only the boxart reservation
changes: 1 MiB to the tested 768 KiB bound. TLB, block, invalidation, ROM, texture
and recompilation pools retain their sizes. The HBC record gap stays fixed.

| Owner | Bank / allocator | Size or constraint |
|---|---|---:|
| Guest RDRAM | MEM1 static | 8 MiB |
| Generated PPC code | MEM1 private heap | 9.5 MiB glN64; 9 MiB Rice |
| Cache nodes | MEM1 private heap | 384 KiB |
| Texture slots | MEM1 static | 409,552 bytes |
| Wii64 audio queue | MEM1 static | 73,728 bytes |
| Wii64 DSP staging | MEM1 static | 1,152 bytes |
| Wii64 output state | MEM1 static | 46,304 bytes |
| AESND stream buffer | MEM1 SDK static | 73,728 bytes |
| Bluetooth buffers | MEM1 SDK static | 65,552 + 205,440 bytes |
| Main stack | MEM1 static | 128 KiB |
| HBC agent / watchdog stacks | MEM1 malloc | 12 / 4 KiB |
| HBC overlay keeper / WAV / NTP buffers | MEM1 static | 4 / 16 / 16 KiB |
| HBC HOME SD-thread workspace | SDK LWP workspace | 16 KiB while active |
| IOS network heap | MEM2, Arena2Hi | 64 KiB |

Static sizes come from matching PPC ELF/archive symbols, not a malloc census.
The Bluetooth buffers are not the IOS LAN pool. libogc2
`43f26d22deaacae287c2f8d96eba00dba1ee0155`, `libogc/network_wii.c`, allocates
the network heap from Arena2Hi. The measured loaded ceiling is `0x933BB7C0`,
below the map's nominal ceiling `0x933E0000`. The observed 84,032-byte drop
from boot includes that 64 KiB heap; the remaining SDK workspace is not fully
attributed. Do not treat nominal unclaimed bytes as runtime malloc capacity.

The tested application archive is HBC agent SDK 1.9.4, matching source
`0b214c7a78d81d87f4a2b24e53feb3c967e26d5c`. Homebrew Channel reports 1.10.0.
The two version numbers describe different programs. HOME uses the other
existing XFB; it does not require two new full-screen allocations.

## Correctness finding

The first 18-ROM census exposed metadata damage in both Dolphin and Wii.
The 007 snapshot missed 176 bytes in physical-versus-ledger accounting.
A stopped diagnostic located a used block at `0x9305CE78`, charged 328 bytes;
the next footer at `0x9305CFC0` had been overwritten. The census correctly
rejected those results. Do not use the affected metadata peaks to size a pool.

The address-map relocation loop in `r4300/ppc/Recompile.c` used consumed
instructions (`src`) as its bound. Conversion can consume an end delay slot
beyond the allocated map. It now uses the exclusive map end (`src_last`).
Guest branch/delay-slot semantics are unchanged. The extracted-loop host test
fails before the change and passes afterward, preserving NULL entries and an
adjacent sentinel. The exact Dolphin 007 case now reconciles all five heaps
with zero errors and unchanged guest/render counters. The corrected full
library completed 18/18 owned ROMs on each platform. All five heaps reconcile
in every snapshot. All 15 native code/metadata teardown checks reached zero
live bytes; each six-ROM chain keeps its final game resident until HBC return.
The fix is commit `6bed00b`.

An external-SDK allocator experiment passed 100,000 seeded allocate/write/free
operations with physical and free-list checks. Its host pointer adaptation is
not native proof. No SDK allocator change was justified or made.

## Capacity experiment

The native allocator charges 737,920 bytes for all 16 browser textures.
A 768 KiB heap has 786,424 charged bytes, leaving 48,504 bytes after those
allocations. Failed controller initialization and each failed texture slot
have host tests; the browser returns safely without a NULL memset or a stale
button texture.

| glN64 + agent | Old 1 MiB | Trial 768 KiB | Difference |
|---|---:|---:|---:|
| Fixed MEM2 bytes | 53,956,608 | 53,694,464 | -262,144 |
| Nominal unclaimed bytes | 438,272 | 700,416 | +262,144 |
| Loaded unallocated Arena2 | 288,704 | 550,848 | +262,144 |

Both matched Wii old/new/old runs preserved guest/render counters and completed
browser/ROM transitions. An initial SM64 7/8/7 underrun count did not repeat:
the second trial recorded 8/8/8. Its other entries were 14/13/12, 5/5/5 and
9/9/9. There is no repeatable audio regression from the reservation change.
Rice completed its four-entry native check and two-entry Dolphin check.
The HOME census recorded enter/active/closed, resumed the game and returned
to HBC; malloc usage did not grow at entry/active (256 bytes after close),
and unallocated Arena2 remained 550,848 bytes throughout.
PAL ROM boot and PAL-sized XFB layout checks do not substitute for a physical
PAL output-mode or vWii test. GameCube sources compile here; linking still
requires the missing GameCube libfat archive. No FPS gain is claimed.

## Probe calibration and traffic

`randomize_interrupt=0` in diagnostic chains gives identical per-cause guest
exceptions and recompiles across the bounded SM64/Kart controls. Production
interrupt randomization is unchanged. Census off/on/off comparisons are still
provisional: two off/on/off trials passed cycle/wall thresholds but failed the
strict extra-audio-underrun gate. Their small cycle cost is not a speed gain.

A separate same-binary PC sampler trial used 3,645,000 eligible cycles per
sample (5 ms at 729 MHz), with the same 128 KiB histogram reservation in its
controls. Controls/sampler/controls passed the existing guest/work/audio and
initial non-sleep-wall gates. Final Kart frames show the same race-grid scene.
Sampler PMC totals are deliberately unavailable; do not claim CPU-cycle cost
from this experiment.

| Scene | Non-sleep wall change | Control drift | Samples |
|---|---:|---:|---:|
| SM64 file menu | +0.01% | 0.16% | 769 |
| Kart race grid | -0.02% | 0.06% | 4,498 |

| Kart sampled residence | Share |
|---|---:|
| Generated guest PPC code | 30.75% |
| YUYV to RGBA5551 conversion | 23.19% |
| Texture CRC | 6.22% |
| RSP NEAD envelope mixing | 5.14% |
| memcpy | 4.42% |
| Function invalidation | 3.96% |
| RSP resampling | 3.45% |

These are emulation-thread PC samples, not total-wall or busy-time shares.
64-byte buckets have boundary ambiguity. The replay has not established
active racing: the final frame is the grid. Long-gameplay working-set and
cache-miss measurements remain prerequisites for placement and LC trials.

The conversion already computes columns once and uses a clamp table. History
`8dba042102148f657ecafa99965774025f54f017` records its measured improvement
and the rejected dcbz/dcbt variant. The depth-transition copy skip is already
present. No further duplicate-copy removal is proved. CPU-readable XFBs
already use the cached fixed-map pointers; converting them to K1 is not a
performance fix. GX producer completion and cached-reader lifetime need a
separate coherency experiment before altering that path.

## Valid full-library capacity census

These are maximum charged peaks across three native six-ROM chains, not
worst-case bounds for a whole game. Dolphin produced the same peaks and
the same paging totals: 5,083 faults, 1,127 reads and 3,956 read-ahead hits.
There were no allocation failures, JIT capacity evictions or accounting
errors. No retired texture overlap was exercised; zero is not a lifetime proof.

| Pool | Charged capacity | Peak | Largest request | Maximum free-minus-largest block |
|---|---:|---:|---:|---:|
| Generated code | 9,961,464 | 7,139,176 | 77,188 | 198,264 |
| Metadata | 4,194,296 | 1,329,744 | 4,096 | 18,360 |
| Nodes | 393,208 | 226,352 | 65,536 | 648 |
| glN64 texture | 15,728,632 | 8,170,808 | 131,072 | 2,088 |
| Boxart | 786,424 | 737,920 | 46,080 | 0 |

The last column is observed fragmentation at stopped snapshots. It is not
newlib fragmentation or a maximum guaranteed payload. The ordinary glN64
release clean-build passed: text/data/BSS are 1,644,108 / 556,588 / 10,492,288
bytes, with no census, HOME-memory callback or page-fault-counting symbols.

The intentional fatal-DSI test also passed with DK64 ROM paging active. The
agent captured exception 3 at DAR `0x10`; the matching ELF identifies
`devAgent_testCrash`. Recovery returned to HBC 1.10.0 without powering off.
This injected fault is a recovery test, not a game failure.

## Candidate decisions

| Rank | Candidate | Type | Evidence / decision | Risk / test |
|---|---|---|---|---|
| P0 | Bound address-map relocation | BUG | Reproduced footer overwrite; narrow fix | Host sentinel, exact Dolphin, native library |
| P0 | Boxart allocation fallback | GUARD | NULL was followed by memset | Every failing slot and controller init |
| P1 | 768 KiB boxart reservation | PERF | Native charged bound; 256 KiB recovered | Layout, browser, HOME, old/new/old, Rice |
| P1 | Cold HOME and VM counters | CLEANUP | Missing lifecycle/fault measurements | Stopped callbacks; release compiles out |
| P2 | XFB reader coherency | BUG | Cached source plus GX producer; stale-read risk not yet reproduced | Distinct cached/device patterns and producer-complete check |
| P2 | Hot data split or pool tuning | PERF | Current short scenes cannot bound long-game peaks/misses | More validated gameplay, largest requests and evictions |
| REJECT | Compact global tables now | PERF | No proved capacity limit; aliases and coverage must remain | Extra lookup/semantic risk unjustified |
| REJECT | Enable LC now | PERF | PC residence is not a data-cache-stall measurement | Smaller normal L1 and setup/coherency cost unmeasured |

Raw ROMs, logs, PCM and captures remain in ignored `.dev/` runs. The first
library covered 18/18 owned files on each platform, with nonflat final frames
and nonzero Dolphin DSP PCM. Several entries have slow windows or underruns;
completion is not proof of correct rendering, audible quality or 100% speed.
The valid rerun supersedes the first rejected census. All launcher and VI
coverage checks passed. Audio counts varied both ways between the two native
sweeps (for example MM 25 to 29, Master Quest 43 to 40); these are not matched
A/B/A performance trials. Timing claims remain provisional, not a promise of
100% speed or audible quality.

## Evidence retained locally

All paths below are ignored directories under `.dev/runs/`:

- Corrected library: `library-glN64_wii-20261004-171853-yyAJ`.
- Rejected pre-fix library: `library-glN64_wii-20261004-163450-7Sgx`.
- Capacity trials: `subsystem-survey-20261004-163207-HVzc` and
  `subsystem-survey-20261004-165426-Jshq` (light census, not subsystem timers).
- PC calibration: `pc-survey-20261004-164513-hETx`.
- HOME: `memory-home-nAhI`, native result `hardware-glN64_wii-20261004-175751-gv59`.
- Rice: `hardware-Rice_wii-20261004-175901-jbfz` and
  `dolphin-chain-20261004-172646-TxXR`.
- Fatal-DSI recovery: `hardware-glN64_wii-20261004-202517-mGCp`.

The subsystem-time comparator correctly rejects light-census capacity trials.
Use `memory_report.py` and guest counters for them; leave its timing gate intact.
The first HOME test inherited the worker's older chain with `memory=0` and
failed its snapshot check. The wrapper now freezes its own chain/launcher.
The general queued launcher also pins empty/default overrides so the persistent
worker cannot silently supply another survey's DOL, ROM source or config.

## Local-LLM use

Muse Glimmer, Qwen3-Coder and Gemma4 completed independent Ollama reviews with
10,000-token caps and 300-second timeouts. vLLM was offline. Those budgets were
reliable for completion, not factual accuracy. Source/tests rejected invented
allocator constants, an irrelevant 64-bit pointer diagnosis and a fabricated
hole-list use-after-free. Improve the tools with explicit target ABI, exact
model-ID validation, separate facts/hypotheses, and compact deduplicated results.
The final two-model diff review also misread removed versus added includes,
ignored the preceding VM range guard, and treated a C static as uninitialized.
Those claims were rejected against source and the release/crash tests. Supply
the current function with its guards, not only a patch, for control-flow review.
