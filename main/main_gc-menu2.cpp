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
#include <fat.h>
#if defined(HW_RVL) && defined(PERF_PROF)
#include <network.h>
#endif
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
#include "../glN64_GX/CacheProbe.h"

#include "winlnxdefs.h"
extern "C" {
#include "main.h"
#include "rom.h"
#include "plugin.h"
#include "perf_prof.h"
#include "perf_memory.h"
#include "dev_agent.h"
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
#include "settings.h"
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

/* settings.ini (main/settings.h). One row per setting: add a setting by adding a row.
   Keys are the 1.6 settings.cfg keys, so an old settings.cfg still reads. Rows marked
   SETTING_GAME can also come from a game's own settings/<game code>.ini. */
#ifdef HW_RVL
#define DEFAULT_COUNT_PER_OP COUNT_PER_OP_2
#else
#define DEFAULT_COUNT_PER_OP COUNT_PER_OP_3
#endif
#ifdef GC_BASIC
#define DEFAULT_MINIMENU MINIMENU_DISABLE
#else
#define DEFAULT_MINIMENU MINIMENU_ENABLE
#endif
#define C SETTING_CHAR
#define I SETTING_INT
#define G SETTING_GAME
static const struct setting SETTINGS[] =
{
  { "General", SETTING_SECTION, 0, 0, 0, 0, 0, "The menu and the emulator." },
  { "MiniMenu", C, 0, &miniMenuActive, MINIMENU_DISABLE, MINIMENU_ENABLE, DEFAULT_MINIMENU,
    "Start Menu: 0 = Advanced (the full menu), 1 = Basic (the mini menu with boxart). A change applies at the next start." },
  { "Core", I, G, &dynacore, DYNACORE_INTERPRETER, DYNACORE_PURE_INTERP, DYNACORE_DYNAREC,
    "The CPU emulator: 0 = interpreter, 1 = dynarec (fast), 2 = pure interpreter (slow, for tests)." },
  { "CountPerOp", I, G, &count_per_op, COUNT_PER_OP_1, COUNT_PER_OP_3, DEFAULT_COUNT_PER_OP,
    "CPU clock divider: 1, 2 or 3 N64 cycles per instruction. More is faster for the Wii but can slow some games." },
  { "LimitVIs", C, G, &Timers.limitVIs, LIMITVIS_NONE, LIMITVIS_WAIT_FOR_FRAME, LIMITVIS_WAIT_FOR_VI,
    "Speed limit: 0 = off (as fast as the Wii can), 1 = N64 speed (wait for each VI), 2 = wait only when a frame was drawn." },

  { "Video", SETTING_SECTION, 0, 0, 0, 0, 0, "The picture." },
  { "VideoMode", C, 0, &videoMode, VIDEOMODE_AUTO, VIDEOMODE_576P, VIDEOMODE_AUTO,
    "TV signal: 0 = auto (Wii settings), 1 = 480i 60 Hz, 2 = 240p, 3 = 480p, 4 = 576i 50 Hz, 5 = 288p, 6 = 576p. A change applies at the next start." },
  { "ScreenMode", C, G, &screenMode, SCREENMODE_4x3, SCREENMODE_16x9_PILLARBOX, SCREENMODE_4x3,
    "Aspect: 0 = 4:3, 1 = 16:9, 2 = force 16:9 in games. The default follows the Wii's 16:9 setting." },
  { "FPS", C, 0, &showFPSonScreen, FPS_HIDE, FPS_SHOW, FPS_SHOW,
    "Show VI/s, speed and DL/s on the screen: 0 = no, 1 = yes." },
  { "NativeOutput", C, G, &nativeOutput, NATIVEOUT_DISABLE, NATIVEOUT_ENABLE, NATIVEOUT_DISABLE,
    "240p output for games that use it: 0 = off, 1 = on." },
  { "FBTex", C, G, &glN64_useFrameBufferTextures, GLN64_FBTEX_DISABLE, GLN64_FBTEX_ENABLE, GLN64_FBTEX_DISABLE,
    "glN64 framebuffer textures (for example the OoT pause screen): 0 = off, 1 = on." },
  { "2xSaI", C, G, &glN64_use2xSaiTextures, GLN64_2XSAI_DISABLE, GLN64_2XSAI_ENABLE, GLN64_2XSAI_DISABLE,
    "glN64 2xSaI texture filter: 0 = off, 1 = on." },
  { "CpuFramebuffer", C, G, &renderCpuFramebuffer, CPUFRAMEBUFFER_DISABLE, CPUFRAMEBUFFER_ENABLE, CPUFRAMEBUFFER_DISABLE,
    "glN64: show the framebuffer the game's CPU draws (some menus and videos): 0 = off, 1 = on." },

  { "Audio", SETTING_SECTION, 0, 0, 0, 0, 0, "The sound. Settings > Audio > Advanced changes the ones after Audio." },
  { "Audio", C, 0, &audioEnabled, AUDIO_DISABLE, AUDIO_ENABLE, AUDIO_ENABLE,
    "Sound: 0 = off, 1 = on." },
  { "AudioQuality", C, G, &audioQuality, AUDIOQUALITY_ACCURATE, AUDIOQUALITY_HIFI, AUDIOQUALITY_ACCURATE,
    "N64 resampling: 0 = accurate, 1 = fast, 2 = hi-fi (more CPU)." },
  { "AudioOutputResampler", C, G, &audioOutputResampler, AUDIOOUTPUT_DSP, AUDIOOUTPUT_HIFI, AUDIOOUTPUT_DSP,
    "Wii output resampling: 0 = Wii DSP, 1 = hi-fi (more CPU)." },
  { "AudioMixerPrecision", C, G, &audioMixerPrecision, AUDIOMIX_ACCURATE, AUDIOMIX_HIFI, AUDIOMIX_ACCURATE,
    "Mixer: 0 = accurate (as the N64), 1 = hi-fi (more CPU)." },
  { "AudioLatency", C, 0, &audioLatency, AUDIOLATENCY_LOW, AUDIOLATENCY_STABLE, AUDIOLATENCY_STABLE,
    "Sound queue: 0 = low latency, 1 = balanced, 2 = stable (fewest gaps)." },
  { "AudioSync", C, G, &audioSync, AUDIOSYNC_NATIVE, AUDIOSYNC_PRESERVE, AUDIOSYNC_NATIVE,
    "When the game runs slow: 0 = native rate, 1 = follow the game speed, 2 = keep the pitch (more CPU)." },

  { "Saves", SETTING_SECTION, 0, 0, 0, 0, 0, "Game saves and save states." },
  { "NativeDevice", C, 0, &nativeSaveDevice, NATIVESAVEDEVICE_SD, NATIVESAVEDEVICE_USB, NATIVESAVEDEVICE_SD,
    "Where game saves go: 0 = SD, 1 = USB." },
  { "StatesDevice", C, 0, &saveStateDevice, SAVESTATEDEVICE_SD, SAVESTATEDEVICE_USB, SAVESTATEDEVICE_SD,
    "Where save states go: 0 = SD, 1 = USB." },
  { "AutoSave", C, 0, &autoSave, AUTOSAVE_DISABLE, AUTOSAVE_ENABLE, AUTOSAVE_ENABLE,
    "Load game saves when a ROM loads and save them when you leave the game: 0 = no, 1 = yes." },

  { "Input", SETTING_SECTION, 0, 0, 0, 0, 0, "Controllers. The button maps are in control*.cfg." },
  { "PadAutoAssign", C, 0, &padAutoAssign, PADAUTOASSIGN_MANUAL, PADAUTOASSIGN_AUTOMATIC, PADAUTOASSIGN_AUTOMATIC,
    "Controllers to N64 ports: 0 = as PadType/PadAssign below, 1 = automatic." },
  { "PadType1", C, 0, &padType[0], PADTYPE_NONE, PADTYPE_WII, PADTYPE_NONE, "Port 1 controller (manual): 0 = none, 1 = GameCube, 2 = Wii." },
  { "PadAssign1", C, 0, &padAssign[0], PADASSIGN_INPUT0, PADASSIGN_INPUT3, PADASSIGN_INPUT0, "Port 1 uses controller 0 to 3 (manual)." },
  { "PadType2", C, 0, &padType[1], PADTYPE_NONE, PADTYPE_WII, PADTYPE_NONE, "Port 2 controller (manual): 0 = none, 1 = GameCube, 2 = Wii." },
  { "PadAssign2", C, 0, &padAssign[1], PADASSIGN_INPUT0, PADASSIGN_INPUT3, PADASSIGN_INPUT1, "Port 2 uses controller 0 to 3 (manual)." },
  { "PadType3", C, 0, &padType[2], PADTYPE_NONE, PADTYPE_WII, PADTYPE_NONE, "Port 3 controller (manual): 0 = none, 1 = GameCube, 2 = Wii." },
  { "PadAssign3", C, 0, &padAssign[2], PADASSIGN_INPUT0, PADASSIGN_INPUT3, PADASSIGN_INPUT2, "Port 3 uses controller 0 to 3 (manual)." },
  { "PadType4", C, 0, &padType[3], PADTYPE_NONE, PADTYPE_WII, PADTYPE_NONE, "Port 4 controller (manual): 0 = none, 1 = GameCube, 2 = Wii." },
  { "PadAssign4", C, 0, &padAssign[3], PADASSIGN_INPUT0, PADASSIGN_INPUT3, PADASSIGN_INPUT3, "Port 4 uses controller 0 to 3 (manual)." },
  { "Pak1", C, G, &pakMode[0], PAKMODE_MEMPAK, PAKMODE_RUMBLEPAK, PAKMODE_MEMPAK, "Port 1 pak: 0 = Controller Pak (saves), 1 = Rumble Pak." },
  { "Pak2", C, G, &pakMode[1], PAKMODE_MEMPAK, PAKMODE_RUMBLEPAK, PAKMODE_MEMPAK, "Port 2 pak: 0 = Controller Pak (saves), 1 = Rumble Pak." },
  { "Pak3", C, G, &pakMode[2], PAKMODE_MEMPAK, PAKMODE_RUMBLEPAK, PAKMODE_MEMPAK, "Port 3 pak: 0 = Controller Pak (saves), 1 = Rumble Pak." },
  { "Pak4", C, G, &pakMode[3], PAKMODE_MEMPAK, PAKMODE_RUMBLEPAK, PAKMODE_MEMPAK, "Port 4 pak: 0 = Controller Pak (saves), 1 = Rumble Pak." },
  { "LoadButtonSlot", C, 0, &loadButtonSlot, LOADBUTTON_SLOT0, LOADBUTTON_DEFAULT, LOADBUTTON_DEFAULT,
    "Button map loaded at start: 0 to 3 = slot 1 to 4, 4 = default." },
};
#undef C
#undef I
#undef G
#define NUM_SETTINGS ((int)(sizeof(SETTINGS) / sizeof(SETTINGS[0])))

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
	snprintf(path, sizeof(path), "%ssettings", prefix);
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
     autonav=loadrom_sd                  Jump straight to Advanced -> Load ROM
                                          -> Load from SD (FileBrowserFrame,
                                          a completely separate directory
                                          browser from the New ROM one above
                                          -- shares fileBrowser_libfat_readDir
                                          but not SelectRomFrame's code path).
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
     save_settings=1                     Write settings.ini at boot, as
                                          Settings > Save Settings does (the
                                          settings.cfg migration, unattended).
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
                                          just a fast scan.
     chain=<vis>[,padsweep=<vi>][,input=<name>] <rom>
                                          One line per game: run each for
                                          <vis> guest VIs, one after another,
                                          in one boot, then return to the loader. On
                                          hardware, moving the SD card is the
                                          slow part, so one boot collects
                                          every game. Each game starts from
                                          VI 0 with native saves neither
                                          loaded nor written (autosave off),
                                          so runs compare across sessions;
                                          other diag.cfg lines apply to every
                                          game. Per game: a game: line and an
                                          "=== chain n/N end ===" line in
                                          perf.log (PERF_PROF builds), the
                                          last displayed frame in
                                          xfb_NN.bin, and padtrace_NN.csv when
                                          the game read a pad. A game that
                                          stops producing VIs is cut off by
                                          a host-retrace watchdog (3x its
                                          length + 60 s) and marked
                                          how=timeout. scripts/chain_table.py
                                          reads it all back. ,padsweep=<vi>
                                          runs the pad sweep (below) in that
                                          game only. ,input=<name> replays
                                          sd:/wii64/input/<name>.txt on port
                                          1 in that game: a pad recording by
                                          guest VI, made from a Dolphin movie
                                          by scripts/dtm2input.py.
     --diag=<line>                       wiiload form of one diag.cfg line. If
                                          present, these lines replace the SD
                                          diag.cfg for this run.
     padsweep=<vi>[,<hold>]              From guest VI <vi> of each game on,
                                          the GameCube driver on port 1 reads
                                          a generated sweep instead of the
                                          pad: each stick axis end to end,
                                          the stick's rim, then every button
                                          alone, <hold> VIs per step
                                          (default 2), repeating. It stands
                                          in for the raw PAD_* reading, so
                                          everything after it runs as for a
                                          real pad -- in Dolphin and on a Wii
                                          alike. padsweep_hold=<n> sets the
                                          step length alone (for a chain's
                                          per-game ,padsweep=).
                                          scripts/padtest.py checks
                                          padtrace_NN.csv. See
                                          doc/controller-testing.md. */
extern "C" void DiagNav_SelectRomSD(void);
extern "C" void DiagNav_LoadFromSD(void);
extern "C" void DiagNav_LoadFromSD_SelectFirst(void);
extern void Func_SR_SD(void);
extern void Func_SR_Select1(void);
extern void Func_ReturnFromSelectRomFrame(void);
extern void Func_SaveGame(void);
extern void Func_LoadSave(void);
extern bool sramWritten, eepromWritten, mempakWritten, flashramWritten;
extern BOOL hasLoadedROM;
extern int randomize_interrupt;
static bool g_diagAutonavSelectRomSD = false;
static int g_diagAutonavLoadFromSD = 0; // 0=off, 1=open Load from SD, 2=also select the first entry (FileBrowserFrame, not SelectRomFrame)
static int g_diagDynacoreOverride = -1; // -1 = not requested; else DYNACORE_* value
static int g_diagAudioQualityOverride = -1; // -1 = use settings.cfg; else AUDIOQUALITY_*
static int g_diagAudioOutput = -1, g_diagAudioMixer = -1, g_diagAudioLatency = -1, g_diagAudioSync = -1;
static int g_diagStressSelectRom = 0; // repeat count for "New ROM -> SD -> back" at boot, 0 = off
static int g_diagTestSaveLoad = 0; // 1 = run the SD/USB save+load round trip at boot, 0 = off
static int g_diagSettingsSubmenu = -1; // -1 = not requested; else SettingsFrame::SUBMENU_* value
static bool g_diagSaveSettings = false;
static int g_diagTestSelectLoad = 0; // 1 = click the first ROM in the SD browser listing at boot, 0 = off
static char g_diagArgs[32][192];
static int g_diagArgN;

/* chain= -- see the doc comment above. */
#define CHAIN_MAX 32
static struct { unsigned int vis, padsweep; char input[32], rom[192]; } g_chain[CHAIN_MAX];
static int g_chainN, g_chainI;
static unsigned int g_padsweepAll; // padsweep= for every game; chain=<vis>,padsweep=<vi> for one
static volatile unsigned int g_chainDeadline;
// Host retraces since the first VIDEO_Init, never reset (libogc's own count restarts at
// every VIDEO_Init): the chain watchdog, and perf.log's vi0_retrace.
extern "C" volatile unsigned int diag_retraces;
volatile unsigned int diag_retraces;
static volatile bool g_chainTimedOut;
#if defined(HW_RVL) && defined(PERF_PROF)
static char g_resultHost[16];
static unsigned int g_resultPort = 39364;
static bool g_fetchRoms;

/* Diagnostic runs may stage their named ROMs from the result receiver. */
static bool fetchRom(const char* path) {
	const char* prefix = "sd:/wii64/roms/";
	if (strncmp(path, prefix, strlen(prefix)) || !g_resultHost[0]) return false;
	const char* name = path + strlen(prefix);
	if (!*name || strchr(name, '/') || strchr(name, '\\') || strstr(name, "..")) return false;
	FILE* existing = fopen(path, "rb");
	if (existing) { fclose(existing); return true; }
	char encoded[3 * 192], request[sizeof(encoded) + 80];
	size_t e = 0;
	for (const unsigned char* p = (const unsigned char*)name; *p; ++p) {
		if (e + 3 >= sizeof(encoded)) return false;
		if ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') ||
			(*p >= '0' && *p <= '9') || *p == '-' || *p == '_' || *p == '.') encoded[e++] = *p;
		else { snprintf(encoded + e, sizeof(encoded) - e, "%%%02X", *p); e += 3; }
	}
	encoded[e] = 0;
	int sock = net_socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) return false;
	struct timeval timeout = { 8, 0 };
	net_setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
	net_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	net_setsockopt(sock, SOL_SOCKET, SO_CONTIMEO, &timeout, sizeof(timeout));
	struct sockaddr_in addr = {};
	addr.sin_len = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(g_resultPort);
	bool ok = inet_aton(g_resultHost, &addr.sin_addr) == 1 &&
		net_connect(sock, (struct sockaddr*)&addr, sizeof(addr)) >= 0;
	int len = snprintf(request, sizeof(request), "GET /rom/%s HTTP/1.0\r\n\r\n", encoded);
	for (int off = 0; ok && off < len; ) {
		int sent = net_write(sock, request + off, len - off);
		if (sent <= 0) ok = false; else off += sent;
	}
	char header[512] = {};
	size_t h = 0;
	while (ok && h + 1 < sizeof(header)) {
		int got = net_read(sock, header + h, 1);
		if (got <= 0) { ok = false; break; }
		h += got;
		if (h >= 4 && !memcmp(header + h - 4, "\r\n\r\n", 4)) break;
	}
	if (h < 4 || memcmp(header + h - 4, "\r\n\r\n", 4)) ok = false;
	char* length = strstr(header, "Content-Length: ");
	unsigned long remaining = length ? strtoul(length + 16, NULL, 10) : 0;
	if (strncmp(header, "HTTP/1.0 200", 12) || !remaining || remaining > 64UL * 1024 * 1024) ok = false;
	char partial[224];
	snprintf(partial, sizeof(partial), "%s.part", path);
	FILE* out = ok ? fopen(partial, "wb") : NULL;
	if (!out) ok = false;
	char buf[8192];
	while (ok && remaining) {
		int got = net_read(sock, buf, remaining < sizeof(buf) ? remaining : sizeof(buf));
		if (got <= 0 || fwrite(buf, 1, got, out) != (size_t)got) { ok = false; break; }
		remaining -= got;
	}
	if (out && fclose(out)) ok = false;
	net_close(sock);
	if (ok && rename(partial, path)) ok = false;
	if (!ok && out) remove(partial);
	return ok;
}

/* Upload only after the measured run. The SD copy remains authoritative if Wi-Fi fails. */
static bool uploadFile(const char* name, bool required) {
	char path[64], header[160], reply[16], buf[4096];
	snprintf(path, sizeof(path), "sd:/wii64/%s", name);
	bool done = !strcmp(name, "done");
	FILE* f = done ? NULL : fopen(path, "rb");
	if (!f && !done) return !required;
	long size = 0;
	if (f) {
		if (fseek(f, 0, SEEK_END) || ftell(f) < 0 || ftell(f) > 2 * 1024 * 1024) { fclose(f); return false; }
		size = ftell(f);
		rewind(f);
	}
	int sock = net_socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) { if (f) fclose(f); return false; }
	struct timeval timeout = { 8, 0 };
	net_setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
	net_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	net_setsockopt(sock, SOL_SOCKET, SO_CONTIMEO, &timeout, sizeof(timeout));
	struct sockaddr_in addr = {};
	addr.sin_len = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(g_resultPort);
	bool ok = inet_aton(g_resultHost, &addr.sin_addr) == 1 &&
		net_connect(sock, (struct sockaddr*)&addr, sizeof(addr)) >= 0;
	int len = snprintf(header, sizeof(header), "POST /%s HTTP/1.0\r\nContent-Length: %ld\r\n\r\n", name, size);
	for (int off = 0; ok && off < len; ) {
		int sent = net_write(sock, header + off, len - off);
		if (sent <= 0) ok = false; else off += sent;
	}
	while (ok && f && !feof(f)) {
		size_t n = fread(buf, 1, sizeof(buf), f);
		if (ferror(f)) { ok = false; break; }
		for (size_t off = 0; ok && off < n; ) {
			int sent = net_write(sock, buf + off, n - off);
			if (sent <= 0) ok = false; else off += sent;
		}
	}
	if (ok) {
		int got = 0;
		while (got < 12) {
			int n = net_read(sock, reply + got, sizeof(reply) - got);
			if (n <= 0) { ok = false; break; }
			got += n;
		}
		if (ok) ok = !memcmp(reply, "HTTP/1.0 200", 12);
	}
	net_close(sock);
	if (f) fclose(f);
	return ok;
}

static void uploadResults_unheld(void);

static void uploadResults(void) {
	devAgent_hold(1); // network start-up and transfers are long, expected waits
	uploadResults_unheld();
	devAgent_hold(0);
}

static void uploadResults_unheld(void) {
	if (!g_resultHost[0]) return;
	if (!devAgent_netReady() || net_init() < 0) { perfProf_mark("hardware upload: network unavailable"); return; }
	bool ok = uploadFile("perf.log", true);
	for (int i = 1; ok && i <= g_chainN; ++i) {
		char name[32];
		snprintf(name, sizeof(name), "xfb_%02d.bin", i);
		ok = uploadFile(name, false);
		snprintf(name, sizeof(name), "padtrace_%02d.csv", i);
		if (ok) ok = uploadFile(name, false);
#ifdef WII64_HPROF
		snprintf(name, sizeof(name), "hprof_%02d.bin", i);
		if (ok) ok = uploadFile(name, true);
#endif
	}
	if (ok) ok = uploadFile("done", false);
	if (!ok) perfProf_mark("hardware upload: failed; results remain on SD");
}
#endif
extern "C" unsigned int diag_vi_count, diag_stop_vi;
extern "C" unsigned int padsweep_vi, padsweep_hold;
extern "C" unsigned int padreplay_load(const char* path);
extern int autobootROM(const char* path);
extern bool autobootQuiet;

static void chainArm(int i) {
	diag_vi_count = 0;
	diag_stop_vi = g_chain[i].vis;
	padsweep_vi = g_chain[i].padsweep ? g_chain[i].padsweep : g_padsweepAll;
	char path[64];
	snprintf(path, sizeof(path), "sd:/wii64/input/%s.txt", g_chain[i].input);
	unsigned int replayRecords = padreplay_load(g_chain[i].input[0] ? path : NULL);
	if(g_chain[i].input[0]) {
		char mark[64];
		snprintf(mark, sizeof(mark), "pad replay records: %u", replayRecords);
		perfProf_mark(mark);
	}
	g_chainTimedOut = false;
	g_chainDeadline = diag_retraces + 3 * g_chain[i].vis + 60 * 60;
}

/* The frame on screen when the game stopped, raw from the XFB (YUYV, fbWidth x xfbHeight
   after a 16-byte "WXFB" w h 0 header): a hardware run can't be screenshotted, and this
   is what says what a row of numbers was measured on. */
static void chainSnapshot(int n) {
	char name[40];
	void* xfb = VIDEO_GetCurrentFramebuffer();
	if (!xfb) return;
	snprintf(name, sizeof(name), "sd:/wii64/xfb_%02d.bin", n);
	FILE* f = fopen(name, "wb");
	if (!f) return;
	u32 hdr[4] = { 0x57584642, vmode->fbWidth, vmode->xfbHeight, 0 };
	fwrite(hdr, sizeof(hdr), 1, f);
	fwrite(xfb, 2, vmode->fbWidth * vmode->xfbHeight, f);
	fclose(f);
}

/* A chained game has come back from go(): file its results and boot the next.
   Network runs return through the loader stub; offline runs keep their old behavior. */
static void diagStressBrowser(void) {
#if !(defined(GC_BASIC))
	for(int stress = 0; stress < g_diagStressSelectRom; stress++) {
		perfProf_mark("stress_selectrom: enter");
		Func_SR_SD();
		menu::Gui::getInstance().draw(); // swapBuffers waits for GX before freeing
		Func_ReturnFromSelectRomFrame();
		menu::Gui::getInstance().draw();
		perfProf_mark("stress_selectrom: back out");
		perfMem_snapshot("browser_closed");
	}
#endif
}

static bool chainNext(void) {
	if (!g_chainN || g_chainI >= g_chainN) return false;
	const char* how = !diag_vi_count ? "load_failed" : g_chainTimedOut ? "timeout" : "vis";
	if (diag_vi_count) chainSnapshot(g_chainI + 1);
	perfProf_gameEnd(g_chainI + 1, g_chainN, diag_vi_count, g_chain[g_chainI].rom, how);
	diagStressBrowser(); // outside gameplay clocks, before loading or unmounting
	if (++g_chainI < g_chainN) {
		chainArm(g_chainI);
		autobootROM(g_chain[g_chainI].rom);
		return true;
	}
	diag_stop_vi = 0;
	#if defined(HW_RVL) && defined(PERF_PROF)
	uploadResults();
	#endif
	// Unmount first so the results survive either exit path.
	fatUnmount("sd");
	fatUnmount("usb");
	#ifdef HW_RVL
	exit(0); // return through the Homebrew Channel reload stub
	#else
	SYS_ResetSystem(SYS_POWEROFF, 0, 0);
	#endif
	return false;
}

static void apply_diag_line(char* line) {
#if defined(WII64_HBC_AGENT) && defined(PERF_PROF)
	if (sscanf(line, "agent_crash_vi=%u", &devAgent_crashVi) == 1) return;
#endif
	line[strcspn(line, "\r\n")] = 0;
#ifdef PERF_PROF
	if (!strncmp(line, "result_tag=", 11)) {
		if (strlen(line + 11) == 32 && strspn(line + 11, "0123456789abcdef") == 32)
			perfProf_mark(line); // Host nonce: reject stale SD results after HBC reload.
		return;
	}
#endif
#if defined(GLN64_GX) && defined(PERF_PROF) && defined(PERF_CACHE_PROBES) && defined(HW_RVL)
	if (!strcmp(line, "cache_probe=1") || !strcmp(line, "cache_probe=0")) {
		cacheProbeGXTest = line[12] == '1';
		return;
	}
#endif
#ifdef WII64_PERF_MEMORY
	if (!strcmp(line, "memory=1") || !strcmp(line, "memory=0")) {
		perfMem_configure(line[7] == '1');
		perfProf_mark(line);
		return;
	}
	if (!strcmp(line, "memory_boxart_probe=1") || !strcmp(line, "memory_boxart_probe=0")) {
		perfMem_boxartTest = line[20] == '1';
		return;
	}
#endif
#ifdef WII64_HPROF
	if (!strcmp(line, "hprof=1") || !strcmp(line, "hprof=0")) {
		hprof_configure(line[6] == '1');
		return;
	}
#endif
	char romPath[192];
	char coreName[32];
	if(!strncmp(line, "dynacore=", 9))
		perfProf_mark("diag received dynacore line");
	if(sscanf(line, "autoboot_rom=%191[^\r\n]", romPath) == 1) {
		#ifdef HW_RVL
			Autoboot::setPath(romPath);
		#endif
		} else if(strncmp(line, "autonav=selectrom_sd", 20) == 0) {
			g_diagAutonavSelectRomSD = true;
		} else if(strncmp(line, "autonav=loadrom_sd_select1", 26) == 0) {
			g_diagAutonavLoadFromSD = 2;
		} else if(strncmp(line, "autonav=loadrom_sd", 18) == 0) {
			g_diagAutonavLoadFromSD = 1;
		} else if(strncmp(line, "autonav=settings_general", 24) == 0) {
			g_diagSettingsSubmenu = 0; // SettingsFrame::SUBMENU_GENERAL
		} else if(strncmp(line, "autonav=settings_video", 22) == 0) {
			g_diagSettingsSubmenu = 1; // SettingsFrame::SUBMENU_VIDEO
		} else if(strncmp(line, "autonav=settings_audio", 22) == 0) {
			g_diagSettingsSubmenu = 3;
		} else if(strncmp(line, "autonav=advanced_audio", 22) == 0) {
			g_diagSettingsSubmenu = 5;
		} else if(strncmp(line, "save_settings=1", 15) == 0) {
			g_diagSaveSettings = true;
		} else if(strncmp(line, "autonav=settings_saves", 22) == 0) {
			g_diagSettingsSubmenu = 4; // SettingsFrame::SUBMENU_SAVES
		} else if(sscanf(line, "dynacore=%31[^\r\n]", coreName) == 1) {
			if(!strcmp(coreName, "dynarec"))         g_diagDynacoreOverride = DYNACORE_DYNAREC;
			else if(!strcmp(coreName, "pureinterp")) g_diagDynacoreOverride = DYNACORE_PURE_INTERP;
			else if(!strcmp(coreName, "interp"))     g_diagDynacoreOverride = DYNACORE_INTERPRETER;
			else                                     g_diagDynacoreOverride = atoi(coreName);
			perfProf_mark(g_diagDynacoreOverride == DYNACORE_PURE_INTERP ?
				"diag requested core: pure interpreter" : g_diagDynacoreOverride == DYNACORE_DYNAREC ?
				"diag requested core: dynarec" : "diag requested core: interpreter");
		} else if(strncmp(line, "audio_quality=", 14) == 0) {
			char quality[16];
			if(sscanf(line + 14, "%15[^\r\n]", quality) == 1) {
				if(!strcmp(quality, "fast")) g_diagAudioQualityOverride = AUDIOQUALITY_FAST;
				else if(!strcmp(quality, "accurate")) g_diagAudioQualityOverride = AUDIOQUALITY_ACCURATE;
				else if(!strcmp(quality, "hifi")) g_diagAudioQualityOverride = AUDIOQUALITY_HIFI;
				else perfProf_mark("diag audio quality: unknown value ignored");
			}
		} else if(!strncmp(line, "audio_output=", 13)) {
			if(!strcmp(line + 13, "dsp")) g_diagAudioOutput = AUDIOOUTPUT_DSP;
			else if(!strcmp(line + 13, "hifi")) g_diagAudioOutput = AUDIOOUTPUT_HIFI;
		} else if(!strncmp(line, "audio_mixer=", 12)) {
			if(!strcmp(line + 12, "accurate")) g_diagAudioMixer = AUDIOMIX_ACCURATE;
			else if(!strcmp(line + 12, "hifi")) g_diagAudioMixer = AUDIOMIX_HIFI;
		} else if(!strncmp(line, "audio_latency=", 14)) {
			if(!strcmp(line + 14, "low")) g_diagAudioLatency = AUDIOLATENCY_LOW;
			else if(!strcmp(line + 14, "balanced")) g_diagAudioLatency = AUDIOLATENCY_BALANCED;
			else if(!strcmp(line + 14, "stable")) g_diagAudioLatency = AUDIOLATENCY_STABLE;
		} else if(!strncmp(line, "audio_sync=", 11)) {
			if(!strcmp(line + 11, "native")) g_diagAudioSync = AUDIOSYNC_NATIVE;
			else if(!strcmp(line + 11, "follow")) g_diagAudioSync = AUDIOSYNC_FOLLOW;
			else if(!strcmp(line + 11, "preserve")) g_diagAudioSync = AUDIOSYNC_PRESERVE;
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
		#if defined(HW_RVL) && defined(PERF_PROF)
		} else if(sscanf(line, "result_host=%15[0-9.]", g_resultHost) == 1) {
			// Numeric IPv4 only; set this to the Mac's LAN address.
		} else if(sscanf(line, "result_port=%u", &g_resultPort) == 1) {
			if (!g_resultPort || g_resultPort > 65535) g_resultPort = 39364;
		} else if(!strcmp(line, "rom_fetch=1")) {
			g_fetchRoms = true;
		#endif
		} else if(strncmp(line, "chain=", 6) == 0 && g_chainN < CHAIN_MAX) {
			// chain=<vis>[,padsweep=<vi>][,input=<name>] <rom path>
			char* p = line + 6;
			g_chain[g_chainN].vis = strtoul(p, &p, 10);
			g_chain[g_chainN].padsweep = 0;
			g_chain[g_chainN].input[0] = 0;
			for(; *p == ','; p += strcspn(p + 1, ", ") + 1)
				if(sscanf(p, ",padsweep=%u", &g_chain[g_chainN].padsweep) != 1)
					sscanf(p, ",input=%31[^, \r\n]", g_chain[g_chainN].input);
			p = strchr(p, ' ');
			if(p && g_chain[g_chainN].vis && sscanf(p + 1, "%191[^\r\n]", g_chain[g_chainN].rom) == 1)
				g_chainN++;
		} else if(sscanf(line, "padsweep_hold=%u", &padsweep_hold) == 1) {
			if(!padsweep_hold) padsweep_hold = 2; // step length for per-game ,padsweep= too
	} else if(sscanf(line, "padsweep=%u,%u", &g_padsweepAll, &padsweep_hold) >= 1) {
		padsweep_vi = g_padsweepAll;
		if(!padsweep_hold) padsweep_hold = 2;
}
}

static void apply_diag_automation(void) {
	if(g_diagArgN) {
		perfProf_mark("diag config: wiiload arguments");
		for(int i = 0; i < g_diagArgN; ++i) {
			if(!strncmp(g_diagArgs[i], "dynacore=", 9))
				perfProf_mark("wiiload dynacore argument received");
			apply_diag_line(g_diagArgs[i]);
		}
	} else {
		FILE* f = fopen("sd:/wii64/diag.cfg", "rb");
		if(!f) return;
		char line[192];
		while(fgets(line, sizeof(line), f))
			apply_diag_line(line);
		fclose(f);
	}
	if(g_chainN) {
		#if defined(HW_RVL) && defined(PERF_PROF)
		if (g_fetchRoms) {
			bool ready = net_init() >= 0;
			for (int i = 0; ready && i < g_chainN; ++i) ready = fetchRom(g_chain[i].rom);
			perfProf_mark(ready ? "ROM fetch: complete" : "ROM fetch: failed");
		}
		#endif
		autobootQuiet = true;
		#ifdef HW_RVL
		Autoboot::setPath(g_chain[0].rom);
		#endif
		chainArm(0);
	}
}

/* The folder of settings.ini, and of settings/<game code>.ini. */
static char settingsDir[16];
int settings_save(const char* path);

/* The settings of the game in settings/<game code>.ini, over the global ones. */
static void loadGameSettings(void){
	char code[5], path[48];
	if(!settingsDir[0] || !rom_game_code(code)) return;
	snprintf(path, sizeof(path), "%ssettings/%s.ini", settingsDir, code);
	FILE* f = fopen(path, "r");
	if(!f) return;
	settings_game_begin(SETTINGS, NUM_SETTINGS, f);
	fclose(f);
}

void load_config(const char *loaded_path) {
	//config stuff
	fileBrowser_file configFile_file;
	char prefix[16];
	int (*configFile_init)(fileBrowser_file*) = fileBrowser_libfat_init;

	if(loaded_path && loaded_path[0] == 'u') {
		memcpy(&configFile_file, &saveDir_libfat_USB, sizeof(fileBrowser_file));
		strcpy(prefix,"usb:/wii64/");
		romFile_topLevel = &topLevel_libfat_USB;
	}
	else if(loaded_path && loaded_path[0] == 's') {
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
		strcpy(settingsDir, prefix);
		// settings.ini; before 1.7.0 the same lines were in settings.cfg
		sprintf(configFile_file.name, "%s%s", prefix, "settings.ini");
		FILE* f = fopen( configFile_file.name, "r" );
		if(!f) {
			sprintf(configFile_file.name, "%s%s", prefix, "settings.cfg");
			f = fopen( configFile_file.name, "r" );
		}
		if(f) {
			settings_read(SETTINGS, NUM_SETTINGS, f);
			fclose(f);
		}
		if(g_diagSaveSettings) {
			sprintf(configFile_file.name, "%s%s", prefix, "settings.ini");
			settings_save(configFile_file.name);
		}
		if(g_diagAudioQualityOverride != -1)
			audioQuality = g_diagAudioQualityOverride;
		if(g_diagAudioOutput != -1) audioOutputResampler = g_diagAudioOutput;
		if(g_diagAudioMixer != -1) audioMixerPrecision = g_diagAudioMixer;
		if(g_diagAudioLatency != -1) audioLatency = g_diagAudioLatency;
		if(g_diagAudioSync != -1) audioSync = g_diagAudioSync;
		perfProf_mark(audioQuality == AUDIOQUALITY_FAST ?
			"audio quality: fast" : audioQuality == AUDIOQUALITY_HIFI ?
			"audio quality: hifi" : "audio quality: accurate");
		char audioModes[128];
		snprintf(audioModes, sizeof(audioModes), "audio modes: n64=%d output=%d mixer=%d latency=%d sync=%d",
			audioQuality, audioOutputResampler, audioMixerPrecision, audioLatency, audioSync);
		perfProf_mark(audioModes);
		if(g_diagDynacoreOverride != -1) { // diag.cfg's dynacore= -- see apply_diag_automation's doc comment
			dynacore = g_diagDynacoreOverride;
			perfProf_mark(dynacore == DYNACORE_PURE_INTERP ?
				"diag applied core: pure interpreter" : dynacore == DYNACORE_DYNAREC ?
				"diag applied core: dynarec" : "diag applied core: interpreter");
		}
		if(g_chainN) // a chain never loads or writes the card's real saves
			autoSave = AUTOSAVE_DISABLE;
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
	if (devAgent_exitRequested()) r4300.stop = 1;
	// Host retraces keep coming when a guest hangs; guest VIs may not.
	if(++diag_retraces > g_chainDeadline && diag_stop_vi && !g_chainTimedOut) {
		g_chainTimedOut = true;
		perfProf_markLater("stop reason: chain host-retrace timeout"); // VI interrupt: no SD here
		stop_it();
	}
	if(!((*(u32*)0xCC003000)>>16)) {
		perfProf_markLater("stop reason: Wii power-status register");
		stop_it();
	}
}

int main(int argc, const char* argv[]) {
	/* INITIALIZE */
#ifdef HW_RVL
	L2Enhance();
	/* Reload to IOS58 for USB */
	if(IOS_GetVersion() != 58)
		IOS_ReloadIOS(58);
	// Fixed regions already own most MEM2; expose only the real remaining arena.
	if ((char*)SYS_GetArena2Hi() < UNCLAIMED_LO) return 1;
	if ((char*)SYS_GetArena2Hi() > MEM2_HI) SYS_SetArena2Hi(MEM2_HI);
	if ((char*)SYS_GetArena2Lo() < UNCLAIMED_LO) SYS_SetArena2Lo(UNCLAIMED_LO);
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

	// Default settings (SETTINGS[] above); the Wii picks the aspect
	settings_defaults(SETTINGS, NUM_SETTINGS);
#ifdef HW_RVL
	screenMode = CONF_GetAspectRatio() == CONF_ASPECT_16_9 ? SCREENMODE_16x9_PILLARBOX : SCREENMODE_4x3;
#endif
	printToScreen    = 1; // Show DEBUG text on screen
	printToSD        = 0; // Disable SD logging
	saveEnabled      = 0; // Don't save game
	creditsScrolling = 0; // Normal menu for now
	menuActive = 1;

#ifdef HW_RVL
	if (argc > 1 && argv && argv[1] && strncmp(argv[1], "--diag=", 7) != 0)
        Autoboot::setPath(argv[1]);
	int i;
	// Some Wii loaders place the first application argument at argv[0]. Only
	// consume diagnostic options here, so checking that slot is safe either way.
	for(i=0; argv && i<argc; ++i){
		if(argv[i] && strncmp(argv[i], "--diag=", 7) == 0 && g_diagArgN < 32) {
			strncpy(g_diagArgs[g_diagArgN], argv[i] + 7, sizeof(g_diagArgs[0]) - 1);
			g_diagArgs[g_diagArgN][sizeof(g_diagArgs[0]) - 1] = 0;
			g_diagArgN++;
		}
	}
	load_config(argc > 0 && argv && argv[0] ? argv[0] : NULL);
	// Apply normal settings overrides after the saved settings have loaded.
	for(i=1; argv && i<argc; ++i)
		if(argv[i] && strncmp(argv[i], "--diag=", 7) != 0)
			settings_line(SETTINGS, NUM_SETTINGS, (char*)argv[i], 0);
#else
	load_config("sd");
#endif
	// The agent chains this callback for frame pacing; do not replace it later.
	devAgent_restoreRetrace();
	devAgent_init();
	perfMem_snapshot("boot");
	MenuContext *menu = new MenuContext(vmode); // runs an autoboot ROM, chain game 1 included
	while (!devAgent_exitRequested() && chainNext()) {}
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
	if(g_diagSettingsSubmenu == 5)
		menu->setActiveFrame(MenuContext::FRAME_ADVANCEDAUDIO);
	else if(g_diagSettingsSubmenu != -1)
		menu->setActiveFrame(MenuContext::FRAME_SETTINGS, g_diagSettingsSubmenu);
	perfProf_mark(g_diagAutonavSelectRomSD ? "diag autonav: flag set" : "diag autonav: flag NOT set");
	if(g_diagAutonavSelectRomSD)
		DiagNav_SelectRomSD();
	perfProf_mark("diag autonav: after DiagNav_SelectRomSD call");
	if(g_diagAutonavLoadFromSD == 1)
		DiagNav_LoadFromSD();
	else if(g_diagAutonavLoadFromSD == 2)
		DiagNav_LoadFromSD_SelectFirst();
	diagStressBrowser();
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
	while (!devAgent_exitRequested() && menu->isRunning()) {
		devAgent_alive(); // a menu frame is progress
		devAgent_menuHome(); // HOME opens the agent overlay over the menu
	}

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

static int loadROM_unheld(fileBrowser_file* rom);

/* A 32 MB ROM takes 30-40 s to read and page in on a Wii: hold the HBC agent's hang
   watchdog (60 s) for the load, the one long wait the menu and chains expect. */
int loadROM(fileBrowser_file* rom){
	devAgent_hold(1);
	int ret = loadROM_unheld(rom);
	devAgent_hold(0);
	return ret;
}

static int loadROM_unheld(fileBrowser_file* rom){
  int ret = 0;
	perfMem_snapshot("load_enter");
	perfProf_mark("loadROM: enter");
	// The global settings again: rom_read's game hacks start from them
	settings_game_end(SETTINGS, NUM_SETTINGS);
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
		perfMem_snapshot("teardown");
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
	loadGameSettings(); // before the plugins and the CPU read the settings

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
	diag_vi_count = 0;
	perfProf_gameBegin();

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

void stop_it() {
	perfProf_markLater("stop_it requested"); // also reached from interrupts
	r4300.stop = 1;
}

#ifdef HW_RVL
void ShutdownWii() {
  perfProf_markLater("ShutdownWii: called"); // power callback (interrupt)
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

/* Settings > Save Settings: settings.ini on SD or USB. */
int settings_save(const char* path){
	FILE* f = fopen(path, "wb");
	if(!f) return 0;
	settings_write(SETTINGS, NUM_SETTINGS, f);
	return fclose(f) == 0;
}
