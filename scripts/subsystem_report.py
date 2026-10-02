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
    rows, stages, vm_io, startup, audio, self_times, gpu, schema, gp, ops, ucodes = [], {}, {}, {}, {}, {}, {}, {}, {}, {}, 0
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
            elif line.startswith("gfx_ucodes: "):
                ucodes = int(line.split("mask=", 1)[1], 16)
            elif line.startswith("gfx_opcode: "):
                fields = dict(part.split("=", 1) for part in line.split()[1:])
                op = int(fields.pop("op"), 16)
                ops[op] = {key: int(value) for key, value in fields.items()}
            elif line.startswith("gpu_counters: "):
                gp = {key: int(value) for key, value in (part.split("=", 1) for part in line.split()[1:])}
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
                row["gpu_counters"] = gp
                row["gfx_opcodes"] = ops
                row["gfx_ucodes"] = ucodes
                rows.append(row)
                stages = {}
                vm_io = {}
                startup = {}
                audio = {}
                self_times, gpu, schema, gp, ops, ucodes = {}, {}, {}, {}, {}, 0
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


# GBI opcodes: RDP commands are shared; RSP commands differ by microcode family.
RDP = {0xc0: "NOOP", 0xe4: "TEXRECT", 0xe5: "TEXRECTFLIP", 0xe6: "LOADSYNC", 0xe7: "PIPESYNC",
       0xe8: "TILESYNC", 0xe9: "FULLSYNC", 0xea: "SETKEYGB", 0xeb: "SETKEYR", 0xec: "SETCONVERT",
       0xed: "SETSCISSOR", 0xee: "SETPRIMDEPTH", 0xef: "SETOTHERMODE", 0xf0: "LOADTLUT",
       0xf2: "SETTILESIZE", 0xf3: "LOADBLOCK", 0xf4: "LOADTILE", 0xf5: "SETTILE", 0xf6: "FILLRECT",
       0xf7: "SETFILLCOLOR", 0xf8: "SETFOGCOLOR", 0xf9: "SETBLENDCOLOR", 0xfa: "SETPRIMCOLOR",
       0xfb: "SETENVCOLOR", 0xfc: "SETCOMBINE", 0xfd: "SETTIMG", 0xfe: "SETZIMG", 0xff: "SETCIMG"}
F3D = {**RDP, 0x00: "SPNOOP", 0x01: "MTX", 0x03: "MOVEMEM", 0x04: "VTX", 0x06: "DL",
                   0x07: "DLINMEM", 0xb1: "TRI2", 0xb2: "MODIFYVTX", 0xb3: "RDPHALF_2", 0xb4: "RDPHALF_1",
                   0xb5: "QUAD", 0xb6: "CLEARGEOMETRYMODE", 0xb7: "SETGEOMETRYMODE", 0xb8: "ENDDL",
                   0xb9: "SETOTHERMODE_L", 0xba: "SETOTHERMODE_H", 0xbb: "TEXTURE", 0xbc: "MOVEWORD",
                   0xbd: "POPMTX", 0xbe: "CULLDL", 0xbf: "TRI1"}
F3DEX2 = {**RDP, 0x01: "VTX", 0x02: "MODIFYVTX", 0x03: "CULLDL", 0x04: "BRANCH_Z", 0x05: "TRI1",
                      0x06: "TRI2", 0x07: "QUAD", 0xd3: "SPECIAL_3", 0xd4: "SPECIAL_2", 0xd5: "SPECIAL_1",
                      0xd6: "DMA_IO", 0xd7: "TEXTURE", 0xd8: "POPMTX", 0xd9: "GEOMETRYMODE", 0xda: "MTX",
                      0xdb: "MOVEWORD", 0xdc: "MOVEMEM", 0xdd: "LOAD_UCODE", 0xde: "DL", 0xdf: "ENDDL",
                      0xe0: "SPNOOP", 0xe1: "RDPHALF_1", 0xe2: "SETOTHERMODE_L", 0xe3: "SETOTHERMODE_H",
                      0xf1: "RDPHALF_2"}
F3DEX2_TYPES = {2, 5, 7, 11, 13, 22}  # glN64 GBI.h: F3DEX2, L3DEX2, S2DEX2, F3DFLX2, F3DZEX2, F3DEX2ACCLAIM


def opcode_lines(row, limit=10):
    """Which display-list commands the CPU spends graphics time in."""
    ops, mask = row.get("gfx_opcodes"), row.get("gfx_ucodes", 0)
    if not ops:
        return []
    types = {t for t in range(32) if mask >> t & 1}
    names = F3DEX2 if types and types <= F3DEX2_TYPES else F3D if types and not types & F3DEX2_TYPES else RDP
    if types == {9}:  # F3DDKR (Diddy Kong Racing, Jet Force Gemini): its own triangle command
        names = {**names, 0x05: "DMATRI"}
    estimate = {op: milliseconds(dict(o, timed_us=o["timed_us"])) for op, o in ops.items()}
    total = sum(ms for ms in estimate.values() if ms)
    lines = [f"  Display-list commands by estimated CPU time (microcode types {sorted(types)}; "
             "calls exact, time from the 1-in-period gfx_command samples):"]
    for op in sorted(ops, key=lambda op: -(estimate[op] or 0))[:limit]:
        o, ms = ops[op], estimate[op]
        name = names.get(op, "?")
        if ms is None:
            lines.append(f"    {op:02x} {name:18} {o['calls']:9} calls; no timed samples")
            continue
        few = " [few samples]" if o["timed_calls"] < 20 else ""
        lines.append(f"    {op:02x} {name:18} {ms:9.1f} ms ({100 * ms / total:5.1f}%), {o['calls']:9} calls, "
                     f"{1000 * ms / o['calls']:7.2f} us/call{few}")
    return lines


def gpu_lines(row, stages):
    """The GP's own counters: whether the GP limits the scene, and which unit."""
    gp = row.get("gpu_counters")
    if not gp:
        return []
    clks, tb = gp["clks"], gp["tb_clks"]
    overflows = gp["fifo_overflows"]
    lines = [f"  GP FIFO overflows: {overflows} ({overflows * 1e6 / row['wall_us']:.1f}/s); "
             "each blocks the CPU until the GP reads the FIFO down"]
    if not clks or abs(clks / tb - 1) > 0.02:
        # Dolphin returns 0; a mismatch means the counters did not count this span.
        return lines + [f"  GP counters: clks {clks} vs time base {tb}: not valid in this run (Dolphin reads 0)"]
    pct = lambda key: 100 * gp[key] / clks
    lines.append(f"  GP time: rasterizer busy {pct('ras_busy'):.1f}% (busiest 500 ms {gp['ras_peak_permille'] / 10:.1f}%), "
                 f"XF waiting for input {pct('xf_wait_in'):.1f}%, waiting for output {pct('xf_wait_out'):.1f}%, "
                 f"EFB copies {pct('copy_clks'):.1f}% of {clks / 1e6:.0f} M GP clocks")
    frames = row.get("avg_fps", 0) * row["wall_us"] / 1e6
    if frames >= 1:
        reject = lambda i, o: f"{100 * (1 - gp[o] / gp[i]):.0f}%" if gp[i] else "n/a"
        lines.append(f"  GP pixels per frame ({frames:.0f} frames): blended {gp['blend_in'] / frames / 1e3:.0f} k "
                     f"({gp['blend_in'] / frames / (640 * 480):.2f} x 640x480); "
                     f"early Z in {gp['ztop_in'] / frames / 1e3:.0f} k, {reject('ztop_in', 'ztop_out')} rejected; "
                     f"late Z in {gp['z_in'] / frames / 1e3:.0f} k, {reject('z_in', 'z_out')} rejected")
    wait = milliseconds(stages.get("gx_wait", {}))
    if wait is not None:
        lines.append(f"  CPU waiting for the GP (GX_DrawDone): {wait:.1f} ms ({100 * wait * 1e3 / row['wall_us']:.1f}% wall)")
    if overflows == 0 and pct("ras_busy") < 50 and gp["ras_peak_permille"] < 900:
        lines.append("  GP verdict: the GP has spare time and never blocks the CPU; "
                     "this scene is limited by CPU-side graphics, not GP throughput")
    elif pct("xf_wait_out") > pct("ras_busy") / 2:
        lines.append("  GP verdict: busy GP, XF held up by setup/raster/TEV/PE: fill or shading cost")
    else:
        lines.append("  GP verdict: busy GP, XF not held up by later stages: transform/command cost")
    return lines


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
        for line in gpu_lines(row, stages):
            print(line)
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
        for line in opcode_lines(row):
            print(line)
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
            "graphics": ("gfx_list", "gfx_command", "vertex", "gfx_state", "tex_hash", "tex_lookup",
                         "tex_load", "tex_activate", "draw_triangles", "draw_rect"),
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
        print("  Not directly measured: GP time per unit beyond the GP counters above, host scheduler/other IRQ work, "
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
