/**
 * glN64_GX - glN64.cpp
 * Copyright (C) 2003 Orkin
 * Copyright (C) 2008, 2009, 2010 sepp256 (Port to Wii/Gamecube/PS3)
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
**/

#include "../main/winlnxdefs.h"
#include <string.h>

#ifdef __GX__
#include "GFXPlugin.h"
#include <gccore.h>
#include <stdio.h>
#include "../gui/DEBUG.h"
#endif

#define MAINDEF 1
//#include <GL/gl.h>	//not available
//#include <GL/glu.h>	//not available
#include "glN64.h"
#include "Debug.h"
#include "Zilmar GFX 1.3.h"
#include "OpenGL.h"
#include "FrameBuffer.h"
#include "N64.h"
#include "RSP.h"
#include "RDP.h"
#include "VI.h"
#include "Config.h"
#include "Textures.h"
#include "Combiner.h"

#ifdef DEBUGON
extern "C" { void _break(); }
#endif

char		pluginName[] = "glN64 v0.4.1 by Orkin - GX port by sepp256";
char		*screenDirectory;

void (*CheckInterrupts)( void );

#ifdef __GX__
void gfx_set_fb(unsigned int* fb1, unsigned int* fb2){
	VI_GX_setFB(fb1, fb2);
}

void gfx_set_window(int x, int y, int width, int height){
	//Note: this will break FB tex for x|y < 0
	OGL.GXorigX = x;
	OGL.GXorigY = y;
	OGL.GXwidth = width;
	OGL.GXheight = height;
}
#endif // __GX__

void
_init( void )
{
	Config_LoadConfig();
}

EXPORT void CALL CaptureScreen ( char * Directory )
{
	screenDirectory = Directory;
	OGL_SaveScreenshot();
}

EXPORT void CALL ChangeWindow (void)
{
}

EXPORT void CALL CloseDLL (void)
{
}

EXPORT void CALL DllAbout ( HWND hParent )
{
}

EXPORT void CALL DllConfig ( HWND hParent )
{
	Config_DoConfig();
}

EXPORT void CALL DllTest ( HWND hParent )
{
}

EXPORT void CALL DrawScreen (void)
{
}

EXPORT void CALL GetDllInfo ( PLUGIN_INFO * PluginInfo )
{
	PluginInfo->Version = 0x103;
	PluginInfo->Type = PLUGIN_TYPE_GFX;
	strcpy( PluginInfo->Name, pluginName );
	PluginInfo->NormalMemory = FALSE;
	PluginInfo->MemoryBswaped = TRUE;
}

EXPORT BOOL CALL InitiateGFX (GFX_INFO Gfx_Info)
{
	Config_LoadConfig();
	DMEM = Gfx_Info.DMEM;
	IMEM = Gfx_Info.IMEM;
	RDRAM = Gfx_Info.RDRAM;
	HEADER = Gfx_Info.HEADER;

	REG.MI_INTR = Gfx_Info.MI_INTR_REG;
	REG.DPC_START = Gfx_Info.DPC_START_REG;
	REG.DPC_END = Gfx_Info.DPC_END_REG;
	REG.DPC_CURRENT = Gfx_Info.DPC_CURRENT_REG;
	REG.DPC_STATUS = Gfx_Info.DPC_STATUS_REG;
	REG.DPC_CLOCK = Gfx_Info.DPC_CLOCK_REG;
	REG.DPC_BUFBUSY = Gfx_Info.DPC_BUFBUSY_REG;
	REG.DPC_PIPEBUSY = Gfx_Info.DPC_PIPEBUSY_REG;
	REG.DPC_TMEM = Gfx_Info.DPC_TMEM_REG;

	REG.VI_STATUS = Gfx_Info.VI_STATUS_REG;
	REG.VI_ORIGIN = Gfx_Info.VI_ORIGIN_REG;
	REG.VI_WIDTH = Gfx_Info.VI_WIDTH_REG;
	REG.VI_INTR = Gfx_Info.VI_INTR_REG;
	REG.VI_V_CURRENT_LINE = Gfx_Info.VI_V_CURRENT_LINE_REG;
	REG.VI_TIMING = Gfx_Info.VI_TIMING_REG;
	REG.VI_V_SYNC = Gfx_Info.VI_V_SYNC_REG;
	REG.VI_H_SYNC = Gfx_Info.VI_H_SYNC_REG;
	REG.VI_LEAP = Gfx_Info.VI_LEAP_REG;
	REG.VI_H_START = Gfx_Info.VI_H_START_REG;
	REG.VI_V_START = Gfx_Info.VI_V_START_REG;
	REG.VI_V_BURST = Gfx_Info.VI_V_BURST_REG;
	REG.VI_X_SCALE = Gfx_Info.VI_X_SCALE_REG;
	REG.VI_Y_SCALE = Gfx_Info.VI_Y_SCALE_REG;

	REG.SP_STATUS = Gfx_Info.SP_STATUS_REG;

	CheckInterrupts = Gfx_Info.CheckInterrupts;

	return TRUE;
}

EXPORT void CALL MoveScreen (int xpos, int ypos)
{
}

EXPORT void CALL ProcessDList(void)
{
	RSP_ProcessDList();
}

EXPORT void CALL ProcessRDPList(void)
{
	//*REG.DPC_CURRENT = *REG.DPC_START;
/*	RSP.PCi = 0;
	RSP.PC[RSP.PCi] = *REG.DPC_CURRENT;
	
	RSP.halt = FALSE;

	while (RSP.PC[RSP.PCi] < *REG.DPC_END)
	{
		RSP.cmd0 = *(DWORD*)&RDRAM[RSP.PC[RSP.PCi]];
		RSP.cmd1 = *(DWORD*)&RDRAM[RSP.PC[RSP.PCi] + 4];
		RSP.PC[RSP.PCi] += 8;
*/
/*		if ((RSP.cmd0 >> 24) == 0xE9)
		{
			*REG.MI_INTR |= MI_INTR_DP;
			CheckInterrupts();
		}
		if ((RSP.cmd0 >> 24) == 0xCD)
			RSP.cmd0 = RSP.cmd0;

		GFXOp[RSP.cmd0 >> 24]();*/
		//*REG.DPC_CURRENT += 8;
//	}
}

EXPORT void CALL RomClosed (void)
{
	OGL_Stop();
	GBI_Destroy();

	GX_SetDrawSyncCallback(NULL);
}

EXPORT void CALL RomOpen (void)
{
	RSP_Init();

	OGL_ResizeWindow();

	GX_SetDrawSyncCallback(VI_GX_DrawSyncCallback);
}

EXPORT void CALL ShowCFB (void)
{	
}

EXPORT void CALL UpdateScreen (void)
{
	VI_UpdateScreen();
}

EXPORT void CALL ViStatusChanged (void)
{
}

EXPORT void CALL ViWidthChanged (void)
{
}


EXPORT void CALL ReadScreen (void **dest, long *width, long *height)
{
	OGL_ReadScreen( dest, width, height );
}

