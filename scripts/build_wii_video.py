#!/usr/bin/env python3
"""Cache an atomically built, ad-hoc signed native capture helper on macOS.

Build/sign pattern: WiiXplorer-NG e85f62851547c0e7dbf74897234eafc1c3621bac,
scripts/build-wii-capture.py. No changes to OS privacy/firewall policy.
"""
import hashlib
import json
import shutil
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def build(application=False):
    if sys.platform != "darwin":
        raise SystemExit("Native HDMI capture requires macOS")
    import fcntl
    directory = ROOT / ".dev/tools"
    directory.mkdir(parents=True, exist_ok=True)
    with (directory / "wii-video-build.lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        binary = build_locked()
        return build_application(binary) if application else binary


def build_application(binary):
    """Keep a real LaunchServices bundle separate from its CLI caller."""
    directory = binary.parent
    output = directory / "Wii64 HDMI Capture.app"
    plist = ROOT / "scripts/wii_video.plist"
    digest = hashlib.sha256(binary.read_bytes() + plist.read_bytes()).hexdigest()
    stamp = directory / "wii-video-app.json"
    try:
        cached = json.loads(stamp.read_text())
        if cached["inputs_sha256"] == digest and \
                hashlib.sha256((output / "Contents/MacOS/wii-video").read_bytes()).hexdigest() == cached["binary_sha256"] and \
                (output / "Contents/Info.plist").read_bytes() == plist.read_bytes():
            subprocess.run(["codesign", "--verify", "--strict", str(output)], check=True)
            return output
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError):
        pass
    with tempfile.TemporaryDirectory(prefix="video-app-", dir=directory) as temporary:
        app = Path(temporary) / output.name
        (app / "Contents/MacOS").mkdir(parents=True)
        shutil.copy2(binary, app / "Contents/MacOS/wii-video")
        shutil.copy2(plist, app / "Contents/Info.plist")
        subprocess.run(["codesign", "--force", "--sign", "-", str(app)], check=True)
        subprocess.run(["codesign", "--verify", "--strict", str(app)], check=True)
        previous = Path(temporary) / "previous.app"
        if output.exists():
            output.replace(previous)
        try:
            app.replace(output)
        except OSError:
            if previous.exists():
                previous.replace(output)
            raise
        stamp.write_text(json.dumps({"inputs_sha256": digest,
                                    "binary_sha256": hashlib.sha256(
                                        (output / "Contents/MacOS/wii-video").read_bytes()).hexdigest()}) + "\n")
    return output


def build_locked():
    source = ROOT / "scripts/wii_video.swift"
    plist = ROOT / "scripts/wii_video.plist"
    inputs = (source, plist, ROOT / "scripts/build_wii_video.py")
    digest = hashlib.sha256(b"".join(p.read_bytes() for p in inputs)).hexdigest()
    directory = ROOT / ".dev/tools"
    directory.mkdir(parents=True, exist_ok=True)
    output = directory / "wii-video"
    stamp = directory / "wii-video.json"
    try:
        cached = json.loads(stamp.read_text())
        if cached["inputs_sha256"] == digest and output.is_file() and \
                cached["binary_sha256"] == hashlib.sha256(output.read_bytes()).hexdigest():
            return output
    except (OSError, ValueError, KeyError):
        pass
    cache = ROOT / ".dev/build_tmp/swift-module-cache"
    cache.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="video-build-", dir=directory) as temporary:
        binary = Path(temporary) / "wii-video"
        command = ["swiftc", "-O", "-module-cache-path", str(cache)]
        for framework in ("AVFoundation", "AppKit", "CoreImage", "CoreMedia"):
            command += ["-framework", framework]
        for argument in ("-sectcreate", "__TEXT", "__info_plist", str(plist)):
            command += ["-Xlinker", argument]
        subprocess.run(command + [str(source), "-o", str(binary)], check=True)
        subprocess.run(["codesign", "--force", "--sign", "-", "--identifier",
                        "org.wii64.dev-capture", str(binary)], check=True)
        subprocess.run(["codesign", "--verify", "--strict", str(binary)], check=True)
        metadata = {"inputs_sha256": digest,
                    "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest()}
        new_stamp = Path(temporary) / "stamp.json"
        new_stamp.write_text(json.dumps(metadata, indent=2) + "\n")
        binary.replace(output)  # Failed compile/sign leaves the previous helper intact.
        new_stamp.replace(stamp)
    return output


if __name__ == "__main__":
    print(build(application="--app" in sys.argv[1:]))
