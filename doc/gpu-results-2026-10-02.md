# GPU survey, 2026-10-02: where graphics time goes

The question: where in the graphics path do we start to optimize? The answer
from the Wii: **not in the GP**. The GP is idle more than 85% of the time in
every scene. The cost is CPU-side display-list translation in glN64, and one
command in Mario Kart takes most of it.

## How it was measured

- Chain `scripts/chains/gpu_survey.txt`: nine 3D scenes, dynarec, glN64.
  SM64 and Mario Kart use replays that reach the castle and a race. The other
  scenes have neutral input. The final frames show 3D play in SM64, Kart,
  Snap, Smash and Wave Race. Banjo, GoldenEye, Diddy Kong Racing and DK64 are
  still on logo screens at 900 VIs. Use those four only as weak evidence.
- `bash .dev/profile_subsystems.sh gpu_survey`: full/control/full on the Wii
  through the central queue. All 27 entries reached their VI targets and the
  Wii returned to HBC after each job. A fourth run used the full probes plus
  the new per-command split (below).
- New in 1.6.8: the GP's own counters in every `PERF_PROF` build, and the
  `gfx_command` time split by GBI opcode. See
  [probe boundaries](subsystem-profile-2026-09-30.md#gp-counters-168-every-perf_prof-build).
- Every run passed the counter check: GP clocks matched the time base x 4.
  The GP numbers repeat within 0.2 points between the three runs.
- Results: `baselines/2026-10-02_hw_gpu_survey_{full1,control,full2,opcodes}`.
  Hashes, flags and job IDs: `baselines/2026-10-02_gpu-survey-artifacts.json`.
  Frozen binaries stay in `.dev/runs/subsystem-survey-20261001-235514-EMk2` and
  `.dev/runs/gpu-opcodes-20261002-001121`.

## The GP is not the limit

Control run (no CPU probes). Percentages are of GP clocks.

| Scene | Rasterizer busy | Busiest 500 ms | XF waiting for input | XF waiting for output | EFB copies | FIFO overflows/s | Blended px/frame |
|---|---:|---:|---:|---:|---:|---:|---:|
| Super Mario 64 | 2.0% | 3.4% | 95.4% | 2.9% | 1.2% | 12.1 | 2.10 screens |
| Mario Kart 64 | 2.5% | 4.0% | 97.0% | 1.5% | 1.0% | 0.0 | 2.80 screens |
| Banjo-Kazooie | 3.8% | 4.9% | 88.1% | 10.6% | 1.4% | 55.9 | 4.20 screens |
| Pokemon Snap | 6.0% | 10.4% | 88.3% | 6.1% | 2.3% | 0.0 | 2.70 screens |
| Super Smash Bros. | 6.5% | 9.4% | 85.1% | 11.2% | 2.4% | 152.9 | 3.38 screens |
| GoldenEye 007 | 1.3% | 2.0% | 96.8% | 1.0% | 2.0% | 0.0 | 0.74 screens |
| Wave Race 64 | 1.3% | 2.2% | 96.4% | 1.5% | 0.8% | 0.0 | 2.11 screens |
| Diddy Kong Racing | 1.9% | 2.8% | 97.0% | 2.3% | 1.2% | 0.6 | 2.19 screens |
| Donkey Kong 64 | 1.3% | 2.4% | 97.9% | 1.8% | 1.8% | 0.0 | 1.57 screens |

- Fill, TEV, Z and EFB copies are not worth work now: the rasterizer is busy
  at most 10% of any 500 ms window.
- The CPU almost never waits for the GP: all `GX_DrawDone` waits total 2.6 ms
  per game.
- FIFO overflows occur in bursts in Banjo and Smash: the CPU sends geometry
  faster than XF takes it for a short time. The control build overflows more
  than the probe builds (Smash 2,321 against 1,875) because its CPU is faster.
  The cost is inside `draw_triangles` (0.2–0.5 s per scene, 1.5–3% of wall
  time). Look at it again after the CPU-side changes below, because a faster
  CPU will overflow more often.

## Where the CPU spends graphics time

Display-list translation (`gfx_list`) takes 3–48% of wall time: 47.6% in the
Kart race and 42.6% in Snap. The opcode run splits it by command. Calls are
exact; times are estimates from the 1-in-127 samples.

| Scene | First | Second | Third |
|---|---|---|---|
| Mario Kart 64 | `SETCIMG` 80.2%, 5.2 ms/call | `TEXRECT` 8.9% | `LOADTILE` 2.9% |
| Pokemon Snap | `TRI2` 50.7% | `VTX` 17.8% | `LOADBLOCK` 9.4% |
| Super Smash Bros. | `TRI2` 52.4% | `VTX` 14.1% | `FILLRECT` 10.7%, 160 us/call |
| Super Mario 64 | `TRI1` 55.2% | `VTX` 24.9% | `LOADBLOCK` 9.3% |
| Wave Race 64 | `TRI1` 33.1% | `QUAD` 25.8% | `VTX` 16.2% |
| Banjo-Kazooie | `TRI2` 64.0% | `LOADBLOCK` 11.6% | `VTX` 10.5% |
| GoldenEye 007 | `TRI2` 46.9% | `VTX` 31.4% | `TEXRECT` 8.9% |
| Diddy Kong Racing | `DMATRI` 52.2% | `LOADBLOCK` 13.0% | `VTX` 11.0% |
| Donkey Kong 64 | `FILLRECT` 33.9%, 93 us/call | `TRI2` 23.5% | `TRI1` 18.2% |

## Where to start, in order

1. **Mario Kart's `SETCIMG`: 22.4 s of the 61 s race, 36% of wall time.**
   `gDPUpdateColorImage` (`glN64_GX/gDP.cpp`) is a Kart-only hack for the
   in-game billboard screen. On each color-image change (2.6 per frame) it
   converts the whole displayed XFB from YUYV to RGBA5551 into RDRAM, with
   float multiplies per pixel. The race runs at 0.978x with 23% limiter
   sleep. Candidate fix: let the GP do the work. One EFB-to-texture copy
   (`GX_CopyTex`) can scale and convert at the N64 size; the CPU then only
   reorders bytes. Alternatively, convert only when the game reads that image.
   Check the billboard and the race picture against the reference.
2. **The triangle path (`TRI1`, `TRI2`, `QUAD`, `DMATRI`): 33–65% of
   display-list time in every 3D scene, 1.9–8 µs per command.** GX submission
   (`draw_triangles`) and state (`gfx_state`) explain only part of it. Next
   probe: split glN64's triangle handling (`gSPTriangle`, `OGL_AddTriangle`,
   clipping, batch flush) before changing it.
3. **`VTX`, the CPU vertex transform and lighting: 10–31%, 3–11 µs per
   command.** Candidates are paired-single math and fewer per-vertex branches.
4. **Texture loads (`LOADBLOCK`, `LOADTILE`, with `tex_hash` inside): 4–13%.**
   Continue the hash work already documented in
   [graphics results](graphics-results-2026-10-01.md).
5. **`FILLRECT`: 30–280 µs per call.** 34% in DK64, 11% in Smash. Find out
   what a fill does (EFB or depth clear, framebuffer emulation) before
   changing it.

## Probe cost and limits

- Full versus control: +9–20% CPU cycles, wall time within 1%, because the
  limiter absorbs the cost. Underruns: Kart 104/105 against 77 in the control.
  Use the control build for speed comparisons.
- One opcode run; the estimates can alias. Confirm a candidate with matched
  candidate/reference/candidate runs, as for earlier experiments.
- glN64 only. Rice has its own display-list loop and no opcode split.
- The GP counters measure the GP. They do not measure the CPU time in GX
  calls; the CPU spans above do that.

## Runner fixes found on the Windows bench

- `.dev/hardware_run.sh` passed its frozen copy as one 8 KB argument. On
  Windows it reached bash cut short, so the job failed after the results were
  in (the three survey jobs show exit 2 for this reason). The copy now goes
  as a file under `.dev/runs/`; the fourth job used it and exited 0.
- `.dev/profile_subsystems.sh` and `.dev/hardware_run.sh` used `python3`
  before they sourced `.dev/env.sh`, which supplies it on Windows.

## Changes from this survey (2026-10-02)

Each change was measured as candidate/reference/candidate on the Wii with the
same probes (`WII64_SURVEY_SAME_PROBES=1`). Results and hashes:
`baselines/2026-10-02_hw_color_image_*`, `baselines/2026-10-02_hw_memo_dcbz_*`
and `baselines/2026-10-02_cpu-opt-artifacts.json`.

### Kept: Mario Kart's color-image copy (`glN64_GX/YUYVConvert.h`)

`gDPUpdateColorImage` now computes the column positions once per call (the
same float arithmetic as before) and clamps with a table instead of branches.
The output is bit-identical: `tests/yuyv_convert_test.py` compares it with the
original for every Y/U/V value and for whole frames at several scales.

| `subsystem_gaps`, Mario Kart race (3,600 VIs) | Reference | Candidate (both runs) |
|---|---:|---:|
| `SETCIMG` per call | 5.24 ms | 2.55 ms |
| CPU cycles | | −25.7% |
| Speed | 0.978x | 0.993x |
| Limiter sleep | 23.0% | 41.8% |
| Audio underruns | 106 | 19 |

SM64, Snap and DK64 do not run this code and stayed within 1.2% of cycles.
The copy still costs 10.9 s of the race (18% of wall time); the next step is
to let the GP convert and scale (an EFB-to-texture copy), or to convert only
when the game reads the image.

### Rejected

- **`dcbz`/`dcbt` in the copy loop.** Prefetching the XFB and claiming output
  lines made each copy slower: 2.85 ms against 2.55 ms.
- **A TMEM hash memo stamped by the loads.** Loads built their words in a
  scratch buffer and copied only changed 64-byte blocks to TMEM; up to 16
  remembered hashes stayed valid while their blocks were unchanged. It
  matched the original hash in 200,000 random load/lookup steps and drew
  identical frames in Dolphin. On the Wii it cut Snap's hash time by 38%
  (1,420 to 876 ms) with no net cycle change (+0.13%), and cost 0.2–4.2% more
  cycles in the other eight scenes. The extra compare in every load costs
  more than the hashes it saves. This is the third memo design to fail; the
  next attempt must make loads cheaper too, not only lookups.

### Pending Wii timing: paired-single vertex loads (`gSPVertex`)

`GLN64_PS_VERTEX` (default 1) loads x, y, z, s, t and the colors or normals
through the quantized paired-single loads (GQR7, GQR6, GQR2), which convert
in hardware instead of through memory. Unaligned vertex addresses keep the C
path. In Dolphin, SM64, Snap and Banjo drew byte-identical final frames with
identical exception, batch and vertex counts, on and off. The matched Wii
survey is queued in `.dev/runs/subsystem-survey-20261002-013643-Uk48`; collect
it with `bash .dev/profile_subsystems.sh --collect <that directory>`.
