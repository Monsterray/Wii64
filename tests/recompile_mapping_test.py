"""A consumed delay slot must not extend the compiled address-map allocation."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'r4300/ppc/Recompile.c').read_text()
loop = source[source.index('// Readjusting pointers to the func buffers'):]
loop = loop[loop.index('\tfor(i=0;'):]
loop = loop[:loop.index('\n\tfor(i=0; i<current_jump;')]
fixture = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
typedef uint32_t MIPS_instr;
typedef uint32_t PowerPC_instr;
int main(void) {
    MIPS_instr mips[74], *src_first=mips, *src=mips+73, *src_last=mips+72;
    unsigned addr_first=0x80001000, addr_last=addr_first+72*sizeof(MIPS_instr);
    PowerPC_instr code_buffer[100], generated[100];
    PowerPC_instr *map[73], **code_addr=map;
    struct { PowerPC_instr *code; } object={generated}, *func=&object;
    for(int n=0;n<73;n++) map[n]=code_buffer+3;
    map[1]=NULL;
    int i;
LOOP
    assert(map[0]==generated+3 && map[71]==generated+3 && !map[1]);
    assert(map[72]==code_buffer+3); /* adjacent heap footer: unchanged */
    (void)src; (void)src_first; (void)addr_first; (void)addr_last;
    puts("Dynarec mapping: consumed end delay slot cannot overwrite the heap footer PASS");
}
'''.replace('LOOP', loop)
with tempfile.TemporaryDirectory(prefix='wii64-mapping-') as directory:
    source, binary = Path(directory)/'test.c', Path(directory)/'test'
    source.write_text(fixture)
    subprocess.run(shlex.split(os.environ.get('CC', 'cc')) + ['-std=c11', '-O1', '-Wall', '-Wextra', '-Werror', '-Wno-sign-compare', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
