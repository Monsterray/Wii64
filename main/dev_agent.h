#ifndef WII64_DEV_AGENT_H
#define WII64_DEV_AGENT_H
#ifdef WII64_HBC_AGENT
#ifdef __cplusplus
extern "C" {
#endif
void devAgent_init(void);
int devAgent_exitRequested(void);
int devAgent_netReady(void);
#ifdef PERF_PROF
extern unsigned int devAgent_crashVi;
void devAgent_testCrash(void);
#endif
#ifdef __cplusplus
}
#endif
#else
#define devAgent_init() ((void)0)
#define devAgent_exitRequested() 0
#define devAgent_netReady() 1
#endif
#endif
