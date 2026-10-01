"""Run the real Bash staging script against fake queue/HBC clients."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

source = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="wii64-replay-") as directory:
    root = Path(directory) / "repo with spaces"
    for name in (".dev", "scripts/chains", "scripts/inputs", "SDK with spaces/tools", "queue"):
        (root / name).mkdir(parents=True)
    shutil.copy(source / ".dev/stage_wii_inputs.sh", root / ".dev")
    (root / "scripts/chains/test.txt").write_text(
        "# input=ignore\nchain=900,input=neutral Foo.z64\n"
        "chain=900,input=press_a_periodically Foo.z64\nchain=900,input=neutral Foo.z64\n")
    for name in ("neutral", "press_a_periodically"):
        (root / f"scripts/inputs/{name}.txt").write_text("0 0000 0 0 0 0\n")
    (root / "SDK with spaces/tools/hbc.py").write_text(
        "import pathlib,sys\n"
        "with pathlib.Path('calls').open('a') as f: f.write(repr(sys.argv[1:]) + '\\n')\n")
    (root / "queue/server").write_text("http://fixture.invalid:4310\n")
    (root / "queue/client.py").write_text(
        "import sys\nassert sys.argv[1] in ('add', 'wait')\nprint('fixture-job')\n")
    env = dict(os.environ, WII64_HBC_ROOT=str(root / "SDK with spaces"),
               WII_BENCH_HOME=str(root / "queue"), WII_BENCH_CLIENT=str(root / "queue/client.py"))
    env.pop("WII_BENCH_JOB", None)
    env.pop("WII_BENCH_SERVER", None)
    def run():
        return subprocess.run(["bash", ".dev/stage_wii_inputs.sh", "test"], cwd=root,
                              env=env, text=True, capture_output=True)
    assert run().returncode == 0 and not (root / "calls").exists()  # enqueue only
    env["WII_BENCH_SERVER"] = ""
    assert run().returncode != 0 and not (root / "calls").exists()
    env.update(WII_BENCH_JOB="fixture-job", WII_BENCH_IP="192.0.2.1")
    result = run()
    assert result.returncode == 0, result.stdout + result.stderr
    calls = (root / "calls").read_text().splitlines()
    assert len(calls) == 3 and "wait" in calls[0]
    assert all("sd:/wii64/input/" in line for line in calls[1:])
    (root / "scripts/inputs/neutral.txt").unlink()
    assert run().returncode != 0 and len((root / "calls").read_text().splitlines()) == 3
print("replay staging: leased only, deduplicated inputs, spaced SDK path, fail before transfer: ok")
