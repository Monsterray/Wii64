/**
 * glN64_GX - DepthBuffer.cpp
 * Copyright (C) 2003 Orkin
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 *
**/

#ifdef __GX__
#include <gccore.h>
#endif // __GX__

#include <malloc.h>
#include "DepthBuffer.h"
#include "Types.h"

DepthBufferInfo depthBuffer;

void DepthBuffer_Init()
{
	depthBuffer.current = NULL;
	depthBuffer.top = NULL;
	depthBuffer.bottom = NULL;
	depthBuffer.numBuffers = 0;
}

void DepthBuffer_RemoveBottom()
{
	DepthBuffer *newBottom = depthBuffer.bottom->higher;

	if (depthBuffer.bottom == depthBuffer.top)
		depthBuffer.top = NULL;

	free( depthBuffer.bottom );

    depthBuffer.bottom = newBottom;
	
	if (depthBuffer.bottom != NULL)
		depthBuffer.bottom->lower = NULL;

	depthBuffer.numBuffers--;
}

void DepthBuffer_Remove( DepthBuffer *buffer )
{
	if ((buffer == depthBuffer.bottom) &&
		(buffer == depthBuffer.top))
	{
		depthBuffer.top = NULL;
		depthBuffer.bottom = NULL;
	}
	else if (buffer == depthBuffer.bottom)
	{
		depthBuffer.bottom = buffer->higher;

		if (depthBuffer.bottom)
			depthBuffer.bottom->lower = NULL;
	}
	else if (buffer == depthBuffer.top)
	{
		depthBuffer.top = buffer->lower;

		if (depthBuffer.top)
			depthBuffer.top->higher = NULL;
	}
	else
	{
		buffer->higher->lower = buffer->lower;
		buffer->lower->higher = buffer->higher;
	}

	free( buffer );

	depthBuffer.numBuffers--;
}

void DepthBuffer_RemoveBuffer( u32 address )
{
	DepthBuffer *current = depthBuffer.bottom;

	while (current != NULL)
	{
		if (current->address == address)
		{
			DepthBuffer_Remove( current );
			return;
		}
		current = current->higher;
	}
}

// Depth-buffer count is capped like the texture cache's maxBytes is --
// nothing ever calls DepthBuffer_RemoveBuffer(), so without a cap this
// MRU-ordered list (top = most recently used, DepthBuffer_MoveToTop()
// already maintains that order on every cache hit) grows once per distinct
// Z-buffer address a game uses, unbounded, for the whole ROM session.
#define DEPTHBUFFER_MAX_COUNT 16

DepthBuffer *DepthBuffer_AddTop()
{
	DepthBuffer *newtop = (DepthBuffer*)malloc( sizeof( DepthBuffer ) );
	if (!newtop)
		return NULL;

	newtop->lower = depthBuffer.top;
	newtop->higher = NULL;

	if (depthBuffer.top)
		depthBuffer.top->higher = newtop;

	if (!depthBuffer.bottom)
		depthBuffer.bottom = newtop;

    depthBuffer.top = newtop;

	depthBuffer.numBuffers++;

	if (depthBuffer.numBuffers > DEPTHBUFFER_MAX_COUNT && depthBuffer.bottom != newtop)
		DepthBuffer_RemoveBottom();

	return newtop;
}

void DepthBuffer_MoveToTop( DepthBuffer *newtop )
{
	if (newtop == depthBuffer.top)
		return;

	if (newtop == depthBuffer.bottom)
	{
		depthBuffer.bottom = newtop->higher;
		depthBuffer.bottom->lower = NULL;
	}
	else
	{
		newtop->higher->lower = newtop->lower;
		newtop->lower->higher = newtop->higher;
	}

	newtop->higher = NULL;
	newtop->lower = depthBuffer.top;
	depthBuffer.top->higher = newtop;
	depthBuffer.top = newtop;
}

void DepthBuffer_Destroy()
{
	while (depthBuffer.bottom)
		DepthBuffer_RemoveBottom();

	depthBuffer.top = NULL;
}

void DepthBuffer_SetBuffer( u32 address )
{
	DepthBuffer *current = depthBuffer.top;

	// Search through saved depth buffers
	while (current != NULL)
	{
		if (current->address == address)
		{
			DepthBuffer_MoveToTop( current );
			depthBuffer.current = current;
			return;
		}
		current = current->lower;
	}

	current = DepthBuffer_AddTop();
	if (!current)
		return;

	current->address = address;
	current->cleared = TRUE;

	depthBuffer.current = current;
}

DepthBuffer *DepthBuffer_FindBuffer( u32 address )
{
	DepthBuffer *current = depthBuffer.top;

	while (current)
	{
		if (current->address == address)
			return current;
		current = current->lower;
	}

	return NULL;
}
