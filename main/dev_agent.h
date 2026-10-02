#ifndef WII64_DEV_AGENT_H
#define WII64_DEV_AGENT_H
/* The HBC-Reborn agent (main/dev_agent.c): in every Wii build (Makefile.wii). It is
   Wii64's HOME menu and gives development tools status, files, exit and crash
   reports. GameCube builds have no agent; these are no-ops there. */
#ifdef WII64_HBC_AGENT
#ifdef __cplusplus
extern "C" {
#endif
void devAgent_init(void);
int devAgent_exitRequested(void);
int devAgent_netReady(void);
void devAgent_alive(void);
void devAgent_hold(int hold);
void devAgent_pollHome(void);      /* per guest VI: HOME stops the game for the overlay */
int devAgent_homeAfterStop(void);  /* after go(): the overlay, if HOME stopped it; 1 = resume */
void devAgent_menuHome(void);      /* per menu frame: HOME opens the overlay */
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
#define devAgent_alive() ((void)0)
#define devAgent_hold(hold) ((void)0)
#define devAgent_pollHome() ((void)0)
#define devAgent_homeAfterStop() 0
#define devAgent_menuHome() ((void)0)
#endif
#endif
