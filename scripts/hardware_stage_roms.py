#!/usr/bin/env python3
"""Stage only missing chain ROMs through HBC, within the caller's Wii lease."""
import argparse
import os
from pathlib import Path
import zlib

from hbc_watch import load_client


def stage(client, wii, chain, source):
    paths = {}
    source = Path(source).resolve()
    for line in Path(chain).read_text().splitlines():
        if not line.startswith("chain="):
            continue
        _, separator, remote = line.partition(" ")
        if not separator:
            raise ValueError("Chain entry has no ROM path")
        name = remote.removeprefix("sd:/wii64/roms/")
        if remote != "sd:/wii64/roms/" + name or not name or "/" in name or "\\" in name or name in (".", ".."):
            raise ValueError("ROM staging requires sd:/wii64/roms/<filename>")
        local = (source / name).resolve()
        if local.parent != source or not local.is_file() or not local.stat().st_size:
            raise ValueError("Missing or out-of-folder ROM: " + name)
        paths[remote] = local
    if not paths:
        raise ValueError("No chain ROMs to stage")
    # Validate all paths before any write; never replace a different owned ROM.
    for remote, local in paths.items():
        data = local.read_bytes()
        try:
            existing = client.checksum(wii, remote)
        except client.HBCError as error:
            if error.code != client.ENOENT:
                raise
        else:
            if existing != (len(data), zlib.crc32(data) & 0xffffffff):
                raise ValueError("Existing ROM differs; preserved " + remote)
            print("ROM already matches: " + local.name, flush=True)
            continue
        client.put_file(wii, remote, data)
        print("Staged ROM: " + local.name, flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("chain", type=Path)
    parser.add_argument("source", type=Path)
    parser.add_argument("--wii-ip", required=True)
    parser.add_argument("--hbc-client", required=True, type=Path)
    args = parser.parse_args()
    if not os.environ.get("WII_BENCH_JOB"):
        parser.error("Use the queued hardware runner; ROM staging requires its lease")
    stage(load_client(args.hbc_client), args.wii_ip, args.chain, args.source)
