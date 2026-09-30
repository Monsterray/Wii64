#!/usr/bin/env python3
"""Summarize sampled audio stages from a PERF_PROF perf.log (hardware times only)."""
import os
import statistics
import sys

from chain_table import parse_game


def fields(line):
    return dict(part.split("=", 1) for part in line.split()[1:] if "=" in part)


def report(path):
    stages = {}
    counts = {}
    steady = {}
    queue = []
    gaps = {}
    output = {}
    with open(path, encoding="latin-1") as log:
        for raw in log:
            line = raw.replace("\0", "").strip()
            if line.startswith("audio:"):
                counts.update(fields(line))
            elif line.startswith("audio_stages:"):
                counts.update(fields(line))
            elif line.startswith("audio_envmix:"):
                steady = fields(line)
                counts.update(steady)
            elif line.startswith("audio_gaps:"):
                gaps = fields(line)
            elif line.startswith("audio_output:"):
                output = fields(line)
            elif line.startswith("audio_time:"):
                stage = fields(line)
                stages[stage["stage"]] = stage
            elif line.startswith("cpu:"):
                sample = fields(line).get("queue_ms")
                if sample is not None:
                    queue.append(int(sample))
            elif line.startswith("game:"):
                game = parse_game(line)
                if game.get("how") != "vis":
                    stages, counts, steady, queue, gaps, output = {}, {}, {}, [], {}, {}
                    continue
                print(f"\n{os.path.basename(game['rom'])}: {game['vis']} VIs, "
                      f"{game['underruns']} underruns, {game['pmc1']} cycles")
                if queue:
                    ordered = sorted(queue)
                    p95 = ordered[(95 * len(ordered) - 1) // 100]
                    print(f"  queued PCM    {min(queue)}-{max(queue)} ms, "
                          f"mean {statistics.mean(queue):.1f} ms, "
                          f"p95 {p95} ms ({len(queue)} sample{'s' if len(queue) != 1 else ''}; "
                          "excludes DSP/output delay)")
                active_gaps = [f"{name}={value}" for name, value in gaps.items() if int(value)]
                if active_gaps:
                    print("  unsupported    " + ", ".join(active_gaps))
                if output:
                    print(f"  AESND stream   {output['stream_fed']}/{output['stream_requests']} "
                          f"buffers fed, input {output['input_hz']} Hz"
                          + (f", producer queue peak {output['queue_peak_ms']} ms"
                             if 'queue_peak_ms' in output else ''))
                for name, data in stages.items():
                    key = {
                        "resample": "alist_resample_calls", "zoh": "alist_zoh_calls",
                        "musyx_fx": "musyx_fx_calls",
                    }.get(name, f"{name}_calls" if name in ("adpcm", "mix")
                          else f"{name[7:]}_calls" if name.startswith("envmix_") else "")
                    calls = int(counts.get(key, 0))
                    sampled = int(data["sampled_calls"])
                    if not calls or not sampled:
                        continue
                    measured = int(data["sampled_us"])
                    estimate = measured * calls / sampled / 1000
                    fraction = ""
                    if name.startswith("envmix_"):
                        held = int(steady.get(f"{name[7:]}_steady", 0))
                        fraction = f", steady {held}/{calls} ({held/calls:.0%})" if held else ""
                    print(f"  {name:13} ~{estimate:7.1f} ms  "
                          f"({measured} us/{sampled} sampled, {calls} total{fraction})")
                stages, counts, steady, queue, gaps, output = {}, {}, {}, [], {}, {}


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("usage: audio_report.py RUN_DIR_OR_PERF_LOG")
    path = sys.argv[1]
    report(os.path.join(path, "perf.log") if os.path.isdir(path) else path)
