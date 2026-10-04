"""Run the actual raw-SD capture loop for short and full-library chains."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / '.dev/dolphin_test.sh').read_text()
start = source.index('\tfor n in $(seq')
loop = source[start:source.index('\nfi\n# A menu-only', start)]
with tempfile.TemporaryDirectory(prefix='wii64 captures ') as directory:
    work = Path(directory)
    (work / '.dev').mkdir()
    (work / 'scripts').mkdir()
    (work / 'scripts/sdimage_read.py').write_text('''
import pathlib, re, sys
assert re.fullmatch(r'wii64/(xfb_[0-9]{2}\\.bin|padtrace_[0-9]{2}\\.csv|hprof_[0-9]{2}\\.bin)', sys.argv[2]), sys.argv
pathlib.Path(sys.argv[3]).write_bytes(b'capture fixture')
''')
    script = work / '.dev/extract.sh'
    script.write_text('frame_count="$COUNT"\nLOCAL_WII64="$OUTPUT"\nraw=fixture\n' + loop)
    for count in (1, 3, 18, 99):
        out = work / str(count)
        out.mkdir()
        subprocess.run(['bash', str(script)], env=dict(os.environ, COUNT=str(count), OUTPUT=str(out)), check=True)
        expected = {f'{kind}_{n:02}.{ext}' for n in range(1, count + 1)
                    for kind, ext in (('xfb', 'bin'), ('padtrace', 'csv'), ('hprof', 'bin'))}
        assert {p.name for p in out.iterdir()} == expected, f'missing captures for {count}-entry chain'
print('Dolphin raw-SD capture names: 1, 3, 18 and 99 entries PASS')
