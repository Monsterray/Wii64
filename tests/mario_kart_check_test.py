"""A completed VI target must not hide a dead graphics/audio pipeline."""
from pathlib import Path
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from mario_kart_check import check

def log(gfx=400, audio=800, fps=25.0, how='vis', vis=900):
    return (f'subsystem_time: stage=rsp_gfx calls={gfx} timed_calls={gfx} period=1 timed_us=100\n'
            f'subsystem_time: stage=rsp_audio calls={audio} timed_calls={audio} period=1 timed_us=100\n'
            f'game: n=1/1 how={how} vis={vis} avg_fps={fps} rom=sd:/wii64/roms/Mario Kart 64.v64\n')

with tempfile.TemporaryDirectory(prefix='wii64-kart-check-') as directory:
    path = Path(directory) / 'perf.log'
    for entry in (log(), log(vis=600)):
        path.write_text(entry)
        assert not check(path)
    for entry in (log(gfx=3, audio=0, fps=1.7), log(audio=0), log(gfx=0),
                  log(fps=1), log(how='timeout'), log(vis=599),
                  log().replace('Mario Kart 64.v64', 'Super Mario 64.v64')):
        path.write_text(entry)
        assert check(path), entry
    path.write_text(log() + log(audio=0))
    assert check(path), 'Every MK64 entry must pass, not only the first'
    path.write_text(log().splitlines()[-1] + '\n')
    assert 'probes absent' in check(path)[0]
print('Mario Kart activity gate: PASS')
