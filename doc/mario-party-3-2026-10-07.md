# Mario Party 3: speed and intro flashing, 2026-10-07

Report: on the Wii, Mario Party 3 (USA) "runs too fast" (gameplay and music),
and the intro flashes a lot before the big white rainbow star.

## Speed: the guest clock is correct

- **VI rate.** The USA version runs 60 VIs/s and the Europe version 50 VIs/s,
  at 99.3–99.5% speed on the Wii (library runs). The VI period is
  `vi_clock / refresh` count cycles, as in mupen64plus.
- **Settings.** The Wii has no `settings.cfg` on SD or USB, so interactive
  runs use the defaults, the same as the library chains (`CountPerOp` 2,
  `LimitVIs` wait for VI, Accurate audio).
- **Audio production.** The game makes 97.9% of its DAC rate per guest second
  (USA, 30 s scene). A game whose music runs fast makes more.
- **Music tempo.** A Dolphin DSP dump of the USA attract sequence (no input,
  9,000 VIs) plays the opening and title songs at exactly 1.000x the tempo of
  the soundtrack rips (`mariopartylegacyosts`, "Opening" and "Title Screen";
  `C:\tools\reference\mario_party_3\`). `scripts/audio_tempo.py` measures
  this: it correlates onset envelopes over stretch factors 0.6–1.6
  (`tests/audio_tempo_test.py`; it also read 1.200 for the rip sped up 20%).
- **Scene timing.** On the Wii and in Dolphin the title screen comes about
  130 s of guest time after the Nintendo logo.
- **CPU budget.** With `CountPerOp` 1, 2 and 3 the game draws the same 48.7
  frames per 50 VIs (Europe): it is not CPU-bound.

No measurement shows a fast guest clock. The visible fault was the flashing
below: it showed parts of the next frame before the current one.

## Flashing: half-drawn frames on the TV

A trace of each VI in Dolphin (guest VIs 4,800–6,300 of the intro) shows that
Mario Party 3 runs at 30 fps and draws each frame as four RSP tasks into one
framebuffer, then swaps `VI_ORIGIN` to it:

```
vi 5800 origin 00385a80  4 lists into 00360000   whole frame between two VIs
vi 5801 origin 00360280  0 lists                 the game shows it
vi 5809 origin 00360280  1 list  into 00385800   next frame split across a VI
vi 5810 origin 00360280  3 lists into 00385800
```

In the busy scenes the game starts the next frame before the VI, so 1 of 4
tasks is often done at a VI (492 of 1,500 VIs). Without framebuffer
textures, glN64 copied the EFB to the TV after any finished display list. A
VI between tasks then showed a part of the next frame: black bands, the cast
without the background, white frames. On an N64 the TV shows the RDRAM
buffer at `VI_ORIGIN`, which the game swaps only after the whole frame.

### Change (`glN64_GX/VI.cpp`, default path, `FBTex` off)

- At a VI, the EFB goes to the TV only when `VI_ORIGIN` is inside the buffer
  the RDP draws into. This starts after the game is seen to show the buffer
  it draws into. Games where that never happens, or 30 VIs without a picture,
  keep the previous behavior (a picture after every finished list).
- `gDPSetColorImage` calls `VI_ColorImageSwitch`: when the game starts to
  draw into another full-screen framebuffer and a list finished since the
  last picture, the finished frame goes to the TV then. The depth image and
  narrower buffers do not count, because games draw into them inside a frame
  (Mario Party 3: B, depth, A; Mario Kart: X, depth, X).
- `RomOpen` resets this (`VI_ResetPresent`).

Trace with the change (same 1,500 VIs): 718 of 721 pictures are whole frames
shown when the next frame starts, one every 2 VIs; 3 come from the 30-VI
fallback at scene changes. Wii screenshots of the intro
(`hbc.py screen` every 1.5 s during a queued run) no longer show half-drawn
frames; the white frames that remain are the intro's own flashes and fades.
Library check on the frozen `PERF_PROF` build: the 9 Dolphin ROMs and the 18
Wii entries (`baselines/library-20261007-present-hardware-0{1,2,3}`) reached
their VI targets with the same replays, exception and recompile counts,
speed and DL/s as before, and their final frames show the expected scenes.

## Production Wii (192.168.8.200), 2026-10-08

The report came from the user's own Wii, not the bench Wii. Its
`sd:/wii64/settings.cfg` has `LimitVIs = 0` (no speed limit) and `FBTex = 1`
(framebuffer textures, so the change above does not apply there).

| Run (MP3 USA, no input, 9,000 VIs) | Wall time | VI/s |
|---|---:|---:|
| The user's settings | 75.4 s | 119 (1.4x to 4.7x on the overlay) |
| The same, `LimitVIs=1` | 151.2 s | 59.7 |

Without a limit the game runs as fast as the Wii can, and `AudioSync = 2`
(preserve pitch) plays the music fast at its normal pitch. The captures of
the unlimited run show torn and smeared frames before the rainbow star; with
the limit the framebuffer-texture path shows clean frames. Both reports have
this one cause.

How the value got there: from 2009 to January 2014 Wii64's default was
`Timers.limitVIs = 0` ("sync to audio": the old AI driver held the speed).
The AESND driver (`ca438f3`) changed the default to wait for VI. Settings >
Save writes every value, so a `settings.cfg` first saved by an older Wii64
kept `LimitVIs = 0` in every later version. The menu option for it had been
commented out since 2009. Not64's own settings on the same card have
`LimitVIs = 1`, and the Wii64 `meta.xml` passes no arguments.

Done: with the user's permission, `LimitVIs = 1` on the production card (the
only change; backup of `sd:/wii64` and `sd:/apps/wii64` with SHA-256 sums in
`C:/backups/wii-production-20261008/`). The test runs used `AutoSave=0`;
all 89 saves and `settings.cfg` matched the backup afterwards, and the test
files were removed and `perf.log` restored. Settings > Audio now has a Speed
Limit row (Off / VI / Frame) for `LimitVIs`.

### Other findings on that card (2026-10-08)

- **Saves Wii64 cannot use.** `sd:/wii64/saves` (and `sd:/not64/saves`) holds
  about 80 Project64 saves and `.pj.zip` save states. They are named
  `GOODNAME.ext`; Wii64 looks for `GOODNAME(REGION).ext`, and it reads a save
  only at full size (EEPROM 2,048, SRAM 32,768, FlashRAM 131,072, Controller
  Pak 131,072 bytes). Many are shorter (512-byte EEPROMs, `MarioParty3.eep`
  1,384, `Kirby64.eep` 280, `.fla` 114,816–127,744). So these games start
  with no save. Possible fix: load `GOODNAME.ext` when `GOODNAME(REGION).ext`
  is missing, accept short files, and write only the region-named file.
- **Two Mario Party 3 saves:** `MarioParty3(U).eep` (Wii64, 2,048 bytes) and
  `MarioParty3.eep` (Project64, 1,384 bytes, not loaded).
- **ROM folder.** The N64 library is in `sd:/ROMS/N64` (23 ROMs);
  `sd:/wii64/roms`, where the browser opens, has only the three Mario Party
  ROMs. Not64 has a `rompath` setting; Wii64 has none.
- **Experimental audio settings.** `AudioQuality = 2`, `AudioMixerPrecision =
  1` and `AudioSync = 2` are the Hi-Fi/Preserve Pitch modes (about 22–25% more
  CPU, see `doc/optimization-handoff.md`); with the limit on, MP3 had 7 audio
  underruns in 150 s.
- **Not faults:** `Killer Instinct Gold.eep` is 0 bytes (from Project64); the
  `perf.log` samples of about 1 menu frame/s are the 38 s ROM load screen.
