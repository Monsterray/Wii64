#ifndef TEST_PERF_IRQ_H
#define TEST_PERF_IRQ_H
static inline unsigned int IRQ_Disable(void) { return 0; }
static inline void IRQ_Restore(unsigned int level) { (void)level; }
#endif
