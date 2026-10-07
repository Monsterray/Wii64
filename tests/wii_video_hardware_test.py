#!/usr/bin/env python3
"""Explicit leased HDMI smoke test. Never run this from the host unit suite.

Queue with wiibench.py add --timeout 180 --cwd <repo> -- python3
tests/wii_video_hardware_test.py .dev/runs/<new-directory>.
Leaves the Wii untouched; records the existing HDMI picture with no audio.
"""
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
WRAPPER = ROOT / "scripts/wii_video_capture.sh"


def main(directory):
    if not os.environ.get("WII_BENCH_JOB_START") or not os.environ.get("WII_BENCH_IP"):
        raise RuntimeError("queue this hardware test; do not invent a lease")
    output = Path(directory).resolve()
    if (ROOT / ".dev/runs").resolve() not in output.parents:
        raise RuntimeError("hardware results must stay under .dev/runs")
    output.mkdir(parents=True, exist_ok=False)
    info = json.loads(subprocess.check_output(["bash", str(WRAPPER), "inspect"], text=True, timeout=180))
    rows = []
    for width, height, rate in ((1280, 720, 60), (1920, 1080, 60), (2560, 1440, 30)):
        formats = [f for f in info["formats"] if f["width"] == width and f["height"] == height
                   and f["fourCC"] == "420v" and any(r["min"] <= rate <= r["max"] for r in f["fpsRanges"])]
        if not formats:
            raise RuntimeError(f"required advertised mode missing: {width}x{height}@{rate}")
        stem = output / f"{width}x{height}-{rate}"
        report = stem.with_suffix(".json")
        command = ["bash", str(WRAPPER), "capture", str(stem.with_suffix(".mov")),
                   "--seconds", "3", "--format", str(formats[0]["index"]), "--fps", str(rate),
                   "--pixel-format", "420v", "--frames-dir", str(stem) + "-frames", "--report", str(report)]
        result = subprocess.run(command, timeout=65)
        data = json.loads(report.read_text())
        assert result.returncode == 0 and not data["error"], data.get("error")
        assert data["movieFinalized"] and data["videoTracks"], "missing finalized video"
        assert all(t["width"] == width and t["height"] == height for t in data["videoTracks"]), "encoded size mismatch"
        assert data["observedFPS"] >= rate * 0.85, "unexpected delivered-rate shortfall"
        rows.append({k: v for k, v in data.items() if k != "samples"})
        if width == 1280:
            control = command
    # Exercise the exact wrapper's graceful cancellation, not an unrelated PID.
    movie, frames, metadata = output / "cancel.mov", output / "cancel-frames", output / "cancel.json"
    command = control.copy()
    command[3] = str(movie)
    for flag, value in (("--seconds", "30"), ("--frames-dir", str(frames)), ("--report", str(metadata))):
        command[command.index(flag) + 1] = value
    with (output / "cancel.log").open("w") as log:
        process = subprocess.Popen(command, stdout=log, stderr=log)
        try:
            deadline = time.monotonic() + 20
            while not (frames / "frame-000000.jpg").exists() and process.poll() is None and time.monotonic() < deadline:
                time.sleep(0.1)
            assert (frames / "frame-000000.jpg").exists(), "capture did not become ready"
            process.send_signal(signal.SIGTERM)
            assert process.wait(timeout=20) != 0, "cancel reported a complete capture"
        finally:
            if process.poll() is None:
                process.terminate()
                process.wait(timeout=20)
    data = json.loads(metadata.read_text())
    assert data["movieFinalized"] and data["interruptedBy"] == "SIGTERM" and data["videoTracks"], "cancel did not finalize video"
    # A file-size stop must not pass as a completed capture window.
    command = control.copy()
    command[3] = str(output / "cap.mov")
    for flag, value in (("--seconds", "10"), ("--frames-dir", str(output / "cap-frames")), ("--report", str(output / "cap.json"))):
        command[command.index(flag) + 1] = value
    result = subprocess.run(command + ["--max-mib", "1"], timeout=65)
    cap = json.loads((output / "cap.json").read_text())
    assert result.returncode != 0 and "file-size cap" in cap["error"], "size cap falsely passed"
    summary = {"queueJob": os.environ.get("WII_BENCH_JOB"), "modes": rows,
               "cancellation": "PASS: finalized partial movie", "fileSizeCap": "PASS: incomplete capture rejected",
               "audio": "not tested: explicit permission/device selection required",
               "480pMOV": "not validated: current AVFoundation path returns Cannot Record"}
    (output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print("HDMI hardware: modes, encoded dimensions, graceful cancel and size cap PASS")


if __name__ == "__main__":
    try:
        main(sys.argv[1])
    except (AssertionError, IndexError, KeyError, OSError, ValueError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"HDMI hardware test failed: {error}", file=sys.stderr)
        sys.exit(1)
