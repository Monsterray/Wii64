# Transfer Pak plan

Status (2026-10-08): phases 1 and 2 are done. `gc_memory/gb_cart.c` (the cartridge) and
`gc_memory/transfer_pak.c` (the pak: protocol, `tpak_insert`, `tpak_save`) are in the Wii
build, tested by `tests/gb_cart_test.c` and `tests/transfer_pak_test.c` in
`.dev/test_subsystems.sh`. `gc_memory/pif.c` answers `PLUGIN_TANSFER_PAK` with
`transferPaks[4]`, but nothing selects that plugin until phase 3. Configure Paks cycles
Controller Pak, Rumble Pak, Bio Sensor and None; Transfer Pak becomes a fifth choice in
phase 3.

Phase 3 also has to:
- call `tpak_insert` for each port with a Transfer Pak when a game starts (and
  `tpak_eject` when it stops), and show its error (no file, unknown type, a type for a
  later phase) in a message;
- call `tpak_save` for every port wherever the native saves are written: the five
  `saveMempak` call sites in `menu/` (MainFrame, MiniMenuFrame, CurrentRomFrame,
  SaveGameFrame twice). Their `if (...Written)` checks do not see the cartridge RAM, so
  check `transferPaks[i].cart.ram_dirty` too;
- send `cart.rumble` (MBC5 rumble cartridges) to the controller's rumble.

Phase 2 choices from the sources:
- Status bits and the 0xB000 write (1 access on, 0 off) as libdragon; the reset bits
  step as mupen64plus (3 to 2 after an access write; 2 to 1 to 0 without one).
- A read with the pak off, or with no access, gives zeros.

Phase 1 choices to check against the games in phase 4:
- A write to a clock register changes the live counter only; the latched copy changes at
  the next latch (as mGBA).
- Unused clock register bits read 0.
- New cartridge RAM reads 0xFF until a .sav loads.

## What a Transfer Pak is

A Transfer Pak holds a Game Boy or Game Boy Color cartridge. The N64 game reads and
writes the cartridge's ROM, RAM and clock through the pak. The N64 game runs any Game Boy
code itself (Pokemon Stadium's GB Tower is a Game Boy emulator in the N64 game).

So Wii64 needs no Game Boy CPU. It needs an accurate **cartridge**: the mapper (MBC) that
switches ROM and RAM banks, the battery RAM, and the real-time clock of some cartridges.
This is the part to copy from the accurate Game Boy emulators.

Game Boy Advance cartridges do not work in a real Transfer Pak (different bus and
voltage), and no N64 game reads one. "Any Game Boy ROM" means every .gb and .gbc
cartridge type.

## Games

| Game | Uses |
| --- | --- |
| Pokemon Stadium | Red, Blue, Yellow (and Green, J): save RAM; whole ROM for the GB Tower |
| Pokemon Stadium 2 | Gold, Silver, Crystal: save RAM and the MBC3 clock; GB Tower |
| Mario Golf | Mario Golf GBC: save RAM (characters) |
| Mario Tennis | Mario Tennis GBC: save RAM (characters) |
| Perfect Dark | Perfect Dark GBC (cheats); Game Boy Camera (face pictures) |
| Mario Artist: Talent Studio (64DD) | Game Boy Camera |

## Protocol (from mupen64plus `transferpak.c`)

The pak uses the controller pak commands (read 0x02, write 0x03, 32 bytes, address in
32-byte steps), in the same place as `gc_memory/pif.c` handles the other paks.

| Address | Read | Write |
| --- | --- | --- |
| 0x8000-0x8FFF | 0x84 when on, 0x00 when off | 0x84 turns on (bank 3, cartridge off); 0xFE turns off |
| 0xA000-0xAFFF | - | bank: 0 to 3 (larger values select 0) |
| 0xB000-0xBFFF | status: bit 0 cartridge on, bits 2-3 reset state, bit 7 pak on | access mode: cartridge on |
| 0xC000-0xFFFF | cartridge, at `(address & 0x3FFF) + bank * 0x4000` | cartridge, same address |

- The reset state starts at 3 and steps 3 to 2 to 1 to 0 on status reads (see
  `read_transferpak`). Games wait for these steps after they insert or turn on the pak.
- mupen64plus says its pak is "certainly NOT accurate". Check the reset steps against
  Pokemon Stadium 1 and 2 first; they check the most.

## Cartridge layer: what to take from which emulator

| Source | Take |
| --- | --- |
| [Pan Docs](https://gbdev.io/pandocs/) | The specification: header (0x134-0x14F), every MBC, RTC registers |
| SameBoy | The most accurate MBC behaviour, the RTC, and the save file with the RTC footer |
| Gambatte | MBC1 and MBC3 edge cases, tested against hardware |
| mGBA | The widest mapper set (MBC6, MBC7, HuC1, HuC3, TAMA5, Pocket Camera) and its save formats |
| mupen64plus `gb_cart.c` | How an N64 emulator joins the cartridge to the pak |

Licences: SameBoy is MIT (code can be adapted). mGBA is MPL 2.0, Gambatte and mupen64plus
are GPL 2. Wii64 is GPL 2, so all can be used; keep their copyright notices on copied code.

### Mappers (header byte 0x147)

| Phase | Mapper | Header values | Notes |
| --- | --- | --- | --- |
| 1 | None | 0x00, 0x08, 0x09 | 32 KB ROM, optional 8 KB RAM |
| 1 | MBC1 | 0x01-0x03 | Banks 0x20/0x40/0x60 select 0x21/0x41/0x61; mode 1; MBC1M multicarts (detect by logo at 0x40104) |
| 1 | MBC2 | 0x05, 0x06 | 512 x 4-bit RAM; address bit 8 selects RAM enable or ROM bank |
| 1 | MBC3 | 0x0F-0x13 | RTC on 0x0F, 0x10; MBC30 (Japanese Crystal): 8 RAM banks, 7-bit ROM bank |
| 1 | MBC5 | 0x19-0x1E | 9-bit ROM bank, bank 0 allowed; rumble types: send the motor bit to the Wii controller's rumble |
| 2 | HuC1 | 0xFF | Infrared register reads "no light" |
| 2 | HuC3 | 0xFE | RTC (different from MBC3) |
| 2 | MMM01 | 0x0B-0x0D | Multicart; the menu ROM is at the end of the image |
| 2 | MBC6, MBC7 | 0x20, 0x22 | MBC7 tilt: Wii Remote tilt, or level (no N64 game reads it) |
| 2 | TAMA5 | 0xFD | Tamagotchi 3 only |
| 2 | Pocket Camera | 0xFC | No camera on a Wii: take the picture from a PNG on SD |

Unknown header values: refuse the cartridge, and show the value in a message.

Rules from the accurate emulators to keep:
- RAM is on only after a write with 0x0A in the low nibble to 0x0000-0x1FFF.
- Reads of RAM that is off or missing return 0xFF (open bus).
- Mask every bank number by the real ROM and RAM size (header 0x148, 0x149, and the file size
  if the header is wrong).
- MBC3 RTC: latch on a 0x00 then 0x01 write to 0x6000-0x7FFF; halt bit; day counter carry
  bit (stays set until a write clears it); seconds write resets the sub-second counter.

### Files

- `wii64/gb/` holds the cartridges: `Pokemon Crystal.gbc` and its save
  `Pokemon Crystal.sav`, the same names that SameBoy, mGBA, BGB and VBA-M use.
- `.sav` is the RAM, then for clock cartridges a 48-byte footer (VBA-M/BGB format: 5
  registers, 5 latched registers, each 4 bytes, then a 64-bit UNIX time). Also read the
  44-byte footer (32-bit time). On load, add the time that passed since the save to the
  clock, as these emulators do. So a save moves between Wii64 and a PC emulator.
- A cartridge ROM is up to 8 MB (MBC5). Keep it in MEM2 when it fits the ROM paging budget
  (doc/rom-paging-and-agent.md); otherwise read 16 KB banks from SD on demand.
- Write the .sav when the game returns to the menu or closes, like the native saves, only
  when it changed; write to a temporary file and rename. Never from an interrupt.

### Settings

- `TransferPak1` to `TransferPak4` (text, per game): the cartridge file of each port, for
  example `TransferPak1 = "sd:/wii64/gb/Pokemon Crystal.gbc"`.
- `Pak1` to `Pak4`: new value 4 = Transfer Pak.

## Phases

1. **Cartridge layer, host-tested** (`gc_memory/gb_cart.c`, `tests/gb_cart_test.c`). No
   libogc. Header parser, phase-1 mappers, RAM, MBC3 RTC, .sav read/write with the
   footers. Tests: bank switching for every mapper, MBC1 quirks, RTC latch/halt/carry,
   save round trip with each footer, and a cartridge size that disagrees with its header.
2. **Pak device** (`gc_memory/pif.c`, `PLUGIN_TRANSFER_PAK`): the protocol table above, with
   a host test that replays the command sequence Pokemon Stadium sends.
3. **Menu and settings**: Transfer Pak in the Configure Paks cycle; a "Cartridge" button
   for that port opens the file browser on `wii64/gb` (.gb, .gbc only).
4. **Game tests** in Dolphin, then on the bench Wii: Pokemon Stadium (Red save import,
   GB Tower), Stadium 2 (Crystal, clock), Mario Golf, Mario Tennis, Perfect Dark cheats.
   Compare the .sav files with the same steps in mupen64plus.
5. **Phase-2 mappers and the Pocket Camera**, when a game or a request needs them.
