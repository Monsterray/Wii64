#!/usr/bin/env python3
"""dtm2input.py MOVIE.dtm [--offset N | --perf perf.log] [--vi-rate 60|50] [--port 1] [--out FILE] [--events]
   dtm2input.py --selftest

Turn a Dolphin input movie recorded on Wii64 into a pad replay for a chain entry:
    chain=<vis>,input=<name> sd:/wii64/roms/<rom>
reads sd:/wii64/input/<name>.txt (put the file in scripts/inputs/; .dev/stage_roms.sh copies
it onto the SD card). Dolphin's own movie playback (-m) does not work in the installed
build (WiiStation's Docs/CONTROLLER_TESTING.md), so Wii64 replays the recording itself, at
the one point a real pad is read (gc_input/controller-GC.c), keyed on the game's own VIs.

Output lines: "<guest VI> <PAD_BUTTON_* mask hex> <sx> <sy> <cx> <cy>", one per change, at
most one per VI. Raw GameCube values, so the replay goes through the same button mapping and
stick maths as a real pad.

Timing. A movie frame is one host VI field (Dolphin's MovieManager::FrameUpdate), counted
from boot. Guest VI = (movie frame - offset) * vi_rate / 60. The offset is the host frame on
which the game's first VI ran. `.dev/wii64_diag.sh record <dol> <rom>` sets up a recording
that starts at power-on and autoboots the ROM on the same path a chain replay takes; with a
PERF_PROF dol, that run's perf.log says "first_vi: vi0_retrace=N", and --perf reads N from
it. (A chain's game: line carries vi0_retrace too.) vi_rate is 50 for a PAL ROM (the game: line's
vi_rate). The recording must have run at full speed, or host frames and guest VIs drift.
--events prints every button change with its movie frame and guest VI, to check the offset
against a press you remember.

Format (Dolphin Source/Core/Core/Movie.h): 256-byte header, then records in poll order with
no frame tags. A GameCube pad poll is 8 bytes (ControllerState: buttons0, buttons1 with
is_connected = 0x40, L, R, stickX, stickY, cX, cY; sticks centred on 128); a Wiimote record
is a size byte then that many bytes, whose first byte has 0x40 clear. Polls cycle through
the connected GameCube ports in order.
"""
import argparse, re, struct, sys

# ControllerState bits -> libogc PAD_BUTTON_* (ogc/pad.h)
B0 = [0x1000, 0x0100, 0x0200, 0x0400, 0x0800, 0x0010, 0x0008, 0x0004]  # Start A B X Y Z Up Down
B1 = [0x0001, 0x0002, 0x0040, 0x0020]                                  # Left Right L R
NAMES = {0x1000: "Start", 0x0100: "A", 0x0200: "B", 0x0400: "X", 0x0800: "Y", 0x0010: "Z",
         0x0008: "Up", 0x0004: "Down", 0x0001: "Left", 0x0002: "Right", 0x0040: "L", 0x0020: "R"}


def parse(data, port):
    if data[:4] != b"DTM\x1a":
        sys.exit("not a Dolphin movie (no DTM header)")
    ports = [p for p in range(4) if data[0x0B] & (1 << p)]
    if port not in ports:
        sys.exit(f"GameCube port {port + 1} was not plugged in when this was recorded (ports: {[p + 1 for p in ports]})")
    hdr = {"from_savestate": data[0x0C], "frames": struct.unpack_from("<Q", data, 0x0D)[0]}
    polls, pos, n, wm = [], 256, 0, 0
    while pos + 2 <= len(data):
        if data[pos + 1] & 0x40:
            if ports[n % len(ports)] == port:
                b0, b1, _, _, sx, sy, cx, cy = data[pos:pos + 8]
                mask = sum(v for i, v in enumerate(B0) if b0 >> i & 1) + sum(v for i, v in enumerate(B1) if b1 >> i & 1)
                polls.append((mask, sx - 128, sy - 128, cx - 128, cy - 128))
            n += 1
            pos += 8
        else:
            pos += 1 + data[pos]
            wm += 1
    hdr["wiimote_records"] = wm
    return hdr, polls


def convert(hdr, polls, offset, vi_rate):
    """[(vi, state)] with one entry per VI (the last poll in it), changes only."""
    ppf = len(polls) / hdr["frames"] if hdr["frames"] else 1.0
    by_vi = {}
    for i, st in enumerate(polls):
        by_vi[max(0, round((i / ppf - offset) * vi_rate / 60))] = st
    out, last = [], (0, 0, 0, 0, 0)
    for vi in sorted(by_vi):
        if by_vi[vi] != last:
            out.append((vi, by_vi[vi]))
            last = by_vi[vi]
    return out, ppf


def names(mask):
    return " ".join(n for v, n in NAMES.items() if mask & v)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("movie", nargs="?")
    ap.add_argument("--offset", type=float, default=0.0)
    ap.add_argument("--perf", help="the recording run's perf.log: --offset from its first_vi line")
    ap.add_argument("--vi-rate", type=float, default=60.0)
    ap.add_argument("--port", type=int, default=1)
    ap.add_argument("--out", default="")
    ap.add_argument("--events", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    a = ap.parse_args()
    if a.selftest:
        return selftest()
    if not a.movie:
        ap.error("MOVIE.dtm required")
    if a.perf:
        m = re.search(r"vi0_retrace=(\d+)", open(a.perf, encoding="latin-1").read())
        if not m:
            sys.exit(f"{a.perf}: no vi0_retrace (not a PERF_PROF build, or the game never started)")
        a.offset = float(m.group(1))
    hdr, polls = parse(open(a.movie, "rb").read(), a.port - 1)
    recs, ppf = convert(hdr, polls, a.offset, a.vi_rate)
    print(f"# {a.movie}: {hdr['frames']} frames ({hdr['frames'] / 60:.0f} s), {len(polls)} polls on port {a.port} "
          f"({ppf:.2f}/frame), {hdr['wiimote_records']} Wiimote records -> {len(recs)} replay lines", file=sys.stderr)
    if hdr["from_savestate"]:
        print("# WARNING: recorded from a save state, not from boot -- frame 0 is not boot, so --offset is unknown",
              file=sys.stderr)
    if a.events:
        last = 0
        for i, st in enumerate(polls):
            if st[0] != last:
                f = i / ppf
                print(f"frame {f:8.0f}  vi {(f - a.offset) * a.vi_rate / 60:8.0f}  {names(st[0]) or '-'}")
                last = st[0]
        return
    text = f"# scripts/dtm2input.py {a.movie} --offset {a.offset:g} --vi-rate {a.vi_rate:g}\n" + "".join(
        f"{vi} {m:04x} {sx} {sy} {cx} {cy}\n" for vi, (m, sx, sy, cx, cy) in recs)
    if a.out:
        open(a.out, "w", newline="\n").write(text)
    else:
        sys.stdout.write(text)


def selftest():
    """A two-port movie with Wiimote records mixed in: port 1's presses come out at the right
    VIs with libogc masks, port 2 and the Wiimote records are skipped."""
    hdr = bytearray(256)
    hdr[:4], hdr[0x0B] = b"DTM\x1a", 0x13          # GC ports 1 and 2, Wiimote 1
    struct.pack_into("<Q", hdr, 0x0D, 100)          # 100 frames
    body = bytearray()
    for i in range(200):                            # 2 polls per frame per port
        f = i // 2
        b0 = 0x01 if 50 <= f < 60 else 0            # Start held, frames 50-59
        b1 = 0x40 | (0x04 if f >= 90 else 0)        # L from frame 90
        sx = 128 + 82 if f >= 70 else 128           # stick right from frame 70
        body += bytes([b0, b1, 0, 0, sx, 128, 128, 20])
        body += bytes([0x02, 0x40, 0, 0, 128, 128, 128, 128])  # port 2: A, ignored
        if i % 3 == 0:
            body += bytes([3, 0x00, 1, 2])          # a Wiimote record
    h, polls = parse(bytes(hdr + body), 0)
    assert len(polls) == 200 and h["wiimote_records"] == 67, (len(polls), h)
    recs, _ = convert(h, polls, 10, 60)
    assert recs[0] == (0, (0, 0, 0, 0, -108)), recs[0]
    assert (40, (0x1000, 0, 0, 0, -108)) in recs and (50, (0, 0, 0, 0, -108)) in recs, recs
    assert (60, (0, 82, 0, 0, -108)) in recs and (80, (0x40, 82, 0, 0, -108)) in recs, recs
    pal, _ = convert(h, polls, 10, 50)
    assert pal[1][0] == 33, pal                     # (50 - 10) * 50/60
    print("selftest ok")


if __name__ == "__main__":
    main()
