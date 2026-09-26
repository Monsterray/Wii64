# Wii64 agent guidance

For build, Dolphin, controller, or performance work, read [the Wii64 build-and-test skill](.agents/skills/wii64-build-and-test/SKILL.md). Use [README.md](README.md) and [the macOS setup notes](doc/macos-development-setup.md) for contributor instructions.

- Keep libogc2 and libfat in separate source checkouts. Build them with the devkitPPC used for Wii64.
- Use `.dev/env.sh` and `.dev/build.sh <target>`. Clean when you change targets, toolchains, or build flags because targets share object files.
- Treat toolchain pins as host-specific: the tested Windows setup uses r41-2; this Intel Mac builds with r50-1. The Wii makefile accepts both tested libogc2 layouts.
- Test the new `.dol`, not only the link result. Use an isolated Dolphin profile. Stage ROMs before launch and keep ROMs, logs, screenshots, and profiles out of Git.
- On macOS, `.dev/dolphin_test.sh wii64-glN64.dol interactive` prepares the profile and starts Dolphin. The script copies the Mac Wii Remote mapping, passes an absolute profile path, and enables MMU and SD folder sync.
- On Windows, use the devkitPro MSYS2 shell and the Windows Dolphin helpers in `.dev/`. Keep native `TMP` and `TEMP` paths for the linker.
- Read only the latest boot segment in Dolphin's append-only log. A null access is a guest fault to trace against the matching ELF; a successful build does not make it harmless.
