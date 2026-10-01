# ROM library sweep, 2026-10-01

All 18 owned ROM files reached their scripted VI targets on glN64 in Dolphin
and on the Wii. There were no missing entries, boot stalls, reported crashes or
paging errors. All Dolphin DSP captures contained nonzero samples. The Wii
returned to HBC after each batch; it was not powered off.

This does **not** meet the strict 100% speed requirement. Every instrumented
run measured below 100%, including some very small shortfalls. Audio underruns
occurred in 16 Dolphin entries and 15 Wii entries. Slow VI windows occurred in
8 Dolphin entries and 9 Wii entries. These are review candidates, not audible
quality or per-frame stutter measurements.

Most tests cover intros or menus. Mario Kart, Super Mario 64 and Zelda have
longer replays. The Mario Party scripts press A periodically. Regional and
byte-order copies are separate entries, not additional game titles.

## Results

Speed is completed VIs divided by gameplay wall time and the nominal VI rate.
The gap columns count audio underruns, not seconds of silence.

| ROM file | Dolphin speed % | Wii speed % | Dolphin/Wii gaps |
|---|---:|---:|---:|
| 007 - The World Is Not Enough (USA).z64 | 99.3116 | 99.2958 | 6/1 |
| Banjo-Kazooie.V64 | 99.6877 | 99.3847 | 2/3 |
| Diddy Kong Racing (USA) (En,Fr) (Rev 1).z64 | 99.9980 | 98.8776 | 0/0 |
| Donkey Kong 64 (USA).z64 | 99.0491 | 98.8539 | 5/4 |
| GoldenEye 007 (USA).z64 | 99.6387 | 98.8585 | 7/10 |
| Legend of Zelda, The - Majora's Mask (E) (M4) [!].z64 | 99.6479 | 99.1857 | 3/38 |
| Mario Kart 64.v64 | 98.3875 | 98.5400 | 131/102 |
| Mario Party (USA).z64 | 99.5157 | 98.7216 | 14/8 |
| Mario Party 2 (E) (M5) [!].z64 | 99.9223 | 99.0401 | 5/11 |
| Mario Party 2 (USA).z64 | 99.9200 | 98.6216 | 4/10 |
| Mario Party 3 (E) (M4) [!].z64 | 99.9723 | 98.9341 | 1/6 |
| Mario Party 3 (USA).z64 | 99.9999 | 98.9420 | 4/9 |
| Mario Party.v64 | 99.5179 | 99.1206 | 14/11 |
| Pokemon Snap.rom | 96.6248 | 99.1834 | 100/0 |
| Super Mario 64.v64 | 99.9701 | 99.5092 | 11/10 |
| Super Smash Bros. (U) [!].z64 | 99.8138 | 99.3534 | 0/0 |
| Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).z64 | 99.6141 | 99.1735 | 10/8 |
| Zelda Ocarina of Time Master Quest.v64 | 97.4270 | 98.6510 | 304/133 |

Wii captures show game scenes in all entries. Master Quest's final frame shows
Navi dialogue on a grey background; this still needs a visual reference check.
A separate 9,000-VI Rice/Dolphin run reached the same dialogue and grey frame.
That comparison does not establish whether the background is correct.
The old Zelda boot hang did not recur; do not mark all Zelda rendering correct
from task activity alone.

The initial Dolphin RAM captures are not reliable visual checks because its
XFB copies were texture-only. Separate RAM-copy checks show visible Pokémon
Snap and Mario Kart scenes. No physical Wii sound recording was made, and
nonzero Dolphin PCM does not prove correct music or absence of clicks.

## Probe controls

The following runs use the same audio modes. The Dolphin pair has identical
ROM order and XFB RAM-copy settings, with no concurrent compiler. The Wii
Mario Kart entry is first in both batches. These are single-run comparisons,
not repeated medians or proof that instrumentation explains every gap.

| Platform / scene | Full-probe speed % | Control speed % | Full/control gaps |
|---|---:|---:|---:|
| Dolphin / Pokémon Snap, 1,800 VIs | 96.6055 | 99.1541 | 100/14 |
| Dolphin / Mario Kart, 5,400 VIs | 98.2996 | 98.8276 | 139/95 |
| Wii / Mario Kart, 5,400 VIs | 98.5400 | 98.8089 | 102/79 |

Removing subsystem probes reduces these warnings, but the control results
still fall short of 100%. Prioritize Mario Kart's graphics bursts and audio
gaps, then Master Quest's gaps and visual check. Pokémon Snap's Dolphin result
also needs longer control runs; its Wii sweep had no audio underruns.
In the full Wii Kart run, inclusive graphics work occupied 44.1% of wall time,
RSP audio 5.3% and requested limiter sleep 23.0%. These timers overlap; do not
sum them or treat requested sleep as proof of smooth frame delivery.

## Provenance and repeat

The sweep uses frozen 1.6.7 diagnostic builds made from `192be2b` with only the
version changed in engine sources, before the new loading UI. Full runs use
`PERF_PROF`, `PERF_SUBSYSTEM_PROBES` and `HBC_AGENT=1`; controls use `PERF_PROF`
and `HBC_AGENT=1`. Later loading-UI tests are separate baselines.

Dolphin ran on the Intel Mac with MMU, DSP LLE, dynarec and muted host playback.
Guest audio stayed enabled: accurate synthesis, DSP output, accurate mixing,
stable latency and native sync. The Wii logged the same audio modes, HBC 1.8.9,
agent SDK 1.8.6 and IOS 58 revision 6175. Some initial Dolphin entries overlapped
host compilation; do not use them as clean timing controls.

Filed raw logs, replays and artifact hashes:

- `baselines/library-20261001-dolphin-01` through `-18`;
- `baselines/library-20261001-hardware-01` through `-03` (six entries each);
- `baselines/library-20261001-dolphin-probes` and `-control`;
- `baselines/library-20261001-hardware-control`.

The ignored session `.dev/runs/library-glN64_wii-20261001-100011-zEPc` retains
the ROM/build SHA-256 manifest, coverage report, WAV statistics and captures.
ROMs, WAVs and images are not committed. Use [library testing](library-testing.md)
to repeat the sweep, then compare filed runs with `scripts/chain_compare.py`.

## Loading progress change

Version 1.6.7 adds percentages to loading bars. Pagefile preparation reports
completed scan/write progress at coarse intervals and never draws under the VM
mutex. Failed writes do not display 100%. The old `Loading ...` message now
shows the existing three-second controller wait.

ASan/UBSan loader, flush and pagefile tests pass. The new build completed a
large-ROM then cached-ROM check in Dolphin and both Zelda boot checks on the
Wii, without reported faults or paging errors. The Wii returned to HBC 1.8.9.
These checks are filed separately as `loading-progress-20261001-dolphin` and
`loading-progress-20261001-hardware`; they are not gameplay speed improvements.
