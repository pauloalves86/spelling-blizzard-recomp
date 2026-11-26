#include "audio_waveout.h"

#include <stdlib.h>
#include <string.h>

SBTimer60Hz *g_DAT_1060_0a1e_timer60Hz = NULL;
SBWavePlayer *g_DAT_1060_07cc_wavePlayer = NULL;
SBSoundManager *g_DAT_1060_07c8_soundMgr = NULL;

static uint16_t read_u16_le(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t read_u32_le(const uint8_t *p)
{
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

/* ==========================================================================
 * 1. 60 Hz Multimedia Timer (`1010:446a..44ec`, `1010:07ae`, `1010:24f8`)
 * ========================================================================== */

/* Original: 1010:446a */
SBTimer60Hz *CTimer60Hz_Ctor_1010_446a(SBTimer60Hz *self)
{
    uint32_t now_ms;
    if (!self) {
        return NULL;
    }
    self->vtable_tag_00 = 0x1010456au;
    now_ms = sb_win16_timeGetTime();
    self->last_ms_04 = now_ms;
    self->current_ms_08 = now_ms;
    self->ticks_60hz_0c = (uint32_t)(((uint64_t)now_ms * 3u) / 50u);
    self->sub_step_10 = 0;
    return self;
}

/* Original: 1010:44d6 */
void CTimer60Hz_Dtor_1010_44d6(SBTimer60Hz *self)
{
    if (self) {
        self->vtable_tag_00 = 0x1010456au;
    }
}

/* Original: 1010:44ec
 * Advances the 60 Hz tick counter in 10 ms increments using a 5-step cycle
 * (steps 0, 2, 4 increment ticks_60hz_0c; steps 1, 3 do not -> 3 ticks per 50 ms = 60 Hz).
 */
uint32_t CTimer60Hz_GetTicks_1010_44ec(SBTimer60Hz *self)
{
    if (!self) {
        return 0;
    }
    self->current_ms_08 = sb_win16_timeGetTime();
    while (self->last_ms_04 + 10u < self->current_ms_08) {
        uint16_t step = self->sub_step_10;
        if (step == 0 || step == 2) {
            self->ticks_60hz_0c++;
            self->sub_step_10++;
        } else if (step == 1 || step == 3) {
            self->sub_step_10++;
        } else if (step == 4) {
            self->ticks_60hz_0c++;
            self->sub_step_10 = 0;
        } else {
            self->sub_step_10 = 0;
        }
        self->last_ms_04 += 10u;
    }
    return self->ticks_60hz_0c;
}

/* Original: 1010:07ae */
uint32_t Timer_GetTicks60Hz_1010_07ae(void)
{
    if (!g_DAT_1060_0a1e_timer60Hz) {
        return (uint32_t)(((uint64_t)sb_win16_timeGetTime() * 3u) / 50u);
    }
    return CTimer60Hz_GetTicks_1010_44ec(g_DAT_1060_0a1e_timer60Hz);
}

/* Original: 1010:24f8 */
int16_t Random_Range_1010_24f8(int16_t min_val, int16_t max_val)
{
    int r;
    if (max_val < min_val) {
        max_val = min_val;
    }
    if (max_val == min_val) {
        return min_val;
    }
    r = rand() & 0x7FFF;
    return (int16_t)(min_val + (r % ((max_val - min_val) + 1)));
}

/* ==========================================================================
 * 2. BSW Audio Sample & WaveOut Player (`1010:312a..3a24`)
 * ========================================================================== */

/* Original: 1010:312a */
SBBswSample *BswSample_InitEmpty_1010_312a(SBBswSample *self)
{
    if (!self) {
        return NULL;
    }
    memset(self, 0, sizeof(*self));
    return self;
}

/* Original: 1010:3152 */
void BswSample_FreeBuffer_1010_3152(SBBswSample *self)
{
    if (self && self->pcm_buffer_08 != NULL) {
        free(self->pcm_buffer_08);
        self->pcm_buffer_08 = NULL;
    }
}

/* Original: 1010:3172 */
SBBswSample *BswSample_Ctor_1010_3172(SBBswSample *self)
{
    return BswSample_InitEmpty_1010_312a(self);
}

/* Original: 1010:3190 */
void BswSample_LoadFromExeResource_1010_3190(SBBswSample *self, uint16_t res_id, uint16_t res_type)
{
    SB_HRSRC hRsrc = sb_win16_FindResource(g_DAT_1060_07d0_hInstance, res_id, res_type);
    SB_HGLOBAL hGlob = sb_win16_LoadResource(g_DAT_1060_07d0_hInstance, hRsrc);
    const void *raw = sb_win16_LockResource(hGlob);
    if (raw != NULL) {
        BswSample_ParseHeaderAndCopyPcm_1010_31d8(self, raw);
    }
    sb_win16_GlobalUnlock(hGlob);
    sb_win16_FreeResource(hGlob);
}

/* Original: 1010:31d8
 * Parses the 16-byte `BSW\0` header and copies the uncompressed PCM payload at +0x10.
 */
void BswSample_ParseHeaderAndCopyPcm_1010_31d8(SBBswSample *self, const void *bsw_raw_data)
{
    const uint8_t *raw = (const uint8_t *)bsw_raw_data;
    uint16_t raw_rate;
    uint32_t bytes_per_frame = 1;
    uint16_t compression_flag;

    if (!self || !raw) {
        return;
    }
    self->format_code_06 = read_u16_le(raw + 6);
    raw_rate = read_u16_le(raw + 8);
    self->sample_frames_00 = read_u32_le(raw + 10);
    compression_flag = read_u16_le(raw + 14);

    if (raw_rate < 0x5622u) {
        self->sample_rate_04 = 0x2b11u; /* 11,025 Hz */
    } else {
        self->sample_rate_04 = 0x5622u; /* 22,050 Hz */
    }

    if (self->format_code_06 == 1) {
        bytes_per_frame = 1;
    } else if (self->format_code_06 == 2 || self->format_code_06 == 3) {
        bytes_per_frame = 2;
    } else if (self->format_code_06 == 4) {
        bytes_per_frame = 4;
    }

    self->pcm_byte_size = self->sample_frames_00 * bytes_per_frame;
    self->pcm_buffer_08 = (uint8_t *)malloc(self->pcm_byte_size > 0 ? self->pcm_byte_size : 1u);
    if (self->pcm_buffer_08 != NULL && compression_flag == 0 && self->pcm_byte_size > 0) {
        memcpy(self->pcm_buffer_08, raw + 0x10, self->pcm_byte_size);
    }
}

/* Original: 1010:32bc */
void BswStream_UnprepareAndFree_1010_32bc(SBBswSample *self)
{
    uint16_t i;
    SB_HWAVEOUT hwo;

    if (!self) {
        return;
    }
    hwo = g_DAT_1060_07cc_wavePlayer ? g_DAT_1060_07cc_wavePlayer->hWaveOut_2c : 0;
    self->stream_pos_16 = 0;
    self->cur_chunk_1c = 0;
    if (self->stream_hdrs_22 != NULL) {
        for (i = 0; i < self->num_chunks_1a; ++i) {
            sb_win16_waveOutUnprepareHeader(hwo, &self->stream_hdrs_22[i], sizeof(SB_WAVEHDR));
        }
        free(self->stream_hdrs_22);
        self->stream_hdrs_22 = NULL;
    }
    if (self->stream_buf_1e != NULL) {
        free(self->stream_buf_1e);
        self->stream_buf_1e = NULL;
    }
    BswSample_FreeBuffer_1010_3152(self);
}

/* Original: 1010:3386 */
void BswStream_PumpNextChunk_1010_3386(SBBswSample *self)
{
    SB_WAVEHDR *hdr;
    SB_HWAVEOUT hwo;
    uint32_t remaining;
    uint32_t chunk_len;

    if (!self || !self->stream_hdrs_22 || !g_DAT_1060_07cc_wavePlayer) {
        return;
    }
    hdr = &self->stream_hdrs_22[self->cur_chunk_1c];
    if ((hdr->dwFlags & SB_WHDR_DONE) == 0) {
        return;
    }
    hwo = g_DAT_1060_07cc_wavePlayer->hWaveOut_2c;
    if (sb_win16_waveOutUnprepareHeader(hwo, hdr, sizeof(SB_WAVEHDR)) != 0) {
        WavePlayer_StopAndClose_1010_36d0(g_DAT_1060_07cc_wavePlayer);
        return;
    }
    if (self->total_stream_12 <= self->stream_pos_16) {
        return;
    }
    remaining = self->total_stream_12 - self->stream_pos_16;
    if (self->chunk_size_0e <= remaining) {
        chunk_len = self->chunk_size_0e;
    } else {
        chunk_len = remaining;
        hdr->dwBufferLength = chunk_len;
    }
    if (self->stream_fill_cb_0a != NULL) {
        self->stream_fill_cb_0a((uint8_t *)hdr->lpData, self->stream_pos_16, chunk_len, hwo);
    }
    if (sb_win16_waveOutPrepareHeader(hwo, hdr, sizeof(SB_WAVEHDR)) == 0 &&
        sb_win16_waveOutWrite(hwo, hdr, sizeof(SB_WAVEHDR)) == 0) {
        self->stream_pos_16 += self->chunk_size_0e;
        self->cur_chunk_1c++;
        if (self->cur_chunk_1c >= self->num_chunks_1a) {
            self->cur_chunk_1c = 0;
        }
        return;
    }
    WavePlayer_StopAndClose_1010_36d0(g_DAT_1060_07cc_wavePlayer);
}

/* Original: 1010:34ea */
void BswStream_StartChunked_1010_34ea(SBBswSample *self, void (*on_chunk_cb)(SB_HWAVEOUT))
{
    uint32_t total_buf_bytes;
    uint16_t i;
    SB_HWAVEOUT hwo;

    if (!self || !g_DAT_1060_07cc_wavePlayer || self->num_chunks_1a == 0) {
        return;
    }
    hwo = g_DAT_1060_07cc_wavePlayer->hWaveOut_2c;
    total_buf_bytes = (uint32_t)self->num_chunks_1a * self->chunk_size_0e;
    self->stream_buf_1e = (uint8_t *)calloc(1, total_buf_bytes > 0 ? total_buf_bytes : 1u);
    self->stream_hdrs_22 = (SB_WAVEHDR *)calloc(self->num_chunks_1a, sizeof(SB_WAVEHDR));
    if (!self->stream_buf_1e || !self->stream_hdrs_22) {
        WavePlayer_StopAndClose_1010_36d0(g_DAT_1060_07cc_wavePlayer);
        return;
    }

    for (i = 0; i < self->num_chunks_1a; ++i) {
        SB_WAVEHDR *hdr = &self->stream_hdrs_22[i];
        hdr->lpData = self->stream_buf_1e + self->stream_pos_16;
        hdr->dwBufferLength = self->chunk_size_0e;
        if (self->stream_fill_cb_0a != NULL) {
            self->stream_fill_cb_0a((uint8_t *)hdr->lpData, self->stream_pos_16, self->chunk_size_0e, hwo);
        }
        self->stream_pos_16 += self->chunk_size_0e;
        if (sb_win16_waveOutPrepareHeader(hwo, hdr, sizeof(SB_WAVEHDR)) != 0 ||
            sb_win16_waveOutWrite(hwo, hdr, sizeof(SB_WAVEHDR)) != 0) {
            WavePlayer_StopAndClose_1010_36d0(g_DAT_1060_07cc_wavePlayer);
            return;
        }
        if (i == 0 && self->sample_rate_04 > 0) {
            uint32_t duration_ticks = (uint32_t)(((uint64_t)self->sample_frames_00 * 60u) / self->sample_rate_04);
            WavePlayer_SetEndTick_1010_3a10(
                g_DAT_1060_07cc_wavePlayer,
                Timer_GetTicks60Hz_1010_07ae() + duration_ticks);
        }
        if (on_chunk_cb != NULL) {
            on_chunk_cb(hwo);
        }
    }
}

/* Original: 1010:36a6 */
SBWavePlayer *WavePlayer_Ctor_1010_36a6(SBWavePlayer *self)
{
    if (self) {
        self->sample_00 = NULL;
    }
    return self;
}

/* Original: 1010:36d0 */
void WavePlayer_StopAndClose_1010_36d0(SBWavePlayer *self)
{
    if (!self || !self->sample_00) {
        return;
    }
    if (self->hWaveOut_2c != 0) {
        sb_win16_waveOutReset(self->hWaveOut_2c);
        if (self->is_streamed_2e == 0) {
            sb_win16_waveOutUnprepareHeader(self->hWaveOut_2c, &self->wave_hdr_0c, sizeof(SB_WAVEHDR));
        } else {
            BswStream_UnprepareAndFree_1010_32bc(self->sample_00);
            self->is_streamed_2e = 0;
        }
        sb_win16_waveOutClose(self->hWaveOut_2c);
        self->hWaveOut_2c = 0;
    }
    if (self->sample_00->pcm_buffer_08 != NULL) {
        BswSample_FreeBuffer_1010_3152(self->sample_00);
    }
    if (self->owns_sample_30 != 0) {
        FreeMem_1008_ef1e(self->sample_00);
    }
    self->sample_00 = NULL;
}

/* Original: 1010:37ae
 * Returns 1 if `sample_00 != NULL` and `Timer_GetTicks60Hz_1010_07ae() <= end_tick_04`, else 0.
 */
uint8_t WavePlayer_IsPlaying_1010_37ae(SBWavePlayer *self)
{
    uint32_t now_ticks;
    if (!self || self->sample_00 == NULL) {
        return 0;
    }
    now_ticks = Timer_GetTicks60Hz_1010_07ae();
    return (now_ticks <= self->end_tick_04) ? 1u : 0u;
}

/* Original: 1010:37e6 */
void WavePlayer_OpenAndWrite_1010_37e6(SBWavePlayer *self, void (*on_chunk_cb)(SB_HWAVEOUT))
{
    SB_PCMWAVEFORMAT fmt;
    SBBswSample *s;

    if (!self || !self->sample_00) {
        return;
    }
    s = self->sample_00;
    memset(&fmt, 0, sizeof(fmt));
    fmt.wFormatTag = 1; /* WAVE_FORMAT_PCM */
    fmt.nSamplesPerSec = s->sample_rate_04;

    if (s->format_code_06 == 1) {
        fmt.nChannels = 1;
        fmt.wBitsPerSample = 8;
        fmt.nBlockAlign = 1;
    } else if (s->format_code_06 == 2) {
        fmt.nChannels = 2;
        fmt.wBitsPerSample = 8;
        fmt.nBlockAlign = 2;
    } else if (s->format_code_06 == 3) {
        fmt.nChannels = 1;
        fmt.wBitsPerSample = 16;
        fmt.nBlockAlign = 2;
    } else if (s->format_code_06 == 4) {
        fmt.nChannels = 2;
        fmt.wBitsPerSample = 16;
        fmt.nBlockAlign = 4;
    } else {
        fmt.nChannels = 1;
        fmt.wBitsPerSample = 8;
        fmt.nBlockAlign = 1;
    }
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;

    if (sb_win16_waveOutOpen(&self->hWaveOut_2c, 0xFFFFu, &fmt, 0, 0, 0) == 0 &&
        self->hWaveOut_2c != 0) {
        if (self->is_streamed_2e != 0) {
            BswStream_StartChunked_1010_34ea(s, on_chunk_cb);
            return;
        }
        if (s->pcm_buffer_08 != NULL) {
            self->locked_pcm_08 = s->pcm_buffer_08;
            memset(&self->wave_hdr_0c, 0, sizeof(self->wave_hdr_0c));
            self->wave_hdr_0c.lpData = self->locked_pcm_08;
            self->wave_hdr_0c.dwBufferLength = s->sample_frames_00 * fmt.nBlockAlign;

            if (sb_win16_waveOutPrepareHeader(self->hWaveOut_2c, &self->wave_hdr_0c, sizeof(SB_WAVEHDR)) == 0 &&
                sb_win16_waveOutWrite(self->hWaveOut_2c, &self->wave_hdr_0c, sizeof(SB_WAVEHDR)) == 0) {
                uint32_t duration_ticks = (s->sample_rate_04 > 0)
                    ? (uint32_t)(((uint64_t)s->sample_frames_00 * 60u) / s->sample_rate_04)
                    : 0;
                self->end_tick_04 = Timer_GetTicks60Hz_1010_07ae() + duration_ticks;
                return;
            }
        }
    }
    WavePlayer_StopAndClose_1010_36d0(self);
}

/* Original: 1010:39d4 */
void WavePlayer_PlaySample_1010_39d4(SBWavePlayer *self,
                                     void (*on_chunk_cb)(SB_HWAVEOUT),
                                     uint16_t is_streamed,
                                     SBBswSample *sample)
{
    if (!self) {
        return;
    }
    WavePlayer_StopAndClose_1010_36d0(self);
    self->sample_00 = sample;
    self->is_streamed_2e = is_streamed;
    self->owns_sample_30 = 0;
    WavePlayer_OpenAndWrite_1010_37e6(self, on_chunk_cb);
}

/* Original: 1010:3a10 */
void WavePlayer_SetEndTick_1010_3a10(SBWavePlayer *self, uint32_t end_tick)
{
    if (self) {
        self->end_tick_04 = end_tick;
    }
}

/* Original: 1010:3a24 */
void WavePlayer_NoopCallback_1010_3a24(void)
{
}

/* ==========================================================================
 * 3. Global Sound Manager (`CSoundManager_1008_7edc` at `DAT_1060_07c8`)
 * ========================================================================== */

/* Original: 1008:21ec */
SBSoundManager *CSoundManager_Init_1008_21ec(SBSoundManager *self)
{
    SBWavePlayer *wp;
    if (!self) {
        return NULL;
    }
    self->vtable_tag_00 = 0x10087edcu;
    self->snd_handle_06 = NULL;
    wp = (SBWavePlayer *)AllocZeroed_1008_eed8(sizeof(SBWavePlayer));
    if (wp != NULL) {
        g_DAT_1060_07cc_wavePlayer = WavePlayer_Ctor_1010_36a6(wp);
    } else {
        g_DAT_1060_07cc_wavePlayer = NULL;
    }
    self->sample_0c = NULL;
    self->next_idle_tick_12 = Timer_GetTicks60Hz_1010_07ae() + 36000u;
    self->is_active_16 = 0;
    g_DAT_1060_07c8_soundMgr = self;
    return self;
}

/* Original: 1008:225a */
void CSoundManager_Dtor_1008_225a(SBSoundManager *self)
{
    if (!self) {
        return;
    }
    CSoundManager_Stop_1008_2462(self);
    if (g_DAT_1060_07cc_wavePlayer != NULL) {
        WavePlayer_StopAndClose_1010_36d0(g_DAT_1060_07cc_wavePlayer);
        FreeMem_1008_ef1e(g_DAT_1060_07cc_wavePlayer);
        g_DAT_1060_07cc_wavePlayer = NULL;
    }
    if (g_DAT_1060_07c8_soundMgr == self) {
        g_DAT_1060_07c8_soundMgr = NULL;
    }
    CObject_Dtor_1008_ef8e(self);
}

/* Original: 1008:22ac */
uint8_t CSoundManager_IsHardwareBusy_1008_22ac(SBSoundManager *self)
{
    (void)self;
    return WavePlayer_IsPlaying_1010_37ae(g_DAT_1060_07cc_wavePlayer);
}

/* Original: 1008:22ba */
uint8_t CSoundManager_IsActive_1008_22ba(const SBSoundManager *self)
{
    return self ? self->is_active_16 : 0;
}

/* Original: 1008:22c8 */
uint8_t CSoundManager_WasInterrupted_1008_22c8(const SBSoundManager *self)
{
    return self ? self->interrupted_0a : 0;
}

/* Original: 1008:22d6
 * Loads `'snd '` (`0x03ee`) resource by ID from `SB_CDRES.DLL`, parses its
 * `BSW\0` header, and plays it via `g_DAT_1060_07cc_wavePlayer`.
 */
void CSoundManager_PlaySoundResource_1008_22d6(SBSoundManager *self, uint16_t snd_res_id)
{
    SBResourceHandle *h;
    SBBswSample *sample;

    if (!self) {
        return;
    }
    self->is_active_16 = 1;
    h = ResMgr_GetResource_1010_10f2(0x03ee, "snd ", snd_res_id);
    if (h != NULL && h->data_00 != NULL) {
        self->current_snd_id_04 = snd_res_id;
        self->snd_handle_06 = h;
        self->saved_res_flags_10 = Handle_GetFlags_1010_0096(h);
        Handle_NoPurge_1010_015c(h);
        self->interrupted_0a = 0;

        sample = (SBBswSample *)AllocZeroed_1008_eed8(sizeof(SBBswSample));
        self->sample_1c = BswSample_Ctor_1010_3172(sample);
        BswSample_ParseHeaderAndCopyPcm_1010_31d8(self->sample_1c, h->data_00);
        WavePlayer_PlaySample_1010_39d4(g_DAT_1060_07cc_wavePlayer, NULL, 0, self->sample_1c);
        self->sample_0c = self->sample_1c;
    }
}

/* Original: 1008:23b2 */
void CSoundManager_WaitOrInterrupt_1008_23b2(SBSoundManager *self)
{
    if (!self) {
        return;
    }
    while (CSoundManager_IsHardwareBusy_1008_22ac(self) != 0) {
        /* In a non-interactive test or when no input message interrupts, advance clock */
        sb_win16_advanceTimeMs(20);
    }
    CSoundManager_Stop_1008_2462(self);
}

/* Original: 1008:2436 */
void CSoundManager_WaitUntilDone_1008_2436(SBSoundManager *self)
{
    if (!self) {
        return;
    }
    while (CSoundManager_IsHardwareBusy_1008_22ac(self) != 0) {
        sb_win16_advanceTimeMs(20);
    }
    CSoundManager_Stop_1008_2462(self);
}

/* Original: 1008:2462 */
void CSoundManager_Stop_1008_2462(SBSoundManager *self)
{
    if (!self) {
        return;
    }
    if (self->snd_handle_06 != NULL) {
        WavePlayer_StopAndClose_1010_36d0(g_DAT_1060_07cc_wavePlayer);
        if (self->sample_1c != NULL) {
            BswSample_FreeBuffer_1010_3152(self->sample_1c);
            FreeMem_1008_ef1e(self->sample_1c);
            self->sample_1c = NULL;
        }
        Handle_SetFlags_1010_00b0(self->snd_handle_06, self->saved_res_flags_10);
        self->snd_handle_06 = NULL;
    }
    self->sample_0c = NULL;
    self->is_active_16 = 0;
}

/* Original: 1008:24ec */
void CSoundManager_IdleTimerCheck_1008_24ec(SBSoundManager *self)
{
    if (!self) {
        return;
    }
    if (self->snd_handle_06 == NULL) {
        uint32_t now = Timer_GetTicks60Hz_1010_07ae();
        if (self->next_idle_tick_12 <= now) {
            self->next_idle_tick_12 = Timer_GetTicks60Hz_1010_07ae() + 36000u;
        }
    }
}

/* Original: 1008:2530 */
uint16_t CSoundManager_GetCurrentSoundId_1008_2530(const SBSoundManager *self)
{
    return self ? self->current_snd_id_04 : 0;
}

/* Original: 1008:6a28
 * Verifies Windows >= 3.10, waveOut device presence, asynchronous waveOut
 * capability (`(dwSupport & 0x10) == 0`), and 11.025 kHz 8-bit mono format
 * support (`(dwFormats & 1) != 0`).
 */
uint16_t System_CheckRequirements_1008_6a28(void)
{
    SB_WAVEOUTCAPS caps;
    if (sb_win16_waveOutGetNumDevs() == 0) {
        return 0;
    }
    if (sb_win16_waveOutGetDevCaps(0, &caps, sizeof(caps)) != 0) {
        return 0;
    }
    if ((caps.dwSupport & 0x10u) != 0) {
        return 0;
    }
    if ((caps.dwFormats & 0x01u) == 0) {
        return 0;
    }
    return 1;
}

/* ==========================================================================
 * 4. Timed Callback & Sound Sequence Channel (`CTimedCallback_1008_823c` & `CSoundChannel_1010_bdd4`)
 * ========================================================================== */

/* Original: 1018:63b2 */
uint8_t CTimedCallback_GetByte0a_1018_63b2(const SBSoundChannel *self)
{
    return self ? self->flag_0a : 0;
}

/* Original: 1018:63c0 */
uint8_t CTimedCallback_GetByte09_1018_63c0(const SBSoundChannel *self)
{
    return self ? self->flag_09 : 0;
}

/* Original: 1018:63ce */
uint8_t CTimedCallback_IsPaused_1018_63ce(const SBSoundChannel *self)
{
    return self ? self->is_paused_08 : 0;
}

/* Original: 1018:63dc */
void CTimedCallback_Resume_1018_63dc(SBSoundChannel *self)
{
    if (self) {
        self->is_paused_08 = 0;
    }
}

/* Original: 1018:63ec */
void CTimedCallback_Pause_1018_63ec(SBSoundChannel *self)
{
    if (self) {
        self->is_paused_08 = 1;
    }
}

/* Original: 1018:63fc */
void CTimedCallback_OnStartNoop_1018_63fc(void)
{
}

/* Original: 1018:6400 */
void CTimedCallback_OnTickNoop_1018_6400(void)
{
}

/* Original: 1018:6404 */
void CTimedCallback_Execute_1018_6404(SBSoundChannel *self, uint32_t *out_interval)
{
    if (!self) {
        return;
    }
    if (out_interval) {
        *out_interval = self->interval_ticks_04;
    }
    CTimedCallback_ScheduleNext_1018_646a(self);
}

/* Original: 1018:642c */
uint8_t CTimedCallback_IsReady_1018_642c(SBSoundChannel *self)
{
    if (!self) {
        return 0;
    }
    if (CTimedCallback_IsPaused_1018_63ce(self) == 0) {
        if (self->next_base_tick_0c <= Timer_GetTicks60Hz_1010_07ae()) {
            return 1;
        }
    }
    return 0;
}

/* Original: 1018:646a */
void CTimedCallback_ScheduleNext_1018_646a(SBSoundChannel *self)
{
    if (self) {
        self->next_base_tick_0c = Timer_GetTicks60Hz_1010_07ae() + self->interval_ticks_04;
    }
}

/* Original: 1018:648e */
uint8_t CTimedCallback_False0_1018_648e(void)
{
    return 0;
}

/* Original: 1018:6494 */
uint8_t CTimedCallback_False1_1018_6494(void)
{
    return 0;
}

void CSoundChannel_Init(SBSoundChannel *self)
{
    if (!self) {
        return;
    }
    memset(self, 0, sizeof(*self));
    self->vtable_tag_00 = 0x1010bdd4u;
    self->primary_snd_id_20 = -1;
    self->current_index_14 = -1;
}

/* Original: 1020:27d8 */
void CSoundChannel_Dtor_1020_27d8(SBSoundChannel *self)
{
    if (!self) {
        return;
    }
    CSoundChannel_ClearSounds_1020_2a96(self);
    CSoundChannel_SetPrimarySound_1020_29b6(self, -1);
    CObject_Dtor_1008_ef8e(self);
}

/* Original: 1020:2810 */
uint16_t CSoundChannel_ExecuteNext_1020_2810(SBSoundChannel *self, uint32_t *out_interval)
{
    uint32_t now;
    uint16_t chosen_snd_id = 0;

    if (!self) {
        return 0;
    }
    CTimedCallback_Execute_1018_6404(self, out_interval);
    now = Timer_GetTicks60Hz_1010_07ae();

    if (now < self->next_fire_tick_16 || self->sound_count_22 < 1) {
        if (self->primary_snd_id_20 == -1) {
            return 0;
        }
        chosen_snd_id = (uint16_t)self->primary_snd_id_20;
    } else {
        int16_t delay;
        if (self->randomize_1a == 0) {
            self->current_index_14++;
            if (self->current_index_14 >= self->sound_count_22) {
                self->current_index_14 = 0;
            }
        } else {
            self->current_index_14 = Random_Range_1010_24f8(0, (int16_t)(self->sound_count_22 - 1));
        }
        chosen_snd_id = self->sound_ids_24[self->current_index_14];
        delay = Random_Range_1010_24f8(self->min_delay_1c, self->max_delay_1e);
        self->next_fire_tick_16 = Timer_GetTicks60Hz_1010_07ae() + (uint32_t)(delay > 0 ? delay : 0);
    }

    if (chosen_snd_id != 0 && g_DAT_1060_07c8_soundMgr != NULL) {
        CSoundManager_PlaySoundResource_1008_22d6(g_DAT_1060_07c8_soundMgr, chosen_snd_id);
    }
    return chosen_snd_id;
}

/* Original: 1020:28e0 */
uint8_t CSoundChannel_IsReady_1020_28e0(SBSoundChannel *self)
{
    if (!self) {
        return 0;
    }
    if (CTimedCallback_IsReady_1018_642c(self) != 0) {
        if (g_DAT_1060_07c8_soundMgr == NULL ||
            CSoundManager_IsActive_1008_22ba(g_DAT_1060_07c8_soundMgr) == 0) {
            return 1;
        }
    }
    return 0;
}

/* Original: 1020:2922 */
uint8_t CSoundChannel_True_1020_2922(void)
{
    return 1;
}

/* Original: 1020:2928 */
void CSoundChannel_AddSound_1020_2928(SBSoundChannel *self, uint16_t snd_res_id)
{
    SBResourceHandle *h;
    int8_t flags;

    if (!self || self->sound_count_22 >= SB_SOUND_CHANNEL_MAX_SOUNDS) {
        return;
    }
    self->sound_ids_24[self->sound_count_22++] = snd_res_id;
    h = ResMgr_GetResource_1010_10f2(0x03ee, "snd ", snd_res_id);
    flags = (int8_t)Handle_GetFlags_1010_0096(h);
    if (flags > 0) {
        if (g_DAT_1060_07f5_preloadFlag != 0) {
            Handle_NoPurge_1010_015c(h);
            Handle_Unlock_1010_0122(h);
            Handle_MoveNeck_1010_0608(h);
            Handle_AllowPurge_1010_0196(h);
        }
        Handle_Lock_1010_00e8(h);
    }
}

/* Original: 1020:29b6 */
void CSoundChannel_SetPrimarySound_1020_29b6(SBSoundChannel *self, int16_t snd_res_id)
{
    if (!self) {
        return;
    }
    if (self->primary_snd_id_20 > 0) {
        SBResourceHandle *old_h;
        if (g_DAT_1060_07c8_soundMgr != NULL &&
            CSoundManager_GetCurrentSoundId_1008_2530(g_DAT_1060_07c8_soundMgr) == (uint16_t)self->primary_snd_id_20) {
            CSoundManager_Stop_1008_2462(g_DAT_1060_07c8_soundMgr);
        }
        old_h = ResMgr_GetResource_1010_10f2(0x03ee, "snd ", (uint16_t)self->primary_snd_id_20);
        Handle_Unlock_1010_0122(old_h);
    }
    self->primary_snd_id_20 = snd_res_id;
    if (snd_res_id > 0) {
        SBResourceHandle *new_h = ResMgr_GetResource_1010_10f2(0x03ee, "snd ", (uint16_t)snd_res_id);
        int8_t flags = (int8_t)Handle_GetFlags_1010_0096(new_h);
        if (flags > 0) {
            if (g_DAT_1060_07f5_preloadFlag != 0) {
                Handle_NoPurge_1010_015c(new_h);
                Handle_Unlock_1010_0122(new_h);
                Handle_MoveNeck_1010_0608(new_h);
                Handle_AllowPurge_1010_0196(new_h);
            }
            Handle_Lock_1010_00e8(new_h);
        }
    }
}

/* Original: 1020:2a96 */
void CSoundChannel_ClearSounds_1020_2a96(SBSoundChannel *self)
{
    int16_t i;
    if (!self) {
        return;
    }
    for (i = 0; i < self->sound_count_22; ++i) {
        uint16_t sid = self->sound_ids_24[i];
        SBResourceHandle *h;
        if (g_DAT_1060_07c8_soundMgr != NULL &&
            CSoundManager_GetCurrentSoundId_1008_2530(g_DAT_1060_07c8_soundMgr) == sid) {
            CSoundManager_Stop_1008_2462(g_DAT_1060_07c8_soundMgr);
        }
        h = ResMgr_GetResource_1010_10f2(0x03ee, "snd ", sid);
        Handle_Unlock_1010_0122(h);
    }
    self->sound_count_22 = 0;
}
