"""Exercise freezing, spaced paths and failure retention without Dolphin/Wii."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='wii64 library ') as directory:
    work = Path(directory)
    (work / '.dev/runs').mkdir(parents=True)
    (work / 'scripts').mkdir()
    for name in ('library_report.py', 'subsystem_report.py', 'chain_table.py'):
        shutil.copy2(root / 'scripts' / name, work / 'scripts' / name)
    shutil.copy2(root / '.dev/library_check.sh', work / '.dev/library_check.sh')
    shutil.copy2(root / '.dev/env.sh', work / '.dev/env.sh')
    dol = work / 'frozen build.dol'
    dol.write_bytes(b'dol fixture'); dol.with_suffix('.elf').write_bytes(b'elf fixture')
    roms = work / 'owned ROMs'; roms.mkdir()
    for name in ('Game A.z64', 'Game B.V64'):
        (roms / name).write_bytes(b'owned ROM fixture')
    (work / '.dev/dolphin_test.sh').write_text('exec python3 fake_dolphin.py "$@"\n')
    (work / 'fake_dolphin.py').write_text('''
import os, pathlib, sys
assert os.environ['WII64_DOLPHIN_DSP_HLE'] == 'False'
assert os.environ['WII64_DOLPHIN_MUTE_AUDIO'] == 'True'
assert os.environ['WII64_DOLPHIN_DUMP_AUDIO'] == 'True'
args = sys.argv[1:]
assert 'audio_quality=accurate' in args and 'audio_sync=native' in args
if os.environ.get('WII64_LIBRARY_MEMORY') == '1':
    assert all(x in args for x in ('memory=1','memory_boxart_probe=1','randomize_interrupt=0','stress_selectrom=1'))
line = args[-1]
rom = line.split(' ',1)[1]
assert pathlib.Path(os.environ['WII64_ROM_DIR'], pathlib.Path(rom).name).is_file()
out = pathlib.Path(args[0]).parent / pathlib.Path(rom).stem
out.mkdir()
(out/'perf.log').write_text(
    'subsystem_time: stage=rsp_gfx calls=200 timed_calls=200 period=1 timed_us=1000\\n'
    'subsystem_time: stage=rsp_audio calls=200 timed_calls=200 period=1 timed_us=1000\\n'
    'game: n=1/1 how=vis vis=1800 vi_rate=60 wall_us=30000000 batches=200 '
    'underruns=0 overruns=0 rom='+rom+'\\n')
print('Dolphin chain results: '+str(out))
if os.environ.get('FIXTURE_FAIL') and rom.endswith('Game B.V64'): sys.exit(3)
''')
    env = dict(os.environ, WII64_ROM_DIR=str(roms))
    command = ['bash', str(work / '.dev/library_check.sh'), str(dol), 'dolphin']
    for fail in (False, True):
        if fail: env['FIXTURE_FAIL'] = '1'
        env['WII64_LIBRARY_MEMORY'] = str(int(fail))
        result = subprocess.run(command, env=env, text=True, capture_output=True, cwd=work)
        assert result.returncode == int(fail), result.stderr + result.stdout
        out = Path(result.stdout.split('Library results: ')[-1].strip())
        manifest = json.loads((out / 'manifest.json').read_text())
        assert manifest['platforms'] == ['dolphin'] and len(manifest['entries']) == 2
        assert (out / 'build.dol').read_bytes() == dol.read_bytes()
        assert (out / 'roms/Game A.z64').read_bytes() == (roms / 'Game A.z64').read_bytes()
        assert 'audio_latency=stable' in (out / 'part_01.txt').read_text()
        assert manifest['diag'] == (['memory=1','memory_boxart_probe=1','randomize_interrupt=0','stress_selectrom=1'] if fail else [])
        assert all(x in (out / 'part_01.txt').read_text() for x in manifest['diag'])
        coverage = json.loads((out / 'coverage.json').read_text())
        assert not coverage['dolphin']['missing'] and coverage['dolphin']['recorded'] == 2
        assert coverage['dolphin']['launcher_failures'] == (['2'] if fail else [])
        gate = subprocess.run(['python3', str(work / 'scripts/library_report.py'), str(out), '--check'],
                              capture_output=True, text=True)
        assert gate.returncode == int(fail), gate.stderr + gate.stdout
    (roms / 'bad|name.z64').write_bytes(b'bad name')
    result = subprocess.run(command, env=env, text=True, capture_output=True, cwd=work)
    assert result.returncode and 'Unsupported ROM filename' in result.stderr
print('Library workflow: frozen spaced paths, host mute, explicit modes and retained launcher failure PASS')
