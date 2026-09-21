/* boxart.h 
	- Boxart texture loading from a binary compilation
	by emu_kidid
 */

#ifndef BOXART_H
#define BOXART_H

#define BOXART_TEX_WD 			120
//#define BOXART_TEX_FRONT_HT		84
//#define BOXART_TEX_SPINE_HT		24
#define BOXART_TEX_FRONT_HT		88
#define BOXART_TEX_SPINE_HT		16
#define BOXART_TEX_HT 			(2*BOXART_TEX_FRONT_HT+BOXART_TEX_SPINE_HT)
#define BOXART_TEX_BPP			2
#define BOXART_TEX_FRONT_SIZE	(BOXART_TEX_WD*BOXART_TEX_FRONT_HT*BOXART_TEX_BPP)
#define BOXART_TEX_SPINE_SIZE	(BOXART_TEX_WD*BOXART_TEX_SPINE_HT*BOXART_TEX_BPP)
#define BOXART_TEX_SIZE			(BOXART_TEX_WD*BOXART_TEX_HT*BOXART_TEX_BPP)
#define BOXART_TEX_FMT 			GX_TF_RGB565

void BOXART_Init();
void BOXART_DeInit();
bool BOXART_LoadTexture(u32 CRC, char *buffer);

/* System-level diagnostic knob, read from sd:/wii64/diag.cfg (a
   "boxart_limit=N" line), never created automatically. Returns defaultLimit
   unchanged if the file or that line is absent, so nobody sees a behavior
   change unless they deliberately create it. Lets someone cap how many
   visible ROM-browser tiles actually load real boxart -- the rest fall back
   to the placeholder texture without touching boxart.bin -- to bisect how
   much of a page's load time boxart accounts for versus everything else. */
int BOXART_GetLoadLimit(int defaultLimit);

#endif

