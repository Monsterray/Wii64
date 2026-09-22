/**
 * glN64_GX - GBI.cpp
 * Copyright (C) 2003 Orkin
 * Copyright (C) 2008, 2009 sepp256 (Port to Wii/Gamecube/PS3)
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
**/

#ifdef __GX__
#include <gccore.h>
#include "../gui/DEBUG.h"
#endif // __GX__

#include <stdio.h>
#include "glN64.h"
#include "GBI.h"
#include "RDP.h"
#include "RSP.h"
#include "F3D.h"
#include "F3DEX.h"
#include "F3DEX2.h"
#include "L3D.h"
#include "L3DEX.h"
#include "L3DEX2.h"
#include "S2DEX.h"
#include "S2DEX2.h"
#include "F3DDKR.h"
#include "F3DPD.h"
#include "F3DCBFD.h"
#include "F3DFLX2.h"
#include "F3DGOLDEN.h"
#include "F3DZEX2.h"
#include "ZSortBOSS.h"
#include "F5Rogue.h"
#include "F3DBETA.h"
#include "F3DSETA.h"
#include "F3DEX095.h"
#include "F3DAM.h"
#include "F3DEX2ACCLAIM.h"
//#include "F5Indi_Naboo.h"
#include "Types.h"
#include "Debug.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <zlib.h>

u32 uc_crc, uc_dcrc;
char uc_str[256];

SpecialMicrocodeInfo specialMicrocodes[] =
{
	{ F3DDKR,		FALSE,	TRUE,	FALSE,	0x3BFF208D, (char*) "Diddy Kong Racing" },
	{ F3DDKR,		FALSE,	TRUE,	FALSE,	0x8F9AE489, (char*) "Diddy Kong Racing" },
	{ F3DJFG,		FALSE,	TRUE,	TRUE,	0x83421788, (char*) "JET FORCE GEMINI" },
	{ F3DPD,		FALSE,	TRUE,	FALSE,	0xC543D0A8, (char*) "Perfect Dark" },
	{ F3DGOLDEN,	FALSE,	TRUE,	FALSE,	0x9CBA9D04, (char*) "RSP SW Version: 2.0G, 09-30-96" }, // GoldenEye 007
	{ F3DCBFD,		TRUE,	TRUE,	FALSE,	0x99E222AC, (char*) "RSP Gfx ucode F3DEXBG.NoN fifo 2.08  Yoshitaka Yasumoto 1999 Nintendo." },
	{ F3DEX2,		TRUE,	TRUE,	FALSE,	0x1DACFAF1, (char*) "ANIMAL FOREST" },
	{ S2DEX2,		FALSE,	TRUE,	FALSE,	0x8E050E8E, (char*) "ANIMAL FOREST" },
	{ ZSortBOSS,	FALSE,	TRUE,	FALSE,	0xe1932671, (char*) "World Driver Championship" },
	{ ZSortBOSS,	FALSE,	TRUE,	FALSE,	0x9a2b06e8, (char*) "Stunt Racer 64" },
	{ ZSortBOSS,	FALSE,	TRUE,	FALSE,	0x39799101, (char*) "World Driver Championship (Euro)" },
	{ F5Rogue,		FALSE,	TRUE,	FALSE,	0x415ef7d5, (char*) "Star Wars Rogue Squadron" },
	{ F3DEX095,		FALSE,	FALSE,	TRUE,	0xcd02d1ad, (char*) "Mario Kart 64, F3DEX 0.95" },
	{ F3D,			FALSE,	FALSE,	FALSE,	0x725db47a, (char*) "AeroFighters Assault" },
	{ F3D,			FALSE,	FALSE,	TRUE,	0xf823e896, (char*) "Wayne Gretzky's 3D Hockey (U)" },
	{ F3D,			FALSE,	TRUE,	TRUE,	0x38477317, (char*) "Pilot Wings 64, Blast Corps" },
	{ F3D,			FALSE,	FALSE,	TRUE,	0x2f9c5063, (char*) "Mischief Makers, MK Trilogy, J.League Live" },
	{ F3DEX,		TRUE,	TRUE,	TRUE,	0x5eef1b83, (char*) "Power League" },
	{ F3D,			FALSE,	FALSE,	TRUE,	0x11d4da6c, (char*) "Super Mario 64" },
	{ F3D,			FALSE,	FALSE,	TRUE,	0xbcebbc25, (char*) "Dark Rift" },
	{ L3D,			FALSE,	TRUE,	TRUE,	0x02668ab6, (char*) "Blast Corps (Line3D)" },
	{ F3D,			FALSE,	FALSE,	FALSE,	0x3a32dce1, (char*) "Pachinko nichi 365" },
	{ F3DBETA,		FALSE,	TRUE,	TRUE,	0xe7fa4491, (char*) "Star Wars: Shadows of the Empire" },
	{ F3DBETA,		FALSE,	TRUE,	TRUE,	0x81fa4a32, (char*) "Wave Race 64 (U)" },
	{ F3D,			FALSE,	FALSE,	TRUE,	0x217b638a, (char*) "Cruis'n USA" },
	{ F3D,			FALSE,	FALSE,	FALSE,	0x3b30eb5d, (char*) "Eikou no Saint Andrews" },
	{ F3D,			FALSE,	TRUE,	FALSE,	0x00000000, (char*) "Fast3D" },
	{ F3DEX095,		FALSE,	FALSE,	TRUE,	0x00c2d08f, (char*) "Mario Kart 64, F3DLX 0.95" },
	{ F3D,			FALSE,	TRUE,	FALSE,	0x00000000, (char*) "Caribbean Nights" },
	{ Turbo3D,		FALSE,	TRUE,	FALSE,	0x42c29cd9, (char*) "Dark Rift (Turbo3D)" },
	{ F3DEX2ACCLAIM,FALSE,	TRUE,	FALSE,	0x8b47d88a, (char*) "Turok 2/3, Armorines, South Park" },
	//{ F5Indi_Naboo,	FALSE,	FALSE,	FALSE,	0x5d7bdce3, (char*) "SW Ep.1 Battle for Naboo" },
	//{ F5Indi_Naboo,	FALSE,	FALSE,	FALSE,	0x37afc470, (char*) "Indiana Jones and the Infernal Machine" },
};

u32 G_RDPHALF_1, G_RDPHALF_2, G_RDPHALF_CONT;
u32 G_PERSPNORM;
u32 G_SPNOOP;
u32 G_SETOTHERMODE_H, G_SETOTHERMODE_L;
u32 G_DL, G_ENDDL, G_CULLDL, G_BRANCH_Z;
u32 G_LOAD_UCODE;
u32 G_MOVEMEM, G_MOVEWORD;
u32 G_MTX, G_POPMTX;
u32 G_GEOMETRYMODE, G_SETGEOMETRYMODE, G_CLEARGEOMETRYMODE;
u32 G_TEXTURE;
u32 G_DMA_IO, G_DMA_DL, G_DMA_TRI, G_DMA_MTX, G_DMA_VTX, G_DMA_OFFSETS;
u32 G_SPECIAL_1, G_SPECIAL_2, G_SPECIAL_3;
u32 G_VTX, G_MODIFYVTX, G_VTXCOLORBASE;
u32 G_TRI1, G_TRI2, G_TRI4;
u32 G_QUAD, G_LINE3D;
u32 G_RESERVED0, G_RESERVED1, G_RESERVED2, G_RESERVED3;
u32 G_SPRITE2D_BASE;
u32 G_BG_1CYC, G_BG_COPY;
u32 G_OBJ_RECTANGLE, G_OBJ_SPRITE, G_OBJ_MOVEMEM;
u32 G_SELECT_DL, G_OBJ_RENDERMODE, G_OBJ_RECTANGLE_R;
u32 G_OBJ_LOADTXTR, G_OBJ_LDTX_SPRITE, G_OBJ_LDTX_RECT, G_OBJ_LDTX_RECT_R;
u32 G_RDPHALF_0;

u32 G_MTX_STACKSIZE;
u32 G_MTX_MODELVIEW;
u32 G_MTX_PROJECTION;
u32 G_MTX_MUL;
u32 G_MTX_LOAD;
u32 G_MTX_NOPUSH;
u32 G_MTX_PUSH;

u32 G_TEXTURE_ENABLE;
u32 G_SHADING_SMOOTH;
u32 G_CULL_FRONT;
u32 G_CULL_BACK;
u32 G_CULL_BOTH;
u32 G_CLIPPING;

u32 G_MV_VIEWPORT;

u32 G_MWO_aLIGHT_1, G_MWO_bLIGHT_1;
u32 G_MWO_aLIGHT_2, G_MWO_bLIGHT_2;
u32 G_MWO_aLIGHT_3, G_MWO_bLIGHT_3;
u32 G_MWO_aLIGHT_4, G_MWO_bLIGHT_4;
u32 G_MWO_aLIGHT_5, G_MWO_bLIGHT_5;
u32 G_MWO_aLIGHT_6, G_MWO_bLIGHT_6;
u32 G_MWO_aLIGHT_7, G_MWO_bLIGHT_7;
u32 G_MWO_aLIGHT_8, G_MWO_bLIGHT_8;

//GBIFunc GBICmd[256];
GBIInfo GBI;

void GBI_Unknown( u32 w0, u32 w1 )
{
}

MicrocodeInfo *GBI_AddMicrocode()
{
	MicrocodeInfo *newtop = (MicrocodeInfo*)malloc( sizeof( MicrocodeInfo ) );

	newtop->lower = GBI.top;
	newtop->higher = NULL;

	if (GBI.top)
		GBI.top->higher = newtop;

	if (!GBI.bottom)
		GBI.bottom = newtop;

    GBI.top = newtop;

	GBI.numMicrocodes++;

	return newtop;
}

void GBI_Init()
{
	GBI.top = NULL;
	GBI.bottom = NULL;
	GBI.current = NULL;
	GBI.numMicrocodes = 0;

	for (u32 i = 0; i <= 0xFF; i++)
		GBI.cmd[i] = GBI_Unknown;
}

void GBI_Destroy()
{
	while (GBI.bottom)
	{
		MicrocodeInfo *newBottom = GBI.bottom->higher;

		if (GBI.bottom == GBI.top)
			GBI.top = NULL;

		free( GBI.bottom );

		GBI.bottom = newBottom;

		if (GBI.bottom)
			GBI.bottom->lower = NULL;

		GBI.numMicrocodes--;
	}
	GBI.top = NULL;
	GBI.bottom = NULL;
	GBI.current = NULL;
	GBI.numMicrocodes = 0;
}

MicrocodeInfo *GBI_DetectMicrocode( u32 uc_start, u32 uc_dstart, u16 uc_dsize )
{
	MicrocodeInfo *current;

	for (unsigned int i = 0; i < GBI.numMicrocodes; i++)
	{
		current = GBI.top;

		while (current)
		{
			if ((current->address == uc_start) && (current->dataAddress == uc_dstart) && (current->dataSize == uc_dsize))
				return current;

			current = current->lower;
		}
	}

	current = GBI_AddMicrocode();

	current->address = uc_start;
	current->dataAddress = uc_dstart;
	current->dataSize = uc_dsize;
	current->NoN = FALSE;
	current->negativeY = TRUE;   // Y inverted is the normal case
	current->fast3DPersp = FALSE;
	current->texturePersp = TRUE;
	current->combineMatrices = FALSE;
	current->type = F3D;

	// See if we can identify it by CRC
	uc_crc = crc32( 0, &RDRAM[uc_start & 0x1FFFFFFF], 4096 );
	current->crc = uc_crc;
#if 0 //def __GX__
	sprintf(txtbuffer,"GBI:uc_crc: %x", uc_crc);
	DEBUG_print(txtbuffer,3); 
#endif // __GX__
	for (u32 i = 0; i < sizeof( specialMicrocodes ) / sizeof( SpecialMicrocodeInfo ); i++)
	{
		if (specialMicrocodes[i].crc != 0 && uc_crc == specialMicrocodes[i].crc)
		{
			current->type = specialMicrocodes[i].type;
			current->NoN = specialMicrocodes[i].NoN;
			current->negativeY = specialMicrocodes[i].negativeY;
			current->fast3DPersp = specialMicrocodes[i].fast3DPersp;
			current->text = specialMicrocodes[i].text;
			return current;
		}
	}

	// See if we can identify it by text
	char uc_data[uc_dsize];
#ifndef _BIG_ENDIAN
	UnswapCopy( &RDRAM[uc_dstart & 0x1FFFFFFF], uc_data, uc_dsize );
#else // !_BIG_ENDIAN
	memcpy( uc_data, &RDRAM[uc_dstart & 0x1FFFFFFF], uc_dsize );
#endif
	strcpy( uc_str, "Not Found" );

	for (u32 i = 0; i < uc_dsize; i++)
	{
		if ((uc_data[i] == 'R') && (uc_data[i+1] == 'S') && (uc_data[i+2] == 'P'))
		{
			u32 j = 0;
			while (uc_data[i+j] > 0x0A)
			{
				uc_str[j] = uc_data[i+j];
				j++;
			}

			uc_str[j] = 0x00;

#if 0 //def __GX__
			sprintf(txtbuffer,"GBI:uc_str(i=%i): %s", i, uc_str);
			DEBUG_print(txtbuffer,4);
#endif // __GX__

			int type = NONE;

			if (strncmp( &uc_str[4], "SW", 2 ) == 0)
			{
				type = F3D;
			}
			else if (strncmp( &uc_str[4], "Gfx", 3 ) == 0)
			{
				current->NoN = (strstr( uc_str + 4, ".NoN" ) != NULL);

				if (strstr( uc_str + 4, ".Rej" ) != NULL)
					current->NoN = TRUE;

				if (strncmp( &uc_str[14], "F3DFLX", 6 ) == 0)
				{
					// F-Zero X's vehicle-rendering microcode
					type = F3DFLX2;
				}
				else if (strncmp( &uc_str[14], "F3DZEX", 6 ) == 0)
				{
					// Zelda: Ocarina of Time / Majora's Mask
					type = F3DZEX2;
				}
				else if (strstr( uc_str, "F3DAM" ) != NULL)
				{
					type = F3DAM;
				}
				else if (strncmp( &uc_str[14], "F3D", 3 ) == 0)
				{
					if (uc_str[28] == '0')
						type = F3DEX;
					else if (uc_str[28] == '1')
						type = F3DEX;
					else if (uc_str[31] == '2')
					{
						type = F3DEX2;
						current->combineMatrices = (uc_str[35] == 'H');
					}

					if (strncmp( &uc_str[14], "F3DLX.Rej", 9 ) == 0)
					{
						current->NoN = TRUE;
					}
					else if (strncmp( &uc_str[14], "F3DLP.Rej", 9 ) == 0)
					{
						current->texturePersp = FALSE;
						current->NoN = TRUE;
					}
				}
				else if (strncmp( &uc_str[14], "L3D", 3 ) == 0)
				{
					if (uc_str[28] == '1')
						type = L3DEX;
					else if (uc_str[31] == '2')
						type = L3DEX2;
				}
				else if (strncmp( &uc_str[14], "S2D", 3 ) == 0)
				{
					if (uc_str[21] == '1')
					{
						type = S2DEX;

						// 1.03 and 1.05 use a different coordinate corrector table from
						// 1.07, which everything else is treated as.
						if (strncmp( &uc_str[21], "1.03", 4 ) == 0)
							S2DEX_SetVersion( S2DEX_VER_1_3 );
						else if (strncmp( &uc_str[21], "1.05", 4 ) == 0)
							S2DEX_SetVersion( S2DEX_VER_1_5 );
						else
							S2DEX_SetVersion( S2DEX_VER_1_7 );
					}
					else if (uc_str[31] == '2')
						type = S2DEX2;
					current->texturePersp = FALSE;
				}
			}

			if (type != NONE)
			{
				current->type = type;
				return current;
			}

			break;
		}
	}

	for (u32 i = 0; i < sizeof( specialMicrocodes ) / sizeof( SpecialMicrocodeInfo ); i++)
	{
		if (strcmp( uc_str, specialMicrocodes[i].text ) == 0)
		{
			current->type = specialMicrocodes[i].type;
			current->NoN = specialMicrocodes[i].NoN;
			current->negativeY = specialMicrocodes[i].negativeY;
			current->fast3DPersp = specialMicrocodes[i].fast3DPersp;
			return current;
		}
	}

	// Let the user choose the microcode
	printf( "glN64: Warning - unknown ucode!!!\n" );
	//TODO: Make sure having ucode = NONE is ok
	return current;
}

void GBI_MakeCurrent( MicrocodeInfo *current )
{
	if (current != GBI.top)
	{
		if (current == GBI.bottom)
		{
			GBI.bottom = current->higher;
			GBI.bottom->lower = NULL;
		}
		else
		{
			current->higher->lower = current->lower;
			current->lower->higher = current->higher;
		}

		current->higher = NULL;
		current->lower = GBI.top;
		GBI.top->higher = current;
		GBI.top = current;
	}

	if (!GBI.current || (GBI.current->type != current->type))
	{
		for (int i = 0; i <= 0xFF; i++)
			GBI.cmd[i] = GBI_Unknown;

		RDP_Init();

		switch (current->type)
		{
			case F3D:		F3D_Init();		break;
			case F3DEX:		F3DEX_Init();	break;
			case F3DEX2:	F3DEX2_Init();	break;
			case L3D:		L3D_Init();		break;
			case L3DEX:		L3DEX_Init();	break;
			case L3DEX2:	L3DEX2_Init();	break;
			case S2DEX:		S2DEX_Init();	break;
			case S2DEX2:	S2DEX2_Init();	break;
			case F3DDKR:	F3DDKR_Init();	break;
			case F3DPD:		F3DPD_Init();	break;
			case F3DCBFD:	F3DCBFD_Init();	break;
			case F3DFLX2:	F3DFLX2_Init();	break;
			case F3DGOLDEN:	F3DGOLDEN_Init();	break;
			case F3DZEX2:	F3DZEX2_Init();	break;
			case ZSortBOSS:	ZSortBOSS_Init();	break;
			case F5Rogue:	F5Rogue_Init();		break;
			case F3DBETA:	F3DBETA_Init();		break;
			case F3DSETA:	F3DSETA_Init();		break;
			case F3DEX095:	F3DEX095_Init();	break;
			case F3DJFG:	F3DJFG_Init();		break;
			case F3DAM:		F3DAM_Init();		break;
			case F3DEX2ACCLAIM:	F3DEX2ACCLAIM_Init();	break;
			//case F5Indi_Naboo:	F5Indi_Naboo_Init();	break;
		}

		//GLideN64 #1303.
		if (current->fast3DPersp)
		{
			GBI_SetGBI( G_PERSPNORM,	F3DBETA_PERSPNORM,	F3DBETA_Perpnorm );
			GBI_SetGBI( G_RDPHALF_1,	F3DBETA_RDPHALF_1,	F3D_RDPHalf_1 );
			GBI_SetGBI( G_RDPHALF_2,	F3DBETA_RDPHALF_2,	F3D_RDPHalf_2 );
		}
	}

	GBI.current = current;
}
