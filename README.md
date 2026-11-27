# Spelling Blizzard Recomp

A project to preserve and decompile **Spelling Blizzard** (Bright Star
Technology, 1994), a Windows 3.x educational spelling game, so it can run on
modern systems without an emulator.

The game is a 16-bit Windows NE executable (`BLIZZARD.EXE`, version
`1.10.CD for Windows`). It loads its art, animation, levels and speech from a
CD resource library (`SB_CDRES.DLL`).

## What's here

There are two ways the original program is being brought back:

1. **Static recompilation (`sb_recomp`): plays the real game today.**
   `tools/recomp/gen.py` translates every function of `BLIZZARD.EXE` from
   x86-16 machine code into C, mechanically. That generated code runs on a
   small host implementation of the Win16 APIs the game uses (KERNEL, USER,
   GDI, MMSYSTEM), built on SDL2 and FreeType. Game logic, drawing, sound and
   input handling are the original code. See [docs/recomp.md](docs/recomp.md).
2. **Clean decompilation (the end goal).** Readable, hand-written C that
   replaces the recompiled functions one by one, checked against the
   recompiled reference at the same address. Earlier analysis notes and
   hand-written reconstruction were removed from the tree; they're kept in
   git history at the `pre-cleanup` tag.

## You need the original game files

The game files are **not** in this repository or its releases, and neither
is any code built from them: the game is recompiled on your own machine. Put
your own copies here (or let `./build.sh /path/to/game` copy them):

```
original/BLIZZARD.EXE    229,472 bytes
original/SB_CDRES.DLL    9,808,128 bytes
```

Check that they're the expected versions:

```sh
sha256sum original/BLIZZARD.EXE original/SB_CDRES.DLL
# ddf0a4c1ff13bcf4460c6061771f930a8f1f519ba24a8ff331b648f5f3f3dce2  original/BLIZZARD.EXE
# 1c6c36bde51b8c9ad3b3a0eaba0283d0468d81326b3dbe4dbfbf7305863b70e1  original/SB_CDRES.DLL
```

The build reads these files but never modifies them.

## Requirements

- A C11 compiler (GCC or Clang), CMake 3.16 or newer, and `pkg-config`
- Python 3, which runs the recompiler at configure time
- SDL2 development files (SDL2, or sdl2-compat over SDL3)
- FreeType development files
- Liberation fonts, which stand in for the Windows fonts the game asks for

For example:

```sh
# Arch Linux
sudo pacman -S base-devel cmake python sdl2 freetype2 ttf-liberation
# Debian / Ubuntu
sudo apt install build-essential cmake pkg-config python3 libsdl2-dev libfreetype-dev fonts-liberation
```

## Build and run

Download the source package from
[Releases](https://github.com/pauloalves86/spelling-blizzard-recomp/releases)
(or clone the repository), install the requirements above, then:

```sh
./build.sh /path/to/your/game     # copies the two files into original/, checks them, builds
build/release/sb_recomp
```

`build.sh` finds the files regardless of case. Run it without an argument if
they're already in `original/`. Run `sb_recomp` from the repository directory,
because the game and data directories are relative to it.

To build by hand instead:

```sh
cmake -S . -B build/cmake
cmake --build build/cmake -j
build/cmake/sb_recomp
```

Without the game files, CMake builds only the host runtime (that's what CI
does).

This opens a 1280×960 window (640×480 at 2×) with sound. Close the window to
quit. Saved players, high scores and the game's INI file go in `run/`.

Useful options:

| Option | Meaning |
| --- | --- |
| `--scale N` | Window scale factor (default 2). |
| `--no-audio` | Don't open an audio device. |
| `--game-dir DIR` | Where `BLIZZARD.EXE` and `SB_CDRES.DLL` are (default `original`). |
| `--data-dir DIR` | Writable directory for saves and settings (default `run`). |
| `--headless` | Run without a window, for automated tests. |
| `--quit-after MS` | Exit after this many milliseconds. |
| `--snapshot FILE.ppm` | Save the last frame as an image on exit. |
| `--script FILE` | Play timed input from a file (see [docs/recomp.md](docs/recomp.md)). |
| `--trace-api` | Log every Windows API call to stderr. |

If the fonts are somewhere other than `/usr/share/fonts/liberation`, set
`SB_FONT_DIR` to the directory that holds the `Liberation*.ttf` files.

## Tests

```sh
ctest --test-dir build/cmake --output-on-failure
```

`recomp_new_game_headless` plays through the opening of the real game without
a window: credits, the map, New Game, typing a player name, and starting the
first river level. It fails if the game hits anything the host layer doesn't
implement.

## Status

Working: the credits and map screens with correct palette colors and fonts,
speech and sound effects, mouse and keyboard input, creating a player, and
playing a river level (animated water, the Spelling Demons, word hints, the
raft).

Not yet checked: full play through many levels, and every screen. If the game
reaches an API or DOS call that isn't implemented, it stops with
`recomp fatal: …` and a backtrace of original addresses. Please report these.
It also uses noticeably more CPU than it should while idle.

## Repository layout

| Path | Contents |
| --- | --- |
| `tools/recomp/` | NE parser, x86-16 decoder, the C generator, the function list (`functions.txt`) and API export tables. |
| `src/recomp/` | Runtime, loader and the host Win16 API layer for `sb_recomp`. |
| `src/recomp/imports/` | The game's import table (API names and ordinals), so the runtime builds without the game. |
| `tests/recomp/` | Input script for the headless test. |
| `build.sh` | One-command build from your game files. |
| `.github/workflows/` | CI (runtime build on Linux and macOS) and releases. |
| `docs/recomp.md` | How the recompiler and host layer work. |
| `original/` | Your copy of the game files (not tracked). |
| `run/` | Saves and settings written by the game (not tracked). |
| `AGENTS.md` | Project rules for contributors and coding agents. |

## Legal

This repository contains tools and new source code only. Spelling Blizzard and
its assets belong to their copyright holders. You need your own copy of the
game to run it.

## Making a release

Push a version tag:

```sh
git tag v0.1.0 && git push origin v0.1.0
```

`.github/workflows/release.yml` builds the runtime on Linux and macOS, then
publishes a release with the source package (`.tar.gz`, `.zip`,
`SHA256SUMS`). Running the workflow by hand from the Actions tab builds the
package as an artifact without publishing it.
