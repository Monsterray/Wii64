"""Run with python3 tests/n64_saves_test.py: Project64 saves become Wii64 saves."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / "scripts"))
from n64_saves import convert, goodname, rom_header


def rom(name, country, order):
    h = bytearray(64)
    h[:4] = b"\x80\x37\x12\x40"
    h[0x20:0x20 + len(name)] = name.encode()
    h[0x20 + len(name):0x34] = b" " * (20 - len(name))
    h[0x3E] = country
    if order == "v64":
        h = bytes(h[i ^ 1] for i in range(64))
    elif order == "n64":
        h = bytes(h[i ^ 3] for i in range(64))
    return bytes(h)


# Header byte orders and the name rules match main/rom_gc.c.
for order in ("z64", "v64", "n64"):
    assert goodname(rom_header(rom("MarioParty3", 0x45, order))) == "MarioParty3"
assert goodname(rom_header(rom("A:B?", 0x45, "z64"))) == "A_B_"
# SRAM/FlashRAM words are swapped; short files are padded with a blank save.
assert convert("sra", b"DLEZ" + b"\0" * 4)[:4] == b"ZELD" and len(convert("sra", b"x")) == 0x8000
assert convert("fla", b"iraM")[:4] == b"Mari" and convert("fla", b"")[-1] == 0xFF
assert convert("eep", b"\1" * 512) == b"\1" * 512 + b"\0" * 1536
assert convert("mpk", b"\2" * 32768) == b"\2" * 32768

with tempfile.TemporaryDirectory() as d:
    d = Path(d)
    (d / "roms").mkdir()
    (d / "saves").mkdir()
    (d / "roms" / "sm64.v64").write_bytes(rom("SUPER MARIO 64", 0x45, "v64"))
    (d / "roms" / "mp3u.z64").write_bytes(rom("MarioParty3", 0x45, "z64"))
    (d / "roms" / "mp3e.z64").write_bytes(rom("MarioParty3", 0x50, "z64"))
    (d / "saves" / "SUPER MARIO 64.eep").write_bytes(b"\1" * 512)
    (d / "saves" / "MarioParty3.eep").write_bytes(b"\3" * 1384)
    (d / "saves" / "MarioParty3(U).eep").write_bytes(b"\4" * 2048)  # Wii64's own: kept
    (d / "saves" / "Killer Instinct Gold.eep").write_bytes(b"")
    out = subprocess.run([sys.executable, str(root / "scripts/n64_saves.py"), str(d / "saves"), str(d / "roms")],
                         capture_output=True, text=True, check=True).stdout
    assert (d / "saves" / "SUPER MARIO 64(U).eep").read_bytes() == b"\1" * 512 + b"\0" * 1536, out
    assert (d / "saves" / "MarioParty3(U).eep").read_bytes() == b"\4" * 2048, out
    assert not (d / "saves" / "MarioParty3(E).eep").exists(), out
    assert (d / "saves" / "SUPER MARIO 64.eep").exists(), out  # originals stay
    assert "keep MarioParty3(U).eep" in out and "skip Killer Instinct Gold.eep" in out, out
print("n64 saves: names, regions, byte orders, swap, padding, no overwrite: ok")
