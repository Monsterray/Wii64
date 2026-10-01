"""Read-only HBC-Reborn monitoring inside an already leased Wii job."""
import importlib.util
import json
import time
from pathlib import Path


def load_client(path):
    spec = importlib.util.spec_from_file_location("wii64_hbc_client", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class RunWatch:
    def __init__(self, client, wii, output, elf=None):
        self.client, self.wii, self.output, self.elf = client, wii, Path(output), elf
        self.agent_seen = False
        self.next_poll = 0
        try:
            self.before = json.loads((self.output / "hbc-before.json").read_text()).get("crash")
        except (OSError, ValueError):
            self.before = None

    def poll(self):
        if not (self.output / ".sent").exists() or time.monotonic() < self.next_poll:
            return None
        self.next_poll = time.monotonic() + 5
        try:
            version = self.client.version(self.wii, 2)
            if self.client.is_agent(version):
                self.agent_seen = True
                # Keep the latest status, not an unbounded polling log.
                status = self.client.status(self.wii)
                (self.output / "agent-status.json").write_text(json.dumps(status, indent=2) + "\n")
                return None
            status = self.client.status(self.wii)
        except (OSError, self.client.HBCError):
            return None  # The reload stub takes the network down briefly.
        report = self.new_crash(status)
        if report:
            return report
        if self.agent_seen:
            return "Wii64 returned to HBC before it delivered all results"
        return None

    def new_crash(self, status):
        """Also check after /done: cleanup can crash after results are uploaded."""
        crash = status.get("crash")
        if crash and crash != self.before and crash.get("app") == "Wii64":
            (self.output / "crash.json").write_text(json.dumps(crash, indent=2) + "\n")
            report = self.client.crash_report(crash, self.elf)
            (self.output / "crash.txt").write_text(report + "\n")
            return report
        return None
