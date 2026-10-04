# Hardware validation: 2026-10-04

Started from clean `master` at `b2f16fe` (1.6.12). No engine change is made in
this validation pass. Use the existing queue, frozen artifacts and replays.
Completed jobs return to Homebrew Channel; none power off the Wii or write saves.

## Frozen environment

The survey and glN64 library use the 1.6.11 runtime from `bf2a6e8`, including
the written-byte invalidation fix. Later changes through `b2f16fe` affect the
SDK builder, version label and documentation, not this emulator code.
The agent is HBC-Reborn 1.9.4, source `0b214c7a`; HBC reports 1.9.3 on return.
The external SDK checkout remains `/private/tmp/wii64-hbc-sdk.0pDjdc`.
The shared source/client checkout was not upgraded during live tests.

Full DOL SHA-256: `7ae4244260123fce0764440a3689f85802c1ce0b5a5aa071ca89703f341e93bc`.
Control/library DOL SHA-256: `a90c504724afad65059b3871bc3f92dfc74a399c25e1013b4d47907bc8e5a47a`.
Matching ELFs, chain, flags, source and archive hashes are retained in
`.dev/runs/subsystem-survey-20261003-210947-L9Sp`.
The original Dolphin DOL/ELF and all 18 ROM hashes were verified before reuse.

## Current four-scene survey

Jobs `20261003-211109-a491b1`, `20261003-211109-6cf69e` and
`20261003-211109-dc0a5d` all pass: 12 requested VI targets, correct replays,
zero paging errors, zero audio overruns, zero dropped self samples, and HBC
return after each run. This is full/control/full **probe cost**, not an emulator
optimization comparison. Full-repeat CPU cycles differ by at most 0.65%.

| Scene | Added CPU cycles | Added non-sleep wall | Total wall change | Underruns, full/control/full |
|---|---:|---:|---:|---|
| SM64 file menu | +16.21% | +14.09% | -0.02% | 10/6/9 |
| Kart race start | +12.94% | +12.96% | +0.07% | 10/9/9 |
| Snap intro | +15.13% | +14.63% | +0.16% | 0/0/0 |
| DK64 intro | +15.76% | +14.17% | -0.02% | 3/3/4 |

Full-run mean below. Graphics and its hash child are observed inclusive CPU
wall spans, not additive partitions. Non-sleep wall excludes requested limiter
sleep but still includes waits and interrupts. Self values are sparse estimates.

| Scene | Graphics, wall/non-sleep % | Hash, wall % | Audio, wall % | Dispatch self, wall %, full1/full2 | Execute self, wall %, full1/full2 |
|---|---:|---:|---:|---:|---:|
| SM64 | 6.22/19.69 | 0.97 | 1.89 | 2.03/2.10 | 19.93/21.67 |
| Kart | 17.78/41.54 | 2.46 | 4.91 | 0.98/0.97 | 21.38/31.44 |
| Snap | 38.54/51.61 | 9.34 | 3.60 | 1.77/1.78 | 31.02/33.86 |
| DK64 | 2.86/7.86 | 0.45 | 2.79 | 1.55/1.58 | 31.74/29.08 |

Dispatch is no longer the old dominant C-path candidate. Graphics is a stable
observed workload in Snap and Kart. Execute and helper estimates remain
unstable despite stable cycle totals; do not choose a generated-code change
from their apparent ranking. Full probes cost 12.9–16.2% extra cycles in these
scenes, above the plan's CPU-focused <=3% target. Use the control for performance
comparisons, not a universal probe correction.

All 12 GP clock spans match the timebase within 0.00001%. Rasterizer busy is
about 1.7/2.6/6.0/1.3% in SM64/Kart/Snap/DK64. FIFO overflow counts are
621/645/627 in SM64 and 3/63/3 in Kart, versus zero in Snap and DK64.
`gp_start` resets the overflow count. Low raster utilization does not rule out
command/FIFO back-pressure. The stalls' durations are not measured; the control
also shows how full probes can hide bursts.

## Crash recovery

Job `20261004-013115-dcbbb7` passes the existing controlled DSI test with DK64's
VM active. It records exception 3, DAR `00000010`, and identifies
`devAgent_testCrash` in the matching ELF. HBC returns automatically in the
62-second job. LTO omits that function's source line; symbol identification and
caller lines remain available. Results: `.dev/runs/hardware-glN64_wii-20261004-013924-wY4f`.
This expected failure is not a game crash or a performance result. Separate
fatal-API, hang and retained-output checks remain outside this test.

## Library and Rice checks

The 18-ROM probe-off glN64 library uses the same binary and inputs as the
previous Dolphin sweep. Results: `.dev/runs/library-glN64_wii-20261004-013119-zLLB`.
All 18 entries complete their requested VI targets. Coverage has no missing,
duplicate or unexpected entries. All three batches return to HBC. Paging
errors and audio overruns are zero. The strict health gate fails: all speeds
are below 100%, 15 entries have underruns, and three have slow VI windows.
There are 109 underruns in total. This is a current baseline, not a measured
speedup or evidence that these warnings are new regressions.

| ROM entry | Speed % | Underruns | Slow VI windows |
|---|---:|---:|---:|
| The World Is Not Enough, USA | 99.3552 | 1 | 0 |
| Banjo-Kazooie | 99.4462 | 4 | 0 |
| Diddy Kong Racing, USA Rev 1 | 99.2981 | 0 | 0 |
| Donkey Kong 64, USA | 99.3032 | 3 | 0 |
| GoldenEye 007, USA | 99.4138 | 6 | 0 |
| Majora's Mask, Europe | 99.6084 | 7 | 1 |
| Mario Kart 64 | 99.6680 | 7 | 0 |
| Mario Party, USA | 99.3722 | 7 | 0 |
| Mario Party 2, Europe | 99.4351 | 4 | 0 |
| Mario Party 2, USA | 99.2122 | 8 | 0 |
| Mario Party 3, Europe | 99.4690 | 1 | 0 |
| Mario Party 3, USA | 99.3190 | 5 | 0 |
| Mario Party.v64 | 99.2953 | 7 | 0 |
| Pokemon Snap | 99.3931 | 0 | 0 |
| Super Mario 64 | 99.4478 | 10 | 0 |
| Super Smash Bros., USA | 99.3003 | 0 | 0 |
| Wave Race 64, USA Rev 1 | 99.1034 | 10 | 1 |
| Ocarina of Time Master Quest | 99.4792 | 29 | 2 |

All final captures were inspected. Kart shows a race, Wave Race shows its
demo, and the other entries show menus, intros or cutscenes. Master Quest
again shows Navi dialogue on grey: the unresolved visual reference check in
[the earlier library report](library-results-2026-10-01.md) still applies.
Nonflat captures are not proof of correct rendering throughout a game.
Physical Wii audio is not captured; activity and gap counters are not a
listening test. Missing subsystem counters are unknown, not zero.

Rice's current-master profiling build is clean-built and frozen with its ELF
in `.dev/runs/cpu-audit-20261004/`. Job `20261004-014445-6ef35a` passes the
existing two-ROM smoke in 61 seconds: SM64 and Kart each reach 900 VIs and
their inspected frames show rendered title screens. Each records six audio
underruns and zero overruns. HBC returns automatically. Results:
`.dev/runs/hardware-Rice_wii-20261004-022627-c7Yn`.
This is functional coverage, not a cross-renderer performance comparison;
the newly built SDK archive is not byte-identical to the old frozen survey
archive. The normal glN64 release build is restored afterward.
Its isolated, muted 25-second Dolphin boot check passes with no invalid-access,
DSP or SD-sync warnings. This boot check does not run a ROM; the earlier
18-ROM Dolphin sweep remains the matching library functional check.

The existing production invalidation-range tests pass under ASan/UBSan with
both fast-path settings. The offline hardware-queue regression also passes.
The library smoke does not replace a targeted guest-instruction test for
unaligned self-modifying writes; that test remains pending.

## Independent review

Muse Glimmer, Gemma4, Qwen3-Coder and Devstral reviewed the measurements in
two parallel pairs with 10,000-token caps and 300-second timeouts. Both pairs
completed without truncation or timeout, in 41 and 22 seconds. Their advice
was checked against source and arithmetic. Reject suggestions that confuse
engine optimizations with probe flags or infer additive costs from nested spans.

## Next experiment

Use the [WiiStation-informed measurement design](performance-methods-2026-10-04.md):
validate and calibrate a minimal opt-in 1 ms PC sampler before choosing another
CPU optimization. Retain the observed graphics ranking and FIFO counts; do not
replace a cache or remove GX waits on the strength of an aliased estimate.
