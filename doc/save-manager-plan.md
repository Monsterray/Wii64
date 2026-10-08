# Save manager plan

Status: plan only (2026-10-08). No code yet. Settings > Saves > Copy Saves and Delete
Saves show "not implemented" until phase 3 replaces them.

## Goal

One screen shows all saves on SD and USB. Each save has an icon for its type. Select a
save to get three actions: Copy, Move and Delete. For Controller Pak saves, the screen
also shows the notes in each pak, so you can move a note from one pak to a different pak.

## What exists now

| Save | File | Written by |
| --- | --- | --- |
| EEPROM | `wii64/saves/GOODNAME(REGION).eep` | `gc_memory/pif.c` |
| SRAM | `wii64/saves/GOODNAME(REGION).sra` | `gc_memory/dma.c` |
| FlashRAM | `wii64/saves/GOODNAME(REGION).fla` | `gc_memory/flashram.c` |
| Controller Pak | `wii64/saves/GOODNAME(REGION).mpk`: four 32 KB paks, ports 1 to 4 | `gc_memory/pif.c` |
| Save state | `wii64/saves/GOODNAME(REGION).st0` to `.st9` | `main/savestates_gc.c` |

- `loadSaveFile` (main/rom_gc.c) also reads Project64 names and byte orders, and keeps
  the Project64 original as `.pj64`.
- Current ROM > Delete Save Game deletes the native saves of the loaded game only (it
  asks first).
- A Controller Pak file belongs to one game, but a real pak holds notes of many games.
  This is why moving notes between paks is useful.

## Controller Pak format (what phase 4 reads)

- 128 pages of 256 bytes. Page 0: ID block (with checksums). Pages 1 and 2: the index
  table (inode table) and its copy. Page 3 and 4: 16 note entries of 32 bytes each.
- A note entry: game code (4 bytes), publisher (2 bytes), first page, status, name (16
  characters in the N64 character set) and extension.
- A note uses a chain of pages in the index table. A copy needs enough free pages and a
  new chain; then the entry goes into a free note slot.

## Phases

1. **Data layer, no UI** (`main/saves.c`, host tests in `tests/saves_test.c`).
   - List the saves in a folder: game name, region, type, size, device.
   - Parse a `.mpk` file: the notes of each of the four paks, free pages, and a check of
     the ID block and index table.
   - Operations: copy, move and delete a file; copy, move and delete a note. An operation
     never overwrites a file or a note without a confirmation from the caller. It writes
     to a temporary file first and renames it when the write is complete.
2. **List screen, read only** (`menu/SaveManagerFrame.cpp`).
   - Replaces the Copy Saves button. Same paging and B-goes-up behaviour as the ROM
     browser (`menu/SelectRomFrame.cpp`).
   - One row for each save: icon, game name, type, device. Icons: EEPROM, SRAM,
     FlashRAM, Controller Pak, save state (new images in `menu/resources`).
   - Select a Controller Pak save to see its four paks and their notes.
3. **File actions.** Copy (SD to USB, USB to SD), Move, Delete, with a Yes/No question for
   each. The Delete Saves button opens this screen. Keep Current ROM > Delete Save Game.
4. **Note actions.** Copy or move a note to a different pak (another port, or another
   game's `.mpk`), and delete a note. Show the free pages before a copy.
5. **Hardware test.** Bench Wii first, with copies of real saves. Back up the production
   Wii's `wii64/saves` before you install a build with these actions.

## Rules for the implementation

- Never change a save of the game that runs now. Open the save manager only from the
  menu, with emulation stopped.
- Nothing from an interrupt touches SD or USB (AGENTS.md).
- The data layer has no libogc calls, so the host tests run it (like `main/settings.c`).
