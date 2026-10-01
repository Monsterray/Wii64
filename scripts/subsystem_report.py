#!/usr/bin/env python3
"""Inclusive subsystem survey; hot-operation estimates are not additive CPU totals."""
import argparse
import json
from pathlib import Path

from chain_table import parse_game


def load(path):
    path = Path(path)
    if path.is_dir():
        path /= "perf.log"
    rows, stages, vm_io, startup, audio, self_times, gpu, schema = [], {}, {}, {}, {}, {}, {}, {}
    with path.open(encoding="latin-1") as log:
        for line in log:
            line = line.replace("\0", "").strip()
            if line == "mark: ROMCache_load: enter":
                startup = {}
            elif line.startswith("mark: ROMCache_load: preflush_us="):
                startup["preflush"] = {key: int(value) for key, value in
                                       (part.split("=", 1) for part in line.split()[2:])}
            elif line.startswith("mark: ROMCache_load: elapsed_us="):
                startup.update({key: int(value) for key, value in
                                (part.split("=", 1) for part in line.split()[2:])})
            elif line.startswith("subsystem_time: "):
                fields = dict(part.split("=", 1) for part in line.split()[1:])
                required = {"stage", "calls", "timed_calls", "period", "timed_us"}
                if not required <= fields.keys():
                    raise ValueError(f"Incomplete subsystem timer in {path}: {line}")
                name = fields.pop("stage")
                stages[name] = {key: int(value) for key, value in fields.items()}
            elif line.startswith("subsystem_self: "):
                fields = dict(part.split("=", 1) for part in line.split()[1:])
                if not {"stage", "calls", "timed_calls", "self_us", "dropped"} <= fields.keys():
                    raise ValueError(f"Incomplete self timer in {path}: {line}")
                name = fields.pop("stage")
                self_times[name] = {key: int(value) for key, value in fields.items()}
                s = self_times[name]
                if not 0 <= s["timed_calls"] <= s["calls"] or s["self_us"] < 0 or s["dropped"] < 0:
                    raise ValueError(f"Invalid self timer in {path}: {line}")
            elif line.startswith(("gpu_status: ", "probe_schema: ")):
                fields = {key: int(value) for key, value in
                          (part.split("=", 1) for part in line.split()[1:])}
                if line.startswith("gpu_status:"):
                    gpu = fields
                else:
                    schema = fields
            elif line.startswith("audio_time: "):
                fields = dict(part.split("=", 1) for part in line.split()[1:])
                name = fields.pop("stage")
                audio[name] = {key: int(value) for key, value in fields.items()}
            elif line.startswith("vm_io: "):
                vm_io = {key: int(value) for key, value in
                         (part.split("=", 1) for part in line.split()[1:])}
            elif line.startswith("game: "):
                row = parse_game(line)
                row["subsystems"] = stages
                row["vm_io"] = vm_io
                row["startup"] = startup
                row["audio_stages"] = audio
                row["self_times"] = self_times
                row["gpu_status"] = gpu
                row["probe_schema"] = schema
                rows.append(row)
                stages = {}
                vm_io = {}
                startup = {}
                audio = {}
                self_times, gpu, schema = {}, {}, {}
    return rows


def milliseconds(stage):
    calls, timed = stage.get("calls", 0), stage.get("timed_calls", 0)
    return stage.get("timed_us", 0) * calls / timed / 1000 if timed else None


def speed(row):
    return row.get("vis", 0) * 1e6 / row["wall_us"] / row["vi_rate"] if row.get("wall_us") and row.get("vi_rate") else 0


def self_milliseconds(stage):
    # Never extrapolate rejected/reentrant roots into an apparent CPU budget.
    if stage.get("dropped"):
        return None
    return milliseconds(dict(stage, timed_us=stage.get("self_us", 0)))


def report(rows):
    for row in rows:
        if row.get("how") != "vis" or not row.get("vis") or not row.get("wall_us") or not row.get("vi_rate"):
            print(f"\n{Path(row['rom']).name}: incomplete ({row.get('how', '?')}, "
                  f"{row.get('vis', 0)} VIs); timers excluded (may belong to the previous game)")
            continue
        wall_ms = row["wall_us"] / 1000
        busy_ms = (row["wall_us"] - row.get("sleep_us", 0)) / 1000
        print(f"\n{Path(row['rom']).name}: {row['vis']} VIs, {speed(row):.3f}x, "
              f"requested sleep {100 * row.get('sleep_us', 0) / row['wall_us']:.1f}%")
        print(f"  Non-sleep wall: {busy_ms / 1000:.3f} s (includes other waits/interruptions, not CPU self time)")
        stages = row["subsystems"]
        gfx = stages.get("rsp_gfx", {})
        if gfx and gfx.get("calls", 0) < 10:
            print("  Scene warning: fewer than 10 graphics tasks; verify gameplay visually. "
                  "VI speed alone is not a correctness result.")
        startup = row.get("startup", {})
        if "elapsed_us" in startup:
            print(f"  Startup: ROM load {startup['elapsed_us'] / 1e6:.3f} s, "
                  f"{startup['bytes']} bytes, VM {startup['vm']} (excluded from gameplay)")
        if "preflush" in startup:
            flush = startup["preflush"]
            print(f"  Preflush: {flush['preflush_us'] / 1e6:.3f} s (included in ROM load)" +
                  (f", {flush['writes']} writes, {flush['bytes']} bytes" if "writes" in flush else ""))
        if row.get("vm_io"):
            io = row["vm_io"]
            print(f"  NAND: {io['reads']} reads, {io['hits']} cached pages, "
                  f"{io['read_bytes']} bytes; {io['writes']} writes, "
                  f"{io['write_bytes']} bytes; {io['errors']} I/O errors")
        if not stages:
            print("  subsystem probes absent; counts/speed cannot identify an operation bottleneck")
            continue
        # Exact inclusive spans first. Sparse estimates are a separate ranking.
        for name, stage in sorted(stages.items(), key=lambda item: (
                item[1]["calls"] != item[1]["timed_calls"], -(milliseconds(item[1]) or 0))):
            ms = milliseconds(stage)
            if ms is None:
                print(f"  {name:18} {stage['calls']:9} calls; no completed timing samples")
                continue
            exact = stage["calls"] == stage["timed_calls"]
            label = "observed" if exact else "estimate"
            few = " [few samples]" if stage["timed_calls"] < 20 else ""
            alias = " [sampling estimate exceeds wall time]" if not exact and ms > wall_ms else ""
            print(f"  {name:18} {ms:9.1f} ms {label:8} ({100 * ms / wall_ms:5.1f}% wall), "
                  f"{stage['timed_calls']}/{stage['calls']} timed{few}{alias}")
        limiter = stages.get("limiter", {})
        if limiter.get("timed_calls") and limiter["timed_calls"] == limiter["calls"]:
            actual, requested = limiter["timed_us"], row.get("sleep_us", 0)
            print(f"  limiter observed-requested: {(actual - requested) / 1000:+.1f} ms")
        for name, stage in row.get("audio_stages", {}).items():
            if stage.get("sampled_calls"):
                print(f"  audio/{name:12} {stage['sampled_us'] / 1000:9.1f} ms sampled only, "
                      f"{stage['sampled_calls']} calls (not extrapolated)")
        if row.get("self_times"):
            print("  Self spans: exclude named nested work within the same sampled call, not independent estimates:")
            for name, stage in row["self_times"].items():
                ms = self_milliseconds(stage)
                if ms is None:
                    print(f"    {name}: no usable self estimate; {stage['timed_calls']} samples, {stage['dropped']} dropped")
                    continue
                busy = f"{100 * ms / busy_ms:.2f}% non-sleep" if busy_ms > 0 else "non-sleep n/a"
                label = "observed" if stage["calls"] == stage["timed_calls"] else "estimate"
                alias = " [estimate exceeds wall time]" if ms > wall_ms else ""
                print(f"    {name:18} {ms:9.1f} ms {label}, {100 * ms / wall_ms:.2f}% wall / {busy}; "
                      f"{stage['timed_calls']}/{stage['calls']} samples{alias}")
        gpu = row.get("gpu_status", {})
        if gpu:
            print(f"  GPU status: {gpu.get('samples', 0)} VI-service observations; "
                  f"command busy {gpu.get('command_busy', 0)}, FIFO busy {gpu.get('fifo_busy', 0)}, "
                  f"high watermark {gpu.get('over_high', 0)}. Not GPU time or utilization.")
        if "dsp_samples" in row:
            print(f"  DSP: {row['dsp_samples']} periodic observations, mean {row.get('dsp_avg', 0):.2f}% / "
                  f"peak {row.get('dsp_peak', 0):.2f}% of the latest AESND block budget, not CPU wall shares")
        groups = {
            "CPU/JIT": ("execute_inclusive", "cpu_helper", "compile", "lookup", "dispatch", "jit_invalidate", "interpreter_inclusive"),
            "memory/IO": ("memory_slow", "tlb_translate", "dma_pi", "dma_sp", "dma_si", "rom_copy", "vm_fault"),
            "RSP": ("rsp_gfx", "rsp_audio", "rsp_other"),
            "graphics": ("gfx_list", "gfx_command", "vertex", "gfx_state", "tex_hash", "draw_triangles"),
            "presentation/waits": ("present", "gx_wait", "limiter"),
            "audio output": ("audio_submit", "audio_callback"),
            "input/system": ("pif", "input", "guest_interrupt"),
            "storage/support": ("storage_read", "storage_write", "save_load", "save_write", "state_load", "state_save", "probe_io", "agent_poll"),
        }
        print("  Coverage (inclusive timers; absent does not mean zero cost):")
        for group, names in groups.items():
            active = [name for name in names if stages.get(name, {}).get("timed_calls")]
            inactive = [name for name in names if name in stages and not stages[name].get("calls")]
            unsampled = [name for name in names if stages.get(name, {}).get("calls") and not stages[name].get("timed_calls")]
            absent = [name for name in names if name not in stages]
            print(f"    {group}: timed={','.join(active) or '-'}; inactive={','.join(inactive) or '-'}; "
                  f"no samples={','.join(unsampled) or '-'}; absent={','.join(absent) or '-'}")
        print("  Not directly measured: GPU execution, host scheduler/other IRQ work, "
              "inline guest memory accesses. DSP mean/peak are periodic samples, not CPU timings.")
        if row.get("self_times"):
            print("  Execute self includes inline guest accesses, uninstrumented arithmetic helpers and other host IRQs; "
                  "memory self is handler remainder, not every memory access. Sparse self estimates can alias.")
    print("\nInclusive spans overlap (dispatch includes lookup/compile; execution can include RSP/limiter; "
          "draw_rect includes state/texture setup; guest_interrupt includes limiter/present; "
          "gfx_list overlaps rsp_gfx; memory_slow includes mapped-device handlers and RSP work). "
          "Do not sum them. Timing includes interruptions and waits; sampled estimates can alias.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run", type=Path)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    rows = load(args.run)
    if not rows:
        parser.error("no completed game rows")
    if args.json:
        print(json.dumps(rows, indent=2))
    else:
        report(rows)
