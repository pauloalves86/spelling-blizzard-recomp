# Static recompilation (`sb_recomp`)

`sb_recomp` runs the original game: every function of `original/BLIZZARD.EXE`
is translated mechanically from x86-16 to C and runs on a host Win16 layer
built on SDL2 and FreeType. Game logic, rendering, sound sequencing and input
handling are the original code, not a re-implementation.

It is the reference for the Path 2 goal. Hand-written clean C for any
function can be checked against, and later swapped in for, the recompiled
`f_SSSS_OOOO` with the same address.

## Build and run

```sh
git submodule update --init               # third_party/tinycc
cmake -S . -B build/cmake && cmake --build build/cmake -j
build/cmake/sb_recomp                     # window at 2x, with sound
build/cmake/sb_recomp --scale 3 --no-audio
```

Two programs run the same recompiled code:

- `sb_recomp` recompiles `BLIZZARD.EXE` when it starts (`src/recomp/jit.c`):
  the recompiler library generates the C in memory (about 40 ms), and
  TinyCC's libtcc compiles it into executable memory (about 60 ms) as one
  translation unit. The source starts with a prelude built into the program
  (`jit_prelude.h`, `cpu.h` and `imports/api_decls.h`) in place of system
  headers; `#include` lines are dropped. The code can only reach the symbols
  `jit.c` hands to libtcc: the runtime's CPU state and helpers, `memcpy`,
  and every imported API. This program contains no game code and builds
  without the game.
- `sb_recomp_aot` (development, built when the game files are present) links
  the same C generated into `build/.../recomp_gen/` and compiled by the host
  compiler (`src/recomp/aot.c`), so a debugger sees every function.

Under a fixed clock (`LD_PRELOAD` over `SDL_GetTicks`, `SDL_Delay` and
`time`) both produce the same frames, saves and API call trace. Without
it, frames from different builds differ slightly, because the game's
animation follows real time.

Options:
- `--prepare --game=PATH [--data DIR]`: read the player's copy (a folder, one
  of its files, an ISO, or a raw BIN/IMG through its CUE; `gamefiles.c`),
  check it against the known SHA-256, keep it in `DIR/game`, recompile it
  once, and print `progress <0..1> <text>` lines. Without `--game-dir` or
  `--data-dir`, playing uses that copy, saves in `DIR/saves`, and copies
  stderr into `DIR/logs` (`player.c`). Exit codes: 64 not set up, 65 copy
  missing or wrong, 67 recompiling failed, 70 a crash (something not
  supported yet).
- `--self-test`: compile a check of the CPU helpers in memory and compare it
  with the same code compiled by the host compiler (`jit_selftest.h`).
- `--game-dir DIR`: where `BLIZZARD.EXE` and `SB_CDRES.DLL` live (default `original`).
- `--data-dir DIR`: writable directory for the INI, player and high-score files (default `run`).
- `--headless`, `--quit-after MS`, `--snapshot FILE.ppm`, `--trace-api`.
- `--script FILE`: timed input, one event per line, `MS command args`:
  `click X Y`, `rclick X Y`, `move X Y`, `key NAME`, `down NAME`, `up NAME`,
  `text STRING`, `snap FILE.ppm`, `quit`.
- `SB_FONT_DIR`: directory of the Liberation TTFs (otherwise `fonts/` next
  to the program, then the usual system folders).

## Pipeline

| Piece | File | Notes |
| --- | --- | --- |
| NE parser | `src/recomp/gen/ne.c` | Segments, every relocation type, entries, module refs. |
| Decoder | `src/recomp/gen/x86.c` | 8086–286 plus the 386 operand-size ops the compiler emits. Every reachable instruction decodes; there is no FPU code. |
| Generator | `src/recomp/gen/gen.c` | One C function per original function (1,489 total: Ghidra's list in `src/recomp/gen/functions.txt`, plus far-call targets, plus function pointers found next to selector relocations, such as the window procedure `1010:5db2`). Relocations are resolved at generation time. Switch tables come from the `cmp ax,N; ja; add ax,ax; xchg ax,bx; jmp cs:[bx+T]` pattern. A `jmp far` through memory to an address that isn't a function is a hand-made return (CRT `_setargv`). |
| Runtime | `src/recomp/runtime.c`, `cpu.h`, `jit.c`, `aot.c` | Registers, flags, selector→linear memory, and a global heap where blocks over 64 KB are tiled across consecutive selectors (`__AHINCR` = 8). Selectors equal Ghidra's segment values. |
| Loader | `src/recomp/loader.c` | Segments, relocations, PSP and environment. Task registers follow Wine's `InitTask`. |
| KERNEL / DOS | `src/recomp/kernel.c`, `hostfs.c` | Memory, resources (EXE and `SB_CDRES.DLL`), files, INI profiles, INT 21h. DOS paths under `C:\SB\` map into the data dir, case-insensitive. |
| USER | `src/recomp/user.c` | Windows, message queue, painting, keyboard and mouse, software cursor, `wsprintf`, presentation. |
| GDI | `src/recomp/gdi.c` | 8-bit palette display. Static colors are at 0–9 and 246–255, `RealizePalette` uses slot assignment, and `AnimatePalette` changes PC_RESERVED entries in place. ROP3 blits follow the Windows mono↔color rules. Regions are pixel masks. Text uses Liberation fonts without antialiasing. |
| MMSYSTEM | `src/recomp/mmsystem.c` | `waveOut` over the SDL audio queue. Buffers finish (`WHDR_DONE`, callbacks) when they have actually played. |

The recompiler is a C library (`gen_recompile()` in `src/recomp/gen/gen.h`)
that returns the generated files as text, in about 40 ms. CMake builds and
runs its command-line front end, `sb_gen`, while configuring:

```sh
sb_gen OUT_DIR --exe original/BLIZZARD.EXE [--imports src/recomp/imports [--update-imports]]
```

API names come from `src/recomp/gen/exports.c` (ordinal, name and argument
bytes for KERNEL, GDI, USER and MMSYSTEM, from Ghidra's export files).

The import table (`src/recomp/imports/`: API names, ordinals and thunk
indices, no game code) is checked in, so the runtime compiles without the
game. `sb_gen --imports` fails if it no longer matches the EXE, and
`--update-imports` rewrites it. The program entry point is called through the
generated `recomp_start()`.

Ghidra mis-sizes some API argument lists, so each implementation hard-codes
the documented Win16 prototype, checked against the pushes at the call
sites.

## Status

Checked headlessly (`tests/recomp/new_game.script`, ctest
`sb_recomp_new_game_headless` and `sb_recomp_aot_new_game_headless`), plus 5 minutes of random input with no fault:

- Credits screen, palette fade to the map, speech, NEW GAME, typing a name
  (saved to `MAGIC/PLAYERS/<NAME>.SBG`), PLAY, and the river level with the
  animated water texture, Spelling Demons, word hints, the raft and the walker.

When the game reaches something not implemented it stops with
`recomp fatal: …` and a BP-chain backtrace of original addresses. Missing
function entries can be added to `src/recomp/gen/functions.txt` (`SSSS:OOOO`).

## Known gaps

- `LoadCursor` by string name is not supported (the game hasn't used it).
- Window non-client areas (caption, frame) aren't drawn. The game uses popup and child windows only.
- Stretch modes are always nearest-neighbour.
