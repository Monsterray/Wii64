# Wii64 development setup

This is the smallest supported local loop for building the Wii targets and launching the resulting DOL in Dolphin on Intel macOS.

## One-time installation

The machine already has Apple Command Line Tools, Homebrew, and Dolphin `2606a` at `/Applications/Dolphin.app`.

1. Install the official [devkitPro pacman installer](https://github.com/devkitPro/pacman/releases/latest). The installer used for this setup is `v6.0.2`; its SHA-256 is `da74156211cd3d8664ba7fb95544fbc0a2fbef1dd87b1a6bfafb30d2d446cd30`.
2. Log out/in, or load `/etc/profile.d/devkit-env.sh` in the shell.
3. Install the Wii toolchain and the portlib dependency:

   ```sh
   sudo dkp-pacman -Syu
   sudo dkp-pacman -S wii-dev ppc-zlib
   ```

Wii64’s makefiles explicitly use `$(DEVKITPRO)/libogc2/wii/{include,lib}`. `libogc2` is not present in the standard devkitPro repositories used here. It must be paired with the legacy libfat source: the newer package is built for standard libogc and references an ABI symbol that libogc2 intentionally does not provide.

Build the two sources in a directory with no spaces. Wii64 itself may remain in a path with spaces; this restriction applies only to the legacy libfat build:

   ```sh
   export DEVKITPRO=/opt/devkitpro
   export DEVKITPPC="$DEVKITPRO/devkitPPC"
   export PATH="$DEVKITPRO/tools/bin:$DEVKITPPC/bin:$PATH"

   mkdir -p "$HOME/devkitpro-src"
   cd "$HOME/devkitpro-src"
   git clone https://github.com/extremscorner/libogc2.git
   git clone https://github.com/extremscorner/libfat.git

   cd libogc2
   make wii
   sudo -E make install

   cd ../libfat
   make -C libogc2 PLATFORM=wii BUILD=wii_release
   sudo -E make -C libogc2 install
   ```

The local `libogc2/` checkout contains a path-space fix on branch `fix-spaced-paths` at commit `bf4e1e9`. It covers GNU Make’s `include`/VPATH parsing, recursive `-C`/`-f` calls, shell path quoting, DSP prerequisites, and install/clean destinations. Both Wii and GameCube builds pass from this spaced workspace on macOS GNU Make 3.81.

## Verify the toolchain

```sh
test -n "$DEVKITPRO" && test -n "$DEVKITPPC"
command -v powerpc-eabi-gcc
command -v powerpc-eabi-g++
command -v elf2dol
test -f "$DEVKITPRO/libogc2/wii/include/ogcsys.h"
test -f "$DEVKITPRO/libogc2/wii/lib/libogc.a"
test -f "$DEVKITPRO/libogc2/wii/include/fat.h"
test -f "$DEVKITPRO/libogc2/wii/lib/libfat.a"
test -f "$DEVKITPRO/portlibs/ppc/include/zlib.h"
```

## Build

From the repository root:

```sh
make -f Makefile.glN64_wii -j"$(sysctl -n hw.ncpu)"
```

This produces `wii64-glN64.dol`. The alternative renderer is:

```sh
make -f Makefile.Rice_wii -j"$(sysctl -n hw.ncpu)"
```

Use `make -f Makefile.glN64_wii clean` between toolchain/configuration experiments. Keep profile builds separate by adding `-DPROFILE` through `DEBUG_FLAGS`; never use a profile build for performance numbers.

## Launch in Dolphin

Dolphin’s DSP HLE does not recognize Wii64’s homebrew audio microcode. Launch with DSP LLE for this target:

```sh
/Applications/Dolphin.app/Contents/MacOS/Dolphin --audio_emulation LLE --exec="$PWD/wii64-glN64.dol"
```

For an automated smoke run, add `--batch`. Dolphin’s log window/console is the place to inspect startup failures; a DOL that depends on real Wii devices or IOS behavior may still boot differently in Dolphin.

Do not place ROMs, BIOS files, save files, or copyrighted captures in this repository. Keep test inputs in a private, legally sourced directory and record only the title/region/revision/CRC in benchmark notes.

## Current state

The repository is on local branch `reconnaissance`, based on Wii64 `9eb19ee…`, with `origin` set to `git@github.com:Monsterray/Wii64.git`. Dolphin and the base devkitPPC toolchain are installed. The remaining system-wide step is installing the locally built compatible `libfat.a` alongside libogc2.
