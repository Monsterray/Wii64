---
name: wii64-build-and-test
description: Build and test Wii64 on Windows or macOS. Use for devkitPro, libogc2, libfat, Dolphin, SD or USB paths, Wii Remote input, crashes, and performance checks.
---

# Wii64 build and test

Use the repository scripts. Read [the build section](../../../README.md#building-from-source) before changing setup steps. Keep libogc2 and libfat in separate source checkouts; install them with the same devkitPPC used for Wii64. Use checkout paths without spaces if an upstream make recipe cannot handle them. Do not commit those checkouts or generated artifacts.

## Build

1. In a devkitPro shell, run `source .dev/env.sh`.
2. Run `.dev/build.sh glN64_wii` for the glN64 Wii target. Other targets include `Rice_wii`, `glN64_gc`, and `Rice_gc`.
3. When you change target, toolchain, or build flags, run `.dev/build.sh glN64_wii clean` before the next build. Objects are shared across targets.
4. Confirm that the new `.dol` exists. A successful link does not prove that the app boots.

### Host-specific setup

- **Windows:** Run `.dev/setup-devkitpro-windows.sh` once. It builds libogc2 and libfat from checkouts beside the repo with devkitPro's current devkitPPC and installs them into `C:\devkitPro\wii64-sdk`; the makefiles use that root when it exists and stop with a pointer to the script when it does not. `.dev/env.sh` forces `DEVKITPRO=/c/devkitPro` and `DEVKITPPC=$DEVKITPRO/devkitPPC`, because shells here often carry a stale `/opt/devkitpro`, and sets native `TMP`/`TEMP`. Do not use `devkitPPC-r41-2` or `C:\devkitPro\libogc2`: they belong to WiiStation, and current Wii64 does not compile against them (`VIDEO_WaitForFlush`, `copy_interlaced`). Scripts that call `python3` source `env.sh`, which maps it to `python` when `python3` is only the Microsoft Store placeholder. See [the build section](../../../README.md#building-from-source).
- **Intel macOS:** Use the devkitPro installation at `/opt/devkitpro`. This Mac built with devkitPPC `r50-1` and GCC `16.1.0`. Install Xcode Command Line Tools and the devkitPro `wii-dev` group. Build libogc2 and libfat from external checkouts if they are not installed. The Wii makefile accepts both `libogc2/include` + `libogc2/lib/wii` and `libogc2/wii/include` + `libogc2/wii/lib`. See [the tested Mac setup](../../../doc/macos-development-setup.md).

If `-lfat` is missing, check that the Wii libfat archive was installed in the library path used by this toolchain. Rebuild libfat with that toolchain if needed. If `make` reports “Nothing to be done,” clean before testing a newly installed library or changed flags.

The GameCube targets compile on this Mac but do not link: this libogc2 installation has no GameCube `libfat.a`. This does not affect the Wii targets.

Treat an LTO serial-compilation note as a build-speed warning, not a link failure. Check each compiler warning on its own terms.

## Test in Dolphin

Run `.dev/dolphin_test.sh wii64-glN64.dol` for a timed smoke test. Use `.dev/dolphin_test.sh wii64-glN64.dol interactive` for controller or ROM tests. The script uses `.dev/dolphin_profile`; do not edit the user's normal Dolphin profile. On macOS it launches through `open` to use the GUI session and copies the normal Mac Wii Remote bindings. On Windows, set `DOLPHIN_EXE` if Dolphin is outside its default location.

The script enables MMU and SD folder sync. The timed test also enables immediate XFB and disables modal panic dialogs. If Dolphin reports an unknown DSP ucode, set `DSPHLE=False` (LLE) in the isolated profile. If the menu stays black, check immediate XFB there. Do not assume every black screen is a Wii64 bug.

Stage legally obtained ROMs in `~/Library/Application Support/Dolphin/Load/WiiSDSync/wii64/roms` on macOS, or set `WII64_ROM_DIR` to another folder. Wii64 reads `sd:/wii64/roms`; Dolphin's SD sync folder must be copied into the isolated profile **before boot**. For USB tests, mount a compatible USB device and use `usb:/wii64/roms`. A missing directory does not show whether the drive mounted; check both separately.

For mouse-as-Wii-Remote on macOS, use an emulated Wii Remote 1 with `Quartz/0/Keyboard & Mouse`: A is left click, B is right click, and IR uses the cursor. Confirm the controller is connected in Dolphin before judging Wii64 input. The script copies these bindings, but visible cursor response still needs an interactive check.

Read only the latest boot section of Dolphin's append-only log. If Dolphin reports an invalid read or write, record its address and PC, then map the PC against the **matching** `.elf`. MMU can expose a guest fault; enabling it is not a code fix. The earlier direct-DOL null reads at `0x80004558` and `0x8005b43c` were traced to unchecked startup arguments/config access and fixed in Wii64. Do not classify a repeat as harmless.

For chain tests, Dolphin can render the game correctly while `xfb_NN.bin` is a solid color. Check Dolphin's frame dump or live video before calling this a rendering failure. The same capture path produced valid frames on the Wii.

## Deeper checks

For MEM2 layout, ROM paging, HBC agent integration, or automated crash capture,
read [paging and agent constraints](../../../doc/rom-paging-and-agent.md).
Use `bash .dev/test_rom_vm.sh` for loader, layout and cache regressions. Preserve
the record at `0x91800000` in agent builds and the prior VM DSI handler. A
running agent also answers port 4299; use the HBC client's version check before
calling that state "Homebrew Channel ready". Raw-SD Dolphin runs use the config
already in the image; stage new arguments with folder sync first.

For real Wii tests, configure the installed HBC-Reborn queue client with the
central server first; see [hardware sessions](../../../doc/hardware-session.md).
Use `.dev/hardware_run.sh`: it queues and waits before contacting the Wii.
With the external HBC-Reborn client, it stages the chain's replay files before
launch. Keep manual replay staging for older HBC with `WII64_STAGE_INPUTS=0`.
Keep each queued DOL unchanged and retain its matching ELF. The same workflow
uses the installed macOS/Linux or Windows client; server failure leaves the
job waiting rather than permitting a direct upload.

For subsystem selection or probe overhead, read [the subsystem survey](../../../doc/subsystem-profile-2026-09-30.md).
Run `.dev/test_subsystems.sh`, then `.dev/profile_subsystems.sh` for clean frozen
full/control/full Wii builds and reports. New timers require both `PERF_PROF`
and `PERF_SUBSYSTEM_PROBES`; ordinary profiling builds have no new timer cost.
Interpret inclusive spans separately. Hot-operation estimates can alias, and
requested limiter sleep is not a measurement of available CPU capacity.
The driver supports Rice, verified artifact reuse, frozen chain configs, and
deferred collection; consult the survey document for those branches. Use the
same probe boundaries for an A/B/A comparison. Failed loads can retain old
counters, and completed VIs can come from a non-rendering scene. Check replay
loads and captures. Keep live Bash launchers unchanged until their jobs end.
Wii paging now defaults to read-ahead plus startup preflush. Explicitly disable
both flags for the old reference path; keep startup time separate from gameplay.
For texture changes, run `bash .dev/test_texture_hash.sh` and use the matched
graphics chain. Texture hashes, lookups, conversion and submission are separate
probes; do not infer GPU execution time from their inclusive CPU-side spans.

Use [controller testing](../../../doc/controller-testing.md) for input checks and [hardware sessions](../../../doc/hardware-session.md) for Wii results. Use `.dev/build_profiling.sh`, `.dev/wii64_diag.sh`, and the existing performance baselines when comparing speed. Compare dynarec and interpreter runs separately. Check the target's preprocessor branches before changing code: Wii builds and host builds do not execute every branch. Keep ROMs, profiles, logs, screenshots, and build products out of Git.
