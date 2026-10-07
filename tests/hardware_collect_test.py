"""Outbound collection: fresh tags, crashes, deadlines and file allowlist."""
import json
from pathlib import Path
import socket
import os
import subprocess
import sys
import tempfile
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from hardware_collect import collect

TAG = "a" * 32
LOG = ("mark: diag config: wiiload arguments\nmark: result_tag=" + TAG + "\n"
       "game: n=1/1 how=vis vis=60 padtrace=0 rom=sd:/wii64/roms/test.z64\n"
       "hprof: n=1 requested=0 saved=1 aggregate_pmc_valid=1\n").encode()


class Client:
    HBCError = RuntimeError
    crash = None
    log = LOG
    calls = []
    version_calls = 0
    fail_file = ""

    @classmethod
    def version(cls, wii, timeout):
        cls.version_calls += 1
        return "1.10.0 agent" if cls.version_calls == 1 else "1.10.0"

    @staticmethod
    def is_agent(value):
        return value.endswith(" agent")

    @classmethod
    def status(cls, wii):
        return {"crash": cls.crash}

    @staticmethod
    def crash_report(crash, elf):
        return "fixture crash"

    @classmethod
    def get_file(cls, wii, remote):
        cls.calls.append(remote)
        if cls.fail_file and remote.endswith(cls.fail_file):
            raise cls.HBCError("CRC mismatch")
        return cls.log if remote.endswith("perf.log") else b"fixture payload"


def run(log=LOG, crash=None, fail_file="", timeout=15):
    with tempfile.TemporaryDirectory() as directory:
        out = Path(directory)
        (out / "diag.cfg").write_text("chain=60 sd:/wii64/roms/test.z64\nresult_tag=" + TAG + "\n")
        (out / "hbc-before.json").write_text(json.dumps({"crash": None}))
        (out / ".sent").touch()
        Client.log, Client.crash, Client.fail_file = log, crash, fail_file
        Client.calls, Client.version_calls = [], 0
        clock = [0.0]
        def sleep(seconds):
            clock[0] += seconds
        with patch("hardware_collect.time.monotonic", lambda: clock[0]), \
             patch("hardware_collect.time.sleep", sleep), \
             patch.object(socket, "socket", side_effect=AssertionError("No sockets in fixture")):
            try:
                collect(Client, "fixture", out, timeout, "matching.elf")
            finally:
                assert not (out / ".ready").exists()
        assert (out / "hprof_01.bin").read_bytes() == b"fixture payload"
        assert Client.calls == ["sd:/wii64/perf.log", "sd:/wii64/xfb_01.bin", "sd:/wii64/hprof_01.bin"]


run()
for kwargs, error in [
    ({"log": LOG.replace(TAG.encode(), b"b" * 32)}, TimeoutError),
    ({"log": LOG.replace(b"vis=60", b"vis=59")}, RuntimeError),
    ({"log": LOG.replace(b"n=1/1", b"n=32/32")}, ValueError),
    ({"crash": {"app": "Wii64", "pc": "0x10"}}, RuntimeError),
    ({"fail_file": "xfb_01.bin"}, RuntimeError),
    ({"timeout": 1}, TimeoutError),
]:
    try:
        run(**kwargs)
    except error:
        pass
    else:
        raise AssertionError("Accepted failing case " + repr(kwargs))

# This assertion goes red until the real guest records the host-generated tag.
root = Path(__file__).resolve().parents[1]
source = (root / "main/main_gc-menu2.cpp").read_text()
start = source.index('\tif (!strncmp(line, "result_tag=", 11))')
branch = source[start:source.index("#endif", start)]
with tempfile.TemporaryDirectory() as directory:
    tmp = Path(directory)
    (tmp / "tag.cpp").write_text('''
#include <cassert>
#include <cstring>
#include <string>
static std::string recorded;
void perfProf_mark(const char *line) { recorded = line; }
void apply(const char *line) {
''' + branch + '''
}
int main() {
    const std::string valid = "result_tag=" + std::string(32, 'a');
    apply(valid.c_str()); assert(recorded == valid);
    const char *bad[] = {"result_tag=", "result_tag=../perf.log",
      "result_tag=AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
      "result_tag=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"};
    for (const char *line : bad) { recorded.clear(); apply(line); assert(recorded.empty()); }
}
''')
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++11", "-fsanitize=address,undefined",
                    str(tmp / "tag.cpp"), "-o", str(tmp / "tag")], check=True)
    subprocess.run([str(tmp / "tag")], check=True)
print("Outbound Wii results: PASS (freshness, bounded files, crashes, timeout, no listener)")
