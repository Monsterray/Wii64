# macOS Intel development setup

## Recommendation

Use devkitPro's macOS pacman installation and its `wii-dev` package group for the Wii cross-toolchain and tools. The devkitPro setup guide for macOS calls for Xcode Command Line Tools, the devkitPro pacman installer, and `dkp-pacman -S wii-dev`. [devkitPro Getting Started](https://devkitpro.org/wiki/Getting_Started/devkitPPC)

Keep libogc2 and libfat in separate source checkouts outside Wii64, then build and install them using their upstream instructions. libogc2 documents `make` followed by `make install`; libfat documents `make ogc-release` followed by `make ogc-install`. Upstream also offers packages from Extrems' supplementary pacman repository, configured on macOS in `/opt/devkitpro/pacman/etc/pacman.conf`, for developers who prefer package-managed updates. [libogc2 build instructions](https://github.com/extremscorner/libogc2#building), [libfat source](https://github.com/extremscorner/libfat), [supplementary repository instructions](https://github.com/extremscorner/pacman-packages#adding-repository)

Keep `DEVKITPRO=/opt/devkitpro` and use the installed `DEVKITPPC` unless a tested project requirement calls for another toolchain. Build any source-built libogc2 and its linked libraries with the same devkitPPC used for Wii64. The libogc2 upstream rules derive Wii include and library paths from its target-specific `wii` directory. [libogc2 `wii_rules`](https://github.com/extremscorner/libogc2/blob/master/wii_rules)

## Findings on this Mac

This is an Intel (`x86_64`) Mac. The installed SDK reports devkitPPC `r50-1`, GCC `16.1.0`, and GNU ld `2.46.0`; `elf2dol` is installed at `/opt/devkitpro/tools/bin/elf2dol`. The environment currently sets `DEVKITPRO=/opt/devkitpro` and `DEVKITPPC=/opt/devkitpro/devkitPPC`.

libogc2 is present under `/opt/devkitpro/libogc2`, with Wii headers in `wii/include`, Wii archives in `wii/lib`, and its `wii_rules` at the package root. Examples include `libogc.a`, `libwiiuse.a`, and `libfat.a` in `wii/lib`. The directory layout is consistent with libogc2's upstream rules.

## Wii64 mismatch

Before this fix, `Makefile.wii` searched only for headers in `$(DEVKITPRO)/libogc2/include` and libraries in `$(DEVKITPRO)/libogc2/lib/wii`. Neither path exists in the local install. The corresponding existing paths are:

| Wii64 currently expects | Present on this Mac |
| --- | --- |
| `/opt/devkitpro/libogc2/include` | `/opt/devkitpro/libogc2/wii/include` |
| `/opt/devkitpro/libogc2/lib/wii` | `/opt/devkitpro/libogc2/wii/lib` |

`Makefile.wii` now detects either layout. The `elf2dol` executable exists, but the Makefile calls it by bare name, so `/opt/devkitpro/tools/bin` must be on `PATH`.

The README's build section needs correction: it refers to a Windows “component” installer, documents `elf2dol.exe`, and pins devkitPPC `r41-2`, while this machine has r50-1. It also says libogc2 has no package route, but libogc2 upstream now documents its supplementary pacman repository. Its claim that prebuilt libogc2 archives work only with r41-2 is project-specific and is not confirmed by the official setup sources reviewed here; do not treat it as a general devkitPro requirement. [README build section](../README.md#building-from-source), [libogc2 upstream instructions](https://github.com/extremscorner/libogc2#installing)

## Build verification

The first normal build failed during compilation because `Makefile.wii` searched the absent `include/` directory. Re-running with explicit include and library paths pointing to the existing `wii/include` and `wii/lib` completed successfully with devkitPPC r50-1/GCC 16.1.0. After updating the Makefile to detect both layouts, a clean build with no path overrides also completed and `elf2dol` produced `wii64-glN64.dol`. The successful link confirms that the installed libraries work with this toolchain; it does not establish that the resulting DOL boots or that all runtime behavior is correct.

The Makefile now checks both libogc2 directory layouts, so the ordinary build command can use either a standard installation or the layout present on this Mac. The build emitted existing compiler warnings and an LTO note about serial compilation; none stopped the build.

## Local build and test workflow

The portable helper scripts live in `.dev/`; generated files, ROMs, Dolphin profiles, screenshots, and logs stay local and are ignored by Git. Source `.dev/env.sh` in a terminal to select the installed macOS toolchain, then run `.dev/build.sh glN64_wii`. The Wii makefile detects this Mac's `libogc2/wii/include` and `libogc2/wii/lib` layout. The build creates `wii64-glN64.dol` in the repository root.

For an automated smoke test, place legally obtained ROMs in `~/Library/Application Support/Dolphin/Load/WiiSDSync/wii64/roms` and run `.dev/dolphin_test.sh wii64-glN64.dol`. The script stages ROMs and box art into an isolated `.dev/dolphin_profile`, leaving the normal Dolphin profile untouched. `WII64_ROM_DIR` can override the ROM source directory.

For an interactive run on this Mac, use:

```sh
.dev/dolphin_test.sh wii64-glN64.dol interactive
```

The script stages ROMs, copies the normal Mac Wii Remote bindings into the isolated profile, and opens Dolphin through macOS Launch Services. Wii Remote 1 uses `Quartz/0/Keyboard & Mouse`: A and B use the left and right mouse buttons, and IR uses the mouse cursor. The script enables MMU and SD folder sync. Its default timed smoke test still dumps a frame and exits. Launching Dolphin's executable directly from a Codex-managed shell can abort during Cocoa initialization; `open` avoids that host GUI-session issue.
