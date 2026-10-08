"""Preserve the SDK/Scan retrace chain across all Wii64 video resets."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
agent = (root / "main/dev_agent.c").read_text()
helper = re.search(r"void devAgent_restoreRetrace\(void\)\n\{.*?\n\}", agent, re.S)
assert helper, "Keep the SDK callback through a shared cold-path helper"
capture = re.search(r"agentRetraceCallback = VIDEO_SetPostRetraceCallback\(ScanPADSandReset\);\s*devAgent_restoreRetrace\(\);", agent)
assert capture and agent.index("hbc_agent_init(&cfg)") < capture.start()
main = (root / "main/main_gc-menu2.cpp").read_text().split("int main(int argc, const char* argv[])", 1)[1]
assert main.index("devAgent_restoreRetrace();") < main.index("devAgent_init();") < main.index("new MenuContext(vmode)")
graphics = (root / "libgui/GraphicsGX.cpp").read_text()
assert graphics.count("devAgent_restoreRetrace();") == 3
assert "VIDEO_SetPostRetraceCallback" not in main + graphics

# Compile the actual helper and callback-capture statements. VIDEO_Init removes
# callbacks; the SDK chains Scan once, and every subsequent video mode retains it.
fixture = r'''
#include <assert.h>
typedef unsigned int u32;
typedef void (*VIRetraceCallback)(u32);
static VIRetraceCallback active, previous, agentRetraceCallback;
static unsigned scans, frames;
static VIRetraceCallback VIDEO_SetPostRetraceCallback(VIRetraceCallback cb) {
    VIRetraceCallback old = active; active = cb; return old;
}
static void VIDEO_Init(void) { active = 0; }
static void ScanPADSandReset(u32 count) { (void)count; scans++; }
static void sdk(u32 count) { frames++; previous(count); }
'''
fixture += helper.group() + "\nstatic void capture(void) {" + capture.group() + "}\n"
fixture += r'''
int main(void) {
    devAgent_restoreRetrace(); active(0); assert(scans == 1 && frames == 0);
    previous = VIDEO_SetPostRetraceCallback(sdk);
    capture();
    for (unsigned i = 0; i < 3; i++) {
        VIDEO_Init(); devAgent_restoreRetrace(); devAgent_restoreRetrace(); active(i);
        assert(scans == i + 2 && frames == i + 1);
    }
    /* Old SDK or failed init has no wrapper: preserve normal input scans. */
    agentRetraceCallback = 0; VIDEO_Init(); devAgent_restoreRetrace(); capture();
    active(0); assert(scans == 5 && frames == 3);
}
'''
gc = r'''
#include <assert.h>
static unsigned scans;
static void ScanPADSandReset(unsigned count) { (void)count; scans++; }
static void (*active)(unsigned);
static void VIDEO_SetPostRetraceCallback(void (*cb)(unsigned)) { active = cb; }
#include "main/dev_agent.h"
int main(void) { devAgent_restoreRetrace(); active(0); assert(scans == 1); }
'''
with tempfile.TemporaryDirectory() as directory:
    for name, source in (("wii", fixture), ("gc", gc)):
        binary = Path(directory) / name
        subprocess.run(["cc", "-x", "c", "-std=c99", "-Wall", "-Wextra", "-Werror",
                        "-I", str(root), "-o", str(binary), "-"], input=source, text=True, check=True)
        subprocess.run([str(binary)], check=True)
print("Agent retrace: all video resets, one SDK/input call per VI, old SDK and GC fallback PASS")
