"""Exercise the boot gate with real historical and current logs, plus failure cases."""
from pathlib import Path
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / 'scripts'))
from zelda_check import ROMS, check


def entry(rom, gfx=400, audio=800, fps=20, vis=900, how='vis', io=0):
    return (f'subsystem_time: stage=rsp_gfx calls={gfx} timed_calls={gfx} period=1 timed_us=100\n'
            f'subsystem_time: stage=rsp_audio calls={audio} timed_calls={audio} period=1 timed_us=100\n'
            f'vm_io: errors={io}\n'
            f'game: n=1/1 how={how} vis={vis} avg_fps={fps} batches=1000 verts=2000 rom=sd:/wii64/roms/{rom}\n')


with tempfile.TemporaryDirectory(prefix='wii64-zelda-check-') as directory:
    path = Path(directory) / 'perf.log'
    for rom in ROMS:
        path.write_text(entry(rom))
        assert not check(path)
        assert check(path, both=True)
        for options in ({'gfx': 0}, {'audio': 0}, {'fps': .2}, {'vis': 599},
                        {'how': 'timeout'}, {'io': 1}):
            path.write_text(entry(rom, **options))
            assert check(path), options
    path.write_text(''.join(entry(rom) for rom in ROMS))
    assert not check(path, both=True)
    path.write_text(entry(ROMS[0]) + entry(ROMS[1], audio=0))
    assert check(path, both=True)
    path.write_text(entry('Super Mario 64.v64'))
    assert check(path)
    for plugin in ('glN64', 'Rice'):
        failures = check(root / f'baselines/2026-09-22_dolphin_all_{plugin}')
        assert sum('stalled graphics' in e for e in failures) == 2
print('Zelda activity gate: PASS (historical stalls rejected)')
