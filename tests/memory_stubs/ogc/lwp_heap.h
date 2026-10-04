#ifndef TEST_MEMORY_HEAP_H
#define TEST_MEMORY_HEAP_H
#include <stdint.h>
typedef uint32_t u32;
typedef uint64_t u64;
typedef int BOOL;
#define HEAP_BLOCK_USED 1u
#define HEAP_DUMMY_FLAG 1u
#define HEAP_OVERHEAD 8u
#define HEAP_BLOCK_USED_OVERHEAD 8u /* PPC allocator's two 32-bit words */
typedef struct heap_block {
    u32 back_flag, front_flag;
} heap_block;
typedef struct { heap_block *start, *final; u32 pg_size; } heap_cntrl;
u32 __lwp_heap_init(heap_cntrl *, void *, u32, u32);
void *__lwp_heap_allocate(heap_cntrl *, u32);
BOOL __lwp_heap_free(heap_cntrl *, void *);
#endif
