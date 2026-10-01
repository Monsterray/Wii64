"""Hardware config provenance is mandatory; Dolphin uses an SD-file config."""
import contextlib
import io
from pathlib import Path
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from check_hardware_run import main

with tempfile.TemporaryDirectory(prefix="wii64-chain-check-") as directory:
    root = Path(directory)
    (root / "diag.cfg").write_text("dynacore=dynarec\naudio_quality=accurate\naudio_output=dsp\n"
                                   "chain=900,input=neutral sd:/wii64/roms/Test.z64\n")
    log = "\n".join("mark: " + marker for marker in (
        "diag received dynacore line", "diag requested core: dynarec", "diag applied core: dynarec",
        "CPU core dispatch: dynarec", "audio quality: accurate", "audio modes: n64=0 output=0 mixer=0 latency=2 sync=0",
        "pad replay records: 1")) + "\ngame: n=1/1 how=vis vis=900 rom=sd:/wii64/roms/Test.z64\n"
    for text, wiiload, expected in (
        (log, False, 0), (log, True, 1),
        ("mark: diag config: wiiload arguments\n" + log, True, 0),
        (log.replace("how=vis vis=900", "how=load_failed vis=0"), False, 1),
        (log.replace("Test.z64", "Test.z64.backup"), False, 1),
        (log.replace("pad replay records: 1", "pad replay records: 0"), False, 1),
        (log.replace("audio modes: n64=0 output=0", "audio modes: n64=0 output=1"), False, 1),
        (log.replace("CPU core dispatch: dynarec", "CPU core dispatch: pure interpreter"), False, 1),
    ):
        (root / "perf.log").write_text(text.replace("mark:", "mark:\0"))
        with contextlib.redirect_stdout(io.StringIO()):
            assert main(root, require_wiiload=wiiload) == expected
print("chain validator: config source, failed loads, replay/core/audio modes: ok")
