# Current-head hot paths and YUYV conversion

Starting point: master `b1695664cde8ac37fcfa9d6d58a3b9834b3aa7b2`,
Wii64 1.6.16. Retained changes are version 1.6.17. Builds used devkitPPC
GCC 16.1.0, installed libogc2/libfat, and the HBC agent 1.9.4 SDK at
`0b214c7a78d81d87f4a2b24e53feb3c967e26d5c`. The Wii returned to HBC 1.10.0
after all six survey runs and the two-ROM staging smoke run. No job bypassed
the central lease.

## Fresh PC survey

`scripts/chains/next_hotpaths.txt` fixes settings and replay inputs for
SM64 / Mario Kart / Pokemon Snap at 900 / 3600 / 900 VIs. These are bounded
scenes: SM64 file selection, Kart's race grid, and Snap's intro. They do not
establish active-race or whole-game performance.

The same frozen DOL/ELF ran control / 5 ms PC sampler / control. Sampler
builds do not provide valid aggregate PMC totals. The following gate uses
wall time minus requested limiter sleep, not measured busy CPU.

| Scene | Control non-sleep ms | Sampler ms | Added | Control drift | Gate |
|---|---:|---:|---:|---:|---|
| SM64 | 4210.092 | 4205.699 | -0.10% | 0.06% | Failed: one extra audio underrun |
| Mario Kart | 23032.656 | 23039.586 | +0.03% | 0.02% | Initial wall-cost gate passed |
| Pokemon Snap | 10003.004 | 10009.220 | +0.06% | 0.04% | Initial wall-cost gate passed |

Kart and Snap frames were inspected. PC shares are samples in 64-byte
buckets, not a wall-time partition or exclusive subsystem times. Boundary
buckets remain ambiguous. The SM64 ranking is not accepted because its
audio gate failed.

| PC residence | Mario Kart, 4,494 samples | Pokemon Snap, 1,924 samples |
|---|---:|---:|
| Generated JIT code | 32.04% | 30.98% |
| YUYV conversion | 23.39% | Not a leading entry |
| Texture CRC | 6.50% | 15.12% |
| AddTriangle | Not a leading entry | 7.95% |
| Boundary-ambiguous samples | 158 | 103 |

No current evidence supports revisiting inactive mip-chain or CPU-reused
framebuffer-copy paths. See [the previous cache census](cache-results-2026-10-04.md).

## Retained optimization: exact YUYV products

Observation: Kart spends substantial sampled CPU residence converting its
XFB for the billboard. Mechanism: five 256-entry signed coefficient tables
replace five per-pixel integer products. They occupy 5,120 additional MEM1
BSS bytes and initialize with the existing clamp table. Integer rounding,
clamping, word swapping, float row/column scaling, XFB selection and cache
ownership are unchanged. The multiplication path remains available with
`GLN64_YUYV_PRODUCTS=0` for matched measurements.

Risk: lookup loads can lose to multiplication through cache pressure. The
host test exhausts every Y/U/V triple through both Y0 and Y1, then checks
scaled, PAL, odd-width, zero-width and guarded frames under ASan/UBSan for
both implementations. Both pass exactly.

Native experiment: candidate / reference / same candidate, with identical
subsystem probes and inputs. Lower is better for the cost columns.

| Scene | CPU cycles | Non-sleep wall | Total wall | Audio underruns A/B/A |
|---|---:|---:|---:|---|
| SM64 | -0.35% | -0.50% | -0.07% | 9 / 9 / 8 |
| Mario Kart | **-2.75%** | **-2.77%** | -0.01% | 9 / 9 / 9 |
| Pokemon Snap | -0.66% | -0.58% | +0.03% | 0 / 0 / 0 |

Kart's sampled SETCIMG time fell from 44,064 us to a candidate mean of
38,973.5 us: **11.55% lower** across the same 36 sampled calls out of
4,281 commands. This command timer includes more than conversion alone.
The candidate cycle drift was 0.007% in Kart. Guest exceptions, recompiles,
triangle batches and vertices matched across the three runs. No overruns
or new crashes occurred. Inspected Kart captures show the same race grid;
SM64 and Snap also retain their reference scene content.

Result: **confirmed improvement in this instrumented native Kart scene**.
Limiter pacing explains the unchanged wall-speed result. The small gains
in scenes without conversion are not attributed to the tables. No claim
is made for ordinary-release FPS, long gameplay, all ROMs or audio quality.

Reproduce with:

```bash
bash .dev/profile_yuyv_products.sh next_hotpaths
# For an already queued survey:
bash .dev/profile_subsystems.sh --collect /absolute/path/to/survey
```

The wrapper reuses clean builds, frozen inputs and DOL/ELF hashes, queue
fairness, file collection and the existing A/B/A comparator. Runtime result
tags are excluded from settings comparison; changed guest settings still fail.

## Tooling and limits

macOS hardware runs now pull CRC-checked results through HBC-Reborn after
the app returns. There is no inbound workstation listener. A guest-recorded
unique tag rejects stale SD logs; unsupported frozen builds fail before upload.
Chain-named missing ROMs can also be staged through HBC without a server.
Matching files are skipped; different existing ROMs are preserved and cause
failure. Explicit push mode retains legacy HBC and the older ROM server.
Windows/Linux defaults are unchanged. See [hardware sessions](hardware-session.md).

The native HDMI helper finds UGREEN 15389 and passes six offline CLI tests.
Camera permission is not yet granted in the agent's host context, so actual
HDMI capture remains unverified. Routine capture does not request permission,
overwrite existing files or fall back to the FaceTime camera. It starts no server.

The initial candidate Dolphin chain exceeded its host budget: SM64 completed,
Kart reached 2,434/3,600 VIs, and Snap did not start. The runner retained the
failure and closed only its test-profile process. The current boot had no
invalid-access, DSP or SD-sync warnings. This is not a three-game pass.

The longer retry completed all three scene targets with native-matching
guest exception and recompile counts. Rendering work counts differed slightly
between platforms; they are not claimed as exact parity. Inspected Kart and
Snap captures retain the native scene content. The current boot had no
invalid-access, DSP or SD-sync warnings. Dolphin needed the profile-scoped
forced-close fallback at shutdown on both attempts; successful chain completion
does not make that shutdown clean. Snap had 10 Dolphin audio underruns versus
zero in the native trial. This is not an audio-quality pass or a physical timing
regression attributed to the patch.

The full host subsystem suite and ROM VM suite passed. Ordinary glN64 and Rice
Wii builds clean-linked as 1.6.17; neither release ELF contains the optional
subsystem, PC-sampler or memory-census entry points. The queued native smoke
with a supplied ROM folder completed both 900-VI targets, passed rendering
checks and returned to HBC through the listener-free result path.

Local evidence (ignored, never committed):

- PC survey: `.dev/runs/pc-survey-20261006-212509-v9mc`.
- YUYV A/B/A: `.dev/runs/subsystem-survey-20261006-213141-IeRh`;
  jobs `20261006-213437-2d517d`, `20261006-213437-8b0ef7`,
  `20261006-213437-a858d1`. Matching hashes and flags are in `artifacts.json`
  and `full.flags` / `control.flags` there.
- Candidate Dolphin: initial `dolphin-chain-20261006-214312-LNtQ`, retry
  `dolphin-chain-20261006-215135-iqYj`, both under `.dev/runs/`.
- Missing-only staging smoke: job `20261006-215004-ec1b33`, run
  `.dev/runs/hardware-glN64_wii-20261006-215025-zKKk`. Both ROMs matched
  existing SD CRCs and were skipped; the real missing-file upload remains
  covered by the shared HBC API and the fixture, not claimed as a new live upload.

Local models independently reviewed the collector, CRC and coefficient trial.
Muse Glimmer and Gemma4 completed with 10,000-token caps and 300 s limits
(39–53 s per panel). Claims were checked against source, PPC assembly and tests.
Several answers misread exact line membership, the ready/sent handshake,
CRC completion and seeded hash semantics; those suggestions were rejected.
The shared GPU benchmark was left running.

## Candidate ledger

| Rank | Candidate | Type | Evidence / expected gain | Risk / scope | Test |
|---|---|---|---|---|---|
| P0, retained | YUYV coefficient lookup | PERF | Native Kart cycle and command-time reduction | 5 KiB cache pressure; one converter | Exhaustive host test, native A/B/A, Dolphin |
| P1 | Snap triangle submission | PERF | AddTriangle is 7.95% of qualified PC samples; gain not yet known | Clipping and renderer-state semantics | Bound one submission path; compare generated PPC and guest counts |
| P2 | Snap texture CRC | PERF | 15.12% sampled residence; gain not yet known | TMEM wrap and seeded row/palette hashes | Identify active row lengths before proposing a mechanism |
| P2 | Generated JIT code | PERF | About 31–32% residence, without live block attribution | High semantic risk; no production change | Attribute a bounded scene to generated blocks first |

Notable rejects: removing the CRC row multiply (already strength-reduced by
the compiler); zero-row/independent-palette hash shortcuts (not seed-equivalent);
more TMEM caching (prior native trials lost); blanket cache-alias changes
(no demonstrated current ownership defect).

Next experiment: trace Snap's AddTriangle clipping/submission path, inspect
the generated PPC, and test one redundant-work removal against the same
native scene. Preserve output and guest counters; reject noise or regressions.
