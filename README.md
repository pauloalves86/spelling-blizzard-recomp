# Spelling Blizzard Recomp

<a href="https://recomplauncher.com"><picture><source media="(prefers-color-scheme: dark)" srcset="https://recomplauncher.com/assets/press/launcher/badge-light.svg"><img alt="Get it on RecompLauncher" src="https://recomplauncher.com/assets/press/launcher/badge-dark.svg" height="48"></picture></a>

A project to preserve and decompile **Spelling Blizzard** (Bright Star
Technology, 1994), a Windows 3.x educational spelling game, so it can run on
modern systems without an emulator.

The game is a 16-bit Windows NE executable (`BLIZZARD.EXE`, version
`1.10.CD for Windows`). It loads its art, animation, levels and speech from a
CD resource library (`SB_CDRES.DLL`).

## How to play

You need your own copy of Spelling Blizzard 1.10.CD for Windows: the CD image
(`.iso`, or a `.bin` with its `.cue`), or the folder with `BLIZZARD.EXE` and
`SB_CDRES.DLL`. Nothing else to install: the program recompiles your copy each
time it starts, in about a tenth of a second. It uses the SDL2 and FreeType
libraries, which almost every Linux desktop and the Steam Deck already have.
Linux (x86_64 and arm64) for now.

- **With [RecompLauncher](https://recomplauncher.com):** install it, pick your
  copy, and press Set Up, then Play.
- **By hand:** download `SpellingBlizzard-<version>-linux-x86_64.tar.gz` (or
  `-linux-arm64`) from
  [Releases](https://github.com/pauloalves86/spelling-blizzard-recomp/releases), unpack it, and run

  ```sh
  ./SpellingBlizzard --prepare --game=/path/to/your/copy.iso   # once: checks your copy and keeps it
  ./SpellingBlizzard                                           # play (add --scale 3, --fullscreen, --no-audio)
  ```

  Your copy, saves and logs all go in one folder:
  `~/.local/share/spelling-blizzard-recomp`, or the one given with `--data DIR`.
  Under RecompLauncher it's the game's `recompdata` folder.

## What's here

There are two ways the original program is being brought back:

1. **Static recompilation (`sb_recomp`): plays the real game today.**
   The recompiler in `src/recomp/gen/` translates every function of `BLIZZARD.EXE` from
   x86-16 machine code into C, mechanically. That generated code runs on a
   small host implementation of the Win16 APIs the game uses (KERNEL, USER,
   GDI, MMSYSTEM), built on SDL2 and FreeType. Game logic, drawing, sound and
   input handling are the original code. See [docs/recomp.md](docs/recomp.md).

   `sb_recomp` does this each time it starts: it recompiles your
   `BLIZZARD.EXE` to C and compiles that C in memory with
   [TinyCC](https://bellard.org/tcc/), in about 0.1 seconds. The program
   itself contains no game code, and nothing from the game is written to disk.
2. **Clean decompilation (the end goal).** Readable, hand-written C that
   replaces the recompiled functions one by one, checked against the
   recompiled reference at the same address. Earlier analysis notes and
   hand-written reconstruction were removed from the tree; they're kept in
   git history at the `pre-cleanup` tag.

## You need the original game files

The game files are **not** in this repository or its releases, and neither
is any code built from them: the game is recompiled on your own machine,
when it starts. Building `sb_recomp` doesn't need them; playing does. Put your
own copies here (or let `./build.sh /path/to/game` copy them):

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

`sb_recomp` and the build read these files but never modify them.

## Requirements

To build from source (playing a release needs none of this):

- A C11 compiler (GCC or Clang), CMake 3.16 or newer, and `pkg-config`
- SDL2 development files (SDL2, or sdl2-compat over SDL3)
- FreeType development files
- Liberation fonts, which stand in for the Windows fonts the game asks for

For example:

```sh
# Arch Linux
sudo pacman -S base-devel cmake sdl2 freetype2 ttf-liberation
# Debian / Ubuntu
sudo apt install build-essential cmake pkg-config libsdl2-dev libfreetype-dev fonts-liberation
```

## Build and run

From a clone of the repository with its TinyCC submodule
(`git clone --recursive`, or `git submodule update --init` in an existing
clone), with the requirements above installed:

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

Without the game files, CMake still builds `sb_recomp` (that's what CI
does). With them, it also builds `sb_recomp_aot` for development: the same
game with the recompiled C generated into the build tree and compiled ahead
of time by GCC or Clang, so a debugger sees every function, and the tests.

This opens a 1280×960 window (640×480 at 2×) with sound. Close the window to
quit. Saved players, high scores and the game's INI file go in `run/`.

Useful options:

| Option | Meaning |
| --- | --- |
| `--prepare` | Check your copy and keep it in the data folder, then stop (no window). |
| `--game PATH` | Your copy: a CD image (`.iso`, `.cue`), a folder with the two files, or one of them. |
| `--data DIR` | The data folder for your copy, saves and logs (default `~/.local/share/spelling-blizzard-recomp`). |
| `--scale N` | Window scale factor (default 2). |
| `--no-audio` | Don't open an audio device. |
| `--fullscreen` | Start full screen. |
| `--game-dir DIR` | Development: play the files in `DIR` in place (default `original`). |
| `--data-dir DIR` | Development: writable directory for saves and settings (default `run`). |
| `--headless` | Run without a window, for automated tests. |
| `--quit-after MS` | Exit after this many milliseconds. |
| `--snapshot FILE.ppm` | Save the last frame as an image on exit. |
| `--script FILE` | Play timed input from a file (see [docs/recomp.md](docs/recomp.md)). |
| `--trace-api` | Log every Windows API call to stderr. |
| `--self-test` | Check that compiling in memory works on this machine. |

Without `--game-dir` or `--data-dir`, `sb_recomp` plays the copy kept in the
data folder; if there is none, it plays `original/` with saves in `run/`.

The fonts are looked for in `$SB_FONT_DIR`, then in `fonts/` next to the
program (as in releases), then in the usual system folders.

## Tests

```sh
ctest --test-dir build/cmake --output-on-failure
```

`sb_recomp_new_game_headless` and `sb_recomp_aot_new_game_headless` play
through the opening of the real game without a window, with the code compiled
in memory and ahead of time: credits, the map, New Game, typing a player name,
and starting the first river level. They fail if the game hits anything the
host layer doesn't implement. They need the game files in `original/` (or
`-DSB_GAME_DIR=...`).

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
| `src/recomp/` | Runtime, loader and the host Win16 API layer for `sb_recomp`. |
| `src/recomp/gen/` | The recompiler, in C: NE parser, x86-16 decoder, the C generator, the function list (`functions.txt`) and API export table. |
| `third_party/tinycc/` | [TinyCC](https://repo.or.cz/tinycc.git) (submodule, pinned), which compiles the recompiled C in memory. |
| `src/recomp/imports/` | The game's import table (API names and ordinals), so the runtime builds without the game. |
| `tests/` | The game-files reader's test and the input script for the headless test. |
| `recomplauncher.json` | Setup, settings and saves for [RecompLauncher](https://recomplauncher.com). |
| `build.sh` | One-command build into the repository (`build/release/`), for development. |
| `.github/workflows/` | CI (build and tests on Linux and macOS) and releases. |
| `docs/recomp.md` | How the recompiler and host layer work. |
| `original/` | Your copy of the game files (not tracked). |
| `run/` | Saves and settings written by the game (not tracked). |
| `AGENTS.md` | Project rules for contributors and coding agents. |

## Legal

This repository contains tools and new source code only. Spelling Blizzard and
its assets belong to their copyright holders. You need your own copy of the
game to run it.

`sb_recomp` includes [TinyCC](https://repo.or.cz/tinycc.git)'s libtcc,
under the GNU LGPL 2.1 (`third_party/tinycc/COPYING`). Everything needed to
rebuild `sb_recomp` with a modified libtcc is in this repository.

## Making a release

Push a version tag:

```sh
git tag v0.1.0 && git push origin v0.1.0
```

`.github/workflows/release.yml` runs CI, then builds `sb_recomp` on Ubuntu
22.04 for x86_64 and arm64, runs its tests there (including `--self-test`,
which compiles in memory on that processor), and publishes a release with
`SpellingBlizzard-<version>-linux-x86_64.tar.gz` and `-linux-arm64.tar.gz`,
a `-source.zip` (with TinyCC) and a `.sha256` file. Each package holds the
program as `SpellingBlizzard`, `recomplauncher.json`, the Liberation fonts
and the licenses. None of them contain anything from the game. Running the
workflow by hand from the Actions tab builds the packages as artifacts
without publishing them.
