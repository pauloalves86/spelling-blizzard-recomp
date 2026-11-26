/* Thin Win16 API Shim — MMSYSTEM WaveOut & Multimedia Timer (`TIMEGETTIME`)
 *
 * Implements the Win16 MMSYSTEM APIs imported by original/BLIZZARD.EXE:
 *   - waveOutGetNumDevs / waveOutGetDevCaps
 *   - waveOutOpen / waveOutClose / waveOutReset
 *   - waveOutPrepareHeader / waveOutUnprepareHeader / waveOutWrite
 *   - timeGetTime (with deterministic clock advancement support for testing)
 */
#ifndef SB_SHIM_WIN16_WAVEOUT_H
#define SB_SHIM_WIN16_WAVEOUT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t SB_HWAVEOUT;

/* Standard Win16 WAVEHDR flags */
#define SB_WHDR_DONE      0x00000001u
#define SB_WHDR_PREPARED  0x00000002u
#define SB_WHDR_BEGINLOOP 0x00000004u
#define SB_WHDR_ENDLOOP   0x00000008u
#define SB_WHDR_INQUEUE   0x00000010u

/* Win16 PCMWAVEFORMAT structure (16 bytes, constructed on stack in 1010:37e6) */
typedef struct SB_PCMWAVEFORMAT {
    uint16_t wFormatTag;      /* +0x00: WAVE_FORMAT_PCM = 1 */
    uint16_t nChannels;       /* +0x02: 1 (mono) or 2 (stereo) */
    uint32_t nSamplesPerSec;  /* +0x04: 11025 or 22050 */
    uint32_t nAvgBytesPerSec; /* +0x08: nSamplesPerSec * nBlockAlign */
    uint16_t nBlockAlign;     /* +0x0c: 1, 2, or 4 */
    uint16_t wBitsPerSample;  /* +0x0e: 8 or 16 */
} SB_PCMWAVEFORMAT;

/* Win16 WAVEHDR structure (32 bytes / 0x20, embedded at +0x0c in SBWavePlayer) */
typedef struct SB_WAVEHDR {
    const void *lpData;       /* +0x00: PCM buffer pointer */
    uint32_t dwBufferLength;  /* +0x04: byte length */
    uint32_t dwBytesRecorded; /* +0x08 */
    uint32_t dwUser;          /* +0x0c */
    uint32_t dwFlags;         /* +0x10: SB_WHDR_* flags */
    uint32_t dwLoops;         /* +0x14 */
    struct SB_WAVEHDR *lpNext;/* +0x18 */
    uint32_t reserved;        /* +0x1c */
} SB_WAVEHDR;

/* Win16 WAVEOUTCAPS subset checked in 1008:6a28 */
typedef struct SB_WAVEOUTCAPS {
    uint16_t wMid;
    uint16_t wPid;
    uint16_t vDriverVersion;
    char szPname[32];
    uint32_t dwFormats;       /* bit 0 (WAVE_FORMAT_1M08 = 1): 11.025 kHz mono 8-bit */
    uint16_t wChannels;
    uint32_t dwSupport;       /* bit 4 (WAVECAPS_SYNC = 0x10): must be 0 for async */
} SB_WAVEOUTCAPS;

/* Multimedia Timer API */
uint32_t sb_win16_timeGetTime(void);
void sb_win16_setTimeMs(uint32_t ms);
void sb_win16_advanceTimeMs(uint32_t delta_ms);

/* MMSYSTEM waveOut APIs */
uint16_t sb_win16_waveOutGetNumDevs(void);
uint16_t sb_win16_waveOutGetDevCaps(uint16_t uDeviceID, SB_WAVEOUTCAPS *lpCaps, uint16_t cbCaps);
uint16_t sb_win16_waveOutOpen(SB_HWAVEOUT *lphWaveOut,
                              uint16_t uDeviceID,
                              const SB_PCMWAVEFORMAT *lpFormat,
                              uintptr_t dwCallback,
                              uintptr_t dwInstance,
                              uint32_t dwFlags);
uint16_t sb_win16_waveOutPrepareHeader(SB_HWAVEOUT hWaveOut, SB_WAVEHDR *lpWaveOutHdr, uint16_t uSize);
uint16_t sb_win16_waveOutUnprepareHeader(SB_HWAVEOUT hWaveOut, SB_WAVEHDR *lpWaveOutHdr, uint16_t uSize);
uint16_t sb_win16_waveOutWrite(SB_HWAVEOUT hWaveOut, SB_WAVEHDR *lpWaveOutHdr, uint16_t uSize);
uint16_t sb_win16_waveOutReset(SB_HWAVEOUT hWaveOut);
uint16_t sb_win16_waveOutClose(SB_HWAVEOUT hWaveOut);

/* Diagnostic counters for testing audio playback */
uint32_t sb_win16_waveOut_getTotalBytesWritten(void);
uint32_t sb_win16_waveOut_getLastSampleRate(void);
void sb_win16_waveOut_resetStats(void);

#ifdef __cplusplus
}
#endif

#endif /* SB_SHIM_WIN16_WAVEOUT_H */
