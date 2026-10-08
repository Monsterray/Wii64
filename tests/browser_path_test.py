"""Run with python3 tests/browser_path_test.py: the ROM browser's up/root rules."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
src = (root / "fileBrowser/fileBrowser-libfat.c").read_text(errors="replace")
start = src.index("int fileBrowser_libfat_isRoot(")
body = src[start:src.index("/* The folder of the last ROM loaded", start)]
fixture = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
''' + body + r'''
static void up(const char* from, int drop, const char* want) {
    char p[256];
    strcpy(p, from);
    fileBrowser_libfat_parent(p, drop);
    if (strcmp(p, want)) { printf("%s (%d) -> %s, want %s\n", from, drop, p, want); assert(0); }
}
int main(void) {
    assert(fileBrowser_libfat_isRoot("sd:/") && fileBrowser_libfat_isRoot("usb:/") && fileBrowser_libfat_isRoot("sd:"));
    assert(!fileBrowser_libfat_isRoot("sd:/ROMS") && !fileBrowser_libfat_isRoot("sd:/wii64/roms"));
    up("sd:/ROMS/N64", 0, "sd:/ROMS");
    up("sd:/ROMS", 0, "sd:/");
    up("sd:/", 0, "sd:/");
    up("sd:/wii64/roms/Mario Party 3 (USA).z64", 0, "sd:/wii64/roms"); /* remembered folder */
    up("sd:/Game.z64", 0, "sd:/");
    up("sd:/ROMS/N64/..", 1, "sd:/ROMS");  /* the '..' entry */
    up("sd:/ROMS/..", 1, "sd:/");
    up("sd:/ROMS/N64", 1, "sd:/ROMS/N64"); /* an ordinary folder opens as it is */
    up("usb:/wii64/roms/..", 1, "usb:/wii64");
    puts("browser paths: root, parent, '..' entry, remembered folder: ok");
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix="wii64-browser-") as d:
    c, exe = Path(d) / "t.c", Path(d) / "t"
    c.write_text(fixture)
    subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=address,undefined", str(c), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
