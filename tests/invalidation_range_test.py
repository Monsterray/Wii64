"""Compile the actual range walker with a modeled func tree; no PowerPC asm needed."""
import os
import re
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / "r4300/ppc/Wrappers.c").read_text()
body = source[source.index("void invalidate_func_range("):source.index("unsigned int dyna_mem(", source.index("void invalidate_func_range("))]
tree = (root / "r4300/ppc/FuncTree.c").read_text()
overlap = tree[tree.index("PowerPC_func* find_func_overlap("):tree.index("void insert_func(")]
word_count = re.search(r"unsigned int words = ([^;]+);", body)[1]
fixture = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PERF_SUBSYSTEM_C_SCOPE(stage) ((void)0)
typedef struct { unsigned int start_addr, end_addr; int alive; } PowerPC_func;
typedef struct node { PowerPC_func* function; struct node *left, *right; } PowerPC_func_node;
typedef struct { PowerPC_func_node* funcs; } PowerPC_block;
static unsigned char invalid_code[1 << 20];
#define invalid_code_get(page) invalid_code[page]
/* Funcs live on a few pages around the test address, in a BST per page. */
#define MAX_FUNCS 4096
static PowerPC_func funcs[MAX_FUNCS];
static PowerPC_func_node nodes[MAX_FUNCS];
static unsigned nfuncs, page_first[19], base_page;
static PowerPC_block page_blocks[1 << 20];
static PowerPC_block* block_ptrs[1 << 20];
#define blocks block_ptrs
static uint32_t actual[MAX_FUNCS], expected[MAX_FUNCS];
static unsigned used;
static uint32_t seed = 1;
static uint32_t random32(void) { seed = seed * 1664525u + 1013904223u; return seed; }
static void tree_insert(PowerPC_func_node** at, PowerPC_func_node* n) {
    while (*at) at = n->function->start_addr < (*at)->function->start_addr ? &(*at)->left : &(*at)->right;
    *at = n;
}
static void tree_remove(PowerPC_func_node** at, PowerPC_func* f) {
    while ((*at)->function != f) at = f->start_addr < (*at)->function->start_addr ? &(*at)->left : &(*at)->right;
    PowerPC_func_node* n = *at;
    if (!n->left) *at = n->right;
    else if (!n->right) *at = n->left;
    else {
        PowerPC_func_node** pre;
        for (pre = &n->left; (*pre)->right; pre = &(*pre)->right);
        n->function = (*pre)->function;
        *pre = (*pre)->left;
    }
}
static PowerPC_func* containing(uint32_t addr) {
    for (unsigned i = 0; i < nfuncs; i++)
        if (funcs[i].alive && addr >= funcs[i].start_addr && addr < funcs[i].end_addr) return &funcs[i];
    return NULL;
}
static void RecompCache_Free(unsigned int addr) {
    for (unsigned i = 0; i < nfuncs; i++)
        if (funcs[i].alive && funcs[i].start_addr == addr) {
            funcs[i].alive = 0;
            tree_remove(&blocks[addr >> 12]->funcs, &funcs[i]);
            assert(used < MAX_FUNCS);
            actual[used++] = addr;
            return;
        }
}
/* The original per-point walker: free the func holding the point. */
static __attribute__((unused)) void invalidate_func(unsigned addr) {
    if (!invalid_code_get(addr >> 12)) {
        PowerPC_func* f = containing(addr);
        if (f) RecompCache_Free(f->start_addr);
    }
}
''' + overlap + body + '\nstatic unsigned word_count(unsigned addr, unsigned bytes) { return bytes ? ' + word_count + ' : 0; }\n' + r'''
/* Disjoint funcs, word aligned, each inside one page, in random tree order. */
static void make_funcs(uint32_t addr) {
    memset(nodes, 0, sizeof(nodes));
    for (unsigned i = 0; i < nfuncs; i++) block_ptrs[funcs[i].start_addr >> 12]->funcs = NULL;
    nfuncs = 0;
    base_page = ((addr >> 12) - 1) & 0xfffff;
    for (unsigned p = 0; p < 18; p++) {
        uint32_t page = (base_page + p) & 0xfffff, at = page << 12;
        page_first[p] = nfuncs;
        block_ptrs[page] = &page_blocks[page];
        page_blocks[page].funcs = NULL;
        unsigned first = nfuncs;
        while (nfuncs < MAX_FUNCS && random32() % 8) {
            uint32_t start = at + (random32() % 64) * 4, end = start + 4 + (random32() % 96) * 4;
            if (end > (page << 12) + 0x1000 || end < start) break;
            funcs[nfuncs++] = (PowerPC_func){start, end, 1};
            at = end;
        }
        for (unsigned n = nfuncs - first; n > 1; n--) { /* shuffle the insert order */
            unsigned j = first + random32() % n;
            PowerPC_func t = funcs[first + n - 1]; funcs[first + n - 1] = funcs[j]; funcs[j] = t;
        }
        for (unsigned i = first; i < nfuncs; i++) {
            nodes[i].function = &funcs[i];
            tree_insert(&page_blocks[page].funcs, &nodes[i]);
        }
    }
    page_first[18] = nfuncs;
}
static int keep_funcs;
static void check(uint32_t addr, unsigned bytes) {
    if (!keep_funcs) make_funcs(addr);
    unsigned count = 0;
    for (unsigned i = 0; i < nfuncs; i++) funcs[i].alive = 2; /* reference pass: 2 = alive */
    /* Byte coverage is the invariant, not the legacy four-byte probe stride. */
    for (unsigned i = 0; i < bytes; i++) {
        uint32_t at = addr + i;
        unsigned p = ((at >> 12) - base_page) & 0xfffff;
        if (invalid_code_get(at >> 12) || p >= 18) continue;
        for (unsigned f = page_first[p]; f < page_first[p + 1]; f++)
            if (funcs[f].alive == 2 && at >= funcs[f].start_addr && at < funcs[f].end_addr) {
                funcs[f].alive = 3;
                expected[count++] = funcs[f].start_addr;
            }
    }
    for (unsigned i = 0; i < nfuncs; i++) funcs[i].alive = 1;
    used = 0;
    invalidate_func_range(addr, bytes);
    if (used != count || memcmp(actual, expected, count * sizeof(uint32_t)))
        fprintf(stderr, "write %08x + %u bytes: invalidated %u funcs, expected %u\n", addr, bytes, used, count);
    assert(used == count && memcmp(actual, expected, count * sizeof(uint32_t)) == 0);
}
int main(void) {
    unsigned sizes[] = {0, 1, 2, 3, 4, 7, 8, 9, 0xfffffffc, 0xfffffffd, 0xfffffffe, 0xffffffff};
    for (unsigned offset = 0; offset < 4; offset++)
        for (unsigned i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++)
            assert(word_count(offset, sizes[i]) == (sizes[i] ? ((uint64_t)sizes[i] + offset + 3) / 4 : 0));
    for (unsigned i = 0; i < (1u << 20); i++) block_ptrs[i] = &page_blocks[i];
    memset(invalid_code, 1, sizeof(invalid_code));
    invalid_code[0] = invalid_code[1] = invalid_code[(1 << 20) - 1] = 0;
    invalid_code[0x80000] = invalid_code[0xa0001] = 0; /* aliases are distinct */
    base_page = 0x80000;
    nfuncs = 1;
    funcs[0] = (PowerPC_func){0x80000004, 0x80000008, 1};
    nodes[0].function = &funcs[0];
    page_blocks[base_page].funcs = &nodes[0];
    for (unsigned i = 1; i < 19; i++) page_first[i] = 1;
    keep_funcs = 1;
    check(funcs[0].start_addr - 2, 3); /* PI DMA / grouped byte stores cross an instruction boundary */
    keep_funcs = 0;
    uint32_t edges[] = {0, 1, 3, 4093, 4095, 4096, 0xfffffffdu, 0xffffffffu,
                        0x80000ffdu, 0xa0000fffu};
    for (unsigned a = 0; a < sizeof(edges)/sizeof(edges[0]); a++)
        for (unsigned bytes = 0; bytes <= 8200; bytes++) check(edges[a], bytes);
    for (unsigned i = 0; i < 3000; i++) { /* one store at or next to a func start */
        uint32_t page = 0x80000000u + (random32() & 0x7ff000u);
        invalid_code[page >> 12] = 0;
        make_funcs(page + 0x800);
        uint32_t at = nfuncs ? funcs[random32() % nfuncs].start_addr : page;
        keep_funcs = 1;
        check(at - 4 + random32() % 9, random32() % 10);
        keep_funcs = 0;
        invalid_code[page >> 12] = 1;
    }
    for (unsigned i = 0; i < 3000; i++) {
        uint32_t addr = random32();
        for (unsigned j = 0; j < 17; j++) invalid_code[(addr + j * 4096) >> 12] = random32() >> 31;
        check(addr, random32() % 65536);
    }
    puts("invalidation range: byte coverage, unaligned/wrap/aliases/many funcs per page: ok");
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
