#!/usr/bin/env python3
"""Validate that a Wii run used the requested chain and loaded its pad replays."""
import pathlib
import re
import sys

from chain_table import games


def main(run_dir):
    root = pathlib.Path(run_dir)
    config = (root / "diag.cfg").read_text(errors="replace").splitlines()
    log_path = root / "perf.log"
    log = log_path.read_text(errors="replace")
    expected = []
    for line in config:
        if not line.startswith("chain="):
            continue
        spec, rom = line[6:].split(" ", 1)
        expected.append((int(spec.split(",", 1)[0]), ",input=" in spec, rom))

    errors = []
    rows = games(log_path)
    if "mark: diag config: wiiload arguments" not in log:
        errors.append("run did not confirm it used the selected wiiload configuration")
    core = next((line.split("=", 1)[1] for line in config if line.startswith("dynacore=")), None)
    if core:
        expected_core = "pure interpreter" if core == "pureinterp" else "dynarec" if core == "dynarec" else None
        if expected_core:
            for marker, stage in (
                ("diag received dynacore line", "receive"),
                (f"diag requested core: {expected_core}", "parse"),
                (f"diag applied core: {expected_core}", "apply"),
                (f"CPU core dispatch: {expected_core}", "runtime dispatch"),
            ):
                if f"mark: {marker}" not in log:
                    errors.append(f"requested {core} core was not confirmed at {stage}")
    if len(rows) != len(expected):
        errors.append(f"expected {len(expected)} game rows; received {len(rows)}")
    for i, (vis, has_input, rom) in enumerate(expected[:len(rows)]):
        row = rows[i]
        if row.get("how") != "vis" or row.get("vis", 0) < vis:
            errors.append(f"game {i + 1} did not reach {vis} VIs")
        if pathlib.PurePosixPath(rom).name.lower() not in str(row.get("rom", "")).lower():
            errors.append(f"game {i + 1} ROM does not match the requested chain")

    replay_counts = [int(n) for n in re.findall(r"^mark: pad replay records: (\d+)\s*$", log, re.M)]
    input_count = sum(has_input for _, has_input, _ in expected)
    if len(replay_counts) != input_count:
        errors.append(f"expected {input_count} replay-load results; received {len(replay_counts)}")
    elif any(n == 0 for n in replay_counts):
        errors.append(f"controller replay did not load: record counts {replay_counts}")

    if errors:
        for error in errors:
            print(f"FAIL: {error}")
        return 1
    print(f"PASS: {len(rows)} requested game rows reached their VI targets; replay records {replay_counts}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1] if len(sys.argv) > 1 else "."))
