/**
 * glN64_GX - CRC.cpp
 * Copyright (C) 2003 Orkin
 * Copyright (C) 2008, 2009 sepp256 (Port to Wii/Gamecube/PS3)
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
**/

#include "../main/winlnxdefs.h"
#define XXH_PRIVATE_API
#define XXH_FORCE_MEMORY_ACCESS 2
#define XXH_FORCE_NATIVE_FORMAT 1
#define XXH_FORCE_ALIGN_CHECK 0
#include "../main/xxhash.h"

DWORD Hash_Calculate( DWORD hash, void *buffer, DWORD count )
{
    return XXH32(buffer, count, hash);
}
