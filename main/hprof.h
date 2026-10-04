#ifndef WII64_HPROF_H
#define WII64_HPROF_H

#if defined(HW_RVL) && defined(PERF_PROF) && defined(PERF_HPROF)
#define WII64_HPROF 1
#ifdef __cplusplus
extern "C" {
#endif
void hprof_configure(int enabled);
int hprof_requested(void);
void hprof_prepare(void);
void hprof_start(void);
void hprof_stop(void);
void hprof_jitRange(void *base, unsigned int size);
int hprof_dump(int game);
#ifdef __cplusplus
}
#endif
#else
#define hprof_configure(enabled) ((void)0)
#define hprof_requested() 0
#define hprof_prepare() ((void)0)
#define hprof_start() ((void)0)
#define hprof_stop() ((void)0)
#define hprof_jitRange(base, size) ((void)0)
#define hprof_dump(game) 0
#endif
#endif
