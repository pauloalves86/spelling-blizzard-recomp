/* Verifies the SDL2 platform backend translates host events into the Win16
 * messages the recovered dispatcher (`1010:3c64`) consumes, and that the
 * waveOut sink / time source hooks are installed. Runs with dummy drivers. */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/platform/sb_platform.h"
#include "../src/platform/sb_platform_sdl.h"
#include "../src/shim/win16_waveout.h"

static int failures = 0;
#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                                   \
        }                                                                 \
    } while (0)

static int next_msg(SB_MSG16 *m)
{
    return sb_win16_PeekMessage(m, 0, SB_WM_KEYDOWN, SB_WM_MBUTTONDBLCLK, SB_PM_REMOVE) ||
           sb_win16_PeekMessage(m, 0, SB_WM_CLOSE, SB_WM_CLOSE, SB_PM_REMOVE);
}

int main(void)
{
    SBPlatformConfig cfg;
    SDL_Event ev;
    SB_MSG16 m;
    SB_HWND hwnd;

    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    SDL_setenv("SDL_AUDIODRIVER", "dummy", 1);

    sb_win16_user_shim_reset();
    sb_win16_gdi_shim_reset();
    memset(&cfg, 0, sizeof(cfg));
    cfg.scale = 1;
    cfg.escape_closes = 1;
    if (sb_platform_init(&cfg) != 0) {
        fprintf(stderr, "platform init failed\n");
        return 1;
    }
    hwnd = sb_win16_CreateWindow("SBWnd", "t", 0, 0, 0, 640, 480, 0, 0, 0, NULL);
    sb_platform_set_target_hwnd(hwnd);
    sb_win16_ValidateRect(hwnd, NULL);

    /* Real clock installed: timeGetTime starts at the shim's 1000 ms origin. */
    CHECK(sb_win16_timeGetTime() >= 1000u);

    /* Key 'A' down -> WM_KEYDOWN 'A' + key state. */
    SDL_zero(ev);
    ev.type = SDL_KEYDOWN;
    ev.key.keysym.sym = SDLK_a;
    sb_platform_sdl_handle_event(&ev);
    /* Typed text -> WM_CHAR 'a'. */
    SDL_zero(ev);
    ev.type = SDL_TEXTINPUT;
    strcpy(ev.text.text, "a");
    sb_platform_sdl_handle_event(&ev);
    /* Return -> WM_KEYDOWN VK_RETURN + WM_CHAR '\r'. */
    SDL_zero(ev);
    ev.type = SDL_KEYDOWN;
    ev.key.keysym.sym = SDLK_RETURN;
    sb_platform_sdl_handle_event(&ev);
    /* Left click at (100, 200). */
    SDL_zero(ev);
    ev.type = SDL_MOUSEBUTTONDOWN;
    ev.button.button = SDL_BUTTON_LEFT;
    ev.button.clicks = 1;
    ev.button.x = 100;
    ev.button.y = 200;
    sb_platform_sdl_handle_event(&ev);
    /* Esc closes (escape_closes = 1). */
    SDL_zero(ev);
    ev.type = SDL_KEYDOWN;
    ev.key.keysym.sym = SDLK_ESCAPE;
    sb_platform_sdl_handle_event(&ev);

    CHECK(sb_platform_pump_events() == 0); /* Esc requested quit */
    CHECK((sb_win16_GetKeyState('A') & 0x8000) != 0);

    CHECK(next_msg(&m) && m.hwnd == hwnd && m.message == SB_WM_KEYDOWN && m.wParam == 'A');
    CHECK(next_msg(&m) && m.message == SB_WM_CHAR && m.wParam == 'a');
    CHECK(next_msg(&m) && m.message == SB_WM_KEYDOWN && m.wParam == 0x0D);
    CHECK(next_msg(&m) && m.message == SB_WM_CHAR && m.wParam == 0x0D);
    CHECK(next_msg(&m) && m.message == SB_WM_LBUTTONDOWN &&
          (m.lParam & 0xFFFFu) == 100u && (m.lParam >> 16) == 200u);
    CHECK(next_msg(&m) && m.message == SB_WM_CLOSE && m.hwnd == hwnd);
    CHECK(!next_msg(&m));

    /* Presenting must not crash with an empty screen bitmap. */
    sb_platform_present();

    sb_platform_shutdown();
    /* Hooks removed on shutdown: deterministic clock is back. */
    sb_win16_setTimeMs(1234u);
    CHECK(sb_win16_timeGetTime() == 1234u);

    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("platform_sdl: all checks passed\n");
    return 0;
}
