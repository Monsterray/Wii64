#!/usr/bin/env python3
"""chain_table.py DIR [--json]

Read back a chained run (diag.cfg chain=, see main/main_gc-menu2.cpp): DIR holds the
perf.log, xfb_NN.bin and padtrace_NN.csv files a run leaves on the SD card -- whether
.dev/wii64_diag.sh chain copied them off Dolphin's SD image or they were pulled off a
real Wii's card. Prints one row per game from its "game:" line, and turns each
xfb_NN.bin (the frame on screen when the game stopped) into xfb_NN.png next to it.

Columns:
  speed   guest VIs per wall second / the ROM's VI rate: 1.00 = full speed. Under
          Dolphin this is Dolphin's speed, not a Wii's; on hardware it is the number.
  idle    share of wall time the frame limiter slept: the headroom. 0% with speed
          under 1.00 means the Wii could not keep up.
  ipc     PMC2/PMC1 = instructions completed per cycle (default PMC selects; hardware
          only -- Dolphin does not count instructions completed and reports 0).
"""
import json, os, re, struct, sys, zlib

GAME = re.compile(r"^game: (.*)$")


def parse_game(line):
    body = GAME.match(line).group(1)
    rom = None
    if " rom=" in body:
        body, rom = body.split(" rom=", 1)
    d = dict(kv.split("=", 1) for kv in body.split() if "=" in kv)
    d["rom"] = rom or ""
    for k, v in list(d.items()):
        try:
            d[k] = float(v) if "." in v else int(v)
        except ValueError:
            pass
    return d


def games(log_path):
    out = []
    with open(log_path, encoding="latin-1") as f:
        for line in f:
            # Dolphin's live Wii SD folder sync can expose FAT-cluster padding
            # as NULs while the guest appends to a file. Ignore those bytes so
            # completed game rows remain readable after the run.
            line = line.replace("\0", "").rstrip("\r\n")
            if GAME.match(line):
                out.append(parse_game(line))
    return out


def png(path, w, h, rgb):
    raw = b"".join(b"\x00" + rgb[y * w * 3:(y + 1) * w * 3] for y in range(h))
    chunk = lambda t, d: struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b""))


def xfb_to_png(src, dst):
    """libogc XFB: 16-byte 'WXFB' w h 0 header (big-endian), then YUYV 4:2:2 --
    each 4 bytes are Y0 U Y1 V for two pixels (BT.601)."""
    data = open(src, "rb").read()
    magic, w, h, _ = struct.unpack(">4I", data[:16])
    if magic != 0x57584642 or len(data) < 16 + w * h * 2:
        return False
    yuv = data[16:16 + w * h * 2]
    clamp = lambda v: 0 if v < 0 else (255 if v > 255 else int(v))
    rgb = bytearray(w * h * 3)
    o = 0
    for i in range(0, len(yuv), 4):
        y0, u, y1, v = yuv[i], yuv[i + 1] - 128, yuv[i + 2], yuv[i + 3] - 128
        for y in (y0, y1):
            rgb[o] = clamp(y + 1.402 * v)
            rgb[o + 1] = clamp(y - 0.344 * u - 0.714 * v)
            rgb[o + 2] = clamp(y + 1.772 * u)
            o += 3
    png(dst, w, h, bytes(rgb))
    return True


def main():
    d = sys.argv[1]
    log = os.path.join(d, "perf.log")
    if not os.path.exists(log):
        sys.exit(f"{log}: missing (was this a PERF_PROF build?)")
    rows = games(log)
    if "--json" in sys.argv:
        print(json.dumps(rows, indent=1))
        return
    if not rows:
        print("no game: lines in perf.log -- the chain did not finish a single game")
        return
    hdr = f"{'#':>3} {'rom':<34} {'how':<11} {'vis':>6} {'wall s':>7} {'speed':>5} {'idle':>5} {'fps':>5} {'exc':>7} {'recomp':>7} {'tree':>4} {'undr':>5} {'ovr':>4} {'ipc':>5} {'pad':>5} png"
    print(hdr)
    print("-" * len(hdr))
    for g in rows:
        n = str(g.get("n", "?")).split("/")[0]
        wall = g.get("wall_us", 0) / 1e6
        rate = g.get("vi_rate") or (50.0 if re.search(r"\((E|Europe|PAL)\)", g["rom"], re.I) else 60.0)
        speed = g.get("vis", 0) / wall / rate if wall else 0
        idle = g.get("sleep_us", 0) / g["wall_us"] * 100 if g.get("wall_us") else 0
        ipc = g.get("pmc2", 0) / g["pmc1"] if g.get("pmc1") else 0
        xfb = os.path.join(d, f"xfb_{int(n):02d}.bin") if n.isdigit() else ""
        shot = ""
        if xfb and os.path.exists(xfb) and xfb_to_png(xfb, xfb[:-4] + ".png"):
            shot = os.path.basename(xfb[:-4] + ".png")
        print(f"{n:>3} {os.path.basename(g['rom'])[:34]:<34} {g.get('how', ''):<11} {g.get('vis', 0):>6} "
              f"{wall:>7.1f} {speed:>5.2f} {idle:>4.0f}% {g.get('avg_fps', 0):>5.1f} {g.get('exceptions', 0):>7} "
              f"{g.get('recompiles', 0):>7} {g.get('treeDepthMax', 0):>4} {g.get('underruns', 0):>5} "
              f"{g.get('overruns', 0):>4} {ipc:>5.2f} {g.get('padtrace', 0):>5} {shot}")
    print(f"\nflush cost: {sum(g.get('flush_us', 0) for g in rows) / 1e3:.1f} ms over "
          f"{sum(g.get('flushes', 0) for g in rows)} flushes")


if __name__ == "__main__":
    main()
