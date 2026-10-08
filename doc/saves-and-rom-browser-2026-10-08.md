# Saves from Project64, and the ROM browser, 2026-10-08

## Save files

Wii64 names a native save `GOODNAME(REGION).ext`: the ROM header's internal
name (20 bytes, trailing spaces cut, `\/:*?"<>|` made `_`) and the region of
its country code, for example `SUPER MARIO 64(U).eep`. Project64 uses the same
internal name without the region, so `SUPER MARIO 64.eep`.

Sizes Wii64 writes:

| Type | File | Bytes | Why |
|---|---|---:|---|
| EEPROM | `.eep` | 2,048 | Carts have 4 Kbit (512 bytes) or 16 Kbit (2,048) EEPROM. Wii64 keeps one 2,048-byte buffer and tells the game its size (`isEEPROM16k`, a ROM table); a 4 Kbit game uses only the first 512 bytes. |
| SRAM | `.sra` | 32,768 | 256 Kbit SRAM. |
| FlashRAM | `.fla` | 131,072 | 1 Mbit FlashRAM. |
| Controller Pak | `.mpk` | 131,072 | Four 32 KB paks. |

Before this change the loaders accepted only a file of that full size and
started with a blank save otherwise, so a 512-byte EEPROM did not load.

Project64 differences, checked on the production card's saves:

- SRAM and FlashRAM are stored as little-endian 32-bit words. OoT's save
  header reads `DLEZ…` at offset 60 in Project64's `THE LEGEND OF ZELDA.sra`
  and `ZELDA…` in Wii64's; Paper Mario's FlashRAM starts `iraM`.
- Project64 stops some files after the last byte the game wrote
  (`MarioParty3.eep` 1,384 bytes, `Kirby64.eep` 280, `.fla` 98–128 KB).
- EEPROM and Controller Pak bytes are in the same order in both.

`loadSaveFile` (`main/rom_gc.c`) now does this for all four types: read the
Wii64 file if it exists; else, if `GOODNAME.ext` exists and is not empty,
read it, swap SRAM/FlashRAM words, write the full-size Wii64 file and rename
the original to `GOODNAME.ext.pj64`. A short file keeps the blank save after
its end. A save under the Wii64 name always wins, so the production card's
`MarioParty3(U).eep` stays in use and `MarioParty3.eep` is not touched.

`scripts/n64_saves.py SAVES ROMS [--out DIR] [--prefer U]` does the same on
a PC, reading the internal names and regions from the ROMs; it never deletes
or replaces a file (`tests/n64_saves_test.py`).

Checked on the bench Wii with copies of the production saves: Project64's
`SUPER MARIO 64.eep` (512 bytes) and `ZELDA MASTER QUEST.sra` became
`SUPER MARIO 64(U).eep` (2,048) and `ZELDA MASTER QUEST(U).sra`, byte for byte
the converter's output, and the originals stayed as `.pj64`. OoT MQ's file
select showed file 1 ("Monster") and the replay continued it; SM64's file A
had its stars. The bench's save folder was restored afterwards.

## ROM browser

Both browsers (the boxart grid from the Mini Menu, `menu/SelectRomFrame.cpp`,
and Load ROM's list, `menu/FileBrowserFrame.cpp`) used to list every ROM under
`sd:/wii64/roms` recursively, with no way to go up. On SD and USB they now:

- list one folder: folders first, then ROMs, each sorted by name; names
  starting with `.` are hidden (macOS writes `._Game.z64` beside each ROM);
  a `.bin` file is listed only if it starts like an N64 ROM;
- show the folder's path at the top, and a `..` entry first below the root;
- go up a folder with B, and leave the browser at the card's root;
- open in the folder of the last ROM loaded, kept in `sd:/wii64/romdir.txt`
  (or `usb:/`), not in `settings.cfg`, which would also save settings the
  user did not choose to save.

The DVD keeps its flat list, and B leaves it. Path rules:
`tests/browser_path_test.py`.
