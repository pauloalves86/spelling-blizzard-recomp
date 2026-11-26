/* Decompiled Bright Star Audio, Timer, & BSW WaveOut Streamer Subsystem (Path 2)
 *
 * Reconstructs the 45 functions governing 60 Hz multimedia timing, `BSW\0`
 * audio resource decoding, `waveOut` PCM playback, `CSoundManager_1008_7edc`
 * (`DAT_1060_07c8`), and `CSoundChannel_1010_bdd4` (and its base timer class
 * `CTimedCallback_1008_823c`):
 *
 * 1. 60 Hz Multimedia Timer (`CTimer60Hz_1010_456a`, `DAT_1060_0a1e`):
 *    - `CTimer60Hz_Ctor_1010_446a` (`1010:446a`)
 *    - `CTimer60Hz_Dtor_1010_44d6` (`1010:44d6`)
 *    - `CTimer60Hz_GetTicks_1010_44ec` (`1010:44ec`)
 *    - `Timer_GetTicks60Hz_1010_07ae` (`1010:07ae`)
 *    - `Random_Range_1010_24f8` (`1010:24f8`)
 *
 * 2. BSW Audio Sample & WaveOut Player (`1010:312a..3a24`):
 *    - `BswSample_InitEmpty_1010_312a` (`1010:312a`)
 *    - `BswSample_FreeBuffer_1010_3152` (`1010:3152`)
 *    - `BswSample_Ctor_1010_3172` (`1010:3172`)
 *    - `BswSample_LoadFromExeResource_1010_3190` (`1010:3190`)
 *    - `BswSample_ParseHeaderAndCopyPcm_1010_31d8` (`1010:31d8`)
 *    - `BswStream_UnprepareAndFree_1010_32bc` (`1010:32bc`)
 *    - `BswStream_PumpNextChunk_1010_3386` (`1010:3386`)
 *    - `BswStream_StartChunked_1010_34ea` (`1010:34ea`)
 *    - `WavePlayer_Ctor_1010_36a6` (`1010:36a6`)
 *    - `WavePlayer_StopAndClose_1010_36d0` (`1010:36d0`)
 *    - `WavePlayer_IsPlaying_1010_37ae` (`1010:37ae`)
 *    - `WavePlayer_OpenAndWrite_1010_37e6` (`1010:37e6`)
 *    - `WavePlayer_PlaySample_1010_39d4` (`1010:39d4`)
 *    - `WavePlayer_SetEndTick_1010_3a10` (`1010:3a10`)
 *    - `WavePlayer_NoopCallback_1010_3a24` (`1010:3a24`)
 *
 * 3. Global Sound Manager (`CSoundManager_1008_7edc` at `DAT_1060_07c8`):
 *    - `CSoundManager_Init_1008_21ec` (`1008:21ec`)
 *    - `CSoundManager_Dtor_1008_225a` (`1008:225a`)
 *    - `CSoundManager_IsHardwareBusy_1008_22ac` (`1008:22ac`)
 *    - `CSoundManager_IsActive_1008_22ba` (`1008:22ba`)
 *    - `CSoundManager_WasInterrupted_1008_22c8` (`1008:22c8`)
 *    - `CSoundManager_PlaySoundResource_1008_22d6` (`1008:22d6`)
 *    - `CSoundManager_WaitOrInterrupt_1008_23b2` (`1008:23b2`)
 *    - `CSoundManager_WaitUntilDone_1008_2436` (`1008:2436`)
 *    - `CSoundManager_Stop_1008_2462` (`1008:2462`)
 *    - `CSoundManager_IdleTimerCheck_1008_24ec` (`1008:24ec`)
 *    - `CSoundManager_GetCurrentSoundId_1008_2530` (`1008:2530`)
 *    - `System_CheckRequirements_1008_6a28` (`1008:6a28`)
 *
 * 4. Timed Callback & Sound Sequence Channel (`CTimedCallback_1008_823c` & `CSoundChannel_1010_bdd4`):
 *    - `CTimedCallback_GetByte0a_1018_63b2` (`1018:63b2`)
 *    - `CTimedCallback_GetByte09_1018_63c0` (`1018:63c0`)
 *    - `CTimedCallback_IsPaused_1018_63ce` (`1018:63ce`)
 *    - `CTimedCallback_Resume_1018_63dc` (`1018:63dc`)
 *    - `CTimedCallback_Pause_1018_63ec` (`1018:63ec`)
 *    - `CTimedCallback_OnStartNoop_1018_63fc` (`1018:63fc`)
 *    - `CTimedCallback_OnTickNoop_1018_6400` (`1018:6400`)
 *    - `CTimedCallback_Execute_1018_6404` (`1018:6404`)
 *    - `CTimedCallback_IsReady_1018_642c` (`1018:642c`)
 *    - `CTimedCallback_ScheduleNext_1018_646a` (`1018:646a`)
 *    - `CTimedCallback_False0_1018_648e` (`1018:648e`)
 *    - `CTimedCallback_False1_1018_6494` (`1018:6494`)
 *    - `CSoundChannel_Dtor_1020_27d8` (`1020:27d8`)
 *    - `CSoundChannel_ExecuteNext_1020_2810` (`1020:2810`)
 *    - `CSoundChannel_IsReady_1020_28e0` (`1020:28e0`)
 *    - `CSoundChannel_True_1020_2922` (`1020:2922`)
 *    - `CSoundChannel_AddSound_1020_2928` (`1020:2928`)
 *    - `CSoundChannel_SetPrimarySound_1020_29b6` (`1020:29b6`)
 *    - `CSoundChannel_ClearSounds_1020_2a96` (`1020:2a96`)
 */
#ifndef SB_RECOVERED_FRAMEWORK_AUDIO_WAVEOUT_H
#define SB_RECOVERED_FRAMEWORK_AUDIO_WAVEOUT_H

#include <stddef.h>
#include <stdint.h>
#include "../../shim/win16_waveout.h"
#include "resource_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 1. 60 Hz Multimedia Timer (`CTimer60Hz_1010_456a`, 0x12 bytes, at `DAT_1060_0a1e`) */
typedef struct SBTimer60Hz {
    uint32_t vtable_tag_00;   /* +0x00: 0x1010456a */
    uint32_t last_ms_04;      /* +0x04: previous timeGetTime() step */
    uint32_t current_ms_08;   /* +0x08: latest timeGetTime() read */
    uint32_t ticks_60hz_0c;   /* +0x0c: 60 Hz tick counter (3 ticks per 50 ms) */
    uint16_t sub_step_10;     /* +0x10: 0..4 modulo counter for 10ms -> 60Hz Bresenham step */
} SBTimer60Hz;

/* 2. BSW Audio Sample Descriptor (10 bytes / `0x0a`, allocated in `1008:22d6` / `1010:3172`)
 * Parses the 16-byte `BSW\0` header:
 *   raw+0x00..0x03: "BSW\0"
 *   raw+0x04..0x05: reserved/version
 *   raw+0x06..0x07: format_code (1 = 8-bit mono, 2 = 8-bit stereo, 3 = 16-bit mono, 4 = 16-bit stereo)
 *   raw+0x08..0x09: sample_rate_hz (< 22050 -> 11025 Hz; >= 22050 -> 22050 Hz)
 *   raw+0x0a..0x0d: uint32_t sample_frames
 *   raw+0x0e..0x0f: uint16_t compression_flag (0 = uncompressed PCM starting at raw+0x10)
 */
typedef struct SBBswSample {
    uint32_t sample_frames_00;/* +0x00: number of sample frames */
    uint16_t sample_rate_04;  /* +0x04: 11025 (0x2b11) or 22050 (0x5622) */
    uint16_t format_code_06;  /* +0x06: 1 (8-bit mono), 2 (8-bit stereo), 3 (16-bit mono), 4 (16-bit stereo) */
    uint8_t *pcm_buffer_08;   /* +0x08: allocated PCM sample buffer */
    uint32_t pcm_byte_size;   /* computed byte size = sample_frames * bytes_per_frame */
    /* Optional streaming fields when `is_streamed_2e != 0` (`1010:32bc..34ea`) */
    void (*stream_fill_cb_0a)(uint8_t *dst, uint32_t offset, uint32_t len, SB_HWAVEOUT hwo);
    uint32_t chunk_size_0e;   /* +0x0e */
    uint32_t total_stream_12; /* +0x12 */
    uint32_t stream_pos_16;   /* +0x16 */
    uint16_t num_chunks_1a;   /* +0x1a */
    uint16_t cur_chunk_1c;    /* +0x1c */
    uint8_t *stream_buf_1e;   /* +0x1e */
    SB_WAVEHDR *stream_hdrs_22; /* +0x22 */
} SBBswSample;

/* WaveOut Hardware Player (`0x32` / 50 bytes, at `DAT_1060_07cc`) */
typedef struct SBWavePlayer {
    SBBswSample *sample_00;   /* +0x00: current active SBBswSample */
    uint32_t end_tick_04;     /* +0x04: 60 Hz tick when playback finishes */
    uint8_t *locked_pcm_08;   /* +0x08: locked pointer to sample->pcm_buffer_08 */
    SB_WAVEHDR wave_hdr_0c;   /* +0x0c..+0x2b: 32-byte Win16 WAVEHDR */
    SB_HWAVEOUT hWaveOut_2c;  /* +0x2c: Win16 HWAVEOUT handle */
    uint16_t is_streamed_2e;  /* +0x2e: 0 = single buffer, nonzero = chunked stream */
    uint16_t owns_sample_30;  /* +0x30: nonzero if StopAndClose should free sample_00 */
} SBWavePlayer;

/* 3. Global Sound Manager (`CSoundManager_1008_7edc`, `0x20` / 32 bytes, at `DAT_1060_07c8`) */
typedef struct SBSoundManager {
    uint32_t vtable_tag_00;   /* +0x00: 0x10087edc */
    uint16_t current_snd_id_04; /* +0x04: resource ID of currently playing `'snd '` */
    SBResourceHandle *snd_handle_06; /* +0x06: locked `'snd '` resource handle */
    uint8_t interrupted_0a;   /* +0x0a: set to 1 if interrupted by keyboard/mouse in 1008:23b2 */
    uint8_t pad_0b;
    SBBswSample *sample_0c;   /* +0x0c: current BSW sample (mirrors sample_1c) */
    uint8_t saved_res_flags_10; /* +0x10: original handle flags restored on stop */
    uint8_t pad_11;
    uint32_t next_idle_tick_12; /* +0x12: 60 Hz tick timer (+36000 ticks = 10 mins) */
    uint8_t is_active_16;     /* +0x16: 1 while sound is loaded/playing */
    uint8_t pad_17[5];
    SBBswSample *sample_1c;   /* +0x1c: allocated 10-byte SBBswSample */
} SBSoundManager;

/* 4. Sound Sequence Channel (`CSoundChannel_1010_bdd4`, inherits `CTimedCallback_1008_823c`) */
#define SB_SOUND_CHANNEL_MAX_SOUNDS 32

typedef struct SBSoundChannel {
    uint32_t vtable_tag_00;   /* +0x00: 0x1010bdd4 */
    uint32_t interval_ticks_04; /* +0x04: base interval in 60 Hz ticks */
    uint8_t is_paused_08;     /* +0x08 */
    uint8_t flag_09;          /* +0x09 */
    uint8_t flag_0a;          /* +0x0a */
    uint8_t pad_0b;
    uint32_t next_base_tick_0c; /* +0x0c */
    void *target_receiver_10; /* +0x10: optional receiver notified when sound triggers */
    int16_t current_index_14; /* +0x14: current index into sound_ids_24 */
    uint32_t next_fire_tick_16; /* +0x16: 60 Hz tick for next sound trigger */
    uint8_t randomize_1a;     /* +0x1a: 0 = sequential cycle, nonzero = random choice */
    uint8_t pad_1b;
    int16_t min_delay_1c;     /* +0x1c: min random delay ticks */
    int16_t max_delay_1e;     /* +0x1e: max random delay ticks */
    int16_t primary_snd_id_20;/* +0x20: primary/fallback `'snd '` resource ID (-1 if none) */
    int16_t sound_count_22;   /* +0x22: number of `'snd '` IDs in sound_ids_24 */
    uint16_t sound_ids_24[SB_SOUND_CHANNEL_MAX_SOUNDS]; /* +0x24: `'snd '` resource IDs */
} SBSoundChannel;

/* Globals in Segment 1060 */
extern SBTimer60Hz *g_DAT_1060_0a1e_timer60Hz;
extern SBWavePlayer *g_DAT_1060_07cc_wavePlayer;
extern SBSoundManager *g_DAT_1060_07c8_soundMgr;

/* 1. 60 Hz Multimedia Timer (`1010:446a..44ec`, `1010:07ae`, `1010:24f8`) */
SBTimer60Hz *CTimer60Hz_Ctor_1010_446a(SBTimer60Hz *self);
void CTimer60Hz_Dtor_1010_44d6(SBTimer60Hz *self);
uint32_t CTimer60Hz_GetTicks_1010_44ec(SBTimer60Hz *self);
uint32_t Timer_GetTicks60Hz_1010_07ae(void);
int16_t Random_Range_1010_24f8(int16_t min_val, int16_t max_val);

/* 2. BSW Audio Sample & WaveOut Player (`1010:312a..3a24`) */
SBBswSample *BswSample_InitEmpty_1010_312a(SBBswSample *self);
void BswSample_FreeBuffer_1010_3152(SBBswSample *self);
SBBswSample *BswSample_Ctor_1010_3172(SBBswSample *self);
void BswSample_LoadFromExeResource_1010_3190(SBBswSample *self, uint16_t res_id, uint16_t res_type);
void BswSample_ParseHeaderAndCopyPcm_1010_31d8(SBBswSample *self, const void *bsw_raw_data);
void BswStream_UnprepareAndFree_1010_32bc(SBBswSample *self);
void BswStream_PumpNextChunk_1010_3386(SBBswSample *self);
void BswStream_StartChunked_1010_34ea(SBBswSample *self, void (*on_chunk_cb)(SB_HWAVEOUT));
SBWavePlayer *WavePlayer_Ctor_1010_36a6(SBWavePlayer *self);
void WavePlayer_StopAndClose_1010_36d0(SBWavePlayer *self);
uint8_t WavePlayer_IsPlaying_1010_37ae(SBWavePlayer *self);
void WavePlayer_OpenAndWrite_1010_37e6(SBWavePlayer *self, void (*on_chunk_cb)(SB_HWAVEOUT));
void WavePlayer_PlaySample_1010_39d4(SBWavePlayer *self,
                                     void (*on_chunk_cb)(SB_HWAVEOUT),
                                     uint16_t is_streamed,
                                     SBBswSample *sample);
void WavePlayer_SetEndTick_1010_3a10(SBWavePlayer *self, uint32_t end_tick);
void WavePlayer_NoopCallback_1010_3a24(void);

/* 3. Global Sound Manager (`CSoundManager_1008_7edc`) */
SBSoundManager *CSoundManager_Init_1008_21ec(SBSoundManager *self);
void CSoundManager_Dtor_1008_225a(SBSoundManager *self);
uint8_t CSoundManager_IsHardwareBusy_1008_22ac(SBSoundManager *self);
uint8_t CSoundManager_IsActive_1008_22ba(const SBSoundManager *self);
uint8_t CSoundManager_WasInterrupted_1008_22c8(const SBSoundManager *self);
void CSoundManager_PlaySoundResource_1008_22d6(SBSoundManager *self, uint16_t snd_res_id);
void CSoundManager_WaitOrInterrupt_1008_23b2(SBSoundManager *self);
void CSoundManager_WaitUntilDone_1008_2436(SBSoundManager *self);
void CSoundManager_Stop_1008_2462(SBSoundManager *self);
void CSoundManager_IdleTimerCheck_1008_24ec(SBSoundManager *self);
uint16_t CSoundManager_GetCurrentSoundId_1008_2530(const SBSoundManager *self);
uint16_t System_CheckRequirements_1008_6a28(void);

/* 4. Timed Callback & Sound Sequence Channel (`CTimedCallback_1008_823c` & `CSoundChannel_1010_bdd4`) */
uint8_t CTimedCallback_GetByte0a_1018_63b2(const SBSoundChannel *self);
uint8_t CTimedCallback_GetByte09_1018_63c0(const SBSoundChannel *self);
uint8_t CTimedCallback_IsPaused_1018_63ce(const SBSoundChannel *self);
void CTimedCallback_Resume_1018_63dc(SBSoundChannel *self);
void CTimedCallback_Pause_1018_63ec(SBSoundChannel *self);
void CTimedCallback_OnStartNoop_1018_63fc(void);
void CTimedCallback_OnTickNoop_1018_6400(void);
void CTimedCallback_Execute_1018_6404(SBSoundChannel *self, uint32_t *out_interval);
uint8_t CTimedCallback_IsReady_1018_642c(SBSoundChannel *self);
void CTimedCallback_ScheduleNext_1018_646a(SBSoundChannel *self);
uint8_t CTimedCallback_False0_1018_648e(void);
uint8_t CTimedCallback_False1_1018_6494(void);

void CSoundChannel_Init(SBSoundChannel *self);
void CSoundChannel_Dtor_1020_27d8(SBSoundChannel *self);
uint16_t CSoundChannel_ExecuteNext_1020_2810(SBSoundChannel *self, uint32_t *out_interval);
uint8_t CSoundChannel_IsReady_1020_28e0(SBSoundChannel *self);
uint8_t CSoundChannel_True_1020_2922(void);
void CSoundChannel_AddSound_1020_2928(SBSoundChannel *self, uint16_t snd_res_id);
void CSoundChannel_SetPrimarySound_1020_29b6(SBSoundChannel *self, int16_t snd_res_id);
void CSoundChannel_ClearSounds_1020_2a96(SBSoundChannel *self);

#ifdef __cplusplus
}
#endif

#endif /* SB_RECOVERED_FRAMEWORK_AUDIO_WAVEOUT_H */
