#!/usr/bin/env python3
"""Reject stalled Zelda boots even when the VI target completes."""
import argparse
from pathlib import Path

from subsystem_report import load

ROMS = ('Zelda Ocarina of Time Master Quest.v64',
        "Legend of Zelda, The - Majora's Mask (E) (M4) [!].z64")


def check(run, both=False):
    rows = [r for r in load(run) if Path(r['rom']).name in ROMS]
    errors = []
    if not rows:
        return ['no Zelda results']
    if both and set(ROMS) - {Path(r['rom']).name for r in rows}:
        errors.append('missing one of the two Zelda results')
    for row in rows:
        name = Path(row['rom']).name
        if row.get('how') != 'vis' or row.get('vis', 0) < 600:
            errors.append(f'{name}: incomplete (at least 600 VIs required)')
        # These totals also reject the historical logs without subsystem probes.
        if (row.get('avg_fps', 0) < 5 or row.get('batches', 0) < 100
                or row.get('verts', 0) < 1000):
            errors.append(f'{name}: stalled graphics')
        stages = row['subsystems']
        if not {'rsp_gfx', 'rsp_audio'} <= stages.keys():
            errors.append(f'{name}: subsystem probes absent')
        elif min(stages[s]['calls'] for s in ('rsp_gfx', 'rsp_audio')) < 100:
            errors.append(f'{name}: insufficient graphics/audio activity')
        if row['vm_io'].get('errors', 0):
            errors.append(f'{name}: paging I/O errors')
    return errors


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('--both', action='store_true')
    args = parser.parse_args()
    problems = check(args.run, args.both)
    for problem in problems:
        print('Zelda FAIL: ' + problem)
    if not problems:
        print('Zelda PASS: graphics/audio active; inspect captures for scene correctness')
    raise SystemExit(bool(problems))
