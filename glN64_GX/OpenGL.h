/**
 * glN64_GX - OpenGL.h
 * Copyright (C) 2003 Orkin
 * Copyright (C) 2008, 2009, 2010 sepp256 (Port to Wii/Gamecube/PS3)
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
**/

#ifndef OPENGL_H
#define OPENGL_H

#include "../main/winlnxdefs.h"
#define GL_GLEXT_PROTOTYPES
#define __WIN32__
#include "gl.h"
#include "glext.h"
#undef __WIN32__

#include "gSP.h"

// Depth guard band
#define GXprojZGuard		 4.0
#define GXprojZScale		( 0.5 / GXprojZGuard)  //0.25 //0.5
#define GXprojZOffset		(-0.5 / GXprojZGuard) //-0.5

#define GXviewportNearZ(_n,_f)	((_f) - GXprojZGuard * ((_f) - (_n)))
#define GXpolyOffsetFactor	 5.0e-4 //Tweaked for co-planar polygons. Interestingly, Z resolution should be 5.96e-8.

struct GLVertex
{
	float x, y, z, w;
	struct
	{
		float r, g, b, a;
	} color, secondaryColor;
	float s0, t0, s1, t1;
	float fog;
};

struct GLInfo
{
	DWORD	fullscreenWidth, fullscreenHeight, fullscreenBits, fullscreenRefresh;
	DWORD	width, height, windowedWidth, windowedHeight, heightOffset;

	BOOL	fullscreen, forceBilinear, fog;

	float	scaleX, scaleY;

	BOOL	ARB_multitexture;
	BOOL	EXT_fog_coord;
	BOOL	EXT_secondary_color;

	int		maxTextureUnits;

	BOOL	enable2xSaI;
	BOOL	frameBufferTextures;
	int		textureBitDepth;
	float	originAdjust;

	GLVertex vertices[256];
	BYTE	triangles[80][3];
	BYTE	numTriangles;
	BYTE	numVertices;
	BOOL	usePolygonStipple;
	GLubyte	stipplePattern[32][8][128];
	BYTE	lastStipple;

	BYTE	combiner;

#ifdef __GX__	//Variables specific to GX
	u32		GXorigX, GXorigY;
	u32		GXwidth, GXheight;
	float	GXscaleX, GXscaleY;
	Mtx44	GXcombW;
	Mtx44	GXprojIdent;
	Mtx44	GXprojTemp;
	Mtx		GXmodelViewIdent;
	BOOL	GXuseCombW;
	BOOL	GXcombWok;
	BOOL	GXupdateMtx;
	int		GXnumVtxMP;
	bool	GXuseAlphaCompare;
	float	GXfogStartZ;
	float	GXfogEndZ;
	GXColor	GXfogColor;
	GXColor GXclearColor;
	u8		GXfogType;
	u8*		GXclearBufferTex;
	bool	GXupdateFog;
	bool	GXpolyOffset;
	bool	GXrenderTexRect;
	bool	GXforceClampS0;
	bool	GXforceClampT0;
	bool	GXforceClampS1;
	bool	GXforceClampT1;
	bool	GXuseMinMagNearest;
	bool	GXclearColorBuffer;
	bool	GXclearDepthBuffer;
#endif
};

extern GLInfo OGL;

struct GLcolor
{
	float r, g, b, a;
};

bool OGL_Start();
void OGL_Stop();
void OGL_AddTriangle( SPVertex *vertices, int v0, int v1, int v2 );
void OGL_DrawTriangles();
void OGL_DrawLine( SPVertex *vertices, int v0, int v1, float width );
void OGL_DrawRect( int ulx, int uly, int lrx, int lry, float *color );
void OGL_DrawTexturedRect( float ulx, float uly, float lrx, float lry, float uls, float ult, float lrs, float lrt, bool flip, const float *colorOverride = NULL );
void OGL_UpdateScale();
void OGL_UpdateStates();
void OGL_UpdateCullFace();
void OGL_UpdateViewport();
void OGL_ClearDepthBuffer();
void OGL_ClearColorBuffer( float *color );
void OGL_ResizeWindow();
void OGL_SaveScreenshot();
void OGL_SwapBuffers();
void OGL_ReadScreen( void **dest, long *width, long *height );
#ifdef __GX__
void OGL_GXinitDlist();
void OGL_GXclearEFB();
void OGL_ApplyPendingClears();
#endif // __GX__

#endif
