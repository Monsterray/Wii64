#!/usr/bin/env python3
"""Separate private heap capacity from newlib/arena space; compare census cost."""
import hashlib
import json
from pathlib import Path
import re
import sys

def fields(line):
    return dict(re.findall(r'(\w+)=([^\s]+)', line))

def read(root):
    rows, heaps, ops, games = {}, {}, {}, []
    log = (Path(root) / 'perf.log').read_text(errors='replace')
    for line in log.splitlines():
        if line.startswith('memory: '):
            row = fields(line)
            n = int(row['n'])
            if n in rows: raise ValueError('duplicate memory snapshot')
            rows[n] = row
        elif line.startswith(('memory_heap: ', 'memory_ops: ')):
            row = fields(line)
            key = int(row['n']), row['pool']
            target = heaps if line.startswith('memory_heap: ') else ops
            if key in target: raise ValueError('duplicate heap row')
            target[key] = row
        elif line.startswith('game: '):
            games.append(fields(line))
        elif line.startswith('vm_io: ') and int(fields(line)['errors']):
            raise ValueError('ROM paging I/O failed')
    if heaps.keys() != ops.keys(): raise ValueError('incomplete heap/operation pairs')
    for key, row in heaps.items():
        if key[0] not in rows: raise ValueError('heap without snapshot')
        values = {k: int(row[k]) for k in ('capacity','live','peak','free','largest','min_free','blocks','peak_blocks','retired')}
        if any(v < 0 for v in values.values()): raise ValueError('negative heap size')
        if (values['live'] + values['free'] != values['capacity'] or
            values['live'] > values['peak'] or values['blocks'] > values['peak_blocks'] or
            values['largest'] > values['free'] or values['retired'] > values['live'] or
            int(ops[key]['errors'])):
            raise ValueError('heap accounting failed: ' + str(key))
    return log, rows, heaps, ops, games

def report(root):
    log, rows, heaps, ops, games = read(root)
    if not rows or not heaps: raise ValueError('no enabled memory census')
    if 'boxart_probe: 16 buffers in 768 KiB PASS' not in log:
        raise ValueError('native boxart bound did not pass')
    print('Private heap charges include allocator padding. Peaks are cumulative across ROMs.')
    print('Largest free is a raw block charge, not the maximum allocatable payload.')
    print('| Pool | Capacity | Peak | Last live | Largest free block | Failures | Peak retired |')
    print('|---|---:|---:|---:|---:|---:|---:|')
    for pool in sorted({key[1] for key in heaps}):
        key = max(key for key in heaps if key[1] == pool)
        r, o = heaps[key], ops[key]
        print(f"| {pool} | {r['capacity']} | {r['peak']} | {r['live']} | {r['largest']} | {o['failures']} | {r['peak_retired']} |")
    print('Snapshots:', ', '.join(rows[n]['stage'] for n in sorted(rows)))
    print('Do not sum private-heap, malloc and arena free bytes into an allocation guarantee.')
    return games

def compare(roots):
    # Runtime modes and same executable are prerequisites, not inferred from counters.
    hashes, settings = [], []
    for root, enabled in zip(map(Path, roots), (0, 1, 0)):
        config = (root / 'diag.cfg').read_text().splitlines()
        if [x for x in config if x.startswith('memory=')] != [f'memory={enabled}']:
            raise ValueError('expected off/on/off runtime modes')
        settings.append([x for x in config if x.strip() and not x.startswith('#')
                         and not x.startswith(('memory=', 'result_host='))])
        artifact = json.loads((root / 'artifacts.json').read_text())
        pair = {}
        for path, digest in artifact.items():
            if Path(path).suffix in ('.dol', '.elf'):
                if not Path(path).is_file() or hashlib.sha256(Path(path).read_bytes()).hexdigest() != digest:
                    raise ValueError('frozen artifact changed')
                pair[Path(path).suffix] = digest
        if pair.keys() != {'.dol', '.elf'}: raise ValueError('missing DOL/ELF hashes')
        hashes.append(pair)
    if not all(pair == hashes[0] for pair in hashes): raise ValueError('different executables')
    if not all(config == settings[0] for config in settings): raise ValueError('different scenes/settings')
    games = [read(root)[4] for root in roots]
    report(roots[1])
    if not games[0] or not all(len(g) == len(games[0]) for g in games):
        raise ValueError('incomplete chain')
    passed = True
    print('| Entry | CPU-cycle change | Cycle drift | Non-sleep change | Non-sleep drift | Guest parity | Audio underruns/overruns |')
    print('|---|---:|---:|---:|---:|---|---|')
    for i, triple in enumerate(zip(*games), 1):
        if any(r['how'] != 'vis' for r in triple): raise ValueError('chain did not complete')
        parity = all(len({r[k] for r in triple}) == 1
                     for k in ('vis','vi_rate','exceptions','cacheResets','recompiles','batches','verts'))
        cycles = [int(r['pmc1']) for r in triple]
        busy = [int(r['wall_us']) - int(r['sleep_us']) for r in triple]
        mean_cycles = (cycles[0] + cycles[2]) / 2
        mean_busy = (busy[0] + busy[2]) / 2
        if not mean_cycles or mean_busy <= 0: raise ValueError('missing Wii cycle/wall measurement')
        cost = 100 * (cycles[1] / mean_cycles - 1)
        drift = 100 * (cycles[2] / cycles[0] - 1) if cycles[0] else float('inf')
        wall = 100 * (busy[1] / mean_busy - 1)
        wall_drift = 100 * (busy[2] / busy[0] - 1) if busy[0] > 0 else float('inf')
        audio = [[int(r[k]) for r in triple] for k in ('underruns', 'overruns')]
        audio_ok = all(a[1] <= max(a[0], a[2]) for a in audio)
        audio_text = '; '.join('/'.join(map(str, a)) for a in audio)
        print(f'| {i} | {cost:+.3f}% | {drift:+.3f}% | {wall:+.3f}% | {wall_drift:+.3f}% | {parity} | {audio_text} |')
        passed &= parity and audio_ok and abs(drift) <= 1 and abs(wall_drift) <= 1 and cost <= 3 and wall <= 3
    print('Census calibration:', 'PASS' if passed else 'FAILED; timings are provisional, not an optimization claim')
    return passed

if __name__ == '__main__':
    try:
        if len(sys.argv) == 2: report(sys.argv[1])
        elif len(sys.argv) == 4: sys.exit(0 if compare(sys.argv[1:]) else 1)
        else: sys.exit('Usage: memory_report.py RUN [PROBE_RUN CONTROL2_RUN]')
    except (ValueError, KeyError, OSError) as error:
        sys.exit(str(error))
