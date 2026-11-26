/* SDL2 implementation of the host platform backend (see sb_platform.h). */
#include "sb_platform.h"
#include "sb_platform_sdl.h"

#include <SDL.h>
#include <stdio.h>
#include <string.h>

#include "../shim/win16_gdi.h"
#include "../shim/win16_waveout.h"

#define SB_SCREEN_W 640
#define SB_SCREEN_H 480

/* Win16 virtual-key codes */
#define VK_BACK    0x08
#define VK_TAB     0x09
#define VK_RETURN  0x0D
#define VK_SHIFT   0x10
#define VK_CONTROL 0x11
#define VK_MENU    0x12
#define VK_PAUSE   0x13
#define VK_CAPITAL 0x14
#define VK_ESCAPE  0x1B
#define VK_SPACE   0x20
#define VK_PRIOR   0x21
#define VK_NEXT    0x22
#define VK_END     0x23
#define VK_HOME    0x24
#define VK_LEFT    0x25
#define VK_UP      0x26
#define VK_RIGHT   0x27
#define VK_DOWN    0x28
#define VK_INSERT  0x2D
#define VK_DELETE  0x2E
#define VK_F1      0x70

#define MK_LBUTTON 0x0001
#define MK_RBUTTON 0x0002
#define MK_SHIFT   0x0004
#define MK_CONTROL 0x0008
#define MK_MBUTTON 0x0010

typedef struct SBPlatform {
    int initialized;
    SBPlatformConfig cfg;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    uint32_t argb[SB_SCREEN_W * SB_SCREEN_H];
    SB_HWND target_hwnd;
    uint32_t start_ticks;
    int quit_posted;

    SDL_AudioDeviceID audio_dev;
    SDL_AudioSpec audio_spec;     /* currently opened format */
} SBPlatform;

static SBPlatform g_plat;

/* ------------------------------------------------------------------ time */

static uint32_t platform_time_source(void)
{
    /* Offset by 1000 ms to match the deterministic shim default origin. */
    return 1000u + (SDL_GetTicks() - g_plat.start_ticks);
}

uint32_t sb_platform_ticks_ms(void)
{
    return SDL_GetTicks() - g_plat.start_ticks;
}

void sb_platform_sleep_ms(uint32_t ms)
{
    SDL_Delay(ms);
}

/* ----------------------------------------------------------------- audio */

static SDL_AudioFormat sdl_format_for(const SB_PCMWAVEFORMAT *fmt)
{
    return (fmt->wBitsPerSample == 16) ? AUDIO_S16LSB : AUDIO_U8;
}

static void sink_open(void *ctx, const SB_PCMWAVEFORMAT *fmt)
{
    SDL_AudioSpec want;
    SDL_AudioSpec have;
    (void)ctx;
    if (!fmt || !g_plat.cfg.enable_audio) {
        return;
    }
    if (g_plat.audio_dev != 0 &&
        g_plat.audio_spec.freq == (int)fmt->nSamplesPerSec &&
        g_plat.audio_spec.channels == fmt->nChannels &&
        g_plat.audio_spec.format == sdl_format_for(fmt)) {
        return; /* same format: keep the device open */
    }
    if (g_plat.audio_dev != 0) {
        SDL_CloseAudioDevice(g_plat.audio_dev);
        g_plat.audio_dev = 0;
    }
    SDL_zero(want);
    want.freq = (int)fmt->nSamplesPerSec;
    want.format = sdl_format_for(fmt);
    want.channels = (Uint8)fmt->nChannels;
    want.samples = 1024;
    want.callback = NULL; /* push model via SDL_QueueAudio */
    /* No allowed changes: SDL converts the 11,025 Hz stream internally. */
    g_plat.audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (g_plat.audio_dev == 0) {
        fprintf(stderr, "[platform] SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
        return;
    }
    g_plat.audio_spec = want;
    SDL_PauseAudioDevice(g_plat.audio_dev, 0);
}

static void sink_write(void *ctx, const void *data, uint32_t len)
{
    (void)ctx;
    if (g_plat.audio_dev != 0 && data && len > 0) {
        SDL_QueueAudio(g_plat.audio_dev, data, len);
    }
}

static void sink_reset(void *ctx)
{
    (void)ctx;
    if (g_plat.audio_dev != 0) {
        SDL_ClearQueuedAudio(g_plat.audio_dev);
    }
}

uint32_t sb_platform_audio_queued_bytes(void)
{
    return g_plat.audio_dev ? SDL_GetQueuedAudioSize(g_plat.audio_dev) : 0;
}

/* ------------------------------------------------------------- lifecycle */

int sb_platform_init(const SBPlatformConfig *cfg)
{
    Uint32 flags = SDL_INIT_VIDEO | SDL_INIT_TIMER;
    SB_WaveOutSink sink;

    memset(&g_plat, 0, sizeof(g_plat));
    if (cfg) {
        g_plat.cfg = *cfg;
    }
    if (!g_plat.cfg.title) {
        g_plat.cfg.title = "Spelling Blizzard";
    }
    if (g_plat.cfg.scale < 1) {
        g_plat.cfg.scale = 1;
    }
    if (g_plat.cfg.enable_audio) {
        flags |= SDL_INIT_AUDIO;
    }
    if (SDL_Init(flags) != 0) {
        fprintf(stderr, "[platform] SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }
    if (g_plat.cfg.enable_audio && SDL_WasInit(SDL_INIT_AUDIO) == 0) {
        g_plat.cfg.enable_audio = 0;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    g_plat.window = SDL_CreateWindow(g_plat.cfg.title,
                                     SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                     SB_SCREEN_W * g_plat.cfg.scale,
                                     SB_SCREEN_H * g_plat.cfg.scale,
                                     SDL_WINDOW_RESIZABLE);
    if (!g_plat.window) {
        fprintf(stderr, "[platform] SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }
    g_plat.renderer = SDL_CreateRenderer(g_plat.window, -1, SDL_RENDERER_PRESENTVSYNC);
    if (!g_plat.renderer) {
        g_plat.renderer = SDL_CreateRenderer(g_plat.window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!g_plat.renderer) {
        fprintf(stderr, "[platform] SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(g_plat.window);
        SDL_Quit();
        return -1;
    }
    /* Logical size: letterboxed scaling, and mouse events in game coords. */
    SDL_RenderSetLogicalSize(g_plat.renderer, SB_SCREEN_W, SB_SCREEN_H);
    g_plat.texture = SDL_CreateTexture(g_plat.renderer, SDL_PIXELFORMAT_ARGB8888,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       SB_SCREEN_W, SB_SCREEN_H);
    if (!g_plat.texture) {
        fprintf(stderr, "[platform] SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(g_plat.renderer);
        SDL_DestroyWindow(g_plat.window);
        SDL_Quit();
        return -1;
    }
    SDL_StartTextInput();

    g_plat.start_ticks = SDL_GetTicks();
    sb_win16_setTimeSource(platform_time_source);

    memset(&sink, 0, sizeof(sink));
    sink.open = sink_open;
    sink.write = sink_write;
    sink.reset = sink_reset;
    sb_win16_waveOut_setSink(&sink);

    g_plat.initialized = 1;
    return 0;
}

/* Writes the last presented frame (exactly what the window showed) as PPM.
 * Enabled with SB_PLATFORM_SNAPSHOT=<path>; used for headless verification. */
static void write_snapshot(const char *path)
{
    FILE *fp = fopen(path, "wb");
    int i;
    if (!fp) {
        return;
    }
    fprintf(fp, "P6\n%d %d\n255\n", SB_SCREEN_W, SB_SCREEN_H);
    for (i = 0; i < SB_SCREEN_W * SB_SCREEN_H; ++i) {
        uint8_t rgb[3];
        rgb[0] = (uint8_t)(g_plat.argb[i] >> 16);
        rgb[1] = (uint8_t)(g_plat.argb[i] >> 8);
        rgb[2] = (uint8_t)g_plat.argb[i];
        fwrite(rgb, 1, 3, fp);
    }
    fclose(fp);
}

void sb_platform_shutdown(void)
{
    const char *snap;
    if (!g_plat.initialized) {
        return;
    }
    snap = SDL_getenv("SB_PLATFORM_SNAPSHOT");
    if (snap && *snap) {
        write_snapshot(snap);
    }
    sb_win16_waveOut_setSink(NULL);
    sb_win16_setTimeSource(NULL);
    if (g_plat.audio_dev) {
        SDL_CloseAudioDevice(g_plat.audio_dev);
    }
    SDL_StopTextInput();
    SDL_DestroyTexture(g_plat.texture);
    SDL_DestroyRenderer(g_plat.renderer);
    SDL_DestroyWindow(g_plat.window);
    SDL_Quit();
    memset(&g_plat, 0, sizeof(g_plat));
}

void sb_platform_set_target_hwnd(SB_HWND hwnd)
{
    g_plat.target_hwnd = hwnd;
}

/* ----------------------------------------------------------------- input */

static int vk_from_sdl(SDL_Keycode k)
{
    if (k >= SDLK_a && k <= SDLK_z) {
        return 'A' + (int)(k - SDLK_a);
    }
    if (k >= SDLK_0 && k <= SDLK_9) {
        return '0' + (int)(k - SDLK_0);
    }
    if (k >= SDLK_F1 && k <= SDLK_F12) {
        return VK_F1 + (int)(k - SDLK_F1);
    }
    switch (k) {
        case SDLK_BACKSPACE: return VK_BACK;
        case SDLK_TAB:       return VK_TAB;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:  return VK_RETURN;
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:    return VK_SHIFT;
        case SDLK_LCTRL:
        case SDLK_RCTRL:     return VK_CONTROL;
        case SDLK_LALT:
        case SDLK_RALT:      return VK_MENU;
        case SDLK_PAUSE:     return VK_PAUSE;
        case SDLK_CAPSLOCK:  return VK_CAPITAL;
        case SDLK_ESCAPE:    return VK_ESCAPE;
        case SDLK_SPACE:     return VK_SPACE;
        case SDLK_PAGEUP:    return VK_PRIOR;
        case SDLK_PAGEDOWN:  return VK_NEXT;
        case SDLK_END:       return VK_END;
        case SDLK_HOME:      return VK_HOME;
        case SDLK_LEFT:      return VK_LEFT;
        case SDLK_UP:        return VK_UP;
        case SDLK_RIGHT:     return VK_RIGHT;
        case SDLK_DOWN:      return VK_DOWN;
        case SDLK_INSERT:    return VK_INSERT;
        case SDLK_DELETE:    return VK_DELETE;
        default:             return 0;
    }
}

static uint16_t mouse_key_flags(void)
{
    uint16_t f = 0;
    Uint32 b = SDL_GetMouseState(NULL, NULL);
    SDL_Keymod m = SDL_GetModState();
    if (b & SDL_BUTTON_LMASK) f |= MK_LBUTTON;
    if (b & SDL_BUTTON_RMASK) f |= MK_RBUTTON;
    if (b & SDL_BUTTON_MMASK) f |= MK_MBUTTON;
    if (m & KMOD_SHIFT) f |= MK_SHIFT;
    if (m & KMOD_CTRL) f |= MK_CONTROL;
    return f;
}

static uint32_t make_lparam_xy(int x, int y)
{
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= SB_SCREEN_W) x = SB_SCREEN_W - 1;
    if (y >= SB_SCREEN_H) y = SB_SCREEN_H - 1;
    return ((uint32_t)(uint16_t)y << 16) | (uint16_t)x;
}

static void post_close(void)
{
    if (!g_plat.quit_posted) {
        sb_win16_PostMessage(g_plat.target_hwnd, SB_WM_CLOSE, 0, 0);
        g_plat.quit_posted = 1;
    }
}

void sb_platform_sdl_handle_event(const SDL_Event *e)
{
    const SDL_Event ev = *e;
    SB_HWND hwnd = g_plat.target_hwnd;
    {
        switch (ev.type) {
            case SDL_QUIT:
                post_close();
                break;
            case SDL_WINDOWEVENT:
                if (ev.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                    sb_win16_PostMessage(hwnd, SB_WM_ACTIVATEAPP, 1, 0);
                } else if (ev.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                    sb_win16_PostMessage(hwnd, SB_WM_ACTIVATEAPP, 0, 0);
                } else if (ev.window.event == SDL_WINDOWEVENT_EXPOSED) {
                    sb_win16_InvalidateRect(hwnd, NULL, 0);
                }
                break;
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                int vk = vk_from_sdl(ev.key.keysym.sym);
                int down = (ev.type == SDL_KEYDOWN);
                /* lParam: repeat count 1, scancode, bit 30 = previous state, bit 31 = transition */
                uint32_t lp = 1u | ((uint32_t)(ev.key.keysym.scancode & 0xFF) << 16);
                if (ev.key.repeat) lp |= 1u << 30;
                if (!down) lp |= (1u << 30) | (1u << 31);
                if (vk == 0) {
                    break;
                }
                sb_win16_SetKeyState(vk, down ? (int16_t)0x8000 : 0);
                if (down && vk == VK_ESCAPE && g_plat.cfg.escape_closes) {
                    post_close();
                    break;
                }
                sb_win16_PostMessage(hwnd, down ? SB_WM_KEYDOWN : SB_WM_KEYUP, (uint16_t)vk, lp);
                /* TranslateMessage would emit WM_CHAR for these control keys;
                 * printable characters come from SDL_TEXTINPUT below. */
                if (down && (vk == VK_BACK || vk == VK_RETURN || vk == VK_ESCAPE || vk == VK_TAB)) {
                    sb_win16_PostMessage(hwnd, SB_WM_CHAR, (uint16_t)vk, lp);
                }
                break;
            }
            case SDL_TEXTINPUT: {
                const unsigned char *t = (const unsigned char *)ev.text.text;
                for (; *t; ++t) {
                    if (*t >= 0x20 && *t < 0x7F) { /* ASCII only, like the 1994 game */
                        sb_win16_PostMessage(hwnd, SB_WM_CHAR, *t, 1u);
                    }
                }
                break;
            }
            case SDL_MOUSEMOTION:
                sb_win16_PostMessage(hwnd, SB_WM_MOUSEMOVE, mouse_key_flags(),
                                     make_lparam_xy(ev.motion.x, ev.motion.y));
                break;
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                int down = (ev.type == SDL_MOUSEBUTTONDOWN);
                uint16_t msg;
                if (ev.button.button == SDL_BUTTON_LEFT) {
                    msg = down ? (ev.button.clicks >= 2 ? SB_WM_LBUTTONDBLCLK : SB_WM_LBUTTONDOWN)
                               : SB_WM_LBUTTONUP;
                } else if (ev.button.button == SDL_BUTTON_RIGHT) {
                    msg = down ? SB_WM_RBUTTONDOWN : SB_WM_RBUTTONUP;
                } else if (ev.button.button == SDL_BUTTON_MIDDLE) {
                    msg = down ? SB_WM_MBUTTONDOWN : SB_WM_MBUTTONUP;
                } else {
                    break;
                }
                sb_win16_PostMessage(hwnd, msg, mouse_key_flags(),
                                     make_lparam_xy(ev.button.x, ev.button.y));
                break;
            }
            default:
                break;
        }
    }
}

int sb_platform_pump_events(void)
{
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        sb_platform_sdl_handle_event(&ev);
    }
    return g_plat.quit_posted ? 0 : 1;
}

/* --------------------------------------------------------------- display */

void sb_platform_present(void)
{
    int16_t w = 0;
    int16_t h = 0;
    const uint8_t *pixels;
    const SB_PALETTEENTRY *pal;
    uint32_t lut[256];
    int x;
    int y;
    int i;

    if (!g_plat.initialized) {
        return;
    }
    pixels = sb_win16_gdi_getBitmapPixels(sb_win16_user_getScreenBitmap(), &w, &h);
    pal = sb_win16_gdi_getActivePaletteEntries();
    if (pixels && pal && w > 0 && h > 0) {
        int cw = w < SB_SCREEN_W ? w : SB_SCREEN_W;
        int ch = h < SB_SCREEN_H ? h : SB_SCREEN_H;
        for (i = 0; i < 256; ++i) {
            lut[i] = 0xFF000000u | ((uint32_t)pal[i].peRed << 16) |
                     ((uint32_t)pal[i].peGreen << 8) | pal[i].peBlue;
        }
        for (y = 0; y < ch; ++y) {
            const uint8_t *src = pixels + (size_t)y * (size_t)w;
            uint32_t *dst = g_plat.argb + (size_t)y * SB_SCREEN_W;
            for (x = 0; x < cw; ++x) {
                dst[x] = lut[src[x]];
            }
        }
        SDL_UpdateTexture(g_plat.texture, NULL, g_plat.argb, SB_SCREEN_W * (int)sizeof(uint32_t));
    }
    SDL_SetRenderDrawColor(g_plat.renderer, 0, 0, 0, 255);
    SDL_RenderClear(g_plat.renderer);
    SDL_RenderCopy(g_plat.renderer, g_plat.texture, NULL, NULL);
    SDL_RenderPresent(g_plat.renderer);
}
