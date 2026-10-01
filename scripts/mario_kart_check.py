#!/usr/bin/env python3
"""Catch MK64's stalled graphics/audio path, not merely completed VIs."""
import argparse
from pathlib import Path

from subsystem_report import load


def check(run):
    rows = [r for r in load(run) if Path(r['rom']).name.lower() == 'mario kart 64.v64']
    if not rows:
        return ['no Mario Kart result']
    errors = []
    for i, row in enumerate(rows, 1):
        stages = row['subsystems']
        if not {'rsp_gfx', 'rsp_audio'} <= stages.keys():
            errors.append(f'entry {i}: subsystem probes absent; use PERF_PROF and PERF_SUBSYSTEM_PROBES')
            continue
        gfx = stages.get('rsp_gfx', {}).get('calls', 0)
        audio = stages.get('rsp_audio', {}).get('calls', 0)
        fps = row.get('avg_fps', 0)
        if row.get('how') != 'vis' or row.get('vis', 0) < 600:
            errors.append(f'entry {i}: incomplete test (at least 600 VIs required)')
        if fps < 10 or gfx < 100 or audio < 100:
            errors.append(f'entry {i}: stalled activity: {fps:.1f} DL/s, {gfx} graphics tasks, {audio} audio tasks')
    return errors


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    args = parser.parse_args()
    problems = check(args.run)
    for problem in problems:
        print('MK64 FAIL: ' + problem)
    if not problems:
        print('MK64 PASS: sustained graphics/audio activity (visual gameplay still needs confirmation)')
    raise SystemExit(bool(problems))
