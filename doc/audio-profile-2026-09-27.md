# Wii audio profile, 2026-09-27

Build: glN64 Wii, Accurate audio, `PERF_PROF`. Each valid survey entry ran for
900 guest VIs. `scripts/chains/audio_survey.txt` lists the eight ROMs staged on
the Wii SD card. The first hardware manifest also named three absent USA Mario
Party files; those entries failed to load and are excluded below. Dolphin had
those files, but the corrected survey uses the same eight names on both systems.

Run `python3 scripts/audio_report.py RUN_DIR` to show every active audio stage.
The report estimates stage time as `sampled_us * total_calls / sampled_calls`;
the timer samples one call in 127. These are estimates, not exact total CPU
time. Each `perf.log` also contains unsummarized queue, DSP, sample, and game
counters. Local run directories are ignored by Git:

- Baseline Wii: `.dev/runs/hardware-glN64_wii-20260927-222636-TlZZ/`
- Optimized Wii: `.dev/runs/hardware-glN64_wii-20260927-223639-8Cn0/`
- Optimized Wii repeat: `.dev/runs/hardware-glN64_wii-20260927-224159-iZaQ/`
- Final-build Wii: `.dev/runs/hardware-glN64_wii-20260927-224647-VIHf/`
- Optimized Dolphin: `.dev/runs/dolphin-audio-survey-optimized-20260927/`
- Final-build Dolphin Smash: `.dev/runs/dolphin-audio-smash-final-20260927/`

| Game | Active envelope | Steady calls | Baseline estimate | Optimized / repeat | Wii cycles, baseline to repeat |
| --- | --- | ---: | ---: | ---: | ---: |
| Super Mario 64 | exp | 82% | 183 ms | 184 / 186 ms | 4.826B to 4.812B |
| Banjo-Kazooie | lin | 92% | 114 ms | 56.5 / 56.5 ms | 5.261B to 5.203B |
| Mario Party | exp | 89% | 10.8 ms | 10.0 / 10.0 ms | 2.408B to 2.405B |
| Mario Party 3 (Europe) | lin | 56% | 582 ms | 379 / 379 ms | 3.283B to 3.148B |
| Pokémon Snap | exp | 55% | 156 ms | 157 / 156 ms | 7.982B to 7.997B |
| Super Smash Bros. | lin | 63% | 336 ms | 226 / 223 ms | 5.264B to 5.255B |

The linear mixer improved by about 51% in Banjo-Kazooie, 35% in Mario Party 3,
and 33% in Smash Bros. in the sampled estimates. The game-wide cycle changes
are smaller and can include scene variation; guest speed stayed near its prior
value. Mario Kart 64 reached 900 VIs but remained on a near-black screen at
under 1 fps, with no audio-stage calls. Mario Party 2's tested scene used the
final mix but no envelope, ADPCM, or resample calls. Neither scene measures
per-voice audio performance. No surveyed game used the GE, NEAD, or MusyX path.

All eight corrected entries passed on the Wii and returned to Homebrew Channel
in both optimized runs. The final build (also containing the signed-shift fix)
passed all eight entries; its result checker and Homebrew Channel reachability
were verified separately because an in-flight edit interrupted the runner
after it printed the results. Dolphin passed seven full entries in the optimized
survey; its timeout cut Smash Bros. short at 488/900 VIs. A separate final-build
Dolphin run passed Smash Bros. at 900/900 VIs. The first hardware manifest's
`press_a_periodically` replay did not exist on the SD card, so menu navigation
was not tested; the corrected survey does not claim to exercise gameplay.

The `alist_envmix_lin` zero-step path reuses gains after one `ramp_step` per
channel, but retains the original sample order and state save. Host snapshots
cover 256 deterministic cases for each exp, lin, and GE mixer, including zero
count and overlapping input/output buffers. UBSan found negative signed shifts
in all three mixers; multiplication by 65536 removed that undefined behavior
without changing the snapshots.

## Exponential envelope follow-up

The exp path now calculates gains once when both ramps equal their targets.
Changing ramps still use the original loop. The host snapshot was expanded to
include steady calls with and without aux output, odd counts, and overlapping
buffers; all three mixer hashes stayed unchanged under UBSan. The optimized
build passed a full eight-game Wii survey and a two-game Wii repeat, returning
to Homebrew Channel both times. Dolphin passed 900 VIs each for Super Mario 64
and Pokémon Snap.

| Game | Before exp change | Full Wii survey | Wii repeat | Final build | Whole-game cycles, before to final |
| --- | ---: | ---: | ---: | ---: | ---: |
| Super Mario 64 | ~184.5 ms | ~88.8 ms | ~85.7 ms | ~88.2 ms | 4.812B to 4.739B |
| Pokémon Snap | ~156.4 ms | ~104.1 ms | ~102.6 ms | ~103.2 ms | 7.997B to 7.958B |

These are sampled stage-time estimates for 900 VIs, not exact wall-time
savings. The full run is
`.dev/runs/hardware-glN64_wii-20260927-230459-Th2l/`; the short repeat is
`.dev/runs/hardware-glN64_wii-20260927-231003-BVC3/`. The clean final build
passed the same two games on Wii and Dolphin at 900 VIs each; its results are
`.dev/runs/hardware-glN64_wii-20260927-232002-MoXO/` and
`.dev/runs/dolphin-audio-exp-final-20260927/`. The Wii returned to Homebrew
Channel. Wii guest speed and underruns stayed at their prior levels in these
scenes.

The final mix looked costly in Smash Bros., but a Wii probe found **zero**
zero-gain calls in 317,256 mix calls across the eight surveyed games. Its
temporary probe run is `.dev/runs/hardware-glN64_wii-20260927-231146-tp8C/`.
No zero-gain branch remains in the source. PowerPC disassembly also showed
that the compiler already separates Accurate and Fast resampler loops; a
manual branch split would duplicate that work.

## Next work

1. Stage a real A-button replay on the Wii SD card and repeat audio measurements
   in gameplay, not only title scenes. Validate replay files before upload.
2. Profile MusyX, GE, and NEAD with games that use them. Profile exp's
   changing-ramp calls separately before altering their per-block state math.
3. Diagnose Mario Kart's black-screen/near-zero-fps scene before using it in
   audio comparisons.
4. Repeat the count-only versus timed-probe overhead check after the new
   per-variant and steady-call counters. Avoid per-sample probes.

## Output queue follow-up, 2026-09-28

The offline audio report now summarizes the existing twice-per-second queue
samples. These are queued PCM only, not speaker latency. The AI ring also has
a sanitized host boundary test. It rejects zero and oversized writes without
signed-length arithmetic and keeps its original spare-space rule.
The run labels below are suffixes of ignored local directories under
`.dev/runs/hardware-glN64_wii-`: `233806-mDUU` uses date `20260927`;
the other labels use `20260928`.

| Wii run, 900 VIs per game | Super Mario 64 queued mean/peak | Pokémon Snap queued mean/peak | Pokémon Snap overruns |
| --- | ---: | ---: | ---: |
| Before ring change (`233806-mDUU`) | 21/88 ms | 115/148 ms | 0 |
| First gap-counter run (`084924-ulBi`) | 27/142 ms | 542/574 ms | 780 |
| Identical-build repeat (`085123-Upxr`) | 20/84 ms | 117/151 ms | 0 |

The first gap-counter run saturated the ring. The same binary, chain, and Wii
returned to normal on repeat. All eight games in the full survey
(`085252-jOWq`) reached 900 VIs with zero overruns and returned to Homebrew
Channel. Whole-game speed and DSP usage matched the previous eight-game survey
closely. All four `audio_gaps:` counters stayed zero; these titles do not test
the unsupported paths. The intermittent saturation cause is unresolved. New
AESND request/feed counters are in the next profiling build so a future event
can distinguish callback starvation from excess producer writes. That build
passed a two-game Dolphin DSP LLE run (900 VIs per game, zero overruns).
The first Wii attempt waited because Homebrew Channel's upload port stopped
listening after the full survey, although the console still answered ping.

The Homebrew Channel restart restored the upload. The callback-counter build
then passed the two-game Wii smoke run (`20260928-093310-7bd6`): both games
reached 900 VIs, zero overruns, and Homebrew Channel returned. The new
five-game coverage run (`20260928-094012-VlPW`) fetched 96 MiB of missing ROMs
over the LAN, then passed all five entries at 900 VIs and returned to Homebrew
Channel. These are title/intro scenes, not gameplay. Wii captures were
recognizable; all five Dolphin diagnostic captures were solid magenta.

| Wii title | Observed audio path | Underruns | Overruns | Guest speed |
| --- | --- | ---: | ---: | ---: |
| GoldenEye 007 | GE envelope | 2 | 0 | 0.99 |
| Diddy Kong Racing | GE envelope | 0 | 0 | 0.99 |
| Wave Race 64 (USA) | exp envelope | 9 | 0 | 0.98 |
| The World Is Not Enough | MusyX effects; no voices counted | 15 | 0 | 0.83 |
| Donkey Kong 64 | lin envelope | 10 | 0 | 0.84 |

`audio_gaps:` stayed zero in all five. The two slower guest-speed entries had
idle time above 50%, so this run does not show a CPU throughput limit. Capture
gameplay and verify audio output before treating the underrun counts as
audible defects. The Dolphin magenta XFBs need separate diagnosis.

## GE envelope follow-up, 2026-09-29

GoldenEye 007 and Diddy Kong Racing used the GE envelope in the 900-VI intro
chain. Before the new fast path, 89% and 96% of their GE calls had zero ramp
steps. The fast path runs each zero-step ramp once, computes four gains once,
and keeps the original sample-mixing order. A host snapshot covers zero-rate
ramps, zero counts, aux on/off, buffer overlap, mixed samples, and saved state.

| Dolphin DSP LLE, 900 VIs | GE stage before | GE stage after | Calls |
| --- | ---: | ---: | ---: |
| GoldenEye 007 | ~456 ms | ~223 ms | 16,034 |
| Diddy Kong Racing | ~206 ms | ~88 ms | 7,516 |

These are sampled stage-time estimates, not whole-game or Wii performance.
Both optimized Dolphin entries reached 900 VIs with unchanged underrun counts
(2 and 0).

The Wii A/B/A check used the same two-game chain and two copies of the saved
baseline DOL around one optimized run. All six entries reached 900 VIs, and
each run returned to Homebrew Channel. The first queued attempt exposed a
macOS Bash 3.2 empty-array error in the runner before any upload; it was fixed
before these three runs.

| Wii game | GE estimate, baseline A / optimized / baseline B | CPU cycles, A / optimized / B | Underruns |
| --- | ---: | ---: | ---: |
| GoldenEye 007 | ~266.9 / 139.7 / 267.2 ms | 3.023 / 2.929 / 3.023 B | 2 / 2 / 2 |
| Diddy Kong Racing | ~120.9 / 58.1 / 120.6 ms | 2.745 / 2.695 / 2.744 B | 0 / 0 / 0 |

The optimized Wii run reduced the GE estimate by about 48% in GoldenEye and
52% in Diddy. Whole-game CPU cycles fell about 3.1% and 1.8%, respectively.
Guest speed stayed 0.99, and both games stayed at 900 VIs. GE estimates come
from sampled calls, while the CPU cycles are whole-game counters; neither
proves audio quality. The ignored local run directories are:

- Baseline A: `.dev/runs/hardware-glN64_wii-20260929-160655-I78m/`
- Optimized: `.dev/runs/hardware-glN64_wii-20260929-160814-Z3Wz/`
- Baseline B: `.dev/runs/hardware-glN64_wii-20260929-160933-k5lI/`

After these runs, the Wii reported HBC-Reborn 1.5.0, developer protocol 3,
and `sd` as its mounted device. The local HBC-Reborn source checkout documents
1.4.1, so its version-specific documentation may lag the installed channel.

## Dolphin PCM capture, 2026-09-29

The isolated Dolphin runner captured the same 900-VI Super Mario 64 opening
in Accurate and Fast with DSP LLE and Mac playback muted. Both reached 900 VIs.
The `dspdump1.wav` track was non-silent 48 kHz stereo in both runs: 18.41 s,
RMS 2433.4 in Accurate and 2476.0 in Fast. The `dspdump.wav` and `dtkdump.wav`
tracks were silent. Muting Mac playback did not mute the WAV capture. These
whole-capture levels include Wii64 boot/menu time and do not measure audible
quality or prove sample-level equivalence. The ignored capture directories are
`.dev/dolphin_profile/AudioCaptures/run-EdaIUu/` (Accurate) and
`.dev/dolphin_profile/AudioCaptures/run-JdL4O3/` (Fast).
