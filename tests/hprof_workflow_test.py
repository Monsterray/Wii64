#!/usr/bin/env python3
"""Queue/freeze checks without a Wii; calibration rejects misleading inputs."""
import contextlib
import hashlib
import io
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
import hprof_compare as hc
import hprof_view as hv

repo = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='wii64 hprof ') as temporary:
    root = Path(temporary)
    (root/'.dev').mkdir()
    (root/'sdk/bin').mkdir(parents=True)
    (root/'scripts/chains').mkdir(parents=True)
    shutil.copyfile(repo/'.dev/profile_pc.sh', root/'.dev/profile_pc.sh')
    (root/'.dev/env.sh').write_text('export DEVKITPPC="$PWD/sdk"\n')
    (root/'scripts/chains/pc_sampler.txt').write_text('chain=900 sd:/wii64/roms/example.z64\n')
    (root/'build.dol').write_bytes(b'dol')
    (root/'build.elf').write_bytes(b'elf')
    (root/'queue.py').write_text('')
    for name, text in {
        'sdk/bin/powerpc-eabi-nm':'#!/bin/sh\nprintf "80000000 T hprof_entry\\n"\n',
        'sdk/bin/powerpc-eabi-gcc':'#!/bin/sh\necho compiler\n',
        '.dev/hardware_run.sh':'#!/bin/sh\nprintf "%s|%s|%s\\n" "$WII64_SKIP_BUILD" "$WII64_DOL" "$WII64_CHAIN_FILE" >> queued\necho job\n',
    }.items():
        path = root/name
        path.write_text(text)
        path.chmod(0o755)
    subprocess.run(['git','init','-q'],cwd=root,check=True)
    subprocess.run(['git','-c','user.name=Test','-c','user.email=test@example.invalid',
                    'commit','--allow-empty','-qm','fixture'],cwd=root,check=True)
    env = dict(os.environ, WII_BENCH_CLIENT=str(root/'queue.py'), WII64_SURVEY_QUEUE_ONLY='1')
    env.pop('WII_BENCH_JOB', None)
    subprocess.run(['bash','.dev/profile_pc.sh','build.dol'],cwd=root,env=env,check=True,stdout=subprocess.DEVNULL)
    queued = (root/'queued').read_text().splitlines()
    assert len(queued) == 3 and all(line.startswith('1|') for line in queued)
    survey = next((root/'.dev/runs').glob('pc-survey-*'))
    assert [int((survey/(label+'.txt')).read_text().split('hprof=')[1])
            for label in ('control1','sampler','control2')] == [0,1,0]
    assert all(Path(line.split('|')[1]).resolve() == (survey/'build.dol').resolve() for line in queued)
    frozen = json.loads((survey/'artifacts.json').read_text())
    assert all(hashlib.sha256((survey/name).read_bytes()).hexdigest() == digest
               for name,digest in frozen.items())
    (survey/'build.dol').write_bytes(b'changed')
    bad = subprocess.run(['bash','.dev/profile_pc.sh','--collect',str(survey)],cwd=root,env=env,
                         stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    assert bad.returncode and 'Frozen artifact changed' in bad.stdout

    roots = [root/f'run{i}' for i in range(3)]
    for i, directory in enumerate(roots):
        directory.mkdir()
        header = [0x48363450,1,0x80004000,0x80004040,5,2,729000,
                  int(i==1),0,0,int(i==1),int(i==1),int(i==1),0,131072,1]
        (directory/'hprof_01.bin').write_bytes(hv.HEADER.pack(*header)+struct.pack('>2I',int(i==1),0))
        hashes = {str(root/name):hashlib.sha256((root/name).read_bytes()).hexdigest()
                  for name in ('build.dol','build.elf')}
        (directory/'artifacts.json').write_text(json.dumps(hashes))
        (directory/'xfb_01.bin').write_bytes(struct.pack('>4I',0x57584642,2,66,0)+b'\0\x80\xff\x80'*66)
        (directory/'perf.log').write_text('vm_io: errors=0\n'
            'game: n=1/1 how=vis vis=900 vi_rate=60 wall_us=1000000 sleep_us=500000 '
            'exceptions=1 cacheResets=1 recompiles=1 batches=1 verts=1 pmc1=0 underruns=0 overruns=0 rom=example.z64\n')
    with contextlib.redirect_stdout(io.StringIO()):
        assert hc.compare(roots)
        samplelog = roots[1]/'perf.log'
        samplelog.write_text(samplelog.read_text().replace('wall_us=1000000','wall_us=1100000'))
        assert not hc.compare(roots)  # measured probe cost exceeds gate
        samplelog.write_text(samplelog.read_text().replace('pmc1=0','pmc1=42'))
        assert not hc.compare(roots)
    manifest = roots[1]/'artifacts.json'
    manifest.write_text(manifest.read_text().replace(hashes[str(root/'build.dol')], '0'*64))
    try:
        hc.compare(roots)
        raise AssertionError('accepted different binary')
    except ValueError:
        pass
print('PC survey: same-binary queue, frozen hashes, parser modes and overhead gate PASS')
