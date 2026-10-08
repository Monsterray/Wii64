/****************************************************************************************
* timers.c - timer functions borrowed from mupen64 and mupen64plus, modified by sepp256
****************************************************************************************/

#include <stdio.h>
#include "winlnxdefs.h"
#include "ogc/lwp_watchdog.h"
#include "rom.h"
#include "timers.h"
#include "../gc_memory/memory.h"
#include "../r4300/r4300.h"
#include "../gui/DEBUG.h"
#include "gamehacks.h"
#include "perf_prof.h"
#include "dev_agent.h"

timers Timers = {0.0, 0.0, 0, 1};
float VILimit = 60.0;
double VILimitMicroseconds = 1000000.0/60.0;

/* Guest VIs since the current game started (loadROM() and a chain zero it), and the
   count a chained game stops at (0 = never). Counting guest VIs rather than wall time is
   what makes two runs of the same game comparable, in Dolphin and on hardware alike. */
unsigned int diag_vi_count = 0;
unsigned int diag_stop_vi = 0;
extern void stop_it(void);

int GetVILimit(void)
{
	switch (ROM_HEADER.Country_code&0xFF)
	{
		// PAL codes
		case 0x44:
		case 0x46:
		case 0x49:
		case 0x50:
		case 0x53:
		case 0x55:
		case 0x58:
		case 0x59:
			return 50;
			break;

		// NTSC codes
		case 0x37:
		case 0x41:
		case 0x45:
		case 0x4a:
			return 60;
			break;

		// Fallback for unknown codes
		default:
			return 60;
	}
}

void InitTimer(void) {
	VILimit = GetVILimit();
	VILimitMicroseconds = (double) 1000000.0/VILimit;
	Timers.frameDrawn = 0;
}

extern unsigned int usleep(unsigned int us);
typedef void (*GameSpecificHack) (void);

// To handle a ucode that yields part-way through a display list
static int Dlist_Incomplete = 0;

void dlist_incomplete(void) {
	Dlist_Incomplete = 1;
}

/* Microseconds from the 64-bit time base. gettick() is its low 32 bits and wraps
   every 70.7 s, which restarted the rate windows and left a VI unpaced each time. */
static u64 now_us(void)
{
	return ticks_to_microsecs(gettime());
}

void new_frame(void) {
	u64 CurrentFPSTime;
	static u64 CounterTime;
	static int Fps_Counter=0;
	const int dlistCompleted = !Dlist_Incomplete;
	Dlist_Incomplete = 0;

	if (r4300.stop) {
		CounterTime = now_us();
		Fps_Counter = 0;
		return;
	}

	//if (!Config.showFPS) return;
	if (dlistCompleted)
		Fps_Counter++;
	Timers.frameDrawn = 1;
	
	CurrentFPSTime = now_us();
	
	if (CounterTime > CurrentFPSTime) {
		CounterTime = now_us();
		Fps_Counter = 0;
	}
	else if (CurrentFPSTime - CounterTime >= 500000.0 ) {
		Timers.fps = (float) (Fps_Counter * 1000000.0 / (CurrentFPSTime - CounterTime));
		perfProf_fpsSample(Timers.fps);
		CounterTime = now_us();
		Fps_Counter = 0;
	}
	// Apply game specific hacks until we resolve actual issues in the core!
	if(GetGameSpecificHack()) {
		GameSpecificHack hack = (GameSpecificHack)GetGameSpecificHack();
		hack();
	}
}

void new_vi(void) {
	if (devAgent_exitRequested()) r4300.stop = 1;
	devAgent_alive(); // a guest VI is progress (the HBC agent's hang watchdog)
	devAgent_pollHome(); // HOME stops the game; the overlay opens after go()
	u64 Dif;
	u64 CurrentFPSTime;
	static u64 LastFPSTime = 0;
	static u64 CounterTime = 0;
	static u64 CalculatedTime;
	static int VI_Counter = 0;
	static int VI_WaitCounter = 0;
	long time;
	
	if (r4300.stop) {
		CounterTime = now_us();
		VI_Counter = 0;
		return;
	}

	start_section(IDLE_SECTION);
//	if ( (!Config.showVIS) && (!Config.limitFps) ) return;
	VI_Counter++;
	if (++diag_vi_count == 1)
		perfProf_clockStart(); // wall time and PMCs count gameplay, not the load before it
#if defined(WII64_HBC_AGENT) && defined(PERF_PROF)
	if (devAgent_crashVi && diag_vi_count == devAgent_crashVi) devAgent_testCrash();
#endif
	if (diag_vi_count >= diag_stop_vi && diag_stop_vi) {
		perfProf_mark("stop reason: chain guest VI limit");
		stop_it();
	}

	CurrentFPSTime = now_us();

	Dif = CurrentFPSTime - LastFPSTime;
	if (Timers.limitVIs) {
		if (Timers.limitVIs == 2 && Timers.frameDrawn == 0)
			VI_WaitCounter++;
		else
		{
			if (Dif <  (double) VILimitMicroseconds * (VI_WaitCounter + 1) )
			{
				CalculatedTime = CounterTime + (double)VILimitMicroseconds * (double)VI_Counter;
				time = (long)((s64)CalculatedTime - (s64)CurrentFPSTime);
				if (time>0&&time<1000000) {
					unsigned long long sleepTimer = perfProf_subsystemBegin(PERF_SUB_LIMITER);
					usleep(time);
					perfProf_subsystemEnd(PERF_SUB_LIMITER, sleepTimer);
					perfProf_limiterSleep(time);
				}
				CurrentFPSTime = CurrentFPSTime + time;
			}
			Timers.frameDrawn = 0;
			VI_WaitCounter = 0;
		}
	}

//	DWORD diff_millisecs = ticks_to_millisecs(diff_ticks(CounterTime,CurrentFPSTime));
	if (CounterTime > CurrentFPSTime) {
		CounterTime = now_us();
		VI_Counter = 0 ;
	}
	else if (CurrentFPSTime - CounterTime >= 500000.0 ) {
		Timers.vis = (float) (VI_Counter * 1000000.0 / (CurrentFPSTime - CounterTime));
		perfProf_visSample(Timers.vis);
		perfProf_cpuSample();
//		sprintf(txtbuffer,"Timer.VIs: Current = %dus; Last = %dus; diff_ms = %d; FPS_count = %d", CurrentFPSTime, CounterTime, diff_millisecs, VI_Counter);
//		DEBUG_print(txtbuffer,0);
		CounterTime = now_us();
		VI_Counter = 0 ;
	}

	LastFPSTime = CurrentFPSTime ;
    end_section(IDLE_SECTION);
}
