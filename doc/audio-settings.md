# Audio settings

Open Settings, Audio, Advanced. Press A on a row to cycle its options.
The Audio On/Off control stays on the main Audio page. Use Save Settings on
the General page to write these choices to `settings.cfg`.

| Setting | Options | Default |
| --- | --- | --- |
| N64 synthesis resampler | Accurate, Fast, Hi-Fi (cubic) | Accurate |
| Output resampler | Wii DSP, Hi-Fi (sinc) | Wii DSP |
| Mixer precision | Accurate, Hi-Fi | Accurate |
| Latency profile | Low, Balanced, Stable (legacy) | Stable |
| Audio synchronization | Native Rate, Follow Speed, Preserve Pitch | Native Rate |

Accurate keeps the N64 lookup filter and original mixer arithmetic. Fast uses
nearest-sample synthesis. Hi-Fi synthesis uses full-phase, four-point cubic
interpolation in alist and MusyX. It is an enhancement, not N64-exact sound,
and it is not an anti-alias filter for large per-voice downsampling ratios.

Hi-Fi output uses an eight-tap, 256-phase windowed-sinc converter to 48 kHz.
Its cutoff changes with the input rate. AESND still handles DSP playback.
The filter runs on the CPU; AESND has no selectable DSP filter API.

Hi-Fi mixing retains fractional envelope/gain products in 64-bit arithmetic
and rounds symmetrically before 16-bit output. Alist exponential, linear,
GoldenEye, NEAD, and final gain mixing are covered, as are MusyX voice
envelopes. N64 command boundaries still saturate to 16 bits. This is not a
floating-point multivoice bus and does not undo clipping in existing PCM.

Low and Balanced limit queued PCM to approximately 40 and 80 ms. Limits are
computed from the requested playback rate, including Follow Speed, then
rounded down to whole AESND input chunks, with a two-chunk minimum. Stable
retains the original queue capacity. These values exclude DSP/output delay
and the time-stretch window. Low/Balanced discard old unread chunks when
necessary; this can create audible gaps. Overrun counts include these drops.
A separate hand-off copy protects samples already supplied to AESND.

Native Rate does not change pitch to follow emulator speed. Follow Speed
changes the playback rate and pitch. Preserve Pitch uses joint-stereo WSOLA
with a nominal 20 ms window and 2 ms search radius. Speed compensation is
limited to 0.5–2.0x. Outside that range, synchronization is approximate.
Queue feedback adjusts tempo by up to 10% to correct clock-estimate drift;
it does not adjust playback pitch. Search starts at the nominal position
and rejects worse candidates early to reduce CPU work.
All enhancements run outside the interrupt-disabled queue copy. Changing
output/sync modes, ROM, or DAC rate clears processing history and queued PCM.
Per-ROM stream counters and the producer duration peak survive DAC/mode changes;
loading a new ROM resets them. `audio_output.playback_hz` logs the requested
playback rate. It does not measure the DSP clock or speaker latency.

Hi-Fi and Preserve Pitch are experimental. They can cost CPU time and add
artifacts, particularly on transients. Leave them off for compatibility
comparisons. No reverb or DSP On/Off control is added.

## Config and diagnostic values

`settings.cfg` uses numeric values:

| Key | Values |
| --- | --- |
| `Audio` | 0 Off, 1 On |
| `AudioQuality` | 0 Accurate, 1 Fast, 2 Hi-Fi |
| `AudioOutputResampler` | 0 Wii DSP, 1 Hi-Fi |
| `AudioMixerPrecision` | 0 Accurate, 1 Hi-Fi |
| `AudioLatency` | 0 Low, 1 Balanced, 2 Stable |
| `AudioSync` | 0 Native Rate, 1 Follow Speed, 2 Preserve Pitch |

Old config files keep their Audio/AudioQuality meaning. Missing keys use
defaults; out-of-range values are ignored. For automation, `diag.cfg` accepts
`audio_quality=accurate|fast|hifi`, `audio_output=dsp|hifi`,
`audio_mixer=accurate|hifi`, `audio_latency=low|balanced|stable`, and
`audio_sync=native|follow|preserve`. Diagnostic values override saved settings.

Run `.dev/test_rsp_audio.sh` for sanitized PCM/state, streaming, pitch, and
queue checks. `scripts/chains/audio_hifi.txt` tests alist and MusyX with the
enhancements enabled. Compare it with the neutral `audio_reference` chain on the
same platform; use the shared Wii queue for hardware runs.

## Verification: 2026-09-30

The Intel macOS glN64 and Rice Wii builds used devkitPPC r50-1 / GCC 16.1.0
and libogc2. Runtime checks below used glN64; Rice was compile-checked only.
Host ASan/UBSan checks passed for envelope/ADPCM reference snapshots,
streaming chunk independence, FIFO bounds, queue wrap/hand-off protection,
unchanged DAC writes, menu pause/resume, config round trips, and all five
menu callbacks. A 440 Hz tone stayed within 5 Hz at 0.5x, 1x, and 2x tempo
over five input rates (8–96 kHz). This is not a listening-quality assessment.

Dolphin LLE/MMU completed both 3,600-VI enhanced scenes with Balanced latency
and Preserve Pitch. A separate 900-VI chain checked Fast, Hi-Fi mixer, Low,
and Follow Speed. Its DSP WAV contained nonzero audio while host playback
was muted. Both audio menu pages were checked from frame captures.

The final Wii pair used 3,600 VIs per game, neutral input, the glN64 renderer,
and full profiling. Reference means Accurate/DSP/Accurate/Stable/Native;
enhanced means Hi-Fi/Hi-Fi/Hi-Fi/Stable/Preserve Pitch. Both returned to HBC.

| Wii scene | CPU cycles, reference → enhanced | Speed, reference → enhanced | Underruns | Overruns |
| --- | --- | --- | --- | --- |
| Super Mario 64 | 23.657 → 29.568 billion (+25.0%) | 1.00x → 1.00x | 11 → 4 | 0 → 5 |
| TWINE (MusyX) | 9.276 → 11.342 billion (+22.3%) | 0.94x → 0.94x | 30 → 7 | 0 → 53 |

Enhanced queued-PCM means were 126.7/136.1 ms (SM64/TWINE), with sampled p95
130/304 ms and producer peaks of 382 ms in both. Stable is a capacity policy,
not a hard 120 ms latency cap. Bursts still overflow; feedback does not
guarantee uninterrupted sound. Overruns and probe samples vary between runs.

Compared with the first enhanced prototype, nominal-first search and early
rejection reduced sampled output time from 27,084 to 18,194 us for SM64 and
26,838 to 15,299 us for TWINE (28/53 sampled calls respectively). These are
sparse inclusive timings, not calibrated whole-run costs. Output-call totals
are now logged so `scripts/audio_report.py` can estimate the stage separately.

Local result directories under `.dev/runs/`:

- `hardware-glN64_wii-20260930-142330-BTrL`: final reference.
- `hardware-glN64_wii-20260930-142642-goxC`: final enhanced.
- `audio-settings-dolphin-final.log`: long enhanced Dolphin chain.
- `audio-settings-dolphin-low-follow.log`: alternate settings and WAV report.

Keep enhancements experimental until matched music/speech/transient listening
and broader game checks establish a benefit. Low latency does not suit every
title; complex audio can still expose WSOLA artifacts or N64 command-boundary
clipping. Custom DSP synthesis and wide multivoice accumulation remain separate
work, not features enabled by these controls.

## Cleanup verification: 1.6.1

Host regressions reproduced the old Follow Speed duration error and counter
reset on a changed DAC. The complete sanitized suite passes after both fixes.
Accurate PCM reference snapshots remain unchanged. No new hot-path probes
were added; the existing output summary now includes `playback_hz`.

The two-game reference chain completed 3,600 VIs per title on Dolphin LLE/MMU
and the Wii. Dolphin host playback was muted; guest audio remained enabled.
The Wii returned to HBC. Relative to the final 1.6.0 reference:

| Wii scene | CPU cycles, 1.6.0 → cleanup | Speed | Underruns | Overruns |
| --- | --- | --- | --- | --- |
| Super Mario 64 | 23.657 → 23.632 billion (-0.11%) | 1.00x → 1.00x | 11 → 14 | 0 → 0 |
| TWINE (MusyX) | 9.276 → 9.257 billion (-0.21%) | 0.94x → 0.94x | 30 → 29 | 0 → 0 |

This single repeat supports compatibility, not a speed improvement. Producer
queue peaks were 117/200 ms; requested rates were 32,006/22,047 Hz.
The sampled queue peaks are separate metrics. Results are preserved in
`.dev/runs/hardware-glN64_wii-20260930-151048-4Yyy` with the matching DOL/ELF.
An earlier cleanup repeat, `hardware-glN64_wii-20260930-145726-5iYQ`, had
12/31 underruns and zero overruns. The long Dolphin run is preserved in
`.dev/runs/dolphin-audio-closeout-20260930`; the final version's 900-VI
default-mode check is in `.dev/runs/dolphin-audio-1.6.1-final` (1.00x,
six underruns, zero overruns).

A separate 900-VI Dolphin check exercised Balanced / Follow Speed and the timed
launcher. It completed at 1.00x, but recorded 150 underruns and 18 latency-cap
drops. This does not establish acceptable audio quality for that policy. The
long reference used Stable / Native and had zero overruns. Dolphin diagnostic
XFB captures remain solid magenta; the Wii captures showed the expected scenes.
Use Wii captures for scene confirmation until the Dolphin capture issue is fixed.
