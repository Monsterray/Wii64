# Subsystem review

Read-only survey, one subsystem at a time (safest/simplest first, CPU
core/dynarec last). A follow-up pass then applied the standout findings --
those are marked **APPLIED** inline below. (Section order got scrambled by
editing -- section 1, Build system, ended up at the bottom of this file;
content is unaffected, just cosmetic.)

## Investigated: "froze Wii64 by clicking New ROM and backing out multiple times"

Added a reusable diag.cfg stress-test hook (`stress_selectrom=N` in
`sd:/wii64/diag.cfg`, see `main/main_gc-menu2.cpp`'s diag.cfg doc comment)
that repeats the exact click path ("New ROM" -> SD -> back out, matching
`MiniMenuFrame.cpp`'s `Func_MMSelectROM` wiring) N times, unattended, right
after boot. Ran it 30x then 150x via `.dev/wii64_soak.sh` -- both times the
process was left idle and responsive at the main menu afterward (confirmed
via a live screenshot, not just CPU%: the spinning logo had visibly rotated
between checks, i.e. genuinely still rendering, not just "quiet"). **Could
not reproduce the freeze this way.** Read through `SelectRomFrame.cpp`'s
activate/return pair, the boxart texture heap alloc/free, and the directory
scan/free logic -- all correctly paired (free-then-realloc on every entry,
`__lwp_heap_free` matched by `__lwp_heap_allocate`, no accumulating state
found).

Likely needs something this synthetic test doesn't replicate to reproduce --
real input timing (backing out while boxart is still mid-load, which the
synthetic loop's synchronous calls can't race), or a longer/different click
sequence (paging through listings, hovering files, actually starting a load
before backing out). The `stress_selectrom` hook is still there and cheap to
extend (e.g. add page-navigation or a mid-load abort) if this recurs.

Also noted in passing: closing an *entirely idle* Wii64 instance (no ROM
ever loaded) still needed a force-kill after the usual ~60s graceful-close
timeout during this investigation -- so the "Dolphin can't always fully shut
down Wii64" issue flagged earlier this session is not exclusively tied to
the dynarec hang/watchdog path; it can happen from menu-only state too.

## Investigated: the graceful-close hang (idle instance, no ROM)

Found and fixed a real, independent bug along the way: `Gui::draw()`
(`libgui/Gui.cpp`) issues the actual shutdown call (`SYS_ResetSystem`/
`exit()`) once `fade` reaches 255, but nothing ever clears `shutdown` or
resets `fade`, and `MenuContext::isRunning()` (`menu/MenuContext.cpp`)
unconditionally returns `true` -- so the `fade==255` branch re-entered and
re-issued the shutdown syscall on *every subsequent frame, forever*, instead
of exactly once. **APPLIED**: a one-shot guard. Real defect regardless of
this specific hang, since it meant a shutdown request got retried
indefinitely rather than once.

That fix alone did not resolve the hang, nor did swapping
`SYS_ResetSystem(SYS_POWEROFF,...)` for `exit()` (tried and reverted).
Instrumented `ShutdownWii()` (`main/main_gc-menu2.cpp`, the
`WPAD_SetPowerButtonCallback`/`SYS_SetPowerCallback` target) with a
`perfProf_mark` and confirmed via `sd:/wii64/perf.log` that **it never
fires at all** -- even minutes after Dolphin's window receives the close
request. The window's rendered surface goes blank white immediately
(Dolphin tearing down its own renderer), confirming Dolphin *does* act on
the close request, but the guest never receives the STM power-button event
that Wii64's shutdown path depends on. That puts the hang on Dolphin's side,
before any Wii64 guest code runs -- **not fixable from this codebase**.
Ruled out along the way: a "confirm stop?" dialog blocking headlessly (no
such window appears, per `EnumWindows`), and a slow `WiiSDCardEnableFolderSync`
write-back of the ~120MB synthetic SD card (waited 5+ minutes past the
normal timeout with zero progress -- not just slow, genuinely stuck).

Left the `ShutdownWii()` perfProf_mark in place since it's the checkpoint
that proved this, and added `autonav=settings_general|settings_video` to
diag.cfg (jump straight to a Settings tab at boot) as reusable tooling from
this pass. The `.dev/wii64_diag.sh`/`wii64_soak.sh` force-kill fallback
remains the practical workaround.

**Correction, found during the next investigation below**: `Makefile.base`'s
`-DPERF_PROF` is commented out of `DEBUG_FLAGS` (the same discrepancy
Section 1 already flagged), so every build this whole pass -- including this
one -- compiled every `perfProf_mark()` call to a no-op, and separately,
`sd:/wii64/perf.log` turned out to only sync back to the host on a *clean*
unmount, which this project's builds never reach. So "confirmed via
perf.log that ShutdownWii() never fires" above was **not actually verified**
-- the instrumentation was silently inert the whole time, both for the
reason it happened to be looked at (the mark itself) and for the reading
method (the log file). The hang itself is still real and reproducible; only
the specific "STM event never reaches the guest" root cause is now
unconfirmed. Re-investigating this properly needs `make DEBUG_FLAGS=-DPERF_PROF`
(a `clean` first -- no header-dependency tracking means a plain rebuild
won't pick up a flag-only change) and reading perf.log's *content*, not just
mtime, immediately after each check, since even then the host-visible copy
may lag behind what the guest actually wrote.

## Investigated: froze at "Searching for ROMs" after clicking New ROM

Reproduced live (not synthetically) -- a real instance the user was clicking
through got stuck on the "Searching for ROMs" message with `Dolphin.exe`'s
working set climbing past 4.8GB and still rising, CPU time still
accumulating: an active runaway, not a deadlock. `sd:/wii64/roms` in the
repro environment was flat (5 files, no subdirectories), so the earlier
`stress_selectrom` synthetic-click tool -- which only exercises that same
flat listing -- couldn't reproduce it (confirmed again this pass, both
before and after the fix, 1 rep). This is exactly the same
"needs something the synthetic test doesn't replicate" gap flagged in the
first freeze investigation above.

Root cause, found by reading `fileBrowser_libfat_readDir()`
(`fileBrowser/fileBrowser-libfat.c`) rather than by reproducing it directly:
```c
sprintf(direntry->name, "%s/%s", file->name, entry->d_name);
```
`direntry->name` is a fixed `char[192]` (`FILE_BROWSER_MAX_PATH_LEN`,
`fileBrowser.h:30`). `file->name` is the *parent* directory's full path --
for a recursive scan (`selectRomFrame_OpenDirectory` calls this with
`recursive=1`), each nesting level's path is the previous level's path
built the same unchecked way, so it keeps growing with depth. A real ROM
collection organized into a few nested category folders with reasonably
long names comfortably exceeds 192 bytes, and this `sprintf` has no bounds
check at all -- it overflows `direntry->name` into the rest of that
heap-allocated `fileBrowser_file` struct and whatever the allocator placed
next to it. Heap corruption from there plausibly explains both symptoms:
CPU burn (a corrupted free list/allocator loop) and unbounded memory growth
(a corrupted size request to a subsequent `malloc`/`realloc`).

**APPLIED**: `snprintf` with the actual buffer size, skipping (`continue`)
any entry whose combined path doesn't fit instead of overflowing into it.
Verified by recreating the failure condition on purpose -- a 3-level-deep
directory with ~70-character names (244-byte full path) under
`sd:/wii64/roms` -- and confirming the automated "New ROM -> SD" click
completes normally (memory stayed flat, reached the main menu) instead of
hanging, both with the offending entry present and skipped.

Not fixed, same-shaped risk noted for a future pass: `fileBrowser-DVD.c:117`
(`strcpy(direntry->name, DVDToc[i].name)`) has the same missing bounds
check, just fed from the DVD TOC instead of a recursive scan -- lower risk
since it's not compounding across recursion depth, but worth the same
`snprintf`-and-skip treatment if it's ever hit.

## Investigated and fixed: New ROM menu slowdown

Used the new profiling tooling end to end for this one: `.dev/build_profiling.sh`
to get real numbers, `.dev/wii64_diag.sh perf` to pull them off a killed
instance's raw SD image, and the ROM browser's existing `pageBegin`/`tileLoaded`/
`pageEnd` instrumentation (boxart page fill) plus two new spans added for this
investigation (`perfProf_dirScan` around `romFile_readDir`/the header-read loop
in `menu/SelectRomFrame.cpp`'s `selectRomFrame_OpenDirectory`).

First measurement, `stress_selectrom=10` against the same flat 5-ROM SD folder
used elsewhere this session: totally flat across all 10 repeats (~1ms readDir,
~1.7ms headers, ~11ms boxart page fill, no degradation with reuse) -- nothing
here matches "slow down" at that scale, so the bug had to be one that only
shows up with a larger, more realistic ROM collection. Copied 150 dummy `.z64`
files into the SD roms folder (155 entries total) and re-measured: `readDirUs`
jumped from ~900us to **135,249us** and `headersUs` from ~1,700us to
**207,503us** -- a 31x entry increase producing a 150x and 122x time increase
respectively, both clearly super-linear. That's ~370ms just to open the ROM
browser, genuinely perceptible, and it gets worse for anyone with a real,
larger romset.

**Root cause 1 (fixed): `fileBrowser_libfat_readDir()`'s per-entry `realloc`.**
`fileBrowser-libfat.c` grew `*dir` by exactly one `fileBrowser_file` at a time
for every matched entry -- O(n^2) copying for a large folder, the classic
dynamic-array-without-amortized-growth anti-pattern. **APPLIED**: doubling
growth (start at 64, double on demand) in both `fileBrowser-libfat.c` and the
identical pattern in `fileBrowser-DVD.c`.

**Root cause 2 (fixed, the dominant one): a redundant `stat()` per entry.**
Isolating it (temporarily zeroing `direntry->size` instead of calling `stat()`)
dropped `readDirUs` from ~135ms to ~3ms for 155 entries -- `stat()` was
~97% of readDir's cost, not the realloc pattern (that fix alone barely moved
the number; it matters more as n grows further, since `stat()`'s cost per
call looks roughly constant here rather than itself growing with n -- the
pinned toolchain's `struct dirent` (`devkitPPC-r41-2/.../sys/dirent.h`) has no
`d_stat`/size field to read the size for free the way a newer newlib does, so
this is a genuine second path-based directory lookup per file). **APPLIED**:
stopped calling `stat()` during the listing entirely (`direntry->size = 0`).
Verified this is safe, not just fast: the only consumer that needs a real
size is the ROM actually being loaded, and `main/rom_gc.c`'s `rom_read()`
already re-derives it correctly via a pre-existing "dummy read" through
`fileBrowser_libfatROM_readFile()`, which does its own single `stat()` for
that one file, before `ROM-Cache.c` ever reads `.size`. Confirmed nothing else
in the codebase reads `.size` off a listing entry (grepped for it) --
`FileBrowserFrame.cpp`'s save-file browser doesn't touch it either.

Added `test_selectload=1` to diag.cfg (click the first entry in the sorted SD
listing, not just navigate to it like `stress_selectrom` does) specifically to
validate this safely -- watched `loadROM`'s marks run all the way through
`ROMCache_load`/`init_memory`/`cpu_init` against both the 155-entry and the
real 5-ROM folder, confirming the ROM still loads correctly with the
now-deferred size.

Net result for the 155-entry case: `readDirUs` 135,249 -> ~3,000 (45x), total
ROM-browser open time ~370ms -> ~240ms. `headersUs` (~207ms) is **not** fixed
-- it's `selectRomFrame_OpenDirectory`'s separate per-entry
`romFile_readHeader()` loop (peeking each ROM's header via a fresh
`fopen()`/`fseek()`/`fread()`/`fclose()`), and unlike the size, the header
bytes genuinely have to be read from each of the N files to sort/display them,
so this can't be skipped the way the redundant `stat()` could. A real fix
would mean reading header bytes during the same `readdir()` pass that already
visits every file (avoiding N separate path-based `fopen()`s), which touches
how the DVD driver populates the same fields too -- a bigger, riskier
restructure than this pass's two fixes, left as follow-up work.

Also found and fixed in passing: `main/main_gc-menu2.cpp`'s `diag.cfg` parser
had the exact same off-by-one `strncmp` length bug twice --
`"test_saveload=1"` was compared with length 16 instead of 15, so it had
*never actually matched*, meaning the earlier session pass that reported
"SD/USB round-tripped cleanly" via `test_saveload=1` never actually ran
`Func_SaveGame()`/`Func_LoadSave()` at all; it only confirmed that *not*
running them didn't crash anything. Fixed alongside the same bug just
introduced in this pass's own new `"test_selectload=1"` (18 instead of 17).
The save/load path itself needs re-testing now that the flag actually fires.

## 2. fileBrowser + vm

**Cleanup**:
- `fileBrowser/fileBrowser-libfat.c:192` -- `static int mounted[5]` only ever uses indices 0-2 (Wii SD/USB use 0/1, everything else collapses to 2); indices 3-4 are unused. **APPLIED** (right-sized to `mounted[3]`).
- `fileBrowser/gc_dvd.h:30` -- `MAXIMUM_ENTRIES_PER_DIR` is defined but never referenced; `read_directory()` (`gc_dvd.c:621-643`) grows its `realloc`'d entry table with no cap despite this constant apparently having been intended as one. **APPLIED** (per explicit request: fixed by actually using it as `read_directory()`'s loop bound, rather than deleting the unused constant).
- `vm/vm.h:50` -- `VM_FILENAME` is defined but commented out (dead). **APPLIED**, with a correction: it isn't actually superseded by `wii_vm.c:35`'s copy as first thought -- `vm.h`/`vm.c` is the separate GameCube VM implementation (`Makefile.gc` only), and `vm.c`'s `VM_Init` never references a pagefile path at all (its paging is pure `memalign`, no ISFS-equivalent backing store on GC). So there's nothing to wire the macro up to; removed the dead line rather than force a cross-platform coupling that doesn't apply.

**Fixes** (all applied):
- `fileBrowser/gc_dvd.c:213-214` -- `DVD_LowRead64` missing braces: `if ((((int)dst) & 0xC0000000) == 0x80000000) // cached?` only guards the next line (`dvd[0] = 0x2E;`); every subsequent register write (`dvd[1]`...`dvd[7]`, `DCInvalidateRange`) runs unconditionally. For a `dst` pointer in the uncached MEM1/MEM2 mirrors (`0xC0000000+`), `dvd[0]` never gets the read opcode `0x2E` set and keeps whatever the previous DI command left there -- a real bug for the uncached case (muted in practice since most call sites pass ordinary cached pointers). **APPLIED**: made the assignment unconditional, matching every other register write in the function.
- `fileBrowser/fileBrowser-DVD.c:156` -- `if(strlen((*dir)[0].name) == 0)` dereferences `(*dir)[0]` unconditionally, but `*dir` is only allocated once at least one entry survives filtering (`:132-139`). An empty/all-filtered directory leaves `*dir` `NULL` -> null deref. **APPLIED**: allocate a ".." placeholder entry when `*dir` is still `NULL` at that point, instead of dereferencing it.
- `fileBrowser/fileBrowser-libfat.c:238,251-256` -- the ROM-streaming file handle shares one file-scope `static FILE* fd;` with the unrelated save-file path (`fileBrowser_libfat_deinit`). Since `main/ROM-Cache.c:247` deliberately never closes the ROM handle after loading, any later save operation's `saveFile_deinit` will `fclose()`+NULL that same static `fd` out from under the ROM loader. **APPLIED**: renamed the ROM-only static to `romFd` and dropped the erroneous `fd` close out of the generic `fileBrowser_libfat_deinit()` (which never legitimately owned that handle -- its own read/write functions already open+close a local `FILE*` per call).
- `fileBrowser/fileBrowser-libfat.c:257-267` -- `fileBrowser_libfatROM_readFile`: if `fopen()` fails but a subsequent `stat()` on the same path succeeds, `fd` stays `NULL` and the function falls through to `fseek(fd,...)`/`fread(fd,...)` with no NULL check. **APPLIED**: added the missing NULL check.
- `vm/wii_vm.c:264-269,291-295` -- both the `ISFS_Open` failure path and the pagefile-growing `ISFS_Write` failure path `return NULL` without destroying the `vm_mutex` created just before (`:254`) or closing `pagefile_fd` -- a retried `VM_Init` re-inits over the leaked handle. **APPLIED**: both paths now destroy the mutex (and the write-failure path also closes `pagefile_fd`) before returning.
- `vm/wii_vm.c:360` -- `if (pagefile_fd)` should be `if (pagefile_fd >= 0)`: 0 is a legitimate ISFS handle but falsy in C, so `VM_Deinit` silently skips `ISFS_Close` for that case. **APPLIED**.

*(`vm/vm.c`, the GC/ARAM counterpart, mirrors the same algorithm with no new issues beyond the already-known "GC build is broken" limitation.)*

## 3. gc_input

**Cleanup**:
- `gc_input/controller-DRC.c:1-2` -- header comment is a copy-paste leftover from `controller-Classic.c` (says "Classic controller input module" but this is the DRC/gamepad module). **APPLIED**.

**Optimizations**:
- `gc_input/controller-Classic.c:128-148,160` and `controller-WiimoteNunchuk.c:175-214,115` -- `available()`/`checkType()` call `WPAD_Probe()` (an IOS/IPC round trip) unconditionally on **every** `_GetKeys` call -- i.e. every input poll for every classic/nunchuk-mapped controller -- not gated behind the cheap `wpadNeedScan` flag the way `WPAD_ScanPads()` a few lines above is (`:154`). Compare `controller-GC.c:105` / `controller-DRC.c:137`, which cache their scan behind a `*NeedScan` flag with no further per-controller IPC call. The expansion type should be derivable from the `WPADData` already fetched by `WPAD_Data()` right above, without a second IPC call per controller per frame. **APPLIED**: both now read `WPAD_Data(Control)->err`/`->exp.type` instead of calling `WPAD_Probe()`, since that data was already refreshed by the caller's `WPAD_ScanPads()`/`WPAD_Data()` a few lines above -- same values, no second IPC round trip.

**Future work**:
- Open TODOs: `controller-GC.c:167-169`, `controller-Classic.c:232-234`, `controller-WiimoteNunchuk.c:263,311`, `controller-DRC.c:200-202`. Correction: these aren't all the same TODO -- `controller-GC.c`'s and the others' `configure()` say "Don't know how this should be integrated" (unrelated, about analog-stick config binding; GC pads have no LEDs at all, and `controller-GC.c`'s own `assign()` already correctly says "Nothing to do here"). The real "light up the LEDs" TODO was specifically `assign()` in `controller-Classic.c`, `controller-WiimoteNunchuk.c` (shared by both `controller_Wiimote` and `controller_WiimoteNunchuk`), and `controller-DRC.c`. **APPLIED** for the two Wiimote-backed ones: `assign(p, v)` now calls `WPAD_ControlLed(p, 1 << v)`, lighting LED `v+1` only (matching the System Menu/most games' per-player convention, not a binary count). `controller-DRC.c`'s left as a stub with a corrected comment -- libogc2's `wiidrc.h` exposes no LED control for the Wii U GamePad at all; it has no per-player indicator lights to drive, only a power/sync light.
- `gc_input/input.c:418-426` -- `load_configurations()` doesn't check `fread`'s return value before comparing against magic bytes; a truncated config file is silently treated as a normal mismatch rather than reported. **APPLIED**: added the missing return-value check.

## 4. gc_audio + rsp_hle

**Cleanup**:
- `rsp_hle/plugin.c:162` sets `g_hle.hle_aud = 0` unconditionally, never toggled elsewhere -- so `hle.c:402`'s `if (hle->hle_aud)` branch and the `send_alist_to_audio_plugin()` it guards (`hle.c:181-185`) are permanently dead; the plugin always emulates the audio ucode in-process instead. Makes the `l_ProcessAlistList`/`HleProcessAlistList` plumbing (`plugin.c:47,88-94,157`) and `rsp_info.ProcessAlistList = processAList` wiring (`main/plugin.c:527`, `main/main_gc-menu2.cpp:720`) dead weight too -- consistent with `gc_audio/audio.c:183-185`'s `ProcessAlist` being an intentionally-empty stub that's never actually called.
- `rsp_hle/plugin.c:161` sets `g_hle.hle_gfx = 1` unconditionally (also never toggled) -- the `hle_gfx == 0` branches at `hle.c:338,415` are unreachable vestiges of upstream's configurable HLE/LLE-graphics split, unused since only one of Rice_GX/glN64_GX is ever linked in.

**Fixes** (both applied):
- `rsp_hle/hle.c:117-138` (`hle_execute`) -- ucode cache insertion writes to `cached_ucodes->infos[cached_ucodes->count]` (line 131) *before* checking `count` against `CACHED_UCODES_MAX_SIZE` (16, `ucodes.h:27`); the bounds `assert()` only runs *after* the write and increment. A 17th distinct ucode signature in one run silently overflows `infos[16]` into the adjacent `count` field (and potentially beyond, since `cached_ucodes` is the last member of the global `g_hle`) before the assert can catch it. **APPLIED**: bounds check moved before the write.
- `gc_audio/audio.c:159-170` / call sites `main/plugin.c:428`, `main/main_gc-menu2.cpp:677` -- `InitiateAudio`'s `BOOL` return is discarded at both call sites. If `AESND_AllocateVoice()` fails, `voice` stays `NULL` but nothing aborts startup -- every later `RomOpen`/`AiLenChanged`/`pauseAudio`/`resumeAudio`/`CloseDLL` call passes that `NULL` `voice` straight into `AESND_Set*`/`AESND_FreeVoice`. **APPLIED** for the live call site (`main_gc-menu2.cpp`'s `audio_info_init()` now returns the result, `loadROM()` aborts on failure like it already does for `rom_read`). `main/plugin.c`'s copy is unreachable -- confirmed not in any Makefile's `OBJ` list -- left alone.

**Future work**:
- Pre-existing upstream TODO/FIXME markers, flagged for completeness only (no DSP math rewrites per this project's established risk constraint): `alist_nead.c:539,550` ("FIXME: implement proper ucode" for `nead_mats`/`nead_efz`), `alist_nead.c:142`, `alist_naudio.c:240,291,305` ("TODO: check which ABI supports it"), `musyx.c:307` (unhandled `ptr_10` case, warns only).

## 5. menu + libgui + gui

**Cleanup**:
- `gui/DEBUG.c` -- confirmed still accurate: almost entirely dead (`SHOW_DEBUG` never defined). The two globals outside the ifdef are genuinely written at call sites (`printToSD` in `main/main_gc-menu2.cpp:406`, `txtbuffer` by several plugin files e.g. `glN64_GX/Combiner.cpp:280-286`) but always go nowhere since `DEBUG_print`'s only live branch needs `SDPRINT` (also commented out).
- `menu/LoadSaveFrame.cpp` and `menu/SaveGameFrame.cpp` (whole files) -- **dead frames**. `MenuContext` constructs/registers/destroys both every run and defines `FRAME_LOADSAVE`/`FRAME_SAVEGAME`, but nothing ever calls `setActiveFrame()` with either index (repo-wide search confirmed) -- `CurrentRomFrame`'s "Load Save File"/"Save Game" buttons use their own local `Func_LoadSave`/`Func_SaveGame` instead (`CurrentRomFrame.cpp:206,250`). All the button/focus/cursor wiring in both is pure overhead.
- `libgui/Button.cpp:139` -- `#include "ogc/lwp_watchdog.h"` sits mid-file instead of with the top includes. **APPLIED.**
- `libgui/LoadingBar.cpp:29` -- unconditional `#include <debug.h>`, unused. **APPLIED** (removed).
- `libgui/LoadingBar.cpp:38,45` and `libgui/MessageBox.cpp:70,78` -- `buttonFocusImage` assigned in both, never read again. **APPLIED** (removed the dead member from both classes -- confirmed write-only via a repo-wide grep of each header).
- `menu/MenuContext.cpp:198-240` (`getFrame()`) -- no case for `FRAME_CONFIGUREPAKS`/`FRAME_ADVANCEDAUDIO` (falls through, returns NULL). Harmless today (nothing calls `getFrame()` with those), one-line fix. **APPLIED.**

**Fixes**:
- `libgui/GuiResources.cpp:28-75` -- `defaultButtonFocusImage` is declared, deleted in the destructor, and returned by `getImage(IMAGE_DEFAULT_BUTTONFOCUS)`, but **never constructed** even though its backing texture asset exists. Every `BUTTON_DEFAULT`-style button (MiniMenuFrame's Save State/Load State/Controller Settings, `MiniMenuFrame.cpp:112-117`) ends up with `focusImage`/`selectedImage` NULL -- no crash (the one deref is null-guarded, `Button.cpp:235`) but the focus-highlight art is silently missing. **APPLIED.**
- `menu/LoadSaveFrame.cpp:163-164` / `SaveGameFrame.cpp:150-151` -- copy-paste bug: the USB variants report `"...from/to SD card"`. Latent since both frames are unreachable, but wrong the moment either gets wired up. **APPLIED** (message text fixed; investigated wiring the frames up too -- they're not redundant with `CurrentRomFrame`'s Load/Save buttons, they offer an explicit per-action SD/USB device choice instead of always using the Settings-configured device, so kept rather than deleted -- but where to put a menu entry point is a UX placement decision, left open).
- `libgui/MessageBox.cpp:69-76` -- constructor never sets `messageFade` in its init list; safe today only because the function-local-static singleton happens to be zero-initialized before the constructor runs, unlike every other member in the same list. **APPLIED.**

**Optimizations**:
- `libgui/InputManager.cpp:63-64` -- `Input::refreshInput()` calls `WPAD_ScanPads()` **twice** every frame on Wii builds: once gated behind `wpadNeedScan` (`:63`), then unconditionally again immediately after (`:64`) -- the flag never actually saves the IPC round-trip it's guarding. Every `Gui::draw()` pays for two full scans instead of one. **APPLIED.**
- `menu/ConfigureButtonsFrame.cpp:326-329` -- `updateFrame()` calls `activateSubmenu(activePad)` on **every frame** this page is visible (~17 `strcpy`s + a `sprintf` + toggling 21 buttons' active state), regardless of whether anything changed. **APPLIED, but more nuanced than it first looked**: every `Func_ToggleButton*` handler already rewrites its own single `FRAME_STRINGS` entry directly, and `Func_NextPad`/`ResetPad`/`LoadPadConfig` already call `activateSubmenu()` themselves -- so a blanket "only refresh when `activePad` changes" gate would have been safe for all of those. The one thing that isn't: `gc_input/input.c:362` can reassign `virtualControllers[activePad].control` (a physical-controller hotplug/auto-detect) while the user is sitting on that exact pad's remap screen without navigating away or pressing Next/Prev -- only a per-frame check catches that. Gated on `virtualControllers[activePad].control` changing instead of removing the check outright.
- `libgui/InputStatusBar.cpp:93` -- `WPAD_Probe()` (IPC round trip) runs unconditionally for up to 4 pads on every draw of the status bar (shown continuously on MainFrame/MiniMenuFrame) -- same per-frame-IPC-probe pattern flagged for gc_input's classic/nunchuk controllers in section 3, a separate call site paying the same cost. **APPLIED**: reads `WPAD_Data()->err`/`->exp.type` instead (already refreshed this frame by `Input::refreshInput()`'s `WPAD_ScanPads()`), same fix as section 3's.

**Future work**:
- `menu/ConfigureButtonsFrame.cpp:426` -- `//todo: save button configuration to file here`: `Func_SaveConfig()` only writes to an in-memory slot, lost on next launch.
- Layout consistency: unlike the newly standardized Settings/Advanced Audio pages, `FileBrowserFrame.cpp`, `SelectRomFrame.cpp`, and `ConfigureButtonsFrame.cpp`'s button tables still use ad hoc per-button x/y placement with no shared column/row convention -- worth a future layout pass, not urgent.

## 6. Rice_GX + glN64_GX + TextureArchive

### Rice_GX

**Cleanup**:
- Remaining `#if !defined(NO_ASM)` sites from the earlier cleanup pass -- **resolved/verified, both APPLIED**:
  - `RenderBase.cpp:1172-1176` -- called `SSELightVert()`, a function with no declaration anywhere in the codebase. **APPLIED** (whole `#if`/`#endif` removed, live `else` branch kept unconditionally).
  - `RenderBase.cpp:1734-1739` -- wrote `gRSPworldProjectTransported` (`RenderBase.h:85` extern), read nowhere else. **APPLIED** (dead branch and the now-fully-unused global both removed).
  - Related, not preprocessor-gated so missed by the earlier pass: `Rice_GX/RSP_GBI_Others.h:126-127` -- guarded by `status.isSSEEnabled`, permanently false since `isSSESupported()` is hardcoded `return false;`. `dkrMatrixTransposed` (`RenderBase.cpp:160`) is consequently dead too. **APPLIED** (both removed).
- **New NO_ASM dead code the earlier pass missed**: `Rice_GX/FrameBuffer.cpp:597-749`'s `CalculateRDRAMCRC()` still carries the full x86 branch family (`__INTEL_COMPILER`, `__GNUC__ && __x86_64__ && !NO_ASM`, separate PIC/non-PIC inline asm) -- ~130 lines that can never compile since the `#ifdef NO_ASM` C fallback is what's live. **APPLIED** (removed, kept the live `#ifdef NO_ASM` branch unconditionally).
- `Rice_GX/Config.cpp:2100-3562` (~1460 lines) -- the entire GTK desktop config-dialog, `#ifndef __GX__`, permanently dead. Large but not urgent (mirrors glN64_GX's equivalent block). Left for a dedicated pass -- not touched here.

**Fixes** (applied):
- `Rice_GX/TextureManager.cpp:71` -- `malloc(sizeof(heap_cntrl))` not NULL-checked before `__lwp_heap_init` dereferences it at startup. **APPLIED**: added `SAFE_CHECK(GXtexCache)`, the same macro already used 17 lines below in this same constructor for the same class of allocation.

**Future work**:
- `Rice_GX/TextureManager.cpp:1190` -- `return; //TODO: Find where used and implement for GX` acknowledged no-op stub.
- `Rice_GX/TextureFilters.cpp:844` -- `FIXME: Compute the correct values. 2/2 seems to always work correctly in Mario64` -- ungeneralized magic number.
- Numerous `//TODO: Implement/Replace in GX` markers in `OGLRender.cpp`/`OGLExtRender.cpp`/`OGLGraphicsContext.cpp`/`TEVCombiner.cpp` mark OpenGL-desktop-only paths with no GX equivalent yet.

### glN64_GX

**Cleanup**:
- `glN64_GX/TEV_combiner.cpp:190-197,784-791` -- two dead functions, author's own comments say "Never Called". Zero call sites confirmed. **APPLIED** (both removed, including their header declarations).
- `glN64_GX/CRC.cpp:42-101` -- `CRC_BuildTable()`/`CRC_Calculate()`/`CRC_CalculatePalette()` (CRC32-table impl) have zero call sites anywhere; actual hashing goes through `Hash_Calculate()` (XXH32) in the same file. ~60 dead lines. **APPLIED** (removed, along with their now-unused `Reflect()` helper and `CRCTable` buffer).
- `glN64_GX/Config_linux.cpp:38-405` (~367 lines) -- GTK/SDL desktop config dialog, `#ifndef __GX__`, permanently dead (same pattern as Rice_GX's Config.cpp). Left for a dedicated pass -- not touched here.
- Large `#ifndef __LINUX__` (Windows-only) blocks are dead throughout since `__LINUX__` is always defined for glN64_GX builds -- e.g. `RSP.cpp:46-89` (Win32 inline-asm), `RSP.cpp:451-465` (SEH RDRAM-size probing), most of `glN64.cpp`'s `DllMain`/window-handle plumbing. Expected given the project's macro rules; real bulk-deletable cruft if this plugin ever gets the Rice_GX treatment. Left for a dedicated pass -- not touched here.

**Fixes** (applied):
- `glN64_GX/RSP.cpp:405-411` -- the main `RSP_ProcessDList()` command loop reads the next opcode with **no bounds check** against `RDRAMSize`. The sibling `_ProcessDListFactor5()` path a few lines above (`:258-264`) already has an explicit guard with a comment noting the upstream unguarded-read risk -- fixed there, not here. A malformed/truncated display list through the normal (non-Factor5) path can read past the end of RDRAM. **APPLIED**: same guard pattern as `_ProcessDListFactor5()`.
- `glN64_GX/DepthBuffer.cpp:28-133` -- the depth-buffer linked list has no LRU cap (unlike the texture cache's `maxBytes`), can grow unbounded over a session. `DepthBuffer_AddTop()` (`:95`) also doesn't NULL-check its `malloc()` before dereferencing. **APPLIED**: capped at 16 entries, evicting the LRU (bottom) entry past that -- the list is already MRU-ordered (`DepthBuffer_MoveToTop()` runs on every cache hit), so this is the same shape of cap the texture cache already has. Added the missing NULL checks in `DepthBuffer_AddTop()` and its one call site.

**Optimizations** (deferred -- need dedicated visual-regression testing, not a quick pass):
- `glN64_GX/Textures.cpp:2271-2303,1861-1880` -- the per-texture-tile-bind lookup (`TextureCache_Update`) does a linear walk over the *entire* MRU texture cache list, no hash/bucket index by CRC -- runs potentially many times per frame, cost scales with cache occupancy rather than O(1). Rewriting the cache's indexing is invasive; needs its own pass with real texture-heavy ROMs to verify no visual regression, not bundled into this cleanup sweep.
- `glN64_GX/2xSAI.cpp:199-217` -- fetches each of 16 neighboring texels via a virtual `PixelIterator::operator[]` call -- 16 virtual dispatches per output pixel, comparatively expensive on PowerPC, for every texture while 2xSAI is on. Same reasoning -- deferred.

**Future work**:
- `glN64_GX/GBI.cpp:565-574` -- unrecognized microcode just `printf`s (USB Gecko only) and leaves type unset for `__GX__` builds; the fallback dialogs are `#ifndef`'d out. Own `//TODO: Make sure having ucode = NONE is ok` admits this was never verified on GX.
- `glN64_GX/gSP.cpp:194-207` -- billboard matrix handling `//TODO: Properly implement this.` (currently just translates by vertex 0).
- `glN64_GX/gSP.cpp:209-213` -- `//TODO: Investigate this` on an active workaround in the live vertex path.
- `glN64_GX/TEV_combiner.cpp:514,544,677,697,714` -- five `//TODO` sites marking a known constant-color combiner gap (RGB vs AAA constant conflict).
- `glN64_GX/OpenGL.cpp:1045,1432,1465` -- unverified Ztex/depth-source interaction, flagged by the author but never followed up.

### TextureArchive

**Fixes**:
- `TextureArchive/ArchiveReader.h:78-79` / `ArchiveReader.cpp:23-25` -- **reachable crash/heap-corruption risk**: `ArchiveReader::ArchiveReader()` only initializes the zlib `stream` fields; `file` (`FILE*`) and `table` (`ArchiveTable*`) are left uninitialized. It's a Meyer's-singleton, and its first real use (`Rice_GX/TextureFilters.cpp:1991-1998`, `InitExternalTextures()`) calls `CloseExternalTextures()` -> `reset()` *before* `setArchiveFile()` is ever called -- `reset()` does `if(file) fclose(file);` and an unconditional `delete table;` against garbage pointer values on that very first call. Hit on every session's first hi-res-texture-pack init, not theoretical. **APPLIED** (constructor now zero-inits `file`/`table`).

**Future work**:
- `ArchiveReader.cpp:118` -- `unsigned int width = info.width * 4 * 4; // FIXME: Look this up` -- unverified stride calculation.
- `ArchiveReader.h:20-38` -- `SortedArray<T>::find()`'s binary search carries `// TODO: Verify correctness` on its own definition line, with the old linear-search implementation left commented out directly below instead of removed.

## 7. gc_memory

**Possible lead for the PC=0 dynarec hang #1 (most concrete)**: `gc_memory/memory.c:370-371` -- MI register mapping:
```c
rwmem[0xa830] = rw_mi;   // should almost certainly be rwmem[0x8430]
rwmem[0xa430] = rw_mi;
```
Every other register block maps a matching *pair* -- cached KSEG0 (`0x8...`) and uncached KSEG1 (`0xa...`) -- to the same handler at the same low bits (DPC: `0x8410`/`0xa410`, VI: `0x8440`/`0xa440`, SI: `0x8480`/`0xa480`). MI is the one exception: `0xa430` (correct) is mapped, but the cached mirror is mapped to `0xa830` -- an address matching no real hardware -- instead of `0x8430`. `rwmem[0x8430]` is left at default `rw_nomem`, so any access to the *cached* MI-register mirror (`0x8430xxxx`) routes through the TLB-miss path (`virtual_to_physical_address()` -> `TLB_refill_exception()`) instead of the real handler -- a spurious guest exception on cached MI access. Most N64 code uses the uncached KSEG1 mirror (correctly mapped), which likely explains why this hasn't been 100% reproducible.

**APPLIED**: fixed to `0x8430`. Not yet re-run against the CIC-6105 hang sweep (that needs a fresh multi-hour soak across Banjo-Kazooie/Zelda MM/Zelda OoT MQ to get a real before/after hang-rate comparison, not a quick smoke test) -- next session should do that A/B before considering this closed.

**Possible lead #2**: `gc_memory/dma.c:292-354` (`dma_sp_write`/`dma_sp_read`, RSP<->RDRAM DMA used by every audio/graphics ucode task) -- `dramaddr` is masked once at entry (24-bit/16MB mask) but advances by `length+skip` for up to 256 iterations with no per-iteration or final clamp against the actual `rdram[]` size (`MEM_SIZE`, 4-8MB). Only the *final* value gets re-masked before being written back to the register -- the loop body already dereferenced the unclamped, larger values. A `count`/`skip` combination that walks `dramaddr` past `MEM_SIZE` (still within the 24-bit mask, so not "invalid") reads/writes out of bounds into whatever follows `rdram[]`. On the SP-task path, active around the same SI-interrupt timing window implicated in the CIC-6105 investigation.

**Possible lead #3**: `gc_memory/pif.c` -- `update_pif_write()`/`update_pif_read()` walk the 64-byte `PIF_RAM` and pass `Command = &PIF_RAMb[i]` into controller/EEPROM command handlers that index up to `Command[0x25]` (37 bytes past `Command[0]`). `i` ranges up to `0x3F` in a 64-byte buffer, so a command block starting late produces accesses up to ~36 bytes past the end of `PIF_RAM` -- OOB write into whatever follows in memory.c's globals. Distinct from the `cic_challenge` logic already investigated; a buffer-bounds bug in the general dispatch that runs on every PIF exchange, including the CIC-6105 handshake's surrounding traffic.

**Fixes**:
- `gc_memory/memory.c:2944-2945` -- `write_pifd()` writes the high 32 bits of `dword`, then immediately writes the low 32 bits to **the same address** instead of `+4` (compare `read_pifd`/`write_pifh` nearby). Any 64-bit store to PIF RAM silently drops the high word. **APPLIED.**
- `gc_memory/dma.c:123-137,199-213` (`dma_pi_read`/`dma_pi_write`) -- the SRAM/FlashRAM branch uses `pi_dram_addr_reg` completely unmasked as an `rdramb[]` index, and a length up to ~16MB as the `memcpy` size into/from a fixed 32KB `sram[]` buffer, no clamp at all. The Cart-ROM DMA branch in the same functions explicitly clamps against `rom_length`/`MEM_SIZE`/`MEMMASK` a few lines below -- the SRAM/FlashRAM branch never got the equivalent hardening.
- `gc_memory/flashram.c:137-151` -- `erase_offset` (`(command & 0xffff) * 128`, up to ~8MB) indexes the fixed 128KB `flashram[]` buffer with no bound check. In the MEM2 layout that buffer is immediately followed by SRAM, MEMPACK, and the 4MB dynarec BLOCKS/RECOMPMETA regions -- an out-of-range offset can write straight into those.
- `gc_memory/flashram.c:178-211` (`dma_read_flashram`/`dma_write_flashram`) -- same unmasked-`pi_dram_addr_reg` pattern as the SRAM DMA above, no `& MEMMASK` anywhere.
- Systemic pattern: `read_vi`/`write_vi`/`write_vid`, `read_ai`/`write_ai`/`write_aid`, `read_ri`/`write_ri`, `read_si`*, `read_rsp`/`write_rsp`(`_reg`), `read_dp`/`write_dp`, `read_dps`/`write_dps` in `memory.c` all index their backing pointer table with the address's raw low-16-bits and **no bounds check** in the fallback path. Needs a malformed/corrupted access to trigger normally -- exactly the state after a wild jump has already started.
- The two blocks that DO bound-check (`read_rdramreg`*, `read_pi`/`write_pi`*) get it wrong: they compare the raw offset against the array's `sizeof()` (byte size) but index directly by that same offset (not divided by 4) -- ~4x too permissive, so offsets between true element count and `sizeof()` read uninitialized static pointer slots and dereference them.
- `gc_memory/pif.c:403-437` (`internal_ControllerCommand`, case 0x03) -- `case PLUGIN_RAW:`/`default:` are nested inside the mempak-range-check's `else` block rather than siblings of `case PLUGIN_MEMPAK:` (unlike the structurally identical case 0x02 block above it). Works today (valid C switch-label jump target), but a landmine for a future edit that doesn't realize this.

**Cleanup**:
- `gc_memory/tlb.h:69-72` -- `TLBCache_get_r/w` macros use `{...}` not `(...)`, unusable as an expression. Dead in practice: only reachable via the GameCube-only `USE_TLB_CACHE`/`TINY_TLBCACHE` path, not compiled for Wii.
- `gc_memory/memory.c:2790-2830` -- commented-out guards/`printf` diagnostics left around otherwise-live flashram-status code.
- `gc_memory/pif.c:218-224` -- commented-out `case 8:` (RTC write) and `default:` diagnostic in `EepromCommand`.

**Future work**:
- `gc_memory/memory.c:215` -- `// TODO MPAL 48628316`: PAL-M VI clock rate unhandled, falls through to PAL.
- `gc_memory/memory.c:685` -- `// TODO: fast_memory = 0?` next to the framebuffer-protection loop, an open question from a previous author.
- `gc_memory/memory.c:2148` -- `/* XXX: assume 16bit stereo */` in `get_dma_duration()`, acknowledged AI-timing simplification.

## 8. r4300 (CPU core: interpreter + dynarec)

**Investigated and ruled out** (recorded so it isn't re-checked): traced the exact path section 7's MI-mirror lead would take through here. `gc_memory/tlb.c:60-80` calls `TLB_refill_exception()` on failed translation; `r4300/exception.c:46-113` always leaves `r4300.pc` at a valid non-zero vector (`0x80000180`/`0x80000000`) on every path -- no way through this function leaves `pc` at 0. On the dynarec side, `Wrappers.c:170-179`: when `update_invalid_addr()` fails, `r4300.pc` has *already* been overwritten with the exception vector by the time control returns, so `address = r4300.pc` correctly picks up the corrected vector. A TLB-refill from a bad MMIO access mid-block does NOT, by itself, leak a wild/zero PC into block dispatch -- both cores' exception-vector-jump logic is sound for this path. The bug is elsewhere; see below.

**Closest thing to a lead -- structural fragility in JR/JALR's jump-target computation**: `r4300/ppc/MIPS-to-PPC.c:2073,2112` (JR/JALR) stash the jump target into `r4300.local_gpr[0]`, and `genJumpTo()`'s `JUMPTO_REG` case (`:4349-4354`) reads it back via a hardcoded `REG_LOCALRS=34` (`Wrappers.h:36`) -- i.e. computes the address of `r4300.gpr[34]`, two past the declared 32-entry array, relying entirely on `hi`/`lo`/`local_gpr[2]` sitting immediately after with zero padding (`r4300.h:45-56`). Verified today's layout is in fact consistent, so not the active cause -- but there's **no static_assert, no named offset**, nothing that would fail to compile if a field were ever inserted/reordered nearby (exactly the kind of change a debugging session tends to make). Such a change would silently redirect every JR/JALR in every recompiled block to read garbage as the jump target -- precisely the "wild jump" symptom under investigation, with zero compiler diagnostic. `r4300.h` wasn't touched this session, so ruled out as the *current* cause, but worth a `static_assert(offsetof(R4300,local_gpr) == offsetof(R4300,gpr) + 34*8, ...)` guard regardless.

**Fixes**:
- `r4300/ppc/Wrappers.c:472-582` (`dyna_mem`) -- every *load* case breaks its loop when `address` reads back 0 (the `write_nomem`/`read_nomem` sentinel set on a failed translation), but none of the *store* cases (`MEM_SW`/`SH`/`SB`/`SD`/`SWC1`/`SDC1`/`SWL`/`SWR`/`SDL`/`SDR`) check this -- a multi-word store that TLB-faults partway through keeps issuing further stores after the exception was already signaled and PC redirected. Doesn't leak a bad address, but a real store-after-exception correctness bug in exactly the MMIO/exception-interaction path this investigation cares about. Should stop like the load cases do.
- `r4300/exception.h:33-38` -- `address_error_exception`, `TLB_invalid_exception`, `TLB_mod_exception`, `integer_overflow_exception`, `coprocessor_unusable_exception` are declared but **never defined or called anywhere**. Only `TLB_refill_exception`/`exception_general` exist. Concretely: misaligned accesses and integer overflow are never trapped -- `dyna_mem` never validates alignment, ADD/ADDI/SUB never check signed overflow. A misaligned/overflowing value that real hardware would trap immediately (keeping PC safely in the vector) instead flows through as "valid" here. Not proven to cause the CIC-6105 hang, but a real, verifiable gap in "is every abnormal-PC condition actually caught."
- `r4300/Recomp-Cache-Heap.c:246-256` (`RecompCache_Alloc`) -- `node_heap` allocation retries `release(size)` exactly once, unlike every other allocation in the file (`cache`, `MetaCache_Alloc`) which loop until success. If one `release()` can't free enough space, `newBlock` stays NULL and the next line writes through it -- NULL-pointer write. Should match the retry-loop pattern used elsewhere in the same file.
- `r4300/interupt.c:333-347` (`gen_interupt`) -- dereferences `q->type` with no NULL check, unlike `check_interupt()`/`get_event()`/`remove_event()` in the same file which all guard `q == NULL`. Not known reachable today (persistent queue entries should always exist), but the one ungated spot, and `gen_interupt()` runs on every dynarec dispatch when `cp0_cycle_count >= 0` -- worth hardening defensively.

**Optimizations**:
- `r4300/ppc/FuncTree.c:28-45` (`find_func`) -- plain unbalanced BST keyed by function start address, no rebalancing, runs on essentially every dynarec dispatch. Blocks compile in increasing-address order as code executes linearly, which is close to the BST's pathological insertion order -- degrades toward O(n) per lookup on pages with many functions (larger ROMs, like the CIC-6105 titles, stress this harder). A sorted array + binary search (functions per page bounded, change rarely) would bound this.
- `r4300/Recomp-Cache-Heap.c:103-106,225-244,317-319,346-389` (`heapify`) -- rebuilds heap order via n-1 individual sift-ups (O(n log n)) instead of linear-time bottom-up heap-build (O(n)). Looks like a deliberate lazy-heapify design (not a correctness bug, callers reliably heapify before popping), but `release()` -- called on every code-cache-full eviction, frequent under heavy compile pressure -- always pays the more expensive rebuild. Worth documenting the design intent and switching to linear-time heapify given how hot this path is.

**Cleanup**:
- `r4300/ppc/MIPS-to-PPC.c:57,90-104,129,149-170,4379-4433,4437-4506` + `Recompile.c:176-263` -- `LAZY_GEN_CALLS` is not in the always-on flag set, so this entire lazy-call-site machinery is permanently dead. The code's own comment (`MIPS-to-PPC.c:172-175`) says "It hasn't proven to be faster" -- good candidate for deletion rather than carrying it indefinitely.
- `r4300/compare_core.c:79-111` -- gated behind `COMPARE_CORE`, not defined anywhere, currently dead. If ever revived: `fread(comp_reg, 4, sizeof(long), f)` (`:88`, similarly `:92/96`) reads `4*sizeof(long)` bytes where the surrounding logic only wants 4.

**Future work**:
- Pre-existing upstream TODO/FIXME markers (not re-litigating the math): `MIPS-to-PPC.c:23` (idle-branch opt), `:2089,2136` ("TODO: jr"/"jalr", actually dead -- `INTERPRET_JR`/`JALR` are permanently defined so the `#else` returning `CONVERT_ERROR` is unreachable, arguably Cleanup), `:807,828,2840,2863` (64-bit value handling), `:3520,3551,3584,3617,3813`, `:3774,3810` (FP rounding-mode FIXMEs), `Register-Cache.c:396`, `Recompile.c:23` (stale -- already implemented via `USE_RECOMP_CACHE`), `:303,415,527`.
- `Recompile.c:463-488` (`pass0`) -- two asserts guard jump-target bounds; the BEQ/BNE one depends on `is_j_out()` (relative to the *function's* start) and `isJmpDst[]`'s indexing (relative to the *page's* start) lining up, which only holds when a function starts exactly at its block's start. `NDEBUG` isn't defined anywhere in the build, so this currently aborts rather than silently corrupting on a mismatch -- but it means a real violation here is a hard `abort()`, not graceful watchdog recovery, and would become silent corruption of the file-scope `isJmpDst[1024]` array if `NDEBUG` were ever added without this being noticed as load-bearing.
- Neither the interpreter (`pure_interp.c:125-138` JR) nor the dynarec (`MIPS-to-PPC.c:2064-2101` JR) validates jump-target alignment/range before using it as the next dispatch address -- consistent with the missing address-error-exception gap above. Real hardware would refuse a misaligned target outright.

**Also checked, found sound**: the self-modifying-code/block-invalidation machinery (`invalidate_block`, `RecompCache_Free`, `unlink_func` in `Recomp-Cache-Heap.c:130-172,298-315` and `Recompile.c:539-553`) -- freeing a function properly patches every incoming linked branch to a safe trampoline before the code memory is released, which is the right guard against a dangling jump into freed code.

## 1. Build system (Makefiles, `.dev/` tooling)

**Fixes** (all applied)
- `Makefile.base:155` -- `ifeq ($(strip mupen64_GX_gfx/main.cpp),)` compares
  a literal path *string* to empty, not a file/variable -- always false, so
  the `else` (`export LD := $(CXX)`) always wins regardless of what's being
  built. **APPLIED**: replaced with the unconditional assignment it always
  resolved to.
- No header-dependency tracking: object rules don't depend on the headers
  they include (the `HEADER` var in Makefile.base is defined but never wired
  to anything), so a header-only change doesn't force its dependents to
  recompile. This isn't hypothetical -- it caused a real ODR-violation/stale
  object bug this session (MenuContext.h) that needed a full `make clean` to
  fix. **APPLIED**: `-MMD -MP` + `-include $(ALL_OBJ:.o=.d)`. This alone
  shifted GNU Make's default goal (a rule read in from a `.d` file via
  `-include`, before `all:` is parsed, becomes the new "first target in the
  first makefile" -- a bare `make` with no target silently built nothing).
  Pinned back with `.DEFAULT_GOAL := all`. Verified: touching a shared
  header (`perf_prof.h`) now recompiles exactly its 8 dependents, not
  everything and not nothing; a clean full build still produces the `.dol`.
- `clean:` does `find . -name '*.o' -delete` from the repo root -- deletes
  `.o` anywhere under the tree, including inside `.claude/worktrees/*` if
  one exists with its own in-progress build. **APPLIED**: scoped to this
  project's own source directories, extended to also remove the new `.d`
  files.

**Cleanup** (all applied)
- `Makefile.base:168` -- dead commented-out line (`#	DolTool -d $(ELF)`). **APPLIED** (removed).
- `Makefile.base:10` -- `DEBUG_FLAGS` comment lists `-DSHOW_DEBUG` twice. **APPLIED** (removed the duplicate).

**Resolved (was a "to verify" flag)**
- Observed real `PERF_PROF` output during this session's testing even
  though `Makefile.base`'s committed `DEBUG_FLAGS` has `-DPERF_PROF`
  commented out. **Answer**: `.dev/build_profiling.sh` passes
  `DEBUG_FLAGS=-DPERF_PROF` as a `make` command-line override (after a
  forced `clean`, since there's no header-dependency tracking to catch a
  flag-only change -- now less of a footgun with the `.d`-file fix above,
  though a flag change still isn't a header change so `clean` is still
  needed for it specifically) -- not a second definition, not an
  uncommitted local edit. `perf_prof.h`'s "opt-in only" framing holds.
- `Makefile.glN64` defines `-D__LINUX__` (glN64_GX builds only -- NOT
  defined for Rice builds). **Resolved during the Rice_GX/glN64_GX pass
  (#6)**: that pass's dead-code removal used `NO_ASM` (universal) and
  runtime-false `isSSEEnabled`/zero-call-site checks, never `__LINUX__` --
  confirmed unaffected, not just assumed.
- **New gotcha found while testing the `#1` header-dependency fix**:
  `Makefile.Rice_wii` and `Makefile.glN64_wii` (and every other
  plugin/platform pair) share object *filenames* (`main/main_gc-menu2.o`,
  `menu/MenuContext.o`, etc.) despite compiling them with different
  preprocessor defines (`-DRICE_GFX` vs `-DGLN64_GX -D__LINUX__`). Building
  one target right after the other **without `make clean` in between**
  produces a real, confusing failure: `-MMD -MP`'s dependency tracking only
  watches source/header mtimes, not compiler flags, so it can leave a
  target's object files built with the *other* target's flags, which
  surfaced as linker "multiple definition" errors on unrelated-looking
  symbols (`glN64_useFrameBufferTextures`, `renderCpuFramebuffer`, etc.)
  when reproduced. Not new -- switching targets always needed a clean
  first, per `.dev/build_profiling.sh`'s own comment -- but easy to forget
  now that plain incremental rebuilds usually just work. No Makefile
  change made for this (a legitimate `-j4` parallel build of the *same*
  target must keep sharing objects); noting it so the next confusing
  "multiple definition" error is recognized immediately as "did I clean
  between targets" instead of re-investigated from scratch.

**Future work**
- 14 Makefiles for an 8-way (2 plugins x 4 platforms, minus one) build
  matrix is otherwise clean/non-duplicative -- each leaf file is ~10 lines
  layering `Makefile.<Plugin>` + `Makefile.<platform>` + `Makefile.base`.
  No action needed, just noting the structure held up fine on inspection.
- GameCube targets' "broken due to missing libfat.a" limitation (already
  documented in the wii64-build-and-test skill) isn't re-litigated here.
