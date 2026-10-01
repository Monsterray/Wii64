# Graphics changes: Wii64 1.6.6

## Mario Kart warm-load stall

Mario Kart boots alone but stalls after 600 neutral Mario 64 VIs in the same
boot. The reference fails in Dolphin and on the Wii. Disabling interrupt jitter
does not fix it. A 60-VI Mario 64 warm-up does not reproduce it.

The glN64 ROM initialization cleared `gSP` but retained most of `gDP`, including
tile, image, load and half-command state. Clearing the complete per-ROM `gDP`
state before restoring the tile links fixes the reproduced stall. This does not
identify one stale field as the sole cause, or imply a dangling-pointer fault.
CPU, interrupt-queue and device-register resets were already present.

Matched Wii builds differ only in that reset. Each runs Mario 64 for 600 VIs,
then Mario Kart for 3,600 VIs with neutral input and the same audio settings.

| Mario Kart measurement | Reference | Full RDP reset |
|---|---:|---:|
| Average display lists/s | 1.13 | 28.06 |
| Graphics tasks | 3 | 1,569 |
| Audio tasks | 0 | 3,539 |
| Render batches | 64 | 216,478 |
| Completed VIs | 3,600 | 3,600 |

The fixed Dolphin run reached 27.9 DL/s over the same sequence. Both Wii builds
returned to Homebrew Channel. The neutral final fixed capture shows the Nintendo
logo; activity counters alone do not prove a race advances. The reference and
reset results are filed as `baselines/2026-10-01_mk64-warm-*`.

The new `mario_kart_warm` chain checks activity as well as VI completion.
The host test uses the actual `RSP_Init()` and RDP structure layout, filled with
nonzero state. The old reset fails; the new reset passes both bitfield layouts
with ASan/UBSan. See [commands and earlier diagnostics](mario-kart-fix-plan.md).

## Texture hash dispatch

The hash wrapper gives the existing XXH32 implementation constant lengths for
8-, 16-, 32- and 64-byte inputs. Other lengths retain its generic path. Seeds,
row order, palette hashing, TMEM wrapping and native-format behavior do not
change. This is not the snapshot-memo experiment; that experiment remains off.
The host checks compare all lengths from 0 to 4,096 with XXH32 and run 10,000
TMEM mutation comparisons in each dispatch mode.

Two matched Wii A/B/A triples use candidate/reference/candidate, identical
subsystem probes and neutral input. Short scenes run for 900 VIs each; long
scenes run for 3,600. The table compares the mean candidate with its reference
in the longer triple. Negative values mean less time or fewer cycles.

| ROM | Total CPU cycles | Inclusive graphics time | Estimated hash time | Non-sleep wall time |
|---|---:|---:|---:|---:|
| Super Mario 64 | −0.70% | −2.42% | −4.01% | −0.59% |
| Banjo-Kazooie | −0.11% | −1.68% | −0.72% | +0.03% |
| Pokémon Snap | −3.33% | −6.04% | −11.88% | −3.34% |

The short triple gives Snap −2.96% total cycles. Banjo's short result is +0.05%
cycles, effectively unchanged. Close results do not establish a general gain.
Snap's long audio underruns fall from 123 in the reference to 62 and 63 in the
candidates; this is correlated with the graphics change, not a separate audio
fix. All 18 entries completed, with no audio overruns or NAND I/O errors, and
returned to HBC. The instrumented candidate has 2,452 fewer text bytes, 64 more
data bytes, and unchanged BSS than its reference.

Graphics timers are inclusive; hash timers are sampled estimates. Do not add
parent and child durations. CPU counts include probe cost. These are matched
probe builds, not a disabled-probe calibration or full-game compatibility test.
Mario 64's long hash-call count varies slightly between runs. Most scenes remain
limiter-bound, so reduced CPU work does not imply the same percentage FPS gain.

The dispatch defaults on. Use `GLN64_FIXED_HASH_LENGTHS=0` for the reference:

```bash
bash .dev/profile_fixed_hash.sh graphics_survey HBC_AGENT=1
bash .dev/profile_fixed_hash.sh graphics_long HBC_AGENT=1
```

Raw results are in `baselines/2026-10-01_fixed-hash_{short,long}_*`.
[Artifact hashes and queue jobs](../baselines/2026-10-01_fixed-hash-artifacts.json)
identify the frozen DOL/ELF pairs, flags and SDK archive. The test artifacts were
development builds based on 1.6.5; the release version is 1.6.6. The Wii environment
remained HBC/agent 1.8.6, IOS 58 revision 6175, with AHBPROT. Every job used the
central queue and lease server.

## Next checks

The five-entry Wii controller replay completed Mario 64, Mario Kart and Mario
Party 1–3. Mario Kart reached map selection at 28.4 average DL/s, with 17 pad
trace records. Its initial replay had 18 records and held A at the last menu;
the revised replay adds a release and press to enter a race. Mario Party 1's
capture shows a game scene; Mario Party 2–3 captures show their Nintendo logos.
These are startup/replay checks, not full-game compatibility claims.

The revised 22-record replay then entered an active race on the Wii using the
1.6.6 default hash dispatch. Its final capture shows a running timer and kart.
It completed 5,400 Mario Kart VIs at 28.3 average DL/s, consumed 21 pad trace
records and returned to HBC. This different scene is not the neutral A/B above.
Dolphin completed the same revised warm race replay at 28.0 average DL/s and
21 pad trace records. Both 1.6.6 runs are filed as `baselines/*mk64-race-1.6.6`.
[Final build and replay hashes](../baselines/2026-10-01_graphics-1.6.6-artifacts.json)
identify these instrumented builds and the separate ordinary releases.

Dolphin completed the first four entries of that combined chain, but its
240-second host limit interrupted the final 32 MiB ROM load. The incomplete
chain is not filed as a validated baseline. An isolated Mario Party 3 replay
then completed 900 VIs at 46.4 DL/s without invalid-access warnings.

Clean glN64 and Rice 1.6.6 release builds and separate muted MMU/LLE Dolphin
menu boots passed. Guest audio remained enabled. Host subsystem, texture,
audio and ROM/VM tests passed. ROMs and binary captures stay local; the filed Wii
results retain build/config hashes and per-game counters. Replay scripts are shared.

Texture hashing still costs about 7.08 seconds inside Snap's 31.18-second
graphics span in the long scene. Use that remaining cost to evaluate dirty-range
tracking or repeated-tile reuse. Keep the seeded hash and live-RDRAM background
checks intact. Require matched CPU measurements and scene checks before enabling
another cache. Vertex processing and draw submission are the next graphics
layers to separate; GPU execution is not directly measured by these CPU probes.
