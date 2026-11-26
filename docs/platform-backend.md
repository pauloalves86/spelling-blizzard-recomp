# Host platform backend (SDL2)

The Win16 API shim (`src/shim/`) stays headless and deterministic: `timeGetTime` is a
counter, `waveOutWrite` only counts bytes, and the "screen" is a 640x480 8-bit GDI
bitmap. The platform backend (`src/platform/`) is the one layer below the shim that
talks to a real OS. The recovered code never includes it; only the runner does.

```text
recovered game + framework (src/recovered/)
        |  Win16 calls (GDI / USER / MMSYSTEM)
        v
thin Win16 shim (src/shim/)  <-- hooks: SB_WaveOutSink, time source
        ^                         screen bitmap + active palette
        |  WM_* messages, GetKeyState
platform backend (src/platform/sb_platform_sdl.c, SDL2)
```

| Concern | How it is bridged |
| --- | --- |
| Display | `sb_platform_present` reads the shim screen bitmap (`sb_win16_user_getScreenBitmap`), maps it through the active GDI palette (so `AnimatePalette` fades and cycles show up), and draws it scaled with nearest-neighbour filtering and letterboxing. |
| Input | SDL key events become `WM_KEYDOWN`/`WM_KEYUP` with Win16 VK codes. Text input becomes `WM_CHAR` (ASCII only). Backspace, Enter, Esc and Tab also get a `WM_CHAR`, as `TranslateMessage` does. Mouse events become `WM_MOUSEMOVE`/`WM_xBUTTONDOWN`/`UP`/`DBLCLK` in 640x480 game coordinates with `MK_*` flags. `GetKeyState` is kept up to date. Closing the window posts `WM_CLOSE`, which goes through `CWindow_OnClose_1010_5d20` to `CApplication_RequestQuit_1008_7bf2`. Focus changes post `WM_ACTIVATEAPP` (palette save/restore). |
| Audio | `sb_win16_waveOut_setSink` routes `waveOutOpen`/`waveOutWrite`/`waveOutReset` to an SDL audio device in push mode (`SDL_QueueAudio`). The 11,025 Hz unsigned 8-bit mono BSW PCM is played as-is; SDL resamples. |
| Time | `sb_win16_setTimeSource` makes `timeGetTime` follow the real clock, starting at the shim's 1000 ms origin. Everything 60 Hz (`CTimer60Hz_1010_456a`, `WavePlayer_IsPlaying_1010_37ae`) follows from that. |

## Running

```sh
cmake -S . -B build/cmake && cmake --build build/cmake -j
build/cmake/spelling_blizzard --window            # 2x scale, live audio; Esc or close to quit
build/cmake/spelling_blizzard --window --scale 1 --no-audio
```

Headless verification uses SDL's dummy drivers. If you set `SB_PLATFORM_SNAPSHOT=<file.ppm>`,
the last presented frame (what the window showed) is written out at shutdown:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy SB_PLATFORM_SNAPSHOT=/tmp/w.ppm \
  build/cmake/spelling_blizzard --window --frames 30
```

The backend is optional (`-DSB_WITH_SDL2=OFF`, or SDL2 is not found). Without it, the
runner and all shim/recovered tests build exactly as before.

## Tests

- `platform_sdl_input` checks that SDL events become the right Win16 messages,
  that key state is updated, that Esc and window close post `WM_CLOSE`, and
  that the hooks are removed on shutdown.
- `spelling_blizzard_window_headless` runs the windowed message loop for 30 frames.

## What it does not do yet

The window shows what the recovered code renders: the board scene composed
from `PICT` #129, the masked HUD and the shimmer. Input reaches the recovered
dispatcher (`1010:3c64`). But the recovered `CEventTarget_HandleKey_1008_b74e`
and `HandleCommand` only forward along the `next_handler` chain. No
game-specific virtual overrides (main menu, game controller view, river match)
are wired in yet, so keys and clicks don't change the game state. The large
flat-green areas in the frame are regions the original fills at run time
(actors, letter blocks, board panels) that the recovered rendering doesn't
draw yet. Both are game-logic and view gaps above the shim, not backend issues.
