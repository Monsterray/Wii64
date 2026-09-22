/**
 * Wii64 - main_gc-menu2.cpp (aka MenuV2)
 * Copyright (C) 2007, 2008, 2009, 2010 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009, 2010, 2013 sepp256
 * Copyright (C) 2007, 2008, 2009, 2010 emu_kidid
 * 
 * New main that uses menu's instead of prompts
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
 *                sepp256@gmail.com
 *                emukidid@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
**/


/* INCLUDES */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <malloc.h>
#ifdef DEBUGON
# include <debug.h>
#endif

#include <gccore.h>
#include "../menu/MenuContext.h"
#include "../libgui/MessageBox.h"
//#include "../gui/gui_GX-menu.h"
//#include "../gui/GUI.h"
//#include "../gui/menu.h"
#include "../gui/DEBUG.h"
#include "timers.h"

#include "winlnxdefs.h"
extern "C" {
#include "main.h"
#include "rom.h"
#include "plugin.h"
#include "perf_prof.h"
#include "dynarec_trace.h"
#include "../gc_input/controller.h"
#include <aesndlib.h>
#include "../r4300/interupt.h"
#include "../r4300/r4300.h"
#include "../gc_memory/memory.h"
#include "../gc_memory/TLB-Cache.h"
#include "../gc_memory/tlb.h"
#include "../gc_memory/pif.h"
#include "../gc_memory/flashram.h"
#include "../gc_memory/Saves.h"
#include "../main/savestates.h"
#include "ROM-Cache.h"
#include "../fileBrowser/fileBrowser.h"
#include "../fileBrowser/fileBrowser-libfat.h"
#include "wii64config.h"
#ifndef HW_RVL
#include "../vm/vm.h"
#include "../gc_memory/ARAM.h"
#endif

#ifdef HW_RVL
extern f32 SYS_GetCoreMultiplier();
#endif
}

#ifdef WII
unsigned int MALLOC_MEM2 = 0;
#include <ogc/conf.h>
#include <wiiuse/wpad.h>
#include "../gc_memory/MEM2.h"
#include "Autoboot.h"
#endif



/* NECESSARY FUNCTIONS AND VARIABLES */

// -- Initialization functions --
static void Initialise(void);
static void gfx_info_init(void);
static BOOL audio_info_init(void);
static void rsp_info_init(void);
void control_info_init(void);
// -- End init functions --

// -- Plugin data --
CONTROL Controls[4];

static GFX_INFO     gfx_info;
       AUDIO_INFO   audio_info;
static CONTROL_INFO control_info;
static RSP_INFO     rsp_info;

extern char audioEnabled;
extern char audioQuality;
extern char printToScreen;
extern char showFPSonScreen;
extern char printToSD;
#ifdef GLN64_GX
extern char glN64_useFrameBufferTextures;
extern char glN64_use2xSaiTextures;
#else //GLN64_GX
char glN64_useFrameBufferTextures;
char glN64_use2xSaiTextures;
char renderCpuFramebuffer;
#endif //!GLN64_GX
char nativeOutput;
extern timers Timers;
char menuActive;
char miniMenuActive;
       char saveEnabled;
       char creditsScrolling;
       char padNeedScan;
       char wpadNeedScan;
	   char drcNeedScan;
       char shutdown = 0;
	   char nativeSaveDevice;
	   char saveStateDevice;
       char autoSave;
       char screenMode = 0;
	   char videoMode = 0;
	   char padAutoAssign;
	   char padType[4];
	   char padAssign[4];
	   char pakMode[4];
	   char loadButtonSlot;

static struct {
	const char* key;
	char* value; // Not a string, but a char pointer
	char  min, max;
} OPTIONS[] =
{ { "MiniMenu", &miniMenuActive, MINIMENU_DISABLE, MINIMENU_ENABLE },
  { "Audio", &audioEnabled, AUDIO_DISABLE, AUDIO_ENABLE },
  { "AudioQuality", &audioQuality, AUDIOQUALITY_HIFI, AUDIOQUALITY_FAST },
  { "FPS", &showFPSonScreen, FPS_HIDE, FPS_SHOW },
//  { "Debug", &printToScreen, DEBUG_HIDE, DEBUG_SHOW },
  { "FBTex", &glN64_useFrameBufferTextures, GLN64_FBTEX_DISABLE, GLN64_FBTEX_ENABLE },
  { "NativeOutput", &nativeOutput, NATIVEOUT_DISABLE, NATIVEOUT_ENABLE },
  { "2xSaI", &glN64_use2xSaiTextures, GLN64_2XSAI_DISABLE, GLN64_2XSAI_ENABLE },
  { "ScreenMode", &screenMode, SCREENMODE_4x3, SCREENMODE_16x9_PILLARBOX },
  { "VideoMode", &videoMode, VIDEOMODE_AUTO, VIDEOMODE_576P },
  { "Core", ((char*)&dynacore)+3, DYNACORE_INTERPRETER, DYNACORE_PURE_INTERP },
  { "CountPerOp", ((char*)&count_per_op)+3, COUNT_PER_OP_1, COUNT_PER_OP_3 },
  { "NativeDevice", &nativeSaveDevice, NATIVESAVEDEVICE_SD, NATIVESAVEDEVICE_USB },
  { "StatesDevice", &saveStateDevice, SAVESTATEDEVICE_SD, SAVESTATEDEVICE_USB },
  { "AutoSave", &autoSave, AUTOSAVE_DISABLE, AUTOSAVE_ENABLE },
  { "LimitVIs", &Timers.limitVIs, LIMITVIS_NONE, LIMITVIS_WAIT_FOR_FRAME },
/*  { "PadType1", &padType[0], PADTYPE_NONE, PADTYPE_WII },
  { "PadType2", &padType[1], PADTYPE_NONE, PADTYPE_WII },
  { "PadType3", &padType[2], PADTYPE_NONE, PADTYPE_WII },
  { "PadType4", &padType[3], PADTYPE_NONE, PADTYPE_WII },
  { "PadAssign1", &padAssign[0], PADASSIGN_INPUT0, PADASSIGN_INPUT3 },
  { "PadAssign2", &padAssign[1], PADASSIGN_INPUT0, PADASSIGN_INPUT3 },
  { "PadAssign3", &padAssign[2], PADASSIGN_INPUT0, PADASSIGN_INPUT3 },
  { "PadAssign4", &padAssign[3], PADASSIGN_INPUT0, PADASSIGN_INPUT3 },*/
  { "Pak1", &pakMode[0], PAKMODE_MEMPAK, PAKMODE_RUMBLEPAK },
  { "Pak2", &pakMode[1], PAKMODE_MEMPAK, PAKMODE_RUMBLEPAK },
  { "Pak3", &pakMode[2], PAKMODE_MEMPAK, PAKMODE_RUMBLEPAK },
  { "Pak4", &pakMode[3], PAKMODE_MEMPAK, PAKMODE_RUMBLEPAK },
  { "LoadButtonSlot", &loadButtonSlot, LOADBUTTON_SLOT0, LOADBUTTON_DEFAULT },
};
void handleConfigPair(char* kv);
void readConfig(FILE* f);
void writeConfig(FILE* f);

extern "C" void gfx_set_fb(unsigned int* fb1, unsigned int* fb2);
void gfx_set_window(int x, int y, int width, int height);
// -- End plugin data --

static unsigned int* xfb[2] = { NULL, NULL };	/*** Framebuffers ***/
//static GXRModeObj *vmode;				/*** Graphics Mode Object ***/
GXRModeObj *vmode, *rmode;				/*** Graphics Mode Object ***/
GXRModeObj vmode_phys, rmode_phys;		/*** Graphics Mode Object ***/
int GX_xfb_offset = 0;

// Dummy functions
void (*fBRead)(DWORD addr) = NULL;
void (*fBWrite)(DWORD addr, DWORD size) = NULL;
void (*fBGetFrameBufferInfo)(void *p) = NULL;
// Read PAD format from Classic if available
u16 readWPAD(void);

/* Nothing in Wii64 ever created its own directories, so on a fresh card every
   config and save write failed ("Error saving settings.cfg to SD") and the ROM
   browser reported an error, with no hint that the cause was a missing folder.
   Create the tree once at startup instead. mkdir failing because a directory is
   already there is the normal case, so nothing is reported either way -- the
   callers that use these paths already report their own failures. */
static void ensure_wii64_dirs(const char *prefix) {
	char path[32];
	size_t len = strlen(prefix);		// prefix is "sd:/wii64/" -- drop the trailing '/'
	if(len == 0 || len >= sizeof(path)) return;
	snprintf(path, sizeof(path), "%.*s", (int)(len-1), prefix);
	mkdir(path, 0777);
	snprintf(path, sizeof(path), "%sroms", prefix);
	mkdir(path, 0777);
	snprintf(path, sizeof(path), "%ssaves", prefix);
	mkdir(path, 0777);
}

/* Debugging tool: sd:/wii64/diag.cfg, never created automatically, drives
   Wii64 through states that otherwise need a live Wiimote/GC pad -- built so
   the whole boot-and-reproduce cycle for a bug (build, boot, get to the
   broken screen) can run unattended off a config file, the same way
   fileBrowser.h's diag_cfg is boxart's load-count knob (see boxart.h).
     autoboot_rom=sd:/wii64/roms/Foo.z64  Load and run this ROM immediately at
                                          boot, via the SAME mechanism Wii64
                                          already uses when a loader passes a
                                          path in argv[1] (which Dolphin's -e
                                          direct-.dol boot never does) --
                                          reproduces "pick a ROM and go"
                                          without a menu click.
     autonav=selectrom_sd                Jump straight to the ROM browser (SD)
                                          at boot -- same as clicking New ROM
                                          then SD -- for screens that need to
                                          be reached but not played, such as
                                          the boxart display.
     dynacore=dynarec|pureinterp|interp  Force the CPU core for this run only,
                                          without touching settings.cfg -- lets
                                          autoboot_rom be combined with a core
                                          override for a one-line, unattended
                                          A/B repro (e.g. does this ROM hang
                                          under the dynarec but not the
                                          interpreter?). Applied after
                                          settings.cfg is read, so it wins over
                                          whatever core the user has saved.
     dynarec_trace=1                     Sample the dynarec dispatch loop's
                                          PC straight to the screen (see
                                          main/dynarec_trace.h) -- combine with
                                          autoboot_rom+dynacore=dynarec to see
                                          exactly which block a JIT hang gets
                                          stuck on, even on the very first one.
     randomize_interrupt=0               r4300.c's randomize_interrupt is 1
                                          unconditionally (up to 63 cycles of
                                          jitter added to every PI/SI
                                          interrupt's delay -- see
                                          add_random_interupt_time). Force it
                                          off for a run to test whether that
                                          jitter is what an intermittent hang
                                          is timing-sensitive to.
     stress_selectrom=N                  Repeat "New ROM -> SD -> back out" N
                                          times right after boot, unattended
                                          -- for reproducing a leak/corruption
                                          that only shows up after several
                                          repeats of entering/leaving the ROM
                                          browser (reported: repeated
                                          New ROM + back-out froze Wii64).
     test_saveload=1                     Exercise CurrentRomFrame's real
                                          Load/Save Native Save path (the one
                                          the menu actually wires up, unlike
                                          LoadSaveFrame/SaveGameFrame) for
                                          both SD and USB right after boot --
                                          save, load, save, load. Forces the
                                          dirty flags so Save doesn't no-op.
                                          Requires autoboot_rom= so a ROM is
                                          loaded first.
     autonav=settings_general|settings_video|settings_saves  Jump straight to
                                          that Settings tab at boot -- for
                                          screenshotting a menu layout
                                          without a controller.
     test_selectload=1                   Jump to the ROM browser (SD) and
                                          click the first entry in the
                                          sorted list, like a real user
                                          would -- unlike autoboot_rom=,
                                          which loads a ROM directly without
                                          going through the browser's
                                          fileBrowser_file at all. Verifies
                                          hasLoadedROM via a perfProf_mark
                                          so a change to how the browser
                                          populates its listing (e.g. a
                                          size=0 placeholder -- see
                                          doc/subsystem-review.md's New ROM
                                          menu slowdown writeup) can be
                                          checked against a real load, not
                                          just a fast scan. */
extern "C" void DiagNav_SelectRomSD(void);
extern void Func_SR_SD(void);
extern void Func_SR_Select1(void);
extern void Func_ReturnFromSelectRomFrame(void);
extern void Func_SaveGame(void);
extern void Func_LoadSave(void);
extern bool sramWritten, eepromWritten, mempakWritten, flashramWritten;
extern BOOL hasLoadedROM;
extern int randomize_interrupt;
static bool g_diagAutonavSelectRomSD = false;
static int g_diagDynacoreOverride = -1; // -1 = not requested; else DYNACORE_* value
static int g_diagStressSelectRom = 0; // repeat count for "New ROM -> SD -> back" at boot, 0 = off
static int g_diagTestSaveLoad = 0; // 1 = run the SD/USB save+load round trip at boot, 0 = off
static int g_diagSettingsSubmenu = -1; // -1 = not requested; else SettingsFrame::SUBMENU_* value
static int g_diagTestSelectLoad = 0; // 1 = click the first ROM in the SD browser listing at boot, 0 = off

static void apply_diag_automation(void) {
	FILE* f = fopen("sd:/wii64/diag.cfg", "rb");
	if(!f) return;
	char line[192];
	while(fgets(line, sizeof(line), f)) {
		char romPath[192];
		char coreName[32];
		if(sscanf(line, "autoboot_rom=%191[^\r\n]", romPath) == 1) {
			Autoboot::setPath(romPath);
		} else if(strncmp(line, "autonav=selectrom_sd", 20) == 0) {
			g_diagAutonavSelectRomSD = true;
		} else if(strncmp(line, "autonav=settings_general", 24) == 0) {
			g_diagSettingsSubmenu = 0; // SettingsFrame::SUBMENU_GENERAL
		} else if(strncmp(line, "autonav=settings_video", 22) == 0) {
			g_diagSettingsSubmenu = 1; // SettingsFrame::SUBMENU_VIDEO
		} else if(strncmp(line, "autonav=settings_saves", 22) == 0) {
			g_diagSettingsSubmenu = 4; // SettingsFrame::SUBMENU_SAVES
		} else if(sscanf(line, "dynacore=%31[^\r\n]", coreName) == 1) {
			if(!strcmp(coreName, "dynarec"))         g_diagDynacoreOverride = DYNACORE_DYNAREC;
			else if(!strcmp(coreName, "pureinterp")) g_diagDynacoreOverride = DYNACORE_PURE_INTERP;
			else if(!strcmp(coreName, "interp"))     g_diagDynacoreOverride = DYNACORE_INTERPRETER;
			else                                     g_diagDynacoreOverride = atoi(coreName);
		} else if(strncmp(line, "dynarec_trace=1", 15) == 0) {
			dynarecTrace_setEnabled(1);
		} else if(strncmp(line, "randomize_interrupt=0", 21) == 0) {
			randomize_interrupt = 0;
		} else if(sscanf(line, "stress_selectrom=%d", &g_diagStressSelectRom) == 1) {
			// no-op besides the sscanf; consumed after MenuContext exists, see main()
		} else if(strncmp(line, "test_saveload=1", 15) == 0) {
			g_diagTestSaveLoad = 1;
		} else if(strncmp(line, "test_selectload=1", 17) == 0) {
			g_diagTestSelectLoad = 1;
		}
	}
	fclose(f);
}

void load_config(const char *loaded_path) {
	//config stuff
	fileBrowser_file configFile_file;
	char prefix[16];
	int (*configFile_init)(fileBrowser_file*) = fileBrowser_libfat_init;

	if(loaded_path[0] == 'u') {  
		memcpy(&configFile_file, &saveDir_libfat_USB, sizeof(fileBrowser_file));
		strcpy(prefix,"usb:/wii64/");
		romFile_topLevel = &topLevel_libfat_USB;
	}
	else if(loaded_path[0] == 's') {
		memcpy(&configFile_file, &saveDir_libfat_Default, sizeof(fileBrowser_file));
		strcpy(prefix,"sd:/wii64/");
		romFile_topLevel = &topLevel_libfat_Default;
	}
	else {
		// Loaded over network or USBGecko, or a loader that doesn't set argv properly, try SD then USB.
		memcpy(&configFile_file, &saveDir_libfat_Default, sizeof(fileBrowser_file));
		strcpy(prefix,"sd:/wii64/");
		romFile_topLevel = &topLevel_libfat_Default;
#ifdef HW_RVL
		if(!configFile_init(&configFile_file)) {
			memcpy(&configFile_file, &saveDir_libfat_USB, sizeof(fileBrowser_file));
			strcpy(prefix,"usb:/wii64/");
			romFile_topLevel = &topLevel_libfat_USB;
		}
#endif
	}
	if(configFile_init(&configFile_file)) {                	//only if device initialized ok
		ensure_wii64_dirs(prefix);
		perfProf_reset(); // truncates sd:/wii64/perf.log once per boot; no-op unless built with -DPERF_PROF
		apply_diag_automation(); // sd:/wii64/diag.cfg -- autoboot_rom / autonav, see comment above
		perfProf_selfTest(); // sanity-checks the timer itself; see perf_prof.c
		sprintf(configFile_file.name, "%s%s", prefix, "settings.cfg");
		FILE* f = fopen( configFile_file.name, "r" );  //attempt to open file
		if(f) {        //open ok, read it
			readConfig(f);
			fclose(f);
		}
		if(g_diagDynacoreOverride != -1) // diag.cfg's dynacore= -- see apply_diag_automation's doc comment
			dynacore = g_diagDynacoreOverride;
		sprintf(configFile_file.name, "%s%s", prefix, "controlG.cfg");
		f = fopen( configFile_file.name, "r" );  //attempt to open file
		if(f) {
			load_configurations(f, &controller_GC);					//write out GC controller mappings
			fclose(f);
		}
#ifdef HW_RVL
		sprintf(configFile_file.name, "%s%s", prefix, "controlC.cfg");
		f = fopen( configFile_file.name, "r" );  //attempt to open file
		if(f) {
			load_configurations(f, &controller_Classic);			//write out Classic controller mappings
			fclose(f);
		}
#ifdef RVL_LIBWIIDRC
		sprintf(configFile_file.name, "%s%s", prefix, "controlD.cfg");
		f = fopen( configFile_file.name, "r" );  //attempt to open file
		if(f) {
			load_configurations(f, &controller_DRC);			//write out DRC controller mappings
			fclose(f);
		}
#endif
		sprintf(configFile_file.name, "%s%s", prefix, "controlN.cfg");
		f = fopen( configFile_file.name, "r" );  //attempt to open file
		if(f) {
			load_configurations(f, &controller_WiimoteNunchuk);	//write out WM+NC controller mappings
			fclose(f);
		}
		sprintf(configFile_file.name, "%s%s", prefix, "controlW.cfg");
		f = fopen( configFile_file.name, "r" );  //attempt to open file
		if(f) {
			load_configurations(f, &controller_Wiimote);			//write out Wiimote controller mappings
			fclose(f);
		}
#endif
	}
}

extern "C" void ScanPADSandReset(u32 _) {
	drcNeedScan = padNeedScan = wpadNeedScan = 1;
	if(!((*(u32*)0xCC003000)>>16))
		stop_it();
}

int main(int argc, const char* argv[]) {
	/* INITIALIZE */
#ifdef HW_RVL
	L2Enhance();
	/* Reload to IOS58 for USB */
	if(IOS_GetVersion() != 58)
		IOS_ReloadIOS(58);
#endif

	AESND_Init();
#ifdef HW_DOL
	VM_Init(ARAM_SIZE, MRAM_BACKING);		// Setup Virtual Memory with the entire ARAM
#endif

#ifdef DEBUGON
	//DEBUG_Init(GDBSTUB_DEVICE_TCP,GDBSTUB_DEF_TCPPORT); //Default port is 2828
	DEBUG_Init(GDBSTUB_DEVICE_USB, 1);
	_break();
#endif

	Initialise(); // Stock OGC initialization
#ifndef HW_RVL
	DVD_Init();  
#endif

	// Default Settings
#if !(defined(GC_BASIC))
	miniMenuActive   = MINIMENU_ENABLE; // Activate MiniMenu
#else
	miniMenuActive   = MINIMENU_DISABLE; // Activate MiniMenu
#endif
	audioEnabled     = 1; // Audio
	audioQuality     = AUDIOQUALITY_HIFI; // Audio resample quality
	showFPSonScreen  = 1; // Show FPS on Screen (default on for now, while diagnosing perf/hangs)
	printToScreen    = 1; // Show DEBUG text on screen
	printToSD        = 0; // Disable SD logging
	Timers.limitVIs  = LIMITVIS_WAIT_FOR_VI; // Sync to VI
	saveEnabled      = 0; // Don't save game
	nativeSaveDevice = 0; // SD
	saveStateDevice	 = 0; // SD
	autoSave         = 1; // Auto Save Game
	creditsScrolling = 0; // Normal menu for now
	dynacore         = 1; // Dynarec
#ifndef HW_RVL
	count_per_op	 = COUNT_PER_OP_3;
	screenMode		 = SCREENMODE_4x3;
#else
	count_per_op	 = COUNT_PER_OP_2;
	screenMode		 = CONF_GetAspectRatio() == CONF_ASPECT_16_9 ? SCREENMODE_16x9_PILLARBOX : SCREENMODE_4x3;
#endif
	videoMode		 = VIDEOMODE_AUTO;
	padAutoAssign	 = PADAUTOASSIGN_AUTOMATIC;
	padType[0]		 = PADTYPE_NONE;
	padType[1]		 = PADTYPE_NONE;
	padType[2]		 = PADTYPE_NONE;
	padType[3]		 = PADTYPE_NONE;
	padAssign[0]	 = PADASSIGN_INPUT0;
	padAssign[1]	 = PADASSIGN_INPUT1;
	padAssign[2]	 = PADASSIGN_INPUT2;
	padAssign[3]	 = PADASSIGN_INPUT3;
	pakMode[0]		 = PAKMODE_MEMPAK; // memPak plugged into controller 1
	pakMode[1]		 = PAKMODE_MEMPAK;
	pakMode[2]		 = PAKMODE_MEMPAK;
	pakMode[3]		 = PAKMODE_MEMPAK;
	loadButtonSlot	 = LOADBUTTON_DEFAULT;
#ifdef GLN64_GX
	// glN64 specific  settings
 	glN64_useFrameBufferTextures = 0; // Disable FrameBuffer textures
	glN64_use2xSaiTextures = 0;	// Disable 2xSai textures
	renderCpuFramebuffer = 0; // Disable CPU Framebuffer Rendering
#endif //GLN64_GX
	menuActive = 1;
	nativeOutput	 = NATIVEOUT_DISABLE;

#ifdef HW_RVL
	if (argc > 1)
        Autoboot::setPath(argv[1]);
	load_config(&argv[0][0]);
	// Handle options passed in through arguments
	int i;
	for(i=1; i<argc; ++i){
		handleConfigPair((char*)argv[i]);
	}
#else
	load_config("sd");
#endif
	MenuContext *menu = new MenuContext(vmode);
	VIDEO_SetPostRetraceCallback (ScanPADSandReset);
	//Switch to MiniMenu if active
	if (miniMenuActive)
	{
		menu->setUseMiniMenu(true);
		menu->setActiveFrame(MenuContext::FRAME_MAIN);
	}
	// Must run AFTER the miniMenuActive block above: miniMenuActive defaults
	// to enabled for this build (see "Default Settings" earlier in this
	// function), and that block unconditionally sets the active frame to
	// FRAME_MAIN -- confirmed by adding perfProf_mark calls around this and
	// finding the ROM browser's own boxart-load sequence had genuinely
	// completed in the log, yet the screen never showed it, because this
	// unconditional reset ran right after and clobbered it before a single
	// frame was drawn.
	if(g_diagSettingsSubmenu != -1)
		menu->setActiveFrame(MenuContext::FRAME_SETTINGS, g_diagSettingsSubmenu);
	perfProf_mark(g_diagAutonavSelectRomSD ? "diag autonav: flag set" : "diag autonav: flag NOT set");
	if(g_diagAutonavSelectRomSD)
		DiagNav_SelectRomSD();
	perfProf_mark("diag autonav: after DiagNav_SelectRomSD call");
	for(int stress = 0; stress < g_diagStressSelectRom; stress++) {
		perfProf_mark("stress_selectrom: enter");
		Func_SR_SD();
		menu::Gui::getInstance().draw();
		Func_ReturnFromSelectRomFrame();
		menu::Gui::getInstance().draw();
		perfProf_mark("stress_selectrom: back out");
	}
	if(g_diagTestSelectLoad) {
		perfProf_mark("test_selectload: Func_SR_SD");
		Func_SR_SD();
		menu::Gui::getInstance().draw();
		perfProf_mark("test_selectload: Func_SR_Select1");
		Func_SR_Select1();
		menu::Gui::getInstance().draw();
		perfProf_mark(hasLoadedROM ? "test_selectload: hasLoadedROM=1" : "test_selectload: hasLoadedROM=0");
	}
	if(g_diagTestSaveLoad) {
		char savedDevice = nativeSaveDevice;
		nativeSaveDevice = NATIVESAVEDEVICE_SD;
		sramWritten = eepromWritten = mempakWritten = flashramWritten = true;
		perfProf_mark("test_saveload: SD save");
		Func_SaveGame();
		perfProf_mark("test_saveload: SD load");
		Func_LoadSave();
		nativeSaveDevice = NATIVESAVEDEVICE_USB;
		sramWritten = eepromWritten = mempakWritten = flashramWritten = true;
		perfProf_mark("test_saveload: USB save");
		Func_SaveGame();
		perfProf_mark("test_saveload: USB load");
		Func_LoadSave();
		perfProf_mark("test_saveload: done");
		nativeSaveDevice = savedDevice;
	}
	while (menu->isRunning()) {}

	delete menu;

	return 0;
}

#if defined(WII)
u16 readWPAD(void){
	if(wpadNeedScan){ WPAD_ScanPads(); wpadNeedScan = 0; }
	WPADData* wpad = WPAD_Data(0);

	u16 b = 0;
	if(wpad->err == WPAD_ERR_NONE &&
	   wpad->exp.type == WPAD_EXP_CLASSIC){
	   	u16 w = wpad->exp.classic.btns;
	   	b |= (w & CLASSIC_CTRL_BUTTON_UP)    ? PAD_BUTTON_UP    : 0;
	   	b |= (w & CLASSIC_CTRL_BUTTON_DOWN)  ? PAD_BUTTON_DOWN  : 0;
	   	b |= (w & CLASSIC_CTRL_BUTTON_LEFT)  ? PAD_BUTTON_LEFT  : 0;
	   	b |= (w & CLASSIC_CTRL_BUTTON_RIGHT) ? PAD_BUTTON_RIGHT : 0;
	   	b |= (w & CLASSIC_CTRL_BUTTON_A) ? PAD_BUTTON_A : 0;
	   	b |= (w & CLASSIC_CTRL_BUTTON_B) ? PAD_BUTTON_B : 0;
	}
#ifdef RVL_LIBWIIDRC
	if(drcNeedScan){ WiiDRC_ScanPads(); drcNeedScan = 0; }

	if(WiiDRC_Inited() && WiiDRC_Connected())
	{
		const WiiDRCData* drc = WiiDRC_Data();
	   	u16 w = drc->button;
	   	b |= (w & WIIDRC_BUTTON_UP)    ? PAD_BUTTON_UP    : 0;
	   	b |= (w & WIIDRC_BUTTON_DOWN)  ? PAD_BUTTON_DOWN  : 0;
	   	b |= (w & WIIDRC_BUTTON_LEFT)  ? PAD_BUTTON_LEFT  : 0;
	   	b |= (w & WIIDRC_BUTTON_RIGHT) ? PAD_BUTTON_RIGHT : 0;
		b |= (w & WIIDRC_BUTTON_A) ? PAD_BUTTON_A : 0;
	   	b |= (w & WIIDRC_BUTTON_B) ? PAD_BUTTON_B : 0;
	}
#endif
	return b;
}
#else
u16 readWPAD(void){ return 0; }
#endif

extern bool eepromWritten;
extern bool mempakWritten;
extern bool sramWritten;
extern bool flashramWritten;
BOOL hasLoadedROM = FALSE;
int autoSaveLoaded = NATIVESAVEDEVICE_NONE;

int loadROM(fileBrowser_file* rom){
  int ret = 0;
	perfProf_mark("loadROM: enter");
	// First, if there's already a loaded ROM
	if(hasLoadedROM){
		// Unload it, and deinit everything
		cpu_deinit();
		eepromWritten = FALSE;
		mempakWritten = FALSE;
		sramWritten = FALSE;
		flashramWritten = FALSE;
		romClosed_RSP();
		romClosed_input();
		romClosed_audio();
		romClosed_gfx();
		closeDLL_RSP();
		closeDLL_input();
		closeDLL_audio();
		closeDLL_gfx();
		ROMCache_deinit();
		free_memory();
	}
	format_mempacks();
	reset_flashram();
	init_eeprom();
	hasLoadedROM = TRUE;
#ifdef USE_TLB_CACHE
	TLBCache_reset();
#else
#ifdef HW_RVL
	tlb_mem2_init();
#endif
#endif
	perfProf_mark("loadROM: before rom_read");
	ret = rom_read(rom);
	perfProf_mark("loadROM: after rom_read");
	if(ret){	// Something failed while trying to read the ROM.
		hasLoadedROM = FALSE;
		return ret;
	}

	// Init everything for this ROM
	perfProf_mark("loadROM: before init_memory");
	init_memory();
	perfProf_mark("loadROM: after init_memory");

	gfx_set_fb(xfb[0], xfb[1]);
	if (screenMode == SCREENMODE_16x9_PILLARBOX)
		gfx_set_window( 80, 0, 480, rmode->efbHeight);
	else
		gfx_set_window( 0, 0, 640, rmode->efbHeight);

	gfx_info_init();
	if(!audio_info_init()){
		// AESND_AllocateVoice() failed -- voice stays NULL, and every later
		// RomOpen/AiLenChanged/pauseAudio/resumeAudio/CloseDLL call would
		// pass that NULL straight into AESND_Set*/AESND_FreeVoice. Abort
		// the load instead, same as the rom_read failure above.
		hasLoadedROM = FALSE;
		return -1;
	}
	rsp_info_init();

	perfProf_mark("loadROM: before romOpen_gfx");
	romOpen_gfx();
	perfProf_mark("loadROM: after romOpen_gfx / before romOpen_audio");
	romOpen_audio();
	perfProf_mark("loadROM: after romOpen_audio / before romOpen_input");
	romOpen_input();
	perfProf_mark("loadROM: after romOpen_input / before cpu_init");

	cpu_init();
	perfProf_mark("loadROM: after cpu_init");

  if(autoSave==AUTOSAVE_ENABLE) {
    switch (nativeSaveDevice)
    {
    	case NATIVESAVEDEVICE_SD:
    	case NATIVESAVEDEVICE_USB:
    		// Adjust saveFile pointers
    		saveFile_dir = (nativeSaveDevice==NATIVESAVEDEVICE_SD) ? &saveDir_libfat_Default:&saveDir_libfat_USB;
    		saveFile_readFile  = fileBrowser_libfat_readFile;
    		saveFile_writeFile = fileBrowser_libfat_writeFile;
    		saveFile_init      = fileBrowser_libfat_init;
    		saveFile_deinit    = fileBrowser_libfat_deinit;
    		break;
    }
    // Try loading everything
  	int result = 0;
  	saveFile_init(saveFile_dir);
  	result += loadEeprom(saveFile_dir);
  	result += loadSram(saveFile_dir);
  	result += loadMempak(saveFile_dir);
  	result += loadFlashram(saveFile_dir);

  	switch (nativeSaveDevice)
  	{
  		case NATIVESAVEDEVICE_SD:
  			if (result) autoSaveLoaded = NATIVESAVEDEVICE_SD;
  			break;
  		case NATIVESAVEDEVICE_USB:
  			if (result) autoSaveLoaded = NATIVESAVEDEVICE_USB;
  			break;
  	}
  }
	return 0;
}

static void gfx_info_init(void){
	gfx_info.MemoryBswaped = TRUE;
	gfx_info.HEADER = (BYTE*)&ROM_HEADER;
	gfx_info.RDRAM = (BYTE*)rdram;
	gfx_info.DMEM = (BYTE*)SP_DMEM;
	gfx_info.IMEM = (BYTE*)SP_IMEM;
	gfx_info.MI_INTR_REG = &(MI_register.mi_intr_reg);
	gfx_info.DPC_START_REG = &(dpc_register.dpc_start);
	gfx_info.DPC_END_REG = &(dpc_register.dpc_end);
	gfx_info.DPC_CURRENT_REG = &(dpc_register.dpc_current);
	gfx_info.DPC_STATUS_REG = &(dpc_register.dpc_status);
	gfx_info.DPC_CLOCK_REG = &(dpc_register.dpc_clock);
	gfx_info.DPC_BUFBUSY_REG = &(dpc_register.dpc_bufbusy);
	gfx_info.DPC_PIPEBUSY_REG = &(dpc_register.dpc_pipebusy);
	gfx_info.DPC_TMEM_REG = &(dpc_register.dpc_tmem);
	gfx_info.VI_STATUS_REG = &(vi_register.vi_status);
	gfx_info.VI_ORIGIN_REG = &(vi_register.vi_origin);
	gfx_info.VI_WIDTH_REG = &(vi_register.vi_width);
	gfx_info.VI_INTR_REG = &(vi_register.vi_v_intr);
	gfx_info.VI_V_CURRENT_LINE_REG = &(vi_register.vi_current);
	gfx_info.VI_TIMING_REG = &(vi_register.vi_burst);
	gfx_info.VI_V_SYNC_REG = &(vi_register.vi_v_sync);
	gfx_info.VI_H_SYNC_REG = &(vi_register.vi_h_sync);
	gfx_info.VI_LEAP_REG = &(vi_register.vi_leap);
	gfx_info.VI_H_START_REG = &(vi_register.vi_h_start);
	gfx_info.VI_V_START_REG = &(vi_register.vi_v_start);
	gfx_info.VI_V_BURST_REG = &(vi_register.vi_v_burst);
	gfx_info.VI_X_SCALE_REG = &(vi_register.vi_x_scale);
	gfx_info.VI_Y_SCALE_REG = &(vi_register.vi_y_scale);
	gfx_info.SP_STATUS_REG = &(sp_register.sp_status_reg);
	gfx_info.CheckInterrupts = check_interupt;
	initiateGFX(gfx_info);
}

static BOOL audio_info_init(void){
	audio_info.MemoryBswaped = TRUE;
	audio_info.HEADER = (BYTE*)&ROM_HEADER;
	audio_info.RDRAM = (BYTE*)rdram;
	audio_info.DMEM = (BYTE*)SP_DMEM;
	audio_info.IMEM = (BYTE*)SP_IMEM;
	audio_info.MI_INTR_REG = &(MI_register.mi_intr_reg);
	audio_info.AI_DRAM_ADDR_REG = &(ai_register.ai_dram_addr);
	audio_info.AI_LEN_REG = &(ai_register.ai_len);
	audio_info.AI_CONTROL_REG = &(ai_register.ai_control);
	audio_info.AI_STATUS_REG = &(ai_register.ai_status); // FIXME: This was set to dummy
	audio_info.AI_DACRATE_REG = &(ai_register.ai_dacrate);
	audio_info.AI_BITRATE_REG = &(ai_register.ai_bitrate);
	audio_info.CheckInterrupts = check_interupt;
	return initiateAudio(audio_info);
}

void control_info_init(void){
	control_info.MemoryBswaped = TRUE;
	control_info.HEADER = (BYTE*)&ROM_HEADER;
	control_info.Controls = Controls;
	int i;
	for (i=0; i<4; i++)
	  {
	     Controls[i].Present = FALSE;
	     Controls[i].RawData = FALSE;
	     Controls[i].Plugin = PLUGIN_NONE;
	  }
	initiateControllers(control_info);
}

static void rsp_info_init(void){
	static int cycle_count;
	rsp_info.MemoryBswaped = TRUE;
	rsp_info.RDRAM = (BYTE*)rdram;
	rsp_info.DMEM = (BYTE*)SP_DMEM;
	rsp_info.IMEM = (BYTE*)SP_IMEM;
	rsp_info.MI_INTR_REG = &MI_register.mi_intr_reg;
	rsp_info.SP_MEM_ADDR_REG = &sp_register.sp_mem_addr_reg;
	rsp_info.SP_DRAM_ADDR_REG = &sp_register.sp_dram_addr_reg;
	rsp_info.SP_RD_LEN_REG = &sp_register.sp_rd_len_reg;
	rsp_info.SP_WR_LEN_REG = &sp_register.sp_wr_len_reg;
	rsp_info.SP_STATUS_REG = &sp_register.sp_status_reg;
	rsp_info.SP_DMA_FULL_REG = &sp_register.sp_dma_full_reg;
	rsp_info.SP_DMA_BUSY_REG = &sp_register.sp_dma_busy_reg;
	rsp_info.SP_PC_REG = &rsp_register.rsp_pc;
	rsp_info.SP_SEMAPHORE_REG = &sp_register.sp_semaphore_reg;
	rsp_info.DPC_START_REG = &dpc_register.dpc_start;
	rsp_info.DPC_END_REG = &dpc_register.dpc_end;
	rsp_info.DPC_CURRENT_REG = &dpc_register.dpc_current;
	rsp_info.DPC_STATUS_REG = &dpc_register.dpc_status;
	rsp_info.DPC_CLOCK_REG = &dpc_register.dpc_clock;
	rsp_info.DPC_BUFBUSY_REG = &dpc_register.dpc_bufbusy;
	rsp_info.DPC_PIPEBUSY_REG = &dpc_register.dpc_pipebusy;
	rsp_info.DPC_TMEM_REG = &dpc_register.dpc_tmem;
	rsp_info.CheckInterrupts = check_interupt;
	rsp_info.ProcessDlistList = processDList;
	rsp_info.ProcessAlistList = processAList;
	rsp_info.ProcessRdpList = processRDPList;
	rsp_info.ShowCFB = showCFB;
	initiateRSP(rsp_info,(DWORD*)&cycle_count);
}

void stop_it() { r4300.stop = 1; }

#ifdef HW_RVL
void ShutdownWii() {
  perfProf_mark("ShutdownWii: called");
  shutdown = 1;
  stop_it();
}
#endif

static void Initialise (void){

	//Initialize controls once before menu runs
	control_info_init();

	// Init PS GQRs so I can load signed/unsigned chars/shorts as PS values
	__asm__ volatile(
		"li		3, 0     \n"
		"mtspr	912, 3   \n" // GQR0 = F32
		:: : "r3");
	CAST_SetGQR2(GQR_TYPE_U8, 8);
	CAST_SetGQR3(GQR_TYPE_U16, 16);
	CAST_SetGQR4(GQR_TYPE_U8, 0);
	CAST_SetGQR5(GQR_TYPE_U16, 0);
	CAST_SetGQR6(GQR_TYPE_S8, 0);
	CAST_SetGQR7(GQR_TYPE_S16, 0);
}

void video_mode_init(GXRModeObj *v,unsigned int *fb1, unsigned int *fb2)
{
	vmode = v;
	rmode = v;
	xfb[0] = fb1;
	xfb[1] = fb2;
}

void setOption(char* key, char value){
	for(unsigned int i=0; i<sizeof(OPTIONS)/sizeof(OPTIONS[0]); ++i){
		if(!strcmp(OPTIONS[i].key, key)){
			if(value >= OPTIONS[i].min && value <= OPTIONS[i].max)
				*OPTIONS[i].value = value;
			break;
		}
	}
}

void handleConfigPair(char* kv){
	char* vs = kv;
	while(*vs != ' ' && *vs != '\t' && *vs != ':' && *vs != '=')
			++vs;
	*(vs++) = 0;
	while(*vs == ' ' || *vs == '\t' || *vs == ':' || *vs == '=')
			++vs;

	setOption(kv, atoi(vs));
}

void readConfig(FILE* f){
	char line[256];
	while(fgets(line, 256, f)){
		if(line[0] == '#') continue;
		handleConfigPair(line);
	}
}

void writeConfig(FILE* f){
	for(unsigned int i=0; i<sizeof(OPTIONS)/sizeof(OPTIONS[0]); ++i){
		fprintf(f, "%s = %d\n", OPTIONS[i].key, *OPTIONS[i].value);
	}
}
