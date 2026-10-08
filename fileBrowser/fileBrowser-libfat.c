/**
 * Wii64 - fileBrowser-libfat.c
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009, 2013 emu_kidid
 * 
 * fileBrowser for any devices using libfat
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
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


#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <dirent.h>
#include "fileBrowser.h"
#include <sdcard/gcsd.h>
#include "../r4300/r4300.h"
#include "../main/rom.h"
#include "../main/ROM-Cache.h"
#include "../main/perf_subsystem.h"

extern BOOL hasLoadedROM;
extern int stop;

#ifdef HW_RVL
#include <sdcard/wiisd_io.h>
#include <ogc/usbstorage.h>
const DISC_INTERFACE* frontsd = &__io_wiisd;
const DISC_INTERFACE* usb = &__io_usbstorage;
const DISC_INTERFACE* carda = &__io_gcsda;
const DISC_INTERFACE* cardb = &__io_gcsdb;

#else
#include <ogc/dvd.h>
const DISC_INTERFACE* gcloader = &__io_gcode; // GC Loader's SD slot, tried first (as upstream)
const DISC_INTERFACE* carda = &__io_gcsda;
const DISC_INTERFACE* cardb = &__io_gcsdb;
const DISC_INTERFACE* sd2sp2 = &__io_gcsd2;
#endif

#define FRONTSD 1
#define CARD_A  2
#define CARD_B  3

fileBrowser_file topLevel_libfat_Default =
	{ "sd:/wii64/roms", // file name
	  0, // sector
	  0, // offset
	  0, // size
	  FILE_BROWSER_ATTR_DIR
	 };
	 
fileBrowser_file topLevel_libfat_USB =
	{ "usb:/wii64/roms", // file name
	  0, // sector
	  0, // offset
	  0, // size
	  FILE_BROWSER_ATTR_DIR
	 };

fileBrowser_file saveDir_libfat_Default =
	{ "sd:/wii64/saves",
	  0,
	  0,
	  0,
	  FILE_BROWSER_ATTR_DIR
	 };
	 
fileBrowser_file saveDir_libfat_USB =
	{ "usb:/wii64/saves",
	  0,
	  0,
	  0,
	  FILE_BROWSER_ATTR_DIR
	 };

static int num_entries = 0;
static int dir_capacity = 0; // slots currently allocated in *dir, not the same as num_entries -- see below
int fileBrowser_libfat_readDir(fileBrowser_file* file, fileBrowser_file** dir, int recursive, int n64only){
	
	DIR* dp = opendir( file->name );
	if(!dp) return FILE_BROWSER_ERROR;
	fileBrowser_file *direntry = malloc(sizeof(fileBrowser_file));
	
	// Read each entry of the directory
	while(1) {
		struct dirent *entry = readdir(dp);
		if(!entry)
			break;

		// Create a temporary entry for this directory entry.
		memset(direntry, 0, sizeof(fileBrowser_file));
		// snprintf, not sprintf: file->name is the parent's full path, which
		// for a deeply-nested recursive scan can already be close to
		// FILE_BROWSER_MAX_PATH_LEN on its own -- an unchecked sprintf here
		// overflows direntry->name into the rest of the (heap-allocated)
		// struct and whatever the allocator placed after it. Skip entries
		// whose combined path doesn't fit rather than corrupt memory.
		const size_t parentLen = strlen(file->name);
		if(snprintf(direntry->name, FILE_BROWSER_MAX_PATH_LEN,
		            parentLen && file->name[parentLen-1] == '/' ? "%s%s" : "%s/%s", file->name, entry->d_name)
		   >= FILE_BROWSER_MAX_PATH_LEN)
			continue;
		direntry->offset = 0;
		// Deliberately not stat()ing every entry here anymore: stat() is a
		// second, path-based directory lookup per file (readdir() already
		// gave us the name for free), and doing it for every entry in a
		// large ROM folder was the dominant cost of opening the ROM
		// browser -- ~130ms of a ~135ms readDir for 155 entries, i.e.
		// essentially all of it (see doc/subsystem-review.md's New ROM
		// menu slowdown writeup). Safe to leave at 0: the only consumer
		// that needs a real size is loading the one ROM actually picked,
		// and main/rom_gc.c's rom_read() already re-derives it correctly
		// via a "dummy read" through fileBrowser_libfatROM_readFile()
		// below, which does its own single stat() for that one file.
		direntry->size = 0;
		direntry->attr   = (entry->d_type == DT_DIR) ?
							FILE_BROWSER_ATTR_DIR : 0;
		if(entry->d_name[0] == '.') continue;
		
		// If recursive, search all directories
		if(recursive && (direntry->attr == FILE_BROWSER_ATTR_DIR)) {
			//print_gecko("Entering directory: %s\r\n", direntry->name);
			fileBrowser_libfat_readDir(direntry, dir, recursive, n64only);
		}
		else {
			// Doubling growth, not a realloc for every single matched
			// file: a large ROM folder (hundreds of entries) used to mean
			// hundreds of individual reallocs, each one a fresh copy of
			// everything added so far -- O(n^2) work just to build the
			// list, measured as the dominant cost of opening the ROM
			// browser on a big collection (see doc/subsystem-review.md's
			// New ROM menu slowdown writeup).
			if(*dir == NULL) {
				dir_capacity = 64;
				*dir = malloc( dir_capacity * sizeof(fileBrowser_file) );
				num_entries = 0;
			}
			else if(num_entries >= dir_capacity) {
				dir_capacity *= 2;
				*dir = realloc( *dir, dir_capacity * sizeof(fileBrowser_file) );
			}
			if(n64only && direntry->attr != FILE_BROWSER_ATTR_DIR) {
				// Byte order comes from the ROM header's magic word (see
				// init_byte_swap() in rom_gc.c), not the extension -- .rom is
				// a plain-bare-extension convention some dumps/sites use for
				// an otherwise ordinary .z64/.v64/.n64 image, so it belongs
				// in this whitelist alongside them.
				const char *ext = strrchr(direntry->name, '.');
				if(!ext || (strcasecmp(ext, ".v64") && strcasecmp(ext, ".z64") &&
					 strcasecmp(ext, ".n64") && strcasecmp(ext, ".bin") &&
					 strcasecmp(ext, ".rom")))
					continue;
				// .bin is also used for other data (boxart.bin, diagnostics):
				// list one only if it starts like an N64 ROM.
				if(!strcasecmp(ext, ".bin")) {
					unsigned char magic[4] = {0};
					FILE* f = fopen(direntry->name, "rb");
					if(f) { if(fread(magic, 1, 4, f) != 4) magic[0] = 0; fclose(f); }
					if(init_byte_swap((magic[0] << 24) | (magic[1] << 16) | (magic[2] << 8) | magic[3]) == BYTE_SWAP_BAD)
						continue;
				}
			}
			
			memcpy(&(*dir)[num_entries], direntry, sizeof(fileBrowser_file));
			//print_gecko("Adding file: %s\r\n", (*dir)[num_entries].name);
			num_entries++;
		}
	}
	if(direntry)
		free(direntry);
	
	closedir(dp);

	return num_entries;
}

/* ROM browsing, one folder at a time (menu/SelectRomFrame.cpp and
   menu/FileBrowserFrame.cpp): B goes up a folder, and leaves at the root. */
int fileBrowser_libfat_isRoot(const char* path){
	const char* colon = strchr(path, ':');
	return !colon || !colon[1] || (colon[1] == '/' && !colon[2]);
}

/* "sd:/ROMS/N64" -> "sd:/ROMS" -> "sd:/". With dropUpEntry, a trailing
   "/.." (the browser's up entry) is dropped first, so "sd:/ROMS/N64/.." ->
   "sd:/ROMS" and "sd:/ROMS/.." -> "sd:/". */
void fileBrowser_libfat_parent(char* path, int dropUpEntry){
	size_t len = strlen(path);
	if(dropUpEntry) {
		if(len < 3 || strcmp(path + len - 3, "/..")) return;
		path[len - 3] = 0;
	}
	char* colon = strchr(path, ':');
	char* slash = strrchr(path, '/');
	if(!colon || !slash || slash < colon) return;
	if(slash == colon + 1) slash[1] = 0;
	else *slash = 0;
}

/* The folder of the last ROM loaded, kept across boots. Not in settings.cfg:
   writing that at every ROM load would also save settings the user did not
   choose to save. */
static const char* const romDirFiles[2] = { "sd:/wii64/romdir.txt", "usb:/wii64/romdir.txt" };

void fileBrowser_libfat_rememberRomDir(const char* romPath){
	char dir[FILE_BROWSER_MAX_PATH_LEN], old[FILE_BROWSER_MAX_PATH_LEN] = "";
	snprintf(dir, sizeof(dir), "%s", romPath);
	fileBrowser_libfat_parent(dir, 0);
	for(int i = 0; i < 2; i++) {
		FILE* f = fopen(romDirFiles[i], "r");
		if(f) { if(!fgets(old, sizeof(old), f)) old[0] = 0; fclose(f); }
		if(!strcmp(old, dir)) return;
		f = fopen(romDirFiles[i], "w");
		if(f) { fputs(dir, f); fclose(f); return; }
	}
}

/* The remembered folder, if it is on the same device as top and still there. */
int fileBrowser_libfat_lastRomDir(const fileBrowser_file* top, fileBrowser_file* out){
	const char* colon = strchr(top->name, ':');
	if(!colon) return 0;
	for(int i = 0; i < 2; i++) {
		char dir[FILE_BROWSER_MAX_PATH_LEN];
		FILE* f = fopen(romDirFiles[i], "r");
		if(!f) continue;
		int ok = fgets(dir, sizeof(dir), f) != NULL;
		fclose(f);
		dir[strcspn(dir, "\r\n")] = 0;
		if(!ok || strncmp(dir, top->name, colon - top->name + 1)) continue;
		DIR* dp = opendir(dir);
		if(!dp) continue;
		closedir(dp);
		memcpy(out, top, sizeof(*out));
		snprintf(out->name, sizeof(out->name), "%s", dir);
		return 1;
	}
	return 0;
}

int fileBrowser_libfat_seekFile(fileBrowser_file* file, unsigned int where, unsigned int type){
	if(type == FILE_BROWSER_SEEK_SET) file->offset = where;
	else if(type == FILE_BROWSER_SEEK_CUR) file->offset += where;
	else file->offset = file->size + where;
	
	return 0;
}

int fileBrowser_libfat_readFile(fileBrowser_file* file, void* buffer, unsigned int length){
	PERF_SUBSYSTEM_C_SCOPE(PERF_SUB_STORAGE_READ);
	FILE* f = fopen( file->name, "rb" );
	if(!f) return FILE_BROWSER_ERROR;
	
	fseek(f, file->offset, SEEK_SET);
	int bytes_read = fread(buffer, 1, length, f);
	if(bytes_read > 0) file->offset += bytes_read;
	
	fclose(f);
	return bytes_read;
}

int fileBrowser_libfat_writeFile(fileBrowser_file* file, void* buffer, unsigned int length){
	PERF_SUBSYSTEM_C_SCOPE(PERF_SUB_STORAGE_WRITE);
	FILE* f = fopen( file->name, "wb" );
	if(!f) return FILE_BROWSER_ERROR;
	
	fseek(f, file->offset, SEEK_SET);
	int bytes_read = fwrite(buffer, 1, length, f);
	if(bytes_read > 0) file->offset += bytes_read;
	
	fclose(f);
	return bytes_read;
}

/* call fileBrowser_libfat_init as much as you like for all devices
    - returns 0 on device not present/error
    - returns 1 on ok
*/
static int mounted[3]; // [0]=Wii SD, [1]=Wii USB, [2]=GC (sd2sp2/carda/cardb all collapse to this slot)

int fileBrowser_libfat_init(fileBrowser_file* f){

	int res = 0;
#ifdef HW_RVL
	if(f->name[0] == 's') {     //SD
		if(mounted[0]) return 1;	// already.
		if(fatMountSimple ("sd", frontsd)) {
			mounted[0] = 1;
			return 1;
		}
	}
	else {
		if(mounted[1]) return 1;	// already.
		if(fatMountSimple ("usb", usb)) {
			mounted[1] = 1;
			return 1;
		}
	}
#else
	// GC has only SD
	if(mounted[2]) return 1;
	res = fatMountSimple ("sd", gcloader);
	if(res) {
		mounted[2] = 1;
		return res;
	}
	res = fatMountSimple ("sd", get_io_gcsd2());
	if(res) {
		mounted[2] = 1;
		return res;
	}
#endif
	res = fatMountSimple ("sd", get_io_gcsda());
	if(res) {
		mounted[2] = 1;
		return res;
	}
	res = fatMountSimple ("sd", get_io_gcsdb());
	if(res) {
		mounted[2] = 1;
		return res;
	}
	return res;
}

int fileBrowser_libfat_deleteFile(fileBrowser_file* file){
	return (remove(file->name) == -1) ? 0 : 1;
}

int fileBrowser_libfat_deinit(fileBrowser_file* f){
	if(f->name[0] == 's') {      //SD
		//fatUnmount("sd");
 	}
	return 0;
}

/* Special for ROM loading only. romFd is separate from the save-file path
   above (which opens/closes its own local FILE* per call) because
   main/ROM-Cache.c deliberately leaves the ROM handle open across reads --
   sharing one static FILE* with fileBrowser_libfat_deinit() used to mean any
   save/load while a ROM was streaming would fclose() the ROM's handle out
   from under it. */
static FILE* romFd;
// Path romFd is currently open for. `if(!romFd)` alone used to gate the
// open+stat below -- if a previous load left romFd open without going
// through fileBrowser_libfatROM_deinit() (e.g. a failure path elsewhere
// that doesn't call it), the NEXT load would skip both the open (silently
// reading the WRONG file's data through the stale fd) and the stat
// (leaving file->size at whatever readDir() set it to -- 0, since that no
// longer stats each entry). Comparing the path makes this self-correcting
// regardless of whether every caller remembered to deinit.
static char romFdPath[FILE_BROWSER_MAX_PATH_LEN];
int fileBrowser_libfatROM_deinit(fileBrowser_file* f){
	if(romFd)
		fclose(romFd);
	romFd = NULL;
	romFdPath[0] = 0;
	return 0;
}
int fileBrowser_libfatROM_readFile(fileBrowser_file* file, void* buffer, unsigned int length){
	if(!romFd || strncmp(romFdPath, file->name, FILE_BROWSER_MAX_PATH_LEN) != 0) {
		if(romFd) fclose(romFd);
		romFd = fopen( file->name, "rb");
		if(!romFd) { romFdPath[0] = 0; return 0; }
		strncpy(romFdPath, file->name, FILE_BROWSER_MAX_PATH_LEN-1);
		romFdPath[FILE_BROWSER_MAX_PATH_LEN-1] = 0;
	}
	// Separate from the open/reopen above: file->size lives in the
	// *caller's* fileBrowser_file, freshly copied from a directory-listing
	// entry that readDir() no longer stats (always 0 there, see the
	// comment on romFdPath above) -- so it needs to be (re-)populated every
	// time it isn't already known, not just when romFd itself needed to be
	// reopened. Re-selecting the same ROM without going through deinit()
	// would otherwise keep the stat skipped and file->size stuck at 0.
	if(!file->size) {
		struct stat fileInfo;
		if(!stat(&file->name[0], &fileInfo)){
			file->size = fileInfo.st_size;
		}
		else {
			fclose(romFd);
			romFd = NULL;
			romFdPath[0] = 0;
			return 0;
		}
	}
	fseek(romFd, file->offset, SEEK_SET);
	int bytes_read = fread(buffer, 1, length, romFd);
	if(bytes_read > 0) file->offset += bytes_read;

	return bytes_read;
}
