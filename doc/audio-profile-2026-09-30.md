# Wii audio measurements, 2026-09-30

Build: glN64 Wii, `PERF_PROF`, devkitPPC r50-1, synthesis code at `5065429`.
The five-game coverage chain
and eight-game survey each completed 900 guest VIs per entry. All entries
had zero queue overruns. Each run returned to HBC-Reborn 1.8.1. The earlier
upload failures did not recur; their cause remains unresolved.

Use `python3 scripts/audio_report.py RUN_DIR` to read the saved counters.
Stage times below are estimates from one timed call in 127, including clock
reads and interruptions. They rank work within the tested scene. They are
not exact CPU totals or evidence of sound quality. Producer queue peaks
include loading and use the last AI input rate to convert bytes to milliseconds.
They exclude DSP and speaker delay.

## Short scene survey

All rows use Accurate mode. Most scenes are intros or title screens. Speed
is guest VI time divided by wall time, using each ROM's 50 or 60 Hz rate.
Reaching the VI target does not prove gameplay or audio coverage.

| Game | Active envelope/path | Largest estimated stage | Speed | Underruns | Producer peak |
| --- | --- | ---: | ---: | ---: | ---: |
| Super Mario 64 | exp | ADPCM 118.7 ms | 0.99x | 7 | 114 ms |
| Mario Kart 64 | No audio calls or fed buffers | No usable audio data | 0.99x | 0 | 0 ms |
| Banjo-Kazooie | lin | mix 95.4 ms | 0.99x | 3 | 81 ms |
| Mario Party | exp | mix 66.7 ms | 0.84x | 14 | 115 ms |
| Mario Party 2 (Europe) | mix only | mix 80.8 ms | 0.88x | 2 | 110 ms |
| Mario Party 3 (Europe) | lin | envelope 377.8 ms | 0.87x | 1 | 206 ms |
| Pokémon Snap | exp | mix 168.2 ms | 0.99x | 0 | 170 ms |
| Super Smash Bros. | lin | mix 311.7 ms | 0.99x | 0 | 161 ms |
| GoldenEye 007 | GE | envelope 138.3 ms | 0.99x | 2 | 71 ms |
| Diddy Kong Racing | GE | mix 156.2 ms | 0.99x | 0 | 150 ms |
| Wave Race 64 (USA, Rev 1) | exp | envelope 210.9 ms | 0.98x | 9 | 66 ms |
| The World Is Not Enough | MusyX effects only | effects 32.3 ms | 0.83x | 15 | 204 ms |
| Donkey Kong 64 | lin | mix 159.2 ms | 0.85x | 10 | 159 ms |

Mario Kart rendered about 1.1 fps and supplied zero AESND buffers. Exclude
this blocked scene from sound comparisons. Mario Party 2 measures final mixing
only. Wave Race USA does not test NEAD. All four `audio_gaps` counters stayed
zero; these scenes do not validate unsupported microcodes or branches.

Local results under the ignored `.dev/runs/` directory:

- Coverage: `hardware-glN64_wii-20260930-111603-stTs`
- Survey: `hardware-glN64_wii-20260930-112046-9OCQ`

## MusyX voice coverage

`scripts/chains/audio_musyx_long.txt` runs TWINE for 3,600 VIs in Accurate
mode. Its natural attract sequence reaches a first-person demo without an
input replay. The Wii capture showed a gun and hallway with “Press Start”.
The run recorded 6,842 subframes, 13,131 voice operations, and a peak of 11
voices per subframe. These counts do not establish persistent channel IDs.

Voice synthesis estimated 253.8 ms from 103 timed operations; effects estimated
134.2 ms from 53. Guest speed was 0.94x, with 27 underruns, zero overruns,
and a 283 ms producer queue peak at 22,047 Hz. A prior 3,600-VI attempt
recorded the same voice count and estimated 255.5 ms, but failed its requested
replay check because the replay file was absent. The clean chain requires no
replay and passed the result checker.

- Clean long run: `hardware-glN64_wii-20260930-112947-c9pF`
- Failed replay check: `hardware-glN64_wii-20260930-112705-JrEL`

The restored full-probe DOL also passed this 3,600-VI chain in Dolphin DSP LLE,
with Mac playback muted and the game sound engine active. It recorded the same
6,842 subframes, 13,131 voice operations, and peak of 11. Guest speed was 0.93x,
with 36 underruns, zero overruns, and a 121 ms producer queue peak. Its voice
and effect estimates were 413.9 and 171.0 ms. Dolphin timings and emulated
cycle counts do not measure Wii CPU cost. Results are saved in
`dolphin-audio-musyx-long-20260930` under `.dev/runs/`.

The report now maps `musyx_voice` timing to `musyx_voices`. Previously this
stage was present in `perf.log` but omitted from the printed report. The
report regression test checks its estimate.

## Accurate and Fast comparison

Both Super Mario 64 runs completed 3,600 VIs at 1.00x speed with zero overruns.
Fast uses nearest-sample interpolation; Accurate uses the N64 four-tap filter.

| Metric | Accurate | Fast | Observed difference |
| --- | ---: | ---: | ---: |
| Resampler estimate | 858.0 ms | 231.9 ms | 73.0% lower |
| Whole-run `pmc1` cycles | 23.116B | 22.696B | 1.8% lower |
| ADPCM estimate | 1,111.5 ms | 1,121.8 ms | Similar |
| exp envelope estimate | 838.4 ms | 838.2 ms | Similar |
| Underruns | 9 | 11 | Two more in Fast |
| Producer queue peak | 109 ms | 113 ms | 4 ms higher in Fast |

This single pair measures the existing mode tradeoff. It does not establish
a quality improvement or a repeatable whole-game gain. Keep Accurate as the
compatibility reference. An aligned PCM reference and listening checks are
still needed before claiming a Hi-Fi quality improvement. The later 1.6.0
settings change exposes experimental enhancements; see
[Audio settings](audio-settings.md) for their separate tests and costs.

- Accurate: `hardware-glN64_wii-20260930-111908-Bm6c`
- Fast: `hardware-glN64_wii-20260930-112527-UTVt`

## Current probe overhead

The current per-variant, steady-call, timing, and producer-peak probes received
a full/disabled/full Super Mario 64 comparison at 3,600 VIs. The disabled
build used `DEBUG_FLAGS=-DPERF_PROF -DPERF_AUDIO_WORK_DISABLE`; existing frame,
DSP, and queue probes remained active. Each flag change used a clean build.

| Build | `pmc1` cycles | Speed | Underruns | Producer peak |
| --- | ---: | ---: | ---: | ---: |
| Full | 23,116,271,764 | 1.00x | 9 | 109 ms |
| Disabled | 23,144,358,915 | 1.00x | 10 | 109 ms |
| Full repeat | 23,130,728,677 | 1.00x | 11 | 109 ms |

The full-probe mean was 0.09% below the disabled run. This small difference
does not resolve a calibrated probe cost. All three had zero overruns and
returned to Homebrew Channel. The working build has full probes restored.

- Disabled: `hardware-glN64_wii-20260930-113239-G7BL`
- Full repeat: `hardware-glN64_wii-20260930-114233-tOSm`

## Next optimization targets

ADPCM was the largest measured stage in the long Super Mario 64 scene.
Final mixing led in Smash, Diddy Kong Racing, and Donkey Kong 64; linear
envelope mixing led in Mario Party 3. Keep those separate reference scenes.
Before changing a loop, compare its PCM and saved state against the existing
implementation, including saturation, buffer overlap, history, and task
boundaries. Then repeat Dolphin checks and the same Wii scene.

MusyX v2, NEAD, NAUDIO MP3/CBFD, and the unsupported 64DD cases still need
appropriate ROMs or captured tasks. The long TWINE chain supplies MusyX v1
voice coverage; it does not cover those other paths. Hi-Fi resampling,
precision, and pitch-preserving sync were subsequently implemented in 1.6.0
as experimental CPU paths. Wide multivoice accumulation, separate voice
routing, and quality validation remain open.

## ADPCM follow-up

The shared residual code shifted negative signed samples and could overflow
signed sums. The new host check reproduced `left shift of negative value
-32768` under UBSan. Residual and reverse-dot sums now use unsigned 32-bit
wrapping, then the original signed shift and 16-bit saturation. This preserves
the tested output rather than replacing it with a wider, different mix.

Alist ADPCM now copies each contiguous native-DMEM frame with `memcpy`.
Wrapped frames and swapped-halfword layouts retain the original stores.
No new hot-path probes were added. `.dev/test_rsp_audio.sh` checks the real
production functions under ASan/UBSan:

- 4,608 residual reference cases, with counts 0–8 and overlapping buffers.
- Reverse-dot sums at counts 0–32, including extreme signed samples.
- Fourteen boundary cases per layout, including exact-end and wrapped frames.
- 512 whole-alist snapshots per layout: both ADPCM formats, init, loop,
  resumed state, input/output overlap, and DMEM wrapping. The original hashes
  remain `9b02f6bc319d81f1` and `06b765ee2c02a0c4`.
- The three existing envelope snapshots, unchanged.

The command passed on this Intel Mac, including `/bin/bash` (Bash 3.2).
The script selects Apple's or GNU's unused-section linker option; Windows
and Linux execution of this new command has not been tested here.

An indirect-decoder-call removal was tested first. Its Super Mario 64 ADPCM
estimate was 1,160.3 ms between baseline runs of 1,126.1 and 1,118.1 ms.
It supplied no measured gain and was discarded. Its first Dolphin run also
hit the time limit at 3,116/3,600 TWINE VIs, so it was not a complete chain pass.

The first frame-copy run received changing controller values and reached
26.006B cycles in a different title animation. It is excluded from performance
comparisons. `scripts/chains/audio_adpcm.txt` now uses the existing replay
system with `scripts/inputs/neutral.txt` to hold port 1 at rest. Stage that
file on the Wii once with HBC-Reborn's `put` command or hardware setup;
Dolphin launchers stage it automatically. Other controller ports remain active.

| Matched Wii, 3,600 VIs per title | Baseline | Frame copy | Frame-copy repeat |
| --- | ---: | ---: | ---: |
| SM64 ADPCM estimate | 1,135.3 ms | 1,027.1 ms | 1,041.3 ms |
| SM64 whole-run cycles | 23.155B | 23.056B | 23.055B |
| SM64 underruns / overruns | 13 / 0 | 12 / 0 | 12 / 0 |
| SM64 producer queue peak | 110 ms | 114 ms | 121 ms |
| TWINE voice estimate | 254.1 ms | 256.2 ms | 252.5 ms |
| TWINE underruns / overruns | 29 / 0 | 31 / 0 | 30 / 0 |

The two frame-copy runs reduced the SM64 ADPCM estimate by 9.5% and 8.3%;
whole-run cycles were 0.43% lower in each. ADPCM work counts differed by about
0.16% from baseline. SM64 held 1.00x speed; TWINE held 0.94x. All three runs
loaded both neutral replays, reached both VI targets, and returned to HBC.
SM64's captured port 1 values stayed zero. MusyX timing stayed
near baseline, with 13,131 voice operations and a peak of 11 in both runs.
These are sampled estimates, not a sound-quality gain or elimination of
underruns. The host snapshots establish equivalence for their test cases.

Local results:

- Baseline binary: `.dev/runs/adpcm-baseline-EWHHqW/`, code at `525c55c`.
- First baseline: `hardware-glN64_wii-20260930-120836-xsB2`.
- Discarded inlining: `hardware-glN64_wii-20260930-121129-I1Tn`.
- Baseline repeat: `hardware-glN64_wii-20260930-121308-DfuA`.
- Live-input run, excluded: `hardware-glN64_wii-20260930-122028-Ttvt`.
- Matched neutral baseline: `hardware-glN64_wii-20260930-122814-ufno`.
- Matched frame copy: `hardware-glN64_wii-20260930-123126-Nzlk`.
- Frame-copy repeat: `hardware-glN64_wii-20260930-123753-vY9S`.
- Muted Dolphin DSP LLE: `dolphin-audio-adpcm-neutral-20260930`.

Dolphin completed both 3,600-VI targets with zero overruns and confirmed both
one-record neutral replays. SM64 and TWINE had 34 and 36 underruns. The Mac
test process needed a forced close after result collection; the game targets
had completed. This does not prove clean Dolphin shutdown or Wii sound quality.
