#!/usr/bin/env python3
"""Pull one completed, tagged Wii64 run from HBC. No listening socket."""

import argparse
import pathlib
import re
import time

from chain_table import games
from check_hardware_run import main as validate_run
from hbc_watch import RunWatch, load_client


def collect(client, wii, output, timeout, elf=None):
    output = pathlib.Path(output)
    config = (output / "diag.cfg").read_text()
    tags = re.findall(r"^result_tag=([0-9a-f]{32})$", config, re.M)
    if len(tags) != 1:
        raise ValueError("Pull collection requires one unique result_tag")
    marker = "mark: result_tag=" + tags[0]
    watch = RunWatch(client, wii, output, elf)
    deadline = time.monotonic() + timeout
    (output / ".ready").touch()
    try:
        while time.monotonic() < deadline:
            if not (output / ".sent").exists():
                time.sleep(0.25)
                continue
            try:
                version = client.version(wii, 2)
                status = client.status(wii)
            except (OSError, client.HBCError):
                time.sleep(2)
                continue  # Network drops briefly while HBC reloads.
            failure = watch.new_crash(status)
            if failure:
                raise RuntimeError(failure)
            if client.is_agent(version):
                time.sleep(5)
                continue
            # HBC can still answer immediately after wiiload. A unique mark,
            # not file existence or an observed agent transition, proves freshness.
            try:
                data = client.get_file(wii, "sd:/wii64/perf.log")
            except (OSError, client.HBCError):
                time.sleep(2)
                continue
            if marker not in data.decode("latin-1").splitlines():
                time.sleep(2)
                continue
            if len(data) > 2 * 1024 * 1024:
                raise ValueError("Oversized perf.log")
            (output / "perf.log").write_bytes(data)
            if validate_run(output):
                raise RuntimeError("Tagged run failed chain validation")
            profiles = {int(n) for n in re.findall(r"^hprof: n=(\d+) .*saved=1\b", data.decode("latin-1"), re.M)}
            for expected_index, row in enumerate(games(output / "perf.log"), 1):
                index = int(str(row["n"]).split("/", 1)[0])
                if index != expected_index:
                    raise ValueError("Nonsequential game index")
                if not 1 <= index <= 32:
                    raise ValueError("Invalid game index")
                names = [f"xfb_{index:02d}.bin"]
                if row.get("padtrace", 0):
                    names.append(f"padtrace_{index:02d}.csv")
                # Both hprof controls and sampler write a file per completed game.
                if index in profiles:
                    names.append(f"hprof_{index:02d}.bin")
                for name in names:
                    if time.monotonic() >= deadline:
                        raise TimeoutError("Timed out collecting Wii files")
                    payload = client.get_file(wii, "sd:/wii64/" + name)
                    if len(payload) > 2 * 1024 * 1024:
                        raise ValueError("Oversized " + name)
                    (output / name).write_bytes(payload)
                    print(f"collected {name}: {len(payload)} bytes", flush=True)
            print("hardware run collected without a local server", flush=True)
            return
        raise TimeoutError("Timed out waiting for the tagged Wii run to return to HBC")
    finally:
        (output / ".ready").unlink(missing_ok=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=pathlib.Path)
    parser.add_argument("--wii-ip", required=True)
    parser.add_argument("--hbc-client", required=True, type=pathlib.Path)
    parser.add_argument("--timeout", type=int, default=1200)
    parser.add_argument("--elf")
    args = parser.parse_args()
    collect(load_client(args.hbc_client), args.wii_ip, args.output, args.timeout, args.elf)
