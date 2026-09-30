/* Audio HLE counters without Wii SDK types (rsp_hle/memory.h defines u32()). */
#ifndef PERF_AUDIO_H
#define PERF_AUDIO_H

enum { PERF_AUDIO_RESAMPLE, PERF_AUDIO_ZOH, PERF_AUDIO_ADPCM,
       PERF_AUDIO_ENVMIX_EXP, PERF_AUDIO_ENVMIX_GE, PERF_AUDIO_ENVMIX_LIN, PERF_AUDIO_ENVMIX_NEAD,
       PERF_AUDIO_MIX,
       PERF_AUDIO_MUSYX_VOICE, PERF_AUDIO_MUSYX_FX, PERF_AUDIO_OUTPUT, PERF_AUDIO_STAGE_COUNT };

enum { PERF_AUDIO_GAP_NEAD_MATS, PERF_AUDIO_GAP_NEAD_EFZ,
       PERF_AUDIO_GAP_RESAMPLE_FLAG2, PERF_AUDIO_GAP_MUSYX_PTR10,
       PERF_AUDIO_GAP_COUNT };

#if defined(PERF_PROF) && !defined(PERF_AUDIO_WORK_DISABLE)
#ifdef __cplusplus
extern "C" {
#endif
unsigned long long perfProf_alistResample(unsigned int samples);
unsigned long long perfProf_alistZoh(unsigned int samples);
void perfProf_musyxVoices(unsigned int voices);
unsigned long long perfProf_audioStage(unsigned int stage, unsigned int samples);
void perfProf_audioStageEnd(unsigned int stage, unsigned long long start);
void perfProf_audioSteady(unsigned int stage);
void perfProf_audioGap(unsigned int gap);
#ifdef __cplusplus
}
#endif
#else
#define perfProf_alistResample(samples) (0ULL)
#define perfProf_alistZoh(samples) (0ULL)
#define perfProf_musyxVoices(voices) ((void)0)
#define perfProf_audioStage(stage, samples) (0ULL)
#define perfProf_audioStageEnd(stage, start) ((void)0)
#define perfProf_audioSteady(stage) ((void)0)
#define perfProf_audioGap(gap) ((void)0)
#endif

#endif
