# Wii64 agent guidance

For build, Dolphin, controller, or performance work, read [the Wii64 build-and-test skill](.agents/skills/wii64-build-and-test/SKILL.md). Use [README.md](README.md) and [the macOS setup notes](doc/macos-development-setup.md) for contributor instructions.

- Keep libogc2 and libfat in separate source checkouts. Build them with the devkitPPC used for Wii64.
- Use `.dev/env.sh` and `.dev/build.sh <target>`. Clean when you change targets, toolchains, or build flags because targets share object files.
- Treat toolchain pins as host-specific: the tested Windows setup uses r41-2; this Intel Mac builds with r50-1. The Wii makefile accepts both tested libogc2 layouts.
- Test the new `.dol`, not only the link result. Use an isolated Dolphin profile. Stage ROMs before launch and keep ROMs, logs, screenshots, and profiles out of Git.
- On macOS, `.dev/dolphin_test.sh wii64-glN64.dol interactive` prepares the profile and starts Dolphin. The script copies the Mac Wii Remote mapping, passes an absolute profile path, and enables MMU and SD folder sync.
- On Windows, use the devkitPro MSYS2 shell and the Windows Dolphin helpers in `.dev/`. Keep native `TMP` and `TEMP` paths for the linker.
- Read only the latest boot segment in Dolphin's append-only log. A null access is a guest fault to trace against the matching ELF; a successful build does not make it harmless.

# Learned lessons (append, do not delete)

- Chains and `perf.log` need a profiling build: `.dev/build_profiling.sh <target>` sets `-DPERF_PROF` and cleans itself. A release build never writes `perf.log`, so `chain=` runs boot but record nothing and the test script waits the full timeout.
- Pass `diag.cfg` lines as extra args: `.dev/dolphin_test.sh <dol> <wait> "audio_quality=hifi" "chain=3600 sd:/wii64/roms/Foo.v64"`. `autoboot_rom=` works in release builds; `chain=` completion needs the profiling build.
- The VI limiter caps Dolphin speed at 1.0. Compare idle percent (`sleep_us`/wall), never speed. Identical overruns across modes in Dolphin are boot-fill artifacts; confirm audio pressure on Wii via `dsp_avg`, `dsp_peak`, and true underruns.
- Menu frame arrays must match: `NUM_FRAME_TEXTBOXES`/`NUM_FRAME_BUTTONS` must equal the initializer count or the constructor reads out of bounds.
- Version bump touches 5 files: `menu/MainFrame.cpp` (`VERSION`) plus `release/apps/*/meta.xml` (`<version>`, `<release_date>` as `%Y%m%d%H%M`, and the `long_description` line).
- Keep agent plans and handoffs in `agent-share/` (git-ignored). Never commit it.
- End of conversation: bump version, rebuild, commit on the work branch, and start Dolphin interactive with the new build for the user to test.
- local-llm MCP lives in `~/.config/opencode/opencode.jsonc` (SSH stdio to ai-server-codex). Skills live in `~/.config/opencode/skills/<name>/SKILL.md`. Newly installed skills load on next session start.
- Menu draw calls run every frame per widget. Guard repeat work on unchanged input (see `Box3D::setTexture`). Never allocate per frame in a draw path.
- Cleanup rule: fix broken code over removing it. Remove only what is proven dead (zero callers) or explicitly stubbed out.
