#ifndef AUTOTEST_H
#define AUTOTEST_H

#ifdef __cplusplus
extern "C" {
#endif

void autotest_load(void);
void autotest_load_result(int result);
void autotest_tick(void);

#ifdef __cplusplus
}
#endif

#endif
