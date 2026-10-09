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
 * For SD/USB: Put ROMs in the directory named /wii64/roms, or in any folder: the ROM browser
    shows one folder at a time (folders first), B goes up a folder and leaves at the card's root,
    and it opens in the folder of the last ROM you loaded (kept in /wii64/romdir.txt).
    All save types will automatically be placed in /wii64/saves
 * Project64 saves (`GAME.eep`, `.sra`, `.fla`, `.mpk` in /wii64/saves) are imported the first
    time the game loads: Wii64 writes `GAME(U).eep` (with the ROM's region) and keeps the original
    as `GAME.eep.pj64`. `python3 scripts/n64_saves.py SAVES ROMS` converts a folder on a PC.
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
	* CPU Clock Divider: N64 cycles for each instruction (1, 2 or 3); more is faster on the Wii but can slow some games
	* Start Menu: Start in Basic (the mini menu with boxart) or Advanced (the full menu); applies at the next start
	* Save Settings: Write settings.ini to SD or USB (see "Settings file" below)
* Video
	* Show FPS: Display the framerate in the top-left corner of the screen
	* Screen Mode: Select the aspect ratio of the display; 'Force 16:9' will pillar-box the in-game display
	* CPU Framebuffer: Enable for games which only draw directly to the framebuffer (this will only need to be set for some homebrew demos)
	* 2xSaI Tex: Scale and Interpolate in-game textures (unstable on GC, not supported in Rice GFX)
	* FB Textures: Enable framebuffer textures (necessary for some games to render everything correctly (e.g. Zelda Subscreen), but can impact performance; unstable on GC, not supported in Rice GFX)
	* Enable 240p: 240p output for games that use it
	* Video Mode: The TV signal: Auto (the Wii's settings), 480i 60 Hz, 240p, 480p, 576i 50 Hz, 288p or 576p (applies at the next start)
* Input
	* Configure Input: Select controllers to use in game
	* Configure Paks: Press A to change the pak in each controller: Controller Pak (saves), Rumble Pak, Transfer Pak, Bio Sensor (Tetris 64) or None. With a Transfer Pak, the button on the right chooses the Game Boy cartridge: put .gb and .gbc files (not zipped) in `wii64/gb`. The cartridge's save is the same name with .sav, as in Game Boy emulators on a PC, and Wii64 writes it each time you return to the menu. Settings > Save Settings keeps the choice (`Pak1` = 4, `TransferPak1`); a game's `settings/<game code>.ini` can give it its own cartridge ([details](doc/transfer-pak-plan.md))
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

### Settings file (settings.ini)
Settings > General > Save Settings writes `wii64/settings.ini` on SD or USB.
Wii64 reads it at start, from the device it was started from (SD first, then
USB). Above each key, a comment tells its values, so you can edit the file on
a PC. Wii64 ignores an unknown key or a value out of range, and keeps the
default for a key that is not in the file. Wii64 1.6 kept the same keys in
`settings.cfg`. Without a `settings.ini`, Wii64 reads `settings.cfg`, and the
next Save Settings writes `settings.ini`.

**Settings for one game.** Put the keys marked "(per game)" in
`wii64/settings/<game code>.ini`. Current ROM shows the game code (for example
NSME for Super Mario 64, USA). When that game starts, its keys replace the
global settings. When you load a different game, Wii64 restores them. For
example, `wii64/settings/NSME.ini`:

```ini
; Super Mario 64 (USA)
LimitVIs = 2
FBTex = 1
```

Save Settings always writes the global values, also while a game uses its own file.

**Launch arguments.** wiiload or the `<arguments>` of meta.xml can give
`key=value` lines with the same keys, for example `LimitVIs=1`. They apply
after settings.ini.

For the audio keys, see [Audio settings](doc/audio-settings.md). Hi-Fi and
Preserve Pitch are optional CPU enhancements.

## CREDITS
 * Author and maintainer since 1.7.0: Monty Perrotti
 * Core Coder: tehpola
 * Graphics & Menu Coder: sepp256
 * General Coder & maintainer before 1.7.0: emu_kidid
 * Original mupen64: Hactarux
 * [Not64](https://github.com/extremscorner/not64) /[libogc2](https://github.com/extremscorner/libogc2): [Extrems](extremscorner.org)
 * WiiVC/DRG stuff: [FIX94](https://github.com/FIX94/)
 * Artwork: drmr
 * Wii64 Demo ROM: marshallh
 * Compiled using [devKitPro](https://devkitpro.org/) and [libogc2](https://github.com/extremscorner/libogc2))
 * [GLideN64](https://github.com/gonetz/GLideN64/) gonetz and others
 * Visit the official code repo on [GitHub](https://github.com/emukidid/Wii64)
