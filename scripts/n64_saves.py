"""Convert Project64 saves to Wii64 saves.

n64_saves.py SAVES_DIR ROMS_DIR [--out DIR] [--prefer U]

Project64 names a save GOODNAME.ext (the ROM's internal name); Wii64 names it
GOODNAME(REGION).ext, with the region from the ROM header. Project64 stores
SRAM (.sra) and FlashRAM (.fla) as little-endian 32-bit words, so their bytes
are swapped; EEPROM (.eep) and Controller Pak (.mpk) files are the same in
both. Wii64 writes full-size files: EEPROM 2,048 bytes (a 4 Kbit cart uses
the first 512), SRAM 32,768, FlashRAM 131,072, Controller Pak 131,072 (four
paks). Shorter files are padded with a blank save; Controller Pak files are
copied as they are (Wii64 reads a short one and keeps the other paks
formatted).

The converted files go to --out (default: SAVES_DIR). Nothing is deleted and
an existing Wii64 save is never replaced. Wii64 itself imports a lone
Project64 save the first time the game loads (main/rom_gc.c loadSaveFile).
"""
import argparse
from pathlib import Path
import sys

FULL = {"eep": (0x800, 0x00), "sra": (0x8000, 0x00), "fla": (0x20000, 0xFF)}
SWAPPED = {"sra", "fla"}
REGIONS = {0: "(Demo)", ord("7"): "(Beta)", 0x41: "(JU)", 0x44: "(G)", 0x45: "(U)",
           0x46: "(F)", ord("I"): "(I)", 0x4A: "(J)", ord("S"): "(S)",
           0x55: "(A)", 0x59: "(A)", 0x50: "(E)", 0x58: "(E)", 0x20: "(E)",
           0x21: "(E)", 0x38: "(E)", 0x70: "(E)"}
INVALID = '\\/:*?"<>|'


def rom_header(data):
    """The first 64 header bytes in big-endian (z64) order, or None."""
    magic = data[:4]
    if magic == b"\x80\x37\x12\x40":
        return data[:64]
    if magic == b"\x37\x80\x40\x12":  # .v64: bytes swapped in pairs
        return bytes(data[i ^ 1] for i in range(64))
    if magic == b"\x40\x12\x37\x80":  # .n64: little-endian words
        return bytes(data[i ^ 3] for i in range(64))
    return None


def goodname(header):
    """Wii64's ROM_SETTINGS.goodname: the internal name, trailing spaces cut."""
    name = header[0x20:0x34].split(b"\0")[0].decode("latin-1").rstrip(" ")
    return "".join("_" if c in INVALID else c for c in name)


def region(header):
    return REGIONS.get(header[0x3E], "(Unk)")


def convert(ext, data):
    """Project64 save bytes -> Wii64 save bytes."""
    if ext in FULL:
        size, blank = FULL[ext]
        data = data[:size] + bytes([blank]) * max(0, size - len(data))
    if ext in SWAPPED:
        data = b"".join(data[i:i + 4][::-1] for i in range(0, len(data), 4))
    return data


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("saves", type=Path)
    ap.add_argument("roms", type=Path)
    ap.add_argument("--out", type=Path)
    ap.add_argument("--prefer", default="U", help="region when one name has several ROMs (default U)")
    args = ap.parse_args()
    out = args.out or args.saves
    out.mkdir(parents=True, exist_ok=True)

    names = {}
    for rom in sorted(args.roms.iterdir()):
        if rom.is_file() and rom.suffix.lower() in (".z64", ".v64", ".n64", ".rom", ".bin"):
            with open(rom, "rb") as f:
                header = rom_header(f.read(64))
            if header:
                names.setdefault(goodname(header), set()).add(region(header))

    for save in sorted(args.saves.iterdir()):
        ext = save.suffix[1:].lower()
        stem = save.stem
        if ext not in ("eep", "sra", "fla", "mpk") or stem.endswith(")"):
            continue
        regions = names.get(stem)
        if not regions:
            print(f"skip {save.name}: no ROM with internal name {stem!r}")
            continue
        pick = f"({args.prefer})" if f"({args.prefer})" in regions else sorted(regions)[0]
        target = out / f"{stem}{pick}.{ext}"
        if (args.saves / target.name).exists() or target.exists():
            print(f"keep {target.name}: a Wii64 save exists ({save.name} not converted)")
            continue
        data = save.read_bytes()
        if not data:
            print(f"skip {save.name}: empty")
            continue
        target.write_bytes(convert(ext, data))
        note = ", bytes swapped" if ext in SWAPPED else ""
        print(f"wrote {target.name} ({len(data)} -> {target.stat().st_size} bytes{note})")


if __name__ == "__main__":
    sys.exit(main())
