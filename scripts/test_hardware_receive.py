#!/usr/bin/env python3
"""Small protocol check for the Wii hardware result receiver."""

import http.client
import pathlib
import tempfile
import threading
import unittest
from http.server import HTTPServer

from hardware_receive import Receiver


class ReceiverTest(unittest.TestCase):
    def test_upload_and_finish(self):
        with tempfile.TemporaryDirectory() as temp:
            with HTTPServer(("127.0.0.1", 0), Receiver) as server:
                server.output = pathlib.Path(temp)
                server.wii_ip = "127.0.0.1"
                server.done = False
                thread = threading.Thread(target=server.serve_forever)
                thread.start()
                try:
                    conn = http.client.HTTPConnection(*server.server_address)
                    conn.request("POST", "/../bad", b"x")
                    self.assertEqual(conn.getresponse().status, 404)
                    conn.close()
                    conn = http.client.HTTPConnection(*server.server_address)
                    conn.request("POST", "/perf.log", b"game: n=1/1 how=vis\n")
                    self.assertEqual(conn.getresponse().status, 200)
                    conn.close()
                    conn = http.client.HTTPConnection(*server.server_address)
                    conn.request("POST", "/done", b"")
                    self.assertEqual(conn.getresponse().status, 200)
                    conn.close()
                    self.assertTrue(server.done)
                    self.assertIn("game:", (pathlib.Path(temp) / "perf.log").read_text())
                finally:
                    server.shutdown()
                    thread.join()


if __name__ == "__main__":
    unittest.main()
