#!/usr/bin/env python3
"""Compare full / disabled / full probe overhead for the same hardware chain."""
import argparse
from pathlib import Path

from subsystem_report import load, speed


def compare(a, b, c, same_probes=False):
    signature = lambda rows: [(r["rom"], r["vis"], r["vi_rate"], r["how"]) for r in rows]
    if not a or signature(a) != signature(b) or signature(a) != signature(c):
        raise ValueError("ROM order, VI targets, region rate and stop reason must match")
    if any(r["how"] != "vis" or not r.get("pmc2") or not r.get("pmc1") or
           r.get("vis", 0) <= 0 or r.get("wall_us", 0) <= 0 or r.get("vi_rate", 0) <= 0
           for rows in (a, b, c) for r in rows):
        raise ValueError("requires successful hardware VI runs with CPU counters")
    if any(r.get("vm_io", {}).get("errors", 0) for rows in (a, b, c) for r in rows):
        raise ValueError("I/O errors invalidate the comparison")
    instrumented = (a, b, c) if same_probes else (a, c)
    if any([(r.get("probe_schema", {}).get("version"),
             r.get("probe_schema", {}).get("interval")) for r in rows] !=
           [(r.get("probe_schema", {}).get("version"),
             r.get("probe_schema", {}).get("interval")) for r in a] for rows in instrumented):
        raise ValueError("probe schema/sampling interval differs; compare matching boundaries")
    if same_probes:
        if any(not r["subsystems"] for rows in (a, b, c) for r in rows):
            raise ValueError("requires enabled subsystem probes in all candidate/reference runs")
    elif any(not r["subsystems"] for rows in (a, c) for r in rows) or any(r["subsystems"] for r in b):
        raise ValueError("expected full / disabled / full subsystem probes")
    return [{"rom": x["rom"], "cycles_percent": 100 * ((x["pmc1"] + z["pmc1"]) / 2 / y["pmc1"] - 1),
             "wall_percent": 100 * ((x["wall_us"] + z["wall_us"]) / 2 / y["wall_us"] - 1),
             "non_sleep_percent": (100 * (((x["wall_us"] - x.get("sleep_us", 0)) +
                                           (z["wall_us"] - z.get("sleep_us", 0))) / 2 /
                                          (y["wall_us"] - y.get("sleep_us", 0)) - 1)
                                   if y["wall_us"] > y.get("sleep_us", 0) else None),
             "speed": [speed(row) for row in (x, y, z)],
             "underruns": [row.get("underruns", 0) for row in (x, y, z)],
             "overruns": [row.get("overruns", 0) for row in (x, y, z)]}
            for x, y, z in zip(a, b, c)]


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("runs", nargs=3, type=Path)
    parser.add_argument("--same-probes", action="store_true", help="candidate/reference/candidate, not probe overhead")
    args = parser.parse_args()
    configs = [[line.strip() for line in (root / "diag.cfg").read_text().splitlines()
                if line.strip() and not line.startswith(("#", "result_host=", "result_tag="))] for root in args.runs]
    if configs[0] != configs[1] or configs[0] != configs[2]:
        parser.error("diagnostic settings/input chains differ")
    try:
        results = compare(*(load(root) for root in args.runs), same_probes=args.same_probes)
    except ValueError as error:
        parser.error(str(error))
    for row in results:
        non_sleep = f"{row['non_sleep_percent']:+.2f}%" if row['non_sleep_percent'] is not None else "n/a"
        print(f"{Path(row['rom']).name}: cycles {row['cycles_percent']:+.2f}%, wall {row['wall_percent']:+.2f}%; "
              f"non-sleep {non_sleep}; "
              f"speed {'/'.join(f'{v:.3f}' for v in row['speed'])}; "
              f"underruns {row['underruns']}; overruns {row['overruns']}")
    print("Candidate mean versus reference, all probes enabled; this is not probe-cost calibration."
          if args.same_probes else "Full mean versus disabled control; one A/B/A triple does not calibrate probe cost.")
    print("Non-sleep wall time = wall minus requested limiter sleep; it includes other waits and interruptions, not CPU self time.")
