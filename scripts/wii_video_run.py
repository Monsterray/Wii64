#!/usr/bin/env python3
"""Queue physical capture; freeze its helper and stop only our own process.

Discovery, permission setup and offline analysis never reserve the Wii.
Video uses native AVFoundation, not a network listener or Python camera API.
"""
import fcntl
import json
import math
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile

from build_wii_video import build, ROOT


def report(path):
    data = json.loads(Path(path).read_text())
    samples = data.pop("samples")
    sampled = [sample for sample in samples if "essentiallyBlack" in sample]
    settled = [sample for sample in samples if sample["monotonicSeconds"] >=
               samples[0]["monotonicSeconds"] + data["warmupSeconds"]] if samples else []
    pts = [sample["ptsSeconds"] for sample in settled
           if isinstance(sample.get("ptsSeconds"), (int, float)) and math.isfinite(sample["ptsSeconds"])]
    data["steadyFPS"] = (len(pts) - 1) / (pts[-1] - pts[0]) if len(pts) > 1 and pts[-1] > pts[0] else None
    data["sampledFrames"] = len(sampled)
    data["darkSampledFrames"] = sum(sample["essentiallyBlack"] for sample in sampled)
    jpegs = [sample["jpeg"] for sample in sampled if "jpeg" in sample]
    data["representativeFrames"] = [jpegs[0], jpegs[-1]] if jpegs else []
    return data


def deadline(args):
    def value(flag, default):
        return float(args[args.index(flag) + 1]) if flag in args else default
    seconds = value("--seconds", 10)
    warmup = value("--warmup", 3)
    if not 1 <= seconds <= 600 or not 0 <= warmup <= 30:
        raise ValueError("duration must be 1...600; warmup must be 0...30")
    return seconds + warmup + 40


def run_owned(binary, args, timeout):
    # ponytail: one capture across Wii64 checkouts; use per-device locks only
    # if this workstation gets a second dongle. Leave the lock inode
    # in place: unlinking it allows two processes to lock different files.
    lock = Path(tempfile.gettempdir()) / f"wii64-hdmi-{os.getuid()}.lock"
    descriptor = os.open(lock, os.O_CREAT | os.O_RDWR | os.O_NOFOLLOW, 0o600)
    with os.fdopen(descriptor, "w") as owned:
        try:
            fcntl.flock(owned, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            raise RuntimeError("another Wii64 capture owns this workstation's capture lock")
        process = subprocess.Popen([str(binary), *args], start_new_session=True)
        previous = {}

        def terminate():
            if process.poll() is None:
                os.killpg(process.pid, signal.SIGTERM)

        def stop(signum, frame):
            raise InterruptedError(f"capture interrupted by signal {signum}")

        for signum in (signal.SIGTERM, signal.SIGINT):
            previous[signum] = signal.signal(signum, stop)
        try:
            try:
                return process.wait(timeout=timeout)
            except (subprocess.TimeoutExpired, InterruptedError) as error:
                for signum in previous:
                    signal.signal(signum, signal.SIG_IGN)
                terminate()
                try:
                    process.wait(timeout=12)
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL)
                    process.wait(timeout=5)
                raise RuntimeError(f"capture stopped ({error}); partial output retained")
        finally:
            for signum, handler in previous.items():
                signal.signal(signum, handler)


def main(args):
    if args == ["usb-info"]:
        if sys.platform != "darwin":
            raise ValueError("USB inventory requires macOS")
        result = subprocess.run(["system_profiler", "SPUSBDataType", "-json"],
                                capture_output=True, text=True, check=True, timeout=30)
        stack, matches = [json.loads(result.stdout)], []
        while stack:
            item = stack.pop()
            if isinstance(item, dict):
                if str(item.get("vendor_id", "")).partition(" ")[0].lower() == "0x2b89" and \
                        str(item.get("product_id", "")).partition(" ")[0].lower() == "0x5389":
                    matches.append(item)
                stack.extend(item.values())
            elif isinstance(item, list):
                stack.extend(item)
        print(json.dumps(matches, indent=2, sort_keys=True))
        return 0 if matches else 1
    if args[:1] == ["report"]:
        if len(args) != 2:
            raise ValueError("usage: wii_video report <capture.json>")
        print(json.dumps(report(args[1]), indent=2, sort_keys=True))
        return 0
    native = None
    capture_root = ROOT
    if args[:1] == ["--native"]:
        if len(args) < 3:
            raise ValueError("--native needs a helper path and command")
        native, args = Path(args[1]).resolve(), args[2:]
        capture_root = Path(json.loads((Path(__file__).parent / "repository.json").read_text())["root"]).resolve()
    capture = bool(args and args[0] in ("snapshot", "capture"))
    if capture and (len(args) < 2 or len(args) % 2 or
                    len(set(args[2::2])) != len(args[2::2])):
        raise ValueError("capture needs an output and unique option/value pairs")
    timeout = deadline(args) if capture else 180
    binary = native or build()
    if not capture:
        return subprocess.call([str(binary), *args])
    if len(args) < 2:
        raise ValueError("capture requires a new output path")
    # All media must stay in the repo's ignored run tree, never source folders.
    run_root = (capture_root / ".dev/runs").resolve()
    for index in (1, *(args.index(flag) + 1 for flag in
                       ("--frames-dir", "--report") if flag in args)):
        path = Path(args[index]).resolve()
        if run_root not in path.parents:
            raise ValueError(f"capture outputs must be under {run_root}")
        args[index] = str(path)
    if "--report" in args and args[args.index("--report") + 1] == args[1]:
        raise ValueError("report and media output must be different files")
    if os.environ.get("WII_BENCH_JOB_START") and os.environ.get("WII_BENCH_IP"):
        return run_owned(binary, args, timeout)
    if native:
        raise ValueError("a frozen capture helper requires an active Wii queue job")
    client = Path(os.environ.get("WII_BENCH_CLIENT", str(Path.home() / ".wii-bench/wiibench.py")))
    if not client.is_file():
        raise ValueError(f"Wii queue client not found: {client}")
    run_root.mkdir(parents=True, exist_ok=True)
    frozen_dir = Path(tempfile.mkdtemp(prefix="hdmi-helper-", dir=run_root))
    frozen = frozen_dir / "wii-video"
    shutil.copy2(binary, frozen)
    launcher = frozen_dir / "wii_video_run.py"
    shutil.copy2(Path(__file__), launcher)
    shutil.copy2(ROOT / "scripts/build_wii_video.py", frozen_dir / "build_wii_video.py")
    (frozen_dir / "repository.json").write_text(json.dumps({"root": str(ROOT)}))
    command = [sys.executable, str(client), "add", "--name", "Wii64 HDMI capture",
               "--agent", "wii64-capture", "--timeout", str(int(timeout + 20)),
               "--cwd", str(ROOT), "--", sys.executable, str(launcher),
               "--native", str(frozen), *args]
    # The queue reports its ID immediately; it owns scheduling, not this script.
    return subprocess.call(command)


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv[1:]))
    except (ValueError, IndexError, KeyError, TypeError, OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"wii_video: {error}", file=sys.stderr)
        sys.exit(1)
