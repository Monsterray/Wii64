"""Release-check guards, preserved results and owned-app cleanup without a Wii."""
from pathlib import Path
import runpy
import tempfile
from unittest.mock import patch

check = runpy.run_path(str(Path(__file__).resolve().parents[1] /
                          "scripts/wii_release_check.py"))["check"]


class Client:
    HBCError = OSError

    def __init__(self, retraces=300, stalled=False, crash=None, exit_crash=None):
        self.agent = False
        self.home = False
        self.retraces = retraces
        self.stalled = stalled
        self.crash = crash
        self.exit_crash = exit_crash
        self.sent = self.exited = 0

    def status(self, wii):
        state = {"version": "1.10.0", "agent": self.agent, "crash": self.crash}
        if self.agent:
            if self.retraces and not self.stalled:
                self.retraces += 60
            state.update(app="Wii64", app_version="1.6.20",
                         safety={"stack_guard": "breakpoint",
                                 "frames": {"retraces": self.retraces}})
            if self.home:
                state["overlay_ui"] = {"menu": -1}
        return state

    def send(self, wii, dol, args, **kwargs):
        assert "AutoSave=0" in args
        self.agent = True
        self.crash = None  # SDK startup clears the prior retained report.
        self.sent += 1

    def screen(self, wii):
        return 2, 1, bytes(4)

    def yuyv_png(self, width, height, data):
        return b"fixture PNG"

    def send_keys(self, wii, keys):
        assert keys == "h"
        self.home = not self.home

    def exit_app(self, wii, seconds):
        self.agent = False
        self.crash = self.exit_crash
        self.exited += 1

    def hbc_wait(self, wii, seconds):
        assert not self.agent


with tempfile.TemporaryDirectory() as directory, patch("time.sleep"):
    root = Path(directory)
    client = Client()
    check(client, "fixture", Path("frozen.dol"), root / "pass", "1.6.20",
          "1.10.0", ["sd:/wii64/roms/fixture.z64"])
    assert client.sent == client.exited == 2
    assert (root / "pass/01-hbc-after.json").exists()
    client = Client(crash={"pc": "prior expected DSI"})
    check(client, "fixture", Path("frozen.dol"), root / "cleared", "1.6.20", "1.10.0", [])
    client = Client(exit_crash={"pc": "new crash"})
    try:
        check(client, "fixture", Path("frozen.dol"), root / "new-crash", "1.6.20", "1.10.0", [])
    except RuntimeError as error:
        assert "new crash" in str(error)
    else:
        raise AssertionError("Accepted a new retained crash")
    assert (root / "new-crash/00-hbc-after.json").exists()
    client = Client(retraces=0)
    try:
        check(client, "fixture", Path("frozen.dol"), root / "fail", "1.6.20", "1.10.0", [])
    except RuntimeError as error:
        assert "retrace callback" in str(error)
    else:
        raise AssertionError("Accepted a replaced SDK callback")
    assert client.exited == 1 and (root / "fail/00-hbc-after.json").exists()
    client = Client(stalled=True)
    try:
        check(client, "fixture", Path("frozen.dol"), root / "stalled", "1.6.20", "1.10.0", [])
    except RuntimeError as error:
        assert "retrace callback" in str(error)
    else:
        raise AssertionError("Accepted a counter which stopped after startup")
    assert client.exited == 1
    client = Client()
    client.agent = True
    try:
        check(client, "fixture", Path("frozen.dol"), root / "foreign", "1.6.20", "1.10.0", [])
    except RuntimeError as error:
        assert "no upload sent" in str(error)
    else:
        raise AssertionError("Uploaded over an existing app")
    assert client.sent == client.exited == 0
print("Release check: matched versions, callback failure, retained evidence and owned cleanup PASS")
