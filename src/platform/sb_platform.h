/* Host platform backend for the Thin Win16 API Shim.
 *
 * This is the only layer that touches a real OS window, audio device and
 * clock. It sits *below* the shim and feeds it:
 *   - Display: converts the shim's 640x480 8-bit screen bitmap through the
 *     active GDI palette and presents it in a host window.
 *   - Input:   translates host keyboard/mouse/close events into Win16
 *     messages (`WM_KEYDOWN`, `WM_CHAR`, `WM_LBUTTONDOWN`, `WM_CLOSE`, ...)
 *     posted to the game's main HWND, and maintains `GetKeyState`.
 *   - Audio:   installs an `SB_WaveOutSink` so `waveOutWrite` PCM reaches a
 *     real output device.
 *   - Time:    installs a real-clock `timeGetTime` source.
 *
 * The recovered game code never includes this header; only the runner does.
 */
#ifndef SB_PLATFORM_H
#define SB_PLATFORM_H

#include <stdint.h>
#include "../shim/win16_user.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SBPlatformConfig {
    const char *title;
    int scale;            /* integer window scale (1 = 640x480) */
    int enable_audio;     /* 0 = no audio device (PCM still counted by shim) */
    int escape_closes;    /* 1 = Esc posts WM_CLOSE to the main window */
} SBPlatformConfig;

/* Returns 0 on success. Installs the shim time source and waveOut sink. */
int sb_platform_init(const SBPlatformConfig *cfg);
void sb_platform_shutdown(void);

/* HWND that receives translated input messages. */
void sb_platform_set_target_hwnd(SB_HWND hwnd);

/* Drains host events into the Win16 message queue. Returns 0 once the host
 * asked to quit (a WM_CLOSE has then been posted to the target HWND). */
int sb_platform_pump_events(void);

/* Converts the shim screen bitmap through the active palette and presents. */
void sb_platform_present(void);

/* Milliseconds since sb_platform_init (same clock as timeGetTime). */
uint32_t sb_platform_ticks_ms(void);
void sb_platform_sleep_ms(uint32_t ms);

/* Bytes still queued on the audio device (0 when audio is disabled). */
uint32_t sb_platform_audio_queued_bytes(void);

#ifdef __cplusplus
}
#endif

#endif /* SB_PLATFORM_H */
