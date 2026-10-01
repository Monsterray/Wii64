from pathlib import Path
import struct
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from library_report import assess, capture_status, coverage, warnings

row = dict(rom='Test.z64', how='vis', vis=900, wall_us=15000000, vi_rate=60,
           batches=1000, underruns=2, overruns=0, vm_io={'errors': 0},
           subsystems={'rsp_gfx': {'calls': 400}, 'rsp_audio': {'calls': 800}})
assert not assess(row, [0, 1, 2, 3, 60], 'missing')['below_100']
row['wall_us'] += 1
assert assess(row, [60]*4 + [40, 60], 'missing')['below_100']
assert assess(row, [60]*4 + [40, 60], 'missing')['slow_vi_windows'] == 1
checks = warnings(assess(row, [60]*4 + [40, 60], 'missing'))
assert 'below 100%' in checks and 'slow VI windows' in checks and 'audio underruns' in checks
silent = dict(assess(row, [], 'missing'), pcm_nonzero=0)
assert 'silent DSP capture' in warnings(silent)
row['subsystems']['rsp_audio']['calls'] = 0
assert assess(row, [], 'missing')['inactive_audio']
control = dict(row, subsystems={})
assert assess(control, [], 'missing')['inactive_audio'] is None
assert assess(control, [], 'missing')['inactive_video'] is None
entries = [{'rom': 'A'}, {'rom': 'B'}]
results = [dict(platform='dolphin', rom='A', completed=True)] * 2
results.append(dict(platform='dolphin', rom='Unknown', completed=False))
summary = coverage(entries, results, ['dolphin', 'hardware'])
assert summary['dolphin']['missing'] == ['B']
assert summary['dolphin']['duplicates'] == ['A']
assert summary['dolphin']['unexpected'] == ['Unknown']
assert summary['dolphin']['incomplete'] == ['Unknown']
assert summary['hardware']['missing'] == ['A', 'B']
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / 'xfb.bin'
    assert capture_status(path) == 'missing'
    path.write_bytes(b'bad')
    assert capture_status(path) == 'invalid'
    path.write_bytes(struct.pack('>4I', 0x57584642, 2, 66, 0) + bytes([16, 128])*132)
    assert capture_status(path) == 'flat/review'
    path.write_bytes(path.read_bytes()[:-2] + bytes([100, 128]))
    assert capture_status(path) == 'nonflat/review'
print('Library output/speed/cadence gate: PASS')
