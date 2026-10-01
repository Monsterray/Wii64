"""Compile the actual range walker with a modeled cache; no PowerPC asm needed."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "r4300/ppc/Wrappers.c").read_text()
body = source[source.index("void invalidate_func_range("):source.index("unsigned int dyna_mem(", source.index("void invalidate_func_range("))]
fixture = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static unsigned char invalid_code[1 << 20];
static uint32_t actual[65536], expected[65536];
static unsigned used;
static uint32_t seed = 1;
static uint32_t random32(void) { seed = seed * 1664525u + 1013904223u; return seed; }
#define invalid_code_get(page) invalid_code[page]
/* Record every address at which the real function would consult its tree.
   Multiple functions on a valid page must not be skipped after the first hit. */
static void invalidate_func(unsigned addr) {
    if (!invalid_code_get(addr >> 12)) {
        assert(used < 65536);
        actual[used++] = addr;
    }
}
''' + body + r'''
static void check(uint32_t addr, unsigned bytes) {
    unsigned count = 0;
    for (unsigned i = 0; i < bytes; i += 4) {
        uint32_t at = addr + i;
        if (!invalid_code_get(at >> 12)) expected[count++] = at;
    }
    used = 0;
    invalidate_func_range(addr, bytes);
    assert(used == count && memcmp(actual, expected, count * sizeof(uint32_t)) == 0);
}
int main(void) {
    memset(invalid_code, 1, sizeof(invalid_code));
    invalid_code[0] = invalid_code[1] = invalid_code[(1 << 20) - 1] = 0;
    invalid_code[0x80000] = invalid_code[0xa0001] = 0; /* aliases are distinct */
    uint32_t edges[] = {0, 1, 3, 4093, 4095, 4096, 0xfffffffdu, 0xffffffffu,
                        0x80000ffdu, 0xa0000fffu};
    for (unsigned a = 0; a < sizeof(edges)/sizeof(edges[0]); a++)
        for (unsigned bytes = 0; bytes <= 8200; bytes++) check(edges[a], bytes);
    for (unsigned i = 0; i < 10000; i++) {
        uint32_t addr = random32();
        for (unsigned j = 0; j < 17; j++) invalid_code[(addr + j * 4096) >> 12] = random32() >> 31;
        check(addr, random32() % 65536);
    }
    puts("invalidation range: 92010 legacy comparisons, unaligned/wrap/aliases/multiple hits: ok");
}
'''
with tempfile.TemporaryDirectory(prefix="wii64-invalidation-") as directory:
    path = Path(directory)
    (path / "test.c").write_text(fixture)
    for mode in (0, 1):
        subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                        f"-DDYNAREC_INVALIDATE_PAGE_SKIP={mode}", str(path / "test.c"), "-o", str(path / "test")], check=True)
        subprocess.run([str(path / "test")], check=True)
