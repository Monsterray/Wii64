# ROM library testing

Use owned, uncompressed ROM files. Do not commit ROMs, saves, WAV files or
captures. Test results stay under the ignored `.dev/runs/` directory.
See [the 2026-10-01 results](library-results-2026-10-01.md) for the first complete
18-file sweep and probe controls.

Build an instrumented glN64 or Rice DOL with its matching ELF, then run:

```bash
bash .dev/build_profiling.sh glN64_wii 'DEBUG_FLAGS=-DPERF_PROF -DPERF_SUBSYSTEM_PROBES' HBC_AGENT=1
bash .dev/library_check.sh path/to/instrumented.dol both
```

Use `dolphin` or `hardware` instead of `both` for one platform. Set
`WII64_ROM_DIR` to change the ROM source folder. The default is the isolated
Dolphin profile's `Load/WiiSDSync/wii64/roms/` folder.

The script freezes the ROMs and build, records their SHA-256 hashes and queues
Wii batches through the central lease server. It runs separate Dolphin boots
in parallel with the Wii queue. Each batch returns to Homebrew Channel. Never
send a build directly while another workstation holds the lease.

Most entries run for 1,800 VIs at their title or intro scene. Mario Party
entries receive periodic A presses. Super Mario 64, Mario Kart 64 and Zelda
use longer input replays. This is a smoke test, not complete gameplay coverage.

## Read the results

`report.txt` and `report.json` contain VI speed, graphics/audio task activity,
slow VI windows, audio gaps, paging errors and capture status. `coverage.json`
lists missing, duplicate, unexpected and incomplete ROM entries per platform.
A launcher failure returns a nonzero script exit but does not discard earlier
results. Its exit codes are retained in the session and coverage report.
A successful launcher exit alone does not establish compatibility.
Run `python3 scripts/library_report.py RESULT_DIRECTORY --check` for a strict
nonzero exit when coverage is incomplete or any measured warning is present.

- Speed uses VI count, nominal VI rate and gameplay wall time. It is not the
  display-list rate or the game's intended rendered frame rate. Every measured
  result below 100% is flagged, including small shortfalls.
- Slow windows are samples below 95% of nominal VI rate after four initial
  samples. They identify stutter candidates, not individual frame-time spikes.
- Audio underruns can indicate gaps. Dolphin DSP WAV statistics detect silence,
  not correct music, pitch or absence of clicks. Host playback stays muted.
  The Wii's physical audio output is not recorded.
  The test launcher defaults to DSP LLE for AESND homebrew microcode.
- Active graphics tasks do not prove correct pixels. Inspect Wii captures.
  Dolphin's texture-only XFB copies can leave its RAM capture invalid for visual
  review; the report marks these captures unverified.
  For a separate visual check, set `WII64_DOLPHIN_XFB_TEXTURE=False` before
  running `.dev/dolphin_test.sh`. This enables RAM copies and records the setting
  in `dolphin-settings.json`. Keep it identical in both sides of a timing comparison.
  Dolphin defines this switch as
  [`Graphics.Hacks.XFBToTextureEnable`](https://github.com/dolphin-emu/dolphin/blob/master/Source/Core/Core/Config/GraphicsSettings.cpp).
- Full probes, cold compilation, ROM paging and log writes affect timing.
  Repeat suspect scenes with a `PERF_PROF`-only control build before assigning
  their cost to an engine subsystem. Missing probe counters mean unknown cost,
  not an inactive pipeline.

Wii startup can remain black while it fetches missing ROMs for the next batch.
Inspect `receiver.log` for transfers and the agent/crash status before calling
this a game hang. Do not reset a Wii that is still making transfer progress.

Normal ROM loading shows a percentage for MEM2 copy and pagefile preparation.
Preparation updates after completed writes, without drawing under the VM mutex.
The former `Loading ...` popup shows the three-second controller wait instead;
it is not an I/O progress estimate. These updates occur before gameplay.
