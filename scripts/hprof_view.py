#!/usr/bin/env python3
"""Decode bounded Wii64 emulation-thread samples against the frozen matching ELF."""
import argparse
import bisect
import hashlib
import pathlib
import shutil
import struct
import subprocess
from collections import Counter

HEADER = struct.Struct(">16I")
CAP = 128 * 1024


def decode(data):
    if len(data) < HEADER.size:
        raise ValueError("truncated hprof header")
    fields = HEADER.unpack_from(data)
    (magic, version, base, end, shift, count, period, total, jit, other,
     requested, started, restored, overflow, reserved, scope) = fields
    if magic != 0x48363450 or version != 1 or scope != 1:
        raise ValueError("unsupported hprof format/scope")
    if not 0x80000000 <= base < end <= 0x81800000 or base % 4 or end % 4:
        raise ValueError("invalid native text range")
    if not 5 <= shift <= 17 or not 0 < count <= CAP // 4:
        raise ValueError("invalid histogram geometry")
    if count != (end - base + (1 << shift) - 1) >> shift:
        raise ValueError("histogram does not cover text exactly")
    if not 72900 <= period < 0x80000000 or reserved != CAP:
        raise ValueError("invalid period or histogram allocation failed")
    if any(v not in (0, 1) for v in (requested, started, restored, overflow)):
        raise ValueError("invalid lifecycle flags")
    if overflow or (requested and (not started or not restored)):
        raise ValueError("overflow or sampler lifecycle failure")
    if not requested and (started or restored or total):
        raise ValueError("control contains sampler state")
    if len(data) != HEADER.size + count * 4:
        raise ValueError("wrong hprof length")
    buckets = struct.unpack_from(f">{count}I", data, HEADER.size)
    if sum(buckets) + jit + other != total:
        raise ValueError("inconsistent sample totals")
    if requested and not total:
        raise ValueError("sampler collected no samples")
    return dict(base=base, end=end, shift=shift, period=period, total=total,
                jit=jit, other=other, requested=requested, buckets=buckets)


def symbols(elf, nm):
    output = subprocess.check_output([nm, "-C", "-S", "-n", str(elf)], text=True)
    bounds, functions = {}, {}
    for line in output.splitlines():
        parts = line.split(None, 3)
        if len(parts) == 3 and parts[2] in {"hprof_text_start", "hprof_text_end"}:
            bounds[parts[2]] = int(parts[0], 16)
        if len(parts) == 4 and parts[2] in {"t", "T", "w", "W"}:
            try:
                addr, size = int(parts[0], 16), int(parts[1], 16)
            except ValueError:
                continue
            if size:
                functions.setdefault((addr, size), parts[3])  # aliases count once
    return bounds, sorted((a, a + n, name) for (a, n), name in functions.items())


def attribute(profile, functions):
    starts = [f[0] for f in functions]
    totals = Counter({"[JIT heap]": profile["jit"], "[other PC]": profile["other"]})
    ambiguous = 0
    for i, samples in enumerate(profile["buckets"]):
        if not samples:
            continue
        lo = profile["base"] + (i << profile["shift"])
        hi = min(lo + (1 << profile["shift"]), profile["end"])
        # Include all overlapping symbols, even an outer symbol spanning several leaves.
        candidates = [(min(hi, end) - max(lo, start), name)
                      for start, end, name in functions[:bisect.bisect_left(starts, hi)]
                      if end > lo]
        if candidates:
            overlap, name = max(candidates)
            if len(candidates) > 1 or overlap < hi - lo:
                ambiguous += samples
                name = f"[boundary {lo:08x}: ~{name}]"
            totals[name] += samples
        else:
            totals["[unmapped text]"] += samples
    return totals, ambiguous


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("profile", type=pathlib.Path)
    parser.add_argument("elf", type=pathlib.Path)
    parser.add_argument("--nm", default=shutil.which("powerpc-eabi-nm"))
    parser.add_argument("--top", type=int, default=25)
    args = parser.parse_args()
    try:
        if not args.nm:
            raise ValueError("powerpc-eabi-nm missing; source .dev/env.sh or pass --nm")
        profile = decode(args.profile.read_bytes())
        bounds, functions = symbols(args.elf, args.nm)
        if (bounds.get("hprof_text_start"), bounds.get("hprof_text_end")) != (profile["base"], profile["end"]):
            raise ValueError("ELF text bounds mismatch; use the frozen matching ELF")
        totals, ambiguous = attribute(profile, functions)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"hprof: {error}\n")
    print(f"ELF SHA256: {hashlib.sha256(args.elf.read_bytes()).hexdigest()}")
    print(f"Emulation-thread sampled residence; not busy time or inclusive wall time. "
          f"{'SAMPLER' if profile['requested'] else 'CONTROL'}")
    print(f"samples={profile['total']} bucket_bytes={1 << profile['shift']} "
          f"period_cycles={profile['period']} boundary/overlap_samples={ambiguous}")
    print("share %  samples  native symbol (bucket attribution is approximate)")
    for name, count in totals.most_common(args.top):
        share = 100 * count / profile["total"] if profile["total"] else 0
        print(f"{share:7.2f} {count:8d}  {name}")


if __name__ == "__main__":
    main()
