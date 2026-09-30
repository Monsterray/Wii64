/* Build: cc -DPERF_PROF -Itests/audio_stubs -fsanitize=address,undefined tests/audio_buffer_test.c -o /tmp/audio_buffer_test */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../gc_audio/audio.c"

struct aesndpb_t { int unused; };
static struct aesndpb_t pb;
static const void *last_buffer;
static uint32_t last_length;
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
void AESND_SetVoiceFrequencyRatio(AESNDPB *v, float r) { (void)v; (void)r; }
void AESND_SetVoiceFormat(AESNDPB *v, uint32_t f) { (void)v; (void)f; }
void AESND_SetVoiceStream(AESNDPB *v, int s) { (void)v; (void)s; }
void AESND_SetVoiceStop(AESNDPB *v, int s) { (void)v; (void)s; }
void AESND_SetVoiceLoop(AESNDPB *v, int l) { (void)v; (void)l; }
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
	length = 4;
	AiLenChanged();
	assert(buffered == BUFFER_SIZE - 4);
	assert(overruns == 2);

	aesnd_callback(voice, VOICE_STATE_STREAM);
	assert(buffered == BUFFER_SIZE - 4 - DSP_STREAMBUFFER_SIZE);
	assert(last_length == DSP_STREAMBUFFER_SIZE);
	unsigned int requests, fed, hz, peakMs;
	audioOutputStats(&requests, &fed, &hz, &peakMs);
	assert(requests == 2 && fed == 1);
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
	CloseDLL();
	puts("audio buffer boundaries: ok");
	return 0;
}
