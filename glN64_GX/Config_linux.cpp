/**
 * glN64_GX - Config_linux.cpp
 * Copyright (C) 2003 Orkin
 * Copyright (C) 2008, 2009 sepp256 (Port to Wii/Gamecube/PS3)
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
**/

#ifndef __GX__
#include <features.h>
#include <dlfcn.h>
#include <unistd.h>
#include "SDL.h"
#include <errno.h>
#include <gtk/gtk.h>
#else // !__GX__
#include <gccore.h>
#endif // __GX__

#include "../main/winlnxdefs.h"
#include <string.h>
#include <stdlib.h>

#include "Config.h"
#include "glN64.h"
#include "RSP.h"
#include "Textures.h"
#include "OpenGL.h"

Config config;

#ifdef __GX__
char glN64_useFrameBufferTextures = 0;
char glN64_use2xSaiTextures = 0;
#endif // __GX__

void Config_LoadConfig()
{
	static int loaded = 0;
#ifndef __GX__
	char line[2000];
	FILE *f;
#endif // !__GX__

	if (loaded)
		return;

	loaded = 1;

#ifndef __GX__
	if (pluginDir == 0)
		pluginDir = GetPluginDir();

	// default configuration
	OGL.fullscreenWidth = 640;
	OGL.fullscreenHeight = 480;
//	OGL.fullscreenBits = 0;
	OGL.windowedWidth = 640;
	OGL.windowedHeight = 480;
//	OGL.windowedBits = 0;
	OGL.forceBilinear = 0;
	OGL.enable2xSaI = 0;
	OGL.fog = 1;
	OGL.textureBitDepth = 1; // normal (16 & 32 bits)
	OGL.frameBufferTextures = 0;
	OGL.usePolygonStipple = 0;
	cache.maxBytes = 32 * 1048576;	
#else //!__GX__
	// GX configuration
	OGL.fullscreenWidth = 640;
	OGL.fullscreenHeight = 480;
	OGL.windowedWidth = 640;
	OGL.windowedHeight = 480;
	OGL.forceBilinear = glN64_use2xSaiTextures;
	OGL.enable2xSaI = glN64_use2xSaiTextures;
	OGL.fog = 1;
	OGL.textureBitDepth = 1; // normal (16 & 32 bits)
	OGL.frameBufferTextures = glN64_useFrameBufferTextures;
	OGL.usePolygonStipple = 0;
	cache.maxBytes = GX_TEXTURE_CACHE_SIZE;
#endif // __GX__

#ifndef __GX__
	// read configuration
	char filename[PATH_MAX];
	snprintf( filename, PATH_MAX, "%s/glN64.conf", pluginDir );
	f = fopen( filename, "r" );
	if (!f)
	{
		fprintf( stderr, "[glN64]: (WW) Couldn't open config file '%s' for reading: %s\n", filename, strerror( errno ) );
		return;
	}

	while (!feof( f ))
	{
		char *val;
		fgets( line, 2000, f );

		val = strchr( line, '=' );
		if (!val)
			continue;
		*val++ = '\0';

/*		if (!strcasecmp( line, "fullscreen width" ))
		{
			OGL.fullscreenWidth = atoi( val );
		}
		else if (!strcasecmp( line, "fullscreen height" ))
		{
			OGL.fullscreenHeight = atoi( val );
		}
		else if (!strcasecmp( line, "fullscreen depth" ))
		{
			OGL.fullscreenBits = atoi( val );
		}
		else if (!strcasecmp( line, "windowed width" ))
		{
			OGL.windowedWidth = atoi( val );
		}
		else if (!strcasecmp( line, "windowed height" ))
		{
			OGL.windowedHeight = atoi( val );
		}
		else if (!strcasecmp( line, "windowed depth" ))
		{
			OGL.windowedBits = atoi( val );
		}*/
		if (!strcasecmp( line, "width" ))
		{
			int w = atoi( val );
			OGL.fullscreenWidth = OGL.windowedWidth = (w == 0) ? (640) : (w);
		}
		else if (!strcasecmp( line, "height" ))
		{
			int h = atoi( val );
			OGL.fullscreenHeight = OGL.windowedHeight = (h == 0) ? (480) : (h);
		}
		else if (!strcasecmp( line, "force bilinear" ))
		{
			OGL.forceBilinear = atoi( val );
		}
		else if (!strcasecmp( line, "enable 2xSAI" ))
		{
			OGL.enable2xSaI = atoi( val );
		}
		else if (!strcasecmp( line, "enable fog" ))
		{
			OGL.fog = atoi( val );
		}
		else if (!strcasecmp( line, "cache size" ))
		{
			cache.maxBytes = atoi( val ) * 1048576;
		}
		else if (!strcasecmp( line, "enable HardwareFB" ))
		{
			OGL.frameBufferTextures = atoi( val );
		}
		else if (!strcasecmp( line, "enable dithered alpha" ))
		{
			OGL.usePolygonStipple = atoi( val );
		}
		else if (!strcasecmp( line, "texture depth" ))
		{
			OGL.textureBitDepth = atoi( val );
		}
		else
		{
			printf( "Unknown config option: %s\n", line );
		}
	}

	fclose( f );
#endif // !__GX__
}

void Config_DoConfig()
{
	Config_LoadConfig();

#ifndef __GX__
	if (!configWindow)
		Config_CreateWindow();

	gtk_widget_show_all( configWindow );
#endif // !__GX__
}
