#!/usr/bin/env python3
"""Only chain-named ROMs may be fetched from the hardware receiver."""

import http.server
import pathlib
import sys
import tempfile
import threading
import urllib.error
import urllib.request

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "scripts"))
from hardware_receive import Receiver


with tempfile.TemporaryDirectory() as directory:
    root = pathlib.Path(directory)
    (root / "GoldenEye 007.z64").write_bytes(b"ROM")
    (root / "other.z64").write_bytes(b"NO")
    with http.server.HTTPServer(("127.0.0.1", 0), Receiver) as server:
        server.wii_ip = "127.0.0.1"
        server.rom_dir = root
        server.rom_names = {"GoldenEye 007.z64", "../other.z64"}
        worker = threading.Thread(target=server.serve_forever, daemon=True)
        worker.start()
        url = f"http://127.0.0.1:{server.server_port}/rom/"
        assert urllib.request.urlopen(url + "GoldenEye%20007.z64").read() == b"ROM"
        for name in ("other.z64", "..%2Fother.z64"):
            try:
                urllib.request.urlopen(url + name)
            except urllib.error.HTTPError as error:
                assert error.code == 404
            else:
                raise AssertionError(f"served unapproved ROM: {name}")
        server.shutdown()
        worker.join()
print("hardware ROM receiver: ok")
