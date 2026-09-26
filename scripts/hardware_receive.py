#!/usr/bin/env python3
"""Receive one Wii64 hardware run over the local network. No packages needed."""

import argparse
import http.server
import pathlib
import re
import time

MAX_FILE = 2 * 1024 * 1024
FILE_NAME = re.compile(r"/(perf\.log|(?:xfb_\d{2}\.bin|padtrace_\d{2}\.csv))\Z")


class Receiver(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.0"

    def do_POST(self):
        self.connection.settimeout(10)
        if self.client_address[0] != self.server.wii_ip:
            self.send_error(403)
            return
        try:
            size = int(self.headers.get("Content-Length", ""))
        except ValueError:
            self.send_error(411)
            return
        if not 0 <= size <= MAX_FILE:
            self.send_error(413)
            return
        if self.path == "/done":
            if size != 0 or not (self.server.output / "perf.log").is_file():
                self.send_error(400)
                return
            self.server.done = True
        else:
            match = FILE_NAME.fullmatch(self.path)
            if not match:
                self.send_error(404)
                return
            target = self.server.output / match.group(1)
            if target.exists():
                self.send_error(409)
                return
            data = self.rfile.read(size)
            if len(data) != size:
                self.send_error(400)
                return
            target.write_bytes(data)
            print(f"received {target.name}: {size} bytes", flush=True)
        self.send_response(200)
        self.send_header("Content-Length", "0")
        self.end_headers()

    def log_message(self, fmt, *args):
        print("Wii " + fmt % args, flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=pathlib.Path)
    parser.add_argument("--bind", required=True, help="Mac LAN IPv4 address")
    parser.add_argument("--wii-ip", required=True)
    parser.add_argument("--port", type=int, default=39364)
    parser.add_argument("--timeout", type=int, default=1200)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    with http.server.HTTPServer((args.bind, args.port), Receiver) as server:
        server.output = args.output
        server.wii_ip = args.wii_ip
        server.done = False
        server.timeout = 1
        (args.output / ".ready").touch()
        print(f"listening on {args.bind}:{args.port} for {args.wii_ip}", flush=True)
        deadline = time.monotonic() + args.timeout
        while not server.done and time.monotonic() < deadline:
            server.handle_request()
        (args.output / ".ready").unlink(missing_ok=True)
        if not server.done:
            raise SystemExit("timed out waiting for Wii results; check the SD card")
    print("hardware run received", flush=True)


if __name__ == "__main__":
    main()
