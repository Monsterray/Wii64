#ifndef TEST_AESNDLIB_H
#define TEST_AESNDLIB_H
#include <stdint.h>
typedef struct aesndpb_t AESNDPB;
#define DSP_STREAMBUFFER_SIZE 1152
#define DSP_DEFAULT_FREQ 48000.0f
#define VOICE_STATE_STREAM 2
#define VOICE_STEREO16 3
AESNDPB *AESND_AllocateVoice(void (*callback)(AESNDPB *, uint32_t));
void AESND_FreeVoice(AESNDPB *voice);
void AESND_SetVoiceBuffer(AESNDPB *voice, const void *data, uint32_t length);
void AESND_SetVoiceFrequency(AESNDPB *voice, unsigned int frequency);
void AESND_SetVoiceFrequencyRatio(AESNDPB *voice, float ratio);
void AESND_SetVoiceFormat(AESNDPB *voice, uint32_t format);
void AESND_SetVoiceStream(AESNDPB *voice, int stream);
void AESND_SetVoiceStop(AESNDPB *voice, int stop);
void AESND_SetVoiceLoop(AESNDPB *voice, int loop);
void AESND_SetVoiceMute(AESNDPB *voice, int mute);
#endif
