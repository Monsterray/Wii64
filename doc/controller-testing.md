# Controller sticks: the mapping and how it is proven

## The mapping

Every controller driver turns its stick into per-axis fractions of full cardinal travel
and hands them to one function, `n64_stick()` in `gc_input/n64_analog.h`. That maps the
source stick's gate onto the N64 stick's gate:

- **The N64 gate** is an octagon, 82 at the cardinals and 98.4 at the diagonal corners
  ((69.6, 69.6) per axis), with straight edges between -- the measured OEM shape from
  N64ModernRuntime's `convert_to_n64_range()` (r0 = 82, alpha = 1.39414574), which the N64
  recompilation projects use.
- **The source gate** is a regular octagon with its corners at full cardinal travel:
  every Wii and GameCube stick, as Dolphin models them (`OctagonAnalogStick`).
- A position is scaled along its own direction by (N64 gate radius / source gate radius)
  there. The rim of the source stick lands on the rim of the N64 stick in every
  direction; anything inside moves linearly; rest is exactly (0, 0). Nothing is dropped
  and there is no dead zone except on the Wii U GamePad, whose sticks do not rest on
  centre (a 7.8% *radial* dead zone, so small movements are not bent onto an axis).

Full cardinal travel per driver:

| Driver | Raw full travel | Source |
|---|---|---|
| GameCube main stick | 0.7937 x 127 = 100.8 | Dolphin `GCPadEmu.h` MAIN_STICK_GATE_RADIUS |
| GameCube C-stick | 0.7221 x 127 = 91.7 | Dolphin `GCPadEmu.h` C_STICK_GATE_RADIUS |
| Classic Controller, Nunchuk | the controller's own calibration (wiiuse `joystick_t` min/centre/max, each side scaled on its own) | Dolphin builds that calibration as centre +/- gate radius |
| Wii U GamePad | 75 | libwiidrc's range, as WiiStation uses it |

### What it replaced (defects found)

- **Classic Controller**: magnitude divided by 0.667 and clamped, plus a 10% per-axis
  dead zone. Full deflection arrived at two thirds of the throw, the last third did
  nothing, and small movements snapped onto the axes.
- **Classic, Nunchuk**: wiiuse's angle/magnitude clamped to a circle, then converted back.
- **GameCube**: a fixed 5/6 (a real stick's ~101 read ~84, the C-stick ~76), no gate shape.
- **Wii U GamePad**: two stacked 1.08 gains around a per-axis dead zone, clamped at 80.
- **D-pad as stick** (every driver): 80 on both axes on a diagonal, radius 113 -- outside
  the N64 gate. Now 82 cardinal, (70, 70) diagonal.

## Proof, layer by layer

### 1. The maths, every input -- host test

```bash
clang -O2 -o .dev/n64_analog_test.exe tests/n64_analog_test.c && .dev/n64_analog_test.exe
```

`n64_analog.h` has no libogc in it, and the drivers call its `gc_stick`, `cal_stick`,
`drc_stick` and `button_stick` and nothing else, so this is the code the Wii runs. For every
raw position each driver can produce it asserts: rest is (0, 0); full cardinal travel is
exactly 82 and a corner about (70, 70); each axis never goes backwards and no run of raw
steps does nothing longer than the source's resolution forces; every output is inside the
N64 gate and keeps its input's sign; a d-pad reaches 82 and (70, 70).

### 2. The live path -- in-emulator sweep

`diag.cfg` `padsweep=<vi>[,<hold>]` makes the GameCube driver on port 1 read a generated
sweep instead of the pad, from guest VI `<vi>` on: each stick axis through every raw value,
the main stick's rim (a degree per step, along the octagonal gate), then every button
alone, `<hold>` VIs a step, repeating. It stands in at the one point the raw `PAD_*`
reading is taken, so the driver's conversion, the controller config, the plugin and the
PIF path all run on it as on a real pad -- and unlike an input movie it works on a Wii.
A PERF_PROF build logs what the game read (the 4 bytes `pif.c` puts in the reply) next to
the raw reading, to `padtrace_NN.csv`.

```bash
.dev/wii64_diag.sh chain .dev/wii64-glN64-prof.dol scripts/chains/padtest.txt .dev/runs/pad
python scripts/padtest.py .dev/runs/pad/padtrace_01.csv
```

`padtest.py` checks, from what the game received: each main-stick axis hits all 165 values
from -82 to 82, in order, with the other axis at 0; the C-stick fires only the C buttons,
each past exactly +/-48 raw; the rim traces the N64 gate; each button sets one N64 bit of
its own. Use 4 VIs a step: a game running below 30 fps polls less often than a 2-VI step
moves (SM64 at 27 fps saw 240 of 256 raw values at 2).

First run (Super Mario 64, glN64, Dolphin): all 256 raw values on every axis, all 165 N64
values per main-stick axis, rim within 0.989..1.005 of the gate, 82 cardinal, (69, 69)
best diagonal (the raw grid's nearest step to 45 degrees), 10 buttons one bit each (GC X
and Y are unmapped by default).

### 3. Real controllers -- hardware only

What neither layer above can see: how much travel a particular physical stick really has
(a worn GameCube stick that tops out at 90 reads 73 at its edge, not 82), Bluetooth
latency, and whether a real Classic Controller's calibration block matches its gate the
way Dolphin's does. Check on a Wii with the controller in hand: push to each notch and
read the value back with a game that shows stick input, or run the stick check with the
real pad (no `padsweep=`) and move it by hand while `padtrace` records.
