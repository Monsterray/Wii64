#!/usr/bin/env python3
"""padtest.py padtrace_NN.csv

Check a pad sweep end to end. diag.cfg padsweep= makes the GameCube driver on port 1 read
a generated sweep instead of the pad (gc_input/controller-GC.c); the PERF_PROF build logs
each change of what the game then read (the 4 bytes pif.c puts in the reply) next to the
raw reading that produced it. This checks, from what the game actually received:

  - each main-stick axis reaches every N64 value from -82 to +82, in order, with the
    other axis at 0 and 0 at rest -- no dead span, no crosstalk;
  - the C-stick drives only the C buttons, each past a 48-unit raw threshold;
  - along the stick's rim the output traces the N64 gate: 82 at the cardinals, (70, 70)
    at the diagonals, nothing outside it;
  - every button pressed alone sets exactly one N64 bit, a different bit each.

The stick maths itself is covered exhaustively on the host by tests/n64_analog_test.c;
this is the proof that the path from the driver to the game carries it intact.
"""
import csv, math, sys

N64_BITS = {  # byte 0 then byte 1 of the reply, as the game sees them
    0x8000: "A", 0x4000: "B", 0x2000: "Z", 0x1000: "Start",
    0x0800: "D-Up", 0x0400: "D-Down", 0x0200: "D-Left", 0x0100: "D-Right",
    0x0020: "L", 0x0010: "R", 0x0008: "C-Up", 0x0004: "C-Down", 0x0002: "C-Left", 0x0001: "C-Right",
}
GC_BTNS = {
    0x0001: "D-Left", 0x0002: "D-Right", 0x0004: "D-Down", 0x0008: "D-Up", 0x0010: "Z",
    0x0020: "R", 0x0040: "L", 0x0100: "A", 0x0200: "B", 0x0400: "X", 0x0800: "Y", 0x1000: "Start",
}
R0, ALPHA = 82.0, 1.39414574


def gate(a):
    return R0 * math.sin(ALPHA) / math.sin(math.pi - a - ALPHA)


def s8(v):
    return v - 256 if v > 127 else v


def load(path):
    rows = []
    for r in csv.DictReader(open(path)):
        if r["ctrl"] != "0":
            continue
        v = int(r["value"], 16)
        rows.append(dict(vi=int(r["vi"]), sx=int(r["sx"]), sy=int(r["sy"]), cx=int(r["cx"]), cy=int(r["cy"]),
                         btns=int(r["btns"]), b=v >> 16, x=s8((v >> 8) & 0xFF), y=s8(v & 0xFF)))
    return rows


fails = 0


def check(ok, msg):
    global fails
    print(("  ok   " if ok else "  FAIL ") + msg)
    fails += 0 if ok else 1


def axis(rows, name, raw, out, other):
    pts = sorted({(r[raw], r[out], r[other]) for r in rows})
    if not pts:
        check(False, f"{name}: no samples (did the sweep reach it?)")
        return
    outs = {p[1] for p in pts}
    missing = [v for v in range(-82, 83) if v not in outs]
    back = [p for a, p in zip(pts, pts[1:]) if p[1] < a[1]]
    cross = {p[2] for p in pts} - {0}
    rest = {p[1] for p in pts if p[0] == 0}
    check(not missing, f"{name}: {len(outs)} distinct values, {min(outs)}..{max(outs)}"
          + (f" -- missing {missing[:8]}{'...' if len(missing) > 8 else ''}" if missing else ""))
    check(not back, f"{name}: never goes backwards" + (f" (at raw {back[0][0]})" if back else ""))
    check(not cross, f"{name}: other axis stays 0" + (f" (saw {sorted(cross)[:5]})" if cross else ""))
    check(rest <= {0}, f"{name}: rest reads 0" + (f" (reads {sorted(rest)})" if rest - {0} else ""))
    raws = {p[0] for p in pts}
    print(f"       raw values seen: {len(raws)} of 256")


def cstick(rows, raw, lo_bit, hi_bit, name):
    bad = [r for r in rows if bool(r["b"] & lo_bit) != (r[raw] < -48) or bool(r["b"] & hi_bit) != (r[raw] > 48)
           or r["x"] or r["y"] or r["b"] & ~(lo_bit | hi_bit)]
    check(rows and not bad, f"{name}: {len(rows)} samples, C buttons exactly past +/-48, stick untouched"
          + (f" -- first bad raw {bad[0][raw]} -> b={bad[0]['b']:04x} x={bad[0]['x']} y={bad[0]['y']}" if bad else ""))


def main():
    rows = load(sys.argv[1])
    if not rows:
        sys.exit("no port-1 rows in the trace")
    print(f"{sys.argv[1]}: {len(rows)} changes, VI {rows[0]['vi']}..{rows[-1]['vi']}")
    idle = lambda r, *keep: all(r[k] == 0 for k in ("sx", "sy", "cx", "cy", "btns") if k not in keep)

    print("main stick")
    axis([r for r in rows if idle(r, "sx")], "X axis", "sx", "x", "y")
    axis([r for r in rows if idle(r, "sy")], "Y axis", "sy", "y", "x")

    print("C-stick")
    cstick([r for r in rows if idle(r, "cx") and r["cx"]], "cx", 0x0002, 0x0001, "C-stick X")
    cstick([r for r in rows if idle(r, "cy") and r["cy"]], "cy", 0x0004, 0x0008, "C-stick Y")

    print("rim")
    rim = [r for r in rows if idle(r, "sx", "sy") and r["sx"] and r["sy"] and math.hypot(r["sx"], r["sy"]) > 60]
    if rim:
        ratios, outside, card, diag = [], [], 0, (0, 0)
        for r in rim:
            m = math.hypot(r["x"], r["y"])
            a = math.atan2(min(abs(r["x"]), abs(r["y"])), max(abs(r["x"]), abs(r["y"])))
            ratios.append(m / gate(a))
            if m > gate(a) + 0.75:
                outside.append(r)
            diag = max(diag, (min(abs(r["x"]), abs(r["y"])), max(abs(r["x"]), abs(r["y"]))))
        full = [r for r in rows if idle(r, "sx", "sy") and (r["sx"], r["sy"]) != (0, 0)]
        card = max(max(abs(r["x"]), abs(r["y"])) for r in full)
        check(not outside, f"rim: {len(rim)} samples, radius / N64 gate {min(ratios):.3f}..{max(ratios):.3f}"
              + (f" -- {len(outside)} outside the gate" if outside else ""))
        check(card == 82, f"rim: cardinal reach {card} (want 82)")
        check(69 <= diag[0] <= 71, f"rim: best diagonal ({diag[0]}, {diag[1]}) (want about (70, 70))")
    else:
        check(False, "rim: no samples")

    print("buttons")
    seen = {}
    for r in rows:
        if idle(r, "btns") and r["btns"] and (r["btns"] & (r["btns"] - 1)) == 0:
            seen.setdefault(r["btns"], set()).add(r["b"])
    single = {}
    for gc, outs in sorted(seen.items()):
        outs -= {0}
        name = GC_BTNS.get(gc, hex(gc))
        if len(outs) == 1 and bin(next(iter(outs))).count("1") == 1:
            bit = next(iter(outs))
            single[gc] = bit
            print(f"       GC {name:<8} -> N64 {N64_BITS.get(bit, hex(bit))}")
        elif not outs:
            print(f"       GC {name:<8} -> (nothing: unmapped by default)")
        else:
            check(False, f"GC {name} -> {sorted(hex(o) for o in outs)} (not a single bit)")
    check(len(set(single.values())) == len(single) and single,
          f"buttons: {len(single)} buttons each set one N64 bit of its own")

    print(f"\n{'all checks passed' if not fails else f'{fails} failure(s)'}")
    sys.exit(1 if fails else 0)


if __name__ == "__main__":
    main()
