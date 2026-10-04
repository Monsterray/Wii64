#!/usr/bin/env python3
"""Calibrate same-binary control/sampler/control. Cycles belong to controls only."""
import argparse
import hashlib
import json
from pathlib import Path
from statistics import mean
from hprof_view import decode
from subsystem_report import load
from library_report import capture_status


def compare(roots):
    rows = [load(root) for root in roots]
    if not rows[0] or len({len(r) for r in rows}) != 1:
        raise ValueError('missing or unequal chain coverage')
    artifacts = [json.loads((r/'artifacts.json').read_text()) for r in roots]
    for suffix in ('.dol', '.elf'):
        hashes = [{v for k,v in a.items() if k.endswith(suffix)} for a in artifacts]
        if len(hashes[0]) != 1 or any(h != hashes[0] for h in hashes):
            raise ValueError('controls and sampler must use one frozen DOL/ELF')
        # The decoder performs the text-range check against this matching ELF.
        source = next(k for k in artifacts[0] if k.endswith(suffix))
        if hashlib.sha256(Path(source).read_bytes()).hexdigest() not in hashes[0]:
            raise ValueError('frozen artifact changed')
    passed = True
    print('| ROM | Control non-sleep ms | Sampler ms | Added % | Control drift % | Result |')
    print('|---|---:|---:|---:|---:|---|')
    for i, triple in enumerate(zip(*rows), 1):
        issues = []
        for j, (root, row) in enumerate(zip(roots, triple)):
            p = decode((root/f'hprof_{i:02d}.bin').read_bytes())
            if p['requested'] != (j == 1): issues.append('wrong sampler mode')
            if row.get('how') != 'vis' or row['vm_io'].get('errors', 0): issues.append('incomplete/VM error')
            if row.get('overruns', 0): issues.append('audio overrun')
            if capture_status(root/f'xfb_{i:02d}.bin') != 'nonflat/review': issues.append('frame review required')
            if j == 1 and any(row.get(f'pmc{n}', 0) for n in range(1,5)): issues.append('invalid sampler PMC totals')
        for key in ('rom','vis','vi_rate','exceptions','cacheResets'):
            if len({r.get(key) for r in triple}) != 1: issues.append('guest/config mismatch: '+key)
        # JIT blocks and display lists can vary with scheduling; report rather than hide this.
        for key in ('recompiles','batches','verts'):
            values = [r.get(key, 0) for r in triple]
            if max(values)-min(values) > max(2, .01*max(values)):
                issues.append('work mismatch: '+key)
        if triple[1].get('underruns',0) > max(r.get('underruns',0) for r in (triple[0],triple[2])):
            issues.append('extra audio underruns')
        times = [r['wall_us']-r.get('sleep_us',0) for r in triple]
        if min(times) <= 0: raise ValueError('nonpositive non-sleep estimate')
        baseline = mean((times[0], times[2]))
        cost = 100*(times[1]/baseline-1)
        drift = 100*abs(times[2]-times[0])/baseline
        if drift > 3: issues.append('unstable controls')
        if cost > 3: issues.append('excess probe cost')
        status = '; '.join(dict.fromkeys(issues)) or 'initial wall-cost gate passed'
        passed &= not issues
        print(f'| {Path(triple[0]["rom"]).name} | {baseline/1000:.3f} | {times[1]/1000:.3f} | '
              f'{cost:+.2f} | {drift:.2f} | {status} |')
    print('Non-sleep = wall minus requested limiter sleep, not measured busy CPU. '
          'No cycle-overhead claim; no rankings accepted without scene/capture review.')
    return passed


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directories', nargs=3, type=Path)
    args = parser.parse_args()
    try:
        passed = compare(args.directories)
    except (ValueError, OSError, KeyError) as error:
        parser.exit(1, f'hprof calibration: {error}\n')
    raise SystemExit(0 if passed else 1)
