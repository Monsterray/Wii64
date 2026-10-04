"""Exercise the installed graph server through MCP stdio, without changing source."""
import json
from pathlib import Path
import queue
import subprocess
import threading
import time

ROOT = Path(__file__).resolve().parents[1]


def main():
    proc = subprocess.Popen(
        ["bash", str(ROOT / ".dev/code_graph.sh")], cwd=ROOT,
        stdin=subprocess.PIPE, stdout=subprocess.PIPE,
    )
    replies = queue.Queue()

    def read_replies():
        for line in proc.stdout:
            replies.put(line)
        replies.put(None)

    threading.Thread(target=read_replies, daemon=True).start()

    def send(message):
        proc.stdin.write((json.dumps(message) + "\n").encode())
        proc.stdin.flush()

    def request(number, method, params):
        send({"jsonrpc": "2.0", "id": number, "method": method, "params": params})
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            try:
                line = replies.get(timeout=max(0, deadline - time.monotonic()))
            except queue.Empty:
                break
            assert line is not None, "MCP server closed stdout"
            reply = json.loads(line)
            if reply.get("id") == number:
                assert "error" not in reply, reply
                result = reply["result"]
                assert not result.get("isError"), result
                return result
        raise AssertionError(f"MCP timeout: {method}")

    try:
        info = request(1, "initialize", {
            "protocolVersion": "2024-11-05", "capabilities": {},
            "clientInfo": {"name": "wii64-graph-check", "version": "1"},
        })
        send({"jsonrpc": "2.0", "method": "notifications/initialized"})
        names = {tool["name"] for tool in request(2, "tools/list", {})["tools"]}
        assert {"get_graph_schema", "trace_path", "check_index_coverage"} <= names, names
        assert not {"delete_project", "manage_adr", "index_repository", "ingest_traces"} & names, names
        schema = request(3, "tools/call", {
            "name": "get_graph_schema", "arguments": {"project": "wii64"},
        })
        assert "Function" in json.dumps(schema), schema
        trace = request(4, "tools/call", {
            "name": "trace_path", "arguments": {
                "project": "wii64", "function_name": "dynarec",
                "direction": "outbound", "depth": 1, "limit": 30,
                "max_output_tokens": 3200, "include_evidence": True,
            },
        })
        text = json.dumps(trace)
        assert "find_func" in text and "recompile_block" in text, trace
        print(f'MCP PASS: {info["serverInfo"]}; {len(names)} tools; dynarec call query passed')
    finally:
        proc.stdin.close()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.terminate()
            proc.wait(timeout=5)


if __name__ == "__main__":
    main()
