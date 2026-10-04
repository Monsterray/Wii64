"""Parser and overhead gates use synthetic counters, never benchmark claims."""
import contextlib
import hashlib
import io
import json
from pathlib import Path
import sys
import tempfile
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import memory_report as mr

with tempfile.TemporaryDirectory() as temporary:
    root = Path(temporary)
    for ext in ('.dol', '.elf'): (root / ('build' + ext)).write_bytes(ext.encode())
    artifact = {str(root / ('build' + ext)): hashlib.sha256(ext.encode()).hexdigest()
                for ext in ('.dol', '.elf')}
    runs = [root / str(i) for i in range(3)]
    memory = ('mark: boxart_probe: 16 buffers in 768 KiB PASS\n'
              'memory: n=1 stage=game_end malloc_free=12 arena1_free=32 arena2_free=64\n'
              'memory_heap: n=1 pool=boxart capacity=1000 live=100 peak=200 blocks=1 '
              'peak_blocks=2 min_free=800 free=900 largest=500 free_blocks=2 retired=0 peak_retired=0\n'
              'memory_ops: n=1 pool=boxart allocs=2 frees=1 failures=0 errors=0\n')
    game = ('game: n=1/1 how=vis vis=900 vi_rate=60 exceptions=12 cacheResets=1 '
            'recompiles=2 batches=3 verts=9 wall_us=1000 sleep_us=100 pmc1=100 underruns=0 overruns=0\n')
    for i, run in enumerate(runs):
        run.mkdir()
        (run / 'artifacts.json').write_text(json.dumps(artifact))
        (run / 'diag.cfg').write_text('chain=900 example\nmemory=' + str(int(i == 1)) + '\n')
        (run / 'perf.log').write_text((memory if i == 1 else '') + game)
    with contextlib.redirect_stdout(io.StringIO()):
        assert mr.compare(runs)
        (runs[1] / 'perf.log').write_text(memory + game.replace('pmc1=100', 'pmc1=110'))
        assert not mr.compare(runs)
        (runs[1] / 'perf.log').write_text(memory + game.replace('exceptions=12', 'exceptions=13'))
        assert not mr.compare(runs)
        (runs[1] / 'perf.log').write_text(memory + game.replace('vi_rate=60', 'vi_rate=50'))
        assert not mr.compare(runs)
        (runs[1] / 'perf.log').write_text(memory + game.replace('underruns=0', 'underruns=1'))
        assert not mr.compare(runs)
    original = (runs[1] / 'diag.cfg').read_text()
    (runs[1] / 'diag.cfg').write_text(original.replace('900 example', '900 different'))
    try: mr.compare(runs); raise AssertionError('accepted changed scene')
    except ValueError: pass
    (runs[1] / 'diag.cfg').write_text(original)
    for bad in (memory.replace('errors=0', 'errors=1'),
                memory.replace('free=900', 'free=899'),
                memory.replace('largest=500', 'largest=999'),
                memory.replace('memory_ops:', 'missing_ops:'),
                memory.replace('peak=200', 'peak=99'),
                memory + 'vm_io: errors=1\n'):
        (runs[1] / 'perf.log').write_text(bad + game)
        try: mr.read(runs[1]); raise AssertionError('accepted corrupt accounting')
        except ValueError: pass
    (runs[1] / 'perf.log').write_text(memory + game)
    (root / 'build.dol').write_bytes(b'tampered')
    try: mr.compare(runs); raise AssertionError('accepted changed frozen binary')
    except ValueError: pass
print('Memory report: paired accounting, bounded values, frozen hashes and overhead/parity gates PASS')
