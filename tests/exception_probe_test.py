"""Run the real Cause classifier; do not substitute guest exception semantics."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'main/perf_prof.c').read_text()
start = source.index('void perfProf_exceptionOccurred(')
end = source.index('\nvoid ', start + 5)
fixture = '''
#include <assert.h>
static struct { unsigned exceptions, exceptionCause[32], irqRcp, irqTimer, irqBoth, irqOther; } g;
''' + source[start:end] + '''
int main(void) {
    for (unsigned code = 0; code < 32; code++) {
        perfProf_exceptionOccurred((code << 2) | 0x80000000);
        assert(g.exceptionCause[code] == 1);
    }
    perfProf_exceptionOccurred(0x400);
    perfProf_exceptionOccurred(0x8000);
    perfProf_exceptionOccurred(0x8400);
    assert(g.exceptions == 35 && g.exceptionCause[0] == 4);
    assert(g.irqRcp == 1 && g.irqTimer == 1 && g.irqBoth == 1 && g.irqOther == 1);
}
'''
with tempfile.TemporaryDirectory(prefix='wii64-exception-probe-') as directory:
    path = Path(directory)
    (path / 'test.c').write_text(fixture)
    subprocess.run([os.environ.get('CC', 'cc'), '-std=c11', '-O2', '-Wall', '-Werror',
                    '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    str(path / 'test.c'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
menu = (root / 'main/main_gc-menu2.cpp').read_text()
chain = menu[menu.index('static bool chainNext'):menu.index('static void apply_diag_line')]
assert chain.index('perfProf_gameEnd(') < chain.index('diagStressBrowser()') < chain.index('autobootROM(')
print('Exception probes: Cause codes/IRQ masks and stopped browser lifecycle PASS')
