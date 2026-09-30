# Audio development plan

Keep the original N64 mix as the compatibility reference. Wii64 currently sends
one stereo PCM stream to one AESND voice. AESND uses the Wii DSP for playback, but
its public interface has no reverb, mixer-precision, interpolation-quality, or
pitch-preserving time-stretch control. Do not display controls without a working
implementation. Do not add a DSP On/Off control.

## 1. Measure the path

- Record time in ADPCM decode, per-voice resample, envelope/mix, effects, and
  audio output separately. Record queue depth in samples, underruns, overruns,
  output rate, and AESND DSP load. The ring's capacity is not measured latency.
- Profiling logs `queue_ms` twice per second and `queue_peak_ms` as the peak
  of those samples. These are queued-PCM estimates, not end-to-end delay or
  exact peaks.
- A 900-VI Super Mario 64 Dolphin LLE check had 30 queue samples: 0-96 ms,
  mean 21.9 ms. This verifies the probe, not a Wii latency target.
- `audio:` and `audio_stages:` in `perf.log` count alist resampling, ADPCM
  output samples, envelope mixing, final mixing, and MusyX voices/effect taps.
  Each counter updates once per operation, not once per sample. These are work
  counts, not stage timings or persistent N64 voice IDs. Zero MusyX counts mean
  the tested game did not use that path; they do not validate it.
- `audio_gaps:` counts unsupported Mats/EFZ tasks, resampler flag 2, and
  MusyX v2 `ptr_10` uses. A zero means the tested scene did not reach that
  case; it does not establish correct emulation. `scripts/audio_report.py`
  prints nonzero gaps and sampled queue min/mean/p95/max without new Wii probes.
- `audio_output:` counts AESND stream requests and buffers supplied, and logs
  the N64 AI input rate and exact producer-side queue peak in bytes since `RomOpen`,
  including loading before the first guest VI. This distinguishes a stalled
  callback from a full queue when an overrun occurs. It is not a measure of
  speaker latency. The reported milliseconds use the last input rate, so they
  are approximate if a game changes DAC rate during the run.
- A 900-VI Super Mario 64 Dolphin DSP LLE run with this counter reached 1.00x
  speed, zero overruns, and a 112 ms producer peak. The older twice-per-second
  samples peaked at 93 ms in the same run. The first two Wii uploads failed
  before boot. The 2026-09-30 retry passed both survey chains and longer checks;
  Super Mario 64's 3,600-VI Accurate run had a 109 ms producer peak and zero
  overruns. See [the current Wii measurements](audio-profile-2026-09-30.md).
  The earlier transfer failures remain unexplained.
- `audio_time:` samples one in 127 calls for resampling (including ZOH),
  ADPCM, each envelope variant, mixing, MusyX voices, and MusyX effects. It reports
  sampled calls, samples, and microseconds. The time is inclusive of clock
  reads and interruptions, and the samples can vary by game segment. Divide
  sampled time by sampled calls or samples for a rough stage ranking; do not
  treat it as exact total audio time. Use
  `DEBUG_FLAGS=-DPERF_PROF -DPERF_AUDIO_TIMING_DISABLE` for a count-only build.
  Compare that build with full timing in a Wii A/B/A run before relying on
  timing-based optimization decisions.
- To measure probe cost, build with `.dev/build_profiling.sh glN64_wii`, run a
  fixed Wii chain, build with `.dev/build_profiling.sh glN64_wii
  'DEBUG_FLAGS=-DPERF_PROF -DPERF_AUDIO_WORK_DISABLE'`, run the same chain, then
  rebuild and run the full-probe build again. Each build cleans shared objects.
  Compare `pmc1`, idle time, speed, queue peak, and underruns. The disabled
  build retains the existing frame, DSP, and queue probes.
- On Wii, a 3,600-VI Super Mario 64 A/B/A check gave 23.782/23.772/23.791
  billion `pmc1` cycles (full/disabled/full). The full-probe mean was 0.06%
  above the control. All three runs held 1.00x speed; underruns were 10/9/9.
  This is an observed difference, not a calibrated per-probe cost: title
  activity and underruns vary between runs. Keep the probes at operation
  boundaries and repeat comparisons after adding hot-path instrumentation.
- The full-probe build also completed a 900-VI Super Mario 64 Dolphin DSP LLE
  run at 1.00x speed. It recorded 16,770 resample calls, 16,496 ADPCM calls,
  16,204 envelope-mix calls, and 3,500 mix calls. Dolphin does not replace
  the Wii overhead comparison.
- Sparse timing completed 900-VI Dolphin DSP LLE checks for Super Mario 64 and
  Mario Kart 64. In both openings, envelope mixing and resampling ranked above
  final mixing. These host-emulated timings guide which Wii stage to measure
  first; Dolphin's emulated CPU-cycle count cannot quantify Wii probe cost.
- A 3,600-VI Super Mario 64 Wii timing run completed at 1.00x speed. The
  sampled operation times were 13,686 us for 1,232 envelope-mix calls,
  8,748 us for 1,234 ADPCM calls, 6,749 us for 1,236 resample calls, and
  608 us for 112 final-mix calls. This points to envelope mixing first.
  The first run did not return to Homebrew Channel. Later runs returned;
  the cause of the first return failure remains unknown.
- A separate timed/count-only/timed Wii check measured 23.787/23.736/23.707
  billion cycles over the same 3,600-VI Super Mario 64 scene. The timed-run
  mean was 0.05% above the control, below the variation between timed runs;
  this does not resolve a timer cost. Speed held at 1.00x. This check preceded
  the per-variant and steady-call counters. A current full/disabled/full check
  gave 23.116/23.144/23.131 billion cycles, all at 1.00x speed. The full mean
  was 0.09% below the control; this does not resolve a calibrated probe cost.
  See [the current Wii measurements](audio-profile-2026-09-30.md).
- The eight-game Wii envelope survey and verified linear-mixer optimization are
  recorded in [audio-profile-2026-09-27.md](audio-profile-2026-09-27.md).
- Use the same scenes and ROMs in Dolphin DSP LLE and on the Wii. Keep Accurate
  mode as the reference; include titles using both alist and MusyX audio.
- Save a short PCM capture from each mode for sample, spectrum, and listening
  comparisons. Do not use total-frame speed alone to judge audio quality.
- Dolphin can capture this without changing Wii64's audio hot path. Run
  `WII64_DOLPHIN_DSP_HLE=False WII64_DOLPHIN_DUMP_AUDIO=True
  .dev/dolphin_test.sh wii64-glN64.dol 90 audio_quality=accurate
  'chain=900 sd:/wii64/roms/Super Mario 64.v64'`, then repeat with
  `audio_quality=fast`. Automated Dolphin runs are muted on the Mac by default;
  interactive runs remain audible. Set `WII64_DOLPHIN_MUTE_AUDIO=False` to hear
  an automated run. WAVs stay under the ignored
  `.dev/dolphin_profile/AudioCaptures/` directory. Run
  `python3 scripts/audio_capture_report.py <capture-directory>` to identify
  non-silent files. In the tested DSP LLE build, `dspdump.wav` and `dtkdump.wav`
  were silent, while `dspdump1.wav` held the AESND output; do not infer signal
  from the filename or from the existence of a WAV alone. This capture includes
  boot/menu audio and Dolphin's emulated DSP output, not the Wii analog output.
  Compare aligned gameplay segments and listen before claiming a quality gain.
  `python3 scripts/audio_capture_compare.py A.wav B.wav` aligns a short active
  segment and reports sample-difference RMS and peak. These values detect a
  changed signal; they do not rate fidelity. For the existing Accurate/Fast
  Super Mario 64 captures, a 1,024-frame segment differed by RMS 192.01 and
  peak 984 after an 18-frame alignment. A longer gameplay capture and listening
  check are still needed.
- For a chosen interval, add `--start-s 25 --duration-s 1`. The start is
  measured from the WAV start, not from the first guest VI. `--lag-frames N`
  reuses a known offset; correlation helps identify captures that do not
  align. Two 2,400-VI Dolphin runs with the same `sm64_start` input replay
  produced non-silent 43.4-second WAVs, but had 60 and 9 underruns. Their
  first active 1,024 frames correlated at 0.956 after alignment; a later
  one-second interval at 25 seconds correlated at only 0.104 after a new
  alignment. These post-DSP captures are not a reliable sample-level
  fidelity comparison across these runs. The diagnostic XFB image was solid
  magenta, so it also does not prove the selected interval was gameplay.
- The AI output ring now rejects oversized lengths without signed arithmetic,
  ignores zero-length writes, and preserves its spare-space rule. Check its
  boundaries with `cc -std=c11 -Wall -Wextra -Werror -DPERF_PROF -Itests/audio_stubs
  -fsanitize=address,undefined tests/audio_buffer_test.c -o
  /tmp/wii64_audio_buffer_test && /tmp/wii64_audio_buffer_test`.

### ROMs needed for microcode coverage

The dispatch signatures in `rsp_hle/hle.c` identify these targets. Add legally
obtained ROMs to the SD test set; confirm each path from `audio_envmix` and
`audio_time` counters before treating a run as audio coverage. Booting a title
screen alone does not prove that its music or effects path ran.

| Path | First game to add | Alternative or special case |
| --- | --- | --- |
| ABI1 GE envelope | GoldenEye 007 (staged and verified) | Diddy Kong Racing also reached GE in the tested scene |
| ABI1 BC | Diddy Kong Racing | Blast Corps |
| NEAD | Wave Race 64 (Europe) | F-Zero X, Star Fox 64, or 1080° Snowboarding; Mario Kart 64 currently has a black-screen blocker |
| MusyX v1 | The World Is Not Enough | Gauntlet Legends or Rogue Squadron |
| MusyX v2 | Indiana Jones and the Infernal Machine | Star Wars: Battle for Naboo |
| NAUDIO DK | Donkey Kong 64 (staged and verified) | No equivalent game listed in dispatch |
| NAUDIO MP3 | Banjo-Tooie | Jet Force Gemini or Perfect Dark |
| NAUDIO CBFD | Conker's Bad Fur Day | No equivalent game listed in dispatch |

The first five-game coverage chain (`audio_coverage`) ran for 900 VIs per title
on Dolphin and Wii. Wii captures showed title/intro scenes; Dolphin's five
diagnostic captures were solid magenta, so its VI results do not verify video.
The tested Diddy Kong Racing scene used the GE envelope, not a distinct BC
envelope. The USA Wave Race 64 scene used the exponential envelope, not NEAD;
the NEAD recommendation above is specifically the Europe version. The World Is
Not Enough called MusyX effects but reported zero MusyX voice calls in this
scene. The 2026-09-30 `audio_musyx_long` chain reached a first-person attract
demo at 3,600 VIs and recorded 13,131 MusyX voice operations, with a peak of
11 voices per subframe. Use this longer chain for MusyX v1 voice measurements.
Select verified scenes before claiming per-voice coverage for other paths.

The review's unsupported NEAD Mats path needs Mario Artist: Talent Studio;
the EFZ path needs F-Zero X Expansion Kit. Both are 64DD titles and need a
separate disk-capable test setup. Do not mark those paths complete from a
cartridge-only run. MusyX v2 `ptr_10` also needs a captured task that sets it;
running a MusyX v2 title does not by itself prove that branch executes.

## 2. Resampling

- Keep explicit mode IDs: Accurate is the N64 4-tap filter; Fast is nearest
  sample. New IDs must have their own code paths, diagnostics, and tests.
- Prototype Hi-Fi as a separate CPU-side filter on the *N64 voice* before it is
  mixed. The current alist history retains four samples, so an eight-tap filter
  needs new history handling across task boundaries. Include MusyX or document
  that the option applies only to compatible microcode. Do not relabel the
  existing 4-tap filter Hi-Fi.
- The finished stereo signal still uses AESND's DSP voice and output-rate
  conversion. AESND does not expose a selectable DSP resampler. A custom DSP
  program would be a separate project with its own compatibility tests.
- Expose Hi-Fi only if spectral/listening tests show a benefit and hardware
  runs show acceptable CPU cost, game speed, and zero new underruns.

## 3. Mixer precision and reverb

- For a Hi-Fi mixer, prototype wider intermediate accumulation before final
  16-bit output. Compare it with the original fixed-point, saturating N64
  behavior. Replacing saturation can change game audio, so retain Accurate as
  the default and expose Hi-Fi only after measured improvement.
- AESND has no reverb option. Games may already mix reverb into the PCM, so
  output-stage processing cannot remove it. Do not add a Reverb control unless
  a separate optional post-mix effect is implemented and named as such.

## 4. Latency and pitch

- Measure actual queue occupancy and output delay first. Then test bounded
  Low, Balanced, and Stable queue targets. Never wait indefinitely for the DSP;
  compare audible delay, underruns, game speed, and frame-time variance on Wii.
- AESND frequency changes alter pitch. Pitch-preserving sync requires a new
  time-stretch path on the finished stereo stream; test a small overlap-add or
  WSOLA prototype against music, speech, and silence before adding a setting.
  The DSP API cannot do this itself. Wii locked cache (LC) is fast scratch
  memory, not a second processor; use it only if profiling proves it helps.

## 5. Per-voice synthesis and optional Wii routing

- Run `.dev/test_rsp_audio.sh` before changing RSP arithmetic. It checks
  ADPCM residuals against a 64-bit reference with explicit 32-bit wrapping,
  both ADPCM formats and saved state, DMEM boundary stores in both layouts,
  and the existing envelope snapshots under ASan/UBSan. This host check does
  not replace Dolphin and Wii runs.
- Profile per-voice ADPCM decode, resampling, envelope, gain, and pan by audio
  microcode. Optimize the hottest operation first, preserving a sample-level
  comparison to Accurate output. Prefer lookup reuse, fixed-point arithmetic,
  and cache-friendly working sets before adding a second playback path.
- Instrument a game's command list to identify temporary voice sources and
  effects buses. There is no universal voice ID or fixed N64 voice count.
- Only then prototype routing *dry* sources into separate AESND voices. Check
  voice allocation, sample timing, pan, volume, wet/effects routing, and the
  32-voice limit. Keep the original stereo path for unsupported microcode and
  for any scene where the DSP path differs audibly or costs more. Do not expose
  per-voice routing in Settings before Wii hardware comparisons pass.
