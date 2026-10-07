"""Offline tests for the Wii video queue and owned-process wrapper."""

import os
import json
from pathlib import Path
import runpy
import signal
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import call, mock_open, patch


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
try:
    runner = runpy.run_path(str(ROOT / "scripts/wii_video_run.py"))
finally:
    sys.path.pop(0)
RUNNER = runner["main"].__globals__


class VideoRunTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()
        self.cwd = Path.cwd()
        os.chdir(self.root)
        self.addCleanup(os.chdir, self.cwd)
        (self.root / ".dev/runs").mkdir(parents=True)
        (self.root / "scripts").mkdir()
        (self.root / "scripts/build_wii_video.py").write_text("# fixture builder\n")

    def main_patches(self):
        return patch.dict(RUNNER, {"ROOT": self.root, "build": lambda: self.root / "helper"})

    def test_capture_without_lease_queues_frozen_helper(self):
        client = self.root / "client.py"
        client.touch()
        binary = self.root / "helper"
        binary.write_bytes(b"mock helper")
        output = ".dev/runs/queued.mov"
        with self.main_patches(), patch.dict(os.environ, {"WII_BENCH_CLIENT": str(client)}, clear=True), \
                patch("subprocess.call", return_value=0) as queue_call, \
                patch.dict(RUNNER, {"run_owned": unittest.mock.Mock(side_effect=AssertionError)}):
            result = runner["main"](["capture", output])

        self.assertEqual(result, 0)
        queue_call.assert_called_once()
        command = queue_call.call_args.args[0]
        self.assertEqual(command[1:3], [str(client), "add"])
        self.assertIn("--native", command)
        frozen = Path(command[command.index("--native") + 1])
        self.assertEqual(frozen.parent.parent, self.root / ".dev/runs")
        self.assertEqual(frozen.read_bytes(), b"mock helper")
        self.assertTrue((frozen.parent / "wii_video_run.py").is_file())
        self.assertTrue((frozen.parent / "repository.json").is_file())
        self.assertEqual(command[-1], str((self.root / output).resolve()))

    def test_capture_with_lease_runs_owned_helper_directly(self):
        with self.main_patches(), patch.dict(os.environ, {
                "WII_BENCH_JOB_START": "started", "WII_BENCH_IP": "192.0.2.1"}, clear=True), \
                patch.dict(RUNNER, {"run_owned": unittest.mock.Mock(return_value=7)}):
            owned = RUNNER["run_owned"]
            result = runner["main"](["capture", ".dev/runs/leased.mov", "--seconds", "5"])

        self.assertEqual(result, 7)
        owned.assert_called_once_with(
            self.root / "helper", ["capture", str(self.root / ".dev/runs/leased.mov"),
                                   "--seconds", "5"], 48.0)

    def test_capture_paths_become_absolute_and_stay_under_run_tree(self):
        client = self.root / "client.py"
        client.touch()
        binary = self.root / "helper"
        binary.write_bytes(b"mock helper")
        args = ["capture", ".dev/runs/out.mov", "--frames-dir", ".dev/runs/frames",
                "--report", ".dev/runs/report.json"]
        with self.main_patches(), patch.dict(os.environ, {"WII_BENCH_CLIENT": str(client)}, clear=True), \
                patch("subprocess.call", return_value=0) as queue_call:
            runner["main"](args)

        command = queue_call.call_args.args[0]
        frozen_args = command[command.index("--") + 5:]
        self.assertEqual(frozen_args, [
            "capture", str(self.root / ".dev/runs/out.mov"), "--frames-dir",
            str(self.root / ".dev/runs/frames"), "--report",
            str(self.root / ".dev/runs/report.json")])

    def test_every_capture_output_rejects_traversal(self):
        client = self.root / "client.py"
        client.touch()
        cases = [
            ["capture", "../escape.mov"],
            ["capture", ".dev/runs/ok.mov", "--frames-dir", "../escape"],
            ["capture", ".dev/runs/ok.mov", "--report", "../escape.json"],
        ]
        with self.main_patches(), patch.dict(os.environ, {"WII_BENCH_CLIENT": str(client)}, clear=True), \
                patch("subprocess.call") as queue_call:
            for args in cases:
                with self.subTest(args=args), self.assertRaises(ValueError):
                    runner["main"](args)
        queue_call.assert_not_called()

    def test_discovery_command_is_not_queued(self):
        with self.main_patches(), patch.dict(os.environ, {
                "WII_BENCH_JOB_START": "started", "WII_BENCH_IP": "192.0.2.1"}, clear=True), \
                patch("subprocess.call", return_value=0) as direct_call, \
                patch.dict(RUNNER, {"run_owned": unittest.mock.Mock(side_effect=AssertionError)}):
            self.assertEqual(runner["main"](["list"]), 0)
        direct_call.assert_called_once_with([str(self.root / "helper"), "list"])

    def test_deadline_rejects_malformed_nonfinite_and_out_of_range_values(self):
        for flag, value in (("--seconds", "bad"), ("--seconds", "inf"),
                            ("--seconds", "-inf"), ("--warmup", "nan"),
                            ("--warmup", "31"), ("--seconds", "0")):
            with self.subTest(flag=flag, value=value), self.assertRaises(ValueError):
                runner["deadline"]([flag, value])

    def test_deadline_accepts_inclusive_bounds(self):
        self.assertEqual(runner["deadline"](["--seconds", "1", "--warmup", "0"]), 41)
        self.assertEqual(runner["deadline"](["--seconds", "600", "--warmup", "30"]), 670)

    def test_duplicate_and_incomplete_options_never_build_or_queue(self):
        with patch.dict(RUNNER, {"build": unittest.mock.Mock(side_effect=AssertionError)}):
            for args in (["capture"], ["capture", "x.mov", "--seconds"],
                         ["capture", "x.mov", "--seconds", "1", "--seconds", "600"]):
                with self.subTest(args=args), self.assertRaises(ValueError):
                    runner["main"](args)

    def test_report_cannot_replace_movie(self):
        with self.main_patches(), patch.dict(os.environ, {}, clear=True), \
                patch("subprocess.call") as queue_call:
            with self.assertRaisesRegex(ValueError, "different files"):
                runner["main"](["capture", ".dev/runs/collision.mov", "--report",
                                ".dev/runs/collision.mov"])
        queue_call.assert_not_called()

    def test_compact_report_keeps_failures_and_omits_frame_dump(self):
        path = self.root / "capture.json"
        path.write_text(json.dumps({"warmupSeconds": 1, "error": "retained failure",
                                   "samples": [{"monotonicSeconds": 0, "ptsSeconds": 100},
                                               {"monotonicSeconds": 1, "ptsSeconds": 101,
                                                "essentiallyBlack": True, "jpeg": "first.jpg"},
                                               {"monotonicSeconds": 2, "ptsSeconds": 102,
                                                "essentiallyBlack": False, "jpeg": "last.jpg"}]}))
        data = runner["report"](path)
        self.assertNotIn("samples", data)
        self.assertEqual(data["error"], "retained failure")
        self.assertEqual(data["steadyFPS"], 1)
        self.assertEqual(data["darkSampledFrames"], 1)
        self.assertEqual(data["representativeFrames"], ["first.jpg", "last.jpg"])

    def test_usb_info_filters_other_devices_without_building(self):
        fixture = {"SPUSBDataType": [{"_items": [
            {"vendor_id": "0x2b89", "product_id": "0x5389", "_name": "UGREEN"},
            {"vendor_id": "0x1234", "product_id": "0x5389", "_name": "other"}]}]}
        with patch("sys.platform", "darwin"), \
                patch("subprocess.run", return_value=subprocess.CompletedProcess([], 0, json.dumps(fixture))) as inventory, \
                patch.dict(RUNNER, {"build": unittest.mock.Mock(side_effect=AssertionError)}):
            self.assertEqual(runner["main"](["usb-info"]), 0)
        self.assertEqual(inventory.call_args.kwargs["timeout"], 30)

    def test_timeout_terminates_then_kills_only_owned_group_with_bounded_waits(self):
        process = unittest.mock.Mock()
        process.pid = 4321
        process.poll.return_value = None
        process.wait.side_effect = [
            subprocess.TimeoutExpired("helper", 2),
            subprocess.TimeoutExpired("helper", 12),
            0,
        ]
        with patch("os.open", return_value=42), patch("os.fdopen", mock_open()), \
                patch("fcntl.flock"), patch("subprocess.Popen", return_value=process) as popen, \
                patch("os.killpg") as killpg, patch("signal.signal", return_value="old"):
            with self.assertRaisesRegex(RuntimeError, "capture stopped"):
                runner["run_owned"]("helper", [], 2)

        popen.assert_called_once_with(["helper"], start_new_session=True)
        self.assertEqual(killpg.call_args_list, [
            call(4321, signal.SIGTERM), call(4321, signal.SIGKILL)])
        self.assertEqual(process.wait.call_args_list,
                         [call(timeout=2), call(timeout=12), call(timeout=5)],
                         "reaping after SIGKILL must also have a finite timeout")

    def test_interrupt_forwards_term_to_owned_group_and_restores_handlers(self):
        process = unittest.mock.Mock()
        process.pid = 8765
        process.poll.return_value = None
        installed = {}
        originals = {signal.SIGTERM: object(), signal.SIGINT: object()}

        def set_handler(signum, handler):
            if handler in originals.values():
                installed.pop(signum, None)
            else:
                installed[signum] = handler
            return originals[signum]

        waits = 0

        def interrupt_then_finish(timeout):
            nonlocal waits
            waits += 1
            if waits == 1:
                installed[signal.SIGINT](signal.SIGINT, None)
            return 0

        process.wait.side_effect = interrupt_then_finish
        with patch("os.open", return_value=42), patch("os.fdopen", mock_open()), \
                patch("fcntl.flock"), patch("subprocess.Popen", return_value=process), \
                patch("os.killpg") as killpg, patch("signal.signal", side_effect=set_handler) as signals:
            with self.assertRaisesRegex(RuntimeError, "capture stopped"):
                runner["run_owned"]("helper", [], 20)

        killpg.assert_called_once_with(8765, signal.SIGTERM)
        self.assertEqual(process.wait.call_args_list, [call(timeout=20), call(timeout=12)])
        self.assertEqual(signals.call_args_list[-2:], [
            call(signal.SIGTERM, originals[signal.SIGTERM]),
            call(signal.SIGINT, originals[signal.SIGINT])])
        self.assertFalse(installed)

    def test_lock_contention_fails_before_spawning_capture(self):
        with patch("os.open", return_value=42), patch("os.fdopen", mock_open()), \
                patch("fcntl.flock", side_effect=BlockingIOError), \
                patch("subprocess.Popen") as popen:
            with self.assertRaisesRegex(RuntimeError, "another Wii64 capture"):
                runner["run_owned"]("helper", [], 10)
        popen.assert_not_called()


if __name__ == "__main__":
    unittest.main()
