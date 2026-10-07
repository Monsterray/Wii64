# Hardware session: one boot, every measurement

One boot collects every ROM baseline and the stick check. The Wii writes results to SD.
With a Mac or Windows computer on the same LAN, it also sends the results back and
returns to Homebrew Channel. The network path needs a **PERF_PROF** build and a Wii
waiting in Homebrew Channel.

## Network workflow

1. Install HBC-Reborn's workstation queue client once. From its source checkout, run:

   ```bash
   python3 tools/wii-bench/wiibench.py setup --server http://homeserver.local:4310
   ```

   That URL is this development LAN's lease server; use your server's URL elsewhere.
   The installed client is `~/.wii-bench/wiibench.py` on macOS/Linux and
   `C:\tools\wii-bench\wiibench.py` on Windows. Its saved `server` file selects the
   central server. `WII_BENCH_SERVER` can override it. Keep the HBC-Reborn checkout
   available: the installed shim imports that checkout.
2. Once, run `.dev/hardware_setup.sh`. The guided setup records the Wii and computer
   IP addresses in ignored `.dev/hardware.env`. Insert the SD card when prompted.
   It checks the named ROMs, backs up any existing `wii64/diag.cfg`, and writes the
   selected test chain plus `result_host=<computer IPv4>` to the card. Start with
   `smoke` (two ROMs); rerun setup and select `hardware` for the full chain.
3. Eject the SD card, insert it into the Wii, and leave Homebrew Channel open.
   Run `.dev/hardware_run.sh` from the repo root. For Rice, use
   `.dev/hardware_run.sh Rice_wii`. The script queues itself and waits for the
   central lease before it contacts the Wii. Within that lease it builds with
   `PERF_PROF` and sends the DOL with `wiiload`. On macOS it collects files
   through HBC-Reborn after the run; Windows and Linux use a one-run receiver.
   Missing server configuration stops the run; a server outage does not permit
   direct upload. The dispatcher waits for Homebrew Channel between jobs.
4. Wii64 runs the chain without a controller and writes results to SD.
   The runner collects `perf.log`, `xfb_NN.bin` frames, replay traces and saved
   PC histograms. In receiver mode, Wii64 sends the available files itself.
   The script waits for Homebrew Channel to return, then makes PNGs and prints a
   per-game table under `.dev/runs/`. Smoke runs also fail if a game stops rendering.

On a Windows bench workstation the SD card usually stays in the Wii. Skip the
setup wizard and write `.dev/hardware.env` by hand: `WII64_WII_IP=`,
`WII64_MAC_IP=` (this computer's LAN IPv4; the name is historical) and
`WII64_CHAIN=`. The run stages replay files over the network. To see which ROMs
are on the card, queue a listing:

```bash
python /c/tools/wii-bench/wiibench.py add --name "ls roms" --cwd "$PWD" -- \
    env X=1 bash -c 'source .dev/env.sh; python3 ../hbc-reborn/tools/hbc.py --wii "$WII_BENCH_IP" ls sd:/wii64/roms'
```

Start a queued command with `env ...` or an absolute program path, never a
bare `bash`: Windows looks in `System32` first and finds WSL's bash, which has
no Python. The receiver listens on TCP 39364; the Windows firewall must allow
inbound Python on the private network. `python3` is a shell function from
`.dev/env.sh`, so `timeout python3 ...` runs the Store placeholder instead.

For a filed nine-entry baseline, run `.dev/hardware_baseline.sh` instead. It runs the
hardware workflow and files the result in `baselines/` only after every entry reports.
Its status and detailed hardware log are under `.dev/runs/<baseline id>.*`.

Use `python3 ~/.wii-bench/wiibench.py status` to inspect the queue on macOS/Linux;
use the installed Windows path in the devkitPro MSYS2 shell. To enqueue without
waiting, set `WII64_QUEUE_ONLY=1`. The command prints the job ID. To test a frozen
build, also set `WII64_SKIP_BUILD=1 WII64_DOL=/absolute/path/to/build.dol`.
Keep that file unchanged until its job finishes, and retain the matching ELF.
The subsystem driver also freezes the chain config. `WII64_CHAIN_FILE` selects
that frozen file for launch and replay staging. Use an absolute path when
queuing it; keep it unchanged until the job finishes.
The job preserves these settings and `WII64_ROM_DIR` even if the dispatcher was
already running. Only the dispatcher supplies `WII_BENCH_JOB`; normal callers
use the queued entry point.

Queue entry points assign a session ID through `WII_BENCH_AGENT` and pass an
explicit job timeout. A survey shares its ID across its jobs so the dispatcher
can apply HBC-Reborn's same-agent chaining and fairness rules. The receiver
bound includes each ROM's PAL-rate watchdog, startup allowance and transfer
margin. `WII64_RECEIVER_TIMEOUT` and `WII64_JOB_TIMEOUT` override these bounds;
the job must allow at least 180 seconds beyond the receiver for cleanup.
These bounds are safety limits, not estimated run times.

The hardware entry point snapshots its Bash source in the queue command.
Keep sourced helpers unchanged: Bash can read later commands after a
long-running child returns. Editing a live helper can corrupt that job even
when the revised file passes `bash -n`. Change helpers between jobs.

For development-agent builds and crash capture, see
[ROM paging and HBC agent](rom-paging-and-agent.md). Set `WII64_HBC_ROOT` for an
external checkout outside the default sibling path. The runner preserves that
setting, distinguishes HBC from a running agent, and retains new crash reports
with the matching ELF. A crash fails the run instead of becoming a speed result.

With the external HBC-Reborn client available, the runner now copies the selected
chain's `scripts/inputs/*.txt` files to `sd:/wii64/input/` before launching.
This happens under the same lease and outside timed gameplay. Missing local
replays stop the run. To stage them separately, run
`bash .dev/stage_wii_inputs.sh <chain-name>`; it also queues and waits.
Set `WII64_STAGE_INPUTS=0` to retain manually staged files or use an older HBC
without file transfers. Do not upload to the shared Wii outside its lease.

Keep the computer awake. `WII64_RESULT_MODE=auto` selects **pull** on macOS:
the workstation makes outbound connections to HBC-Reborn, without a Python
listener or an incoming-connection firewall prompt. This requires HBC protocol
2 or later and a current `PERF_PROF` build. Each run gets a
unique tag; a stale SD log cannot complete a new job. Transfers use the HBC
client's CRC checks. Missing files, new crash records and incomplete chains fail
the run. Pull mode never falls back to a listener.

`WII64_RESULT_MODE=push` explicitly selects the existing receiver. Use it for
legacy frozen builds or original HBC. It requires an
incoming firewall allowance on TCP 39364 and accepts results only from the
configured Wii IP. Windows and Linux retain push as their automatic mode;
they can select pull when HBC-Reborn is available. Keep the firewall enabled.
If transfer fails or a game hangs outside the watchdog, use the SD results below.
The Wii must be on for the first run. Later LAN runs can start directly from Homebrew
Channel. Use only a trusted LAN: this simple result channel is not encrypted.
If the Wii answers ping but uploads fail, check that Homebrew Channel's lower-right
network icon is lit. Press HOME there to confirm its IP matches `.dev/hardware.env`.
The runner retries the upload port five times, then stops without sending a build.
Its probe sends a complete rejected `PING` header; a bare connect-and-close
can hold an older Homebrew Channel loader thread and make later uploads fail.

For a LAN-only test with new ROMs, leave the files in a folder outside the repository.
For example, from the repo root:

```bash
WII64_ROM_DIR="$(dirname "$PWD")/temp-roms" .dev/hardware_run.sh glN64_wii audio_coverage
```

In pull mode the runner stages only chain-named missing ROMs through HBC's
CRC-checked transfers, before launch. Matching existing files are skipped;
different existing files are preserved and stop the run. It validates local
paths before writing and deduplicates repeated chain entries. This also keeps
the library runner's frozen ROM-folder workflow free of a workstation listener.

In push mode the diagnostic build downloads only missing ROMs named in that chain to
`sd:/wii64/roms/` before testing. It writes each download to a temporary file
and renames it after transfer. Keep Homebrew Channel open and the SD card in the
Wii. Without `WII64_ROM_DIR`, the run uses ROMs already on the card. This
transfer is for local tests on a trusted LAN; do not commit ROMs or run the
receiver on an untrusted network.

If HBC-Reborn 1.2.0 or later is running, its `tools/hbc.py status` shows the
mounted SD/USB device, and `tools/hbc.py put` can copy individual files there
with CRC-checked transfers and no partial destination file on failure. Its
`sync` command copies only changed files. These commands are optional; the
Wii64 runner also works with the original Homebrew Channel. The installed
workstation client and central lease coordinate all projects' Wii tests.

## Direct HDMI capture on macOS

The workstation's UGREEN 15389 dongle is a local video device, not a URL.
The native helper uses AVFoundation. It has no server, Python dependency or
audio input. The default device is exact; it will not select the FaceTime camera.
Xcode Command Line Tools provide the Swift compiler. The wrapper caches its
binary under ignored `.dev/tools/` and rebuilds it when the source changes.

```bash
bash scripts/wii_video_capture.sh list
bash scripts/wii_video_capture.sh authorize
bash scripts/wii_video_capture.sh snapshot .dev/runs/wii-frame.jpg
bash scripts/wii_video_capture.sh capture .dev/runs/wii-video.mov --seconds 10
```

Run `authorize` once from the intended host application and approve macOS Camera
access there. This privacy permission is separate from the firewall. Routine
captures check authorization and fail with instructions rather than request it.
Use `--device "name from list"` for another dongle. Choose a new output path:
existing files are preserved. Movies are limited to 1–300 seconds; missing frames
and unfinished recordings have deadlines. Captures remain local and ignored.
Capture during your queue job, not another project's turn.

## What is on the card

| Path | What |
|---|---|
| `apps/wii64/boot.dol` | a **PERF_PROF** build for manual launch only; `wiiload` sends the DOL without storing it on SD |
| `wii64/diag.cfg` | the default chain for manual launches. LAN runs send the selected diagnostic lines with `wiiload`; these lines replace this file for that run. |
| `wii64/roms/` | the ROMs the selected chain names, under exactly those file names |
| `wii64/input/` | `scripts/inputs/*.txt`, if a chain line has `,input=` (recorded play, `doc/controller-testing.md`) |

No other files are needed. A chain never loads or writes the card's saves (autosave is off for
the whole boot), so a card with real saves on it is safe and the runs are comparable.

## On the Wii

Boot Wii64 from the Homebrew Channel and leave it. No controller is needed: each game runs
for a fixed number of its own VIs, the stick check drives port 1 itself, and the run
returns to Homebrew Channel at the end. The full
`hardware.txt` chain has nine entries and can take more than 10 minutes if a game
needs the watchdog. That chain retains its original Zelda exclusion. Current
Zelda tests use `zelda_boot`, `zelda_menus`, and `zelda_new_game`; see
[Zelda results](zelda-results-2026-10-01.md).

If a game hangs, a watchdog on the host retrace cuts it off at 3x its length plus
60 s and marks it `how=timeout`; the chain goes
on to the next game. If the Wii is still on long after, it hung somewhere the watchdog
cannot reach -- pull the card anyway: every finished game's results are already on it.

## Back on the workstation

Copy the card's `wii64/` folder into a run directory, then:

```bash
python scripts/chain_table.py <run dir>                     # one row per game; xfb_NN.png screenshots
python scripts/padtest.py <run dir>/padtrace_09.csv         # the stick check (game 9 in hardware.txt)
python scripts/baseline_add.py --chain <run dir> --platform hardware --plugin glN64 \
    --id 2026-MM-DD_hw_glN64 --purpose "hardware.txt, first hardware baseline"
python scripts/chain_compare.py <matching Dolphin baseline id> 2026-MM-DD_hw_glN64
```

## What the hardware numbers add

Dolphin answers what the code does; only the Wii answers how long it takes. The columns
that mean something only on hardware:

- **speed / idle** -- `speed` is completed VIs divided by wall time and the ROM's
  nominal VI rate. `idle` uses requested limiter sleep, not measured free CPU
  capacity. A game can stall during ROM paging and still request substantial
  sleep. The optional subsystem probes measure actual limiter sleep. Under
  Dolphin these timings describe emulation on the workstation.
- **ipc** (`pmc2 / pmc1`) -- Broadway's performance counters: instructions completed per
  cycle. Dolphin does not count instructions completed, so this is 0 there. Other events
  can be selected at build time (`-DPMC_MMCR0=...`, see `main/perf_prof.c`).
- **underruns** -- audible audio gaps. Dolphin's DSP timing is not the Wii's.
- **dsp_avg / dsp_peak** -- AESND DSP use, in percent of its latest 2 ms work period. Wii64 samples about twice per second. These are the mean and peak of those samples, not a continuous average or true peak. Compare only Wii runs.
- **flush_us** -- what writing `perf.log` itself cost; it should stay a few ms per game.
  If it is large, the SD card is slow and the probes are disturbing the run.

## Audio mode check

Settings, Audio, Advanced has synthesis, output, mixer, latency, and sync controls.
**Accurate** keeps the original N64 4-tap filter. **Fast** uses a nearest sample.
Optional **Hi-Fi** modes add cubic synthesis, a CPU sinc output converter, and
higher-precision gain arithmetic. **Preserve Pitch** adds CPU time stretching.
See [Audio settings](audio-settings.md) for limits, defaults, and diagnostic keys.
These enhancements do not replace AESND's DSP mixer, which resamples by 16.16 zero-order
hold (WiiStation `Docs/SOUND_SYSTEM.md` section 4.2; an earlier FIR note was wrong): the
Wii DSP output path adds hold images at N64 rates, which the Hi-Fi sinc converter avoids.

The Wii audio hardware path is already active: Wii64 streams one stereo PCM voice;
AESND handles output rate, pitch, mute, mixing, and the Wii DSP transfer. libogc2
also aligns and flushes its DSP input buffer. N64 RSP ADPCM decoding, envelopes,
and the 4-tap filter remain on the CPU. AESND accepts PCM voices, not N64 audio
commands; moving those commands to the DSP requires a separate DSP program and
accuracy tests, not another AESND setting.

For a matched Wii check, run:

```bash
.dev/build_profiling.sh glN64_wii
WII64_SKIP_BUILD=1 .dev/hardware_run.sh glN64_wii audio_accurate
WII64_SKIP_BUILD=1 .dev/hardware_run.sh glN64_wii audio_fast
# All enhancements versus explicit legacy defaults, alist and MusyX:
WII64_SKIP_BUILD=1 .dev/hardware_run.sh glN64_wii audio_reference
WII64_SKIP_BUILD=1 .dev/hardware_run.sh glN64_wii audio_hifi
```

The chains use the Super Mario 64 title screen for 3,600 VIs. Keep Homebrew
Channel open between runs. For Dolphin audio checks, set
`WII64_DOLPHIN_DSP_HLE=False` when running `.dev/dolphin_test.sh`; DSP HLE does not
recognize this AESND ucode. Compare only runs from the same platform and scene.

On 2026-09-26, one matched Wii pair had 23.77 billion CPU cycles in Accurate and
23.30 billion in Fast (2.0% less), with 0 overruns in both. DSP use was 1.54% and
1.48% on average. These are sampled title-screen results, not a claim about game
play or audible quality. See `baselines/2026-09-26_hw_audio_accurate/` and
`baselines/2026-09-26_hw_audio_fast/` for the raw logs.

The `audio_smash_accurate` and `audio_smash_fast` chains repeat the check with
Super Smash Bros. Both 2026-09-26 Wii runs completed with one underrun and zero
overruns. An earlier nine-entry repeat had 3,204 Smash Bros. overruns; this
single-game title-screen check did not reproduce that result. Its cause remains open.

## Running the same chain in Dolphin

```bash
.dev/build_profiling.sh glN64_wii && cp wii64-glN64.dol .dev/wii64-glN64-prof.dol
.dev/wii64_diag.sh chain .dev/wii64-glN64-prof.dol scripts/chains/hardware.txt .dev/runs/hw_dolphin
```

Chains run in their own Dolphin profile (`.dev/dolphin_runs`), so they do not collide with
an interactive session. Clean between plugins because targets share objects.
