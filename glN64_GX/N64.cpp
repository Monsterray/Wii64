/**
 * glN64_GX - N64.cpp
 * Copyright (C) 2003 Orkin
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 *
**/

#ifdef __GX__
#include <gccore.h>
#endif // __GX__

#include "../main/winlnxdefs.h"
#include "N64.h"
#include "Types.h"

u8 *DMEM;
u8 *IMEM;
u64 TMEM[512] ATTRIBUTE_ALIGN(32);
u8 *RDRAM;
u8 *HEADER;
u32 RDRAMSize;

N64Regs REG;
