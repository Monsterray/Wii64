"""Exercise the real survey driver with fake builds/queue; never contact a Wii."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

source = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="wii64-survey-") as directory:
    root = Path(directory) / "repo with spaces"
    for name in (".dev", "scripts/chains", "toolchain/bin", "queue"):
        (root / name).mkdir(parents=True)
    for name in (".dev/profile_subsystems.sh", ".dev/bench_session.sh", "scripts/subsystem_report.py",
                 "scripts/subsystem_compare.py", "scripts/chain_table.py",
                 "scripts/chains/subsystem_survey.txt"):
        shutil.copy(source / name, root / name)
    subprocess.run(["git", "init", "-q"], cwd=root, check=True)
    subprocess.run(["git", "-c", "user.name=Fixture", "-c", "user.email=fixture@invalid",
                    "commit", "--allow-empty", "-qm", "fixture"], cwd=root, check=True)
    (root / "queue/server").write_text("http://fixture.invalid:4310\n")
    (root / ".dev/env.sh").write_text('DEVKITPPC="$PWD/toolchain"\n')
    scripts = {
        "toolchain/bin/powerpc-eabi-gcc": "#!/bin/bash\necho fixture compiler\n",
        ".dev/build_profiling.sh": "#!/bin/bash\nset -eu\n"
            'name=wii64-glN64; [[ "$1" != Rice_wii ]] || name=wii64-Rice\n'
            "printf '%s\\n' \"$@\" > \"$name.dol\"\ncp \"$name.dol\" \"$name.elf\"\n",
        ".dev/hardware_run.sh": "#!/bin/bash\nset -eu\n"
            '[[ "$WII64_QUEUE_ONLY" = 1 && "$WII64_SKIP_BUILD" = 1 ]]\n'
            "python3 queue/fake.py add\n",
    }
    for name, text in scripts.items():
        (root / name).write_text(text)
        (root / name).chmod(0o755)
    (root / "queue/fake.py").write_text('''import os, pathlib, sys
queue = pathlib.Path('queue')
if os.environ.get('FIXTURE_QUEUE_FAIL'):
    sys.exit(9)
if sys.argv[1] == 'add':
    agent = os.environ['WII_BENCH_AGENT']
    assert agent.startswith('wii64-')
    identity = queue / 'agent'
    if identity.exists() and len(list(queue.glob('*.job'))) % 3:
        assert identity.read_text() == agent
    else:
        identity.write_text(agent)
    count = len(list(queue.glob('*.job')))
    dol = pathlib.Path(os.environ['WII64_DOL'])
    full = count % 3 != 1 or os.environ.get('WII64_SURVEY_SAME_PROBES') == '1'
    assert ('PERF_SUBSYSTEM_PROBES' in dol.read_text()) == full
    assert dol.with_suffix('.elf').is_file()
    assert pathlib.Path(os.environ['WII64_CHAIN_FILE']).is_file()
    job = f'fixture-{count}'
    (queue / (job + '.job')).write_text(str(dol))
    run = queue / job
    run.mkdir()
    (run / 'diag.cfg').write_text('chain=900,input=neutral Test.z64\\n')
    timing = 'subsystem_time: stage=lookup calls=127 timed_calls=1 period=127 timed_us=100\\n' if full else ''
    (run / 'perf.log').write_text(timing + 'game: n=1/1 how=vis vis=900 vi_rate=60 wall_us=15000000 pmc1=100 pmc2=90 rom=Test.z64\\n')
    print(job)
else:
    assert sys.argv[1] == 'wait'
    print('Hardware run complete: queue/' + sys.argv[2])
''')
    env = dict(os.environ, WII_BENCH_HOME=str(root / "queue"),
               WII_BENCH_CLIENT=str(root / "queue/fake.py"))
    for name in ("WII_BENCH_JOB", "WII_BENCH_SERVER"):
        env.pop(name, None)
    result = subprocess.run(["bash", ".dev/profile_subsystems.sh"], cwd=root,
                            env=env, text=True, capture_output=True)
    assert result.returncode == 0, result.stdout + result.stderr
    assert "Survey complete:" in result.stdout and "cycles +0.00%" in result.stdout
    out, = (root / ".dev/runs").iterdir()
    assert len(json.loads((out / "artifacts.json").read_text())) == 5
    for label, number in (("full1", 0), ("control", 1), ("full2", 2)):
        assert (out / (label + ".job")).read_text().strip() == f"fixture-{number}"
        assert (out / (label + ".report.txt")).is_file()
    assert (out / "source.json").is_file()
    sdk = root / 'SDK with spaces'
    (sdk / 'sdk/hbc_agent/libogc2').mkdir(parents=True)
    (sdk / 'sdk/hbc_agent/libogc2/libhbcagent.a').write_bytes(b'fixture archive')
    subprocess.run(['git', 'init', '-q', str(sdk)], check=True)
    subprocess.run(['git', '-C', str(sdk), '-c', 'user.name=Fixture', '-c',
                    'user.email=fixture@invalid', 'commit', '--allow-empty', '-qm', 'SDK'], check=True)
    env.update(WII64_SURVEY_FULL_FLAGS='-DPERF_PROF -DPERF_SUBSYSTEM_PROBES -DVM_PAGE_READAHEAD=1',
               WII64_SURVEY_CONTROL_FLAGS='-DPERF_PROF -DPERF_SUBSYSTEM_PROBES -DVM_PAGE_READAHEAD=0',
               WII64_SURVEY_SAME_PROBES='1')
    result = subprocess.run(['bash', '.dev/profile_subsystems.sh', 'subsystem_survey',
                             'HBC_AGENT=1', f'HBC_AGENT_ROOT={sdk}'], cwd=root,
                            env=env, text=True, capture_output=True)
    assert result.returncode == 0 and 'not probe-cost calibration' in result.stdout, result.stdout + result.stderr
    custom, = [p for p in (root / '.dev/runs').iterdir() if p != out]
    metadata = json.loads((custom / 'source.json').read_text())
    assert metadata['bench_agent'].startswith('wii64-') and metadata['job_timeout'] > metadata['receiver_timeout']
    assert len(metadata['hbc_sdk']['archive_sha256']) == 64 and metadata['make_args'][0] == 'HBC_AGENT=1'
    for mode, ahead in [('full', 1), ('control', 0)]:
        build = (custom / (mode + '.dol')).read_text()
        assert f'VM_PAGE_READAHEAD={ahead}' in build and f'HBC_AGENT_ROOT={sdk}' in build
    # Resume without flags in the environment, rebuilding or creating jobs.
    for name in ('WII64_SURVEY_FULL_FLAGS', 'WII64_SURVEY_CONTROL_FLAGS', 'WII64_SURVEY_SAME_PROBES'):
        env.pop(name)
    result = subprocess.run(['bash', '.dev/profile_subsystems.sh', '--collect', str(custom)],
                            cwd=root, env=env, text=True, capture_output=True)
    assert result.returncode == 0 and 'not probe-cost calibration' in result.stdout
    assert len(list((root / 'queue').glob('*.job'))) == 6
    (custom / 'full.dol').write_text('changed after queueing')
    result = subprocess.run(['bash', '.dev/profile_subsystems.sh', '--collect', str(custom)],
                            cwd=root, env=env, text=True, capture_output=True)
    assert result.returncode != 0 and 'Survey complete:' not in result.stdout
    # Reuse verified artifacts without rebuilding; freeze the current chain too.
    result = subprocess.run(['bash', '.dev/profile_subsystems.sh', '--reuse', str(out), 'subsystem_survey'],
                            cwd=root, env=env, text=True, capture_output=True)
    assert result.returncode == 0, result.stdout + result.stderr
    assert 'Clean build:' not in result.stdout and len(list((root / 'queue').glob('*.job'))) == 9
    env['WII64_SURVEY_TARGET'] = 'Rice_wii'
    result = subprocess.run(['bash', '.dev/profile_subsystems.sh'], cwd=root, env=env, text=True, capture_output=True)
    assert result.returncode == 0, result.stdout + result.stderr
    rice = next(p for p in (root / '.dev/runs').iterdir()
                if (p / 'source.json').exists() and json.loads((p / 'source.json').read_text())['target'] == 'Rice_wii')
    assert 'Rice_wii' in (rice / 'full.dol').read_text()
    assert len(list((root / 'queue').glob('*.job'))) == 12
    env["FIXTURE_QUEUE_FAIL"] = "1"
    result = subprocess.run(["bash", ".dev/profile_subsystems.sh"], cwd=root,
                            env=env, text=True, capture_output=True)
    assert result.returncode == 9 and "Survey complete:" not in result.stdout
    assert len(list((root / "queue").glob("*.job"))) == 12
print("survey driver: frozen builds, spaced paths, queued A/B/A and reports: ok")
