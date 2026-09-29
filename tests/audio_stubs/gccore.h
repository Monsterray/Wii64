#ifndef TEST_GCCORE_H
#define TEST_GCCORE_H
#include <stdint.h>
#include <stdbool.h>
#define MIN(a, b) ((a) < (b) ? (a) : (b))
static inline uint32_t IRQ_Disable(void) { return 0; }
static inline void IRQ_Restore(uint32_t level) { (void)level; }
#endif
