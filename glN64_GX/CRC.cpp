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

#ifndef GLN64_FIXED_HASH_LENGTHS
#define GLN64_FIXED_HASH_LENGTHS 1
#endif

DWORD Hash_Calculate( DWORD hash, void *buffer, DWORD count )
{
#if GLN64_FIXED_HASH_LENGTHS
	// Keep the existing seeded hash; constant lengths remove generic loop setup.
	switch (count) {
		case 8: return XXH32(buffer, 8, hash);
		case 16: return XXH32(buffer, 16, hash);
		case 32: return XXH32(buffer, 32, hash);
		case 64: return XXH32(buffer, 64, hash);
	}
#endif
	return XXH32(buffer, count, hash);
}
