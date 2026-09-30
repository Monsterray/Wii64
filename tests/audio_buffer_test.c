/* Run through .dev/test_rsp_audio.sh, with the real output processor linked. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../gc_audio/audio.c"

struct aesndpb_t { int unused; };
static struct aesndpb_t pb;
static const void *last_buffer;
static uint32_t last_length;
static float last_ratio;
static int last_loop;
float VILimit = 60.0f;
timers Timers;
static unsigned int overruns, underruns;
void perfProf_audioOverrun(void) { overruns++; }
void perfProf_audioUnderrun(void) { underruns++; }

AESNDPB *AESND_AllocateVoice(void (*callback)(AESNDPB *, uint32_t))
{
	(void)callback;
	return &pb;
}
void AESND_FreeVoice(AESNDPB *v) { (void)v; }
void AESND_SetVoiceBuffer(AESNDPB *v, const void *data, uint32_t length)
{
	(void)v;
	last_buffer = data;
	last_length = length;
}
void AESND_SetVoiceFrequency(AESNDPB *v, unsigned int f) { (void)v; (void)f; }
void AESND_SetVoiceFrequencyRatio(AESNDPB *v, float r) { (void)v; last_ratio = r; }
void AESND_SetVoiceFormat(AESNDPB *v, uint32_t f) { (void)v; (void)f; }
void AESND_SetVoiceStream(AESNDPB *v, int s) { (void)v; (void)s; }
void AESND_SetVoiceStop(AESNDPB *v, int s) { (void)v; (void)s; }
void AESND_SetVoiceLoop(AESNDPB *v, int l) { (void)v; last_loop = l; }
void AESND_SetVoiceMute(AESNDPB *v, int m) { (void)v; (void)m; }

int main(void)
{
	static unsigned char rdram[BUFFER_SIZE + DSP_STREAMBUFFER_SIZE];
	DWORD address = 0, length = 0, dacrate = 1519;
	AUDIO_INFO info = {0};
	for (size_t i = 0; i < sizeof(rdram); i++) rdram[i] = (unsigned char)i;
	info.RDRAM = rdram;
	info.AI_DRAM_ADDR_REG = &address;
	info.AI_LEN_REG = &length;
	info.AI_DACRATE_REG = &dacrate;
	assert(InitiateAudio(info));
	AiDacrateChanged(SYSTEM_NTSC);
	audioEnabled = 1;
	RomOpen();
	assert(last_loop);
	pauseAudio();
	assert(!last_loop);
	resumeAudio();
	assert(last_loop); /* Native keeps the legacy streaming policy after menu exit. */
	AiLenChanged();
	assert(buffered == 0);
	assert(streamStarted == 0);
	aesnd_callback(voice, VOICE_STATE_STREAM);
	assert(last_length == 0);
	assert(underruns == 0);

	length = BUFFER_SIZE;
	AiLenChanged();
	assert(buffered == 0); /* preserve the original spare-space rule */
	assert(overruns == 1);
	length = BUFFER_SIZE - 4;
	AiLenChanged();
	assert(buffered == BUFFER_SIZE - 4);
	assert(streamStarted == 1);
	assert(memcmp(buffer, rdram, BUFFER_SIZE - 4) == 0);
	AiDacrateChanged(SYSTEM_NTSC);
	assert(buffered == BUFFER_SIZE - 4); /* unchanged DAC writes keep the queue/history */
	length = 4;
	AiLenChanged();
	assert(buffered == BUFFER_SIZE - 4);
	assert(overruns == 2);

	aesnd_callback(voice, VOICE_STATE_STREAM);
	assert(buffered == BUFFER_SIZE - 4 - DSP_STREAMBUFFER_SIZE);
	assert(last_length == DSP_STREAMBUFFER_SIZE);
	unsigned int requests, fed, hz, peakMs, playbackHz;
	audioOutputStats(&requests, &fed, &hz, &peakMs, &playbackHz);
	assert(requests == 2 && fed == 1 && playbackHz == playbackRate);
	assert(hz == 48681812u / 1520u);
	assert(peakMs == (BUFFER_SIZE - 4u) * 250u / hz);
	assert(memcmp(last_buffer, rdram, DSP_STREAMBUFFER_SIZE) == 0);
	address = DSP_STREAMBUFFER_SIZE;
	length = DSP_STREAMBUFFER_SIZE;
	AiLenChanged();
	assert(buffered == BUFFER_SIZE - 4);
	assert(memcmp(buffer + BUFFER_SIZE - 4, rdram + DSP_STREAMBUFFER_SIZE, 4) == 0);
	assert(memcmp(buffer, rdram + DSP_STREAMBUFFER_SIZE + 4, DSP_STREAMBUFFER_SIZE - 4) == 0);

	length = (DWORD)-1;
	AiLenChanged();
	assert(buffered == BUFFER_SIZE - 4);
	assert(overruns == 3);

	/* Bounded profiles trim only unread chunks, not the DSP hand-off copy. */
	RomOpen();
	address = 0;
	length = 8 * DSP_STREAMBUFFER_SIZE;
	audioLatency = AUDIOLATENCY_LOW;
	AiLenChanged();
	assert((unsigned int)buffered <= queue_limit());
	assert(audioQueuedMilliseconds() <= 40);
	assert(buffered >= DSP_STREAMBUFFER_SIZE * 2);
	aesnd_callback(voice, VOICE_STATE_STREAM);
	unsigned char handed[DSP_STREAMBUFFER_SIZE];
	memcpy(handed, last_buffer, sizeof(handed));
	for (int i = 0; i < 100; i++) AiLenChanged();
	assert(!memcmp(handed, last_buffer, sizeof(handed)));
	assert((unsigned int)buffered <= queue_limit());
	audioLatency = AUDIOLATENCY_BALANCED;
	RomOpen();
	AiLenChanged();
	assert(audioQueuedMilliseconds() <= 80);
	assert(buffered > DSP_STREAMBUFFER_SIZE * 2);

	audioSync = AUDIOSYNC_FOLLOW;
	resumeAudio();
	assert(!last_loop);
	Timers.vis = 90;
	AiLenChanged();
	assert(fabsf(last_ratio - playbackRate * 1.5f / DSP_DEFAULT_FREQ) < 0.00001f);
	assert(audioQueuedMilliseconds() == (unsigned int)buffered * 250u /
		(unsigned int)(playbackRate * 1.5f + 0.5f));
	Timers.vis = 30;
	AiLenChanged();
	assert((unsigned int)buffered <= queue_limit());
	assert(audioQueuedMilliseconds() <= 80);
	Timers.vis = 60;
	AiLenChanged();
	assert(fabsf(last_ratio - playbackRate / DSP_DEFAULT_FREQ) < 0.00001f);
	audioSync = AUDIOSYNC_PRESERVE;
	audioOutputResampler = AUDIOOUTPUT_HIFI;
	AiLenChanged();
	assert(playbackRate == 48000 && last_ratio == 1);
	assert(output.preserve && output.hifi);
	audioSync = AUDIOSYNC_NATIVE;
	audioOutputResampler = AUDIOOUTPUT_DSP;
	AiLenChanged();
	assert(!output.preserve && !output.hifi);
	unsigned int before = overruns;
	address = 0x800000;
	AiLenChanged();
	assert(overruns == before + 1);
	address = 1;
	AiLenChanged();
	assert(overruns == before + 2);
	dacrate = UINT32_MAX;
	AiDacrateChanged(SYSTEM_NTSC);
	assert(freq == 1000);

	/* Counters and the peak belong to the ROM, not the current DAC/mode. */
	dacrate = 1519;
	AiDacrateChanged(SYSTEM_NTSC);
	audioLatency = AUDIOLATENCY_STABLE;
	address = 0;
	length = 8 * DSP_STREAMBUFFER_SIZE;
	RomOpen();
	AiLenChanged();
	aesnd_callback(voice, VOICE_STATE_STREAM);
	audioOutputStats(&requests, &fed, &hz, &peakMs, &playbackHz);
	unsigned int oldPeak = peakMs;
	assert(requests == 1 && fed == 1);
	dacrate = 2207;
	AiDacrateChanged(SYSTEM_NTSC);
	audioOutputStats(&requests, &fed, &hz, &peakMs, &playbackHz);
	assert(requests == 1 && fed == 1 && peakMs == oldPeak);
	audioOutputResampler = AUDIOOUTPUT_HIFI;
	resumeAudio();
	audioOutputStats(&requests, &fed, &hz, &peakMs, &playbackHz);
	assert(requests == 1 && fed == 1 && peakMs == oldPeak);
	RomOpen();
	audioOutputStats(&requests, &fed, &hz, &peakMs, &playbackHz);
	assert(requests == 0 && fed == 0 && peakMs == 0);

	/* Follow Speed queue duration uses its requested DSP frequency, not input Hz. */
	audioOutputResampler = AUDIOOUTPUT_DSP;
	audioSync = AUDIOSYNC_FOLLOW;
	Timers.vis = 90;
	RomOpen();
	AiLenChanged();
	unsigned int followedHz = (unsigned int)(playbackRate * 1.5f + 0.5f);
	assert(audioQueuedMilliseconds() == (unsigned int)buffered * 250u / followedHz);
	RomOpen();
	length = BUFFER_SIZE - 4;
	AiLenChanged();
	Timers.vis = 30;
	length = 4;
	AiLenChanged(); /* Stable rejects the write, but slower playback extends the peak. */
	audioOutputStats(&requests, &fed, &hz, &peakMs, &playbackHz);
	assert(buffered == BUFFER_SIZE - 4);
	assert(peakMs == audioQueuedMilliseconds());
	assert(playbackHz == (unsigned int)(playbackRate * 0.5f + 0.5f));
	CloseDLL();
	puts("audio buffer boundaries: ok");
	return 0;
}
