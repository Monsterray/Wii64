"""Queue guard check: fake client, no lease server or Wii access."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="wii64-queue-") as directory:
    state = Path(directory)
    client = state / "wiibench.py"
    client.write_text("import sys\nprint('fixture-job' if sys.argv[1] == 'add' else repr(sys.argv))\n")
    env = dict(os.environ, WII_BENCH_HOME=directory, WII_BENCH_CLIENT=str(client),
               WII64_SKIP_BUILD="1", WII64_DOL="/tmp/frozen build.dol")
    for key in ("WII_BENCH_JOB", "WII_BENCH_SERVER"):
        env.pop(key, None)
    command = ["bash", str(root / ".dev/hardware_run.sh"), "glN64_wii", "audio_reference"]
    result = subprocess.run(command, env=env, capture_output=True, text=True)
    assert result.returncode == 2 and "central lease server" in result.stderr
    (state / "server").write_text(" \n")
    result = subprocess.run(command, env=env, capture_output=True, text=True)
    assert result.returncode == 2
    (state / "server").write_text("http://fixture.invalid:4310\n")
    client.write_text("import sys\n"
                      "if sys.argv[1] == 'add':\n"
                      " assert sys.argv[sys.argv.index('--agent') + 1].startswith('wii64-')\n"
                      " assert int(sys.argv[sys.argv.index('--timeout') + 1]) > 180\n"
                      " assert any(x.startswith('WII64_RECEIVER_TIMEOUT=') for x in sys.argv)\n"
                      " assert 'WII64_DOL=/tmp/frozen build.dol' in sys.argv\n"
                      " assert 'WII64_SKIP_BUILD=1' in sys.argv\n"
                      " i = sys.argv.index('bash')\n"
                      " assert sys.argv[i + 1] == '-c'\n"
                      " boot, script, frozen = sys.argv[i + 2:i + 5]\n"
                      " from pathlib import Path\n"
                      " source = Path(frozen).read_text()\n"
                      " assert source == Path(script).read_text()  # a frozen copy, passed as a file\n"
                      " import subprocess\n"
                      " subprocess.run(['bash', '-n', '-c', source], check=True)\n"
                      " import os\n"
                      " Path(frozen).write_text(source.replace('source .dev/env.sh', 'pwd; exit 0', 1))\n"
                      " result = subprocess.check_output(['bash', '-c', boot, script, frozen],\n"
                      "   cwd=Path(__file__).parent, env=dict(os.environ, WII_BENCH_JOB='fixture'), text=True)\n"
                      " assert result.strip() == str(Path(script).parents[1])\n"
                      " Path(frozen).unlink()\n"
                      " assert sys.argv[i + 5:] == ['glN64_wii', 'audio_reference']\n"
                      " print('fixture-job')\n"
                      "else:\n"
                      " assert sys.argv[1:] == ['wait', 'fixture-job']\n"
                      " print('waited')\n")
    result = subprocess.run(command, env=env, capture_output=True, text=True)
    assert result.returncode == 0 and "Queued Wii64" in result.stdout and "waited" in result.stdout
    env["WII64_QUEUE_ONLY"] = "1"
    result = subprocess.run(command, env=env, capture_output=True, text=True)
    assert result.returncode == 0 and result.stdout.strip() == "fixture-job"
    env["WII_BENCH_SERVER"] = ""
    result = subprocess.run(command, env=env, capture_output=True, text=True)
    assert result.returncode == 2
    env["WII_BENCH_SERVER"] = "http://fixture.invalid:4310"
    for receiver, timeout in [('bad', '2400'), ('0', '2400'), ('1200', '1201')]:
        invalid = dict(env, WII64_RECEIVER_TIMEOUT=receiver, WII64_JOB_TIMEOUT=timeout)
        result = subprocess.run(command, env=invalid, capture_output=True, text=True)
        assert result.returncode != 0 and not result.stdout.strip()
    client.write_text("import sys\nsys.exit(9)\n")
    result = subprocess.run(command, env=env, capture_output=True, text=True)
    assert result.returncode == 9 and not result.stdout.strip()
    dol = state / 'agent build.dol'
    dol.touch()
    dol.with_suffix('.elf').touch()
    command = ['bash', str(root / '.dev/test_agent_wii.sh'), str(dol)]
    env['WII_BENCH_SERVER'] = ''
    result = subprocess.run(command, env=env, capture_output=True, text=True)
    assert result.returncode == 2 and 'central Wii queue' in result.stderr
    env['WII_BENCH_SERVER'] = 'http://fixture.invalid:4310'
    client.write_text("import sys\nassert sys.argv[1] == 'add'\nprint('fixture-crash-job')\n")
    result = subprocess.run(command, env=env, capture_output=True, text=True)
    assert result.returncode == 0 and result.stdout.strip() == 'fixture-crash-job'
print("hardware entry point requires the central lease and preserves job arguments: ok")
