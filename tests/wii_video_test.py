"""Offline CLI smoke checks for the macOS AVFoundation video helper."""

import pathlib
import re
import subprocess
import sys
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = ROOT / "scripts/wii_video.swift"
WRAPPER = ROOT / "scripts/wii_video_capture.sh"
HELPER = ROOT / ".dev/tools/wii-video"


@unittest.skipUnless(sys.platform == "darwin", "wii_video requires macOS")
class VideoCLITest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        subprocess.run([str(WRAPPER), "list"], check=True, text=True, capture_output=True,
                       timeout=180)
        if not HELPER.is_file():
            raise AssertionError(f"wrapper did not create cached helper: {HELPER}")
        cls.temp = tempfile.TemporaryDirectory()
        cls.tmp = pathlib.Path(cls.temp.name)

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def run_helper(self, *args):
        return subprocess.run([str(HELPER), *map(str, args)], text=True,
                              capture_output=True, timeout=20)

    def assert_cli_error(self, result, message):
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertIn(message, result.stderr)
        self.assertNotIn("camera access", result.stderr)

    def test_list_runs_from_cached_helper(self):
        result = self.run_helper("list")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue(result.stdout.strip())

    def test_invalid_command_fails_without_camera_permission(self):
        self.assert_cli_error(self.run_helper("unknown"), "usage:")

    def test_wrong_duration_fails_without_camera_permission(self):
        result = self.run_helper("capture", self.tmp / "duration.mov", "--seconds", "0")
        self.assert_cli_error(result, "duration must be between 1 and 300 seconds")

    def test_existing_output_fails_without_camera_permission(self):
        output = self.tmp / "exists.jpg"
        output.write_bytes(b"keep")
        self.assert_cli_error(self.run_helper("snapshot", output), "output already exists:")
        self.assertEqual(output.read_bytes(), b"keep")

    def test_missing_directory_fails_without_camera_permission(self):
        output = self.tmp / "missing" / "frame.jpg"
        self.assert_cli_error(self.run_helper("snapshot", output), "output directory does not exist:")

    def test_default_device_is_exact_and_has_no_fallback(self):
        source = SOURCE.read_text()
        self.assertIn('private let defaultDevice = "UGREEN 15389"', source)
        self.assertIn("deviceName = defaultDevice", source)
        self.assertRegex(source, re.compile(
            r"devices\s*\.first\(where:\s*\{\s*\$0\.localizedName\s*==\s*deviceName\s*\}\)"
        ))
        self.assertNotRegex(source, r"\?\?\s*devices\.first\b")


if __name__ == "__main__":
    unittest.main()
