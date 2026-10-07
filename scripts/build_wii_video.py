#!/usr/bin/env python3
"""Cache an atomically built, ad-hoc signed native capture helper on macOS.

Build/sign pattern: WiiXplorer-NG e85f62851547c0e7dbf74897234eafc1c3621bac,
scripts/build-wii-capture.py. No changes to OS privacy/firewall policy.
"""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def build():
    if sys.platform != "darwin":
        raise SystemExit("Native HDMI capture requires macOS")
    import fcntl
    directory = ROOT / ".dev/tools"
    directory.mkdir(parents=True, exist_ok=True)
    with (directory / "wii-video-build.lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        return build_locked()


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
    print(build())
