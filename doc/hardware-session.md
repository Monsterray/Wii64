# Hardware session: one boot, every measurement

One boot collects every ROM baseline and the stick check. The Wii writes results to SD.
With a Mac or Windows computer on the same LAN, it also sends the results back and
returns to Homebrew Channel. The network path needs a **PERF_PROF** build and a Wii
waiting in Homebrew Channel.

## Network workflow

1. Once, run `.dev/hardware_setup.sh`. The guided setup records the Wii and computer
   IP addresses in ignored `.dev/hardware.env`. Insert the SD card when prompted.
   It checks the named ROMs, backs up any existing `wii64/diag.cfg`, and writes the
   selected test chain plus `result_host=<computer IPv4>` to the card. Start with
   `smoke` (two ROMs); rerun setup and select `hardware` for the full chain.
2. Eject the SD card, insert it into the Wii, and leave Homebrew Channel open.
   Run `.dev/hardware_run.sh` from the repo root. For Rice, use
   `.dev/hardware_run.sh Rice_wii`. The script builds with `PERF_PROF`, starts a
   one-run result receiver, then sends the DOL with devkitPro's `wiiload`.
3. Wii64 runs the chain without a controller. At the end, it sends `perf.log`,
   available `xfb_NN.bin` frames, and `padtrace_NN.csv` files to the computer.
   The script waits for Homebrew Channel to return, then makes PNGs and prints a
   per-game table under `.dev/runs/`. Smoke runs also fail if a game stops rendering.

For a filed nine-entry baseline, run `.dev/hardware_baseline.sh` instead. It runs the
hardware workflow and files the result in `baselines/` only after every entry reports.
Its status and detailed hardware log are under `.dev/runs/<baseline id>.*`.

Keep the computer awake. On macOS, the receiver uses Apple's built-in Python so the
firewall can allow it without a repeated Homebrew Python prompt. If the firewall does
prompt, allow incoming connections for the test; keep the firewall enabled. The
receiver accepts files only from the configured Wii IP.
If transfer fails or a game hangs outside the watchdog, use the SD results below.
The Wii must be on for the first run. Later LAN runs can start directly from Homebrew
Channel. Use only a trusted LAN: this simple result channel is not encrypted.

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
needs the watchdog. The Zelda games are excluded because they currently fail.

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

- **speed / idle** -- `idle` is the share of wall time the frame limiter slept: the
  headroom. `speed` under 1.00 with `idle` at 0% means that game does not run full speed
  on a Wii. Under Dolphin these describe the workstation.
- **ipc** (`pmc2 / pmc1`) -- Broadway's performance counters: instructions completed per
  cycle. Dolphin does not count instructions completed, so this is 0 there. Other events
  can be selected at build time (`-DPMC_MMCR0=...`, see `main/perf_prof.c`).
- **underruns** -- audible audio gaps. Dolphin's DSP timing is not the Wii's.
- **flush_us** -- what writing `perf.log` itself cost; it should stay a few ms per game.
  If it is large, the SD card is slow and the probes are disturbing the run.

## Running the same chain in Dolphin

```bash
.dev/build_profiling.sh glN64_wii && cp wii64-glN64.dol .dev/wii64-glN64-prof.dol
.dev/wii64_diag.sh chain .dev/wii64-glN64-prof.dol scripts/chains/hardware.txt .dev/runs/hw_dolphin
```

Chains run in their own Dolphin profile (`.dev/dolphin_runs`), so they do not collide with
an interactive session. Clean between plugins because targets share objects.
