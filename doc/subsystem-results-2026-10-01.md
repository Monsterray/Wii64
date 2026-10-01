# Whole-system survey: 2026-10-01

Wii64 1.6.5 adds 17 timer stages to the existing subsystem probes. They cover
display-list parsing, commands, vertices, graphics state, existing GX waits,
DMA, TLB translation, slow memory handlers, invalidation, input, interrupts,
interpreter execution, and audio submission/callbacks. Release builds compile
these timers out. See [probe boundaries and commands](subsystem-profile-2026-09-30.md).

## Tests and environment

- Clean glN64 and Rice Wii release builds passed with this Intel Mac's
  devkitPPC GCC 16.1.0 and installed external libogc2/libfat.
- Both release DOLs passed separate muted Dolphin MMU/LLE menu boots. These
  boots test startup, not ROM gameplay.
- Both renderers completed eight-ROM Wii full/control/full surveys: 48 entries.
  Loads, replay records, core/audio settings, VI targets and HBC returns passed.
- The glN64 audio-system chain added five ROMs and repeated Mario Party 3 PAL.
  Its control and second full run passed: 12 entries. The first full run
  delivered six results but hit a host Bash error before its HBC validation.
  Those results are retained locally, not filed as a validated baseline triple.
- The invalidation A/B/A and a separate frozen-launcher check passed: 12 entries.
- Validated Dolphin chains cover eight glN64 entries, eight Rice entries, and
  six audio-system entries. A short pure-interpreter check also exercised its
  timer. These are intro/menu/replay checks, not full-game compatibility tests.
- Host subsystem, texture, audio and ROM/VM suites passed. The invalidation
  walker passed 92,010 legacy comparisons in each mode with ASan/UBSan.

All Wii jobs used the central queue/lease server. The measured Wii environment
was HBC/agent 1.8.6, IOS 58 revision 6175, with AHBPROT. Keep that environment
fixed when comparing these captures. No HBC installation change was made.
The final verification returned to Homebrew Channel; no shutdown was requested.
Host playback was muted in Dolphin; guest audio stayed enabled.

The two chains cover 13 distinct ROMs, with no Zelda entries. The initial
glN64 Dolphin chain used USA Mario Party 2/3; hardware and corrected Rice
Dolphin chains used PAL files. Do not compare those region variants as an A/B.

## Probe overhead

The table compares the mean of two full runs with their disabled-probe control.
Both retain the existing performance/audio counters and the agent. These
original binaries timed invalidation calls before the already-invalid-page
guard. New binaries time only eligible-page work; that reduced boundary needs
its own overhead calibration.

| ROM | glN64 added CPU cycles | Rice added CPU cycles |
|---|---:|---:|
| Super Mario 64 | 9.69% | 9.68% |
| Mario Kart 64 | 14.98%* | 8.75% |
| Banjo-Kazooie | 10.97% | 11.40% |
| Mario Party | 14.21% | 14.96% |
| Mario Party 2 PAL | 10.70% | 10.92% |
| Mario Party 3 PAL | 8.96% | 8.98% |
| Pokémon Snap | 8.77% | 9.95% |
| Super Smash Bros. | 9.06% | 9.97% |

*The glN64 Mario Kart capture is black apart from its overlay and has little
graphics/audio activity. It is a correctness failure, not a healthy benchmark.

Wall time changed by less than 0.7% across these limiter-bound scenes. That
does not make probes free: non-sleep wall time rose about 8–13%. One triple per
renderer does not establish a general calibration. Keep probes opt-in, compare
CPU counters, and repeat with another sampling interval before a close decision.

## Workload ranking

These are fully timed inclusive spans from the first glN64 system run. Durations
differ by VI target and region; compare layers within a row, not raw times
between ROMs. Children overlap their parents and must not be summed.

| Scene | Graphics | RSP audio | Texture hashing, inside graphics | ROM copying |
|---|---:|---:|---:|---:|
| Super Mario 64, 2400 VIs | 3918 ms | 1168 ms | 614 ms | 42 ms |
| Banjo, 900 VIs | 1380 ms | 306 ms | 384 ms | 14 ms |
| Mario Party, 900 VIs | 631 ms | 160 ms | 220 ms | 449 ms |
| Mario Party 2 PAL, 900 VIs | 685 ms | 181 ms | 319 ms | 184 ms |
| Mario Party 3 PAL, 900 VIs | 697 ms | 820 ms | 321 ms | 194 ms |
| Pokémon Snap, 900 VIs | 5981 ms | 591 ms | 1591 ms | 15 ms |
| Smash, 900 VIs | 2689 ms | 974 ms | 299 ms | 29 ms |

Snap graphics used about 39% of wall time. Hashing used about 27% of that
graphics span; triangle submission added an overlapping 619 ms span.
Its sampled slow-memory estimate includes mapped-device handlers and RSP work,
not memory access self time. Likewise, interrupt time includes the limiter.
GPU execution, inline guest memory accesses, and host scheduling remain outside
direct coverage. Rice lacks glN64's detailed texture/state timers.

Graphics is the clearest next optimization target in Snap and several other
scenes. Audio remains significant in Mario Party 3; this is not a claim that
one layer dominates every game.

## Invalidation experiment

`DYNAREC_INVALIDATE_PAGE_SKIP=1` skips four-byte invalidation checks on pages
already marked invalid. Eligible addresses still use the original stride;
cached and uncached aliases retain separate checks. It defaults to 0.
Both candidate and reference used the same reduced probes and neutral 900-VI
graphics chain. All nine entries loaded and returned to HBC, with no NAND I/O
errors or audio overruns.

| ROM | PI DMA time change | Total CPU cycles | Non-sleep wall time |
|---|---:|---:|---:|
| Super Mario 64 | −42.8% | −0.88% | −0.95% |
| Banjo-Kazooie | −41.6% | −0.84% | −0.80% |
| Pokémon Snap | −39.6% | +0.08% | +0.15% |

This improves a small path, not the dominant graphics layer. Short-run FPS
varies, and one A/B/A is insufficient to establish a general win. The experiment
remains opt-in pending longer replay/visual checks and repeated control runs.
The texture-hash memo experiment also remains off; earlier mixed results do
not justify a global enable.

## Runner fixes

The queue uses session-specific agent IDs and explicit chain-derived receiver
and job deadlines. Surveys freeze the selected config with the DOL/ELF pair;
reuse verifies hashes, and collection can resume without a rebuild. The hardware
entry point now snapshots its Bash source in the queued command and records
DOL/ELF/config hashes. Keep its sourced helpers unchanged while jobs run.
The new launcher passed a three-ROM real-Wii verification after host tests.

Dolphin now checks staged ROMs before boot, retains each completed chain, and
scans the whole latest boot for invalid accesses, unknown DSP ucode, and SD-sync
failures. Baseline filing retains small artifact and validation records.
Failed ROM loads are excluded from timer rankings because they can retain old
counters. Low graphics activity raises a warning instead of a speed claim.

A leftover `WiiSDSync.xxx` caused Dolphin to fail its backup rename and partially
replace the sync folder. With Dolphin stopped, the backup was moved to
`.dev/runs/dolphin-sd-recovery-5c64/original-sync`; missing original files were
recovered with the staging helper. Nothing in that backup was deleted. Subsequent
chains and release boots completed SD sync without warnings. This matches
[Dolphin's backup/rename sync implementation](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/Common/FatFsUtil.cpp).

## Retained evidence and next work

Tracked logs, configs, traces, validation records and index rows are under
`baselines/2026-10-01_*`. The glN64, Rice and invalidation `full1/survey/` folders
retain original compiler, flags, source/artifact hashes, queue IDs, result paths,
and comparisons. Frozen binaries and screenshots remain ignored locally.
Those original surveys predate tracked-diff capture; retain their frozen DOL/ELF
pairs. New survey builds also record the tracked working-tree diff. Hashes alone
cannot reconstruct an uncommitted source snapshot.

1. Isolate glN64 Mario Kart's stalled render/audio path before using it for speed
   comparisons. Rice reaches the Mario GP menu. A glN64 Dolphin no-jitter check
   restored activity, but this does not prove a general interrupt-randomization
   fix. Its SD sync failed, so it remains diagnostic evidence only.
2. Split graphics command/state work further only where the existing probes
   cannot explain cost. Confirm sparse estimates with another interval; avoid
   per-vertex tracing or frame dumps as defaults.
3. Test texture/state reuse or batching in Snap with exact legacy hash/output
   checks. Repeat candidate/reference/candidate on Wii, then extend to rendered
   gameplay and other ROMs before enabling a default.
4. Recalibrate the reduced probes, and repeat the page-skip experiment with longer
   fixed replays. Do not subtract a universal probe percentage from timings.
5. Keep MEM2 unchanged for now. The agent survey reported roughly 577 KB free;
   the layout test leaves only about 0.69 MiB unclaimed. Additional cache buffers
   require a separate layout/peak-usage audit, not an assumption of spare space.
