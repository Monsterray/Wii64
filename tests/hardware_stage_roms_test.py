"""Missing-only ROM staging: no deletion, overwrite, traversal or server."""
from pathlib import Path
import sys
import tempfile
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from hardware_stage_roms import stage


class Client:
    ENOENT = 2
    class HBCError(Exception):
        def __init__(self, code):
            self.code = code
    files = {}
    puts = []
    error = 0
    @classmethod
    def checksum(cls, wii, remote):
        if cls.error:
            raise cls.HBCError(cls.error)
        if remote not in cls.files:
            raise cls.HBCError(2)
        data = cls.files[remote]
        return len(data), zlib.crc32(data) & 0xffffffff
    @classmethod
    def put_file(cls, wii, remote, data):
        cls.puts.append(remote)
        cls.files[remote] = data


with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    roms = root / "owned roms"
    roms.mkdir()
    (roms / "game.z64").write_bytes(b"owned fixture")
    chain = root / "chain.txt"
    remote = "sd:/wii64/roms/game.z64"
    chain.write_text(f"chain=60 {remote}\nchain=60 {remote}\n")
    stage(Client, "fixture", chain, roms)
    assert Client.puts == [remote]
    stage(Client, "fixture", chain, roms)
    assert Client.puts == [remote]
    Client.files[remote] = b"different owned fixture"
    try:
        stage(Client, "fixture", chain, roms)
    except ValueError as error:
        assert "preserved" in str(error)
    else:
        raise AssertionError("Overwrote an existing ROM")
    assert Client.files[remote] == b"different owned fixture"
    for bad in ("usb:/wii64/roms/game.z64", "sd:/wii64/roms/../outside", "sd:/wii64/roms/missing.z64", ""):
        chain.write_text(f"chain=60 {remote}\nchain=60 {bad}\n")
        try:
            stage(Client, "fixture", chain, roms)
        except ValueError:
            pass
        else:
            raise AssertionError("Accepted bad chain path " + bad)
    assert Client.puts == [remote]
    chain.write_text(f"chain=60 {remote}\n")
    Client.error = 13
    try:
        stage(Client, "fixture", chain, roms)
    except Client.HBCError as error:
        assert error.code == 13
    else:
        raise AssertionError("Treated an access error as a missing ROM")
    assert Client.puts == [remote]
    Client.error = 0
    (roms / "empty.z64").touch()
    chain.write_text("chain=60 sd:/wii64/roms/empty.z64\n")
    try:
        stage(Client, "fixture", chain, roms)
    except ValueError:
        pass
    else:
        raise AssertionError("Staged an empty ROM")
print("HBC ROM staging: missing-only, matching CRC, deduplicated names and path guards PASS")
