#!/usr/bin/env python3
"""Probe-off release boot, SDK safety, HOME and HBC return checks under a lease.

Queue this script with wiibench.py. Freeze the script, client, DOL and matching
ELF before submission. All connections are outbound; no log receiver is started.
"""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import time


def check(client, wii, dol, output, version, sdk_version, roms, settle_seconds=5):
    output.mkdir(parents=True, exist_ok=False)

    def save(name, data):
        (output / name).write_text(json.dumps(data, indent=2) + "\n")

    def wait_for(predicate, seconds=90):
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            try:
                state = client.status(wii)
                if predicate(state):
                    return state
            except (OSError, client.HBCError):
                pass  # A launch/reload temporarily takes networking down.
            time.sleep(1)
        raise RuntimeError("Wii did not reach the expected state before the deadline")

    for index, rom in enumerate([None, *roms]):
        label = f"{index:02d}"
        before = client.status(wii)
        if before.get("agent") or before.get("version") != sdk_version:
            raise RuntimeError("Expected the current HBC, not a running app; no upload sent")
        save(label + "-hbc-before.json", before)
        args = ["--diag=dynacore=dynarec", "AutoSave=0"]
        if rom:
            args.append("--diag=autoboot_rom=" + rom)
        launched = False
        try:
            client.send(wii, str(dol), args, name="Wii64 release check")
            launched = True
            state = wait_for(lambda s: s.get("agent") and s.get("app") == "Wii64")
            if state.get("version") != sdk_version or state.get("app_version") != version:
                raise RuntimeError("Running SDK or Wii64 version differs from the frozen build")
            initial_retraces = ((state.get("safety") or {}).get("frames") or {}).get("retraces", 0)
            time.sleep(settle_seconds)
            state = client.status(wii)
            save(label + "-agent.json", state)
            safety = state.get("safety", {})
            if (safety.get("frames") or {}).get("retraces", 0) <= initial_retraces:
                raise RuntimeError("SDK retrace callback was replaced or never ran")
            if safety.get("stack_guard") not in ("breakpoint", "marker"):
                raise RuntimeError("SDK stack guard is not active")
            width, height, picture = client.screen(wii)
            (output / (label + "-frame.png")).write_bytes(client.yuyv_png(width, height, picture))
            client.send_keys(wii, "h")
            opened = wait_for(lambda s: "overlay_ui" in s, 30)
            save(label + "-home.json", opened)
            client.send_keys(wii, "h")
            closed = wait_for(lambda s: s.get("agent") and "overlay_ui" not in s, 30)
            time.sleep(1)
            resumed = client.status(wii)
            save(label + "-resumed.json", resumed)
            if resumed["safety"]["frames"]["retraces"] <= closed["safety"]["frames"]["retraces"]:
                raise RuntimeError("SDK retrace callback stopped after HOME")
        finally:
            if launched:
                current = wait_for(lambda s: True)
                # The client refuses to exit an app older than this queue job.
                if (current.get("agent") and current.get("app") == "Wii64"
                        and current.get("app_version") == version):
                    client.exit_app(wii, 90)
                client.hbc_wait(wii, 90)
                save(label + "-hbc-after.json", client.status(wii))
        after = client.status(wii)
        save(label + "-hbc-after.json", after)
        if after.get("agent") or after.get("version") != sdk_version:
            raise RuntimeError("Release did not return to current HBC")
        if after.get("crash") and after["crash"] != before.get("crash"):
            raise RuntimeError("A new crash was recorded; retained in hbc-after.json")
        print(f"PASS {label}: {'ROM boot' if rom else 'menu'}, SDK {sdk_version}, retrace/stack guards, HOME and HBC return", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client", type=Path, required=True)
    parser.add_argument("--dol", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--sdk-version", required=True)
    parser.add_argument("--rom", action="append", default=[])
    parser.add_argument("--settle-seconds", type=int, default=5,
                        help="Wait after agent startup before sampling each boot (1..120)")
    args = parser.parse_args()
    if not 1 <= args.settle_seconds <= 120:
        parser.error("--settle-seconds must be 1..120")
    if not os.environ.get("WII_BENCH_JOB_START") or not os.environ.get("WII_BENCH_IP"):
        parser.error("Run through wiibench.py; no Wii request was sent")
    if not args.dol.is_file() or not args.dol.with_suffix(".elf").is_file():
        parser.error("Keep the frozen DOL and matching ELF together")
    spec = importlib.util.spec_from_file_location("wii64_release_hbc", args.client)
    client = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(client)
    check(client, os.environ["WII_BENCH_IP"], args.dol, args.output,
          args.version, args.sdk_version, args.rom, args.settle_seconds)


if __name__ == "__main__":
    main()
