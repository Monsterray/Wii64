# README : Wii64 / Cube64

> LICENSE:
>     This software is licensed under the GNU General Public License v2
>       which is available at: http://www.gnu.org/licenses/gpl-2.0.txt
>     This requires any released modifications to be licensed similarly,
>       and to have the source available.
 

**Wii64/Cube64 and their respective logos are trademarks of Team Wii64 and should not be used in unofficial builds.**

## QUICK USAGE:
 * On SD/USB, use uncompressed N64 ROM images (`.z64`, `.v64`, `.n64`, `.bin`, or `.rom`). Wii64 does not open ROMs inside `.zip` or `.7z` archives.
 * To install: Extract the contents of the latest release zip to the root of your SD card
 * For SD/USB: Put ROMs in the directory named /wii64/roms,
    All save types will automatically be placed in /wii64/saves
 * For DVD: ROMs may be anywhere on the disc (requires a compatible Wii)
 * Load the desired executable from the HBC or in the loader of your choice, the emulator is shipped with 2 graphics plugins now.
	 * **Rice GFX Plugin version** 
		 * Slightly faster and has fixed sky boxes in games but isn't as refined as glN64
		 * Experimental support for hi-res texture paks via Wii64 specific [texture packer](https://github.com/emukidid/Wii64/releases/tag/texturepacker_1.2)
	 * **glN64 GFX Plugin version**
		 * Slightly slower than Rice GFX on certain games
		 * FrameBuffer texture support (OoT subscreen)
		 * Optional 2xSaI texture filtering
		 * Less buggy, more progressed port (e.g. emulates fog)
 * Once loaded, select 'New ROM' and select the ROM to load and it will automatically start
 * The game can be exited any time by pressing X and Y together on a GC pad or Classic Controller,
   1 and 2 together on a Wiimote (only with Nunchuck attached), or the reset button
     (Note: this must be done to save your game; it will not be done automatically)
 * On a Wii, HOME on a Wii Remote or Classic Controller opens the HOME menu, in a game and in
   Wii64's menus. In a game it pauses emulation; close it with HOME or B to carry on. Its
   **Wii64** button shows the game, speed, video plugin and CPU core, and goes to Wii64's
   own menu (save states, settings). **Exit** returns to the Homebrew Channel, the System
   Menu, or restarts or powers off the Wii; **Shot** saves a screenshot to sd:/screenshots.

### Controls:
* Controls are fully configurable so any button on your controller can be mapped
* The controller configuration screen presents each N64 button and allows you to toggle through sources
* There are 4 configuration slots for each type of controller
	* To load a different, previously saved configuration, select the slot, and click 'Load'
	* After configuring the controls as desired, select the slot, and click 'Save'
	* After saving different configurations to the slots, be sure to save your configs in the input tab of the settings frame
* Clicking 'Next Pad' will cycle through the N64 controllers assigned
* There is an option to invert the Y axis of the N64's analog stick; by default this is 'Normal Y'
* The 'Menu Combo' configuration allows you to select a button combination to return to the menu

### Settings:
* General
	* Native Saves Device: Choose where to load and save native game saves
	* Save States Device: Choose where to load and save save states
	* Select CPU Core: Choose whether to play games with pure interpreter (better compatibility) or dynarec (better speed)
	* Save settings.cfg: Save all of these settings either SD or USB (to be loaded automatically next time)
* Video
	* Show FPS: Display the framerate in the top-left corner of the screen
	* Screen Mode: Select the aspect ratio of the display; 'Force 16:9' will pillar-box the in-game display
	* CPU Framebuffer: Enable for games which only draw directly to the framebuffer (this will only need to be set for some homebrew demos)
	* 2xSaI Tex: Scale and Interpolate in-game textures (unstable on GC, not supported in Rice GFX)
	* FB Textures: Enable framebuffer textures (necessary for some games to render everything correctly (e.g. Zelda Subscreen), but can impact performance; unstable on GC, not supported in Rice GFX)
* Input
	* Configure Input: Select controllers to use in game
	* Configure Paks: Select which controller paks to use in which controllers
	* Configure Buttons: Enter the controller configuration screen described above
	* Save Button Configs: Save all of the controller configuration slots to SD or USB
	* Auto Load Slot: Select which slot to automatically be loaded for each type of controller
* Audio
	* Audio: Select On or Off
* Saves
	* Auto Save Native Saves: When enabled, the emulator will automatically load
     saves from the selected device on ROM load and save when returning to the menu or
     turning off the console

## ADVANCED USAGE
### GameCube Version(s)
Wii64 also exists as Cube64, a version of the emulator with the same UI and features albeit with tighter memory restrictions and less CPU power availble. This version has significantly less memory available and requires paging of ROM data from your storage medium into a small ARAM cache and then into main memory, this is denoted by every time you see a cartridge icon in the top right hand corner of the screen. GameCube builds don't have boxart support and the mini menu.

There's also 2 versions per graphics plugin shipped (4 .dol files total). 

The "-exp.dol" versions support the expansion pak by reducing certain cache sizes, this should only really be used for expansion pak required games as it will likely thrash more often due to less memory being available and cause slowdown that could be avoided by using the non "exp" version.

### "WiiVC" Version
This version isn't really supported much due to the niche nature of it. It enables DRC (Wii U GamePad) support, and also takes advantage of unlocked CPU multiplier support if enabled. The gist of it is that you can run Wii64 on a Wii U in "Wii mode" but with Wii U Game Pad support and (optionally) the CPU multiplier unlocked. To boot this version you can either just use the vWii mode to use the GamePad, or to unlock the CPU multiplier you'll need to be well versed in Wii U homebrew setups (essentially there's a process that exists to inject homebrew into a WiiVC title - e.g. Wii titles were available on the Wii U via the eShop) and there's a thing called [sign_c2w_patcher](https://github.com/FIX94/sign_c2w_patcher) that you must boot before loading this title to unlock the CPU multiplier. This version is shipped as "Wii64 | Rice GFX | WiiVC" and "Wii64 | glN64 GFX | WiiVC" in the archive.

### Boot time arguments
The following can be passed in via wiiload or by editing the meta.xml to override settings. They can also be changed via the settings.cfg that's created upon booting up the emulator for the first time.

For synthesis/output resampling, mixer precision, latency, and synchronization,
see [Audio settings](doc/audio-settings.md). Save menu changes with General,
Save Settings. Hi-Fi and Preserve Pitch are optional CPU enhancements.

* **MiniMenu** - Which menu style should the emulator default to.
	 * 0 = Don't boot to Mini Menu
	 * 1 = Boot to mini menu (default)
* **Audio** - Audio toggle
	 * 0 = Disabled
	 * 1 = Enabled (default)

 * **FPS** - FPS display toggle
	 * 0 = Disabled (default)
	 * 1 = Enabled
 * **FBTex** - FrameBuffer textures for glN64 (e.g. OoT pause screen)
	 * 0 = Disabled (default)
	 * 1 = Enabled
 * **2xSaI** - 2xSaI texture upscaling for glN64
	 * 0 = Disabled (default)
	 * 1 = Enabled
 * **ScreenMode**
	 * 0 = 4:3
	 * 1 = 16:9
	 * 2 = 16:9 Pillar box
 * **VideoMode**
	 * 0 = VIDEOMODE_AUTO (default)
	 * 1 = VIDEOMODE_PAL60
	 * 2 = VIDEOMODE_240P
	 * 3 = VIDEOMODE_480P
	 * 4 = VIDEOMODE_PAL
	 * 5 = VIDEOMODE_288P
	 * 6 = VIDEOMODE_576P
 * **Core**
	 * 0 = Pure Interpreter
	 * 1 = Dynamic Recompiler (default)
 * **CountPerOp**
	 * 0 = 1 per Op (default for WiiVC)
	 * 1 = 2 per Op (default for Wii)
	 * 2 = 3 per Op (default for GameCube)
 * **NativeDevice** - Which device to use for Native (SRAM/FlashRAM/EEPROM) saves
	 * 0 = SD
	 * 1 = USB
	 * 2 = Memory Card A
	 * 3 = Memory Card B
 *  **StatesDevice** - Which device to use for Save States
	 * 0 = SD
	 * 1 = USB
 * **AutoSave** - Whether or not to automatically save native saves when returning to the menu
	 * 0 = Disabled
	 * 1 = Enabled (default)
 * **LimitVIs** - How to cap emulation speed
	 * 0 = No VI limit
	 * 1 = Wait for VI (default)
	 * 2 = Wait for Frame
 * **Pak1 / Pak2 / Pak3 / Pak4** - What's inserted in each Controller Pak slot
	 * 0 = Memory Pak (default)
	 * 1 = Rumble Pak
 * **LoadButtonSlot** - Which slot to load button mappings from
	 * 0 to 3 = Button slots 0 to 3
	 * 4 = Default

## COMPATIBILITY
Report and view any open issues on the [issue tracker](https://github.com/emukidid/Wii64/issues).

## BUILDING FROM SOURCE
Install [devkitPro](https://devkitpro.org/wiki/Getting_Started) and its Wii development tools. Keep [libogc2](https://github.com/extremscorner/libogc2) and [libfat](https://github.com/extremscorner/libfat) in separate checkouts and build/install them with the same devkitPPC toolchain as Wii64. Wii builds also link [HBC-Reborn](https://github.com/Monsterray/hbc-reborn)'s agent, the HOME menu: clone it beside this repository and run `.dev/build_agent.sh`, which builds its library with the same toolchain into `.dev/hbc_agent/`. Set `DEVKITPRO`, `DEVKITPPC`, and `PATH` for that installation; `PATH` must include `$(DEVKITPRO)/tools/bin` and `$(DEVKITPPC)/bin`.

On Windows:

1. Install [devkitPro](https://github.com/devkitPro/installer/releases/latest) with "Wii Development" checked, at `C:\devkitPro`. Install Git for Windows and Python 3.
2. Clone this repository to a path without spaces, for example `C:\projects\Wii64`.
3. From devkitPro's MSYS2 shell or Git Bash, run `.dev/setup-devkitpro-windows.sh` once. It clones libogc2, libfat and HBC-Reborn beside this repository, builds libogc2 and libfat with devkitPro's current devkitPPC and installs them into `C:\devkitPro\wii64-sdk`, then builds the HBC agent library (`.dev/build_agent.sh`). It does not change `C:\devkitPro\libogc2`, so other projects that use that copy are not affected. Run it again after you update any of the checkouts.
4. Build with `.dev/build.sh glN64_wii`, or with `make -f Makefile.glN64_wii` as shown below. The makefiles use `C:\devkitPro\wii64-sdk` when it exists. When it does not exist, they stop and tell you to run the setup script.

On macOS, see [the Intel Mac development notes](doc/macos-development-setup.md).

Build any target with the matching makefile, for example:

```
make -f Makefile.glN64_wii
```

Other targets: `Makefile.glN64_gc[_exp]`, `Makefile.glN64_wiivc`, `Makefile.Rice_wii`, `Makefile.Rice_gc[_exp]`, and `Makefile.Rice_wiivc`. The Wii makefile recognizes both `libogc2/include` + `libogc2/lib/wii` and `libogc2/wii/include` + `libogc2/wii/lib` layouts. Clean before switching targets or toolchains because object files are built beside their sources.

For local macOS builds and Dolphin smoke tests, source `.dev/env.sh`, then use `.dev/build.sh glN64_wii` and `.dev/dolphin_test.sh wii64-glN64.dol`. The scripts use an isolated Dolphin profile and stage ROMs from `~/Library/Application Support/Dolphin/Load/WiiSDSync/wii64/roms`; set `WII64_ROM_DIR` to use a different ROM folder. On Windows, ROMs come from `C:\tools\Dolphin-x64\User\Load\WiiSDSync\wii64\roms` (or `WII64_ROM_DIR`), and Dolphin is `C:\Tools\Dolphin-x64\Dolphin.exe` unless `DOLPHIN_EXE` says otherwise. Dolphin runs with MMU emulation on: ROMs larger than 16 MB page through address-translation faults and do not boot without it. It also stores XFB copies in emulated RAM and presents them at VI scan (`XFBToTextureEnable=False`, `ImmediateXFBEnable=False`), as a Wii does; otherwise the HOME menu, which the CPU draws, never shows, and chain screenshots read stale memory.

For audio changes, run `.dev/test_rsp_audio.sh` with a host C compiler that supports AddressSanitizer and UndefinedBehaviorSanitizer. It checks synthesis, output streaming, queue boundaries, settings, and diagnostics. See [the optimization handoff](doc/optimization-handoff.md) for current audio status and test-profile storage limits.

For a real Wii, see [hardware testing](doc/hardware-session.md). Set up the shared Wii queue client and SD test chain once. `.dev/hardware_baseline.sh` waits for the central lease, builds a profiling DOL, sends it from Homebrew Channel with `wiiload`, collects the results, and files a baseline. SD results remain available if transfer fails. Use `.dev/profile_subsystems.sh` for a matched subsystem probe/control survey.

For a ROM-library smoke test on Dolphin and the Wii, see [library testing](doc/library-testing.md). It records coverage, VI speed, video/audio activity and stutter candidates without muting the guest sound engine.

Wii builds prepare large ROMs before gameplay to reduce paging stalls. This can increase load time. For the HBC agent (HOME menu, crash reports, file transfer) and matched paging comparisons, see [the agent and MEM2 notes](doc/rom-paging-and-agent.md).

## CREDITS
 * Core Coder: tehpola
 * Graphics & Menu Coder: sepp256
 * General Coder & current maintainer: emu_kidid
 * Original mupen64: Hactarux
 * [Not64](https://github.com/extremscorner/not64) /[libogc2](https://github.com/extremscorner/libogc2): [Extrems](extremscorner.org)
 * WiiVC/DRG stuff: [FIX94](https://github.com/FIX94/)
 * Artwork: drmr
 * Wii64 Demo ROM: marshallh
 * Compiled using [devKitPro](https://devkitpro.org/) and [libogc2](https://github.com/extremscorner/libogc2))
 * [GLideN64](https://github.com/gonetz/GLideN64/) gonetz and others
 * Visit the official code repo on [GitHub](https://github.com/emukidid/Wii64)
