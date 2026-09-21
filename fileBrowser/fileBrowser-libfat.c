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
#include "../main/ROM-Cache.h"

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
		sprintf(direntry->name, "%s/%s", file->name, entry->d_name);
		direntry->offset = 0;
		{
			struct stat st;
			direntry->size = (stat(direntry->name, &st) == 0) ? st.st_size : 0;
		}
		direntry->attr   = (entry->d_type == DT_DIR) ?
							FILE_BROWSER_ATTR_DIR : 0;
		
		// If recursive, search all directories
		if(recursive && (direntry->attr == FILE_BROWSER_ATTR_DIR)) {
			if(entry->d_name[0] == '.') continue;	// hide/do not browse these directories.
			//print_gecko("Entering directory: %s\r\n", direntry->name);
			fileBrowser_libfat_readDir(direntry, dir, recursive, n64only);
		}
		else {
			if(*dir == NULL) {
				*dir = malloc( sizeof(fileBrowser_file) );
				num_entries = 0;
			}
			else {
				//print_gecko("Size of *dir = %i\r\n", num_entries);
				*dir = realloc( *dir, ((num_entries)+1) * sizeof(fileBrowser_file) ); 
			}
			if(n64only) {
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

int fileBrowser_libfat_seekFile(fileBrowser_file* file, unsigned int where, unsigned int type){
	if(type == FILE_BROWSER_SEEK_SET) file->offset = where;
	else if(type == FILE_BROWSER_SEEK_CUR) file->offset += where;
	else file->offset = file->size + where;
	
	return 0;
}

int fileBrowser_libfat_readFile(fileBrowser_file* file, void* buffer, unsigned int length){
	FILE* f = fopen( file->name, "rb" );
	if(!f) return FILE_BROWSER_ERROR;
	
	fseek(f, file->offset, SEEK_SET);
	int bytes_read = fread(buffer, 1, length, f);
	if(bytes_read > 0) file->offset += bytes_read;
	
	fclose(f);
	return bytes_read;
}

int fileBrowser_libfat_writeFile(fileBrowser_file* file, void* buffer, unsigned int length){
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
	res = fatMountSimple ("sd", sd2sp2);
#endif
	if(res) {
		mounted[2] = 1;
		return res;
	}
	res = fatMountSimple ("sd", carda);
	if(res) {
		mounted[2] = 1;
		return res;
	}
	res = fatMountSimple ("sd", cardb);
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
int fileBrowser_libfatROM_deinit(fileBrowser_file* f){
	if(romFd)
		fclose(romFd);
	romFd = NULL;
	return 0;
}
int fileBrowser_libfatROM_readFile(fileBrowser_file* file, void* buffer, unsigned int length){
	if(!romFd) {
		romFd = fopen( file->name, "rb");
		if(!romFd) return 0;
		struct stat fileInfo;
		if(!stat(&file->name[0], &fileInfo)){
			file->size = fileInfo.st_size;
		}
		else {
			return 0;
		}
	}
	fseek(romFd, file->offset, SEEK_SET);
	int bytes_read = fread(buffer, 1, length, romFd);
	if(bytes_read > 0) file->offset += bytes_read;

	return bytes_read;
}

