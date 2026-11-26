#include "win16_waveout.h"

#include <string.h>

static uint32_t g_shim_time_ms = 1000u;
static uint16_t g_waveout_open_count = 0;
static uint32_t g_total_bytes_written = 0;
static uint32_t g_last_sample_rate = 0;
static SB_WaveOutSink g_sink;
static int g_has_sink = 0;
static uint32_t (*g_time_source)(void) = NULL;

void sb_win16_waveOut_setSink(const SB_WaveOutSink *sink)
{
    if (sink) {
        g_sink = *sink;
        g_has_sink = 1;
    } else {
        memset(&g_sink, 0, sizeof(g_sink));
        g_has_sink = 0;
    }
}

void sb_win16_setTimeSource(uint32_t (*now_ms)(void))
{
    g_time_source = now_ms;
}

uint32_t sb_win16_timeGetTime(void)
{
    if (g_time_source) {
        return g_time_source();
    }
    return g_shim_time_ms;
}

void sb_win16_setTimeMs(uint32_t ms)
{
    g_shim_time_ms = ms;
}

void sb_win16_advanceTimeMs(uint32_t delta_ms)
{
    g_shim_time_ms += delta_ms;
}

uint16_t sb_win16_waveOutGetNumDevs(void)
{
    return 1;
}

uint16_t sb_win16_waveOutGetDevCaps(uint16_t uDeviceID, SB_WAVEOUTCAPS *lpCaps, uint16_t cbCaps)
{
    (void)uDeviceID;
    (void)cbCaps;
    if (!lpCaps) {
        return 1;
    }
    memset(lpCaps, 0, sizeof(*lpCaps));
    lpCaps->wMid = 1;
    lpCaps->wPid = 1;
    lpCaps->vDriverVersion = 0x030a;
    strncpy(lpCaps->szPname, "Spelling Blizzard WaveOut Shim", sizeof(lpCaps->szPname) - 1);
    lpCaps->dwFormats = 0x00000001u; /* WAVE_FORMAT_1M08 (11.025 kHz, 8-bit, mono) */
    lpCaps->wChannels = 1;
    lpCaps->dwSupport = 0;           /* Asynchronous waveOut supported (bit 0x10 == 0) */
    return 0;
}

uint16_t sb_win16_waveOutOpen(SB_HWAVEOUT *lphWaveOut,
                              uint16_t uDeviceID,
                              const SB_PCMWAVEFORMAT *lpFormat,
                              uintptr_t dwCallback,
                              uintptr_t dwInstance,
                              uint32_t dwFlags)
{
    (void)uDeviceID;
    (void)dwCallback;
    (void)dwInstance;
    (void)dwFlags;
    if (!lphWaveOut || !lpFormat) {
        return 1;
    }
    g_waveout_open_count++;
    *lphWaveOut = (SB_HWAVEOUT)g_waveout_open_count;
    g_last_sample_rate = lpFormat->nSamplesPerSec;
    if (g_has_sink && g_sink.open) {
        g_sink.open(g_sink.ctx, lpFormat);
    }
    return 0;
}

uint16_t sb_win16_waveOutPrepareHeader(SB_HWAVEOUT hWaveOut, SB_WAVEHDR *lpWaveOutHdr, uint16_t uSize)
{
    (void)uSize;
    if (hWaveOut == 0 || !lpWaveOutHdr) {
        return 1;
    }
    lpWaveOutHdr->dwFlags |= SB_WHDR_PREPARED;
    return 0;
}

uint16_t sb_win16_waveOutUnprepareHeader(SB_HWAVEOUT hWaveOut, SB_WAVEHDR *lpWaveOutHdr, uint16_t uSize)
{
    (void)uSize;
    if (hWaveOut == 0 || !lpWaveOutHdr) {
        return 1;
    }
    lpWaveOutHdr->dwFlags &= ~SB_WHDR_PREPARED;
    return 0;
}

uint16_t sb_win16_waveOutWrite(SB_HWAVEOUT hWaveOut, SB_WAVEHDR *lpWaveOutHdr, uint16_t uSize)
{
    (void)uSize;
    if (hWaveOut == 0 || !lpWaveOutHdr || !lpWaveOutHdr->lpData) {
        return 1;
    }
    g_total_bytes_written += lpWaveOutHdr->dwBufferLength;
    if (g_has_sink && g_sink.write) {
        g_sink.write(g_sink.ctx, lpWaveOutHdr->lpData, lpWaveOutHdr->dwBufferLength);
    }
    lpWaveOutHdr->dwFlags |= SB_WHDR_DONE;
    return 0;
}

uint16_t sb_win16_waveOutReset(SB_HWAVEOUT hWaveOut)
{
    if (hWaveOut == 0) {
        return 1;
    }
    if (g_has_sink && g_sink.reset) {
        g_sink.reset(g_sink.ctx);
    }
    return 0;
}

uint16_t sb_win16_waveOutClose(SB_HWAVEOUT hWaveOut)
{
    if (hWaveOut == 0) {
        return 1;
    }
    return 0;
}

uint32_t sb_win16_waveOut_getTotalBytesWritten(void)
{
    return g_total_bytes_written;
}

uint32_t sb_win16_waveOut_getLastSampleRate(void)
{
    return g_last_sample_rate;
}

void sb_win16_waveOut_resetStats(void)
{
    g_total_bytes_written = 0;
    g_last_sample_rate = 0;
}
