"""Baseline filing retains small validation/provenance records, not ROMs."""
from pathlib import Path
from types import SimpleNamespace
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
import baseline_add

with tempfile.TemporaryDirectory(prefix="wii64-baseline-") as directory:
    root = Path(directory)
    source, dest = root / "run", root / "baselines" / "fixture"
    source.mkdir()
    dest.mkdir(parents=True)
    (source / "perf.log").write_text(
        "game: n=1/2 how=vis vis=900 vi_rate=60 wall_us=15000000 padtrace=1 "
        "avg_fps=30 exceptions=0 recompiles=0 treeDepthMax=0 underruns=0 "
        "rom=sd:/wii64/roms/Test.z64\n"
        "game: n=2/2 how=vis vis=900 vi_rate=60 wall_us=15000000 padtrace=0 "
        "avg_fps=30 exceptions=0 recompiles=0 treeDepthMax=0 underruns=0 "
        "rom=sd:/wii64/roms/Next.z64\n")
    names = ("diag.cfg", "artifacts.json", "fault-check.txt", "hbc-before.json",
             "hbc-after.json", "agent-status.json", "dolphin-settings.json", "padtrace_01.csv")
    for name in names:
        (source / name).write_text("fixture\n")
    (source / "private.z64").write_bytes(b"not a ROM")
    (source / "padtrace_02.csv").write_text("stale zero-trace game\n")
    (source / "padtrace_03.csv").write_text("stale previous chain\n")
    baseline_add.BASELINES = root / "baselines"
    baseline_add.GAMES = root / "baselines" / "games.csv"
    args = SimpleNamespace(chain=str(source), id="fixture", purpose="host test",
                           plugin="glN64", platform="hardware", build="fixture")
    baseline_add.file_chain(args, dest)
    assert all((dest / name).read_bytes() == (source / name).read_bytes() for name in names)
    assert not (dest / "private.z64").exists()
    assert not (dest / "padtrace_02.csv").exists()
    assert not (dest / "padtrace_03.csv").exists()
print("baseline validation/provenance retention: ok")
