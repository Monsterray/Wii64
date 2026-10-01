/**
 * Wii64 - audio.c
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009 emu_kidid
 * 
 * Low-level Audio plugin with linear interpolation & 
 * resampling to 32/48KHz for the GC/Wii
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

/*
 * Copyright (c) 2013 Extrems <metaradil@gmail.com>
 *
 * This file is part of Not64.
 *
 * Not64 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Not64 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Not64; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "../main/winlnxdefs.h"
#include <gccore.h>
#include <string.h>
#include <math.h>
#include "output.h"
#include "../main/perf_prof.h"
#include <aesndlib.h>

#include "AudioPlugin.h"
#include "Audio_#1.1.h"
#include "../main/timers.h"
#include "../main/wii64config.h"

AUDIO_INFO AudioInfo;
extern float VILimit;
#define DEFAULT_FREQUENCY 33600
#ifdef RVL_LIBWIIDRC
#include <unistd.h>
#define BUFFER_SIZE (DSP_STREAMBUFFER_SIZE * 32)
#else
#define BUFFER_SIZE (DSP_STREAMBUFFER_SIZE * 64)
#endif
static char buffer[BUFFER_SIZE];
static const char *end_ptr = buffer + BUFFER_SIZE;
static char *write_ptr, *read_ptr;
static volatile int buffered;
static unsigned int freq;
static AESNDPB *voice;
static unsigned char dsp_buffer[DSP_STREAMBUFFER_SIZE] __attribute__((aligned(32)));
static audio_output_t output;
static unsigned int playbackRate, effectiveRate;
static int activeResampler = -1, activeSync = -1;

char audioEnabled;
char audioOutputResampler;
char audioLatency = AUDIOLATENCY_STABLE;
char audioSync;

#ifdef PERF_PROF
/* Audible gaps, for perf_prof: the DSP asked for the next buffer and there was none.
   Counted once per gap (fed -> starved), and only after this game's audio has started --
   before a game first writes AI, every DSP frame is "starved" and means nothing. */
static int streamStarted, starved;
static volatile unsigned int streamRequests, streamFed;
static unsigned int queuePeakMs;
#endif

static void aesnd_callback(AESNDPB *pb, uint32_t state)
{
	PERF_SUBSYSTEM_C_SCOPE(PERF_SUB_AUDIO_CALLBACK);
	if (state == VOICE_STATE_STREAM) {
#ifdef PERF_PROF
		streamRequests++;
#endif
		if (buffered >= DSP_STREAMBUFFER_SIZE) {
			/* AESND may still read this after the callback. Queue trimming must
			   never let the producer overwrite a handed-off ring region. */
			memcpy(dsp_buffer, read_ptr, DSP_STREAMBUFFER_SIZE);
			AESND_SetVoiceBuffer(pb, dsp_buffer, DSP_STREAMBUFFER_SIZE);
			read_ptr += DSP_STREAMBUFFER_SIZE;
			if (read_ptr >= end_ptr)
				read_ptr = buffer;
			buffered -= DSP_STREAMBUFFER_SIZE;
#ifdef PERF_PROF
			streamFed++;
			starved = 0;
		} else if (streamStarted && !starved) {
			starved = 1;
			perfProf_audioUnderrun();
#endif
		}
	}
}

static void reset_buffer(void)
{
	write_ptr = buffer;
	read_ptr = buffer;
	buffered = 0;
	memset(buffer, 0, BUFFER_SIZE);
#ifdef PERF_PROF
	streamStarted = starved = 0;
#endif
}

static void configure_output(int newRom)
{
	uint32_t level = IRQ_Disable();
	reset_buffer();
#ifdef PERF_PROF
	if (newRom) streamRequests = streamFed = queuePeakMs = 0;
#else
	(void)newRom;
#endif
	IRQ_Restore(level);
	audio_output_reset(&output, freq ? freq : DEFAULT_FREQUENCY,
		audioOutputResampler == AUDIOOUTPUT_HIFI, audioSync == AUDIOSYNC_PRESERVE);
	activeResampler = audioOutputResampler;
	activeSync = audioSync;
	playbackRate = output.hifi ? 48000 : output.rate;
	effectiveRate = playbackRate;
	AESND_SetVoiceFrequency(voice, playbackRate);
	AESND_SetVoiceLoop(voice, audioSync == AUDIOSYNC_NATIVE);
}

static unsigned int queue_limit(void)
{
	if (audioLatency == AUDIOLATENCY_STABLE) return BUFFER_SIZE - 4;
	unsigned int ms = audioLatency == AUDIOLATENCY_LOW ? 40 : 80;
	unsigned int limit = effectiveRate * 4u * ms / 1000u;
	limit = limit / DSP_STREAMBUFFER_SIZE * DSP_STREAMBUFFER_SIZE;
	if (limit < DSP_STREAMBUFFER_SIZE * 2) limit = DSP_STREAMBUFFER_SIZE * 2;
	if (limit > BUFFER_SIZE - 4) limit = BUFFER_SIZE - 4;
	return limit;
}

static void discard_queued_chunk(void)
{
	read_ptr += DSP_STREAMBUFFER_SIZE;
	if (read_ptr >= end_ptr) read_ptr = buffer;
	buffered -= DSP_STREAMBUFFER_SIZE;
}

static void enqueue_pcm(void *unused, const int16_t *samples, size_t frames)
{
	(void)unused;
	size_t length = frames * 4;
	uint32_t level = IRQ_Disable();
	/* Keep the original Stable full-ring rule. Other profiles keep fresh PCM. */
	int dropped = 0;
	if (audioLatency != AUDIOLATENCY_STABLE && length < BUFFER_SIZE) {
		while (buffered >= DSP_STREAMBUFFER_SIZE && length >= (size_t)(BUFFER_SIZE - buffered)) {
			discard_queued_chunk();
			dropped = 1;
		}
	}
	if (length < (size_t)(BUFFER_SIZE - buffered)) {
		const unsigned char *stream = (const unsigned char *)samples;
		while (length) {
			size_t size = MIN((size_t)(end_ptr - write_ptr), length);
			memcpy(write_ptr, stream, size);
			stream += size;
			length -= size;
			write_ptr += size;
			if (write_ptr >= end_ptr) write_ptr = buffer;
			buffered += size;
		}
		while ((unsigned int)buffered > queue_limit() && buffered >= DSP_STREAMBUFFER_SIZE) {
			discard_queued_chunk();
			dropped = 1;
		}
#ifdef PERF_PROF
		streamStarted = 1;
#endif
	} else dropped = 1;
#ifdef PERF_PROF
	/* A slower Follow Speed rate also lengthens a full, rejected queue. */
	unsigned int ms = effectiveRate ? (unsigned int)buffered * 250u / effectiveRate : 0;
	if (ms > queuePeakMs) queuePeakMs = ms;
#endif
	if (dropped) perfProf_audioOverrun(); /* includes intentional latency-cap drops */
	IRQ_Restore(level);
}

#ifdef PERF_PROF
/* Queued PCM only. AESND/DSP output delay is not included. */
unsigned int audioQueuedMilliseconds(void)
{
	return effectiveRate ? ((unsigned int)buffered * 250u) / effectiveRate : 0;
}

/* Retain the largest queued duration at its contemporaneous requested DSP rate.
   Counts and queue occupancy do not measure speaker latency. */
void audioOutputStats(unsigned int *requests, unsigned int *fed, unsigned int *hz,
		unsigned int *peakMs, unsigned int *playbackHz)
{
	uint32_t level = IRQ_Disable();
	*requests = streamRequests;
	*fed = streamFed;
	*hz = freq;
	*peakMs = queuePeakMs;
	*playbackHz = effectiveRate;
	IRQ_Restore(level);
}
#endif

EXPORT void CALL AiDacrateChanged(int SystemType)
{
	unsigned int previous = freq;
	freq = DEFAULT_FREQUENCY;
	
	switch (SystemType)
	{
		case SYSTEM_NTSC:
			freq = 48681812 / ((uint64_t)*AudioInfo.AI_DACRATE_REG + 1);
			break;
		case SYSTEM_PAL:
			freq = 49656530 / ((uint64_t)*AudioInfo.AI_DACRATE_REG + 1);
			break;
		case SYSTEM_MPAL:
			freq = 48628316 / ((uint64_t)*AudioInfo.AI_DACRATE_REG + 1);
			break;
	}
	
	if (freq < 1000) freq = 1000;
	if (freq > 144000) freq = 144000;
	if (previous != freq || activeResampler != audioOutputResampler || activeSync != audioSync)
		configure_output(0);
}

EXPORT void CALL AiLenChanged(void)
{
	PERF_SUBSYSTEM_C_SCOPE(PERF_SUB_AUDIO_SUBMIT);
	if (audioEnabled) {
		size_t length = *AudioInfo.AI_LEN_REG;
		if (length == 0)
			return;
		unsigned int address = *AudioInfo.AI_DRAM_ADDR_REG & 0xFFFFFF;
		if ((address & 3) || (length & 3) || address >= 0x800000 || length > 0x800000 - address) {
			perfProf_audioOverrun();
			return;
		}
		if (activeResampler != audioOutputResampler || activeSync != audioSync)
			configure_output(0);
		float speed = VILimit > 0 ? Timers.vis / VILimit : 1;
		if (!isfinite(speed) || speed <= 0) speed = 1;
		if (speed < 0.5f) speed = 0.5f;
		if (speed > 2) speed = 2;
		if (audioSync == AUDIOSYNC_PRESERVE) {
			/* VI-rate estimates drift. Correct tempo, not pitch, to keep the
			   queue near its target instead of growing until packets drop. */
			float target = audioLatency == AUDIOLATENCY_LOW ? 25 :
				audioLatency == AUDIOLATENCY_BALANCED ? 50 : 120;
			float queuedMs = playbackRate ? buffered * 250.0f / playbackRate : 0;
			float correction = (queuedMs - target) * 0.002f;
			if (correction < -0.1f) correction = -0.1f;
			if (correction > 0.1f) correction = 0.1f;
			speed *= 1 + correction;
		}
		/* Always restore the ratio, including a return from faster emulation. */
		effectiveRate = (unsigned int)(playbackRate *
			(audioSync == AUDIOSYNC_FOLLOW ? speed : 1) + 0.5f);
		AESND_SetVoiceFrequencyRatio(voice, playbackRate / (float)DSP_DEFAULT_FREQ *
			(audioSync == AUDIOSYNC_FOLLOW ? speed : 1));
		unsigned long long timer = perfProf_audioStage(PERF_AUDIO_OUTPUT, length / 4);
		audio_output_push(&output, (const int16_t *)(AudioInfo.RDRAM + address), length / 4,
			speed, enqueue_pcm, NULL);
		if (timer) perfProf_audioStageEnd(PERF_AUDIO_OUTPUT, timer);
	}
}

EXPORT void CALL CloseDLL(void)
{
	AESND_FreeVoice(voice);
	voice = NULL;
}

EXPORT BOOL CALL InitiateAudio(AUDIO_INFO Audio_Info)
{
	AudioInfo = Audio_Info;
	
	voice = AESND_AllocateVoice(aesnd_callback);
	if (voice == NULL) return FALSE;
	
	AESND_SetVoiceFormat(voice, VOICE_STEREO16);
	AESND_SetVoiceStream(voice, true);
	
	return TRUE;
}

EXPORT void CALL RomOpen(void)
{
	configure_output(1);
	AESND_SetVoiceStop(voice, false);
}

EXPORT void CALL RomClosed(void)
{
	AESND_SetVoiceStop(voice, true);
}

EXPORT void CALL ProcessAlist(void)
{
}

void pauseAudio(void)
{
	AESND_SetVoiceLoop(voice, false);
	AESND_SetVoiceMute(voice, true);
}

void resumeAudio(void)
{
	if (activeResampler != audioOutputResampler || activeSync != audioSync)
		configure_output(0);
	AESND_SetVoiceFrequency(voice, playbackRate);
	effectiveRate = playbackRate;
	AESND_SetVoiceLoop(voice, audioSync == AUDIOSYNC_NATIVE);
	AESND_SetVoiceMute(voice, !audioEnabled);
}
