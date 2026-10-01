"""Deterministic agent/HBC distinction and stale-crash attribution; no Wii."""
import json
from pathlib import Path
import sys
import tempfile
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from hbc_watch import RunWatch


class Client:
    HBCError = RuntimeError
    running = "1.8.6"
    crash = {"app": "Wii64", "pc": "0x80001234", "uptime_ms": 1}
    @classmethod
    def version(cls, wii, timeout):
        return cls.running
    @staticmethod
    def is_agent(version):
        return version.endswith(" agent")
    @classmethod
    def status(cls, wii):
        return {"crash": cls.crash}
    @staticmethod
    def crash_report(crash, elf):
        assert elf == "matching.elf"
        return "symbolized " + crash["pc"]


with tempfile.TemporaryDirectory() as directory:
    out = Path(directory)
    (out / "hbc-before.json").write_text(json.dumps({"crash": Client.crash}))
    watcher = RunWatch(Client, "fixture", out, "matching.elf")
    assert watcher.poll() is None and not (out / "crash.json").exists()
    (out / ".sent").touch()
    assert watcher.poll() is None  # previous crash not this run
    Client.running = "1.8.6 agent"; watcher.next_poll = 0
    assert watcher.poll() is None and watcher.agent_seen
    Client.running = "1.8.6"; watcher.next_poll = 0
    Client.crash = dict(Client.crash, uptime_ms=3000)
    assert watcher.poll() == "symbolized 0x80001234"
    assert json.loads((out / "crash.json").read_text())["uptime_ms"] == 3000
    watcher = RunWatch(Client, "fixture", out, "matching.elf")
    Client.crash = None
    Client.running = "1.8.6 agent"; assert watcher.poll() is None
    Client.running = "1.8.6"; watcher.next_poll = 0
    assert "before" in watcher.poll()
    # /done stops receiver polling, but a crash during cleanup must still fail.
    Client.crash = {"app": "Wii64", "pc": "0x80001234", "uptime_ms": 4000}
    assert watcher.new_crash({"crash": Client.crash}) == "symbolized 0x80001234"
    assert watcher.new_crash({"crash": {"app": "Another app"}}) is None
print("HBC agent distinction, launch arming, stale/new crashes and early return: ok")
