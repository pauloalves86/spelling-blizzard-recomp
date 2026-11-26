/* Host MMSYSTEM waveOut for the recompiled game (SDL2 audio queue).
 *
 * waveOutWrite queues the buffer's PCM to SDL. A buffer becomes WHDR_DONE when
 * the device has actually consumed it (tracked by queued-byte accounting), and
 * then the game's callback (window message or function) is delivered from the
 * game's own polling (PeekMessage / timeGetTime), mirroring Win16 behaviour.
 * Without an audio device, playback progress is simulated from the clock.
 */
#include <SDL.h>
#include <stdio.h>
#include <string.h>

#include "api.h"
#include "win16.h"

#define WHDR_DONE 0x01u
#define WHDR_PREPARED 0x02u
#define WHDR_INQUEUE 0x10u
#define MM_WOM_OPEN 0x3BBu
#define MM_WOM_CLOSE 0x3BCu
#define MM_WOM_DONE 0x3BDu
#define HWAVEOUT_ID 0x0777u

void api_USER_POSTMESSAGE(void);

typedef struct Pending {
    uint32_t hdr; /* far pointer to WAVEHDR */
    uint64_t end; /* cumulative byte position when fully played */
} Pending;

static struct {
    int enabled;
    int open;
    SDL_AudioDeviceID dev;
    SDL_AudioSpec spec;
    uint32_t rate, block;
    uint32_t cb, cb_inst, flags;
    uint64_t written;     /* bytes queued since open */
    uint64_t base_played; /* for simulated playback */
    uint32_t sim_t0;
    Pending pend[256];
    int npend;
    int in_poll;
} M;

void mm_init(int enable_audio) { memset(&M, 0, sizeof(M)); M.enabled = enable_audio; }
void mm_shutdown(void)
{
    if (M.dev) SDL_CloseAudioDevice(M.dev);
    M.dev = 0;
}

static uint64_t played_bytes(void)
{
    if (M.dev) {
        uint32_t q = SDL_GetQueuedAudioSize(M.dev);
        return M.written > q ? M.written - q : 0;
    }
    /* simulated device */
    {
        uint64_t p = (uint64_t)(SDL_GetTicks() - M.sim_t0) * M.rate * M.block / 1000u;
        return p > M.written ? M.written : p;
    }
}

static void notify(uint16_t msg, uint32_t p1)
{
    uint32_t type = M.flags & 0x70000u;
    if (type == 0x10000u) { /* CALLBACK_WINDOW */
        PUSH16((uint16_t)M.cb);
        PUSH16(msg);
        PUSH16(HWAVEOUT_ID);
        PUSH32(p1);
        PUSH16(0); PUSH16(0); /* fake return address */
        api_USER_POSTMESSAGE();
    } else if (type == 0x30000u && M.cb) { /* CALLBACK_FUNCTION: void FAR PASCAL f(hwo, msg, inst, p1, p2) */
        uint16_t sax = AX, sbx = BX, scx = CX, sdx = DX;
        PUSH16(HWAVEOUT_ID);
        PUSH16(msg);
        PUSH32(M.cb_inst);
        PUSH32(p1);
        PUSH32(0);
        recomp_callback((uint16_t)(M.cb >> 16), (uint16_t)M.cb);
        AX = sax; BX = sbx; CX = scx; DX = sdx;
    }
}

static void complete(int k)
{
    uint32_t hdr = M.pend[k].hdr;
    uint8_t *h = (uint8_t *)farp(hdr);
    uint32_t fl;
    memcpy(&fl, h + 16, 4);
    fl = (fl & ~WHDR_INQUEUE) | WHDR_DONE;
    memcpy(h + 16, &fl, 4);
    memmove(&M.pend[k], &M.pend[k + 1], sizeof(Pending) * (size_t)(M.npend - k - 1));
    M.npend--;
    notify(MM_WOM_DONE, hdr);
}

void mm_poll(void)
{
    uint64_t p;
    if (!M.open || M.in_poll) return;
    M.in_poll = 1;
    p = played_bytes();
    while (M.npend > 0 && M.pend[0].end <= p) {
        complete(0);
    }
    M.in_poll = 0;
}

void api_MMSYSTEM_WAVEOUTGETNUMDEVS(void) { AX = 1; api_ret(0); }

void api_MMSYSTEM_WAVEOUTGETDEVCAPS(void)
{
    ArgReader a = args_pascal(8);
    (void)arg_w(&a);
    uint32_t lp = arg_d(&a);
    uint16_t cb = arg_w(&a);
    uint8_t caps[52];
    uint16_t w;
    uint32_t d;
    memset(caps, 0, sizeof(caps));
    w = 1; memcpy(caps, &w, 2);          /* wMid */
    w = 2; memcpy(caps + 2, &w, 2);      /* wPid */
    w = 0x0300; memcpy(caps + 4, &w, 2); /* vDriverVersion */
    snprintf((char *)caps + 6, 32, "SDL Wave Out");
    d = 0x0FFF; memcpy(caps + 38, &d, 4); /* dwFormats: all 11/22/44 kHz formats */
    w = 2; memcpy(caps + 42, &w, 2);      /* wChannels */
    d = 0x0C; memcpy(caps + 44, &d, 4);   /* dwSupport: VOLUME|LRVOLUME, async */
    if (lp) memcpy(farp(lp), caps, cb < 48 ? cb : 48);
    AX = 0;
    api_ret(8);
}

void api_MMSYSTEM_WAVEOUTOPEN(void)
{
    ArgReader a = args_pascal(22);
    uint32_t lph = arg_d(&a);
    (void)arg_w(&a);
    uint32_t lpfmt = arg_d(&a);
    uint32_t cb = arg_d(&a), inst = arg_d(&a), flags = arg_d(&a);
    const uint8_t *f = (const uint8_t *)farp(lpfmt);
    uint16_t tag, ch, align, bits = 8;
    uint32_t rate;
    memcpy(&tag, f, 2);
    memcpy(&ch, f + 2, 2);
    memcpy(&rate, f + 4, 4);
    memcpy(&align, f + 12, 2);
    if (tag == 1) memcpy(&bits, f + 14, 2);
    api_ret(22);
    if (tag != 1) { AX = 32; return; } /* WAVERR_BADFORMAT */
    if (flags & 1) { AX = 0; return; } /* WAVE_FORMAT_QUERY */
    if (M.open) { AX = 4; return; }    /* MMSYSERR_ALLOCATED */
    M.open = 1;
    M.rate = rate;
    M.block = align ? align : (uint32_t)(ch * bits / 8);
    M.cb = cb;
    M.cb_inst = inst;
    M.flags = flags;
    M.written = 0;
    M.npend = 0;
    M.sim_t0 = SDL_GetTicks();
    if (M.enabled) {
        SDL_AudioSpec want;
        if (!M.dev || M.spec.freq != (int)rate || M.spec.channels != ch ||
            M.spec.format != (bits == 16 ? AUDIO_S16LSB : AUDIO_U8)) {
            if (M.dev) SDL_CloseAudioDevice(M.dev);
            SDL_zero(want);
            want.freq = (int)rate;
            want.format = bits == 16 ? AUDIO_S16LSB : AUDIO_U8;
            want.channels = (Uint8)ch;
            want.samples = 512;
            M.dev = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
            if (!M.dev) {
                fprintf(stderr, "[recomp] SDL_OpenAudioDevice: %s (continuing silently)\n", SDL_GetError());
                M.enabled = 0;
            } else {
                M.spec = want;
                SDL_PauseAudioDevice(M.dev, 0);
            }
        } else {
            SDL_ClearQueuedAudio(M.dev);
        }
    }
    api_log("waveOutOpen(%u Hz, %u ch, %u bit, cb %08x flags %08x)", rate, ch, bits, cb, flags);
    if (lph) WW((uint16_t)(lph >> 16), (uint16_t)lph, HWAVEOUT_ID);
    notify(MM_WOM_OPEN, 0);
    AX = 0;
}

void api_MMSYSTEM_WAVEOUTPREPAREHEADER(void)
{
    ArgReader a = args_pascal(8);
    (void)arg_w(&a);
    uint8_t *h = (uint8_t *)farp(arg_d(&a));
    uint32_t fl;
    memcpy(&fl, h + 16, 4);
    fl |= WHDR_PREPARED;
    memcpy(h + 16, &fl, 4);
    AX = 0;
    api_ret(8);
}

void api_MMSYSTEM_WAVEOUTUNPREPAREHEADER(void)
{
    ArgReader a = args_pascal(8);
    (void)arg_w(&a);
    uint8_t *h = (uint8_t *)farp(arg_d(&a));
    uint32_t fl;
    memcpy(&fl, h + 16, 4);
    if (fl & WHDR_INQUEUE) {
        AX = 33; /* WAVERR_STILLPLAYING */
    } else {
        fl &= ~WHDR_PREPARED;
        memcpy(h + 16, &fl, 4);
        AX = 0;
    }
    api_ret(8);
}

void api_MMSYSTEM_WAVEOUTWRITE(void)
{
    ArgReader a = args_pascal(8);
    (void)arg_w(&a);
    uint32_t hdr = arg_d(&a);
    uint8_t *h = (uint8_t *)farp(hdr);
    uint32_t data, len, fl;
    api_ret(8);
    memcpy(&data, h, 4);
    memcpy(&len, h + 4, 4);
    memcpy(&fl, h + 16, 4);
    if (!M.open || M.npend >= 256) { AX = 5; return; }
    fl = (fl & ~WHDR_DONE) | WHDR_INQUEUE;
    memcpy(h + 16, &fl, 4);
    if (M.dev && len) {
        SDL_QueueAudio(M.dev, farp(data), len); /* huge buffers are contiguous in MEM */
    }
    M.written += len;
    api_log("waveOutWrite(%u bytes)", len);
    M.pend[M.npend].hdr = hdr;
    M.pend[M.npend].end = M.written;
    M.npend++;
    AX = 0;
}

void api_MMSYSTEM_WAVEOUTRESET(void)
{
    ArgReader a = args_pascal(2);
    (void)arg_w(&a);
    api_ret(2);
    if (M.dev) SDL_ClearQueuedAudio(M.dev);
    while (M.npend > 0) {
        complete(0);
    }
    M.written = 0;
    M.sim_t0 = SDL_GetTicks();
    AX = 0;
}

void api_MMSYSTEM_WAVEOUTCLOSE(void)
{
    ArgReader a = args_pascal(2);
    (void)arg_w(&a);
    api_ret(2);
    if (M.npend) { AX = 33; return; }
    notify(MM_WOM_CLOSE, 0);
    M.open = 0;
    AX = 0;
}
