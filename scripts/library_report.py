#!/usr/bin/env python3
"""Separate progress, output, cadence warnings and strict 100% VI-speed results."""
import argparse
from collections import Counter
import json
from pathlib import Path
import re
import struct

from subsystem_report import load, speed


def capture_status(path):
    if not path.is_file():
        return 'missing'
    data = path.read_bytes()
    if len(data) < 16:
        return 'invalid'
    magic, width, height, _ = struct.unpack('>4I', data[:16])
    if magic != 0x57584642 or not width or height <= 64 or len(data) != 16 + width * height * 2:
        return 'invalid'
    # Exclude the FPS overlay. A flat frame is a review request, not proof of a hang.
    luma = data[16 + width * 64 * 2::2]
    return 'flat/review' if max(luma) - min(luma) < 8 else 'nonflat/review'


def assess(row, vi_windows, capture):
    stages = row['subsystems']
    gfx, audio = (stages.get(s, {}).get('calls') for s in ('rsp_gfx', 'rsp_audio'))
    rate = row.get('vi_rate', 0)
    # First four samples include cold compilation/fades. Still retained in raw logs.
    windows = vi_windows[4:]
    slow = sum(v < .95 * rate for v in windows) if rate else 0
    return dict(rom=Path(row['rom']).name, completed=row.get('how') == 'vis',
                vis=row.get('vis', 0), speed_pct=100 * speed(row),
                below_100=speed(row) < 1, gfx=gfx, audio=audio,
                inactive_video=(gfx < 100 or row.get('batches', 0) < 100) if gfx is not None else None,
                inactive_audio=audio < 100 if audio is not None else None, slow_vi_windows=slow,
                vi_windows=len(windows), underruns=row.get('underruns', 0),
                overruns=row.get('overruns', 0), io_errors=row['vm_io'].get('errors', 0),
                capture=capture)


def coverage(entries, results, platforms):
    expected = {e['rom'] for e in entries}
    output = {}
    for platform in platforms:
        selected = [r for r in results if r['platform'] == platform]
        counts = Counter(r['rom'] for r in selected)
        output[platform] = dict(expected=len(expected), recorded=len(selected),
                                missing=sorted(expected - counts.keys()),
                                unexpected=sorted(counts.keys() - expected),
                                duplicates=sorted(k for k, v in counts.items() if v > 1),
                                incomplete=sorted(r['rom'] for r in selected if not r['completed']))
    return output


def warnings(row):
    return [label for label, flagged in (
        ('incomplete VI target', not row['completed']),
        ('below 100%', row['below_100']), ('inactive video', row['inactive_video']),
        ('inactive audio', row['inactive_audio']), ('paging error', row['io_errors']),
        ('silent DSP capture', row.get('pcm_nonzero') == 0),
        ('slow VI windows', row['slow_vi_windows']),
        ('audio underruns', row['underruns']), ('audio overruns', row['overruns'])) if flagged]


def report(root):
    manifest = json.loads((root / 'manifest.json').read_text())
    results = []
    platforms = manifest.get('platforms', ['dolphin', 'hardware'])
    for platform in platforms:
        index = root / f'{platform}_runs.txt'
        if not index.exists():
            continue
        for name in index.read_text().splitlines():
            run = Path(name)
            rows = load(run)
            samples, windows = [], []
            for line in (run / 'perf.log').read_text(encoding='latin1').replace('\0', '').splitlines():
                if line.startswith('mark: loadROM: enter'): samples = []
                elif line.startswith('vis: '): samples.append(float(line[5:]))
                elif line.startswith('game: '): windows.append(samples); samples = []
            for i, row in enumerate(rows):
                n = int(str(row['n']).split('/')[0])
                result = assess(row, windows[i] if i < len(windows) else [],
                                capture_status(run / f'xfb_{n:02d}.bin'))
                result.update(platform=platform, run=str(run.resolve()),
                              requested_vis=next((e['vis'] for e in manifest['entries']
                                                  if e['rom'] == result['rom']), None))
                result['completed'] &= result['vis'] == result['requested_vis']
                result['pcm_nonzero'] = result['pcm_peak'] = None
                if platform == 'dolphin':
                    # Dolphin can keep XFB copies GPU-only. A RAM dump is not a
                    # reliable screenshot with that setting; use Wii captures.
                    result['capture'] = 'unverified-XFB'
                    settings = run / 'dolphin-settings.json'
                    if settings.exists() and json.loads(settings.read_text()).get('xfb_to_texture') is False:
                        result['capture'] = capture_status(run / f'xfb_{n:02d}.bin')
                    entry = next((j for j, e in enumerate(manifest['entries'], 1)
                                  if e['rom'] == result['rom']), None)
                    log = root / f'dolphin-{entry}.log'
                    if log.exists():
                        signals = re.findall(r'_dspdump\d*\.wav: .*?nonzero=(\d+), peak=(\d+)', log.read_text())
                        if signals:
                            result['pcm_nonzero'] = sum(int(s[0]) for s in signals)
                            result['pcm_peak'] = max(int(s[1]) for s in signals)
                results.append(result)
    summary = coverage(manifest['entries'], results, platforms)
    for platform in platforms:
        exits = root / f'{platform}_exits.txt'
        summary[platform]['launcher_failures'] = [name for name, status in
            (line.split() for line in exits.read_text().splitlines()) if int(status)] if exits.exists() else []
        selected = [r for r in results if r['platform'] == platform]
        print(f'\n{platform}: speed is VI/wall time, not display-list rate. Strict 100% threshold.')
        print(f"Coverage: {len(selected)}/{summary[platform]['expected']} ROM files recorded.")
        print('| ROM | Speed % | Gfx/audio calls | Slow VI windows | Underrun/overrun | PCM peak | Capture |')
        print('|---|---:|---:|---:|---:|---:|---|')
        for row in selected:
            print(f"| {row['rom']} | {row['speed_pct']:.4f} | {row['gfx']}/{row['audio']} | "
                  f"{row['slow_vi_windows']}/{row['vi_windows']} | {row['underruns']}/{row['overruns']} | "
                  f"{row['pcm_peak'] if row['pcm_peak'] is not None else 'not captured'} | {row['capture']} |")
            checks = warnings(row)
            if checks: print('  CHECK: ' + '; '.join(checks))
        for key in ('missing', 'unexpected', 'duplicates', 'launcher_failures'):
            for rom in summary[platform][key]: print(key.upper() + ': ' + rom)
    (root / 'report.json').write_text(json.dumps(results, indent=2) + '\n')
    (root / 'coverage.json').write_text(json.dumps(summary, indent=2) + '\n')
    print('\nActivity is not proof of correct pixels or audible sound. Inspect captures and per-ROM WAV reports.')
    print('Slow VI windows (<95% nominal, after four samples) and audio gaps are stutter candidates, not listening tests.')
    print('Probes, compilation, paging and log flushes can lower measured speed; retest marginal failures with control flags.')
    print('Absent subsystem counters are unknown, not inactive. Wii PCM is not captured by this test.')
    return results


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    parser.add_argument('--check', action='store_true', help='exit 1 for incomplete coverage or any measured warning')
    args = parser.parse_args()
    rows = report(args.directory)
    if args.check:
        summary = json.loads((args.directory / 'coverage.json').read_text())
        failed = any(warnings(row) for row in rows) or any(
            s[k] for s in summary.values() for k in ('missing', 'unexpected', 'duplicates', 'incomplete', 'launcher_failures'))
        raise SystemExit(int(failed))
