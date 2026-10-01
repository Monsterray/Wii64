"""Run the renderer's real ROM initialization with its real RDP state layout."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
header = (root / 'glN64_GX/gDP.h').read_text()
types = header[header.index('struct gDPCombine'):header.index('extern gDPInfo gDP;')]
source = (root / 'glN64_GX/RSP.cpp').read_text()
init = source[source.index('void RSP_Init()'):]
fixture = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
typedef uint64_t u64; typedef int16_t s16; typedef int32_t s32;
typedef float f32;
#define FALSE 0
''' + types + r'''
gDPInfo gDP;
struct gSPInfo { gDPTile *textureTile[2]; u32 changed; } gSP;
struct { u32 DList, uc_start, uc_dstart, infloop; } RSP;
u32 RDRAMSize;
void _RSP_SetGameHacks() {}
void DepthBuffer_Init() {}
void GBI_Init() {}
void OGL_Start() { gSP.changed = gDP.changed = 0xFFFFFFFF; }
''' + init + r'''
int main() {
    // Both the first boot and repeated switches must start from the same state.
    for (unsigned fill = 1; fill < 256; ++fill) {
        memset(&gDP, fill, sizeof(gDP));
        RSP_Init();
        gDPInfo expected = {};
        expected.loadTile = &gDP.tiles[7];
        expected.changed = 0xFFFFFFFF;
        assert(!memcmp(&gDP, &expected, sizeof(gDP)));
        assert(gSP.textureTile[0] == &gDP.tiles[0]);
        assert(gSP.textureTile[1] == &gDP.tiles[1]);
    }
}
'''
with tempfile.TemporaryDirectory(prefix='wii64-graphics-reset-') as directory:
    path = Path(directory)
    (path / 'test.cpp').write_text(fixture)
    for endian in ([], ['-D_BIG_ENDIAN']):
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++11', '-O2',
                        '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        *endian, str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
        subprocess.run([str(path / 'test')], check=True)
print('Graphics ROM reset: PASS (both state layouts)')
