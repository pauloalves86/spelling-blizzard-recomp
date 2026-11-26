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
cmake -S . -B build/cmake && cmake --build build/cmake -j
build/cmake/sb_recomp                     # window at 2x, with sound
build/cmake/sb_recomp --scale 3 --no-audio
```

Options:
- `--game-dir DIR`: where `BLIZZARD.EXE` and `SB_CDRES.DLL` live (default `original`).
- `--data-dir DIR`: writable directory for the INI, player and high-score files (default `run`).
- `--headless`, `--quit-after MS`, `--snapshot FILE.ppm`, `--trace-api`.
- `--script FILE`: timed input, one event per line, `MS command args`:
  `click X Y`, `rclick X Y`, `move X Y`, `key NAME`, `down NAME`, `up NAME`,
  `text STRING`, `snap FILE.ppm`, `quit`.
- `SB_FONT_DIR`: directory of the Liberation TTFs (default `/usr/share/fonts/liberation`).

## Pipeline

| Piece | File | Notes |
| --- | --- | --- |
| NE parser | `tools/recomp/ne.py` | Segments, every relocation type, entries, module refs. |
| Decoder | `tools/recomp/x86.py` | 8086–286 plus the 386 operand-size ops the compiler emits. Every reachable instruction decodes; there is no FPU code. |
| Generator | `tools/recomp/gen.py` | One C function per original function (1,489 total: Ghidra's list in `tools/recomp/functions.txt`, plus far-call targets, plus function pointers found next to selector relocations, such as the window procedure `1010:5db2`). Relocations are resolved at generation time. Switch tables come from the `cmp ax,N; ja; add ax,ax; xchg ax,bx; jmp cs:[bx+T]` pattern. A `jmp far` through memory to an address that isn't a function is a hand-made return (CRT `_setargv`). |
| Runtime | `src/recomp/runtime.c`, `cpu.h` | Registers, flags, selector→linear memory, and a global heap where blocks over 64 KB are tiled across consecutive selectors (`__AHINCR` = 8). Selectors equal Ghidra's segment values. |
| Loader | `src/recomp/loader.c` | Segments, relocations, PSP and environment. Task registers follow Wine's `InitTask`. |
| KERNEL / DOS | `src/recomp/kernel.c`, `hostfs.c` | Memory, resources (EXE and `SB_CDRES.DLL`), files, INI profiles, INT 21h. DOS paths under `C:\SB\` map into the data dir, case-insensitive. |
| USER | `src/recomp/user.c` | Windows, message queue, painting, keyboard and mouse, software cursor, `wsprintf`, presentation. |
| GDI | `src/recomp/gdi.c` | 8-bit palette display. Static colors are at 0–9 and 246–255, `RealizePalette` uses slot assignment, and `AnimatePalette` changes PC_RESERVED entries in place. ROP3 blits follow the Windows mono↔color rules. Regions are pixel masks. Text uses Liberation fonts without antialiasing. |
| MMSYSTEM | `src/recomp/mmsystem.c` | `waveOut` over the SDL audio queue. Buffers finish (`WHDR_DONE`, callbacks) when they have actually played. |

Ghidra mis-sizes some API argument lists, so each implementation hard-codes
the documented Win16 prototype, checked against the pushes at the call
sites.

## Status

Checked headlessly (`tests/recomp/new_game.script`, ctest
`recomp_new_game_headless`), plus 5 minutes of random input with no fault:

- Credits screen, palette fade to the map, speech, NEW GAME, typing a name
  (saved to `MAGIC/PLAYERS/<NAME>.SBG`), PLAY, and the river level with the
  animated water texture, Spelling Demons, word hints, the raft and the walker.

When the game reaches something not implemented it stops with
`recomp fatal: …` and a BP-chain backtrace of original addresses. Missing
function entries can be added to `tools/recomp/extra_entries.txt` (`SSSS:OOOO`).

## Known gaps

- `LoadCursor` by string name is not supported (the game hasn't used it).
- Window non-client areas (caption, frame) aren't drawn. The game uses popup and child windows only.
- Stretch modes are always nearest-neighbour.
