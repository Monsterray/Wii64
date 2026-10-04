#!/usr/bin/env python3
import pathlib
import struct
import sys
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / "scripts"))
import hprof_view as hv

header = [0x48363450, 1, 0x80004000, 0x80004040, 5, 2, 729000,
          9, 2, 1, 1, 1, 1, 0, 131072, 1]
def blob(h=header, buckets=(2, 4)):
    return struct.pack(">16I", *h) + struct.pack(">2I", *buckets)

p = hv.decode(blob())
totals, ambiguous = hv.attribute(p, [(p["base"], p["base"] + 16, "short"),
                                    (p["base"] + 16, p["end"], "long")])
assert totals["long"] == 4 and totals["short"] == 0 and ambiguous == 2 and sum(totals.values()) == 9
assert totals[f'[boundary {p["base"]:08x}: ~short]'] == 2
for field, bad in [(0, 0), (1, 2), (2, 0), (3, header[2]), (4, 31), (5, 32769),
                   (6, 0), (7, 10), (10, 2), (11, 0), (12, 0), (13, 1), (14, 0), (15, 0)]:
    corrupt = header.copy()
    corrupt[field] = bad
    try:
        hv.decode(blob(corrupt))
        raise AssertionError((field, bad))
    except ValueError:
        pass
for corrupt in [blob()[:-1], blob() + b"\0", b""]:
    try:
        hv.decode(corrupt)
        raise AssertionError("accepted incorrect length")
    except ValueError:
        pass
control = header.copy()
for i in (7, 8, 9, 10, 11, 12): control[i] = 0
assert hv.decode(blob(control, (0, 0)))["total"] == 0
print("hprof decoder: PASS")
