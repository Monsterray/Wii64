"""Execute the real browser allocation/cleanup blocks with failing allocators."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'menu/SelectRomFrame.cpp').read_text()
alloc = source[source.index('\t//Init textures'):source.index('\tGX_InvalidateTexAll();', source.index('\t//Init textures'))]
start = source.index('void Func_ReturnFromSelectRomFrame()\n{')
cleanup = source[start:source.index('\nvoid Func_SR_Select1()', start)]
start = source.index('\tif(boxartTexCache == NULL) {')
controller = source[start:source.index('\n#endif\n\n\tfor (int i = 0; i<NUM_FILE_SLOTS;', start)]
fixture = r'''
#include <cassert>
#include <cstdlib>
#include <cstring>
using u8 = unsigned char;
struct GXColor { unsigned char r,g,b,a; };
#define NUM_FILE_SLOTS 16
#define BOXART_TEX_SIZE 46080
#define PERF_MEM_BOXART 4
static u8 *fileTextures[16];
static int calls, released, fail = -1, message, frame;
static void *boxartTexCache = (void*)1;
struct heap_cntrl { unsigned unused; };
#define BOXART_ICON_LO nullptr
#define BOXART_ICON_SIZE 786432
static bool initFail;
static int registered;
static void *dir_entries, *rom_headers, *rom_headers_valid;
static int num_entries, current_page, max_page;
struct Button { u8 *texture = nullptr; void setBoxTexture(u8 *p) { texture=p; }
                void setLabelColor(GXColor) {} } buttons[21];
struct { Button *button; } FRAME_BUTTONS[21];
struct MenuContext { enum {FRAME_MAIN}; void setActiveFrame(int) { frame++; } } context;
static MenuContext *pMenuContext = &context;
namespace menu { struct MessageBox { static MessageBox& getInstance() { static MessageBox m; return m; }
 void fadeMessage(const char*) { message++; } }; }
static void *allocate(unsigned n) { return calls++ == fail ? nullptr : malloc(n); }
static void *perfMem_allocate(int,void*,unsigned n) { return allocate(n); }
static void perfMem_free(int,void*,void *p) { released++; free(p); }
static void *memalign(unsigned,unsigned n) { return allocate(n); }
static void DCFlushRange(void *p,unsigned) { assert(p); }
static void countedFree(void *p) { released++; std::free(p); }
static int perfMem_init(int, void*, void*, unsigned, unsigned) { return !initFail; }
static void DEBUG_registerHeap(void *p, const char*) { assert(p); registered++; }
#define free countedFree
#define malloc allocate
''' + cleanup + '\nstatic void activate() {\n' + alloc + '\n}\n' + '\nstatic void initController() {\n' + controller + '\n}\n' + r'''
int main() {
    for (unsigned i=0;i<21;i++) FRAME_BUTTONS[i].button=&buttons[i];
    for (unsigned condition=0; condition<3; condition++) {
        calls=released=registered=0; boxartTexCache=nullptr;
        fail=condition==0 ? 0 : -1; initFail=condition==1;
        initController();
        assert(bool(boxartTexCache)==(condition==2));
        assert(registered==int(condition==2) && released==int(condition==1));
        if (boxartTexCache) free(boxartTexCache);
    }
    boxartTexCache=(void*)1;
    for (fail=-1;fail<16;fail++) {
        calls=released=message=frame=0;
        activate();
        if (fail<0) { assert(calls==16 && !message); Func_ReturnFromSelectRomFrame(); }
        else { assert(calls==fail+1 && released==fail && message==1 && frame==1); }
        for (unsigned i=0;i<16;i++) assert(!fileTextures[i] && !buttons[i+5].texture);
    }
#ifdef HW_RVL
    boxartTexCache=nullptr; calls=message=0; activate();
    assert(!calls && message==1);
#endif
}
'''
with tempfile.TemporaryDirectory(prefix='wii64-boxart-') as directory:
    path = Path(directory)
    (path / 'test.cpp').write_text(fixture)
    for flags in (['-DSHOW_DEBUG'], ['-DHW_RVL', '-DSHOW_DEBUG']):
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++11', '-O2',
                        '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        *flags, str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
        subprocess.run([str(path / 'test')], check=True)
print('Boxart allocation: every failure slot, cleanup/reopen and missing Wii heap PASS')
